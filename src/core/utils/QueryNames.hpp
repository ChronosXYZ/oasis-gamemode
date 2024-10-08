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
inline const auto UPDATE_X1_STATS = "update_x1_stats"s;
inline const auto LOAD_X1_STATS_FOR_PLAYER = "load_x1_stats_for_player"s;
inline const auto UPDATE_DUEL_STATS = "update_duel_stats"s;
inline const auto LOAD_DUEL_STATS_FOR_PLAYER = "load_duel_stats_for_player"s;
inline const auto LOAD_PLAYER_SETTINGS = "load_player_settings"s;
inline const auto SAVE_PLAYER_SETTINGS = "save_player_settings"s;
}