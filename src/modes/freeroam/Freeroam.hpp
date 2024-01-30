#pragma once

#include <memory>

#include <player.hpp>

#include "../../constants.hpp"

namespace Core
{
class CoreManager;
}

namespace Modes::Freeroam
{
class FreeroamHandler : public PlayerSpawnEventHandler
{
	std::weak_ptr<Core::CoreManager> _coreManager;
	IPlayerPool* _playerPool;

public:
	FreeroamHandler(std::weak_ptr<Core::CoreManager> coreManager, IPlayerPool* playerPool);
	~FreeroamHandler();

	bool onPlayerRequestSpawn(IPlayer& player) override;
	void onPlayerSpawn(IPlayer& player) override;
};
}