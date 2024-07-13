#pragma once

#include "Maps.hpp"
#include "Server/Components/Timers/timers.hpp"
#include "WeaponSet.hpp"
#include "values.hpp"

#include <atomic>
#include <chrono>
#include <optional>
#include <player.hpp>

#include <ratio>
#include <string>
#include <unordered_set>
#include <vector>

namespace Modes::Deathmatch
{
class PrivacyMode
{
public:
	enum class Value
	{
		Everyone,
		OnlyFriends,
		OnlyGroup
	};
	PrivacyMode() = default;
	constexpr PrivacyMode(Value privacyMode)
		: value(privacyMode)
	{
	}

	constexpr operator Value() const { return value; }
	explicit operator bool() const = delete;

	constexpr bool operator==(PrivacyMode a) const { return value == a.value; }
	constexpr bool operator!=(PrivacyMode a) const { return value != a.value; }

	std::string toString(IPlayer& player)
	{
		switch (value)
		{
		case Value::Everyone:
			return _("Everyone", player);
		case Value::OnlyFriends:
			return _("Only friends", player);
		case Value::OnlyGroup:
			return _("Only group members", player);
		default:
			return "";
		}
	}

private:
	Value value;
};

struct Room
{
	/// Room map
	Map map;

	/// Allowed weapons in the room
	std::array<PlayerWeapon, MAX_WEAPON_SLOTS> allowedWeapons;

	/// Type of room
	WeaponSet weaponSet;

	/// A list of players which joined the room.
	std::unordered_set<IPlayer*> players;

	/// Room host is a player who created the room. Set to 'Server' if this is
	/// server-created room.
	std::optional<std::string> host;

	/// Room virtual world
	unsigned int virtualWorld;

	/// Whether +C-bug is enabled for the room
	bool cbugEnabled;

	/// State variable to indicate round time
	std::chrono::seconds countdown;

	/// Default round time
	std::chrono::seconds defaultTime;

	float defaultHealth = 100.0;

	float defaultArmor = 0.0;

	bool refillEnabled = true;

	bool randomMap = true;

	/// Set to true on round end
	bool isRestarting;

	/// Set to true on round start 3-2-1 countdown
	bool isStarting;

	/// Last round results in form of string
	std::optional<std::string> cachedLastResult;

	PrivacyMode privacyMode = PrivacyMode(PrivacyMode::Value::Everyone);

	std::optional<ITimer*> roundStartTimer;
	unsigned int roundStartTimerCount;

	template <typename... T>
	void sendMessageToAll(const std::string& message, const T&... args);
};
}