#pragma once

#include "../modes/Modes.hpp"

namespace Core
{
struct PlayerTempData
{
	bool isLoggedIn = false;
	Modes::Mode currentMode = Modes::Mode::None;
	bool skinSelectionMode = false;
};
}