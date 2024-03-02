#include "DeathmatchController.hpp"
#include "Maps.hpp"
#include "PlayerVars.hpp"
#include "Room.hpp"
#include "Server/Components/Dialogs/dialogs.hpp"
#include "WeaponSet.hpp"
#include "../../core/utils/Localization.hpp"
#include "../../core/player/PlayerExtension.hpp"
#include "../../core/PlayerVars.hpp"

#include <array>
#include <fmt/printf.h>
#include <magic_enum/magic_enum.hpp>
#include <types.hpp>
#include <player.hpp>

#include <cstddef>
#include <optional>
#include <vector>

namespace Modes::Deathmatch
{
DeathmatchController::DeathmatchController(
	std::weak_ptr<Core::CoreManager> coreManager,
	IPlayerPool* playerPool)
	: _coreManager(coreManager)
	, _playerPool(playerPool)
{
	_playerPool->getPlayerDamageDispatcher().addEventHandler(this);
	_playerPool->getPlayerSpawnDispatcher().addEventHandler(this);
}

DeathmatchController::~DeathmatchController()
{
	_playerPool->getPlayerDamageDispatcher().removeEventHandler(this);
	_playerPool->getPlayerSpawnDispatcher().removeEventHandler(this);
}

void DeathmatchController::onModeJoin(IPlayer& player)
{
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
	std::size_t roomId = std::get<std::size_t>(pData->getTempData(PlayerVars::ROOM_ID).value_or(0));
	auto room = this->_rooms[roomId];
	this->setupRoomForPlayer(player, room);
}

void DeathmatchController::onPlayerDeath(IPlayer& player, IPlayer* killer, int reason)
{
	auto pData = Core::Player::getPlayerData(player);
	auto mode = Core::PlayerVars::getPlayerMode(pData);
	if (mode != Modes::Mode::Deathmatch)
		return;
	std::size_t roomId = std::get<std::size_t>(pData->getTempData(PlayerVars::ROOM_ID).value_or(0));
	auto room = this->_rooms[roomId];
	this->setupSpawn(player, room);
}

DeathmatchController* DeathmatchController::create(std::weak_ptr<Core::CoreManager> coreManager, IPlayerPool* playerPool)
{
	auto controller = new DeathmatchController(coreManager, playerPool);
	controller->initRooms();
	controller->initCommand();
	return controller;
}

void DeathmatchController::initCommand()
{
	// init DM-specific commands
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
		}),
		std::shared_ptr<Room>(new Room {
			.map = randomlySelectMap(WeaponSet::Run),
			.allowedWeapons = { PlayerWeapon_Deagle, PlayerWeapon_Shotgun, PlayerWeapon_Sniper },
			.weaponSet = WeaponSet::Walk,
			.host = {},
			.virtualWorld = VIRTUAL_WORLD_PREFIX + 1,
		}),
		std::shared_ptr<Room>(new Room {
			.map = randomlySelectMap(WeaponSet::Run),
			.allowedWeapons = { PlayerWeapon_Deagle },
			.weaponSet = WeaponSet::DSS,
			.host = {},
			.virtualWorld = VIRTUAL_WORLD_PREFIX + 2,
		}),
	};
}

void DeathmatchController::showRoomSelectionDialog(IPlayer& player)
{
	std::string body = _("Map\tWeapon\tHost\tPlayers\n", player);
	body += _("Create custom room\n", player);
	for (std::size_t i = 0; i < _rooms.size(); i++)
	{
		auto room = _rooms[i];

		body += fmt::sprintf("%d. %s\t%s\t%s\t%d\n",
			i + 1,
			room->map.name,
			weaponSetToString(room->weaponSet, player),
			room->host.value_or(_("Server", player)),
			room->playerCount);
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
	room->playerCount++;
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
	if (auto roomIdOpt = pData->getTempData(PlayerVars::ROOM_ID))
	{
		auto roomId = std::get<std::size_t>(*roomIdOpt);
		this->_rooms[roomId]->playerCount--;
		pData->deleteTempData(PlayerVars::ROOM_ID);
	}
}
}
