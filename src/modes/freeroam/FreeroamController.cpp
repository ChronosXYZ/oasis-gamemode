#include "FreeroamController.hpp"
#include "../../core/CoreManager.hpp"
#include "../../core/player/PlayerExtension.hpp"
#include "FreeroamVehicles.hpp"
#include "eventbus/event_bus.hpp"
#include "types.hpp"

#include <fmt/printf.h>
#include <magic_enum/magic_enum.hpp>
#include <player.hpp>
#include <Server/Components/Vehicles/vehicles.hpp>

#include <functional>
#include <memory>
#include <spdlog/spdlog.h>

namespace Modes::Freeroam
{

FreeroamController::FreeroamController(
	std::weak_ptr<Core::CoreManager> coreManager, IPlayerPool* playerPool,
	std::shared_ptr<dp::event_bus> bus)
	: super(Mode::Freeroam, bus, playerPool)
	, _coreManager(coreManager)
	, _vehiclesComponent(
		  coreManager.lock()->components->queryComponent<IVehiclesComponent>())
	, _playerPool(playerPool)
{
	using namespace std::placeholders;
	_playerPool->getPlayerSpawnDispatcher().addEventHandler(this);
	_playerPool->getPlayerDamageDispatcher().addEventHandler(this);
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
	std::shared_ptr<dp::event_bus> bus)
{
	auto handler = new FreeroamController(coreManager, playerPool, bus);
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
					_("You can spawn vehicles only in Freeroam mode!", player));
				return;
			}
			if (modelId < 400 || modelId > 611)
			{
				playerExt->sendErrorMessage(_("Invalid car model ID!", player));
				return;
			}
			if (color1 < 0 || color1 > 255 || color2 < 0 || color2 > 255)
			{
				playerExt->sendErrorMessage(_("Invalid color ID!", player));
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
				playerExt->sendErrorMessage(_("Invalid skin ID!", player));
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
			vehicle->setVirtualWorld(VIRTUAL_WORLD_ID);
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
	player.setVirtualWorld(VIRTUAL_WORLD_ID);
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

}