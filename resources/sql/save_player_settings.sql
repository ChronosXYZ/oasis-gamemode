UPDATE player_settings
SET pms_enabled = $1, notification_position = $2
WHERE account_id = $3;