UPDATE duel_player_stats
SET score=$1,
    highest_kill_streak=$2, 
    kills=$3, 
    deaths=$4, 
    hand_kills=$5, 
    handheld_weapon_kills=$6, 
    melee_kills=$7,
    handgun_kills=$8,
    shotgun_kills=$9,
    smg_kills=$10,
    assault_rifles_kills=$11,
    rifles_kills=$12,
    heavy_weapon_kills=$13,
    explosives_kills=$14
WHERE account_id=$15
