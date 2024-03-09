#pragma once

#include "player/PlayerExtension.hpp"
#include "player/PlayerModel.hpp"
#include "../modes/Modes.hpp"

#include <magic_enum/magic_enum.hpp>
#include <memory>
#include <player.hpp>
#include <string>

namespace Core::PlayerVars
{
using namespace std::string_literals;

inline const auto IS_LOGGED_IN = "IS_LOGGED_IN"s;
inline const auto CURRENT_MODE = "CURRENT_MODE"s;
inline const auto SKIN_SELECTION_MODE = "SKIN_SELECTION_MODE"s;

inline Modes::Mode getPlayerMode(std::shared_ptr<PlayerModel> pData)
{
	return magic_enum::enum_cast<Modes::Mode>(
		*pData->getTempData<int>(CURRENT_MODE))
		.value_or(Modes::Mode::None);
}
}