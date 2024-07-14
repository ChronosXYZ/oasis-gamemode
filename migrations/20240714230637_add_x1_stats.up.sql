CREATE TABLE x1_player_stats (
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
	CONSTRAINT x1_player_stats_account_id_fk 
        FOREIGN KEY (account_id) REFERENCES public.players(id)
            ON DELETE CASCADE
            ON UPDATE CASCADE
);

INSERT INTO public.x1_player_stats
(
    account_id
)
SELECT
    id
FROM
    public.players;

CREATE OR REPLACE FUNCTION create_stats_rows_after_insert()
RETURNS TRIGGER AS $$
BEGIN
    INSERT INTO general_player_stats ( account_id )
    VALUES(NEW.id);

    INSERT INTO dm_player_stats ( account_id )
    VALUES(NEW.id);

    INSERT INTO freeroam_player_stats ( account_id )
    VALUES(NEW.id);

    INSERT INTO derby_player_stats ( account_id )
    VALUES(NEW.id);

    INSERT INTO cnr_player_stats ( account_id )
    VALUES(NEW.id);

    INSERT INTO ptp_player_stats ( account_id )
    VALUES(NEW.id);

    INSERT INTO group_war_player_stats ( account_id )
    VALUES(NEW.id);

    INSERT INTO x1_player_stats ( account_id )
    VALUES(NEW.id);

    RETURN NEW;
END;
$$ LANGUAGE plpgsql;