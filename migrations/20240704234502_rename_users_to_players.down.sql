-- players table
ALTER TABLE players
RENAME TO users;

ALTER SEQUENCE players_id_seq
RENAME TO users_id_seq;

ALTER TABLE users
RENAME CONSTRAINT players_pk TO users_pk;

ALTER TABLE users
RENAME CONSTRAINT players_unique TO users_unique;

-- admins table
ALTER TABLE admins
RENAME COLUMN account_id TO user_id;

ALTER TABLE admins
RENAME CONSTRAINT admins_players_fk TO admins_users_fk;

ALTER SEQUENCE admins_account_id_seq
RENAME TO admins_user_id_seq;

-- bans table
ALTER TABLE bans
RENAME COLUMN account_id TO user_id;

ALTER TABLE bans
RENAME CONSTRAINT bans_players_fk TO bans_users_fk;

ALTER TABLE bans
RENAME CONSTRAINT bans_players_fk_1 TO bans_users_fk_1;
