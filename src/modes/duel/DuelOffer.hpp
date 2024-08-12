#pragma once

#include "../deathmatch/Maps.hpp"
#include "../deathmatch/WeaponSet.hpp"

#include <optional>
#include <player.hpp>

#include <vector>

namespace Modes::Duel
{
struct DuelOffer
{
	Deathmatch::Map map;
	std::vector<PlayerWeapon> weapons;
	Deathmatch::WeaponSet weaponSet;
	unsigned int roundCount;
	float defaultHealth;
	float defaultArmour;
	IPlayer* from;
	IPlayer* to;
	std::optional<unsigned int> tempRoomId;
};
}