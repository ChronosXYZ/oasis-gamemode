#include "DuelController.hpp"
#include "../deathmatch/Maps.hpp"
#include "../../core/player/PlayerExtension.hpp"
#include "../../core/utils/Common.hpp"
#include "../../core/utils/QueryNames.hpp"
#include "../../core/SQLQueryManager.hpp"
#include "DuelOffer.hpp"
#include "PlayerTempData.hpp"
#include "events.hpp"
#include "values.hpp"

#include <bits/chrono.h>
#include <functional>
#include <magic_enum/magic_enum.hpp>
#include <player.hpp>
#include <chrono>
#include <fmt/printf.h>

#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

namespace Modes::Duel
{

// TODO implement duel accepting
void DuelController::createDuel(IPlayer& player, int id)
{
	auto playerExt = Core::Player::getPlayerExt(player);
	auto receivingPlayer = this->playerPool->get(id);
	if (!receivingPlayer || id == player.getID())
	{
		playerExt->sendErrorMessage(_("Invalid player id", player));
		return;
	}

	auto receivingPlayerExt = Core::Player::getPlayerExt(*receivingPlayer);
	if (!receivingPlayerExt->isAuthorized())
	{
		playerExt->sendErrorMessage(__("Player has not been authorized"));
		return;
	}
	auto duelOffersSent
		= receivingPlayerExt->getPlayerData()->tempData->core->duelOffersSent;
	if (auto it = std::find_if(duelOffersSent.begin(), duelOffersSent.end(),
			[&player](std::shared_ptr<DuelOffer> x)
			{
				return x->from->getID() == player.getID();
			});
		it != duelOffersSent.end())
	{
		playerExt->sendErrorMessage(__("You already sent him a duel offer!"));
		return;
	}

	auto playerData = Core::Player::getPlayerData(player);
	auto defaultWeaponSet
		= Deathmatch::WeaponSet(Deathmatch::WeaponSet::Value::Run);
	auto weaponsArray = defaultWeaponSet.getWeapons();
	std::vector<PlayerWeapon> weapons(weaponsArray.begin(), weaponsArray.end());
	playerData->tempData->core->temporaryDuelSettings
		= std::shared_ptr<DuelOffer>(new DuelOffer {
			.map = Deathmatch::MAPS[0],
			.weapons = weapons,
			.roundCount = 1,
			.defaultHealth = 100.0,
			.defaultArmour = 100.0,
			.from = &player,
			.to = receivingPlayer,
		});
	this->showDuelCreationDialog(player);
}

void DuelController::setRandomSpawnPoint(
	IPlayer& player, std::shared_ptr<Room> room)
{
	auto spawnPoint = room->map.getRandomSpawn();
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

void DuelController::setupRoomForPlayer(
	IPlayer& player, std::shared_ptr<Room> room)
{
	player.setHealth(room->defaultHealth);
	player.setArmour(room->defaultArmor);
	player.setVirtualWorld(room->virtualWorld);
	player.setInterior(room->map.interiorID);
	player.setCameraBehind();
	player.setArmedWeapon(room->allowedWeapons[0]);
}

void DuelController::logStatsForPlayer(IPlayer& player, bool winner, int weapon)
{
	auto playerData = Core::Player::getPlayerData(player);
	if (winner)
	{
		playerData->duelStats->kills++;
		playerData->duelStats->score++;

		switch (Core::Utils::getWeaponType(weapon))
		{
		case Core::Utils::WeaponType::Hand:
		{
			playerData->duelStats->handKills++;
			break;
		}
		case Core::Utils::WeaponType::HandheldItems:
		{
			playerData->duelStats->handheldWeaponKills++;
			break;
		}
		case Core::Utils::WeaponType::Melee:
		{
			playerData->duelStats->meleeKills++;
			break;
		}
		case Core::Utils::WeaponType::Handguns:
		{
			playerData->duelStats->handgunKills++;
			break;
		}
		case Core::Utils::WeaponType::Shotguns:
		{
			playerData->duelStats->shotgunKills++;
			break;
		}
		case Core::Utils::WeaponType::SMG:
		{
			playerData->duelStats->smgKills++;
			break;
		}
		case Core::Utils::WeaponType::AssaultRifles:
		{
			playerData->duelStats->assaultRiflesKills++;
			break;
		}
		case Core::Utils::WeaponType::Rifles:
		{
			playerData->duelStats->riflesKills++;
			break;
		}
		case Core::Utils::WeaponType::HeavyWeapons:
		{
			playerData->duelStats->heavyWeaponKills++;
			break;
		}
		case Core::Utils::WeaponType::Explosives:
		{
			playerData->duelStats->explosivesKills++;
			break;
		}
		case Core::Utils::WeaponType::Unknown:
			break;
		}
	}
	else
	{
		if (playerData->tempData->duel->subsequentKills
			> playerData->duelStats->highestKillStreak)
		{
			playerData->duelStats->highestKillStreak
				= playerData->tempData->duel->subsequentKills;
		}
		playerData->tempData->duel->subsequentKills = 0;
		playerData->duelStats->deaths++;
	}
}

void DuelController::createDuelOffer(IPlayer& player)
{
	auto playerData = Core::Player::getPlayerData(player);
	auto playerExt = Core::Player::getPlayerExt(player);
	if (!playerData->tempData->core->temporaryDuelSettings)
		return;
	auto tempDuelSettings
		= playerData->tempData->core->temporaryDuelSettings.value();
	if (tempDuelSettings->to->getID() == -1)
	{
		playerExt->sendErrorMessage(
			_("The player you want to call for a duel has disconnected!",
				player));
		return;
	}
	playerData->tempData->core->duelOffersSent.push_back(tempDuelSettings);
	playerData->tempData->core->temporaryDuelSettings.reset();
	auto receivingPlayerData
		= Core::Player::getPlayerData(*tempDuelSettings->to);
	auto receivingPlayerExt = Core::Player::getPlayerExt(*tempDuelSettings->to);
	receivingPlayerData->tempData->core->duelOffersReceived.push_back(
		tempDuelSettings);
	receivingPlayerExt->sendInfoMessage(
		__("DUEL: {%06x}%s(%d) #WHITE#sent you a duel offer, use "
		   "/duela to accept it"),
		player.getColour().RGBA() >> 8, player.getName().to_string(),
		player.getID());
	playerExt->sendInfoMessage(
		__("DUEL: You have created duel offer, wait for an answer"));
}

void DuelController::showDuelStatsDialog(IPlayer& player, unsigned int id)
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
	int ratioDeaths = playerData->duelStats->deaths;
	if (ratioDeaths == 0)
		ratioDeaths = 1;
	auto formattedBody = fmt::sprintf(_(body, player),
		anotherPlayer->getName().to_string(), anotherPlayer->getID(),
		playerData->duelStats->score, playerData->x1Stats->highestKillStreak,
		playerData->duelStats->kills, playerData->x1Stats->deaths,
		float(playerData->duelStats->kills) / float(ratioDeaths),
		playerData->duelStats->handKills,
		playerData->duelStats->handheldWeaponKills,
		playerData->duelStats->meleeKills, playerData->x1Stats->handgunKills,
		playerData->duelStats->shotgunKills, playerData->x1Stats->smgKills,
		playerData->duelStats->assaultRiflesKills,
		playerData->duelStats->riflesKills,
		playerData->x1Stats->heavyWeaponKills,
		playerData->duelStats->explosivesKills);

	auto dialog = std::shared_ptr<Core::MessageDialog>(new Core::MessageDialog(
		fmt::sprintf(DIALOG_HEADER_TITLE, _("Duel stats", player)),
		formattedBody, _("OK", player), ""));
	this->dialogManager->showDialog(player, dialog,
		[](Core::DialogResult result)
		{
		});
}

void DuelController::showDuelCreationDialog(IPlayer& player)
{
	auto playerData = Core::Player::getPlayerData(player);
	// playerData->tempData->core->temporaryDuelSettings
	// 	= std::shared_ptr<DuelOffer>(
	// 		new DuelOffer { .from = player, .to = *playerPool->get(id) });
	if (!playerData->tempData->core->temporaryDuelSettings)
		return;
	auto tempDuelSettings
		= playerData->tempData->core->temporaryDuelSettings.value();
	auto dialog = std::shared_ptr<Core::TabListDialog>(new Core::TabListDialog(
		fmt::sprintf(DIALOG_HEADER_TITLE, _("Create duel", player)),
		{ { _("Map", player), tempDuelSettings->map.name },
			{ _("Round count", player),
				std::to_string(tempDuelSettings->roundCount) },
			{ _("Weapon set", player),
				tempDuelSettings->weaponSet.toString(player) },
			{ _("Confirm", player) } },
		_("Select", player), _("Cancel", player)));
	this->dialogManager->showDialog(player, dialog,
		[&player, playerData, this](Core::DialogResult result)
		{
			if (result.response())
			{
				switch (result.listItem())
				{
				case 0:
				{
					this->showDuelMapSelectionDialog(player);
					break;
				}
				case 1:
				{
					this->showDuelRoundCountSettingDialog(player);
					break;
				}
				case 2:
				{
					this->showDuelWeaponSelectionDialog(player);
					break;
				}
				case 3:
				{
					this->createDuelOffer(player);
					break;
				}
				}
			}
			else
			{
				playerData->tempData->core->temporaryDuelSettings->reset();
			}
		});
}

void DuelController::showDuelMapSelectionDialog(IPlayer& player)
{
	auto playerData = Core::Player::getPlayerData(player);
	if (!playerData->tempData->core->temporaryDuelSettings)
		return;

	std::vector<std::string> maps;
	for (const auto& map : Deathmatch::MAPS)
	{
		maps.push_back(map.name);
	}

	auto dialog = std::shared_ptr<Core::ListDialog>(new Core::ListDialog(
		fmt::sprintf(DIALOG_HEADER_TITLE, _("Duel map selection", player)),
		maps, _("Select", player), _("Cancel", player)));
	this->dialogManager->showDialog(player, dialog,
		[&player, this, playerData](Core::DialogResult result)
		{
			if (result.response())
			{
				playerData->tempData->core->temporaryDuelSettings.value()->map
					= Deathmatch::MAPS[result.listItem()];
			}
			this->showDuelCreationDialog(player);
		});
}

void DuelController::showDuelWeaponSelectionDialog(IPlayer& player)
{
	auto playerData = Core::Player::getPlayerData(player);
	if (!playerData->tempData->core->temporaryDuelSettings)
		return;

	auto tempDuelSettings
		= playerData->tempData->core->temporaryDuelSettings.value();

	std::vector<std::string> items = {
		Deathmatch::WeaponSet(Deathmatch::WeaponSet::Value::Run)
			.toString(player),
		Deathmatch::WeaponSet(Deathmatch::WeaponSet::Value::DSS)
			.toString(player),
	};

	auto dialog = std::shared_ptr<Core::ListDialog>(
		new Core::ListDialog(fmt::sprintf(DIALOG_HEADER_TITLE,
								 _("Duel weapon set selection", player)),
			items, _("Select", player), _("Cancel", player)));
	this->dialogManager->showDialog(player, dialog,
		[&player, this, playerData](Core::DialogResult result)
		{
			if (!playerData->tempData->core->temporaryDuelSettings)
				return;
			if (result.response())
			{
				auto duelSettings
					= playerData->tempData->core->temporaryDuelSettings.value();
				duelSettings->weaponSet = Deathmatch::WeaponSet(
					magic_enum::enum_value<Deathmatch::WeaponSet::Value>(
						result.listItem()));
				auto weapons = duelSettings->weaponSet.getWeapons();
				duelSettings->weapons
					= std::vector<PlayerWeapon>(weapons.begin(), weapons.end());
			}
			this->showDuelCreationDialog(player);
		});
}

void DuelController::showDuelRoundCountSettingDialog(IPlayer& player)
{
	auto playerData = Core::Player::getPlayerData(player);
	if (!playerData->tempData->core->temporaryDuelSettings)
		return;

	auto tempDuelSettings
		= playerData->tempData->core->temporaryDuelSettings.value();

	std::vector<std::string> items;

	for (int i = 0; i < 15; i++)
	{
		items.push_back(std::to_string(i * 2 + 1));
	}

	auto dialog = std::shared_ptr<Core::ListDialog>(new Core::ListDialog(
		fmt::sprintf(DIALOG_HEADER_TITLE, _("Duel rounds", player)), items,
		_("Select", player), _("Cancel", player)));
	this->dialogManager->showDialog(player, dialog,
		[&player, this, playerData](Core::DialogResult result)
		{
			if (!playerData->tempData->core->temporaryDuelSettings)
				return;
			if (result.response())
			{
				auto duelSettings
					= playerData->tempData->core->temporaryDuelSettings.value();
				duelSettings->roundCount = result.listItem() * 2 + 1;
			}
			this->showDuelCreationDialog(player);
		});
}

void DuelController::showDuelAcceptListDialog(IPlayer& player)
{
	std::vector<std::string> duels;

	auto playerData = Core::Player::getPlayerData(player);
	for (auto offer : playerData->tempData->core->duelOffersReceived)
	{
		duels.push_back(
			fmt::sprintf("{%06x}%s(%d)", offer->from->getColour().RGBA() >> 8,
				offer->from->getName().to_string(), offer->to->getID()));
	}

	auto playerExt = Core::Player::getPlayerExt(player);
	if (duels.empty())
	{
		playerExt->sendErrorMessage(__("No duel invites received"));
		return;
	}

	auto dialog = std::shared_ptr<Core::ListDialog>(new Core::ListDialog(
		fmt::sprintf(DIALOG_HEADER_TITLE, _("Duels", player)), duels,
		_("Accept", player), _("Cancel", player)));
	this->dialogManager->showDialog(player, dialog,
		[this, &player, playerData](Core::DialogResult result)
		{
			if (!result.response())
				return;
			auto playerExt = Core::Player::getPlayerExt(player);
			if (result.listItem()
				>= playerData->tempData->core->duelOffersReceived.size())
			{
				playerExt->sendErrorMessage(
					_("The player which sent you the duel offer has canceled "
					  "it or disconnected from the server",
						player));
				return;
			}
			auto offer = playerData->tempData->core->duelOffersReceived.at(
				result.listItem());
			this->showDuelAcceptConfirmDialog(player, offer);
		});
}

void DuelController::showDuelAcceptConfirmDialog(
	IPlayer& player, std::shared_ptr<DuelOffer> offer)
{
	auto playerExt = Core::Player::getPlayerExt(player);
	if (offer->from->getID() == -1)
	{
		playerExt->sendErrorMessage(_("The player has disconnected!", player));
		return;
	}

	auto dialog = std::shared_ptr<Core::MessageDialog>(new Core::MessageDialog(
		fmt::sprintf(DIALOG_HEADER_TITLE, _("Duel confirmation", player)),
		fmt::sprintf(
			_("#WHITE#- Player: {%06x}%s(%d)\n#WHITE#- Map: %s\n- Weapon set: "
			  "%s\n- Round count: "
			  "%d\n- Starting health: %.1f\n- Starting armour: %.1f\n\nDo you "
			  "accept the duel?",
				player),
			offer->from->getColour().RGBA() >> 8,
			offer->from->getName().to_string(), offer->from->getID(),
			offer->map.name, offer->weaponSet.toString(player),
			offer->roundCount, offer->defaultHealth, offer->defaultArmour),
		_("Confirm", player), _("Cancel", player)));
	this->dialogManager->showDialog(player, dialog,
		[](Core::DialogResult result)
		{
			if (!result.response())
				return;
			// TODO accept duel
		});
}

void DuelController::onRoomJoin(IPlayer& player, unsigned int roomId)
{
	auto room = this->rooms.at(roomId);
	auto playerData = Core::Player::getPlayerData(player);
	auto playerExt = Core::Player::getPlayerExt(player);

	playerData->tempData->duel->roomId = roomId;
	room->players.emplace(&player);
	player.setHealth(room->defaultHealth);
	player.setArmour(room->defaultArmor);
	player.resetWeapons();

	this->setRandomSpawnPoint(player, room);
	player.spawn();

	for (auto player : room->players)
	{
		auto playerExt = Core::Player::getPlayerExt(*player);
		playerExt->showNotification(_("~r~The fight has started!", *player),
			Core::TextDraws::NotificationPosition::Bottom);
		room->fightStarted = std::chrono::system_clock::now();
	}
}

void DuelController::onRoundEnd(IPlayer& winner, IPlayer& loser, int weaponId)
{
	this->logStatsForPlayer(winner, true, weaponId);
	this->logStatsForPlayer(loser, false, weaponId);

	auto winnerData = Core::Player::getPlayerData(winner);
	winnerData->tempData->duel->subsequentKills++;
}

void DuelController::onDuelEnd(std::shared_ptr<Room> duelRoom)
{
	// TODO show results dialog and send message to the chat
	// then remove room from the map
}

void DuelController::deleteDuelOffersFromPlayer(IPlayer& player)
{
	auto playerData = Core::Player::getPlayerData(player);
	for (auto offer : playerData->tempData->core->duelOffersSent)
	{
		auto playerData = Core::Player::getPlayerData(*offer->to);
		std::erase_if(playerData->tempData->core->duelOffersReceived,
			[&player](std::shared_ptr<Modes::Duel::DuelOffer> x)
			{
				return x->from->getID() == player.getID();
			});
	}
}

void DuelController::onPlayerSpawn(IPlayer& player)
{
	auto playerExt = Core::Player::getPlayerExt(player);
	if (!playerExt->isInMode(Modes::Mode::Duel))
		return;
	auto pData = Core::Player::getPlayerData(player);
	if (pData->tempData->duel->duelEnd)
	{
		this->coreManager.lock()->joinMode(player, Mode::Freeroam, {});
		return;
	}
	auto roomId = pData->tempData->duel->roomId;
	auto room = this->rooms.at(roomId);
	this->setupRoomForPlayer(player, room);
	// TODO implement freezing the player while start countdown is ticking
}

void DuelController::onPlayerDeath(IPlayer& player, IPlayer* killer, int reason)
{
	auto playerExt = Core::Player::getPlayerExt(player);
	if (!playerExt->isInMode(Mode::X1))
		return;
	auto playerData = Core::Player::getPlayerData(player);
	auto room = this->rooms.at(playerData->tempData->duel->roomId);
	if (room->players.size() < 2)
		return;

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

	// auto loserPos = loser->getPosition();
	// auto winnerPos = winner->getPosition();
	// this->bus->fire_event(Core::Utils::Events::DuelWin { .winner = *winner,
	// 	.loser = *loser,
	// 	.armourLeft = winner->getArmour(),
	// 	.healthLeft = winner->getHealth(),
	// 	.weapon = reason,
	// 	.distance = glm::distance(loserPos, winnerPos),
	// 	.fightDuration
	// 	= std::chrono::duration_cast<std::chrono::seconds>(fightDuration) });
	this->onRoundEnd(*winner, *loser, reason);
}

void DuelController::onPlayerDisconnect(
	IPlayer& player, PeerDisconnectReason reason)
{
	this->deleteDuelOffersFromPlayer(player);
}

DuelController::DuelController(std::weak_ptr<Core::CoreManager> coreManager,
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
	this->playerPool->getPlayerConnectDispatcher().addEventHandler(
		this, EventPriority_Highest);

	// re-register PoF event subscription
	this->playerOnFireEventRegistration
		= this->hookEvent<Core::Utils::Events::PlayerOnFireEvent>(
			this->playerOnFireEventRegistration, this,
			&DuelController::onPlayerOnFire);

	// re-register PoF been killed event subscription
	this->playerOnFireBeenKilledRegistration
		= this->hookEvent<Core::Utils::Events::PlayerOnFireBeenKilled>(
			this->playerOnFireBeenKilledRegistration, this,
			&DuelController::onPlayerOnFireBeenKilled);
}

DuelController::~DuelController()
{
	this->playerPool->getPlayerSpawnDispatcher().removeEventHandler(this);
	this->playerPool->getPlayerDamageDispatcher().removeEventHandler(this);
}

void DuelController::initCommands()
{
	this->commandManager->addCommand(
		"duel",
		[this](std::reference_wrapper<IPlayer> player, int id)
		{
			this->createDuel(player, id);
		},
		Core::Commands::CommandInfo { .args = { __("player id") },
			.description = __("Create duel"),
			.category = DUEL_MODE_NAME });

	this->commandManager->addCommand(
		"duelstats",
		[this](std::reference_wrapper<IPlayer> player, int id)
		{
			if (id < 0 || id >= this->playerPool->players().size())
			{
				Core::Player::getPlayerExt(player)->sendErrorMessage(
					_("Invalid player ID", player));
				return;
			}
			this->showDuelStatsDialog(player, id);
		},
		Core::Commands::CommandInfo { .args = { __("player id") },
			.description = __("Show Duel stats"),
			.category = DUEL_MODE_NAME });
	this->commandManager->addCommand(
		"duela",
		[this](std::reference_wrapper<IPlayer> player)
		{
			this->showDuelAcceptListDialog(player);
		},
		Core::Commands::CommandInfo {
			.description = __("Accept duels"), .category = DUEL_MODE_NAME });
}

void DuelController::onModeSelect(IPlayer& player) { }

void DuelController::onModeJoin(IPlayer& player, JoinData joinData)
{
	super::onModeJoin(player, joinData);
	// auto roomIndex = std::get<unsigned int>(joinData[DUEL_ROOM_INDEX]);
	// TODO create room
	// this->onRoomJoin(player, roomIndex);
}

void DuelController::onModeLeave(IPlayer& player)
{
	auto pData = Core::Player::getPlayerData(player);
	auto roomId = pData->tempData->x1->roomId;
	auto room = this->rooms.at(roomId);
	room->players.erase(&player);
	pData->tempData->x1.reset();

	super::onModeLeave(player);
}

void DuelController::onPlayerOnFire(
	Core::Utils::Events::PlayerOnFireEvent event)
{
	if (event.mode != this->mode)
		return;
	auto playerData = Core::Player::getPlayerData(event.player);
	playerData->duelStats->score += 4;
	super::onPlayerOnFire(event);
}

void DuelController::onPlayerOnFireBeenKilled(
	Core::Utils::Events::PlayerOnFireBeenKilled event)
{
	if (event.mode != this->mode)
		return;
	auto killerData = Core::Player::getPlayerData(event.killer);
	killerData->duelStats->score += 4;
	auto killerExt = Core::Player::getPlayerExt(event.killer);
	killerExt->sendInfoMessage(
		__("You killed player on fire and got 4 extra points!"));
	super::onPlayerOnFireBeenKilled(event);
}

void DuelController::onPlayerLoad(
	std::shared_ptr<Core::PlayerModel> data, pqxx::work& txn)
{
	auto result = txn.exec_params(
		Core::SQLQueryManager::Get()
			->getQueryByName(
				Core::Utils::SQL::Queries::LOAD_X1_STATS_FOR_PLAYER)
			.value(),
		data->userId);
	data->duelStats->updateFromRow(result[0]);
}

void DuelController::onPlayerSave(
	std::shared_ptr<Core::PlayerModel> data, pqxx::work& txn)
{
	txn.exec_params(
		Core::SQLQueryManager::Get()
			->getQueryByName(Core::Utils::SQL::Queries::UPDATE_X1_STATS)
			.value(),
		data->duelStats->score, data->x1Stats->highestKillStreak,
		data->duelStats->kills, data->x1Stats->deaths, data->x1Stats->handKills,
		data->duelStats->handheldWeaponKills, data->x1Stats->meleeKills,
		data->duelStats->handgunKills, data->x1Stats->shotgunKills,
		data->duelStats->smgKills, data->x1Stats->assaultRiflesKills,
		data->duelStats->riflesKills, data->x1Stats->heavyWeaponKills,
		data->duelStats->explosivesKills, data->userId);
}

DuelController* DuelController::create(
	std::weak_ptr<Core::CoreManager> coreManager,
	std::shared_ptr<Core::Commands::CommandManager> commandManager,
	std::shared_ptr<Core::DialogManager> dialogManager, IPlayerPool* playerPool,
	ITimersComponent* timersComponent, std::shared_ptr<dp::event_bus> bus)
{
	auto controller = new DuelController(coreManager, commandManager,
		dialogManager, playerPool, timersComponent, bus);
	controller->initCommands();
	return controller;
}
}