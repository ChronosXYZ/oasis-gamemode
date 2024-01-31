SELECT id, 
name,
users.password_hash,
"language",
email,
last_skin_id,
last_ip,
last_login_at,
registration_date,
bans.reason as "ban_reason",
bans.by as "banned_by",
bans.expires_at as "ban_expires_at",
admins."level" as "admin_level",
admins.password_hash as "admin_pass_hash"
FROM users
LEFT JOIN bans
ON users.id = bans.user_id
LEFT JOIN admins
ON users.id = admins.user_id
WHERE name=$1