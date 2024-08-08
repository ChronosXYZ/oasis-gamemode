#pragma once

namespace Modes::Duel
{
struct PlayerTempData
{
	unsigned int roomId;
	unsigned int subsequentKills;
	unsigned int kills;
	unsigned int deaths;
	float ratio;
	float damageInflicted;
	bool duelEnd = false;

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