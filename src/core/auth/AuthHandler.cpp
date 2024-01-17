#include "AuthHandler.hpp"
#include "../CoreManager.hpp"

using namespace Core;

AuthHandler::AuthHandler(IPlayerPool* playerPool, std::weak_ptr<CoreManager> coreManager)
	: coreManager(coreManager)
	, playerPool(playerPool)
{
	playerPool->getPlayerConnectDispatcher().addEventHandler(this);
}

AuthHandler::~AuthHandler()
{
	playerPool->getPlayerConnectDispatcher().removeEventHandler(this);
}

void AuthHandler::onPlayerConnect(IPlayer& player)
{
	this->showRegistrationDialog(player);
}

void AuthHandler::showRegistrationDialog(IPlayer& player)
{
	this->coreManager.lock()->getDialogManager()->createDialog(player,
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
}
