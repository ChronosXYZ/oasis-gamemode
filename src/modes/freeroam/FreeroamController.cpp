#include "FreeroamController.hpp"
#include "../../core/CoreManager.hpp"
#include "../../core/PlayerVars.hpp"
#include "../../core/player/PlayerExtension.hpp"
#include "PlayerVars.hpp"

#include <fmt/printf.h>
#include <player.hpp>
#include <Server/Components/Vehicles/vehicles.hpp>

#include <functional>
#include <memory>
#include <spdlog/spdlog.h>

namespace Modes::Freeroam
{

FreeroamController::FreeroamController(std::weak_ptr<Core::CoreManager> coreManager, IPlayerPool* playerPool)
	: _coreManager(coreManager)
	, _vehiclesComponent(coreManager.lock()->components->queryComponent<IVehiclesComponent>())
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

FreeroamController* FreeroamController::create(std::weak_ptr<Core::CoreManager> coreManager, IPlayerPool* playerPool)
{
	auto handler = new FreeroamController(coreManager, playerPool);
	handler->initCommands();
	return handler;
}

void FreeroamController::initCommands()
{
	this->_coreManager.lock()->getCommandManager()->addCommand(
		"fr", [&](std::reference_wrapper<IPlayer> player)
		{
			this->_coreManager.lock()->selectMode(player, Mode::Freeroam);
		},
		Core::Commands::CommandInfo { .args = {}, .description = __("Teleports player to the Freeroam mode"), .category = MODE_NAME });

	this->_coreManager.lock()->getCommandManager()->addCommand(
		"v", [&](std::reference_wrapper<IPlayer> player, int modelId, int color1, int color2)
		{
			auto playerExt = Core::Player::getPlayerExt(player);
			if (!playerExt->isInMode(Mode::Freeroam))
			{
				playerExt->sendErrorMessage(_("You can spawn vehicles only in Freeroam mode!", player));
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

			auto playerPosition
				= player.get().getPosition();
			auto vehicle = _vehiclesComponent->create(false, modelId, playerPosition, 0.0, color1, color2, Seconds(60000));
			vehicle->putPlayer(player, 0);
			playerExt->sendInfoMessage(_("You have sucessfully spawned the vehicle!", player));
			playerExt->getPlayerData()->setTempData(PlayerVars::LAST_VEHICLE_ID, vehicle->getID());
		},
		Core::Commands::CommandInfo { .args = { __("vehicle model id"), __("color 1"), __("color 2") }, .description = __("Spawns a vehicle"), .category = MODE_NAME });

	this->_coreManager.lock()->getCommandManager()->addCommand(
		"kill", [](std::reference_wrapper<IPlayer> player)
		{
			player.get().setHealth(0.0);
			Core::Player::getPlayerExt(player.get())->sendInfoMessage(_("You have killed yourself!", player));
		},
		Core::Commands::CommandInfo {
			.args = {},
			.description = __("Kill yourself"),
			.category = MODE_NAME,
		});
	this->_coreManager.lock()->getCommandManager()->addCommand(
		"skin", [&](std::reference_wrapper<IPlayer> player, int skinId)
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
			playerExt->sendInfoMessage(fmt::sprintf(_("You have changed your skin to ID: %d!", player), skinId));
		},
		Core::Commands::CommandInfo {
			.args = { __("skin ID") },
			.description = __("Set player skin"),
			.category = MODE_NAME,
		});
}

void FreeroamController::onPlayerDeath(IPlayer& player, IPlayer* killer, int reason)
{
	auto playerExt = Core::Player::getPlayerExt(player);
	if (!playerExt->isInMode(Mode::Freeroam))
	{
		return;
	}
	setupSpawn(player);
}

void FreeroamController::onModeJoin(IPlayer& player)
{
	setupSpawn(player);
}

void FreeroamController::setupSpawn(IPlayer& player)
{
	auto classData = queryExtension<IPlayerClassData>(player);
	classData->setSpawnInfo(PlayerClass(Core::Player::getPlayerData(player)->lastSkinId,
		TEAM_NONE,
		SPAWN_LOCATION,
		SPAWN_ANGLE,
		WeaponSlots {}));
}

void FreeroamController::onModeLeave(IPlayer& player)
{
	this->deleteLastSpawnedCar(player);
}

void FreeroamController::deleteLastSpawnedCar(IPlayer& player)
{
	auto playerExt = Core::Player::getPlayerExt(player);
	if (auto lastVehicleId = playerExt->getPlayerData()->getTempData(PlayerVars::LAST_VEHICLE_ID))
	{
		auto vid = std::get<int>(*lastVehicleId);
		auto vehicle = _vehiclesComponent->get(vid);
		_vehiclesComponent->release(vid);
	}
}
}