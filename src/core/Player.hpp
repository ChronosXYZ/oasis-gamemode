#pragma once

#include <sdk.hpp>

#include <memory>

#include "PlayerData.hpp"

namespace Core
{
class Player
{
public:
	std::unique_ptr<PlayerData> data;

	Player(std::unique_ptr<PlayerData> playerData, IPlayer& serverPlayer)
		: data(std::move(playerData))
		, serverPlayer(serverPlayer)
	{
	}

public:
	IPlayer& serverPlayer;
};
}