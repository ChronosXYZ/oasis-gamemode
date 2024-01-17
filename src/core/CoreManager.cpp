#include "CoreManager.hpp"
#include "Player.hpp"

using namespace Core;

CoreManager::CoreManager(IComponentList* components, IPlayerPool* playerPool)
	: components(components)
	, playerPool(playerPool)
{
	playerPool->getPlayerConnectDispatcher().addEventHandler(this);
	dialogManager = make_unique<DialogManager>(components);
}

CoreManager::~CoreManager()
{
	playerPool->getPlayerConnectDispatcher().removeEventHandler(this);

	components = nullptr;
	playerPool = nullptr;
}

void CoreManager::addRegisteredPlayer(unique_ptr<Player> player)
{
	this->players[player->serverPlayer.getID()] = move(player);
}

void CoreManager::onPlayerConnect(IPlayer& player)
{
	this->dialogManager->createDialog(player,
		DialogStyle_PASSWORD,
		"Registration",
		"Enter the password:",
		"OK",
		"Quit",
		[&](DialogResponse resp, int listItem, StringView inputText)
		{
			switch (resp)
			{
			case DialogResponse_Left:
			{
				player.sendClientMessage(Colour::White(), "You have been registered!");
				player.spawn();
				break;
			}
			case DialogResponse_Right:
			{
				player.kick();
				break;
			}
			}
		});

	unique_ptr<PlayerData>
		pData(new PlayerData);
	unique_ptr<Player> wrappedPlayer(new Player(move(pData), player));
	this->addRegisteredPlayer(move(wrappedPlayer));
}

void CoreManager::onPlayerDisconnect(IPlayer& player, PeerDisconnectReason reason)
{
	if (this->players.count(player.getID()) != 0)
	{
		this->players.erase(player.getID());
	}
}