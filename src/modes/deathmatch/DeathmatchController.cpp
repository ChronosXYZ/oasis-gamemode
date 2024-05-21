#include "DeathmatchController.hpp"
#include "Maps.hpp"
#include "PlayerTempData.hpp"
#include "Room.hpp"
#include "WeaponSet.hpp"
#include "../../core/utils/Localization.hpp"
#include "../../core/player/PlayerExtension.hpp"
#include "../../core/utils/Random.hpp"
#include "../../core/utils/Events.hpp"
#include "../../core/utils/Events.hpp"
#include "textdraws/DeathmatchTimer.hpp"
#include "DeathmatchResult.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
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
#include <eventbus/event_bus.hpp>

#include <cstddef>
#include <optional>
#include <vector>

#define PRESSED(newkeys, oldkeys, k)                                           \
	(((newkeys & (k)) == (k)) && ((oldkeys & (k)) != (k)))

#define CBUG_FREEZE_DELAY 1500
#define ROOM_INDEX "roomIndex"
#define DEFAULT_ROOM_ROUND_TIME_MIN 2

namespace Modes::Deathmatch
{
DeathmatchController::DeathmatchController(
	std::weak_ptr<Core::CoreManager> coreManager, IPlayerPool* playerPool,
	ITimersComponent* timersComponent, std::shared_ptr<dp::event_bus> bus)
	: super(Mode::Deathmatch, bus)
	, _coreManager(coreManager)
	, _playerPool(playerPool)
	, _timersComponent(timersComponent)
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
	super::onModeLeave(player);
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

void DeathmatchController::onPlayerDeath(
	IPlayer& player, IPlayer* killer, int reason)
{
	auto playerExt = Core::Player::getPlayerExt(player);
	auto playerData = Core::Player::getPlayerData(player);
	if (!playerExt->isInMode(Modes::Mode::Deathmatch))
		return;

	playerData->tempData->deathmatch->increaseDeaths();

	auto roomId = playerData->tempData->deathmatch->roomId;
	auto room = this->_rooms[roomId];
	if (killer)
	{
		auto killerData = Core::Player::getPlayerData(*killer);
		killerData->tempData->deathmatch->increaseKills();
		if (room->refillHealth)
			killer->setHealth(room->defaultHealth);
		// TODO add score to player stats
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
	if (this->_rooms.at(roomId)->cbugEnabled)
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
	auto room = this->_rooms[playerData->tempData->deathmatch->roomId];

	for (auto roomPlayer : room->players)
	{
		auto roomPlayerExt = Core::Player::getPlayerExt(*roomPlayer);
		roomPlayerExt->sendTranslatedMessageFormatted(
			__("#LIME#>> #RED#PLAYER ON FIRE#LIGHT_GRAY#: %s(%d) killed %s(%d) "
			   "and is now on fire!"),
			event.player.getName().to_string(), event.player.getID(),
			event.lastKillee.getName().to_string(), event.lastKillee.getID());
	}
}

DeathmatchController* DeathmatchController::create(
	std::weak_ptr<Core::CoreManager> coreManager, IPlayerPool* playerPool,
	ITimersComponent* timersComponent, std::shared_ptr<dp::event_bus> bus)
{
	auto controller = new DeathmatchController(
		coreManager, playerPool, timersComponent, bus);
	controller->initRooms();
	controller->initCommand();
	return controller;
}

void DeathmatchController::initCommand()
{
	this->_coreManager.lock()->getCommandManager()->addCommand(
		"dm",
		[&](std::reference_wrapper<IPlayer> player, int id)
		{
			auto playerExt = Core::Player::getPlayerExt(player.get());
			auto playerData = Core::Player::getPlayerData(player.get());
			if (id > this->_rooms.size() || id <= 0)
			{
				playerExt->sendErrorMessage(
					_("Such room doesn't exist!", player));
				return;
			}
			this->_coreManager.lock()->joinMode(player, Mode::Deathmatch,
				{ { ROOM_INDEX, std::size_t(id - 1) } });
		},
		Core::Commands::CommandInfo { .args = { __("room number") },
			.description = __("Enter DM room"),
			.category = MODE_NAME });
	this->_coreManager.lock()->getCommandManager()->addCommand(
		"dm",
		[&](std::reference_wrapper<IPlayer> player)
		{
			this->_coreManager.lock()->selectMode(player, Mode::Deathmatch);
		},
		Core::Commands::CommandInfo { .args = {},
			.description = __("Enter DM mode"),
			.category = MODE_NAME });
}

void DeathmatchController::initRooms()
{
	WeaponSet runWeaponSet(WeaponSet::Value::Run);
	WeaponSet walkWeaponSet(WeaponSet::Value::Walk);
	WeaponSet dssWeaponSet(WeaponSet::Value::DSS);

	this->_rooms = {
		std::shared_ptr<Room>(new Room {
			.map = randomlySelectMap(runWeaponSet),
			.allowedWeapons = runWeaponSet.getWeapons(),
			.weaponSet = runWeaponSet,
			.host = {},
			.virtualWorld = VIRTUAL_WORLD_PREFIX + 0,
			.cbugEnabled = true,
			.countdown = std::chrono::minutes(DEFAULT_ROOM_ROUND_TIME_MIN),
			.defaultTime = std::chrono::minutes(DEFAULT_ROOM_ROUND_TIME_MIN),
		}),
		std::shared_ptr<Room>(new Room {
			.map = randomlySelectMap(walkWeaponSet),
			.allowedWeapons = walkWeaponSet.getWeapons(),
			.weaponSet = walkWeaponSet,
			.host = {},
			.virtualWorld = VIRTUAL_WORLD_PREFIX + 1,
			.cbugEnabled = true,
			.countdown = std::chrono::minutes(DEFAULT_ROOM_ROUND_TIME_MIN),
			.defaultTime = std::chrono::minutes(DEFAULT_ROOM_ROUND_TIME_MIN),
		}),
		std::shared_ptr<Room>(new Room {
			.map = randomlySelectMap(dssWeaponSet),
			.allowedWeapons = dssWeaponSet.getWeapons(),
			.weaponSet = dssWeaponSet,
			.host = {},
			.virtualWorld = VIRTUAL_WORLD_PREFIX + 2,
			.cbugEnabled = false,
			.countdown = std::chrono::minutes(DEFAULT_ROOM_ROUND_TIME_MIN),
			.defaultTime = std::chrono::minutes(DEFAULT_ROOM_ROUND_TIME_MIN),
		}),
	};
}

void DeathmatchController::showRoomSelectionDialog(
	IPlayer& player, bool modeSelection)
{
	std::string body = _("#CREAM_BRULEE#Map\t#CREAM_BRULEE#Weapon\t#CREAM_"
						 "BRULEE#Host\t#CREAM_BRULEE#Players",
		player);
	body += "\n";
	body += _("Create custom room", player);
	body += "\n";
	for (std::size_t i = 0; i < _rooms.size(); i++)
	{
		auto room = _rooms[i];

		body += fmt::sprintf(
			"{999999}%d. {00FF00}%s\t{00FF00}%s\t{00FF00}%s\t{00FF00}%d\n",
			i + 1, room->map.name, room->weaponSet.toString(player),
			room->host.value_or(_("Server", player)), room->players.size());
	}
	this->_coreManager.lock()->getDialogManager()->createDialog(player,
		DialogStyle_TABLIST_HEADERS,
		fmt::sprintf(DIALOG_HEADER_TITLE, _("Select Deathmatch room", player)),
		body, _("Select", player), _("Close", player),
		[modeSelection, this, &player](
			DialogResponse response, int listItem, StringView input)
		{
			if (response)
			{
				if (listItem == 0)
				{
					this->showRoomCreationDialog(player);
					return;
				}
				if (listItem > this->_rooms.size())
				{
					// when player selected a room and this room has been
					// evicted
					this->showRoomSelectionDialog(player);
					return;
				}
				auto roomIndex = std::size_t(listItem - 1);
				this->_coreManager.lock()->joinMode(
					player, Mode::Deathmatch, { { ROOM_INDEX, roomIndex } });
			}
			else
			{
				if (modeSelection)
					this->_coreManager.lock()->showModeSelectionDialog(player);
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
	this->_coreManager.lock()->getDialogManager()->createDialog(player,
		DialogStyle_TABLIST_HEADERS,
		fmt::sprintf(DIALOG_HEADER_TITLE, _("Deathmatch statistics", player)),
		header + room->cachedLastResult.value(), _("Close", player), "",
		[this, &player, room](
			DialogResponse response, int listitem, StringView inputText)
		{
			if (room->isRestarting)
			{
				this->showRoundResultDialog(player, room);
			}
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
	auto dialogBody
		= fmt::sprintf(_("Map\t%s\nWeapon set\t%s\nPrivacy mode\t%s\nIs C-bug "
						 "enabled?\t%s\nRound time\t%d minutes\nDefault "
						 "health\t%d\nDefault armor\t%d\nRefill player "
						 "health\t%s\nRandom map\t%s\n#YELLOW#Start",
						   player),
			room->map.name, room->weaponSet.toString(player),
			room->privacyMode.toString(player),
			room->cbugEnabled ? _("Yes", player) : _("No", player),
			std::chrono::duration_cast<std::chrono::minutes>(room->defaultTime)
				.count(),
			static_cast<unsigned int>(room->defaultHealth),
			static_cast<unsigned int>(room->defaultArmor),
			room->refillHealth ? _("Yes", player) : _("No", player),
			room->randomMap ? _("Yes", player) : _("No", player));
	this->_coreManager.lock()->getDialogManager()->createDialog(player,
		DialogStyle::DialogStyle_TABLIST,
		fmt::sprintf(
			DIALOG_HEADER_TITLE, _("Create new deathmatch room", player)),
		dialogBody, _("Select", player), _("Cancel", player),
		[this, playerData, &player](
			DialogResponse response, int listitem, StringView inputText)
		{
			if (response)
			{
				switch (listitem)
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

	std::string body;

	for (const auto& map : MAPS)
	{
		body += map.name + "\n";
	}

	this->_coreManager.lock()->getDialogManager()->createDialog(player,
		DialogStyle::DialogStyle_LIST,
		fmt::sprintf(DIALOG_HEADER_TITLE, _("Map selection", player)), body,
		"Select", "Cancel",
		[this, &player, playerData](
			DialogResponse response, int listitem, StringView inputText)
		{
			if (response)
			{
				playerData->tempData->deathmatch->temporaryRoomSettings->map
					= MAPS[listitem];
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

	auto body = _("Run", player) + "\n";
	body += _("Walk", player) + "\n";
	body += _("DSS", player);

	this->_coreManager.lock()->getDialogManager()->createDialog(player,
		DialogStyle::DialogStyle_LIST,
		fmt::sprintf(DIALOG_HEADER_TITLE, _("Weapon set selection", player)),
		body, _("Select", player), _("Cancel", player),
		[this, &player, playerData](
			DialogResponse response, int listitem, StringView inputText)
		{
			if (response)
			{
				WeaponSet set(
					magic_enum::enum_value<WeaponSet::Value>(listitem));
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

	std::string body;
	auto modes = magic_enum::enum_values<PrivacyMode::Value>();
	for (auto mode : modes)
	{
		body += PrivacyMode(mode).toString(player) + "\n";
	}

	this->_coreManager.lock()->getDialogManager()->createDialog(player,
		DialogStyle::DialogStyle_LIST,
		fmt::sprintf(DIALOG_HEADER_TITLE, _("Privacy mode selection", player)),
		body, _("Select", player), _("Cancel", player),
		[this, &player, playerData](
			DialogResponse response, int listitem, StringView inputText)
		{
			if (response)
			{
				PrivacyMode mode(
					magic_enum::enum_value<PrivacyMode::Value>(listitem));
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

	this->_coreManager.lock()->getDialogManager()->createDialog(player,
		DialogStyle::DialogStyle_LIST,
		fmt::sprintf(
			DIALOG_HEADER_TITLE, _("Should C-bug be enabled?", player)),
		body, _("Select", player), _("Cancel", player),
		[this, &player, playerData](
			DialogResponse response, int listitem, StringView inputText)
		{
			if (response)
			{
				bool enabled = true;
				switch (listitem)
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

	this->_coreManager.lock()->getDialogManager()->createDialog(player,
		DialogStyle::DialogStyle_INPUT,
		fmt::sprintf(DIALOG_HEADER_TITLE, _("Round time", player)), body,
		_("Enter", player), _("Cancel", player),
		[this, &player, playerData](
			DialogResponse response, int listitem, StringView inputText)
		{
			if (response)
			{
				auto playerExt = Core::Player::getPlayerExt(player);
				if (inputText == "")
				{
					this->showRoomSetRoundTimeDialog(player);
					return;
				}
				int minutes;

				try
				{
					minutes = std::stoi(inputText.to_string());
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

	this->_coreManager.lock()->getDialogManager()->createDialog(player,
		DialogStyle::DialogStyle_INPUT,
		fmt::sprintf(DIALOG_HEADER_TITLE, _("Default room HP", player)), body,
		_("Enter", player), _("Cancel", player),
		[this, &player, playerData](
			DialogResponse response, int listitem, StringView inputText)
		{
			if (response)
			{
				auto playerExt = Core::Player::getPlayerExt(player);
				if (inputText == "")
				{
					this->showRoomSetHealthDialog(player);
					return;
				}
				int hp;

				try
				{
					hp = std::stoi(inputText.to_string());
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

	this->_coreManager.lock()->getDialogManager()->createDialog(player,
		DialogStyle::DialogStyle_INPUT,
		fmt::sprintf(DIALOG_HEADER_TITLE, _("Default room armor", player)),
		body, _("Enter", player), _("Cancel", player),
		[this, &player, playerData](
			DialogResponse response, int listitem, StringView inputText)
		{
			if (response)
			{
				auto playerExt = Core::Player::getPlayerExt(player);
				if (inputText == "")
				{
					this->showRoomSetArmorDialog(player);
					return;
				}
				int armor;

				try
				{
					armor = std::stoi(inputText.to_string());
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

	std::string body = _("Yes", player) + "\n" + _("No", player) + "\n";

	this->_coreManager.lock()->getDialogManager()->createDialog(player,
		DialogStyle::DialogStyle_LIST,
		fmt::sprintf(DIALOG_HEADER_TITLE,
			_("Should player health be refilled when they kill someone?",
				player)),
		body, _("Select", player), _("Cancel", player),
		[this, &player, playerData](
			DialogResponse response, int listitem, StringView inputText)
		{
			if (response)
			{
				bool refillHealth = true;
				switch (listitem)
				{
				case 0:
				{
					refillHealth = true;
					break;
				}
				case 1:
				{
					refillHealth = false;
					break;
				}
				}
				playerData->tempData->deathmatch->temporaryRoomSettings
					->refillHealth
					= refillHealth;
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

	std::string body = _("Yes", player) + "\n" + _("No", player) + "\n";

	this->_coreManager.lock()->getDialogManager()->createDialog(player,
		DialogStyle::DialogStyle_LIST,
		fmt::sprintf(DIALOG_HEADER_TITLE,
			_("Change map to random one after round ends?", player)),
		body, _("Select", player), _("Cancel", player),
		[this, &player, playerData](
			DialogResponse response, int listitem, StringView inputText)
		{
			if (response)
			{
				bool randomMap = true;
				switch (listitem)
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
	room->virtualWorld = VIRTUAL_WORLD_PREFIX + this->_rooms.size();
	auto roomId = this->_rooms.size();
	this->_rooms.push_back(std::make_shared<Room>(*room));
	this->_coreManager.lock()->joinMode(
		player, Mode::Deathmatch, { { ROOM_INDEX, roomId } });
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
		spdlog::warn("player {} doesn't have deathmatch timer textdraw, but he "
					 "is in dm room",
			player.getName().to_string());
		return {};
	}
	return std::dynamic_pointer_cast<TextDraws::DeathmatchTimer>(
		deathmatchTimerTxdAbstract.value());
}

void DeathmatchController::updateDeathmatchTimer(
	IPlayer& player, std::size_t roomIndex, std::shared_ptr<Room> room)
{
	auto playerData = Core::Player::getPlayerData(player);
	auto deathmatchTimerTxd = this->getDeathmatchTimer(player);
	if (!deathmatchTimerTxd.has_value())
		return;
	deathmatchTimerTxd.value()->update(
		fmt::sprintf(_("~w~Mode Deathmatch /DM %d", player), roomIndex + 1),
		playerData->tempData->deathmatch->kills,
		playerData->tempData->deathmatch->deaths, 0.0, room->countdown);
}

void DeathmatchController::onRoomJoin(IPlayer& player, std::size_t roomId)
{
	auto room = this->_rooms.at(roomId);
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
	}

	for (auto roomPlayer : room->players)
	{
		Core::Player::getPlayerExt(*roomPlayer)
			->sendTranslatedMessageFormatted(
				__("#LIME#>> #DEEP_SAFFRON#DM#LIGHT_GRAY#: Player %s has "
				   "joined the room (%d players)"),
				player.getName().to_string(), room->players.size());
	}
}

void DeathmatchController::onNewRound(std::shared_ptr<Room> room)
{
	auto weaponSet = room->weaponSet;
	if (room->weaponSet
		== WeaponSet(WeaponSet::Value::DSS)) // use maps from Walk weapon set
		weaponSet = WeaponSet(WeaponSet::Value::Walk);

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
	}

	// TODO replace thread with coroutine
	std::thread(
		[room, this]()
		{
			for (int i = 3; i > 0; i--)
			{
				for (auto player : room->players)
				{
					player->setControllable(false);
					player->sendGameText(
						fmt::sprintf("~w~%d", i), Seconds(1), 6);
				}
				using namespace std::chrono_literals;
				std::this_thread::sleep_for(1s);
			}
			for (auto player : room->players)
			{
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

	std::string lastResults;
	for (std::size_t i = 0; i < resultArray.size(); i++)
	{
		auto playerResult = resultArray.at(i);
		lastResults += fmt::sprintf("%d. %s\t%d : %d\t%.2f\t%.2f\n", i + 1,
			playerResult.playerName, playerResult.kills, playerResult.deaths,
			playerResult.ratio, playerResult.damageInflicted);
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
	auto room = this->_rooms[pData->tempData->deathmatch->roomId];
	room->players.erase(&player);

	pData->tempData->deathmatch.reset();

	auto playerExt = Core::Player::getPlayerExt(player);
	playerExt->getTextDrawManager()->destroy(TextDraws::DeathmatchTimer::NAME);

	for (auto roomPlayer : room->players)
	{
		Core::Player::getPlayerExt(*roomPlayer)
			->sendTranslatedMessageFormatted(
				__("#LIME#>> #DEEP_SAFFRON#DM#LIGHT_GRAY#: Player %s has left "
				   "the room (%d players)"),
				player.getName().to_string(), room->players.size());
	}
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
		if (!room->players.empty() && room->countdown > std::chrono::seconds(0)
			&& !room->isStarting)
		{
			room->countdown--;
		}
		for (auto player : room->players)
		{
			this->updateDeathmatchTimer(*player, i, room);
		}
	}
}
}
