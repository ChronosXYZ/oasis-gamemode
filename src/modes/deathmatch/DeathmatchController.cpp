#include "DeathmatchController.hpp"
#include "Maps.hpp"
#include "Room.hpp"
#include "WeaponSet.hpp"
#include "../../core/utils/Localization.hpp"
#include "../../core/player/PlayerExtension.hpp"
#include "../../core/utils/Random.hpp"
#include "textdraws/DeathmatchTimer.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <fmt/printf.h>
#include <magic_enum/magic_enum.hpp>
#include <memory>
#include <string>
#include <types.hpp>
#include <player.hpp>
#include <Server/Components/Dialogs/dialogs.hpp>
#include <Server/Components/Timers/Impl/timers_impl.hpp>
#include <unordered_map>
#include <uuid.h>

#include <cstddef>
#include <optional>
#include <vector>

#define PRESSED(newkeys, oldkeys, k) \
	(((newkeys & (k)) == (k)) && ((oldkeys & (k)) != (k)))

#define CBUG_FREEZE_DELAY 1500
#define ROOM_INDEX "roomIndex"

namespace Modes::Deathmatch
{
DeathmatchController::DeathmatchController(
	std::weak_ptr<Core::CoreManager> coreManager,
	IPlayerPool* playerPool,
	ITimersComponent* timersComponent)
	: _coreManager(coreManager)
	, _playerPool(playerPool)
	, _timersComponent(timersComponent)
{
	_playerPool->getPlayerDamageDispatcher().addEventHandler(this);
	_playerPool->getPlayerSpawnDispatcher().addEventHandler(this);
	_playerPool->getPlayerChangeDispatcher().addEventHandler(this);
	_ticker = _timersComponent->create(new Impl::SimpleTimerHandler(
										   std::bind(&DeathmatchController::onTick, this)),
		Milliseconds(1000), true);
}

DeathmatchController::~DeathmatchController()
{
	_playerPool->getPlayerDamageDispatcher().removeEventHandler(this);
	_playerPool->getPlayerSpawnDispatcher().removeEventHandler(this);
	_playerPool->getPlayerChangeDispatcher().removeEventHandler(this);
	_ticker->kill();
}

void DeathmatchController::onModeJoin(IPlayer& player, std::unordered_map<std::string, Core::PrimitiveType> joinData)
{
	auto roomIndex = std::get<std::size_t>(joinData[ROOM_INDEX]);
	this->onRoomJoin(player, roomIndex);
}

void DeathmatchController::onModeSelect(IPlayer& player)
{
	auto isInAnyMode = Core::Player::getPlayerExt(player)->isInAnyMode();
	showRoomSelectionDialog(player, !isInAnyMode);
}

void DeathmatchController::onModeLeave(IPlayer& player)
{
	this->removePlayerFromRoom(player);
}

void DeathmatchController::onPlayerSpawn(IPlayer& player)
{
	auto playerExt = Core::Player::getPlayerExt(player);
	auto pData = Core::Player::getPlayerData(player);
	if (!playerExt->isInMode(Modes::Mode::Deathmatch))
		return;
	auto roomId = pData->tempData->deathmatch->roomId;
	if (!roomId)
		return;
	auto room = this->_rooms[roomId.value()];
	this->setupRoomForPlayer(player, room);
}

void DeathmatchController::onPlayerDeath(IPlayer& player, IPlayer* killer, int reason)
{
	auto playerExt = Core::Player::getPlayerExt(player);
	auto pData = Core::Player::getPlayerData(player);
	if (!playerExt->isInMode(Modes::Mode::Deathmatch))
		return;
	auto roomId = pData->tempData->deathmatch->roomId;
	if (!roomId)
		return;
	auto room = this->_rooms[roomId.value()];
	this->setupSpawn(player, room);
}

void DeathmatchController::onPlayerKeyStateChange(IPlayer& player, uint32_t newKeys, uint32_t oldKeys)
{
	auto playerExt = Core::Player::getPlayerExt(player);
	auto playerData = playerExt->getPlayerData();
	if (!playerExt->isInMode(Modes::Mode::Deathmatch))
		return;

	if (player.getState() != PlayerState_OnFoot)
		return;

	auto roomId = playerData->tempData->deathmatch->roomId;
	if (!roomId)
		return;
	if (this->_rooms.at(*roomId)->cbugEnabled)
		return;
	using namespace std::chrono;
	if (PRESSED(newKeys, oldKeys, Key::FIRE))
	{
		playerData->tempData->deathmatch->lastShootTime = system_clock::to_time_t(system_clock::now());
	}
	else if (PRESSED(newKeys, oldKeys, Key::CROUCH))
	{
		if (auto lastShootTime = playerData->tempData->deathmatch->lastShootTime)
		{
			auto currentTimestamp = system_clock::to_time_t(system_clock::now());
			if (currentTimestamp - *lastShootTime < 1)
			{
				player.setControllable(false);
				playerData->tempData->deathmatch->cbugging = true;
				player.sendGameText(_("Don't use ~r~C-bug!", player), Milliseconds(3000), 5);
				if (auto timerIdOpt = playerData->tempData->deathmatch->cbugFreezeTimerId)
				{
					this->_freezeTimers[*timerIdOpt]->kill();
					this->_freezeTimers.erase(*timerIdOpt);
				}
				auto timerId = uuids::to_string(Utils::getUuidGenerator()());
				auto timerHandler = new Impl::SimpleTimerHandler([&player, playerData, this, timerId]()
					{
						player.setControllable(true);
						playerData->tempData->deathmatch->cbugging = false;
						playerData->tempData->deathmatch->cbugFreezeTimerId.reset();
						this->_freezeTimers.erase(timerId);
					});
				auto timer = _timersComponent->create(timerHandler, Milliseconds(1500), false);
				playerData->tempData->deathmatch->cbugFreezeTimerId = timerId;
				this->_freezeTimers[timerId] = timer;
			}
		}
	}
}

DeathmatchController* DeathmatchController::create(
	std::weak_ptr<Core::CoreManager> coreManager,
	IPlayerPool* playerPool,
	ITimersComponent* timersComponent)
{
	auto controller = new DeathmatchController(coreManager, playerPool, timersComponent);
	controller->initRooms();
	controller->initCommand();
	return controller;
}

void DeathmatchController::initCommand()
{
	this->_coreManager.lock()->getCommandManager()->addCommand(
		"dm", [&](std::reference_wrapper<IPlayer> player, int id)
		{
			auto playerExt = Core::Player::getPlayerExt(player.get());
			auto playerData = Core::Player::getPlayerData(player.get());
			if (id > this->_rooms.size() || id <= 0)
			{
				playerExt->sendErrorMessage(_("Such room doesn't exist!", player));
				return;
			}
			this->_coreManager.lock()->joinMode(player, Mode::Deathmatch, { { ROOM_INDEX, std::size_t(id - 1) } });
		},
		Core::Commands::CommandInfo { .args = { __("room number") }, .description = __("Enter DM room"), .category = MODE_NAME });
	this->_coreManager.lock()->getCommandManager()->addCommand(
		"dm", [&](std::reference_wrapper<IPlayer> player)
		{
			this->_coreManager.lock()->selectMode(player, Mode::Deathmatch);
		},
		Core::Commands::CommandInfo { .args = {}, .description = __("Enter DM mode"), .category = MODE_NAME });
}

void DeathmatchController::initRooms()
{
	this->_rooms = {
		std::shared_ptr<Room>(new Room {
			.map = randomlySelectMap(WeaponSet::Run),
			.allowedWeapons = { PlayerWeapon_Sawedoff, PlayerWeapon_UZI, PlayerWeapon_Colt45 },
			.weaponSet = WeaponSet::Run,
			.host = {},
			.virtualWorld = VIRTUAL_WORLD_PREFIX + 0,
			.cbugEnabled = true,
		}),
		std::shared_ptr<Room>(new Room {
			.map = randomlySelectMap(WeaponSet::Run),
			.allowedWeapons = { PlayerWeapon_Deagle, PlayerWeapon_Shotgun, PlayerWeapon_Sniper },
			.weaponSet = WeaponSet::Walk,
			.host = {},
			.virtualWorld = VIRTUAL_WORLD_PREFIX + 1,
			.cbugEnabled = true,
		}),
		std::shared_ptr<Room>(new Room {
			.map = randomlySelectMap(WeaponSet::Run),
			.allowedWeapons = { PlayerWeapon_Deagle },
			.weaponSet = WeaponSet::DSS,
			.host = {},
			.virtualWorld = VIRTUAL_WORLD_PREFIX + 2,
			.cbugEnabled = false,
		}),
	};
}

void DeathmatchController::showRoomSelectionDialog(IPlayer& player, bool modeSelection)
{
	std::string body = _("#CREAM_BRULEE#Map\t#CREAM_BRULEE#Weapon\t#CREAM_BRULEE#Host\t#CREAM_BRULEE#Players", player);
	body += "\n";
	body += _("Create custom room", player);
	body += "\n";
	for (std::size_t i = 0; i < _rooms.size(); i++)
	{
		auto room = _rooms[i];

		body += fmt::sprintf("{999999}%d. {00FF00}%s\t{00FF00}%s\t{00FF00}%s\t{00FF00}%d\n",
			i + 1,
			room->map.name,
			weaponSetToString(room->weaponSet, player),
			room->host.value_or(_("Server", player)),
			room->playerIds.size());
	}
	this->_coreManager.lock()->getDialogManager()->createDialog(
		player,
		DialogStyle_TABLIST_HEADERS,
		fmt::sprintf(DIALOG_HEADER_TITLE, _("Select Deathmatch room", player)),
		body,
		_("Select", player),
		_("Close", player),
		[&](DialogResponse response, int listItem, StringView input)
		{
			if (response)
			{
				if (listItem == 0)
				{
					// TODO show room creation dialog
					this->showRoomSelectionDialog(player);
					return;
				}
				if (listItem > this->_rooms.size())
				{
					// when player selected a room and this room has been evicted
					this->showRoomSelectionDialog(player);
					return;
				}
				auto roomIndex = std::size_t(listItem - 1);
				auto joinData = std::unordered_map<std::string, Core::PrimitiveType> {
					{ ROOM_INDEX, roomIndex }
				};
				this->_coreManager.lock()->joinMode(player, Modes::Mode::Deathmatch, joinData);
			}
			else
			{
				if (modeSelection)
					this->_coreManager.lock()->showModeSelectionDialog(player);
			}
		});
}

void DeathmatchController::onRoomJoin(IPlayer& player, std::size_t roomId)
{
	auto room = this->_rooms.at(roomId);
	auto pData = Core::Player::getPlayerData(player);
	pData->tempData->deathmatch->roomId = roomId;
	room->playerIds.push_back(player.getID());
	player.setHealth(100.0);
	player.resetWeapons();
	this->setupSpawn(player, room);
	player.spawn();

	auto playerExt = Core::Player::getPlayerExt(player);
	std::shared_ptr<TextDraws::DeathmatchTimer> timer(new TextDraws::DeathmatchTimer(player));
	playerExt->getTextDrawManager()->add(TextDraws::DeathmatchTimer::NAME, timer);
	timer->show();
}

void DeathmatchController::setupSpawn(IPlayer& player, std::shared_ptr<Room> room)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(0, room->map.spawnPoints.size() - 1);

	int spawnPointIndex = dis(rd);
	auto spawnPoint = room->map.spawnPoints[spawnPointIndex];
	auto classData = queryExtension<IPlayerClassData>(player);
	std::vector<WeaponSlotData> slotsVector;
	for (const auto& weapon : room->allowedWeapons)
	{
		slotsVector.push_back(WeaponSlotData(weapon, 9999));
	}
	WeaponSlots weaponSlots;
	std::ranges::copy(slotsVector.begin(), slotsVector.end(), weaponSlots.begin());
	classData->setSpawnInfo(PlayerClass(Core::Player::getPlayerData(player)->lastSkinId,
		TEAM_NONE,
		Vector3(spawnPoint),
		spawnPoint.w,
		weaponSlots));
}

void DeathmatchController::setupRoomForPlayer(IPlayer& player, std::shared_ptr<Room> room)
{
	player.setVirtualWorld(room->virtualWorld);
	player.setInterior(room->map.interiorID);
	player.setCameraBehind();
	player.setArmedWeapon(room->allowedWeapons[0]);
}

void DeathmatchController::removePlayerFromRoom(IPlayer& player)
{
	auto pData = Core::Player::getPlayerData(player);
	if (auto roomId = pData->tempData->deathmatch->roomId)
	{
		std::erase_if(this->_rooms[*roomId]->playerIds, [&](const auto& x)
			{
				return x == player.getID();
			});
		pData->tempData->deathmatch->roomId.reset();
	}
	auto playerExt = Core::Player::getPlayerExt(player);
	playerExt->getTextDrawManager()->destroy(TextDraws::DeathmatchTimer::NAME);
}

void DeathmatchController::onTick()
{
	for (int i = 0; i < this->_rooms.size(); i++)
	{
		auto room = this->_rooms[i];
		for (int j = 0; j < room->playerIds.size(); j++)
		{
			IPlayer* player = _playerPool->get(room->playerIds[j]);
			auto playerExt = Core::Player::getPlayerExt(*player);
			auto txd = std::dynamic_pointer_cast<TextDraws::DeathmatchTimer>(playerExt->getTextDrawManager()->get(TextDraws::DeathmatchTimer::NAME).value());
			// TODO update player dm timer txd
			txd->update(
				fmt::sprintf(_("~w~Mode Deathmatch /DM %d", *player), i + 1),
				0,
				0,
				0.0,
				0);
		}
	}
}
}
