#pragma once

#include <memory>
#include <iostream>
#include <string>
#include <format>
#include <exception>
#include <string>
#include <regex>
#include <chrono>

#include <sdk.hpp>
#include <player.hpp>
#include <Server/Components/Classes/classes.hpp>
#include <spdlog/spdlog.h>
#include <network.hpp>
#include <fmt/printf.h>
#include <spdlog/spdlog.h>

#include "../../constants.hpp"
#include "../utils/LocaleUtils.hpp"
#include "../utils/Hash.hpp"
#include "../SQLQueryManager.hpp"
#include "../utils/QueryNames.hpp"
#include "PlayerVars.hpp"

namespace Core
{
class CoreManager;
}

namespace Core::Auth
{
class AuthHandler : public PlayerConnectEventHandler, public ClassEventHandler
{
public:
	AuthHandler(IPlayerPool* playerPool, std::weak_ptr<Core::CoreManager> coreManager);
	~AuthHandler();

	void onPlayerConnect(IPlayer& player) override;
	bool onPlayerRequestClass(IPlayer& player, unsigned int classId) override;

private:
	IPlayerPool* const _playerPool;
	IClassesComponent* const _classesComponent;
	std::weak_ptr<Core::CoreManager> _coreManager;

	void showLanguageDialog(IPlayer& player);
	void showRegistrationDialog(IPlayer& player);
	void showEmailDialog(IPlayer& player);
	void showLoginDialog(IPlayer& player, bool wrongPass);
	void showRegistrationInfoDialog(IPlayer& player);

	// Callbacks
	void onLoginSubmit(IPlayer& player, const std::string& password);
	void onPasswordSubmit(IPlayer& player, const std::string& password);
	void onEmailSubmit(IPlayer& player, const std::string& email);
	void onRegistrationSubmit(IPlayer& player);
};
}