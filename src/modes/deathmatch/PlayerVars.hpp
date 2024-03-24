#pragma once

#include <string>
namespace Modes::Deathmatch::PlayerVars
{
using namespace std::string_literals;

inline const auto ROOM_ID = "DM_ROOM_ID"s;
inline const auto LAST_SHOOT_TIME = "DM_LAST_SHOOT_TIME"s;
inline const auto CBUGGING = "DM_CBUGGING"s;
inline const auto CBUG_FREEZE_TIMER_ID = "DM_CBUG_FREEZE_TIMER_ID"s;
}