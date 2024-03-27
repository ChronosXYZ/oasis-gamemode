#include "DeathmatchController.hpp"
#include "Maps.hpp"
#include "PlayerTempData.hpp"
#include "Room.hpp"
#include "WeaponSet.hpp"
#include "../../core/utils/Localization.hpp"
#include "../../core/player/PlayerExtension.hpp"
#include "../../core/utils/Random.hpp"
#include "textdraws/DeathmatchTimer.hpp"
#include "DeathmatchResult.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <fmt/printf.h>
#include <iterator>
#include <magic_enum/magic_enum.hpp>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <thread>
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
#define DEFAULT_ROOM_ROUND_TIME_MIN 1

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
	auto room = this->_rooms[roomId];
	this->setupRoomForPlayer(player, room);
}

void DeathmatchController::onPlayerDeath(IPlayer& player, IPlayer* killer, int reason)
{
	auto playerExt = Core::Player::getPlayerExt(player);
	auto playerData = Core::Player::getPlayerData(player);
	if (!playerExt->isInMode(Modes::Mode::Deathmatch))
		return;

	playerData->tempData->deathmatch->increaseDeaths();

	if (killer)
	{
		auto killerData = Core::Player::getPlayerData(*killer);
		killerData->tempData->deathmatch->increaseKills();
		// TODO add score to player stats
	}

	auto roomId = playerData->tempData->deathmatch->roomId;
	auto room = this->_rooms[roomId];
	this->setRandomSpawnPoint(player, room);
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
	if (this->_rooms.at(roomId)->cbugEnabled)
		return;
	using namespace std::chrono;
	if (PRESSED(newKeys, oldKeys, Key::FIRE))
	{
		playerData->tempData->deathmatch->lastShootTime = system_clock::to_time_t(system_clock::now());
	}
	else if (PRESSED(newKeys, oldKeys, Key::CROUCH))
	{
		auto lastShootTime = playerData->tempData->deathmatch->lastShootTime;
		auto currentTimestamp = system_clock::to_time_t(system_clock::now());
		if (currentTimestamp - lastShootTime < 1)
		{
			player.setControllable(false);
			playerData->tempData->deathmatch->cbugging = true;
			player.sendGameText(_("Don't use ~r~C-bug!", player), Milliseconds(3000), 5);

			if (auto oldTimerId = playerData->tempData->deathmatch->cbugFreezeTimerId)
			{
				this->_cbugFreezeTimers[oldTimerId.value()]->kill();
				this->_cbugFreezeTimers.erase(oldTimerId.value());
			}

			auto timerId = uuids::to_string(Utils::getUuidGenerator()());
			auto timerHandler = new Impl::SimpleTimerHandler([&player, playerData, this, timerId]()
				{
					player.setControllable(true);
					playerData->tempData->deathmatch->cbugging = false;
					playerData->tempData->deathmatch->cbugFreezeTimerId.reset();
					this->_cbugFreezeTimers.erase(timerId);
				});
			auto timer = _timersComponent->create(timerHandler, Milliseconds(1500), false);
			playerData->tempData->deathmatch->cbugFreezeTimerId = timerId;
			this->_cbugFreezeTimers[timerId] = timer;
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
			.countdown = std::chrono::minutes(DEFAULT_ROOM_ROUND_TIME_MIN),
			.defaultTime = std::chrono::minutes(DEFAULT_ROOM_ROUND_TIME_MIN),
		}),
		std::shared_ptr<Room>(new Room {
			.map = randomlySelectMap(WeaponSet::Walk),
			.allowedWeapons = { PlayerWeapon_Deagle, PlayerWeapon_Shotgun, PlayerWeapon_Sniper },
			.weaponSet = WeaponSet::Walk,
			.host = {},
			.virtualWorld = VIRTUAL_WORLD_PREFIX + 1,
			.cbugEnabled = true,
			.countdown = std::chrono::minutes(DEFAULT_ROOM_ROUND_TIME_MIN),
			.defaultTime = std::chrono::minutes(DEFAULT_ROOM_ROUND_TIME_MIN),
		}),
		std::shared_ptr<Room>(new Room {
			.map = randomlySelectMap(WeaponSet::Walk),
			.allowedWeapons = { PlayerWeapon_Deagle },
			.weaponSet = WeaponSet::DSS,
			.host = {},
			.virtualWorld = VIRTUAL_WORLD_PREFIX + 2,
			.cbugEnabled = false,
			.countdown = std::chrono::minutes(DEFAULT_ROOM_ROUND_TIME_MIN),
			.defaultTime = std::chrono::minutes(DEFAULT_ROOM_ROUND_TIME_MIN),
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
		[modeSelection, this, &player](DialogResponse response, int listItem, StringView input)
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

void DeathmatchController::showRoundResultDialog(IPlayer& player, std::shared_ptr<Room> room)
{
	if (!room->cachedLastResult)
	{
		spdlog::warn("last result cache of room is poisoned!");
		return;
	}
	std::string header = _("Player\tK : D\tRatio\n", player);
	this->_coreManager.lock()->getDialogManager()->createDialog(
		player,
		DialogStyle_TABLIST_HEADERS,
		fmt::sprintf(DIALOG_HEADER_TITLE, _("Deathmatch statistics", player)),
		header + room->cachedLastResult.value(),
		_("Close", player),
		"",
		[this, &player, room](DialogResponse response, int listitem, StringView inputText)
		{
			if (room->isRestarting)
			{
				this->showRoundResultDialog(player, room);
			}
		});
}

std::shared_ptr<TextDraws::DeathmatchTimer> DeathmatchController::createDeathmatchTimer(IPlayer& player)
{
	auto playerExt = Core::Player::getPlayerExt(player);
	std::shared_ptr<TextDraws::DeathmatchTimer> timer(new TextDraws::DeathmatchTimer(player));
	playerExt->getTextDrawManager()->add(TextDraws::DeathmatchTimer::NAME, timer);
	return timer;
}

std::optional<std::shared_ptr<TextDraws::DeathmatchTimer>> DeathmatchController::getDeathmatchTimer(IPlayer& player)
{
	auto playerExt = Core::Player::getPlayerExt(player);
	auto deathmatchTimerTxdAbstract = playerExt->getTextDrawManager()->get(TextDraws::DeathmatchTimer::NAME);
	if (!deathmatchTimerTxdAbstract.has_value())
	{
		spdlog::warn("player {} doesn't have deathmatch timer textdraw, but he is in dm room",
			player.getName().to_string());
		return {};
	}
	return std::dynamic_pointer_cast<TextDraws::DeathmatchTimer>(deathmatchTimerTxdAbstract.value());
}

void DeathmatchController::updateDeathmatchTimer(IPlayer& player, std::size_t roomIndex, std::shared_ptr<Room> room)
{
	auto playerData = Core::Player::getPlayerData(player);
	auto deathmatchTimerTxd = this->getDeathmatchTimer(player);
	if (!deathmatchTimerTxd.has_value())
		return;
	deathmatchTimerTxd.value()->update(
		fmt::sprintf(_("~w~Mode Deathmatch /DM %d", player), roomIndex + 1),
		playerData->tempData->deathmatch->kills,
		playerData->tempData->deathmatch->deaths,
		0.0,
		room->countdown);
}

void DeathmatchController::onRoomJoin(IPlayer& player, std::size_t roomId)
{
	auto room = this->_rooms.at(roomId);
	auto playerData = Core::Player::getPlayerData(player);

	playerData->tempData->deathmatch = std::make_unique<PlayerTempData>();
	playerData->tempData->deathmatch->roomId = roomId;
	room->playerIds.push_back(player.getID());
	player.setHealth(100.0);
	player.resetWeapons();

	auto timer = this->createDeathmatchTimer(player);
	if (room->isRestarting)
	{
		player.setSpectating(true);
		this->showRoundResultDialog(player, room);
	}
	else
	{
		timer->show();
		this->setRandomSpawnPoint(player, room);
		player.spawn();
	}
}

void DeathmatchController::onNewRound(std::shared_ptr<Room> room)
{
	auto weaponSet = room->weaponSet;
	if (room->weaponSet == WeaponSet::DSS)
		weaponSet = WeaponSet::Walk;

	room->map = randomlySelectMap(weaponSet);
	room->countdown = room->defaultTime;
	room->isRestarting = false;
	room->isStarting = true;

	for (auto const& playerId : room->playerIds)
	{
		IPlayer* player = this->_playerPool->get(playerId);
		this->setRandomSpawnPoint(*player, room);
		player->setSpectating(false);
		auto playerData = Core::Player::getPlayerData(*player);
		playerData->tempData->deathmatch->resetKD();
		if (auto timer = this->getDeathmatchTimer(*player))
			timer.value()->show();
	}

	// TODO replace thread with coroutine
	std::thread([room, this]()
		{
			for (int i = 3; i > 0; i--)
			{
				for (auto const& playerId : room->playerIds)
				{
					IPlayer* player = this->_playerPool->get(playerId);
					player->setControllable(false);
					player->sendGameText(fmt::sprintf("~w~%d", i), Seconds(1), 6);
				}
				using namespace std::chrono_literals;
				std::this_thread::sleep_for(1s);
			}
			for (auto const& playerId : room->playerIds)
			{
				IPlayer* player = this->_playerPool->get(playerId);
				player->sendGameText(_("~g~~h~~h~GO!", *player), Seconds(1), 6);
				player->setControllable(true);
			}
			room->cachedLastResult = {};
			room->isStarting = false;
		})
		.detach();
}

void DeathmatchController::onRoundEnd(std::shared_ptr<Room> room)
{
	room->isRestarting = true;
	std::vector<DeathmatchResult> resultArray;
	for (auto& playerId : room->playerIds)
	{
		auto player = this->_playerPool->get(playerId);
		auto playerData = Core::Player::getPlayerData(*player);
		resultArray.push_back(DeathmatchResult {
			.playerName = player->getName().to_string(),
			.kills = playerData->tempData->deathmatch->kills,
			.deaths = playerData->tempData->deathmatch->deaths,
			.ratio = playerData->tempData->deathmatch->ratio,
		});
	}
	std::sort(resultArray.begin(), resultArray.end(), [](DeathmatchResult x1, DeathmatchResult x2)
		{
			return x1.kills > x2.kills;
		});

	std::string lastResults;
	for (std::size_t i = 0; i < resultArray.size(); i++)
	{
		auto playerResult = resultArray.at(i);
		lastResults += fmt::sprintf("%d. %s\t%d : %d\t%.2f\n",
			i,
			playerResult.playerName,
			playerResult.kills,
			playerResult.deaths,
			playerResult.ratio);
	}
	room->cachedLastResult = lastResults;

	for (auto& playerId : room->playerIds)
	{
		auto player = this->_playerPool->get(playerId);
		player->setControllable(false);
		player->setSpectating(true);
		this->showRoundResultDialog(*player, room);
		if (auto timer = this->getDeathmatchTimer(*player))
		{
			timer.value()->hide();
		}
	}

	_timersComponent->create(
		new Impl::SimpleTimerHandler(std::bind(&DeathmatchController::onNewRound, this, room)),
		Seconds(5),
		false);
}

void DeathmatchController::setRandomSpawnPoint(IPlayer& player, std::shared_ptr<Room> room)
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
	std::erase_if(this->_rooms[pData->tempData->deathmatch->roomId]->playerIds, [&](const auto& x)
		{
			return x == player.getID();
		});

	pData->tempData->deathmatch.reset();

	auto playerExt = Core::Player::getPlayerExt(player);
	playerExt->getTextDrawManager()->destroy(TextDraws::DeathmatchTimer::NAME);
}

void DeathmatchController::onTick()
{
	for (int i = 0; i < this->_rooms.size(); i++)
	{
		auto room = this->_rooms[i];
		if (room->isRestarting || room->isStarting)
			continue;
		if (room->countdown == std::chrono::seconds(0))
		{
			this->onRoundEnd(room);
		}
		if (!room->playerIds.empty() && room->countdown > std::chrono::seconds(0) && !room->isStarting)
		{
			room->countdown--;
		}
		for (int j = 0; j < room->playerIds.size(); j++)
		{
			IPlayer* player = _playerPool->get(room->playerIds[j]);
			this->updateDeathmatchTimer(*player, i, room);
		}
	}
}
}
