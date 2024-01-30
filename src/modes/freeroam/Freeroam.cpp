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

bool FreeroamHandler::onPlayerRequestSpawn(IPlayer& player)
{
	auto pData = _coreManager.lock()->getPlayerData(player);
	if (pData->getTempData(Core::CLASS_SELECTION).has_value())
	{
		pData->deleteTempData(Core::CLASS_SELECTION);
	}

	pData->lastSkinId = player.getSkin();

	return true;
}

void FreeroamHandler::onPlayerSpawn(IPlayer& player)
{
	player.setPosition(consts::randomSpawnArray[random() % consts::randomSpawnArray.size()]);
}
}