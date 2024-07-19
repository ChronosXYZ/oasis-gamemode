#pragma once

#include "../../modes/Modes.hpp"

#include <chrono>
#include <player.hpp>

#include <unordered_set>

namespace Core::Utils::Events
{
struct PlayerOnFireEvent
{
	IPlayer& player;
	IPlayer& lastKillee;
	Modes::Mode mode;
};

struct PlayerOnFireBeenKilled
{
	IPlayer& player;
	IPlayer& killer;
	Modes::Mode mode;
};

struct RoundEndEvent
{
	Modes::Mode mode;
	std::unordered_set<IPlayer*> players;
};

struct PlayerJoinedMode
{
	IPlayer& player;
	Modes::Mode mode;
	Modes::JoinData joinData;
};

struct X1ArenaWin
{
	IPlayer& winner;
	IPlayer& loser;
	float armourLeft;
	float healthLeft;
	int weapon;
	float distance;
	std::chrono::seconds fightDuration;
};

struct DuelWin
{
	IPlayer& winner;
	IPlayer& loser;
	float armourLeft;
	float healthLeft;
	int weapon;
	float distance;
	std::chrono::seconds fightDuration;
};
}