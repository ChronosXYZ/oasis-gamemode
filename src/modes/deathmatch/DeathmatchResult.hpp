#pragma once

#include <types.hpp>
#include <string>

namespace Modes::Deathmatch
{
struct DeathmatchResult
{
	std::string playerName;
	Colour playerColor;
	unsigned int kills;
	unsigned int deaths;
	float ratio;
	float damageInflicted;
};
}