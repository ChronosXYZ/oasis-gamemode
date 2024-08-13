#pragma once

#include "../deathmatch/Maps.hpp"
#include "Server/Components/Timers/timers.hpp"

#include <chrono>
#include <optional>
#include <player.hpp>
#include <string>
#include <vector>

namespace Modes::Duel
{
struct Room
{
	/// Room map
	Deathmatch::Map map;

	/// Allowed weapons in the room
	std::array<PlayerWeapon, MAX_WEAPON_SLOTS> allowedWeapons;

	/// A list of players which joined the room.
	std::vector<IPlayer*> players;

	/// Room virtual world
	unsigned int virtualWorld;

	/// State variable to indicate round time
	std::chrono::seconds countdown;

	/// Default round time
	std::chrono::seconds defaultTime;

	float defaultHealth = 100.0;

	float defaultArmor = 0.0;

	std::chrono::time_point<std::chrono::system_clock> fightStarted;

	unsigned int currentRound;
	unsigned int maxRounds;
	std::optional<ITimer*> roundStartTimer;
	bool roundStarted;
	std::optional<IPlayer*> lastWinner;

	std::optional<std::vector<std::vector<std::string>>> cachedResults;

	template <typename... T>
	void sendMessageToAll(const std::string& message, const T&... args);
};
}