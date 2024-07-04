CREATE TABLE general_player_stats (
    account_id serial4,
    playtime_score INTEGER NOT NULL,
	CONSTRAINT general_player_stats_account_id_fk 
        FOREIGN KEY (account_id) REFERENCES public.players(id)
            ON DELETE CASCADE
            ON UPDATE CASCADE
);

CREATE TABLE dm_player_stats (
    account_id SERIAL4,
    score INTEGER NOT NULL,
    highest_kill_streak INTEGER NOT NULL,
    kills INTEGER NOT NULL,
    deaths INTEGER NOT NULL,
    handgun_kills INTEGER NOT NULL,
    smg_kills INTEGER NOT NULL,
    assault_rifles_kills INTEGER NOT NULL,
    rifles_kills INTEGER NOT NULL,
    heavy_weapons_kills INTEGER NOT NULL,
    handheld_weapon_kills INTEGER NOT NULL,
    explosives_kills INTEGER NOT NULL,
	CONSTRAINT dm_player_stats_account_id_fk 
        FOREIGN KEY (account_id) REFERENCES public.players(id)
            ON DELETE CASCADE
            ON UPDATE CASCADE
);

CREATE TABLE freeroam_player_stats (
    account_id SERIAL4,
    score INTEGER NOT NULL,
    kills INTEGER NOT NULL,
    on_fire_players_kills INTEGER NOT NULL,
    times_on_fire INTEGER NOT NULL,
	CONSTRAINT freeroam_player_stats_account_id_fk 
        FOREIGN KEY (account_id) REFERENCES public.players(id)
            ON DELETE CASCADE
            ON UPDATE CASCADE
);

CREATE TABLE derby_player_stats (
    account_id SERIAL4,
    score INTEGER NOT NULL,
    drops INTEGER NOT NULL,
    dropped INTEGER NOT NULL,
    rounds_won INTEGER NOT NULL,
	CONSTRAINT derby_player_stats_account_id_fk 
        FOREIGN KEY (account_id) REFERENCES public.players(id)
            ON DELETE CASCADE
            ON UPDATE CASCADE
);

CREATE TABLE cnr_player_stats (
    account_id SERIAL4,
    score INTEGER NOT NULL,
    places_robbed INTEGER NOT NULL,
    heists INTEGER NOT NULL,
    kills INTEGER NOT NULL,
    deaths INTEGER NOT NULL,
    arrests INTEGER NOT NULL,
    cops_killed INTEGER NOT NULL,
    criminals_killed INTEGER NOT NULL,
    biggest_robbery_amount INTEGER NOT NULL,
	CONSTRAINT cnr_player_stats_account_id_fk 
        FOREIGN KEY (account_id) REFERENCES public.players(id)
            ON DELETE CASCADE
            ON UPDATE CASCADE
);

CREATE TABLE ptp_player_stats (
    account_id serial4,
    score INTEGER NOT NULL,
    kills INTEGER NOT NULL,
    deaths INTEGER NOT NULL,
    president_kills INTEGER NOT NULL,
    generals_kills INTEGER NOT NULL,
    rounds_won INTEGER NOT NULL,
    fbi_kills INTEGER NOT NULL,
    police_kills INTEGER NOT NULL,
    mafia_kills INTEGER NOT NULL,
    psycho_kills INTEGER NOT NULL,
	CONSTRAINT ptp_player_stats_account_id_fk 
        FOREIGN KEY (account_id) REFERENCES public.players(id)
            ON DELETE CASCADE
            ON UPDATE CASCADE
);

CREATE TABLE group_war_player_stats (
    account_id serial4,
    kills INTEGER NOT NULL,
    highest_kill_streak INTEGER NOT NULL,
    total_zone_wars INTEGER NOT NULL,
    wars_won INTEGER NOT NULL,
    handgun_kills INTEGER NOT NULL,
    smg_kills INTEGER NOT NULL,
    assault_rifles_kills INTEGER NOT NULL,
    rifles_kills INTEGER NOT NULL,
    heavy_weapons_kills INTEGER NOT NULL,
    handheld_weapon_kills INTEGER NOT NULL,
    explosives_kills INTEGER NOT NULL,
	CONSTRAINT group_war_player_stats_account_id_fk 
        FOREIGN KEY (account_id) REFERENCES public.players(id)
            ON DELETE CASCADE
            ON UPDATE CASCADE
)