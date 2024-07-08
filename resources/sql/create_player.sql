INSERT INTO players
("name", password_hash, "language", email, last_skin_id, last_ip)
VALUES ($1, $2, $3, $4, $5, $6)
RETURNING id INTO new_player_id;

INSERT INTO public.general_player_stats ( account_id )
VALUES (new_player_id);

INSERT INTO public.dm_player_stats ( account_id )
VALUES (new_player_id);

INSERT INTO public.freeroam_player_stats ( account_id )
VALUES (new_player_id);

INSERT INTO public.derby_player_stats ( account_id )
VALUES (new_player_id);

INSERT INTO public.cnr_player_stats ( account_id )
VALUES (new_player_id);

INSERT INTO public.ptp_player_stats ( account_id )
VALUES (new_player_id);

INSERT INTO public.group_war_player_stats ( account_id )
VALUES (new_player_id);