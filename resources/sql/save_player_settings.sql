UPDATE player_settings
SET pms_enabled = $2, notification_position = $3
WHERE account_id = $1;