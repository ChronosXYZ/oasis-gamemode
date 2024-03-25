#pragma once

#include <optional>

namespace Modes::Freeroam
{
struct PlayerTempData
{
	std::optional<int> lastVehicleId;
};
}