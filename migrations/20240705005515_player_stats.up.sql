CREATE TABLE general_player_stats (
    account_id serial4,
    playtime_score INTEGER NOT NULL DEFAULT 0,
	CONSTRAINT general_player_stats_account_id_fk 
        FOREIGN KEY (account_id) REFERENCES public.players(id)
            ON DELETE CASCADE
            ON UPDATE CASCADE
);

CREATE TABLE dm_player_stats (
    account_id SERIAL4,
    score INTEGER NOT NULL DEFAULT 0,
    highest_kill_streak INTEGER NOT NULL DEFAULT 0,
    kills INTEGER NOT NULL DEFAULT 0,
    deaths INTEGER NOT NULL DEFAULT 0,
    hand_kills INTEGER NOT NULL DEFAULT 0,
    handheld_weapon_kills INTEGER NOT NULL DEFAULT 0,
    melee_kills INTEGER NOT NULL DEFAULT 0,
    handgun_kills INTEGER NOT NULL DEFAULT 0,
    shotgun_kills INTEGER NOT NULL DEFAULT 0,
    smg_kills INTEGER NOT NULL DEFAULT 0,
    assault_rifles_kills INTEGER NOT NULL DEFAULT 0,
    rifles_kills INTEGER NOT NULL DEFAULT 0,
    heavy_weapon_kills INTEGER NOT NULL DEFAULT 0,
    explosives_kills INTEGER NOT NULL DEFAULT 0,
	CONSTRAINT dm_player_stats_account_id_fk 
        FOREIGN KEY (account_id) REFERENCES public.players(id)
            ON DELETE CASCADE
            ON UPDATE CASCADE
);

CREATE TABLE freeroam_player_stats (
    account_id SERIAL4,
    score INTEGER NOT NULL DEFAULT 0,
    kills INTEGER NOT NULL DEFAULT 0,
    on_fire_player_kills INTEGER NOT NULL DEFAULT 0,
    times_on_fire INTEGER NOT NULL DEFAULT 0,
	CONSTRAINT freeroam_player_stats_account_id_fk 
        FOREIGN KEY (account_id) REFERENCES public.players(id)
            ON DELETE CASCADE
            ON UPDATE CASCADE
);

CREATE TABLE derby_player_stats (
    account_id SERIAL4,
    score INTEGER NOT NULL DEFAULT 0,
    drops INTEGER NOT NULL DEFAULT 0,
    dropped INTEGER NOT NULL DEFAULT 0,
    rounds_won INTEGER NOT NULL DEFAULT 0,
	CONSTRAINT derby_player_stats_account_id_fk 
        FOREIGN KEY (account_id) REFERENCES public.players(id)
            ON DELETE CASCADE
            ON UPDATE CASCADE
);

CREATE TABLE cnr_player_stats (
    account_id SERIAL4,
    score INTEGER NOT NULL DEFAULT 0,
    total_places_robbed INTEGER NOT NULL DEFAULT 0,
    heists INTEGER NOT NULL DEFAULT 0,
    kills INTEGER NOT NULL DEFAULT 0,
    deaths INTEGER NOT NULL DEFAULT 0,
    arrests INTEGER NOT NULL DEFAULT 0,
    cop_kills INTEGER NOT NULL DEFAULT 0,
    criminal_kills INTEGER NOT NULL DEFAULT 0,
    biggest_robbery_amount INTEGER NOT NULL DEFAULT 0,
	CONSTRAINT cnr_player_stats_account_id_fk 
        FOREIGN KEY (account_id) REFERENCES public.players(id)
            ON DELETE CASCADE
            ON UPDATE CASCADE
);

CREATE TABLE ptp_player_stats (
    account_id serial4,
    score INTEGER NOT NULL DEFAULT 0,
    kills INTEGER NOT NULL DEFAULT 0,
    deaths INTEGER NOT NULL DEFAULT 0,
    president_kills INTEGER NOT NULL DEFAULT 0,
    generals_kills INTEGER NOT NULL DEFAULT 0,
    rounds_won INTEGER NOT NULL DEFAULT 0,
    fbi_kills INTEGER NOT NULL DEFAULT 0,
    police_kills INTEGER NOT NULL DEFAULT 0,
    mafia_kills INTEGER NOT NULL DEFAULT 0,
    psycho_kills INTEGER NOT NULL DEFAULT 0,
	CONSTRAINT ptp_player_stats_account_id_fk 
        FOREIGN KEY (account_id) REFERENCES public.players(id)
            ON DELETE CASCADE
            ON UPDATE CASCADE
);

CREATE TABLE group_war_player_stats (
    account_id serial4,
    kills INTEGER NOT NULL DEFAULT 0,
    highest_kill_streak INTEGER NOT NULL DEFAULT 0,
    total_zone_wars INTEGER NOT NULL DEFAULT 0,
    wars_won INTEGER NOT NULL DEFAULT 0,
    hand_kills INTEGER NOT NULL DEFAULT 0,
    handheld_weapon_kills INTEGER NOT NULL DEFAULT 0,
    melee_kills INTEGER NOT NULL DEFAULT 0,
    handgun_kills INTEGER NOT NULL DEFAULT 0,
    shotgun_kills INTEGER NOT NULL DEFAULT 0,
    smg_kills INTEGER NOT NULL DEFAULT 0,
    assault_rifles_kills INTEGER NOT NULL DEFAULT 0,
    rifles_kills INTEGER NOT NULL DEFAULT 0,
    heavy_weapon_kills INTEGER NOT NULL DEFAULT 0,
    explosives_kills INTEGER NOT NULL DEFAULT 0,
	CONSTRAINT group_war_player_stats_account_id_fk 
        FOREIGN KEY (account_id) REFERENCES public.players(id)
            ON DELETE CASCADE
            ON UPDATE CASCADE
);

INSERT INTO public.general_player_stats
(
    account_id
)
SELECT
    id
FROM
    players;


INSERT INTO public.dm_player_stats
(
    account_id
)
SELECT
    id
FROM
    public.players;

INSERT INTO public.freeroam_player_stats
(
    account_id
)
SELECT
    id
FROM
    public.players;

INSERT INTO public.derby_player_stats
(
    account_id
)
SELECT
    id
FROM
    public.players;

INSERT INTO public.cnr_player_stats
(
    account_id
)
SELECT
    id
FROM
    public.players;

INSERT INTO public.ptp_player_stats
(
    account_id
)
SELECT
    id
FROM
    public.players;

INSERT INTO public.group_war_player_stats
(
    account_id
)
SELECT
    id
FROM
    public.players;