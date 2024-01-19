#pragma once

#include <memory>
#include <iostream>
#include <string>
#include <format>

#include <sdk.hpp>
#include <player.hpp>
#include <Server/Components/Dialogs/dialogs.hpp>
#include <spdlog/spdlog.h>

#include "../../constants.hpp"
#include "../utils/LocaleUtils.hpp"
#include "../utils/Hash.hpp"

namespace Core
{
class CoreManager;
class AuthHandler : public PlayerConnectEventHandler
{
public:
	AuthHandler(IPlayerPool* playerPool, std::weak_ptr<CoreManager> coreManager);
	~AuthHandler();

	void onPlayerConnect(IPlayer& player) override;

private:
	IPlayerPool* const playerPool;
	std::weak_ptr<CoreManager> coreManager;

	bool loadPlayerData(IPlayer& player);
	void showRegistrationDialog(IPlayer& player);
	void showLoginDialog(IPlayer& player, bool wrongPass);
	void onLoginSubmit(IPlayer& player, const std::string& password);
	void onRegistrationSubmit(IPlayer& player, const std::string& password);
};
}