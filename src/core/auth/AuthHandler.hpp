#pragma once

#include <memory>
#include <iostream>
#include <string>
#include <format>
#include <exception>
#include <string>
#include <regex>

#include <sdk.hpp>
#include <player.hpp>
#include <Server/Components/Dialogs/dialogs.hpp>
#include <spdlog/spdlog.h>
#include <network.hpp>
#include <fmt/printf.h>
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

	void showLanguageDialog(IPlayer& player);
	void showRegistrationDialog(IPlayer& player);
	void showEmailDialog(IPlayer& player);
	void showLoginDialog(IPlayer& player, bool wrongPass);

	// Callbacks
	void onLoginSubmit(IPlayer& player, const std::string& password);
	void onPasswordSubmit(IPlayer& player, const std::string& password);
	void onEmailSubmit(IPlayer& player, const std::string& email);
	void onRegistrationSubmit(IPlayer& player);
};
}