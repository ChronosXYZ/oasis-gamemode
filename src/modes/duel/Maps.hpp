#pragma once

#include "../deathmatch/Maps.hpp"
#include <vector>

namespace Modes::Duel
{
inline const std::vector<Deathmatch::Map> ARENAS = {
	Deathmatch::Map {
		"Warehouse",
		Deathmatch::WeaponSet::Value::Custom,
		{
			{ 1413.35, -0.89, 1000.92, 152.14 },
			{ 1363.63, -38.27, 1000.92, 311.10 },
		},
		1,
	},
	Deathmatch::Map {
		"Ring Field",
		Deathmatch::WeaponSet::Value::Custom,
		{
			{ -1400.30, 1283.50, 1039.86, 185.79 },
			{ -1425.98, 1269.98, 1039.86, 231.83 },
		},
		16,
	},
	Deathmatch::Map {
		"SF Airport",
		Deathmatch::WeaponSet::Value::Custom,
		{
			{ -1183.46, -78.23, 14.14, 133.77 },
			{ -1206.58, -101.45, 14.14, 311.72 },
		},
		0,
	},
	Deathmatch::Map { "Auto School", Deathmatch::WeaponSet::Value::Custom,
		{
			{ 1106.8771, 1304.1173, 10.8203, 271.1833 },
			{ 1162.8754, 1304.7198, 10.8203, 269.6405 },
		},
		0 },
	Deathmatch::Map {
		"Baseball Field",
		Deathmatch::WeaponSet::Value::Custom,
		{
			{ 1308.19, 2187.19, 11.02, 241.62 },
			{ 1390.34, 2154.79, 11.02, 103.21 },
		},
		0,
	},
	Deathmatch::Map {
		"LVPD Police Dept.",
		Deathmatch::WeaponSet::Value::Custom,
		{
			{ 294.63, 185.48, 1007.17, 143.06 },
			{ 238.83, 144.05, 1003.02, 3.19 },
		},
		3,
	},
	Deathmatch::Map {
		"LV Roof Top",
		Deathmatch::WeaponSet::Value::Custom,
		{
			{ 2628.00, 1196.00, 26.92, 310.00 },
			{ 2656.85, 1188.17, 26.91, 43.14 },
		},
		0,
	},
	Deathmatch::Map {
		"LV Come a Lot",
		Deathmatch::WeaponSet::Value::Custom,
		{
			{ 2411.40, 1128.02, 34.25, 80.57 },
			{ 2362.88, 1179.92, 34.25, 224.87 },
		},
		0,
	},
	Deathmatch::Map {
		"LV Granule Roof",
		Deathmatch::WeaponSet::Value::Custom,
		{
			{ 2479.30, 1118.57, 21.14, 229.92 },
			{ 2497.57, 1084.59, 21.14, 356.80 },
		},
		0,
	},
	Deathmatch::Map {
		"LV Strain Hard",
		Deathmatch::WeaponSet::Value::Custom,
		{
			{ 2007.91, 2380.74, 23.85, 117.71 },
			{ 1947.16, 2381.27, 23.84, 263.13 },
		},
		0,
	},
	Deathmatch::Map {
		"SF Old Warehouse",
		Deathmatch::WeaponSet::Value::Custom,
		{
			{ -2102.85, -266.73, 35.32, 23.57 },
			{ -2100.06, -200.59, 35.32, 146.82 },
		},
		0,
	},
	Deathmatch::Map {
		"LV Driving School",
		Deathmatch::WeaponSet::Value::Custom,
		{
			{ 1165.97, 1328.67, 10.81, 144.78 },
			{ 1150.91, 1232.72, 10.82, 31.57 },
		},
		0,
	},
	Deathmatch::Map {
		"Open World",
		Deathmatch::WeaponSet::Value::Custom,
		{
			{ 88.03, 1040.43, 13.61, 326.64 },
			{ 134.47, 1062.72, 13.60, 86.88 },
		},
		0,
	},
	Deathmatch::Map {
		"Vinewood Roof",
		Deathmatch::WeaponSet::Value::Custom,
		{
			{ 965.34, -1180.76, 54.90, 247.50 },
			{ 1029.86, -1180.33, 54.90, 140.18 },
		},
		0,
	},
};

inline const Deathmatch::Map randomMap(const std::vector<Deathmatch::Map>& maps)
{
	std::random_device rd;
	std::mt19937 gen(rd());

	std::uniform_int_distribution<> dis(0, maps.size() - 1);
	return maps.at(dis(rd));
};
}