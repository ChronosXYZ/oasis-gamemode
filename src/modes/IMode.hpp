#pragma once

#include "../core/player/PlayerModel.hpp"

#include <player.hpp>
#include <string>
#include <unordered_map>

namespace Modes
{
struct IMode
{
	IMode() { }
	virtual ~IMode() { }

	virtual void onModeSelect(IPlayer& player) = 0;
	virtual void onModeJoin(IPlayer& player, std::unordered_map<std::string, Core::PrimitiveType> joinData) = 0;
	virtual void onModeLeave(IPlayer& player) = 0;
};
}