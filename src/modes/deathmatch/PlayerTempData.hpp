#pragma once

#include <cstddef>
#include <ctime>
#include <optional>
#include <string>

namespace Modes::Deathmatch
{
struct PlayerTempData
{
	std::optional<std::size_t> roomId;
	std::optional<std::time_t> lastShootTime;
	bool cbugging = false;
	std::optional<std::string> cbugFreezeTimerId;
};
}