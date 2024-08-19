#pragma once

#include "player.hpp"
#include <types.hpp>
#include <string>

namespace Modes::Deathmatch
{
struct DeathmatchResult
{
	IPlayer* player;
	unsigned int kills;
	unsigned int deaths;
	float ratio;
	float damageInflicted;
};
}