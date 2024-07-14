#pragma once

namespace Modes::Deathmatch
{
struct X1PlayerTempData
{
	unsigned int roomId;
	unsigned int subsequentKills;
	bool endArena = false;
};
}