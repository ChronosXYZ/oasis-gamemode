#pragma once

#include <string>

namespace Modes::Deathmatch
{
struct DeathmatchResult
{
	std::string playerName;
	unsigned int kills;
	unsigned int deaths;
	float ratio;
};
}