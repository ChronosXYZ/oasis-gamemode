#include "DuelController.hpp"
#include "../deathmatch/Maps.hpp"
#include "../deathmatch/DeathmatchResult.hpp"
#include "../../core/player/PlayerExtension.hpp"
#include "../../core/utils/Common.hpp"
#include "../../core/utils/QueryNames.hpp"
#include "../../core/SQLQueryManager.hpp"
#include "DuelOffer.hpp"
#include "PlayerTempData.hpp"
#include "events.hpp"
#include "types.hpp"
#include "values.hpp"

#include <array>
#include <bits/chrono.h>
#include <functional>
#include <magic_enum/magic_enum.hpp>
#include <map>
#include <player.hpp>
#include <chrono>
#include <fmt/printf.h>
#include <scn/scan.h>
#include <Server/Components/Timers/Impl/timers_impl.hpp>

#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

namespace Modes::Duel
{
void DuelController::createDuel(IPlayer& player, int id)
{
	auto playerExt = Core::Player::getPlayerExt(player);
	auto receivingPlayer = this->playerPool->get(id);
	if (!receivingPlayer || id == player.getID())
	{
		playerExt->sendErrorMessage(__("Invalid player id"));
		return;
	}

	auto receivingPlayerExt = Core::Player::getPlayerExt(*receivingPlayer);
	if (!receivingPlayerExt->isAuthorized())
	{
		playerExt->sendErrorMessage(__("Player has not been authorized"));
		return;
	}
	if (playerExt->getPlayerData()->tempData->core->duelOfferSent.has_value())
	{
		playerExt->sendErrorMessage(__("You already sent a duel offer!"));
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

unsigned int DuelController::createDuelRoom(std::shared_ptr<DuelOffer> offer)
{
	auto roomId = this->roomIdPool->allocateId();
	this->rooms[roomId] = std::shared_ptr<Room>(new Room {
		.map = offer->map,
		.allowedWeapons = offer->weaponSet.getWeapons(),
		.virtualWorld = this->coreManager.lock()->allocateVirtualWorldId(),
		.maxRounds = offer->roundCount,
	});
	return roomId;
}

void DuelController::deleteDuel(unsigned int id)
{
	if (!this->rooms.contains(id))
		return;
	auto room = this->rooms.at(id);
	for (auto player : room->players)
	{
		auto playerData = Core::Player::getPlayerData(*player);
		if (!playerData->tempData->duel->duelEnd)
			this->coreManager.lock()->joinMode(*player, Mode::Freeroam, {});
	}
	if (room->roundStartTimer.has_value())
		room->roundStartTimer.value()->kill();
	this->rooms.erase(id);
	this->roomIdPool->freeId(id);
	this->coreManager.lock()->freeVirtualWorldId(room->virtualWorld);
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
		playerData->tempData->duel->increaseKills();

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
		playerData->tempData->duel->increaseDeaths();
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
			__("The player you want to call for a duel has disconnected!"));
		return;
	}
	playerData->tempData->core->duelOfferSent = tempDuelSettings;
	playerData->tempData->core->temporaryDuelSettings.reset();
	auto receivingPlayerData
		= Core::Player::getPlayerData(*tempDuelSettings->to);
	auto receivingPlayerExt = Core::Player::getPlayerExt(*tempDuelSettings->to);
	receivingPlayerData->tempData->core->duelOffersReceived[player.getID()]
		= tempDuelSettings;
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
		playerData->duelStats->score, playerData->duelStats->highestKillStreak,
		playerData->duelStats->kills, playerData->duelStats->deaths,
		float(playerData->duelStats->kills) / float(ratioDeaths),
		playerData->duelStats->handKills,
		playerData->duelStats->handheldWeaponKills,
		playerData->duelStats->meleeKills, playerData->duelStats->handgunKills,
		playerData->duelStats->shotgunKills, playerData->duelStats->smgKills,
		playerData->duelStats->assaultRiflesKills,
		playerData->duelStats->riflesKills,
		playerData->duelStats->heavyWeaponKills,
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
	std::map<int, int> duelItemToPlayerId;

	auto playerData = Core::Player::getPlayerData(player);
	int i = 0;
	for (auto [id, offer] : playerData->tempData->core->duelOffersReceived)
	{
		duels.push_back(
			fmt::sprintf("{%06x}%s(%d)", offer->from->getColour().RGBA() >> 8,
				offer->from->getName().to_string(), offer->to->getID()));
		duelItemToPlayerId[i] = id;
		i++;
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
		[this, &player, playerData, duelItemToPlayerId](
			Core::DialogResult result)
		{
			if (!result.response())
				return;
			auto playerExt = Core::Player::getPlayerExt(player);
			if (result.listItem()
				>= playerData->tempData->core->duelOffersReceived.size())
			{
				playerExt->sendErrorMessage(
					__("The player which sent you the duel offer has canceled "
					   "it or disconnected from the server"));
				return;
			}
			auto offer = playerData->tempData->core->duelOffersReceived.at(
				duelItemToPlayerId.at(result.listItem()));
			this->showDuelAcceptConfirmDialog(player, offer);
		});
}

void DuelController::showDuelAcceptConfirmDialog(
	IPlayer& player, std::shared_ptr<DuelOffer> offer)
{
	auto playerExt = Core::Player::getPlayerExt(player);
	if (offer->from->getID() == -1)
	{
		playerExt->sendErrorMessage(__("The player has disconnected!"));
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
		_("Accept", player), _("Refuse", player)));
	this->dialogManager->showDialog(player, dialog,
		[this, offer, &player](Core::DialogResult result)
		{
			if (!result.response())
			{
				this->deleteDuelOfferFromPlayer(*offer->from, true);
				auto senderExt = Core::Player::getPlayerExt(*offer->from);
				senderExt->sendInfoMessage(__("DUEL: {%06x}%s(%d) #RED#refused "
											  "#WHITE#the duel!"),
					player.getColour().RGBA() >> 8,
					offer->to->getName().to_string(), offer->to->getID());
				return;
			}
			auto roomId = this->createDuelRoom(offer);
			offer->tempRoomId = roomId;
			auto senderJoinResult = this->coreManager.lock()->joinMode(
				*offer->from, Mode::Duel, { { DUEL_ROOM_ID, roomId } });
			auto receiverJoinResult = this->coreManager.lock()->joinMode(
				player, Mode::Duel, { { DUEL_ROOM_ID, roomId } });
			if (!senderJoinResult || !receiverJoinResult)
			{
				auto senderExt = Core::Player::getPlayerExt(*offer->from);
				auto receiverExt = Core::Player::getPlayerExt(*offer->to);
				senderExt->sendErrorMessage(
					__("One of the players cannot join duel, try again!"));
				receiverExt->sendErrorMessage(
					__("One of the players cannot join duel, try again!"));
				return;
			}
			this->deleteDuelOfferFromPlayer(*offer->from, false);
		});
}

void DuelController::showDuelResults(std::shared_ptr<Room> room)
{
	if (!room->cachedResults)
	{
		spdlog::warn("result cache of room is poisoned!");
		return;
	}
	for (auto player : room->players)
	{
		std::string header
			= _("Player\tK : D\tRatio\tDamage inflicted\n", *player);
		auto dialog = std::shared_ptr<Core::TabListHeadersDialog>(
			new Core::TabListHeadersDialog(
				fmt::sprintf(
					DIALOG_HEADER_TITLE, _("Deathmatch statistics", *player)),
				{ _("Player", *player), _("K : D", *player),
					_("Ratio", *player), _("Damage inflicted", *player) },
				room->cachedResults.value(), _("Close", *player), ""));
		this->dialogManager->showDialog(*player, dialog,
			[](Core::DialogResult result)
			{
			});
	}
}

void DuelController::onRoomJoin(IPlayer& player, unsigned int roomId)
{
	auto room = this->rooms.at(roomId);
	auto playerData = Core::Player::getPlayerData(player);
	auto playerExt = Core::Player::getPlayerExt(player);

	playerData->tempData->duel->roomId = roomId;
	room->players.push_back(&player);
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

void DuelController::onRoundEnd(IPlayer* winner, IPlayer* loser, int weaponId)
{
	this->logStatsForPlayer(*winner, true, weaponId);
	this->logStatsForPlayer(*loser, false, weaponId);

	auto winnerData = Core::Player::getPlayerData(*winner);
	auto loserData = Core::Player::getPlayerData(*loser);
	winnerData->tempData->duel->subsequentKills++;

	auto room = this->rooms.at(winnerData->tempData->duel->roomId);
	room->currentRound++;
	if (room->currentRound == room->maxRounds)
	{
		winnerData->tempData->duel->duelEnd = true;
		loserData->tempData->duel->duelEnd = true;
		winner->spawn();
		this->onDuelEnd(room);
	}
}

void DuelController::onDuelEnd(std::shared_ptr<Room> duelRoom)
{
	std::vector<Deathmatch::DeathmatchResult> resultArray;
	for (auto& player : duelRoom->players)
	{
		auto playerData = Core::Player::getPlayerData(*player);
		resultArray.push_back(Deathmatch::DeathmatchResult {
			.playerName = player->getName().to_string(),
			.playerColor = player->getColour(),
			.kills = playerData->tempData->duel->kills,
			.deaths = playerData->tempData->duel->deaths,
			.ratio = playerData->tempData->duel->ratio,
			.damageInflicted = playerData->tempData->duel->damageInflicted });
	}
	std::sort(resultArray.begin(), resultArray.end(),
		[](Deathmatch::DeathmatchResult x1, Deathmatch::DeathmatchResult x2)
		{
			return x1.kills > x2.kills;
		});

	std::vector<std::vector<std::string>> results;
	for (std::size_t i = 0; i < resultArray.size(); i++)
	{
		auto playerResult = resultArray.at(i);
		results.push_back({ fmt::sprintf("{%06x}%d. %s",
								playerResult.playerColor.RGBA() >> 8, i + 1,
								playerResult.playerName),
			fmt::sprintf("%d : %d", playerResult.kills, playerResult.deaths),
			fmt::sprintf("%.2f", playerResult.ratio),
			fmt::sprintf("%.2f", playerResult.damageInflicted) });
	}
	duelRoom->cachedResults = results;
	this->showDuelResults(duelRoom);
}

void DuelController::deleteDuelOfferFromPlayer(IPlayer& player, bool deleteRoom)
{
	auto senderData = Core::Player::getPlayerData(player);
	if (auto offerOpt = senderData->tempData->core->duelOfferSent)
	{
		auto offer = *offerOpt;
		if (deleteRoom && offer->tempRoomId)
		{
			this->deleteDuel(offer->tempRoomId.value());
		}
		auto receivingPlayerData = Core::Player::getPlayerData(*offer->to);
		auto senderExt = Core::Player::getPlayerExt(player);
		receivingPlayerData->tempData->core->duelOffersReceived.erase(
			player.getID());
		senderData->tempData->core->duelOfferSent = {};
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

	player.setControllable(false);

	if (!room->roundStartTimer.has_value())
	{
		if (room->lastWinner)
		{
			room->lastWinner.value()->spawn();
		}
		auto startSecs = std::make_shared<unsigned int>(3);
		for (auto player : room->players)
		{
			player->sendGameText(
				fmt::sprintf("~r~DUEL~n~~w~%s~n~~r~VS.~n~~w~%s~n~Round %d/%d",
					room->players[0]->getName().to_string(),
					room->players[1]->getName().to_string(),
					room->currentRound + 1, room->maxRounds),
				Seconds(3), 6);
		}
		room->roundStartTimer = this->timersComponent->create(
			new Impl::SimpleTimerHandler(
				[this, room, startSecs]()
				{
					if ((*startSecs)-- == 0)
					{
						room->roundStartTimer.value()->kill();
						room->roundStartTimer.reset();
						for (auto player : room->players)
						{
							player->sendGameText("~g~"
									+ _(fmt::sprintf("%s",
											ROUND_START_TEXT[random()
												% ROUND_START_TEXT.size()]),
										*player),
								Seconds(1), 6);
							player->setControllable(true);
						}
					}
				}),
			Seconds(1), true);
		room->roundStartTimer.value()->trigger();
	}
}

void DuelController::onPlayerDeath(IPlayer& player, IPlayer* killer, int reason)
{
	auto playerExt = Core::Player::getPlayerExt(player);
	if (!playerExt->isInMode(Mode::Duel))
		return;
	auto playerData = Core::Player::getPlayerData(player);
	auto room = this->rooms.at(playerData->tempData->duel->roomId);
	if (room->players.size() < 2)
		return;

	IPlayer* loser;
	IPlayer* winner;
	for (auto roomPlayer : room->players)
	{
		if (roomPlayer->getID() == player.getID())
			loser = roomPlayer;
		else
			winner = roomPlayer;
	}

	room->lastWinner = winner;

	this->setRandomSpawnPoint(*winner, room);
	this->setRandomSpawnPoint(*loser, room);

	this->onRoundEnd(winner, loser, reason);
	// auto now = std::chrono::system_clock::now();
	// auto fightDuration = now - room->fightStarted;
	// auto loserPos = loser->getPosition();
	// auto winnerPos = winner->getPosition();
	// this->bus->fire_event(Core::Utils::Events::DuelWin { .winner =
	// *winner, 	.loser = *loser, 	.armourLeft = winner->getArmour(),
	// 	.healthLeft = winner->getHealth(),
	// 	.weapon = reason,
	// 	.distance = glm::distance(loserPos, winnerPos),
	// 	.fightDuration
	// 	= std::chrono::duration_cast<std::chrono::seconds>(fightDuration)
	// });
}

void DuelController::onPlayerGiveDamage(IPlayer& player, IPlayer& to,
	float amount, unsigned int weapon, BodyPart part)
{
	auto playerExt = Core::Player::getPlayerExt(player);
	auto playerData = playerExt->getPlayerData();
	if (!playerExt->isInMode(Modes::Mode::Duel))
		return;
	playerData->tempData->duel->damageInflicted += amount;
}

void DuelController::onPlayerDisconnect(
	IPlayer& player, PeerDisconnectReason reason)
{
	auto playerData = Core::Player::getPlayerData(player);
	for (auto [id, offer] : playerData->tempData->core->duelOffersReceived)
	{
		if (offer->tempRoomId)
		{
			this->deleteDuel(offer->tempRoomId.value());
		}
		auto senderData = Core::Player::getPlayerData(*offer->from);
		auto senderExt = Core::Player::getPlayerExt(*offer->from);
		senderExt->sendInfoMessage(
			__("DUEL: {%06x}%s(%d) #WHITE#declined offer: "
			   "disconnected from the server!"),
			player.getColour().RGBA() >> 8, player.getName().to_string(),
			player.getID());
		senderData->tempData->core->duelOfferSent = {};
	}
	if (auto offerOpt = playerData->tempData->core->duelOfferSent)
	{
		this->deleteDuelOfferFromPlayer(player, true);
	}
}

DuelController::DuelController(std::weak_ptr<Core::CoreManager> coreManager,
	std::shared_ptr<Core::Commands::CommandManager> commandManager,
	std::shared_ptr<Core::DialogManager> dialogManager, IPlayerPool* playerPool,
	ITimersComponent* timersComponent, std::shared_ptr<dp::event_bus> bus)
	: super(Mode::Duel, bus, playerPool)
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
		[this](std::reference_wrapper<IPlayer> player, std::string args)
		{
			auto scanResult = scn::scan<int>(args, "{}");
			if (!scanResult)
				return false;
			auto [id] = scanResult->values();

			this->createDuel(player, id);

			return true;
		},
		Core::Commands::CommandInfo { .args = { __("player id") },
			.description = __("Create duel"),
			.category = DUEL_MODE_NAME });

	this->commandManager->addCommand(
		"duelstats",
		[this](std::reference_wrapper<IPlayer> player, std::string args)
		{
			int id;
			auto scanResult = scn::scan<int>(args, "{}");
			if (!scanResult)
			{
				id = player.get().getID();
			}
			else
			{
				id = scanResult->value();
			}

			if (id < 0 || id >= this->playerPool->players().size())
			{
				Core::Player::getPlayerExt(player)->sendErrorMessage(
					__("Invalid player ID"));
				return true;
			}
			this->showDuelStatsDialog(player, id);
			return true;
		},
		Core::Commands::CommandInfo { .args = { __("player id") },
			.description = __("Show Duel stats"),
			.category = DUEL_MODE_NAME });
	this->commandManager->addCommand(
		"duela",
		[this](std::reference_wrapper<IPlayer> player, std::string args)
		{
			if (!args.empty())
				return false;
			this->showDuelAcceptListDialog(player);
			return true;
		},
		Core::Commands::CommandInfo {
			.description = __("Accept duels"), .category = DUEL_MODE_NAME });
}

void DuelController::onModeSelect(IPlayer& player) { }

void DuelController::onModeJoin(IPlayer& player, JoinData joinData)
{
	super::onModeJoin(player, joinData);
	auto playerData = Core::Player::getPlayerData(player);
	if (!joinData.contains(DUEL_ROOM_ID)
		&& !playerData->tempData->core->duelOfferSent)
	{
		this->coreManager.lock()->joinMode(player, Mode::Freeroam);
		Core::Player::getPlayerExt(player)->sendErrorMessage(
			__("Invalid duel settings!"));
		return;
	}
	auto roomId = std::get<unsigned int>(joinData[DUEL_ROOM_ID]);
	playerData->tempData->duel
		= std::unique_ptr<PlayerTempData>(new PlayerTempData());
	this->onRoomJoin(player, roomId);
}

void DuelController::onModeLeave(IPlayer& player)
{
	auto pData = Core::Player::getPlayerData(player);
	auto roomId = pData->tempData->duel->roomId;
	if (this->rooms.contains(roomId))
	{
		auto room = this->rooms.at(roomId);
		this->deleteDuel(roomId);
		pData->tempData->duel.reset();
	}

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
				Core::Utils::SQL::Queries::LOAD_DUEL_STATS_FOR_PLAYER)
			.value(),
		data->userId);
	data->duelStats->updateFromRow(result[0]);
}

void DuelController::onPlayerSave(
	std::shared_ptr<Core::PlayerModel> data, pqxx::work& txn)
{
	txn.exec_params(
		Core::SQLQueryManager::Get()
			->getQueryByName(Core::Utils::SQL::Queries::UPDATE_DUEL_STATS)
			.value(),
		data->duelStats->score, data->duelStats->highestKillStreak,
		data->duelStats->kills, data->duelStats->deaths,
		data->x1Stats->handKills, data->duelStats->handheldWeaponKills,
		data->duelStats->meleeKills, data->duelStats->handgunKills,
		data->duelStats->shotgunKills, data->duelStats->smgKills,
		data->duelStats->assaultRiflesKills, data->duelStats->riflesKills,
		data->duelStats->heavyWeaponKills, data->duelStats->explosivesKills,
		data->userId);
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