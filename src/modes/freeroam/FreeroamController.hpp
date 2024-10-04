#pragma once

#include "../ModeBase.hpp"
#include "../../core/utils/VehicleList.hpp"
#include "../../core/dialogs/DialogManager.hpp"
#include "../../core/commands/CommandManager.hpp"
#include "../../core/ModeManager.hpp"
#include "../../core/utils/IDPool.hpp"
#include "component.hpp"

#include <types.hpp>
#include <Server/Components/Vehicles/vehicle_components.hpp>
#include <player.hpp>

#include <memory>
#include <string>
#include <functional>

namespace Modes::Freeroam
{
inline const std::string MODE_NAME = "freeroam";

inline const auto SPAWN_LOCATION = Vector3(2037.4828, -1193.1844, 22.7924);
inline const auto SPAWN_ANGLE = 99.7903;

class FreeroamController : public Modes::ModeBase,
						   public PlayerSpawnEventHandler
{
	IPlayerPool* playerPool;
	IVehiclesComponent* vehiclesComponent;

	std::weak_ptr<Core::ModeManager> modeManager;
	std::shared_ptr<Core::DialogManager> dialogManager;
	std::shared_ptr<Core::Commands::CommandManager> commandManager;
	unsigned int virtualWorldId;

	void initCommands();
	void initVehicles();
	void setupSpawn(IPlayer& player);
	void deleteLastSpawnedCar(IPlayer& player);

	void showVehicleSpawningDialog(IPlayer& player);
	void showVehicleListDialog(
		IPlayer& player, Core::Utils::VehicleType vehicleTypeSelected);
	void showCreateWeaponSetDialog(IPlayer& player);

public:
	FreeroamController(IComponentList* components, IPlayerPool* playerPool,
		std::shared_ptr<Core::Utils::IDPool> virtualWorldIdPool,
		std::weak_ptr<Core::ModeManager> modeManager,
		std::shared_ptr<Core::DialogManager> dialogManager,
		std::shared_ptr<Core::Commands::CommandManager> commandManager,
		std::shared_ptr<dp::event_bus> bus);
	virtual ~FreeroamController();

	void onModeJoin(IPlayer& player,
		std::unordered_map<std::string, Core::PrimitiveType> joinData) override;
	void onModeLeave(IPlayer& player) override;
	void onModeSelect(IPlayer& player) override;

	void onPlayerSave(
		std::shared_ptr<Core::PlayerModel> data, pqxx::work& txn) override;
	void onPlayerLoad(
		std::shared_ptr<Core::PlayerModel> data, pqxx::work& txn) override;

	void onPlayerSpawn(IPlayer& player) override;
	void onPlayerDeath(IPlayer& player, IPlayer* killer, int reason) override;
};
}