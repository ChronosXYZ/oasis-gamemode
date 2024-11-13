#pragma once

#include <pqxx/pqxx>
#include "../textdraws/Notification.hpp"

namespace Core::Player
{
struct PlayerSettings {
    bool pmsEnabled;
    TextDraws::NotificationPosition notificationPos = TextDraws::NotificationPosition::Bottom;

    PlayerSettings() = default;

    void updateFromRow(const pqxx::row& row) {
        pmsEnabled = row["pms_enabled"].as<bool>();
    }
};
}