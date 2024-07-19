#pragma once

#include "../deathmatch/Maps.hpp"
#include "../deathmatch/WeaponSet.hpp"

#include <player.hpp>

#include <vector>

namespace Modes::Duel
{
struct DuelOffer
{
	Deathmatch::Map map;
	std::vector<PlayerWeapon> weapons;
	Deathmatch::WeaponSet weaponSet
		= Deathmatch::WeaponSet(Deathmatch::WeaponSet::Value::Run);
	unsigned int roundCount;
	float defaultHealth;
	float defaultArmour;
	IPlayer& from;
	IPlayer* to;
};
}