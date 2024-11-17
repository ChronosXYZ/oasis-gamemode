#pragma once

#include <pqxx/pqxx>
#include "../textdraws/Notification.hpp"

namespace Core::Player
{
struct PlayerSettings {
    bool pmsEnabled;
    bool notifyOnGiveDamage = true;
    TextDraws::NotificationPosition notificationPos;

    PlayerSettings() = default;

    void updateFromRow(const pqxx::row& row) {
        pmsEnabled = row["pms_enabled"].as<bool>();
        notificationPos = (TextDraws::NotificationPosition)row["notification_position"].as<int>();
        notifyOnGiveDamage = row["notify_on_give_damage"].as<bool>();
    }
};
}