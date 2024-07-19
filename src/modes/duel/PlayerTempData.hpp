#pragma once

namespace Modes::Duel
{
struct PlayerTempData
{
	unsigned int roomId;
	unsigned int subsequentKills;
	bool duelEnd = false;
};
}