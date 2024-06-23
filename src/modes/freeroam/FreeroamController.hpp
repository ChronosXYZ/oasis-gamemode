#pragma once

#include "../../core/CoreManager.hpp"
#include "../ModeBase.hpp"

#include <types.hpp>
#include <Server/Components/Vehicles/vehicle_components.hpp>
#include <player.hpp>

#include <memory>
#include <string>
#include <functional>

namespace Modes::Freeroam
{
inline const std::string MODE_NAME = "freeroam";
inline const unsigned int VIRTUAL_WORLD_ID = 0;

inline const auto SPAWN_LOCATION = Vector3(2037.4828, -1193.1844, 22.7924);
inline const auto SPAWN_ANGLE = 99.7903;

class FreeroamController : public Modes::ModeBase,
						   public PlayerSpawnEventHandler,
						   public PlayerDamageEventHandler
{
	FreeroamController(std::weak_ptr<Core::CoreManager> coreManager, IPlayerPool* playerPool, std::shared_ptr<dp::event_bus> bus);

	std::weak_ptr<Core::CoreManager> _coreManager;
	IPlayerPool* _playerPool;
	IVehiclesComponent* _vehiclesComponent;

	void initCommands();
	void setupSpawn(IPlayer& player);
	void deleteLastSpawnedCar(IPlayer& player);

public:
	virtual ~FreeroamController();

	static FreeroamController* create(std::weak_ptr<Core::CoreManager> coreManager, IPlayerPool* playerPool, std::shared_ptr<dp::event_bus> bus);

	void onModeJoin(IPlayer& player, std::unordered_map<std::string, Core::PrimitiveType> joinData) override;
	void onModeLeave(IPlayer& player) override;
	void onModeSelect(IPlayer& player) override;

	void onPlayerSpawn(IPlayer& player) override;
	void onPlayerDeath(IPlayer& player, IPlayer* killer, int reason) override;
};
}