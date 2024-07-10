#pragma once

#include "../core/player/PlayerModel.hpp"
#include "Modes.hpp"
#include "../core/utils/Events.hpp"

#include <eventbus/event_bus.hpp>
#include <memory>
#include <player.hpp>

#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace Modes
{
struct ModeBase
{
	ModeBase(Mode mode, std::shared_ptr<dp::event_bus> bus);
	virtual ~ModeBase();

	virtual void onModeSelect(IPlayer& player) = 0;
	virtual void onModeJoin(IPlayer& player,
		std::unordered_map<std::string, Core::PrimitiveType> joinData);
	virtual void onModeLeave(IPlayer& player);
	virtual void onPlayerSave(
		std::shared_ptr<Core::PlayerModel> data, pqxx::work& txn)
		= 0;
	virtual void onPlayerLoad(
		std::shared_ptr<Core::PlayerModel> data, pqxx::work& txn)
		= 0;
	virtual void onPlayerOnFire(Core::Utils::Events::PlayerOnFireEvent event);
	unsigned int playerCount();
	const Mode& getModeType();

protected:
	std::unordered_set<IPlayer*> players;
	typedef ModeBase super;
	Mode mode;
	std::shared_ptr<dp::event_bus> bus;

	dp::handler_registration playerOnFireEventRegistration;
};
}