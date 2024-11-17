UPDATE player_settings
SET pms_enabled = $2, notification_position = $3, notify_on_give_damage = $4
WHERE account_id = $1;