#include "DeathmatchController.hpp"
#include "Maps.hpp"
#include "PlayerVars.hpp"
#include "Room.hpp"
#include "WeaponSet.hpp"
#include "../../core/utils/Localization.hpp"
#include "../../core/player/PlayerExtension.hpp"
#include "../../core/PlayerVars.hpp"
#include "../../core/utils/Random.hpp"

#include <array>
#include <chrono>
#include <fmt/printf.h>
#include <magic_enum/magic_enum.hpp>
#include <types.hpp>
#include <player.hpp>
#include <Server/Components/Dialogs/dialogs.hpp>
#include <Server/Components/Timers/Impl/timers_impl.hpp>
#include <uuid.h>

#include <cstddef>
#include <optional>
#include <vector>

#define PRESSED(newkeys, oldkeys, k) \
	(((newkeys & (k)) == (k)) && ((oldkeys & (k)) != (k)))

#define CBUG_FREEZE_DELAY 1500

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
}

DeathmatchController::~DeathmatchController()
{
	_playerPool->getPlayerDamageDispatcher().removeEventHandler(this);
	_playerPool->getPlayerSpawnDispatcher().removeEventHandler(this);
	_playerPool->getPlayerChangeDispatcher().removeEventHandler(this);
}

void DeathmatchController::onModeJoin(IPlayer& player)
{
	auto playerData = Core::Player::getPlayerData(player);
	if (auto roomId = playerData->getTempData<std::size_t>(PlayerVars::ROOM_ID_TO_JOIN))
	{
		onRoomJoin(player, this->_rooms[*roomId], *roomId);
		playerData->deleteTempData(PlayerVars::ROOM_ID_TO_JOIN);
		return;
	}
	showRoomSelectionDialog(player);
}

void DeathmatchController::onModeLeave(IPlayer& player)
{
	this->removePlayerFromRoom(player);
}

void DeathmatchController::onPlayerSpawn(IPlayer& player)
{
	auto pData = Core::Player::getPlayerData(player);
	auto mode = Core::PlayerVars::getPlayerMode(pData);
	if (mode != Modes::Mode::Deathmatch)
		return;
	std::size_t roomId = pData->getTempData<std::size_t>(PlayerVars::ROOM_ID).value_or(0);
	auto room = this->_rooms[roomId];
	this->setupRoomForPlayer(player, room);
}

void DeathmatchController::onPlayerDeath(IPlayer& player, IPlayer* killer, int reason)
{
	auto pData = Core::Player::getPlayerData(player);
	auto mode = Core::PlayerVars::getPlayerMode(pData);
	if (mode != Modes::Mode::Deathmatch)
		return;
	auto roomId = pData->getTempData<std::size_t>(PlayerVars::ROOM_ID).value_or(0);
	auto room = this->_rooms[roomId];
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

	auto roomIdOpt = playerData->getTempData<std::size_t>(PlayerVars::ROOM_ID);
	if (_rooms[*roomIdOpt]->cbugEnabled)
		return;
	using namespace std::chrono;
	if (PRESSED(newKeys, oldKeys, Key::FIRE))
	{
		playerData->setTempData(PlayerVars::LAST_SHOOT_TIME, system_clock::to_time_t(system_clock::now()));
	}
	else if (PRESSED(newKeys, oldKeys, Key::CROUCH))
	{
		if (auto lastShootTime = playerData->getTempData<std::time_t>(PlayerVars::LAST_SHOOT_TIME))
		{
			auto currentTimestamp = system_clock::to_time_t(system_clock::now());
			if (currentTimestamp - *lastShootTime < 1)
			{
				player.setControllable(false);
				playerData->setTempData(PlayerVars::CBUGGING, true);
				player.sendGameText(_("Don't use ~r~C-bug!", player), Milliseconds(3000), 5);
				if (auto timerIdOpt = playerData->getTempData<std::string>(PlayerVars::CBUG_FREEZE_TIMER_ID))
				{
					this->_freezeTimers[*timerIdOpt]->kill();
					this->_freezeTimers.erase(*timerIdOpt);
				}
				auto timerId = uuids::to_string(Utils::getUuidGenerator()());
				auto timer = _timersComponent->create(new Impl::SimpleTimerHandler([&player, playerData, this, &timerId]()
														  {
															  player.setControllable(true);
															  playerData->deleteTempData(PlayerVars::CBUGGING);
															  playerData->deleteTempData(PlayerVars::CBUG_FREEZE_TIMER_ID);
															  this->_freezeTimers.erase(timerId);
														  }),
					Milliseconds(1500), false);
				playerData->setTempData(PlayerVars::CBUG_FREEZE_TIMER_ID, timerId);
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
			playerData->setTempData(PlayerVars::ROOM_ID_TO_JOIN, std::size_t(id - 1));
			this->_coreManager.lock()->selectMode(player, Mode::Deathmatch);
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

void DeathmatchController::showRoomSelectionDialog(IPlayer& player)
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
				auto roomIndex = listItem - 1;
				auto room = this->_rooms[roomIndex];
				this->onRoomJoin(player, room, roomIndex);
			}
			else
			{
				this->_coreManager.lock()->showModeSelectionDialog(player);
			}
		});
}

void DeathmatchController::onRoomJoin(IPlayer& player, std::shared_ptr<Room> room, std::size_t roomId)
{
	this->removePlayerFromRoom(player);
	auto pData = Core::Player::getPlayerData(player);
	pData->setTempData(PlayerVars::ROOM_ID, roomId);
	room->playerIds.push_back(player.getID());
	player.setHealth(100.0);
	player.resetWeapons();
	this->setupSpawn(player, room);
	player.spawn();
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
	if (auto roomIdOpt = pData->getTempData<std::size_t>(PlayerVars::ROOM_ID))
	{
		std::erase_if(this->_rooms[*roomIdOpt]->playerIds, [&](const auto& x)
			{
				return x == player.getID();
			});
		pData->deleteTempData(PlayerVars::ROOM_ID);
	}
}
}
