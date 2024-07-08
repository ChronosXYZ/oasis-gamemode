#pragma once

#include "../modes/Modes.hpp"

namespace Core
{
struct PlayerTempData
{
	bool isLoggedIn = false;
	Modes::Mode currentMode = Modes::Mode::None;
	Modes::Mode lastMode = Modes::Mode::None;
	bool skinSelectionMode = false;

	unsigned int subsequentKills = 0;
	bool lastPlayerOnFireState = false;
};
}