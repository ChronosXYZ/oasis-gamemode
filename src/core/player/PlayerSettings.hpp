#pragma once

#include <pqxx/pqxx>

namespace Core::Player
{
struct PlayerSettings {
    bool pmsEnabled;

    PlayerSettings() = default;

    void updateFromRow(const pqxx::row& row) {
        pmsEnabled = row["pms_enabled"].as<bool>();
    }
};
}