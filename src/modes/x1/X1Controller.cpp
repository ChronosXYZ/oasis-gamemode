#include "X1Controller.hpp"
#include "../deathmatch/Maps.hpp"
#include "../../core/player/PlayerExtension.hpp"
#include "../../core/utils/Common.hpp"
#include "../../core/utils/QueryNames.hpp"
#include "../../core/SQLQueryManager.hpp"
#include "X1PlayerTempData.hpp"

#include <bits/chrono.h>
#include <player.hpp>
#include <chrono>
#include <fmt/printf.h>

#include <memory>

namespace Modes::X1
{
void X1Controller::initRooms()
{
	Deathmatch::WeaponSet runWeaponSet(Deathmatch::WeaponSet::Value::Run);
	Deathmatch::WeaponSet dssWeaponSet(Deathmatch::WeaponSet::Value::DSS);

	this->createRoom(
		std::shared_ptr<Room>(new Room { .map = Deathmatch::MAPS.at(0),
			.allowedWeapons = runWeaponSet.getWeapons(),
			.weaponSet = runWeaponSet,
			.defaultArmor = 100.0 }));
	this->createRoom(
		std::shared_ptr<Room>(new Room { .map = Deathmatch::MAPS.at(1),
			.allowedWeapons = runWeaponSet.getWeapons(),
			.weaponSet = runWeaponSet,
			.defaultArmor = 100.0 }));
	this->createRoom(
		std::shared_ptr<Room>(new Room { .map = Deathmatch::MAPS.at(3),
			.allowedWeapons = runWeaponSet.getWeapons(),
			.weaponSet = runWeaponSet,
			.defaultArmor = 100.0 }));
	this->createRoom(
		std::shared_ptr<Room>(new Room { .map = Deathmatch::MAPS.at(11),
			.allowedWeapons = runWeaponSet.getWeapons(),
			.weaponSet = runWeaponSet,
			.defaultArmor = 100.0 }));

	// deagle
	this->createRoom(
		std::shared_ptr<Room>(new Room { .map = Deathmatch::MAPS.at(0),
			.allowedWeapons = dssWeaponSet.getWeapons(),
			.weaponSet = dssWeaponSet,
			.defaultArmor = 100.0 }));
	this->createRoom(
		std::shared_ptr<Room>(new Room { .map = Deathmatch::MAPS.at(1),
			.allowedWeapons = dssWeaponSet.getWeapons(),
			.weaponSet = dssWeaponSet,
			.defaultArmor = 100.0 }));
	this->createRoom(
		std::shared_ptr<Room>(new Room { .map = Deathmatch::MAPS.at(3),
			.allowedWeapons = dssWeaponSet.getWeapons(),
			.weaponSet = dssWeaponSet,
			.defaultArmor = 100.0 }));
	this->createRoom(
		std::shared_ptr<Room>(new Room { .map = Deathmatch::MAPS.at(11),
			.allowedWeapons = dssWeaponSet.getWeapons(),
			.weaponSet = dssWeaponSet,
			.defaultArmor = 100.0 }));
}

void X1Controller::createRoom(std::shared_ptr<Room> room)
{
	room->virtualWorld = this->coreManager.lock()->allocateVirtualWorldId();
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

void X1Controller::logStatsForPlayer(IPlayer& player, bool winner, int weapon)
{
	auto playerData = Core::Player::getPlayerData(player);
	if (winner)
	{
		playerData->x1Stats->kills++;
		playerData->x1Stats->score++;

		switch (Core::Utils::getWeaponType(weapon))
		{
		case Core::Utils::WeaponType::Hand:
		{
			playerData->x1Stats->handKills++;
			break;
		}
		case Core::Utils::WeaponType::HandheldItems:
		{
			playerData->x1Stats->handheldWeaponKills++;
			break;
		}
		case Core::Utils::WeaponType::Melee:
		{
			playerData->x1Stats->meleeKills++;
			break;
		}
		case Core::Utils::WeaponType::Handguns:
		{
			playerData->x1Stats->handgunKills++;
			break;
		}
		case Core::Utils::WeaponType::Shotguns:
		{
			playerData->x1Stats->shotgunKills++;
			break;
		}
		case Core::Utils::WeaponType::SMG:
		{
			playerData->x1Stats->smgKills++;
			break;
		}
		case Core::Utils::WeaponType::AssaultRifles:
		{
			playerData->x1Stats->assaultRiflesKills++;
			break;
		}
		case Core::Utils::WeaponType::Rifles:
		{
			playerData->x1Stats->riflesKills++;
			break;
		}
		case Core::Utils::WeaponType::HeavyWeapons:
		{
			playerData->x1Stats->heavyWeaponKills++;
			break;
		}
		case Core::Utils::WeaponType::Explosives:
		{
			playerData->x1Stats->explosivesKills++;
			break;
		}
		case Core::Utils::WeaponType::Unknown:
			break;
		}
	}
	else
	{
		if (playerData->tempData->x1->subsequentKills
			> playerData->x1Stats->highestKillStreak)
		{
			playerData->x1Stats->highestKillStreak
				= playerData->tempData->x1->subsequentKills;
		}
		playerData->tempData->x1->subsequentKills = 0;
		playerData->x1Stats->deaths++;
		playerData->tempData->x1->endArena = true;
	}
}

void X1Controller::showArenaSelectionDialog(IPlayer& player)
{
	std::vector<std::vector<std::string>> items;
	for (auto [roomId, room] : this->rooms)
	{
		std::string playerCount;
		switch (room->players.size())
		{
		case 0:
		{
			playerCount = "{FFFFFF}0/2";
			break;
		}
		case 1:
		{
			playerCount = "{00FF00}1/2";
			break;
		}
		case 2:
		{
			playerCount = "{FF0000}2/2";
			break;
		}
		default:
		{
			playerCount = fmt::sprintf("{FFFFFF}%d/2", room->players.size());
			break;
		}
		}

		items.push_back({ fmt::sprintf("{999999}%d. {FFFFFF}%s", roomId + 1,
							  room->map.name),
			fmt::sprintf("{FFFFFF}%s", room->weaponSet.toString(player)),
			playerCount });
	}
	auto dialog = std::shared_ptr<Core::TabListHeadersDialog>(
		new Core::TabListHeadersDialog(
			fmt::sprintf(DIALOG_HEADER_TITLE, _("Select Arena", player)),
			{ _("#CREAM_BRULEE#Map", player),
				_("#CREAM_BRULEE#Weapon set", player),
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
						__("This arena is #RED#full#WHITE#!"));
					this->showArenaSelectionDialog(player);
					return;
				}
				this->coreManager.lock()->joinMode(
					player, Mode::X1, { { X1_ROOM_INDEX, roomIndex } });
			}
		});
}

void X1Controller::showX1StatsDialog(IPlayer& player, unsigned int id)
{
	auto anotherPlayer = this->playerPool->get(id);
	auto playerData = Core::Player::getPlayerData(*anotherPlayer);
	auto body = __("#WHITE#- Player:\t\t\t\t\t%s (%d)\n"
				   "- Score:\t\t\t\t\t\t%d\n"
				   "- Highest kill streak:\t\t\t\t%d\n"
				   "- Total kills:\t\t\t\t\t%d\n"
				   "- Total deaths:\t\t\t\t\t%d\n"
				   "- Ratio:\t\t\t\t\t\t%.2f\n"
				   "- Hand kills:\t\t\t\t\t%d\n"
				   "- Handheld weapon kills:\t\t\t%d\n"
				   "- Melee kills:\t\t\t\t\t%d\n"
				   "- Handgun kills:\t\t\t\t\t%d\n"
				   "- Shotgun kills:\t\t\t\t\t%d\n"
				   "- SMG kills:\t\t\t\t\t%d\n"
				   "- Assault rifles kills:\t\t\t\t%d\n"
				   "- Rifles kills:\t\t\t\t\t%d\n"
				   "- Heavy weapon kills:\t\t\t\t%d\n"
				   "- Explosives kills:\t\t\t\t%d");
	int ratioDeaths = playerData->x1Stats->deaths;
	if (ratioDeaths == 0)
		ratioDeaths = 1;
	auto formattedBody = fmt::sprintf(_(body, player),
		anotherPlayer->getName().to_string(), anotherPlayer->getID(),
		playerData->x1Stats->score, playerData->x1Stats->highestKillStreak,
		playerData->x1Stats->kills, playerData->x1Stats->deaths,
		float(playerData->x1Stats->kills) / float(ratioDeaths),
		playerData->x1Stats->handKills,
		playerData->x1Stats->handheldWeaponKills,
		playerData->x1Stats->meleeKills, playerData->x1Stats->handgunKills,
		playerData->x1Stats->shotgunKills, playerData->x1Stats->smgKills,
		playerData->x1Stats->assaultRiflesKills,
		playerData->x1Stats->riflesKills, playerData->x1Stats->heavyWeaponKills,
		playerData->x1Stats->explosivesKills);

	auto dialog = std::shared_ptr<Core::MessageDialog>(new Core::MessageDialog(
		fmt::sprintf(DIALOG_HEADER_TITLE, _("X1 stats", player)), formattedBody,
		_("OK", player), ""));
	this->dialogManager->showDialog(player, dialog,
		[](Core::DialogResult result)
		{
		});
}

void X1Controller::onRoomJoin(IPlayer& player, unsigned int roomId)
{
	auto room = this->rooms.at(roomId);
	auto playerData = Core::Player::getPlayerData(player);
	auto playerExt = Core::Player::getPlayerExt(player);

	playerData->tempData->x1->roomId = roomId;
	room->players.emplace(&player);
	player.setHealth(room->defaultHealth);
	player.setArmour(room->defaultArmor);
	player.resetWeapons();

	this->setRandomSpawnPoint(player, room);
	player.spawn();

	room->sendMessageToAll(__("#LIME#>> #RED#X1#LIGHT_GRAY#: Player "
							  "%s has "
							  "joined the arena"),
		player.getName().to_string());

	if (room->players.size() == 2)
	{
		for (auto player : room->players)
		{
			auto playerExt = Core::Player::getPlayerExt(*player);
			playerExt->showNotification(_("~r~The fight has started!", *player),
				Core::TextDraws::NotificationPosition::Bottom);
			room->fightStarted = std::chrono::system_clock::now();
		}
	}
}

void X1Controller::onPlayerSpawn(IPlayer& player)
{
	auto playerExt = Core::Player::getPlayerExt(player);
	if (!playerExt->isInMode(Modes::Mode::X1))
		return;
	auto pData = Core::Player::getPlayerData(player);
	if (pData->tempData->x1->endArena)
	{
		pData->tempData->x1->endArena = false;
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
	auto playerData = Core::Player::getPlayerData(player);
	auto room = this->rooms.at(playerData->tempData->x1->roomId);
	if (room->players.size() < 2)
		return;

	playerData->tempData->x1->endArena = true;
	this->logStatsForPlayer(player, false, reason);

	IPlayer* loser;
	IPlayer* winner;
	for (auto roomPlayer : room->players)
	{
		if (roomPlayer->getID() == player.getID())
			loser = roomPlayer;
		else
			winner = roomPlayer;
	}

	auto now = std::chrono::system_clock::now();
	auto fightDuration = now - room->fightStarted;

	auto loserPos = loser->getPosition();
	auto winnerPos = winner->getPosition();
	this->bus->fire_event(Core::Utils::Events::X1ArenaWin { .winner = *winner,
		.loser = *loser,
		.armourLeft = winner->getArmour(),
		.healthLeft = winner->getHealth(),
		.weapon = reason,
		.distance = glm::distance(loserPos, winnerPos),
		.fightDuration
		= std::chrono::duration_cast<std::chrono::seconds>(fightDuration) });
	this->logStatsForPlayer(*winner, true, reason);

	auto winnerData = Core::Player::getPlayerData(*winner);
	winnerData->tempData->x1->subsequentKills++;
	this->coreManager.lock()->joinMode(*winner, Mode::Freeroam, {});
}

X1Controller::X1Controller(std::weak_ptr<Core::CoreManager> coreManager,
	std::shared_ptr<Core::Commands::CommandManager> commandManager,
	std::shared_ptr<Core::DialogManager> dialogManager, IPlayerPool* playerPool,
	ITimersComponent* timersComponent, std::shared_ptr<dp::event_bus> bus)
	: super(Mode::X1, bus, playerPool)
	, roomIdPool(std::make_unique<Core::Utils::IDPool>())
	, coreManager(coreManager)
	, commandManager(commandManager)
	, dialogManager(dialogManager)
	, playerPool(playerPool)
	, timersComponent(timersComponent)
{
	this->playerPool->getPlayerSpawnDispatcher().addEventHandler(this);
	this->playerPool->getPlayerDamageDispatcher().addEventHandler(this);

	// re-register PoF event subscription
	this->playerOnFireEventRegistration
		= this->hookEvent<Core::Utils::Events::PlayerOnFireEvent>(
			this->playerOnFireEventRegistration, this,
			&X1Controller::onPlayerOnFire);

	// re-register PoF been killed event subscription
	this->playerOnFireBeenKilledRegistration
		= this->hookEvent<Core::Utils::Events::PlayerOnFireBeenKilled>(
			this->playerOnFireBeenKilledRegistration, this,
			&X1Controller::onPlayerOnFireBeenKilled);
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

	this->commandManager->addCommand(
		"x1stats",
		[this](std::reference_wrapper<IPlayer> player, int id)
		{
			if (id < 0 || id >= this->playerPool->players().size())
			{
				Core::Player::getPlayerExt(player)->sendErrorMessage(
					__("Invalid player ID"));
				return;
			}
			this->showX1StatsDialog(player, id);
		},
		Core::Commands::CommandInfo { .args = { "player id" },
			.description = __("Show X1 stats"),
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

	super::onModeLeave(player);
}

void X1Controller::onPlayerOnFire(Core::Utils::Events::PlayerOnFireEvent event)
{
	if (event.mode != this->mode)
		return;
	auto playerData = Core::Player::getPlayerData(event.player);
	playerData->x1Stats->score += 4;
	super::onPlayerOnFire(event);
}

void X1Controller::onPlayerOnFireBeenKilled(
	Core::Utils::Events::PlayerOnFireBeenKilled event)
{
	if (event.mode != this->mode)
		return;
	auto killerData = Core::Player::getPlayerData(event.killer);
	killerData->x1Stats->score += 4;
	auto killerExt = Core::Player::getPlayerExt(event.killer);
	killerExt->sendInfoMessage(
		__("You killed player on fire and got 4 extra points!"));
	super::onPlayerOnFireBeenKilled(event);
}

void X1Controller::onPlayerLoad(
	std::shared_ptr<Core::PlayerModel> data, pqxx::work& txn)
{
	auto result = txn.exec_params(
		Core::SQLQueryManager::Get()
			->getQueryByName(
				Core::Utils::SQL::Queries::LOAD_X1_STATS_FOR_PLAYER)
			.value(),
		data->userId);
	data->x1Stats->updateFromRow(result[0]);
}

void X1Controller::onPlayerSave(
	std::shared_ptr<Core::PlayerModel> data, pqxx::work& txn)
{
	txn.exec_params(
		Core::SQLQueryManager::Get()
			->getQueryByName(Core::Utils::SQL::Queries::UPDATE_X1_STATS)
			.value(),
		data->x1Stats->score, data->x1Stats->highestKillStreak,
		data->x1Stats->kills, data->x1Stats->deaths, data->x1Stats->handKills,
		data->x1Stats->handheldWeaponKills, data->x1Stats->meleeKills,
		data->x1Stats->handgunKills, data->x1Stats->shotgunKills,
		data->x1Stats->smgKills, data->x1Stats->assaultRiflesKills,
		data->x1Stats->riflesKills, data->x1Stats->heavyWeaponKills,
		data->x1Stats->explosivesKills, data->userId);
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