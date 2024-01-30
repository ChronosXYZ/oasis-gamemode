#include "Freeroam.hpp"
#include "../../core/CoreManager.hpp"

namespace Modes::Freeroam
{

FreeroamHandler::FreeroamHandler(std::weak_ptr<Core::CoreManager> coreManager, IPlayerPool* playerPool)
	: _coreManager(coreManager)
	, _playerPool(playerPool)
{
	_playerPool->getPlayerSpawnDispatcher().addEventHandler(this);
}

FreeroamHandler::~FreeroamHandler()
{
	_playerPool->getPlayerSpawnDispatcher().removeEventHandler(this);
}

void FreeroamHandler::onPlayerSpawn(IPlayer& player)
{
	auto pData = _coreManager.lock()->getPlayerData(player);
	std::string mode = std::get<string>(pData->getTempData(Core::CURRENT_MODE).value());
	if (mode != Freeroam::MODE_NAME)
	{
		return;
	}
	player.setPosition(consts::randomSpawnArray[random() % consts::randomSpawnArray.size()]);
}
}