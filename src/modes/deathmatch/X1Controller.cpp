#include "X1Controller.hpp"
#include "Maps.hpp"
#include "Room.hpp"
#include "Room.tpp"
#include "../../core/player/PlayerExtension.hpp"
#include "X1PlayerTempData.hpp"

#include <chrono>
#include <fmt/printf.h>

#include <memory>

namespace Modes::Deathmatch
{

void X1Controller::initRooms()
{
	WeaponSet runWeaponSet(WeaponSet::Value::Run);
	this->createRoom(std::shared_ptr<Room>(new Room { .map = MAPS.at(0),
		.allowedWeapons = runWeaponSet.getWeapons(),
		.weaponSet = runWeaponSet,
		.virtualWorld = X1_VIRTUAL_WORLD_PREFIX + 0,
		.defaultArmor = 100.0 }));
	this->createRoom(std::shared_ptr<Room>(new Room { .map = MAPS.at(1),
		.allowedWeapons = runWeaponSet.getWeapons(),
		.weaponSet = runWeaponSet,
		.virtualWorld = X1_VIRTUAL_WORLD_PREFIX + 1,
		.defaultArmor = 100.0 }));
	this->createRoom(std::shared_ptr<Room>(new Room { .map = MAPS.at(3),
		.allowedWeapons = runWeaponSet.getWeapons(),
		.weaponSet = runWeaponSet,
		.virtualWorld = X1_VIRTUAL_WORLD_PREFIX + 2,
		.defaultArmor = 100.0 }));
	this->createRoom(std::shared_ptr<Room>(new Room { .map = MAPS.at(11),
		.allowedWeapons = runWeaponSet.getWeapons(),
		.weaponSet = runWeaponSet,
		.virtualWorld = X1_VIRTUAL_WORLD_PREFIX + 3,
		.defaultArmor = 100.0 }));
}

void X1Controller::createRoom(std::shared_ptr<Room> room)
{
	room->virtualWorld = X1_VIRTUAL_WORLD_PREFIX + this->rooms.size();
	auto roomId = this->roomIdPool->allocateId();
	this->rooms[roomId] = room;
}

void X1Controller::setRandomSpawnPoint(
	IPlayer& player, std::shared_ptr<Room> room)
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
	std::ranges::copy(
		slotsVector.begin(), slotsVector.end(), weaponSlots.begin());
	classData->setSpawnInfo(
		PlayerClass(Core::Player::getPlayerData(player)->lastSkinId, TEAM_NONE,
			Vector3(spawnPoint), spawnPoint.w, weaponSlots));
}

void X1Controller::setupRoomForPlayer(
	IPlayer& player, std::shared_ptr<Room> room)
{
	player.setArmour(room->defaultArmor);
	player.setVirtualWorld(room->virtualWorld);
	player.setInterior(room->map.interiorID);
	player.setCameraBehind();
	player.setArmedWeapon(room->allowedWeapons[0]);
}

void X1Controller::showArenaSelectionDialog(IPlayer& player)
{
	std::vector<std::vector<std::string>> items;
	for (auto [roomId, room] : this->rooms)
	{
		items.push_back({ fmt::sprintf("{999999}%d. {FFFFFF}%s", roomId + 1,
							  room->map.name),
			room->players.size() < 2
				? fmt::sprintf("{00FF00}%d/2", room->players.size())
				: fmt::sprintf("{FF0000}2/2") });
	}
	auto dialog = std::shared_ptr<Core::TabListHeadersDialog>(
		new Core::TabListHeadersDialog(
			fmt::sprintf(DIALOG_HEADER_TITLE, _("Select Arena", player)),
			{ _("#CREAM_BRULEE#Map", player),
				_("#CREAM_BRULEE#Players", player) },

			items, _("Select", player), _("Close", player)));

	this->dialogManager->showDialog(player, dialog,
		[this, &player](Core::DialogResult result)
		{
			if (result.response())
			{
				auto playerExt = Core::Player::getPlayerExt(player);
				auto roomIndex = (unsigned int)result.listItem();
				auto room = this->rooms[roomIndex];
				if (room->players.size() == 2)
				{
					playerExt->sendErrorMessage(
						_("This arena is #RED#full#WHITE#!", player));
					return;
				}
				this->coreManager.lock()->joinMode(
					player, Mode::X1, { { X1_ROOM_INDEX, roomIndex } });
			}
		});
}

void X1Controller::onRoomJoin(IPlayer& player, unsigned int roomId)
{
	auto room = this->rooms.at(roomId);
	auto playerData = Core::Player::getPlayerData(player);
	auto playerExt = Core::Player::getPlayerExt(player);

	playerData->tempData->x1 = std::make_unique<X1PlayerTempData>();
	playerData->tempData->x1->roomId = roomId;
	room->players.emplace(&player);
	player.setHealth(room->defaultHealth);
	player.setArmour(room->defaultArmor);
	player.resetWeapons();

	this->setRandomSpawnPoint(player, room);
	player.spawn();

	room->sendMessageToAll(__("#LIME#>> #RED#X1#LIGHT_GRAY#: Player "
							  "%s has "
							  "joined the arena!"),
		player.getName().to_string());
}

void X1Controller::onPlayerSpawn(IPlayer& player)
{
	auto playerExt = Core::Player::getPlayerExt(player);
	if (!playerExt->isInMode(Modes::Mode::X1))
		return;
	auto pData = Core::Player::getPlayerData(player);
	if (pData->tempData->x1->defeated)
	{
		this->coreManager.lock()->joinMode(player, Mode::Freeroam, {});
		return;
	}
	auto roomId = pData->tempData->x1->roomId;
	auto room = this->rooms.at(roomId);
	this->setupRoomForPlayer(player, room);
}

void X1Controller::onPlayerDeath(IPlayer& player, IPlayer* killer, int reason)
{
	auto playerExt = Core::Player::getPlayerExt(player);
	if (!playerExt->isInMode(Mode::X1))
		return;
	// TODO scoring
	this->bus->fire_event(Core::Utils::Events::X1ArenaWin { .winner = *killer,
		.loser = player,
		.armourLeft = killer->getArmour(),
		.healthLeft = killer->getHealth(),
		.fightDuration = std::chrono::seconds(0) });
	auto playerData = Core::Player::getPlayerData(player);
	playerData->tempData->x1->defeated = true;
}

X1Controller::X1Controller(std::weak_ptr<Core::CoreManager> coreManager,
	std::shared_ptr<Core::Commands::CommandManager> commandManager,
	std::shared_ptr<Core::DialogManager> dialogManager, IPlayerPool* playerPool,
	ITimersComponent* timersComponent, std::shared_ptr<dp::event_bus> bus)
	: super(Mode::X1, bus)
	, roomIdPool(std::make_unique<Core::Utils::IDPool>())
	, coreManager(coreManager)
	, commandManager(commandManager)
	, dialogManager(dialogManager)
	, playerPool(playerPool)
	, timersComponent(timersComponent)
{
	this->bus->remove_handler(this->playerOnFireEventRegistration);
	this->playerPool->getPlayerSpawnDispatcher().addEventHandler(this);
	this->playerPool->getPlayerDamageDispatcher().addEventHandler(this);
}

X1Controller::~X1Controller()
{
	this->playerPool->getPlayerSpawnDispatcher().removeEventHandler(this);
	this->playerPool->getPlayerDamageDispatcher().removeEventHandler(this);
}

void X1Controller::initCommands()
{
	this->commandManager->addCommand(
		"x1",
		[&](std::reference_wrapper<IPlayer> player)
		{
			this->coreManager.lock()->selectMode(player, Mode::X1);
		},
		Core::Commands::CommandInfo { .args = {},
			.description = __("Enter Arena"),
			.category = X1_MODE_NAME });
}

void X1Controller::onModeSelect(IPlayer& player)
{
	this->showArenaSelectionDialog(player);
}

void X1Controller::onModeJoin(IPlayer& player, JoinData joinData)
{
	super::onModeJoin(player, joinData);
	auto roomIndex = std::get<unsigned int>(joinData[X1_ROOM_INDEX]);
	this->onRoomJoin(player, roomIndex);
}

void X1Controller::onModeLeave(IPlayer& player)
{
	auto pData = Core::Player::getPlayerData(player);
	auto roomId = pData->tempData->x1->roomId;
	auto room = this->rooms.at(roomId);
	room->players.erase(&player);

	pData->tempData->x1.reset();

	super::onModeLeave(player);
}

X1Controller* X1Controller::create(std::weak_ptr<Core::CoreManager> coreManager,
	std::shared_ptr<Core::Commands::CommandManager> commandManager,
	std::shared_ptr<Core::DialogManager> dialogManager, IPlayerPool* playerPool,
	ITimersComponent* timersComponent, std::shared_ptr<dp::event_bus> bus)
{
	auto controller = new X1Controller(coreManager, commandManager,
		dialogManager, playerPool, timersComponent, bus);
	controller->initRooms();
	controller->initCommands();
	return controller;
}
}