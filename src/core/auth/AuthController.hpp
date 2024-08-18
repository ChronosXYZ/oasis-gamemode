#pragma once

#include "../dialogs/DialogManager.hpp"
#include "../ModeManager.hpp"
#include "../utils/ConnectionPool.hpp"

#include <Server/Components/Timers/timers.hpp>
#include <Server/Components/Classes/classes.hpp>
#include <player.hpp>

#include <regex>
#include <memory>

namespace Core::Auth
{
inline const std::regex EMAIL_REGEX(
	"(?:(?:[^<>()\\[\\].,;:\\s@\"]+(?:\\.[^<>()\\[\\].,;:\\s@\"]+)*)|\".+\")@(?"
	":(?:[^<>()‌​\\[\\].,;:\\s@\"]+\\.)+[^<>()\\[\\].,;:\\s@\"]{2,})");

class AuthController : public PlayerConnectEventHandler,
					   public ClassEventHandler
{
public:
	AuthController(IComponentList* components, IPlayerPool* playerPool,
		cp::connection_pool& pool, std::weak_ptr<ModeManager> modeManager,
		std::shared_ptr<DialogManager> dialogManager);
	~AuthController();

	void onPlayerConnect(IPlayer& player) override;

private:
	IPlayerPool* const playerPool;
	IClassesComponent* const classesComponent;
	ITimersComponent* const timersComponent;
	std::shared_ptr<DialogManager> dialogManager;
	std::weak_ptr<ModeManager> modeManager;
	cp::connection_pool& pool;
	// std::weak_ptr<Core::CoreManager> _coreManager;

	void showLanguageDialog(IPlayer& player);
	void showRegistrationDialog(IPlayer& player);
	void showEmailDialog(IPlayer& player);
	void showLoginDialog(IPlayer& player, bool wrongPass);
	void showRegistrationInfoDialog(IPlayer& player);
	void interpolatePlayerCamera(IPlayer& player);
	bool loadPlayerData(IPlayer& player);

	// Callbacks
	void onLoginSubmit(IPlayer& player, const std::string& password);
	void onPasswordSubmit(IPlayer& player, const std::string& password);
	void onEmailSubmit(IPlayer& player, const std::string& email);
	void onRegistrationSubmit(IPlayer& player);
	void onPlayerLoggedIn(IPlayer& player);
};
}