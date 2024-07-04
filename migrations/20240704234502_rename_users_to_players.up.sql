-- players table
ALTER TABLE users
RENAME TO players;

ALTER SEQUENCE users_id_seq
RENAME TO players_id_seq;

ALTER TABLE players
RENAME CONSTRAINT users_pk TO players_pk;

ALTER TABLE players
RENAME CONSTRAINT users_unique TO players_unique;

-- admins table
ALTER TABLE admins
RENAME COLUMN user_id TO account_id;

ALTER TABLE admins
RENAME CONSTRAINT admins_users_fk TO admins_players_fk;

ALTER SEQUENCE admins_user_id_seq
RENAME TO admins_account_id_seq;

-- bans table
ALTER TABLE bans
RENAME COLUMN user_id TO account_id;

ALTER TABLE bans
RENAME CONSTRAINT bans_users_fk TO bans_players_fk;

ALTER TABLE bans
RENAME CONSTRAINT bans_users_fk_1 TO bans_players_fk_1;