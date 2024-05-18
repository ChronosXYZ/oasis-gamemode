#pragma once

#include "../../modes/Modes.hpp"
#include "../utils/Events.hpp"

#include <eventbus/event_bus.hpp>
#include <player.hpp>

#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace Core::Controllers
{
class PlayerOnFireController : public PlayerDamageEventHandler, public PlayerConnectEventHandler
{
	std::shared_ptr<dp::event_bus> bus;
	IPlayerPool* playerPool;
	std::unordered_map<Modes::Mode, std::unordered_set<IPlayer*>> playersOnFire;
	dp::handler_registration roundEndRegistration;

public:
	PlayerOnFireController(IPlayerPool* playerPool, std::shared_ptr<dp::event_bus> bus);
	~PlayerOnFireController();

	void onPlayerDeath(IPlayer& player, IPlayer* killer, int reason) override;
	void onPlayerDisconnect(IPlayer& player, PeerDisconnectReason reason) override;

	void onModeLeave(IPlayer& player, Modes::Mode mode);
	void onRoundEnd(Utils::Events::RoundEndEvent event);
};
}