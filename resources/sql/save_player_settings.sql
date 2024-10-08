UPDATE player_settings
SET pms_enabled = $1
WHERE account_id = $2;