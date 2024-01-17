#pragma once

#include <memory>
#include <map>
#include <Server/Components/Dialogs/dialogs.hpp>

#include "Player.hpp"
#include "DialogManager.hpp"

using namespace std;

namespace Core
{
class CoreManager : public PlayerConnectEventHandler
{
public:
	IComponentList* components;

	CoreManager(IComponentList* components, IPlayerPool* playerPool);
	~CoreManager();

	void addRegisteredPlayer(unique_ptr<Player> player);

	void onPlayerConnect(IPlayer& player) override;
	void onPlayerDisconnect(IPlayer& player, PeerDisconnectReason reason) override;

private:
	IPlayerPool* playerPool = nullptr;

	unique_ptr<DialogManager> dialogManager;
	map<unsigned int, unique_ptr<Player>>
		players;
};
}