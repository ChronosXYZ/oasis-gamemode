SELECT id, 
name,
players.password_hash,
"language",
email,
last_skin_id,
last_ip,
last_login_at,
registration_date,
pms_enabled,
bans.reason as "ban_reason",
bans.by as "banned_by",
bans.expires_at as "ban_expires_at",
admins."level" as "admin_level",
admins.password_hash as "admin_pass_hash"
FROM players
LEFT JOIN bans
ON players.id = bans.account_id
LEFT JOIN admins
ON players.id = admins.account_id
WHERE name=$1