#pragma once

#include "Room.hpp"
#include <cstddef>
#include <ctime>
#include <optional>
#include <string>

namespace Modes::Deathmatch
{
struct PlayerTempData
{
	unsigned int roomId;
	std::time_t lastShootTime = 0;
	bool cbugging = false;
	std::optional<std::string> cbugFreezeTimerId;
	std::optional<Room> temporaryRoomSettings; // used for rooms creating

	unsigned int kills = 0;
	unsigned int deaths = 0;
	unsigned int subsequentKills = 0;
	float ratio = 0.0;
	float damageInflicted = 0.0;

	inline void increaseKills()
	{
		this->kills += 1;
		this->updateRatio();
	}

	inline void increaseDeaths()
	{
		this->deaths += 1;
		this->updateRatio();
	}

	inline void resetKD()
	{
		this->kills = 0;
		this->deaths = 0;
		this->ratio = 0.0;
		this->damageInflicted = 0.0;
	}

	inline void updateRatio()
	{
		auto deathsLocal = this->deaths;
		if (deathsLocal == 0)
		{
			deathsLocal = 1;
		}
		this->ratio = float(kills) / float(deathsLocal);
	}
};
}