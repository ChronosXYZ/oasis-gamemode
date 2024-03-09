#pragma once

#include "Maps.hpp"
#include "WeaponSet.hpp"
#include "values.hpp"

#include <optional>
#include <player.hpp>

#include <string>
#include <vector>

namespace Modes::Deathmatch
{
struct Room
{
	Map map;
	std::array<PlayerWeapon, MAX_WEAPON_SLOTS> allowedWeapons;
	WeaponSet weaponSet;
	std::vector<int> playerIds;
	std::optional<std::string> host;
	unsigned int virtualWorld;
	bool cbugEnabled;
};
}