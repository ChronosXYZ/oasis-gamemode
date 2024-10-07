UPDATE players
SET "language"=$1, last_skin_id=$2, last_ip=$3, last_login_at=$4, pms_enabled=$5
WHERE id=$6;