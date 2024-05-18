#pragma once

#include "../core/player/PlayerModel.hpp"
#include "Modes.hpp"

#include <player.hpp>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace Modes
{
struct ModeBase
{
	ModeBase(Mode mode)
		: mode(mode) {};
	virtual ~ModeBase() {};

	virtual void onModeSelect(IPlayer& player) = 0;
	virtual void onModeJoin(IPlayer& player, std::unordered_map<std::string, Core::PrimitiveType> joinData);
	virtual void onModeLeave(IPlayer& player);
	unsigned int playerCount();
	const Mode& getModeType();

protected:
	std::unordered_set<IPlayer*> players;
	typedef ModeBase super;
	Mode mode;
};
}