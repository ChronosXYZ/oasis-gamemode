#pragma once

#include "../../core/utils/Localization.hpp"

#include <values.hpp>
#include <player.hpp>

#include <array>
#include <string>
#include <vector>

namespace Modes::Deathmatch
{

class WeaponSet
{
public:
	enum class Value : unsigned char
	{
		Run,
		Walk,
		DSS,
		Team,
		Custom
	};
	WeaponSet() = default;
	constexpr WeaponSet(Value weaponSet)
		: value(weaponSet)
	{
	}

	constexpr operator Value() const { return value; }
	explicit operator bool() const = delete;

	constexpr bool operator==(WeaponSet a) const { return value == a.value; }
	constexpr bool operator!=(WeaponSet a) const { return value != a.value; }

	std::array<PlayerWeapon, MAX_WEAPON_SLOTS> getWeapons() const
	{
		switch (value)
		{
		case Value::Run:
			return { PlayerWeapon_Sawedoff, PlayerWeapon_UZI,
				PlayerWeapon_Colt45 };
		case Value::Walk:
			return { PlayerWeapon_Deagle, PlayerWeapon_Shotgun,
				PlayerWeapon_Sniper };
		case Value::DSS:
			return { PlayerWeapon_Deagle };
		default:
			return {};
		}
	}

	std::string toString(IPlayer& player)
	{
		switch (value)
		{
		case Value::Run:
			return _("Run", player);
		case Value::Walk:
			return _("Walk", player);
		case Value::DSS:
			return _("DSS", player);
		case Value::Custom:
			return _("Custom", player);
		case Value::Team:
			return _("Team", player);
		default:
			return "";
		}
	}

private:
	Value value;
};
}