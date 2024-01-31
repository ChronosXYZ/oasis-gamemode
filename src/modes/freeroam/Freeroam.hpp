#pragma once

#include <memory>
#include <functional>

#include <player.hpp>
#include <string>

#include "../../constants.hpp"

namespace Core
{
class CoreManager;
}

namespace Modes::Freeroam
{
const inline static std::string MODE_NAME = "freeroam";
const inline static unsigned int VIRTUAL_WORLD_ID = 0;

class FreeroamHandler : public PlayerSpawnEventHandler, public PlayerDamageEventHandler
{
	FreeroamHandler(std::weak_ptr<Core::CoreManager> coreManager, IPlayerPool* playerPool);
	std::weak_ptr<Core::CoreManager> _coreManager;
	IPlayerPool* _playerPool;
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