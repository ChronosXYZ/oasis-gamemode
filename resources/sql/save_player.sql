UPDATE players
SET "language"=$2, last_skin_id=$3, last_ip=$4, last_login_at=$5
WHERE id=$1;