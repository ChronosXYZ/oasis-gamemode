#include "Freeroam.hpp"
#include "../../core/CoreManager.hpp"
#include "player.hpp"
#include <functional>
#include <memory>

namespace Modes::Freeroam
{

FreeroamHandler::FreeroamHandler(std::weak_ptr<Core::CoreManager> coreManager, IPlayerPool* playerPool)
	: _coreManager(coreManager)
	, _playerPool(playerPool)
{
	using namespace std::placeholders;
	_playerPool->getPlayerSpawnDispatcher().addEventHandler(this);
}

FreeroamHandler::~FreeroamHandler()
{
	_playerPool->getPlayerSpawnDispatcher().removeEventHandler(this);
}

void FreeroamHandler::onPlayerSpawn(IPlayer& player)
{
	auto pData = _coreManager.lock()->getPlayerData(player);
	auto mode = static_cast<Mode>(std::get<int>(*pData->getTempData(Core::CURRENT_MODE)));
	if (mode != Mode::Freeroam)
	{
		return;
	}
	player.setPosition(consts::randomSpawnArray[random() % consts::randomSpawnArray.size()]);
}

std::unique_ptr<FreeroamHandler> FreeroamHandler::create(std::weak_ptr<Core::CoreManager> coreManager, IPlayerPool* playerPool)
{
	auto handler = new FreeroamHandler(coreManager, playerPool);
	handler->initCommands();
	return std::unique_ptr<FreeroamHandler>(handler);
}
void FreeroamHandler::initCommands()
{
	this->_coreManager.lock()->addCommand("fr", [&](std::reference_wrapper<IPlayer> player)
		{
			this->_coreManager.lock()->selectMode(player, Mode::Freeroam);
		});
}
}