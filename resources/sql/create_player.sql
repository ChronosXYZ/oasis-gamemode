INSERT INTO players
("name", password_hash, "language", email, last_skin_id, last_ip)
VALUES ($1, $2, $3, $4, $5, $6);