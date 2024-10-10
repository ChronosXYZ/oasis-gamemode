ALTER TABLE public.players
ADD COLUMN pms_enabled boolean NOT NULL DEFAULT true;

UPDATE public.players
SET pms_enabled = ps.pms_enabled
FROM public.player_settings ps
WHERE players.id = ps.account_id;

DROP TABLE public.player_settings;

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

    INSERT INTO duel_player_stats ( account_id )
    VALUES(NEW.id);

    RETURN NEW;
END;
$$ LANGUAGE plpgsql;