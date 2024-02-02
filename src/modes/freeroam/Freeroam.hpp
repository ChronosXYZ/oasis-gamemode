#pragma once

#include <types.hpp>
#include <Server/Components/Vehicles/vehicle_components.hpp>
#include <player.hpp>

#include <memory>
#include <string>
#include <functional>

namespace Core
{
class CoreManager;
}

namespace Modes::Freeroam
{
inline const std::string MODE_NAME = "freeroam";
inline const unsigned int VIRTUAL_WORLD_ID = 0;

inline const auto SPAWN_LOCATION = Vector3(2037.4828, -1193.1844, 22.7924);
inline const auto SPAWN_ANGLE = 99.7903;

class FreeroamHandler : public PlayerSpawnEventHandler,
						public PlayerDamageEventHandler
{
	FreeroamHandler(std::weak_ptr<Core::CoreManager> coreManager, IPlayerPool* playerPool);

	std::weak_ptr<Core::CoreManager> _coreManager;
	IPlayerPool* _playerPool;
	IVehiclesComponent* _vehiclesComponent;

	void initCommands();
	void setRandomSpawnInfo(IPlayer& player);

public:
	~FreeroamHandler();

	static std::unique_ptr<FreeroamHandler> create(std::weak_ptr<Core::CoreManager> coreManager, IPlayerPool* playerPool);

	void onModeJoin(IPlayer& player);

	void onPlayerSpawn(IPlayer& player) override;
	void onPlayerDeath(IPlayer& player, IPlayer* killer, int reason) override;
};
}