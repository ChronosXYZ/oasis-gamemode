#pragma once

#include "../../modes/Modes.hpp"
#include "../utils/Events.hpp"
#include "../commands/CommandManager.hpp"
#include "../dialogs/DialogManager.hpp"

#include <eventbus/event_bus.hpp>
#include <player.hpp>

#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace Core::Controllers
{
class PlayerOnFireController : public PlayerDamageEventHandler,
							   public PlayerConnectEventHandler
{
	std::shared_ptr<dp::event_bus> bus;
	IPlayerPool* playerPool;
	std::unordered_set<IPlayer*> playersOnFire;
	std::shared_ptr<Commands::CommandManager> commandManager;
	std::shared_ptr<DialogManager> dialogManager;

	void initCommands();

public:
	PlayerOnFireController(IPlayerPool* playerPool,
		std::shared_ptr<dp::event_bus> bus,
		std::shared_ptr<Commands::CommandManager> commandManager,
		std::shared_ptr<DialogManager> dialogManager);
	~PlayerOnFireController();

	void onPlayerDeath(IPlayer& player, IPlayer* killer, int reason) override;
	void onPlayerDisconnect(
		IPlayer& player, PeerDisconnectReason reason) override;
};
}