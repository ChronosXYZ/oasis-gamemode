#include "Freeroam.hpp"
#include "../../core/CoreManager.hpp"
#include "../../core/PlayerVars.hpp"
#include "../../core/player/PlayerExtension.hpp"

#include <player.hpp>
#include <Server/Components/Vehicles/vehicles.hpp>

#include <functional>
#include <memory>

namespace Modes::Freeroam
{

FreeroamHandler::FreeroamHandler(std::weak_ptr<Core::CoreManager> coreManager, IPlayerPool* playerPool)
	: _coreManager(coreManager)
	, _vehiclesComponent(coreManager.lock()->components->queryComponent<IVehiclesComponent>())
	, _playerPool(playerPool)
{
	using namespace std::placeholders;
	_playerPool->getPlayerSpawnDispatcher().addEventHandler(this);
	_playerPool->getPlayerDamageDispatcher().addEventHandler(this);
}

FreeroamHandler::~FreeroamHandler()
{
	_playerPool->getPlayerSpawnDispatcher().removeEventHandler(this);
	_playerPool->getPlayerDamageDispatcher().removeEventHandler(this);
}

void FreeroamHandler::onPlayerSpawn(IPlayer& player)
{
	auto playerExt = Core::Player::getPlayerExt(player);
	auto mode = static_cast<Mode>(std::get<int>(*playerExt->getPlayerData()->getTempData(Core::PlayerVars::CURRENT_MODE)));
	if (mode != Mode::Freeroam)
	{
		return;
	}
	setupSpawn(player);

	// player.setPosition(SPAWN_LOCATION);
	// playerExt->setFacingAngle(SPAWN_ANGLE);
	// player.setCameraBehind();
	// player.setSkin(playerExt->getPlayerData()->lastSkinId);
}

std::unique_ptr<FreeroamHandler> FreeroamHandler::create(std::weak_ptr<Core::CoreManager> coreManager, IPlayerPool* playerPool)
{
	auto handler = new FreeroamHandler(coreManager, playerPool);
	handler->initCommands();
	return std::unique_ptr<FreeroamHandler>(handler);
}
void FreeroamHandler::initCommands()
{
	this->_coreManager.lock()->getCommandManager()->addCommand(
		"fr", [&](std::reference_wrapper<IPlayer> player)
		{
			this->_coreManager.lock()->selectMode(player, Mode::Freeroam);
		},
		Core::Commands::CommandInfo { .args = {}, .description = "Teleports player to the Freeroam mode", .category = MODE_NAME });

	this->_coreManager.lock()->getCommandManager()->addCommand(
		"v", [&](std::reference_wrapper<IPlayer> player, int modelId, int color1, int color2)
		{
			auto playerExt = Core::Player::getPlayerExt(player);
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

			auto playerPosition = player.get().getPosition();
			auto vehicle = _vehiclesComponent->create(false, modelId, playerPosition, 0.0, color1, color2, Seconds(60000));
			vehicle->putPlayer(player, 0);
			playerExt->sendInfoMessage(_("You have sucessfully spawned the vehicle!", player));
		},
		Core::Commands::CommandInfo { .args = { "vehicle model id", "color 1", "color 2" }, .description = "Spawns a vehicle", .category = MODE_NAME });
}

void FreeroamHandler::onPlayerDeath(IPlayer& player, IPlayer* killer, int reason)
{
}

void FreeroamHandler::onModeJoin(IPlayer& player)
{
	setupSpawn(player);
}

void FreeroamHandler::setupSpawn(IPlayer& player)
{
	queryExtension<IPlayerClassData>(player)->setSpawnInfo(PlayerClass(this->_coreManager.lock()->getPlayerData(player)->lastSkinId,
		TEAM_NONE,
		SPAWN_LOCATION,
		SPAWN_ANGLE,
		WeaponSlots {}));
}
}