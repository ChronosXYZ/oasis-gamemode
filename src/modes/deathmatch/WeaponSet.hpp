#pragma once

#include "../../core/utils/Localization.hpp"

#include <player.hpp>
#include <string>

namespace Modes::Deathmatch
{
enum class WeaponSet
{
	Run,
	Walk,
	DSS,
	Team,
	Custom
};

inline const std::string weaponSetToString(WeaponSet set, IPlayer& player)
{
	switch (set)
	{
	case WeaponSet::Run:
		return _("Run", player);
	case WeaponSet::Walk:
		return _("Walk", player);
	case WeaponSet::DSS:
		return _("DSS", player);
	case WeaponSet::Custom:
		return _("Custom", player);
	case WeaponSet::Team:
		return _("Team", player);
	default:
		return "";
	}
}
}