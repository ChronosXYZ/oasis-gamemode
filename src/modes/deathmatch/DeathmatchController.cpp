#include "DeathmatchController.hpp"
#include "Maps.hpp"
#include "PlayerTempData.hpp"
#include "Room.hpp"
#include "Room.tpp"
#include "WeaponSet.hpp"
#include "../../core/utils/Localization.hpp"
#include "../../core/player/PlayerExtension.hpp"
#include "../../core/utils/Random.hpp"
#include "../../core/utils/Events.hpp"
#include "../../core/utils/Common.hpp"
#include "../../core/utils/QueryNames.hpp"
#include "../../core/dialogs/DialogManager.hpp"
#include "../../core/SQLQueryManager.hpp"
#include "textdraws/DeathmatchTimer.hpp"
#include "DeathmatchResult.hpp"

#include <chrono>
#include <cstddef>
#include <fmt/printf.h>
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
#include <eventbus/event_bus.hpp>

#include <optional>
#include <vector>

#define PRESSED(newkeys, oldkeys, k)                                           \
	(((newkeys & (k)) == (k)) && ((oldkeys & (k)) != (k)))

#define CBUG_FREEZE_DELAY 1500
#define ROOM_INDEX "roomIndex"
#define DEFAULT_ROOM_ROUND_TIME_MIN 10

namespace Modes::Deathmatch
{
DeathmatchController::DeathmatchController(
	std::weak_ptr<Core::CoreManager> coreManager,
	std::shared_ptr<Core::Commands::CommandManager> commandManager,
	std::shared_ptr<Core::DialogManager> dialogManager, IPlayerPool* playerPool,
	ITimersComponent* timersComponent, std::shared_ptr<dp::event_bus> bus,
	std::shared_ptr<pqxx::connection> dbConnection)
	: super(Mode::Deathmatch, bus, playerPool)
	, coreManager(coreManager)
	, commandManager(commandManager)
	, dialogManager(dialogManager)
	, roomIdPool(std::make_unique<Core::Utils::IDPool>())
	, _playerPool(playerPool)
	, _timersComponent(timersComponent)
	, dbConnection(dbConnection)
{
	_playerPool->getPlayerDamageDispatcher().addEventHandler(this);
	_playerPool->getPlayerSpawnDispatcher().addEventHandler(this);
	_playerPool->getPlayerChangeDispatcher().addEventHandler(this);
	_ticker = _timersComponent->create(
		new Impl::SimpleTimerHandler(
			std::bind(&DeathmatchController::onTick, this)),
		Milliseconds(1000), true);

	// re-register player on fire handler
	this->bus->remove_handler(this->playerOnFireEventRegistration);
	this->playerOnFireEventRegistration
		= this->bus->register_handler<Core::Utils::Events::PlayerOnFireEvent>(
			this, &DeathmatchController::onPlayerOnFire);

	// re-register player on fire been killed handler
	this->bus->remove_handler(this->playerOnFireBeenKilledRegistration);
	this->playerOnFireEventRegistration
		= this->bus
			  ->register_handler<Core::Utils::Events::PlayerOnFireBeenKilled>(
				  this, &DeathmatchController::onPlayerOnFireBeenKilled);
}

DeathmatchController::~DeathmatchController()
{
	_playerPool->getPlayerDamageDispatcher().removeEventHandler(this);
	_playerPool->getPlayerSpawnDispatcher().removeEventHandler(this);
	_playerPool->getPlayerChangeDispatcher().removeEventHandler(this);
	_ticker->kill();
}

void DeathmatchController::onModeJoin(IPlayer& player,
	std::unordered_map<std::string, Core::PrimitiveType> joinData)
{
	super::onModeJoin(player, joinData);
	auto roomIndex = std::get<unsigned int>(joinData[ROOM_INDEX]);
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
	super::onModeLeave(player);
}

void DeathmatchController::onPlayerSave(
	std::shared_ptr<Core::PlayerModel> data, pqxx::work& txn)
{
	txn.exec_params(
		Core::SQLQueryManager::Get()
			->getQueryByName(Core::Utils::SQL::Queries::UPDATE_DM_STATS)
			.value(),
		data->dmStats->score, data->dmStats->highestKillStreak,
		data->dmStats->kills, data->dmStats->deaths, data->dmStats->handKills,
		data->dmStats->handheldWeaponKills, data->dmStats->meleeKills,
		data->dmStats->handgunKills, data->dmStats->shotgunKills,
		data->dmStats->smgKills, data->dmStats->assaultRiflesKills,
		data->dmStats->riflesKills, data->dmStats->heavyWeaponKills,
		data->dmStats->explosivesKills, data->userId);
}

void DeathmatchController::onPlayerLoad(
	std::shared_ptr<Core::PlayerModel> data, pqxx::work& txn)
{
	auto result = txn.exec_params(
		Core::SQLQueryManager::Get()
			->getQueryByName(
				Core::Utils::SQL::Queries::LOAD_DM_STATS_FOR_PLAYER)
			.value(),
		data->userId);
	data->dmStats->updateFromRow(result[0]);
}

void DeathmatchController::onPlayerSpawn(IPlayer& player)
{
	auto playerExt = Core::Player::getPlayerExt(player);
	auto pData = Core::Player::getPlayerData(player);
	if (!playerExt->isInMode(Modes::Mode::Deathmatch))
		return;
	auto roomId = pData->tempData->deathmatch->roomId;
	auto room = this->rooms.at(roomId);
	this->setupRoomForPlayer(player, room);
}

void DeathmatchController::onPlayerDeath(
	IPlayer& player, IPlayer* killer, int reason)
{
	auto playerExt = Core::Player::getPlayerExt(player);
	auto playerData = Core::Player::getPlayerData(player);
	if (!playerExt->isInMode(Modes::Mode::Deathmatch))
		return;

	playerData->tempData->deathmatch->increaseDeaths();
	playerData->dmStats->deaths += 1;
	if (playerData->tempData->deathmatch->subsequentKills
		> playerData->dmStats->highestKillStreak)
	{
		playerData->dmStats->highestKillStreak
			= playerData->tempData->deathmatch->subsequentKills;
	}
	playerData->tempData->deathmatch->subsequentKills = 0;

	auto roomId = playerData->tempData->deathmatch->roomId;
	auto room = this->rooms.at(roomId);
	if (killer)
	{
		auto killerData = Core::Player::getPlayerData(*killer);
		killerData->tempData->deathmatch->increaseKills();
		killerData->tempData->deathmatch->subsequentKills++;
		if (room->refillEnabled)
		{
			killer->setHealth(room->defaultHealth);
			killer->setArmour(room->defaultArmor);
		}
		killerData->dmStats->kills += 1;
		killerData->dmStats->score += 1;

		switch (Core::Utils::getWeaponType(reason))
		{
		case Core::Utils::WeaponType::Hand:
		{
			killerData->dmStats->handKills++;
			break;
		}
		case Core::Utils::WeaponType::HandheldItems:
		{
			killerData->dmStats->handheldWeaponKills++;
			break;
		}
		case Core::Utils::WeaponType::Melee:
		{
			killerData->dmStats->meleeKills++;
			break;
		}
		case Core::Utils::WeaponType::Handguns:
		{
			killerData->dmStats->handgunKills++;
			break;
		}
		case Core::Utils::WeaponType::Shotguns:
		{
			killerData->dmStats->shotgunKills++;
			break;
		}
		case Core::Utils::WeaponType::SMG:
		{
			killerData->dmStats->smgKills++;
			break;
		}
		case Core::Utils::WeaponType::AssaultRifles:
		{
			killerData->dmStats->assaultRiflesKills++;
			break;
		}
		case Core::Utils::WeaponType::Rifles:
		{
			killerData->dmStats->riflesKills++;
			break;
		}
		case Core::Utils::WeaponType::HeavyWeapons:
		{
			killerData->dmStats->heavyWeaponKills++;
			break;
		}
		case Core::Utils::WeaponType::Explosives:
		{
			killerData->dmStats->explosivesKills++;
			break;
		}
		case Core::Utils::WeaponType::Unknown:
			break;
		}
	}

	this->setRandomSpawnPoint(player, room);
}

void DeathmatchController::onPlayerKeyStateChange(
	IPlayer& player, uint32_t newKeys, uint32_t oldKeys)
{
	auto playerExt = Core::Player::getPlayerExt(player);
	auto playerData = playerExt->getPlayerData();
	if (!playerExt->isInMode(Modes::Mode::Deathmatch))
		return;

	if (player.getState() != PlayerState_OnFoot)
		return;

	auto roomId = playerData->tempData->deathmatch->roomId;
	if (this->rooms.at(roomId)->cbugEnabled)
		return;
	using namespace std::chrono;
	if (PRESSED(newKeys, oldKeys, Key::FIRE))
	{
		playerData->tempData->deathmatch->lastShootTime
			= system_clock::to_time_t(system_clock::now());
	}
	else if (PRESSED(newKeys, oldKeys, Key::CROUCH))
	{
		auto lastShootTime = playerData->tempData->deathmatch->lastShootTime;
		auto currentTimestamp = system_clock::to_time_t(system_clock::now());
		if (currentTimestamp - lastShootTime < 1)
		{
			player.setControllable(false);
			playerData->tempData->deathmatch->cbugging = true;
			player.sendGameText(
				_("Don't use ~r~C-bug!", player), Milliseconds(3000), 5);
			player.playSound(4604, Vector3(0.0, 0.0, 0.0));

			if (auto oldTimerId
				= playerData->tempData->deathmatch->cbugFreezeTimerId)
			{
				this->_cbugFreezeTimers[oldTimerId.value()]->kill();
				this->_cbugFreezeTimers.erase(oldTimerId.value());
			}

			auto timerId = uuids::to_string(Core::Utils::getUuidGenerator()());
			auto timerHandler = new Impl::SimpleTimerHandler(
				[&player, playerData, this, timerId]()
				{
					player.setControllable(true);
					playerData->tempData->deathmatch->cbugging = false;
					playerData->tempData->deathmatch->cbugFreezeTimerId.reset();
					this->_cbugFreezeTimers.erase(timerId);
				});
			auto timer = _timersComponent->create(
				timerHandler, Milliseconds(1500), false);
			playerData->tempData->deathmatch->cbugFreezeTimerId = timerId;
			this->_cbugFreezeTimers[timerId] = timer;
		}
	}
}

void DeathmatchController::onPlayerGiveDamage(IPlayer& player, IPlayer& to,
	float amount, unsigned int weapon, BodyPart part)
{
	auto playerExt = Core::Player::getPlayerExt(player);
	auto playerData = playerExt->getPlayerData();
	if (!playerExt->isInMode(Modes::Mode::Deathmatch))
		return;
	playerData->tempData->deathmatch->damageInflicted += amount;
}

void DeathmatchController::onPlayerOnFire(
	Core::Utils::Events::PlayerOnFireEvent event)
{
	if (event.mode != this->mode)
		return;
	auto playerData = Core::Player::getPlayerData(event.player);
	playerData->dmStats->score += 4;
	super::onPlayerOnFire(event);
}

void DeathmatchController::onPlayerOnFireBeenKilled(
	Core::Utils::Events::PlayerOnFireBeenKilled event)
{
	if (event.mode != this->mode)
		return;
	auto killerData = Core::Player::getPlayerData(event.killer);
	killerData->dmStats->score += 4;
	auto killerExt = Core::Player::getPlayerExt(event.killer);
	killerExt->sendInfoMessage(
		__("You killed player on fire and got 4 extra points!"));
	super::onPlayerOnFireBeenKilled(event);
}

DeathmatchController* DeathmatchController::create(
	std::weak_ptr<Core::CoreManager> coreManager,
	std::shared_ptr<Core::Commands::CommandManager> commandManager,
	std::shared_ptr<Core::DialogManager> dialogManager, IPlayerPool* playerPool,
	ITimersComponent* timersComponent, std::shared_ptr<dp::event_bus> bus,
	std::shared_ptr<pqxx::connection> dbConnection)
{
	auto controller = new DeathmatchController(coreManager, commandManager,
		dialogManager, playerPool, timersComponent, bus, dbConnection);
	controller->initRooms();
	controller->initCommand();
	return controller;
}

void DeathmatchController::initCommand()
{
	this->commandManager->addCommand(
		"dm",
		[&](std::reference_wrapper<IPlayer> player, int id)
		{
			auto playerExt = Core::Player::getPlayerExt(player.get());
			auto playerData = Core::Player::getPlayerData(player.get());
			if (!this->rooms.contains((unsigned int)id - 1) || id <= 0)
			{
				playerExt->sendErrorMessage(
					_("Such room doesn't exist!", player));
				return;
			}
			this->coreManager.lock()->joinMode(player, Mode::Deathmatch,
				{ { ROOM_INDEX, (unsigned int)id - 1 } });
		},
		Core::Commands::CommandInfo { .args = { __("room number") },
			.description = __("Enter DM room"),
			.category = MODE_NAME });
	this->commandManager->addCommand(
		"dm",
		[&](std::reference_wrapper<IPlayer> player)
		{
			this->coreManager.lock()->selectMode(player, Mode::Deathmatch);
		},
		Core::Commands::CommandInfo { .args = {},
			.description = __("Enter DM mode"),
			.category = MODE_NAME });
	this->commandManager->addCommand(
		"dmstats",
		[this](std::reference_wrapper<IPlayer> player, int id)
		{
			if (id < 0 || id >= this->_playerPool->players().size())
			{
				Core::Player::getPlayerExt(player)->sendErrorMessage(
					_("Invalid player ID", player));
				return;
			}
			this->showDeathmatchStatsDialog(player, id);
		},
		Core::Commands::CommandInfo { .args = { "player id" },
			.description = __("Show DM stats"),
			.category = MODE_NAME });
}

void DeathmatchController::initRooms()
{
	WeaponSet runWeaponSet(WeaponSet::Value::Run);
	WeaponSet dssWeaponSet(WeaponSet::Value::DSS);

	auto roomId1 = this->roomIdPool->allocateId();
	auto roomId2 = this->roomIdPool->allocateId();
	auto roomId3 = this->roomIdPool->allocateId();
	this->rooms = {
		{ roomId1,
			std::shared_ptr<Room>(new Room {
				.map = randomlySelectMap(runWeaponSet),
				.allowedWeapons = runWeaponSet.getWeapons(),
				.weaponSet = runWeaponSet,
				.host = {},
				.virtualWorld = VIRTUAL_WORLD_PREFIX + 0,
				.cbugEnabled = true,
				.countdown = std::chrono::minutes(DEFAULT_ROOM_ROUND_TIME_MIN),
				.defaultTime
				= std::chrono::minutes(DEFAULT_ROOM_ROUND_TIME_MIN),
				.defaultArmor = 100.0,
			}) },
		{ roomId2,
			std::shared_ptr<Room>(new Room {
				.map = randomlySelectMap(dssWeaponSet),
				.allowedWeapons = dssWeaponSet.getWeapons(),
				.weaponSet = dssWeaponSet,
				.host = {},
				.virtualWorld = VIRTUAL_WORLD_PREFIX + 1,
				.cbugEnabled = true,
				.countdown = std::chrono::minutes(DEFAULT_ROOM_ROUND_TIME_MIN),
				.defaultTime
				= std::chrono::minutes(DEFAULT_ROOM_ROUND_TIME_MIN),
				.defaultArmor = 100.0,
			}) },
		{ roomId3,
			std::shared_ptr<Room>(new Room {
				.map = randomlySelectMap(dssWeaponSet),
				.allowedWeapons = dssWeaponSet.getWeapons(),
				.weaponSet = dssWeaponSet,
				.host = {},
				.virtualWorld = VIRTUAL_WORLD_PREFIX + 2,
				.cbugEnabled = false,
				.countdown = std::chrono::minutes(DEFAULT_ROOM_ROUND_TIME_MIN),
				.defaultTime
				= std::chrono::minutes(DEFAULT_ROOM_ROUND_TIME_MIN),
				.defaultArmor = 100.0,
			}) },
	};
}

void DeathmatchController::showRoomSelectionDialog(
	IPlayer& player, bool modeSelection)
{
	std::vector<std::vector<std::string>> items;
	items.push_back({ _("Create custom room", player) });
	for (auto [roomId, room] : this->rooms)
	{
		items.push_back({ fmt::sprintf("{999999}%d. {00FF00}%s", roomId + 1,
							  room->map.name),
			fmt::sprintf("{00FF00}%s",
				room->weaponSet.toString(player).append(
					room->cbugEnabled ? "" : _(" #RED#(NO CBUG)", player))),
			fmt::sprintf(
				"{00FF00}%s", room->host.value_or(_("Server", player))),
			fmt::sprintf("{00FF00}%d", room->players.size()) });
	}
	auto dialog = std::shared_ptr<Core::TabListHeadersDialog>(
		new Core::TabListHeadersDialog(fmt::sprintf(DIALOG_HEADER_TITLE,
										   _("Select Deathmatch room", player)),
			{ _("#CREAM_BRULEE#Map", player), _("#CREAM_BRULEE#Weapon", player),
				_("#CREAM_BRULEE#Host", player),
				_("#CREAM_BRULEE#Players", player) },

			items, _("Select", player), _("Close", player)));

	this->dialogManager->showDialog(player, dialog,
		[modeSelection, this, &player](Core::DialogResult result)
		{
			if (result.response())
			{
				if (result.listItem() == 0)
				{
					this->showRoomCreationDialog(player);
					return;
				}
				if (result.listItem() > this->rooms.size())
				{
					// when player selected a room and this room has
					// been evicted
					this->showRoomSelectionDialog(player);
					return;
				}
				auto roomIndex = (unsigned int)result.listItem() - 1;
				this->coreManager.lock()->joinMode(
					player, Mode::Deathmatch, { { ROOM_INDEX, roomIndex } });
			}
			else
			{
				if (modeSelection)
					this->coreManager.lock()->showModeSelectionDialog(player);
			}
		});
}

void DeathmatchController::showRoundResultDialog(
	IPlayer& player, std::shared_ptr<Room> room)
{
	if (!room->cachedLastResult)
	{
		spdlog::warn("last result cache of room is poisoned!");
		return;
	}
	std::string header = _("Player\tK : D\tRatio\tDamage inflicted\n", player);
	auto dialog = std::shared_ptr<Core::TabListHeadersDialog>(
		new Core::TabListHeadersDialog(fmt::sprintf(DIALOG_HEADER_TITLE,
										   _("Deathmatch statistics", player)),
			{ _("Player", player), _("K : D", player), _("Ratio", player),
				_("Damage inflicted", player) },
			{} /* TODO */, _("Close", player), ""));
	this->dialogManager->showDialog(player, dialog,
		[this, &player, room](Core::DialogResult result)
		{
			if (room->isRestarting)
			{
				this->showRoundResultDialog(player, room);
			}
		});
}

void DeathmatchController::showDeathmatchStatsDialog(
	IPlayer& player, unsigned int id)
{
	auto anotherPlayer = this->_playerPool->get(id);
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
	int ratioDeaths = playerData->dmStats->deaths;
	if (ratioDeaths == 0)
		ratioDeaths = 1;
	auto formattedBody = fmt::sprintf(_(body, player),
		anotherPlayer->getName().to_string(), anotherPlayer->getID(),
		playerData->dmStats->score, playerData->dmStats->highestKillStreak,
		playerData->dmStats->kills, playerData->dmStats->deaths,
		float(playerData->dmStats->kills) / float(ratioDeaths),
		playerData->dmStats->handKills,
		playerData->dmStats->handheldWeaponKills,
		playerData->dmStats->meleeKills, playerData->dmStats->handgunKills,
		playerData->dmStats->shotgunKills, playerData->dmStats->smgKills,
		playerData->dmStats->assaultRiflesKills,
		playerData->dmStats->riflesKills, playerData->dmStats->heavyWeaponKills,
		playerData->dmStats->explosivesKills);

	auto dialog = std::shared_ptr<Core::MessageDialog>(new Core::MessageDialog(
		fmt::sprintf(DIALOG_HEADER_TITLE, _("DM stats", player)), formattedBody,
		_("OK", player), ""));
	this->dialogManager->showDialog(player, dialog,
		[](Core::DialogResult result)
		{
		});
}

void DeathmatchController::showRoomCreationDialog(IPlayer& player)
{
	auto playerData = Core::Player::getPlayerData(player);
	if (!playerData->tempData->deathmatch)
	{
		playerData->tempData->deathmatch = std::make_unique<PlayerTempData>();
	}
	if (!playerData->tempData->deathmatch->temporaryRoomSettings)
	{
		auto room = Room {
			.map = MAPS[0],
			.allowedWeapons = DEFAULT_WEAPON_SET.getWeapons(),
			.weaponSet = DEFAULT_WEAPON_SET,
			.host = player.getName().to_string(),
			.cbugEnabled = true,
			.countdown = std::chrono::minutes(DEFAULT_ROOM_ROUND_TIME_MIN),
			.defaultTime = std::chrono::minutes(DEFAULT_ROOM_ROUND_TIME_MIN),
			.privacyMode = PrivacyMode(PrivacyMode::Value::Everyone),
		};
		playerData->tempData->deathmatch->temporaryRoomSettings = { room };
	}
	auto room = playerData->tempData->deathmatch->temporaryRoomSettings;
	auto dialog = std::shared_ptr<Core::TabListDialog>(new Core::TabListDialog(
		fmt::sprintf(
			DIALOG_HEADER_TITLE, _("Create new deathmatch room", player)),
		{ { _("Map", player), room->map.name },
			{ _("Weapon set", player), room->weaponSet.toString(player) },
			{ _("Privacy mode", player), room->privacyMode.toString(player) },
			{ _("Is C-bug enabled?", player),
				room->cbugEnabled ? _("Yes", player) : _("No", player) },
			{ _("Round time", player),
				fmt::sprintf(_("%d minutes", player),
					std::chrono::duration_cast<std::chrono::minutes>(
						room->defaultTime)
						.count()) },
			{ _("Default health", player),
				std::to_string(
					static_cast<unsigned int>(room->defaultHealth)) },
			{ _("Default armor", player),
				std::to_string(static_cast<unsigned int>(room->defaultArmor)) },
			{ _("Refill player health", player),
				room->refillEnabled ? _("Yes", player) : _("No", player) },
			{ _("Random map", player),
				room->randomMap ? _("Yes", player) : _("No", player) },
			{ _("#YELLOW#Start", player) } },
		_("Select", player), _("Cancel", player)));
	this->dialogManager->showDialog(player, dialog,
		[this, playerData, &player](Core::DialogResult result)
		{
			if (result.response())
			{
				switch (result.listItem())
				{
				case 0:
				{
					this->showRoomMapSelectionDialog(player);
					break;
				}
				case 1:
				{
					this->showRoomWeaponSetSelectionDialog(player);
					break;
				}
				case 2:
				{
					this->showRoomPrivacyModeSelectionDialog(player);
					break;
				}
				case 3:
				{
					this->showRoomSetCbugEnabledDialog(player);
					break;
				}
				case 4:
				{
					this->showRoomSetRoundTimeDialog(player);
					break;
				}
				case 5:
				{
					this->showRoomSetHealthDialog(player);
					break;
				}
				case 6:
				{
					this->showRoomSetArmorDialog(player);
					break;
				}
				case 7:
				{
					this->showRoomSetRefillHealthDialog(player);
					break;
				}
				case 8:
				{
					this->showRoomSetRandomMapDialog(player);
					break;
				}
				case 9:
				{
					this->createRoom(player);
					break;
				}
				default:
					break;
				};
			}
			else
			{
				playerData->tempData->deathmatch->temporaryRoomSettings = {};
			}
		});
}

void DeathmatchController::showRoomMapSelectionDialog(IPlayer& player)
{
	auto playerData = Core::Player::getPlayerData(player);
	if (!playerData->tempData->deathmatch)
		return;
	if (!playerData->tempData->deathmatch->temporaryRoomSettings)
		return;

	std::vector<std::string> items;
	for (const auto& map : MAPS)
	{
		items.push_back(map.name);
	}
	auto dialog = std::shared_ptr<Core::ListDialog>(new Core::ListDialog(
		fmt::sprintf(DIALOG_HEADER_TITLE, _("Map selection", player)), items,
		_("Select", player), _("Cancel", player)));

	this->dialogManager->showDialog(player, dialog,
		[this, &player, playerData](Core::DialogResult result)
		{
			if (result.response())
			{
				playerData->tempData->deathmatch->temporaryRoomSettings->map
					= MAPS[result.listItem()];
				this->showRoomCreationDialog(player);
			}
			else
			{
				this->showRoomCreationDialog(player);
			}
		});
}

void DeathmatchController::showRoomWeaponSetSelectionDialog(IPlayer& player)
{
	auto playerData = Core::Player::getPlayerData(player);
	if (!playerData->tempData->deathmatch)
		return;
	if (!playerData->tempData->deathmatch->temporaryRoomSettings)
		return;

	auto dialog = std::shared_ptr<Core::ListDialog>(new Core::ListDialog(
		fmt::sprintf(DIALOG_HEADER_TITLE, _("Weapon set selection", player)),
		{ _("Run", player), _("DSS", player) }, _("Select", player),
		_("Cancel", player)));

	this->dialogManager->showDialog(player, dialog,
		[this, &player, playerData](Core::DialogResult result)
		{
			if (result.response())
			{
				WeaponSet set(magic_enum::enum_value<WeaponSet::Value>(
					result.listItem()));
				playerData->tempData->deathmatch->temporaryRoomSettings
					->weaponSet
					= set;
				playerData->tempData->deathmatch->temporaryRoomSettings
					->allowedWeapons
					= set.getWeapons();
				this->showRoomCreationDialog(player);
			}
			else
			{
				this->showRoomCreationDialog(player);
			}
		});
}

void DeathmatchController::showRoomPrivacyModeSelectionDialog(IPlayer& player)
{
	auto playerData = Core::Player::getPlayerData(player);
	if (!playerData->tempData->deathmatch)
		return;
	if (!playerData->tempData->deathmatch->temporaryRoomSettings)
		return;

	std::vector<std::string> items;
	auto modes = magic_enum::enum_values<PrivacyMode::Value>();
	for (auto mode : modes)
	{
		items.push_back(PrivacyMode(mode).toString(player));
	}

	auto dialog = std::shared_ptr<Core::ListDialog>(new Core::ListDialog(
		fmt::sprintf(DIALOG_HEADER_TITLE, _("Privacy mode selection", player)),
		items, _("Select", player), _("Cancel", player)));

	this->dialogManager->showDialog(player, dialog,
		[this, &player, playerData](Core::DialogResult result)
		{
			if (result.response())
			{
				PrivacyMode mode(magic_enum::enum_value<PrivacyMode::Value>(
					result.listItem()));
				playerData->tempData->deathmatch->temporaryRoomSettings
					->privacyMode
					= mode;
				this->showRoomCreationDialog(player);
			}
			else
			{
				this->showRoomCreationDialog(player);
			}
		});
}

void DeathmatchController::showRoomSetCbugEnabledDialog(IPlayer& player)
{
	auto playerData = Core::Player::getPlayerData(player);
	if (!playerData->tempData->deathmatch)
		return;
	if (!playerData->tempData->deathmatch->temporaryRoomSettings)
		return;

	std::string body = _("Yes", player) + "\n" + _("No", player) + "\n";
	auto dialog = std::shared_ptr<Core::ListDialog>(
		new Core::ListDialog(fmt::sprintf(DIALOG_HEADER_TITLE,
								 _("Should C-bug be enabled?", player)),
			{ _("Yes", player), _("No", player) }, _("Select", player),
			_("Cancel", player)));

	this->dialogManager->showDialog(player, dialog,
		[this, &player, playerData](Core::DialogResult result)
		{
			if (result.response())
			{
				bool enabled = true;
				switch (result.listItem())
				{
				case 0:
				{
					enabled = true;
					break;
				}
				case 1:
				{
					enabled = false;
					break;
				}
				}
				playerData->tempData->deathmatch->temporaryRoomSettings
					->cbugEnabled
					= enabled;
				this->showRoomCreationDialog(player);
			}
			else
			{
				this->showRoomCreationDialog(player);
			}
		});
}

void DeathmatchController::showRoomSetRoundTimeDialog(IPlayer& player)
{
	auto playerData = Core::Player::getPlayerData(player);
	if (!playerData->tempData->deathmatch)
		return;
	if (!playerData->tempData->deathmatch->temporaryRoomSettings)
		return;

	std::string body = _("Enter the round time in minutes (1-60):", player);
	auto dialog = std::shared_ptr<Core::InputDialog>(new Core::InputDialog(
		fmt::sprintf(DIALOG_HEADER_TITLE, _("Round time", player)), body, false,
		_("Enter", player), _("Cancel", player)));

	this->dialogManager->showDialog(player, dialog,
		[this, &player, playerData](Core::DialogResult result)
		{
			if (result.response())
			{
				auto playerExt = Core::Player::getPlayerExt(player);
				if (result.inputText() == "")
				{
					this->showRoomSetRoundTimeDialog(player);
					return;
				}
				int minutes;

				try
				{
					minutes = std::stoi(result.inputText());
				}
				catch (std::exception&)
				{
					this->showRoomSetRoundTimeDialog(player);
					return;
				}

				if (minutes <= 0 || minutes > 60)
				{
					playerExt->sendErrorMessage(_("Invalid number of minutes! "
												  "(available values are 1-60)",
						player));
					this->showRoomSetRoundTimeDialog(player);
					return;
				}

				playerData->tempData->deathmatch->temporaryRoomSettings
					->defaultTime
					= std::chrono::minutes(minutes);
				playerData->tempData->deathmatch->temporaryRoomSettings
					->countdown
					= std::chrono::minutes(minutes);
				this->showRoomCreationDialog(player);
			}
			else
			{
				this->showRoomCreationDialog(player);
			}
		});
}

void DeathmatchController::showRoomSetHealthDialog(IPlayer& player)
{
	auto playerData = Core::Player::getPlayerData(player);
	if (!playerData->tempData->deathmatch)
		return;
	if (!playerData->tempData->deathmatch->temporaryRoomSettings)
		return;

	std::string body
		= _("Enter the default HP for players in the room (1-100):", player);

	auto dialog = std::shared_ptr<Core::InputDialog>(new Core::InputDialog(
		fmt::sprintf(DIALOG_HEADER_TITLE, _("Default room HP", player)), body,
		false, _("Enter", player), _("Cancel", player)));

	this->dialogManager->showDialog(player, dialog,
		[this, &player, playerData](Core::DialogResult result)
		{
			if (result.response())
			{
				auto playerExt = Core::Player::getPlayerExt(player);
				if (result.inputText() == "")
				{
					this->showRoomSetHealthDialog(player);
					return;
				}
				int hp;

				try
				{
					hp = std::stoi(result.inputText());
				}
				catch (std::exception&)
				{
					this->showRoomSetHealthDialog(player);
					return;
				}

				if (hp <= 0 || hp > 100)
				{
					playerExt->sendErrorMessage(
						_("Invalid HP value! "
						  "(available values are 1-100)",
							player));
					this->showRoomSetHealthDialog(player);
					return;
				}

				playerData->tempData->deathmatch->temporaryRoomSettings
					->defaultHealth
					= static_cast<float>(hp);
				this->showRoomCreationDialog(player);
			}
			else
			{
				this->showRoomCreationDialog(player);
			}
		});
}

void DeathmatchController::showRoomSetArmorDialog(IPlayer& player)
{
	auto playerData = Core::Player::getPlayerData(player);
	if (!playerData->tempData->deathmatch)
		return;
	if (!playerData->tempData->deathmatch->temporaryRoomSettings)
		return;

	std::string body
		= _("Enter the default armor for players in the room (1-100):", player);

	auto dialog = std::shared_ptr<Core::InputDialog>(new Core::InputDialog(
		fmt::sprintf(DIALOG_HEADER_TITLE, _("Default room armor", player)),
		body, false, _("Enter", player), _("Cancel", player)));

	this->dialogManager->showDialog(player, dialog,
		[this, &player, playerData](Core::DialogResult result)
		{
			if (result.response())
			{
				auto playerExt = Core::Player::getPlayerExt(player);
				if (result.inputText() == "")
				{
					this->showRoomSetArmorDialog(player);
					return;
				}
				int armor;

				try
				{
					armor = std::stoi(result.inputText());
				}
				catch (std::exception&)
				{
					this->showRoomSetArmorDialog(player);
					return;
				}

				if (armor <= 0 || armor > 100)
				{
					playerExt->sendErrorMessage(
						_("Invalid armor value! "
						  "(available values are 1-100)",
							player));
					this->showRoomSetArmorDialog(player);
					return;
				}

				playerData->tempData->deathmatch->temporaryRoomSettings
					->defaultArmor
					= static_cast<float>(armor);
				this->showRoomCreationDialog(player);
			}
			else
			{
				this->showRoomCreationDialog(player);
			}
		});
}

void DeathmatchController::showRoomSetRefillHealthDialog(IPlayer& player)
{
	auto playerData = Core::Player::getPlayerData(player);
	if (!playerData->tempData->deathmatch)
		return;
	if (!playerData->tempData->deathmatch->temporaryRoomSettings)
		return;

	auto dialog = std::shared_ptr<Core::ListDialog>(
		new Core::ListDialog(fmt::sprintf(DIALOG_HEADER_TITLE,
								 _("Should refill be enabled?", player)),
			{ _("Yes", player), _("No", player) }, _("Select", player),
			_("Cancel", player)));

	this->dialogManager->showDialog(player, dialog,
		[this, &player, playerData](Core::DialogResult result)
		{
			if (result.response())
			{
				bool refillEnabled = true;
				switch (result.listItem())
				{
				case 0:
				{
					refillEnabled = true;
					break;
				}
				case 1:
				{
					refillEnabled = false;
					break;
				}
				}
				playerData->tempData->deathmatch->temporaryRoomSettings
					->refillEnabled
					= refillEnabled;
				this->showRoomCreationDialog(player);
			}
			else
			{
				this->showRoomCreationDialog(player);
			}
		});
}

void DeathmatchController::showRoomSetRandomMapDialog(IPlayer& player)
{
	auto playerData = Core::Player::getPlayerData(player);
	if (!playerData->tempData->deathmatch)
		return;
	if (!playerData->tempData->deathmatch->temporaryRoomSettings)
		return;

	auto dialog = std::shared_ptr<Core::ListDialog>(new Core::ListDialog(
		fmt::sprintf(DIALOG_HEADER_TITLE,
			_("Change map to random one after round ends?", player)),
		{ _("Yes", player), _("No", player) }, _("Select", player),
		_("Cancel", player)));

	this->dialogManager->showDialog(player, dialog,
		[this, &player, playerData](Core::DialogResult result)
		{
			if (result.response())
			{
				bool randomMap = true;
				switch (result.listItem())
				{
				case 0:
				{
					randomMap = true;
					break;
				}
				case 1:
				{
					randomMap = false;
					break;
				}
				}
				playerData->tempData->deathmatch->temporaryRoomSettings
					->randomMap
					= randomMap;
				this->showRoomCreationDialog(player);
			}
			else
			{
				this->showRoomCreationDialog(player);
			}
		});
}

void DeathmatchController::createRoom(IPlayer& player)
{
	auto playerData = Core::Player::getPlayerData(player);
	if (!playerData->tempData->deathmatch)
		return;
	if (!playerData->tempData->deathmatch->temporaryRoomSettings)
		return;

	auto room = playerData->tempData->deathmatch->temporaryRoomSettings;
	room->virtualWorld = VIRTUAL_WORLD_PREFIX + this->rooms.size();
	auto roomId = this->roomIdPool->allocateId();
	this->rooms[roomId] = std::make_shared<Room>(*room);
	this->coreManager.lock()->joinMode(
		player, Mode::Deathmatch, { { ROOM_INDEX, roomId } });
}

void DeathmatchController::deleteRoom(unsigned int roomId)
{
	if (!this->rooms.contains(roomId))
		return;
	auto room = this->rooms.at(roomId);
	if (room->roundStartTimer.has_value())
		room->roundStartTimer.value()->kill();
	this->rooms.erase(roomId);
	this->roomIdPool->freeId(roomId);
}

std::shared_ptr<TextDraws::DeathmatchTimer>
DeathmatchController::createDeathmatchTimer(IPlayer& player)
{
	auto playerExt = Core::Player::getPlayerExt(player);
	std::shared_ptr<TextDraws::DeathmatchTimer> timer(
		new TextDraws::DeathmatchTimer(player));
	playerExt->getTextDrawManager()->add(
		TextDraws::DeathmatchTimer::NAME, timer);
	return timer;
}

std::optional<std::shared_ptr<TextDraws::DeathmatchTimer>>
DeathmatchController::getDeathmatchTimer(IPlayer& player)
{
	auto playerExt = Core::Player::getPlayerExt(player);
	auto deathmatchTimerTxdAbstract = playerExt->getTextDrawManager()->get(
		TextDraws::DeathmatchTimer::NAME);
	if (!deathmatchTimerTxdAbstract.has_value())
	{
		spdlog::warn("player {} doesn't have deathmatch timer "
					 "textdraw, but he "
					 "is in dm room",
			player.getName().to_string());
		return {};
	}
	return std::dynamic_pointer_cast<TextDraws::DeathmatchTimer>(
		deathmatchTimerTxdAbstract.value());
}

void DeathmatchController::updateDeathmatchTimer(
	IPlayer& player, unsigned int roomIndex, std::shared_ptr<Room> room)
{
	auto playerData = Core::Player::getPlayerData(player);
	auto deathmatchTimerTxd = this->getDeathmatchTimer(player);
	if (!deathmatchTimerTxd.has_value())
		return;
	deathmatchTimerTxd.value()->update(
		fmt::sprintf(_("~w~Mode Deathmatch /DM %d", player), roomIndex + 1),
		playerData->tempData->deathmatch->kills,
		playerData->tempData->deathmatch->deaths,
		playerData->tempData->deathmatch->damageInflicted, room->countdown);
}

void DeathmatchController::onRoomJoin(IPlayer& player, unsigned int roomId)
{
	auto room = this->rooms.at(roomId);
	if (room->deletionTimer.has_value())
	{
		room->deletionTimer.value()->kill();
	}
	auto playerData = Core::Player::getPlayerData(player);

	playerData->tempData->deathmatch = std::make_unique<PlayerTempData>();
	playerData->tempData->deathmatch->roomId = roomId;
	room->players.emplace(&player);
	player.setHealth(room->defaultHealth);
	player.setArmour(room->defaultArmor);
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
		if (room->isStarting)
		{
			player.setControllable(false);
		}
	}

	room->sendMessageToAll(__("#LIME#>> #DEEP_SAFFRON#DM#LIGHT_GRAY#: Player "
							  "%s has "
							  "joined the room (%d players)"),
		player.getName().to_string(), room->players.size());
}

void DeathmatchController::onRoomLeave(IPlayer& player, unsigned int roomId)
{
	auto room = this->rooms.at(roomId);
	if (room->players.size() == 0 && room->host.has_value())
	{
		auto deletionTimer
			= this->_timersComponent->create(new Impl::SimpleTimerHandler(
												 [this, roomId]()
												 {
													 this->deleteRoom(roomId);
												 }),
				Seconds(30), false);
		room->deletionTimer = deletionTimer;
	}
}

void DeathmatchController::onNewRound(std::shared_ptr<Room> room)
{
	auto weaponSet = room->weaponSet;

	if (room->randomMap)
		room->map = randomlySelectMap(weaponSet);
	room->countdown = room->defaultTime;
	room->isRestarting = false;
	room->isStarting = true;

	for (auto player : room->players)
	{
		this->setRandomSpawnPoint(*player, room);
		player->setSpectating(false);
		auto playerData = Core::Player::getPlayerData(*player);
		playerData->tempData->deathmatch->resetKD();
		if (auto timer = this->getDeathmatchTimer(*player))
			timer.value()->show();
		player->setControllable(false);
	}

	room->roundStartTimerCount = 3;
	room->roundStartTimer = this->_timersComponent->create(
		new Impl::SimpleTimerHandler(
			[this, room]()
			{
				for (auto player : room->players)
				{
					player->sendGameText(
						fmt::sprintf("~w~%d", room->roundStartTimerCount),
						Seconds(1), 6);
				}
				if (room->roundStartTimerCount-- == 0)
				{
					room->roundStartTimer.value()->kill();
					room->roundStartTimer.reset();
					for (auto player : room->players)
					{
						player->sendGameText(
							_("~g~~h~~h~GO!", *player), Seconds(1), 6);
						player->setControllable(true);
					}
					room->cachedLastResult = {};
					room->isStarting = false;
				}
			}),
		Seconds(1), true);
	room->roundStartTimer.value()->trigger();
}

void DeathmatchController::onRoundEnd(std::shared_ptr<Room> room)
{
	room->isRestarting = true;
	std::vector<DeathmatchResult> resultArray;
	for (auto& player : room->players)
	{
		auto playerData = Core::Player::getPlayerData(*player);
		resultArray.push_back(
			DeathmatchResult { .playerName = player->getName().to_string(),
				.kills = playerData->tempData->deathmatch->kills,
				.deaths = playerData->tempData->deathmatch->deaths,
				.ratio = playerData->tempData->deathmatch->ratio,
				.damageInflicted
				= playerData->tempData->deathmatch->damageInflicted });
	}
	std::sort(resultArray.begin(), resultArray.end(),
		[](DeathmatchResult x1, DeathmatchResult x2)
		{
			return x1.kills > x2.kills;
		});

	std::vector<std::vector<std::string>> lastResults;
	for (std::size_t i = 0; i < resultArray.size(); i++)
	{
		auto playerResult = resultArray.at(i);
		lastResults.push_back({ fmt::sprintf(
									"%d. %s", i + 1, playerResult.playerName),
			fmt::sprintf("%d : %d", playerResult.kills, playerResult.deaths),
			fmt::sprintf("%.2f", playerResult.ratio),
			fmt::sprintf("%.2f", playerResult.damageInflicted) });
	}
	room->cachedLastResult = lastResults;

	for (auto& player : room->players)
	{
		player->setControllable(false);
		player->setSpectating(true);
		this->showRoundResultDialog(*player, room);
		if (auto timer = this->getDeathmatchTimer(*player))
		{
			timer.value()->hide();
		}
	}

	_timersComponent->create(
		new Impl::SimpleTimerHandler(
			std::bind(&DeathmatchController::onNewRound, this, room)),
		Seconds(5), false);

	this->bus->fire_event(Core::Utils::Events::RoundEndEvent {
		.mode = this->mode, .players = room->players });
}

void DeathmatchController::setRandomSpawnPoint(
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

void DeathmatchController::setupRoomForPlayer(
	IPlayer& player, std::shared_ptr<Room> room)
{
	player.setArmour(room->defaultArmor);
	player.setVirtualWorld(room->virtualWorld);
	player.setInterior(room->map.interiorID);
	player.setCameraBehind();
	player.setArmedWeapon(room->allowedWeapons[0]);
}

void DeathmatchController::removePlayerFromRoom(IPlayer& player)
{
	auto pData = Core::Player::getPlayerData(player);
	auto roomId = pData->tempData->deathmatch->roomId;
	auto room = this->rooms.at(roomId);
	room->players.erase(&player);
	this->onRoomLeave(player, roomId);

	pData->tempData->deathmatch.reset();

	auto playerExt = Core::Player::getPlayerExt(player);
	playerExt->getTextDrawManager()->destroy(TextDraws::DeathmatchTimer::NAME);

	room->sendMessageToAll(__("#LIME#>> #DEEP_SAFFRON#DM#LIGHT_GRAY#: Player "
							  "%s has left "
							  "the room (%d players)"),
		player.getName().to_string(), room->players.size());
}

void DeathmatchController::onTick()
{
	for (auto [id, room] : this->rooms)
	{
		if (!room)
			continue;
		if (room->isRestarting || room->isStarting)
			continue;
		if (room->countdown == std::chrono::seconds(0))
		{
			this->onRoundEnd(room);
		}
		if (!room->players.empty() && room->countdown > std::chrono::seconds(0)
			&& !room->isStarting)
		{
			room->countdown--;
		}
		for (auto player : room->players)
		{
			this->updateDeathmatchTimer(*player, id, room);
		}
	}
}
}
