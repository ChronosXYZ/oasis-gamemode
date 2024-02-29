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

class AuthController : public PlayerConnectEventHandler, public ClassEventHandler
{
public:
	AuthController(IPlayerPool* playerPool, std::weak_ptr<Core::CoreManager> coreManager);
	~AuthController();

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