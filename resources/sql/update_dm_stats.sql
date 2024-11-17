UPDATE dm_player_stats
SET score=$2,
    highest_kill_streak=$3, 
    kills=$4, 
    deaths=$5, 
    hand_kills=$6, 
    handheld_weapon_kills=$7, 
    melee_kills=$8,
    handgun_kills=$9,
    shotgun_kills=$10,
    smg_kills=$11,
    assault_rifles_kills=$12,
    rifles_kills=$13,
    heavy_weapon_kills=$14,
    explosives_kills=$15
WHERE account_id=$1
