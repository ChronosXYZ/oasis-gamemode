#include "FreeroamController.hpp"
#include "../../core/player/PlayerExtension.hpp"
#include "../../core/utils/VehicleList.hpp"
#include "FreeroamVehicles.hpp"
#include "component.hpp"
#include "eventbus/event_bus.hpp"
#include "types.hpp"

#include <cstdlib>
#include <fmt/printf.h>
#include <magic_enum/magic_enum.hpp>
#include <player.hpp>
#include <Server/Components/Vehicles/vehicles.hpp>
#include <Server/Components/Classes/classes.hpp>

#include <functional>
#include <memory>
#include <scn/scan.h>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

namespace Modes::Freeroam
{
FreeroamController::FreeroamController(IComponentList* components,
	IPlayerPool* playerPool,
	std::shared_ptr<Core::Utils::IDPool> virtualWorldIdPool,
	std::weak_ptr<Core::ModeManager> modeManager,
	std::shared_ptr<Core::DialogManager> dialogManager,
	std::shared_ptr<Core::Commands::CommandManager> commandManager,
	std::shared_ptr<dp::event_bus> bus)
	: super(Mode::Freeroam, bus, playerPool)
	, modeManager(modeManager)
	, vehiclesComponent(components->queryComponent<IVehiclesComponent>())
	, playerPool(playerPool)
	, dialogManager(dialogManager)
	, commandManager(commandManager)
	, virtualWorldId(virtualWorldIdPool->allocateId())
{
	playerPool->getPlayerSpawnDispatcher().addEventHandler(this);
	playerPool->getPlayerDamageDispatcher().addEventHandler(this);

	this->initCommands();
	this->initVehicles();
}

FreeroamController::~FreeroamController()
{
	playerPool->getPlayerSpawnDispatcher().removeEventHandler(this);
	playerPool->getPlayerDamageDispatcher().removeEventHandler(this);
}

void FreeroamController::onPlayerSpawn(IPlayer& player)
{
	auto playerExt = Core::Player::getPlayerExt(player);
	if (!playerExt->isInMode(Mode::Freeroam))
	{
		return;
	}
}

void FreeroamController::onModeJoin(IPlayer& player,
	std::unordered_map<std::string, Core::PrimitiveType> joinData)
{
	super::onModeJoin(player, joinData);
	setupSpawn(player);
	player.spawn();
}

void FreeroamController::initCommands()
{
	this->commandManager->addCommand(
		"fr",
		[&](std::reference_wrapper<IPlayer> player, std::string args)
		{
			if (!args.empty())
				return false;
			this->modeManager.lock()->selectMode(player, Mode::Freeroam);
			return true;
		},
		Core::Commands::CommandInfo { .args = {},
			.description = __("Teleports player to the Freeroam mode"),
			.category = MODE_NAME });

	this->commandManager->addCommand(
		"v",
		[&](std::reference_wrapper<IPlayer> player, std::string args)
		{
			auto result = scn::scan<int, int, int>(args, "{} {} {}");
			if (!result)
				return false;

			auto [modelId, color1, color2] = result->values();

			auto playerExt = Core::Player::getPlayerExt(player);
			if (!playerExt->isInMode(Mode::Freeroam))
			{
				playerExt->sendErrorMessage(
					__("You can spawn vehicles only in Freeroam mode!"));
				return true;
			}
			if (modelId < 400 || modelId > 611)
			{
				playerExt->sendErrorMessage(__("Invalid car model ID!"));
				return true;
			}
			if (color1 < 0 || color1 > 255 || color2 < 0 || color2 > 255)
			{
				playerExt->sendErrorMessage(__("Invalid color ID!"));
				return true;
			}

			if (player.get().getState() == PlayerState_Driver)
			{
				player.get().removeFromVehicle(true);
			}
			this->deleteLastSpawnedCar(player);

			auto playerPosition = player.get().getPosition();
			auto vehicle = vehiclesComponent->create(false, modelId,
				playerPosition, 0.0, color1, color2, Seconds(60000));
			vehicle->putPlayer(player, 0);
			playerExt->sendInfoMessage(
				_("You have sucessfully spawned the vehicle!", player));
			playerExt->getPlayerData()->tempData->freeroam->lastVehicleId
				= vehicle->getID();

			return true;
		},
		Core::Commands::CommandInfo {
			.args = { __("vehicle model id"), __("color 1"), __("color 2") },
			.description = __("Spawns a vehicle"),
			.category = MODE_NAME });
	this->commandManager->addCommand(
		"v",
		[this](std::reference_wrapper<IPlayer> player, std::string args)
		{
			if (!args.empty())
				return false;

			auto playerExt = Core::Player::getPlayerExt(player);
			if (!playerExt->isInMode(Mode::Freeroam))
			{
				playerExt->sendErrorMessage(
					__("You can spawn vehicles only in Freeroam mode!"));
				return true;
			}
			this->showVehicleSpawningDialog(player);
			return true;
		},
		Core::Commands::CommandInfo { .args = {},
			.description = __("Shows dialog with vehicle list for spawning"),
			.category = MODE_NAME });
	this->commandManager->addCommand(
		"kill",
		[](std::reference_wrapper<IPlayer> player, std::string args)
		{
			if (!args.empty())
				return false;

			player.get().setHealth(0.0);
			Core::Player::getPlayerExt(player.get())
				->sendInfoMessage(__("You have killed yourself!"));
			return true;
		},
		Core::Commands::CommandInfo {
			.args = {},
			.description = __("Kill yourself"),
			.category = MODE_NAME,
		});
	this->commandManager->addCommand(
		"skin",
		[&](std::reference_wrapper<IPlayer> player, std::string args)
		{
			auto result = scn::scan<int>(args, "{}");
			if (!result)
				return false;
			auto [skinId] = result->values();

			auto playerExt = Core::Player::getPlayerExt(player);
			if (skinId < 0 || skinId > 311)
			{
				playerExt->sendErrorMessage(__("Invalid skin ID!"));
				return true;
			}
			player.get().setSkin(skinId);
			auto data = Core::Player::getPlayerData(player.get());
			data->lastSkinId = skinId;
			playerExt->sendInfoMessage(fmt::sprintf(
				_("You have changed your skin to ID: %d!", player), skinId));
			return true;
		},
		Core::Commands::CommandInfo {
			.args = { __("skin ID") },
			.description = __("Set player skin"),
			.category = MODE_NAME,
		});
	this->commandManager->addCommand(
		"w",
		[&](std::reference_wrapper<IPlayer> player, std::string args)
		{
			auto dialog = Core::ListDialogBuilder()
							  .setTitle(fmt::sprintf(DIALOG_HEADER_TITLE,
								  _("Weapon set editor", player)))
							  .setItems({
								  _("Create", player),
								  _("Load", player),
								  _("Edit", player),
								  _("Delete", player),
							  })
							  .setLeftButton(_("Ok", player))
							  .setRightButton(_("Cancel", player))
							  .build();
			this->dialogManager->showDialog(player, dialog,
				[this, player](auto result)
				{
					if (!result.response())
						return;
					switch (result.listItem())
					{
					case 0:
					{
						this->showCreateWeaponSetDialog(player);
						break;
					}
					default:
					{
						break;
					}
					};
				});
			return true;
		},
		Core::Commands::CommandInfo {
			.args = {},
			.description = __("Weapon set editor"),
			.category = MODE_NAME,
		});
	this->commandManager->addCommand(
		"pm",
		[&](std::reference_wrapper<IPlayer> player, std::string args)
		{
			auto result = scn::scan<int, std::string>(args, "{} {}");
			if (!result)
				return false;
			auto [recipientId, message] = result->values();
			auto recipient = this->playerPool->get(recipientId);
			auto senderExt = Core::Player::getPlayerExt(player);
			if (!Core::Player::getPlayerData(player)->settings->pmsEnabled)
			{
				senderExt->sendErrorMessage(
					__("Your PMs are disabled. Enable them to send PMs."));
				return true;
			}
			if (!recipient)
			{
				senderExt->sendErrorMessage(__("This ID is not online!"));
				return true;
			}
			if (!Core::Player::getPlayerData(*recipient)->settings->pmsEnabled)
			{
				senderExt->sendErrorMessage(
					__("This player has disabled their PMs."));
				return true;
			}
			auto senderName = Core::Player::getPlayerData(player)->name;
			int senderId = player.get().getID();
			auto recipientExt = Core::Player::getPlayerExt(*recipient);
			recipientExt->sendInfoMessage(
				__("[PM] %s (%d): %s"), senderName, senderId, message);
			return true;
		},
		Core::Commands::CommandInfo {
			.args = { __("recipient ID"), __("message content") },
			.description = __("Send a private message"),
			.category = MODE_NAME,
		});
	this->commandManager->addCommand(
		"pms",
		[&](std::reference_wrapper<IPlayer> player, std::string args)
		{
			auto playerData = Core::Player::getPlayerData(player);
			auto playerExt = Core::Player::getPlayerExt(player);
			playerData->settings->pmsEnabled
				= !(playerData->settings->pmsEnabled);
			if (playerData->settings->pmsEnabled)
				playerExt->sendInfoMessage(__("You have enabled your PMs."));
			else
				playerExt->sendInfoMessage(__("You have disabled your DMs."));
			return true;
		},
		Core::Commands::CommandInfo {
			.args = {},
			.description = __("Toggle private messages"),
			.category = MODE_NAME,
		});
	this->commandManager->addCommand(
		"settings",
		[&](std::reference_wrapper<IPlayer> player, std::string args)
		{
			auto playerData = Core::Player::getPlayerData(player);
			auto dialog = Core::SettingsDialogBuilder(player, this->dialogManager)
				.setTitle(fmt::sprintf(DIALOG_HEADER_TITLE,
				_("Player settings", player)))
				.setItems(std::vector<std::shared_ptr<Core::SettingItem>>
				{
					std::make_shared<Core::SettingBooleanItem>("pmsEnabled",
					"Private messages", "Enable private messages?", "Yes", "No"),
					std::make_shared<Core::SettingEnumItem>("notificationPos",
					_("Notification position", player),
					std::vector<Core::SettingEnumItem::EnumChoice> {
						Core::SettingEnumItem::EnumChoice {
							.id = "BOTTOM",
							.text = __("Bottom")
						},
						Core::SettingEnumItem::EnumChoice {
							.id = "TOP",
							.text = __("Top")
						},
						Core::SettingEnumItem::EnumChoice {
							.id = "LEFT",
							.text = __("Left")
						},
						Core::SettingEnumItem::EnumChoice {
							.id = "RIGHT",
							.text = __("Right")
						}
					}, true),
					std::make_shared<Core::SettingBooleanItem>("notifyOnGiveDamage",
					"Hit notifications", "Notify when dealing damage?", "Yes", "No")
				})
				.setInitialValues(std::unordered_map<std::string, Core::SettingValue> 
				{
					{"pmsEnabled", playerData->settings->pmsEnabled},
					{"notificationPos", [playerData]
						{
							switch ((int)playerData->settings->notificationPos)
							{
								case (int)Core::TextDraws::NotificationPosition::Bottom:
									return "BOTTOM";
								case (int)Core::TextDraws::NotificationPosition::Top:
									return "TOP";
								case (int)Core::TextDraws::NotificationPosition::Left:
									return "LEFT";
								case (int)Core::TextDraws::NotificationPosition::Right:
									return "RIGHT";
								default:
									return "BOTTOM";
							}
						}()
					},
					{"notifyOnGiveDamage", playerData->settings->notifyOnGiveDamage}
				})
				.setOnConfigurationChanged([playerData](std::pair<std::string, Core::SettingValue> pair) 
				{
					std::string item = std::get<std::string>(pair);
					Core::SettingValue state = std::get<Core::SettingValue>(pair);
					// if (item that we need) our field = std::get<our type>(state);
					if (item == "pmsEnabled") playerData->settings->pmsEnabled = std::get<bool>(state);
					if (item == "notificationPos") {
						std::string pos = std::get<std::string>(state);
						if (pos == "BOTTOM") 
							playerData->settings->notificationPos = Core::TextDraws::NotificationPosition::Bottom;
						else if (pos == "TOP")
							playerData->settings->notificationPos = Core::TextDraws::NotificationPosition::Top;
						else if (pos == "LEFT")
							playerData->settings->notificationPos = Core::TextDraws::NotificationPosition::Left;
						else if (pos == "RIGHT")
							playerData->settings->notificationPos = Core::TextDraws::NotificationPosition::Right;
					};
					if (item == "notifyOnGiveDamage") playerData->settings->notifyOnGiveDamage = std::get<bool>(state);
				})
				.setLeftButton(__("Select"))
				.setRightButton(__("Exit"))
				.build();
			dialog->show();
			return true;
		},
		Core::Commands::CommandInfo {
			.args = {},
			.description = __("Edit player settings"),
			.category = MODE_NAME,
		});
		this->commandManager->addCommand(
			"notify",
			[&](std::reference_wrapper<IPlayer> player, std::string args){
				auto playerExt = Core::Player::getPlayerExt(player);
				playerExt->showNotification(args);
				return true;
			},
			Core::Commands::CommandInfo {
				.args = { __("text") },
				.description = __("Notify yourself"),
				.category = "test"
			}
		);
}

void FreeroamController::initVehicles()
{
	std::vector<std::vector<Vehicle>> vehiclesList
		= { GEN_LS_INNER, GEN_LS_OUTER, LS_LAW, LS_AIRPORT, BONE, FLINT,
			  LV_AIRPORT, LV_GEN, LV_LAW, PILOTS, RED_COUNTY, SF_AIRPORT,
			  SF_GEN, SF_LAW, SF_TRAIN, TIERRA, TRAINS, WHETSTONE };
	for (auto list : vehiclesList)
	{
		for (auto v : list)
		{
			auto vehicle = this->vehiclesComponent->create(VehicleSpawnData {
				.respawnDelay = Minutes(30),
				.modelID = v.vehicleType,
				.position = Vector3(v.position.x, v.position.y, v.position.z),
				.zRotation = v.position.w,
				.colour1 = v.color1,
				.colour2 = v.color2,

			});
			vehicle->setVirtualWorld(this->virtualWorldId);
			vehicle->setPlate(
				fmt::sprintf("oasis{44AA33}%d", vehicle->getID()));
		}
	}
}

void FreeroamController::onPlayerDeath(
	IPlayer& player, IPlayer* killer, int reason)
{
	auto playerExt = Core::Player::getPlayerExt(player);
	if (!playerExt->isInMode(Mode::Freeroam))
	{
		return;
	}
	setupSpawn(player);
}

void FreeroamController::setupSpawn(IPlayer& player)
{
	player.setControllable(true);
	player.setVirtualWorld(this->virtualWorldId);
	player.setInterior(0);
	auto classData = queryExtension<IPlayerClassData>(player);
	classData->setSpawnInfo(
		PlayerClass(Core::Player::getPlayerData(player)->lastSkinId, TEAM_NONE,
			SPAWN_LOCATION, SPAWN_ANGLE, WeaponSlots {}));
}

void FreeroamController::onModeLeave(IPlayer& player)
{
	super::onModeLeave(player);
	this->deleteLastSpawnedCar(player);
}

void FreeroamController::onModeSelect(IPlayer& player)
{
	this->modeManager.lock()->joinMode(player, Modes::Mode::Freeroam, {});
}

void FreeroamController::onPlayerSave(
	std::shared_ptr<Core::PlayerModel> data, pqxx::work& txn)
{
	// TODO
}

void FreeroamController::onPlayerLoad(
	std::shared_ptr<Core::PlayerModel> data, pqxx::work& txn)
{
	// TODO
}

void FreeroamController::deleteLastSpawnedCar(IPlayer& player)
{
	auto playerData = Core::Player::getPlayerData(player);
	if (auto lastVehicleId = playerData->tempData->freeroam->lastVehicleId)
	{
		auto vehicle = vehiclesComponent->get(lastVehicleId.value());
		vehiclesComponent->release(lastVehicleId.value());
		playerData->tempData->freeroam->lastVehicleId.reset();
	}
}

void FreeroamController::showVehicleSpawningDialog(IPlayer& player)
{
	std::vector<std::vector<std::string>> items = {
		{ fmt::sprintf("{999999}%d. {FFFFFF}%s", 1, _("Aircraft", player)),
			fmt::sprintf("{999999}%d",
				Core::Utils::VEHICLE_LIST.at(Core::Utils::VehicleType::Aircraft)
					.size()) },
		{ fmt::sprintf("{999999}%d. {FFFFFF}%s", 2, _("Helicopter", player)),
			fmt::sprintf("{999999}%d",
				Core::Utils::VEHICLE_LIST
					.at(Core::Utils::VehicleType::Helicopter)
					.size()) },
		{ fmt::sprintf("{999999}%d. {FFFFFF}%s", 3, _("Bike", player)),
			fmt::sprintf("{999999}%d",
				Core::Utils::VEHICLE_LIST.at(Core::Utils::VehicleType::Bike)
					.size()) },
		{ fmt::sprintf("{999999}%d. {FFFFFF}%s", 4, _("Convertible", player)),
			fmt::sprintf("{999999}%d",
				Core::Utils::VEHICLE_LIST
					.at(Core::Utils::VehicleType::Convertible)
					.size()) },
		{ fmt::sprintf("{999999}%d. {FFFFFF}%s", 5, _("Industrial", player)),
			fmt::sprintf("{999999}%d",
				Core::Utils::VEHICLE_LIST
					.at(Core::Utils::VehicleType::Industrial)
					.size()) },
		{ fmt::sprintf("{999999}%d. {FFFFFF}%s", 6, _("Lowrider", player)),
			fmt::sprintf("{999999}%d",
				Core::Utils::VEHICLE_LIST.at(Core::Utils::VehicleType::Lowrider)
					.size()) },
		{ fmt::sprintf("{999999}%d. {FFFFFF}%s", 7, _("Off Road", player)),
			fmt::sprintf("{999999}%d",
				Core::Utils::VEHICLE_LIST.at(Core::Utils::VehicleType::OffRoad)
					.size()) },
		{ fmt::sprintf(
			  "{999999}%d. {FFFFFF}%s", 8, _("Public Service", player)),
			fmt::sprintf("{999999}%d",
				Core::Utils::VEHICLE_LIST
					.at(Core::Utils::VehicleType::PublicService)
					.size()) },
		{ fmt::sprintf("{999999}%d. {FFFFFF}%s", 9, _("Saloon", player)),
			fmt::sprintf("{999999}%d",
				Core::Utils::VEHICLE_LIST.at(Core::Utils::VehicleType::Saloon)
					.size()) },
		{ fmt::sprintf("{999999}%d. {FFFFFF}%s", 10, _("Sport", player)),
			fmt::sprintf("{999999}%d",
				Core::Utils::VEHICLE_LIST
					.at(Core::Utils::VehicleType::SportsCar)
					.size()) },
		{ fmt::sprintf(
			  "{999999}%d. {FFFFFF}%s", 11, _("Station Wagon", player)),
			fmt::sprintf("{999999}%d",
				Core::Utils::VEHICLE_LIST
					.at(Core::Utils::VehicleType::StationWagon)
					.size()) },
		{ fmt::sprintf("{999999}%d. {FFFFFF}%s", 12, _("Boat", player)),
			fmt::sprintf("{999999}%d",
				Core::Utils::VEHICLE_LIST.at(Core::Utils::VehicleType::Boat)
					.size()) },
		{ fmt::sprintf("{999999}%d. {FFFFFF}%s", 13, _("Trailer", player)),
			fmt::sprintf("{999999}%d",
				Core::Utils::VEHICLE_LIST.at(Core::Utils::VehicleType::Trailer)
					.size()) },
		{ fmt::sprintf(
			  "{999999}%d. {FFFFFF}%s", 14, _("Unique Vehicle", player)),
			fmt::sprintf("{999999}%d",
				Core::Utils::VEHICLE_LIST.at(Core::Utils::VehicleType::Unique)
					.size()) },
		{ fmt::sprintf("{999999}%d. {FFFFFF}%s", 15, _("RC Vehicle", player)),
			fmt::sprintf("{999999}%d",
				Core::Utils::VEHICLE_LIST.at(Core::Utils::VehicleType::RC)
					.size()) }
	};

	auto dialog = std::shared_ptr<Core::TabListHeadersDialog>(
		new Core::TabListHeadersDialog(
			fmt::sprintf(DIALOG_HEADER_TITLE, _("Select vehicle type", player)),
			{ _("Vehicle type", player), _("Total vehicles", player) }, items,
			_("Select", player), _("Cancel", player)));
	this->dialogManager->showDialog(player, dialog,
		[this, &player](Core::DialogResult result)
		{
			if (!result.response())
				return;
			this->showVehicleListDialog(player,
				magic_enum::enum_value<Core::Utils::VehicleType>(
					result.listItem()));
		});
};

void FreeroamController::showVehicleListDialog(
	IPlayer& player, Core::Utils::VehicleType vehicleTypeSelected)
{
	std::vector<std::vector<std::string>> items;

	int i = 1;
	for (auto v : Core::Utils::VEHICLE_LIST.at(vehicleTypeSelected))
	{
		items.push_back({ fmt::sprintf("{999999}%d. {FFFFFF}%s", i, v.name),
			fmt::sprintf("{999999}%d", v.modelId) });
		i++;
	}

	auto dialog = std::shared_ptr<Core::TabListHeadersDialog>(
		new Core::TabListHeadersDialog(
			fmt::sprintf(DIALOG_HEADER_TITLE, _("Select vehicle", player)),
			{ _("#. Vehicle name", player), _("Vehicle ID", player) }, items,
			_("Select", player), _("Cancel", player)));
	this->dialogManager->showDialog(player, dialog,
		[this, &player, vehicleTypeSelected](Core::DialogResult result)
		{
			if (result.response())
			{
				auto playerExt = Core::Player::getPlayerExt(player);
				if (player.getState() == PlayerState_Driver)
				{
					player.removeFromVehicle(true);
				}
				this->deleteLastSpawnedCar(player);

				auto playerPosition = player.getPosition();
				auto vehicle = vehiclesComponent->create(false,
					Core::Utils::VEHICLE_LIST
						.at(vehicleTypeSelected)[result.listItem()]
						.modelId,
					playerPosition, 0.0, std::rand() % 128, std::rand() % 128,
					Seconds(60000));
				vehicle->putPlayer(player, 0);
				playerExt->sendInfoMessage(
					_("You have sucessfully spawned the vehicle!", player));
				playerExt->getPlayerData()->tempData->freeroam->lastVehicleId
					= vehicle->getID();
			}
			else
			{
				this->showVehicleSpawningDialog(player);
			}
		});
}

void FreeroamController::showCreateWeaponSetDialog(IPlayer& player)
{
	auto dialog
		= Core::SettingsDialogBuilder(player, this->dialogManager)
			  .setTitle(fmt::sprintf(
				  DIALOG_HEADER_TITLE, _("Create weapon set", player)))
			  .setItems(std::vector<std::shared_ptr<Core::SettingItem>> {
				  std::make_shared<Core::SettingStringItem>("name",
					  _("Weapon set name", player),
					  _("Enter the name of new weapon set", player)),
				  std::make_shared<Core::SettingEnumItem>("slot1",
					  _("Handheld weapon", player),
					  std::vector<Core::SettingEnumItem::EnumChoice> {
						  Core::SettingEnumItem::EnumChoice {
							  .id = "WEAPON_GOLFCLUB",
							  .text = __("Golf club"),
						  },
						  Core::SettingEnumItem::EnumChoice {
							  .id = "WEAPON_NITESTICK",
							  .text = __("Nightstick"),
						  },
						  Core::SettingEnumItem::EnumChoice {
							  .id = "WEAPON_KNIFE",
							  .text = __("Knife"),
						  },
						  Core::SettingEnumItem::EnumChoice {
							  .id = "WEAPON_SHOVEL",
							  .text = __("Shovel"),
						  },
						  Core::SettingEnumItem::EnumChoice {
							  .id = "WEAPON_POOLSTICK",
							  .text = __("Pool cue"),
						  },
						  Core::SettingEnumItem::EnumChoice {
							  .id = "WEAPON_KATANA",
							  .text = __("Katana"),
						  },
						  Core::SettingEnumItem::EnumChoice {
							  .id = "WEAPON_CHAINSAW",
							  .text = __("Chainsaw"),
						  },
					  },
					  true),
			  })
			  .setLeftButton(__("Ok"))
			  .setRightButton(__("Cancel"))
			  .build();
	dialog->show();
}
}