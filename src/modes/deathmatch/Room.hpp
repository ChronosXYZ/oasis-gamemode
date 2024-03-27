#pragma once

#include "DeathmatchResult.hpp"
#include "Maps.hpp"
#include "WeaponSet.hpp"
#include "values.hpp"

#include <atomic>
#include <chrono>
#include <optional>
#include <player.hpp>

#include <ratio>
#include <string>
#include <vector>

namespace Modes::Deathmatch
{
struct Room
{
	/// Room map
	Map map;

	/// Allowed weapons in the room
	std::array<PlayerWeapon, MAX_WEAPON_SLOTS> allowedWeapons;

	/// Type of room
	WeaponSet weaponSet;

	/// Basically list of player IDs which joined the room.
	std::vector<int> playerIds;

	/// Room host is a player who created the room. Set to 'Server' if this is server-created room.
	std::optional<std::string> host;

	/// Room virtual world
	unsigned int virtualWorld;

	/// Whether +C-bug is enabled for the room
	bool cbugEnabled;

	/// State variable to indicate round time
	std::chrono::seconds countdown;

	/// Default round time
	std::chrono::seconds defaultTime;

	/// Set to true on round end
	bool isRestarting;

	/// Set to true on round start 3-2-1 countdown
	bool isStarting;

	/// Last round results in form of string
	std::optional<std::string> cachedLastResult;
};
}