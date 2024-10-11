#pragma once

// #include "Room.hpp"
#include "Server/Components/Timers/timers.hpp"
#include <cstddef>
#include <ctime>
#include <optional>
#include <string>

namespace Modes::Derby
{
struct PlayerTempData
{
	bool cbugging = false;
	std::optional<ITimer*> cbugFreezeTimer;
};
}
