#pragma once

#include <memory>

#include <sdk.hpp>
#include <Server/Components/Dialogs/dialogs.hpp>

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

	void showRegistrationDialog(IPlayer& player);
};
}