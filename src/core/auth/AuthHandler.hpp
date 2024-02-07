#pragma once

#include "Server/Components/Timers/timers.hpp"
#include <Server/Components/Classes/classes.hpp>
#include <player.hpp>
#include <regex>

namespace Core
{
class CoreManager;
}

namespace Core::Auth
{
inline const std::regex EMAIL_REGEX("(?:(?:[^<>()\\[\\].,;:\\s@\"]+(?:\\.[^<>()\\[\\].,;:\\s@\"]+)*)|\".+\")@(?:(?:[^<>()‌​\\[\\].,;:\\s@\"]+\\.)+[^<>()\\[\\].,;:\\s@\"]{2,})");

class AuthHandler : public PlayerConnectEventHandler, public ClassEventHandler
{
public:
	AuthHandler(IPlayerPool* playerPool, std::weak_ptr<Core::CoreManager> coreManager);
	~AuthHandler();

	void onPlayerConnect(IPlayer& player) override;

private:
	IPlayerPool* const _playerPool;
	IClassesComponent* const _classesComponent;
	ITimersComponent* const _timersComponent;
	std::weak_ptr<Core::CoreManager> _coreManager;

	void showLanguageDialog(IPlayer& player);
	void showRegistrationDialog(IPlayer& player);
	void showEmailDialog(IPlayer& player);
	void showLoginDialog(IPlayer& player, bool wrongPass);
	void showRegistrationInfoDialog(IPlayer& player);
	void interpolatePlayerCamera(IPlayer& player);

	// Callbacks
	void onLoginSubmit(IPlayer& player, const std::string& password);
	void onPasswordSubmit(IPlayer& player, const std::string& password);
	void onEmailSubmit(IPlayer& player, const std::string& email);
	void onRegistrationSubmit(IPlayer& player);
};
}