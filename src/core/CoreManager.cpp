#include "CoreManager.hpp"

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

void CoreManager::addRegisteredPlayer(shared_ptr<Player> player)
{
	this->_players[player->serverPlayer.getID()] = move(player);
}

optional<shared_ptr<Player>> CoreManager::getPlayerData(unsigned int id)
{
	if (this->_players.contains(id))
		return this->_players[id];
	return {};
}

void CoreManager::onPlayerConnect(IPlayer& player)
{
}

void CoreManager::onPlayerDisconnect(IPlayer& player, PeerDisconnectReason reason)
{
	if (this->_players.count(player.getID()) != 0)
	{
		this->_players.erase(player.getID());
	}
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