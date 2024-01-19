#include "CoreManager.hpp"
#include "component.hpp"
#include "player.hpp"
#include "player/PlayerExtension.hpp"
#include "player/PlayerModel.hpp"
#include <memory>

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

shared_ptr<PlayerModel> CoreManager::getPlayerData(IPlayer& player)
{
	auto ext = queryExtension<OasisPlayerExt>(player);
	if (ext)
	{
		if (auto data = ext->getPlayerData())
		{
			return data;
		}
	}
	return {};
}

OasisPlayerExt* CoreManager::getPlayerExt(IPlayer& player)
{
	return queryExtension<OasisPlayerExt>(player);
}

void CoreManager::onPlayerConnect(IPlayer& player)
{
	player.addExtension(new OasisPlayerExt(std::shared_ptr<PlayerModel>(new PlayerModel()), player), true);
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

bool CoreManager::refreshPlayerData(IPlayer& player)
{
	auto db = this->getDBConnection();
	pqxx::work txn(*db);
	pqxx::result res = txn.exec_params("SELECT id, \
					name, \
					users.password_hash, \
					\"language\", \
					email, \
					last_skin_id, \
					last_ip, \
					last_login_at, \
					bans.reason as \"ban_reason\", \
					bans.by as \"banned_by\", \
					bans.expires_at as \"ban_expires_at\", \
					admins.\"level\" as \"admin_level\", \
					admins.password_hash as \"admin_pass_hash\" \
					FROM users \
					LEFT JOIN bans \
					ON users.id = bans.user_id \
					LEFT JOIN admins \
					ON users.id = admins.user_id \
					WHERE name=$1",
		player.getName().to_string());

	if (res.size() == 0)
	{
		return false;
	}
	spdlog::info("Found player data for " + player.getName().to_string() + " in DB");

	auto row = res[0];
	this->getPlayerData(player)->updateFromRow(row);
	txn.commit();
	return true;
}