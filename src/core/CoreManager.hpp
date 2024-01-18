#pragma once

#include <memory>
#include <map>
#include <pqxx/pqxx>
#include <sdk.hpp>

#include "Player.hpp"
#include "DialogManager.hpp"
#include "auth/AuthHandler.hpp"

using namespace std;

namespace Core
{
class CoreManager : public PlayerConnectEventHandler, public std::enable_shared_from_this<CoreManager>
{
public:
	const IComponentList* components;

	static shared_ptr<CoreManager> create(IComponentList* components, IPlayerPool* playerPool);
	~CoreManager();

	void addRegisteredPlayer(shared_ptr<Player> player);
	optional<shared_ptr<Player>> getPlayerData(unsigned int id);
	shared_ptr<DialogManager> getDialogManager();
	shared_ptr<pqxx::connection> getDBConnection();

	void onPlayerConnect(IPlayer& player) override;
	void onPlayerDisconnect(IPlayer& player, PeerDisconnectReason reason) override;

private:
	CoreManager(IComponentList* components, IPlayerPool* playerPool);

	void initHandlers();

	IPlayerPool* playerPool = nullptr;

	map<unsigned int, shared_ptr<Player>> _players;
	shared_ptr<DialogManager> _dialogManager;
	shared_ptr<pqxx::connection> _dbConnection;

	// Handlers
	unique_ptr<AuthHandler> _authHandler;
};
}