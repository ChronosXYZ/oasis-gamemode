#include "FreeroamController.hpp"
#include "../../core/CoreManager.hpp"
#include "../../core/player/PlayerExtension.hpp"
#include "../../core/utils/VehicleList.hpp"
#include "FreeroamVehicles.hpp"
#include "eventbus/event_bus.hpp"
#include "types.hpp"

#include <cstdlib>
#include <fmt/printf.h>
#include <magic_enum/magic_enum.hpp>
#include <player.hpp>
#include <Server/Components/Vehicles/vehicles.hpp>

#include <functional>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

namespace Modes::Freeroam
{
FreeroamController::FreeroamController(
	std::weak_ptr<Core::CoreManager> coreManager, IPlayerPool* playerPool,
	std::shared_ptr<Core::DialogManager> dialogManager,
	std::shared_ptr<dp::event_bus> bus)
	: super(Mode::Freeroam, bus, playerPool)
	, _coreManager(coreManager)
	, _vehiclesComponent(
		  coreManager.lock()->components->queryComponent<IVehiclesComponent>())
	, _playerPool(playerPool)
	, dialogManager(dialogManager)
{
	using namespace std::placeholders;
	_playerPool->getPlayerSpawnDispatcher().addEventHandler(this);
	_playerPool->getPlayerDamageDispatcher().addEventHandler(this);
	this->virtualWorldId = this->_coreManager.lock()->allocateVirtualWorldId();
}

FreeroamController::~FreeroamController()
{
	_playerPool->getPlayerSpawnDispatcher().removeEventHandler(this);
	_playerPool->getPlayerDamageDispatcher().removeEventHandler(this);
}

void FreeroamController::onPlayerSpawn(IPlayer& player)
{
	auto playerExt = Core::Player::getPlayerExt(player);
	if (!playerExt->isInMode(Mode::Freeroam))
	{
		return;
	}
}

FreeroamController* FreeroamController::create(
	std::weak_ptr<Core::CoreManager> coreManager, IPlayerPool* playerPool,
	std::shared_ptr<Core::DialogManager> dialogManager,
	std::shared_ptr<dp::event_bus> bus)
{
	auto handler
		= new FreeroamController(coreManager, playerPool, dialogManager, bus);
	handler->initCommands();
	handler->initVehicles();
	return handler;
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
	this->_coreManager.lock()->getCommandManager()->addCommand(
		"fr",
		[&](std::reference_wrapper<IPlayer> player)
		{
			this->_coreManager.lock()->selectMode(player, Mode::Freeroam);
		},
		Core::Commands::CommandInfo { .args = {},
			.description = __("Teleports player to the Freeroam mode"),
			.category = MODE_NAME });

	this->_coreManager.lock()->getCommandManager()->addCommand(
		"v",
		[&](std::reference_wrapper<IPlayer> player, int modelId, int color1,
			int color2)
		{
			auto playerExt = Core::Player::getPlayerExt(player);
			if (!playerExt->isInMode(Mode::Freeroam))
			{
				playerExt->sendErrorMessage(
					__("You can spawn vehicles only in Freeroam mode!"));
				return;
			}
			if (modelId < 400 || modelId > 611)
			{
				playerExt->sendErrorMessage(__("Invalid car model ID!"));
				return;
			}
			if (color1 < 0 || color1 > 255 || color2 < 0 || color2 > 255)
			{
				playerExt->sendErrorMessage(__("Invalid color ID!"));
				return;
			}

			if (player.get().getState() == PlayerState_Driver)
			{
				player.get().removeFromVehicle(true);
			}
			this->deleteLastSpawnedCar(player);

			auto playerPosition = player.get().getPosition();
			auto vehicle = _vehiclesComponent->create(false, modelId,
				playerPosition, 0.0, color1, color2, Seconds(60000));
			vehicle->putPlayer(player, 0);
			playerExt->sendInfoMessage(
				_("You have sucessfully spawned the vehicle!", player));
			playerExt->getPlayerData()->tempData->freeroam->lastVehicleId
				= vehicle->getID();
		},
		Core::Commands::CommandInfo {
			.args = { __("vehicle model id"), __("color 1"), __("color 2") },
			.description = __("Spawns a vehicle"),
			.category = MODE_NAME });
	this->_coreManager.lock()->getCommandManager()->addCommand(
		"v",
		[this](std::reference_wrapper<IPlayer> player)
		{
			auto playerExt = Core::Player::getPlayerExt(player);
			if (!playerExt->isInMode(Mode::Freeroam))
			{
				playerExt->sendErrorMessage(
					__("You can spawn vehicles only in Freeroam mode!"));
				return;
			}
			this->showVehicleSpawningDialog(player);
		},
		Core::Commands::CommandInfo { .args = {},
			.description = __("Shows dialog with vehicle list for spawning"),
			.category = MODE_NAME });
	this->_coreManager.lock()->getCommandManager()->addCommand(
		"kill",
		[](std::reference_wrapper<IPlayer> player)
		{
			player.get().setHealth(0.0);
			Core::Player::getPlayerExt(player.get())
				->sendInfoMessage(__("You have killed yourself!"));
		},
		Core::Commands::CommandInfo {
			.args = {},
			.description = __("Kill yourself"),
			.category = MODE_NAME,
		});
	this->_coreManager.lock()->getCommandManager()->addCommand(
		"skin",
		[&](std::reference_wrapper<IPlayer> player, int skinId)
		{
			auto playerExt = Core::Player::getPlayerExt(player);
			if (skinId < 0 || skinId > 311)
			{
				playerExt->sendErrorMessage(__("Invalid skin ID!"));
				return;
			}
			player.get().setSkin(skinId);
			auto data = Core::Player::getPlayerData(player.get());
			data->lastSkinId = skinId;
			playerExt->sendInfoMessage(fmt::sprintf(
				_("You have changed your skin to ID: %d!", player), skinId));
		},
		Core::Commands::CommandInfo {
			.args = { __("skin ID") },
			.description = __("Set player skin"),
			.category = MODE_NAME,
		});
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
			auto vehicle = this->_vehiclesComponent->create(VehicleSpawnData {
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
	this->_coreManager.lock()->joinMode(player, Modes::Mode::Freeroam, {});
}

void FreeroamController::onPlayerSave(
	std::shared_ptr<Core::PlayerModel> data, pqxx::work& txn)
{
	// TODO
}

void FreeroamController::onPlayerLoad(
	std::shared_ptr<Core::PlayerModel> data, pqxx::work& txn)
{ // TODO
}

void FreeroamController::deleteLastSpawnedCar(IPlayer& player)
{
	auto playerData = Core::Player::getPlayerData(player);
	if (auto lastVehicleId = playerData->tempData->freeroam->lastVehicleId)
	{
		auto vehicle = _vehiclesComponent->get(lastVehicleId.value());
		_vehiclesComponent->release(lastVehicleId.value());
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
				auto vehicle = _vehiclesComponent->create(false,
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
}