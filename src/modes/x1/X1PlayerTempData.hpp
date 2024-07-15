#pragma once

namespace Modes::X1
{
struct X1PlayerTempData
{
	unsigned int roomId;
	unsigned int subsequentKills;
	bool endArena = false;
};
}