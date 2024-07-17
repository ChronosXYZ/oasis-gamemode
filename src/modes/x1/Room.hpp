#pragma once

#include "../deathmatch/Maps.hpp"
#include "../deathmatch/WeaponSet.hpp"

#include <chrono>
#include <player.hpp>
#include <unordered_set>

namespace Modes::X1
{
struct Room
{
	/// Room map
	Deathmatch::Map map;

	/// Allowed weapons in the room
	std::array<PlayerWeapon, MAX_WEAPON_SLOTS> allowedWeapons;

	/// Type of room
	Deathmatch::WeaponSet weaponSet;

	/// A list of players which joined the room.
	std::unordered_set<IPlayer*> players;

	/// Room virtual world
	unsigned int virtualWorld;

	/// State variable to indicate round time
	std::chrono::seconds countdown;

	/// Default round time
	std::chrono::seconds defaultTime;

	float defaultHealth = 100.0;

	float defaultArmor = 0.0;

	std::chrono::time_point<std::chrono::system_clock> fightStarted;

	template <typename... T>
	void sendMessageToAll(const std::string& message, const T&... args);
};
}