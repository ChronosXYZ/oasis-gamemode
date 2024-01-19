#include "CoreManager.hpp"
#include "component.hpp"
#include "player.hpp"
#include "player/PlayerModel.hpp"

using namespace Core;

CoreManager::CoreManager(IComponentList* components, IPlayerPool* playerPool)
	: components(components)
	, playerPool(playerPool)
	, _dialogManager(make_shared<DialogManager>(components))
{
	playerPool->getPlayerConnectDispatcher()
		.addEventHandler(this);

	this->_dbConnection = make_shared<pqxx::connection>(getenv("DB_CONNECTION_STRING"));
}

shared_ptr<CoreManager> CoreManager::create(IComponentList* components, IPlayerPool* playerPool)
{
	shared_ptr<CoreManager> pManager(new CoreManager(components, playerPool));
	pManager->initHandlers();
	return pManager;
}

CoreManager::~CoreManager()
{
	playerPool->getPlayerConnectDispatcher().removeEventHandler(this);

	components = nullptr;
	playerPool = nullptr;
}

void CoreManager::attachPlayerData(IPlayer& player, std::shared_ptr<PlayerModel> data)
{
	auto ext = queryExtension<OasisPlayerDataExt>(player);
	if (ext)
	{
		ext->setPlayerData(data);
	}
}

optional<shared_ptr<PlayerModel>> CoreManager::getPlayerData(IPlayer& player)
{
	auto ext = queryExtension<OasisPlayerDataExt>(player);
	if (ext)
	{
		if (auto data = ext->getPlayerData())
		{
			return data;
		}
	}
	return {};
}

void CoreManager::onPlayerConnect(IPlayer& player)
{
	player.addExtension(new OasisPlayerDataExt(), true);
}

void CoreManager::onPlayerDisconnect(IPlayer& player, PeerDisconnectReason reason)
{
}

shared_ptr<DialogManager> CoreManager::getDialogManager()
{
	return std::shared_ptr<DialogManager>(this->_dialogManager);
}

void CoreManager::initHandlers()
{
	_authHandler = make_unique<AuthHandler>(playerPool, weak_from_this());
}

shared_ptr<pqxx::connection> CoreManager::getDBConnection()
{
	return this->_dbConnection;
}