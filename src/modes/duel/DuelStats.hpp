#pragma once

#include <pqxx/pqxx>

namespace Modes::Duel
{
struct DuelStats
{
	unsigned int score = 0;
	unsigned int highestKillStreak = 0;
	unsigned int kills = 0;
	unsigned int deaths = 0;
	unsigned int handKills = 0;
	unsigned int meleeKills = 0;
	unsigned int handgunKills = 0;
	unsigned int shotgunKills = 0;
	unsigned int smgKills = 0;
	unsigned int assaultRiflesKills = 0;
	unsigned int riflesKills = 0;
	unsigned int heavyWeaponKills = 0;
	unsigned int handheldWeaponKills = 0;
	unsigned int explosivesKills = 0;

	DuelStats() = default;

	void updateFromRow(const pqxx::row& row)
	{
		score = row["score"].as<unsigned int>();
		highestKillStreak = row["highest_kill_streak"].as<unsigned int>();
		kills = row["kills"].as<unsigned int>();
		deaths = row["deaths"].as<unsigned int>();
		handKills = row["hand_kills"].as<unsigned int>();
		meleeKills = row["melee_kills"].as<unsigned int>();
		handgunKills = row["handgun_kills"].as<unsigned int>();
		shotgunKills = row["shotgun_kills"].as<unsigned int>();
		smgKills = row["smg_kills"].as<unsigned int>();
		assaultRiflesKills = row["assault_rifles_kills"].as<unsigned int>();
		riflesKills = row["rifles_kills"].as<unsigned int>();
		heavyWeaponKills = row["heavy_weapon_kills"].as<unsigned int>();
		handheldWeaponKills = row["handheld_weapon_kills"].as<unsigned int>();
		explosivesKills = row["explosives_kills"].as<unsigned int>();
	}
};
}