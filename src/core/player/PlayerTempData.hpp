#pragma once

#include "../PlayerTempData.hpp"
#include "../auth/PlayerTempData.hpp"
#include "../../modes/deathmatch/PlayerTempData.hpp"
#include "../../modes/x1/X1PlayerTempData.hpp"
#include "../../modes/freeroam/PlayerTempData.hpp"

#include <memory>

namespace Core::Player
{
struct PlayerTempData
{
	std::unique_ptr<Core::Auth::PlayerTempData> auth
		= std::make_unique<Core::Auth::PlayerTempData>();
	std::unique_ptr<Core::PlayerTempData> core
		= std::make_unique<Core::PlayerTempData>();

	std::unique_ptr<Modes::Freeroam::PlayerTempData> freeroam
		= std::make_unique<Modes::Freeroam::PlayerTempData>();
	std::unique_ptr<Modes::Deathmatch::PlayerTempData> deathmatch;
	std::unique_ptr<Modes::X1::X1PlayerTempData> x1
		= std::make_unique<Modes::X1::X1PlayerTempData>();
};
}