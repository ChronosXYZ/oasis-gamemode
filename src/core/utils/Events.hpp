#pragma once

#include "../../modes/Modes.hpp"

#include <player.hpp>

#include <unordered_set>

namespace Core::Utils::Events
{
struct PlayerOnFireEvent
{
	IPlayer& player;
	IPlayer& lastKillee;
	Modes::Mode mode;
};
struct RoundEndEvent
{
	Modes::Mode mode;
	std::unordered_set<IPlayer*> players;
};
}