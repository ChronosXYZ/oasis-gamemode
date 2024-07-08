#pragma once

#include <string>

namespace Core::Utils::SQL::Queries
{
using namespace std::string_literals;

inline const auto LOAD_PLAYER = "load_player"s;
inline const auto CREATE_PLAYER = "create_player"s;
inline const auto SAVE_PLAYER = "save_player"s;
inline const auto UPDATE_DM_STATS = "update_dm_stats"s;
inline const auto LOAD_DM_STATS_FOR_PLAYER = "load_dm_stats_for_player"s;
}