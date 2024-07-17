#pragma once

#include <player.hpp>

#include <numbers>
#include <string>

namespace Core::Utils
{
enum class WeaponType
{
	Hand,
	Melee,
	Handguns,
	Shotguns,
	SMG,
	AssaultRifles,
	Rifles,
	HeavyWeapons,
	Explosives,
	HandheldItems,
	Unknown
};

const inline double deg2Rad(const double& degrees)
{
	return degrees * std::numbers::pi / 180;
}

template <typename E>
constexpr typename std::underlying_type<E>::type to_underlying(E e) noexcept
{
	return static_cast<typename std::underlying_type<E>::type>(e);
}

inline const WeaponType getWeaponType(int weaponId)
{
	switch (weaponId)
	{
	case PlayerWeapon_Fist:
	case PlayerWeapon_BrassKnuckle:
		return WeaponType::Hand;
	case PlayerWeapon_Knife:
	case PlayerWeapon_GolfClub:
	case PlayerWeapon_Shovel:
	case PlayerWeapon_PoolStick:
	case PlayerWeapon_NiteStick:
	case PlayerWeapon_Bat:
	case PlayerWeapon_Katana:
	case PlayerWeapon_Chainsaw:
	case PlayerWeapon_Flower:
	case PlayerWeapon_Cane:
	case PlayerWeapon_Dildo:
		return WeaponType::HandheldItems;
	case PlayerWeapon_Deagle:
	case PlayerWeapon_Silenced:
	case PlayerWeapon_Colt45:
		return WeaponType::Handguns;
	case PlayerWeapon_Sawedoff:
	case PlayerWeapon_Shotgun:
	case PlayerWeapon_Shotgspa:
		return WeaponType::Shotguns;
	case PlayerWeapon_TEC9:
	case PlayerWeapon_UZI:
	case PlayerWeapon_MP5:
		return WeaponType::SMG;
	case PlayerWeapon_AK47:
	case PlayerWeapon_M4:
		return WeaponType::AssaultRifles;
	case PlayerWeapon_Sniper:
	case PlayerWeapon_Rifle:
		return WeaponType::Rifles;
	case PlayerWeapon_FlameThrower:
	case PlayerWeapon_RocketLauncher:
	case PlayerWeapon_HeatSeeker:
	case PlayerWeapon_Minigun:
		return WeaponType::HeavyWeapons;
	}
	return WeaponType::Unknown;
}

inline const std::string getWeaponName(int weaponId)
{
	switch (weaponId)
	{
	case PlayerWeapon_Fist:
		return "Fist";
	case PlayerWeapon_BrassKnuckle:
		return "Brass Knuckle";
	case PlayerWeapon_Knife:
		return "Knife";
	case PlayerWeapon_GolfClub:
		return "Golf Club";
	case PlayerWeapon_Shovel:
		return "Shovel";
	case PlayerWeapon_PoolStick:
		return "Pool Stick";
	case PlayerWeapon_NiteStick:
		return "Baton";
	case PlayerWeapon_Bat:
		return "Baseball Bat";
	case PlayerWeapon_Katana:
		return "Katana";
	case PlayerWeapon_Chainsaw:
		return "Chainsaw";
	case PlayerWeapon_Flower:
		return "Flowers";
	case PlayerWeapon_Cane:
		return "Cane";
	case PlayerWeapon_Dildo:
		return "Dildo";
	case PlayerWeapon_Deagle:
		return "Desert Eagle";
	case PlayerWeapon_Silenced:
		return "Silenced Pistol";
	case PlayerWeapon_Colt45:
		return "9mm";
	case PlayerWeapon_Sawedoff:
		return "Sawn-off Shotgun";
	case PlayerWeapon_Shotgun:
		return "Shotgun";
	case PlayerWeapon_Shotgspa:
		return "Combat Shotgun";
	case PlayerWeapon_TEC9:
		return "TEC-9";
	case PlayerWeapon_UZI:
		return "UZI";
	case PlayerWeapon_MP5:
		return "MP5";
	case PlayerWeapon_AK47:
		return "AK-47";
	case PlayerWeapon_M4:
		return "M4";
	case PlayerWeapon_Sniper:
		return "Sniper Rifle";
	case PlayerWeapon_Rifle:
		return "Country Rifle";
	case PlayerWeapon_FlameThrower:
		return "Flame Thrower";
	case PlayerWeapon_RocketLauncher:
		return "Rocket Launcher";
	case PlayerWeapon_HeatSeeker:
		return "Heat Seeker";
	case PlayerWeapon_Minigun:
		return "Minigun";
	default:
		return "Unknown";
	}
}

// trim from start (in place)
inline void ltrim(std::string& s)
{
	s.erase(s.begin(),
		std::find_if(s.begin(), s.end(),
			[](unsigned char ch)
			{
				return !std::isspace(ch);
			}));
}

// trim from end (in place)
inline void rtrim(std::string& s)
{
	s.erase(std::find_if(s.rbegin(), s.rend(),
				[](unsigned char ch)
				{
					return !std::isspace(ch);
				})
				.base(),
		s.end());
}

// trim from both ends (in place)
inline void trim(std::string& s)
{
	rtrim(s);
	ltrim(s);
}

// trim from both ends (copying)
inline std::string trim_copy(std::string s)
{
	trim(s);
	return s;
}
}