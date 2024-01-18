#pragma once

#include <sdk.hpp>

#include <memory>

#include "player.hpp"
#include "player/PlayerModel.hpp"

namespace Core
{
class Player
{
public:
	std::unique_ptr<PlayerModel> data;

	Player(std::unique_ptr<PlayerModel> playerData, IPlayer& serverPlayer)
		: data(std::move(playerData))
		, serverPlayer(serverPlayer)
	{
	}

	Player(IPlayer& player)
		: serverPlayer(player)
	{
		Player(make_unique<PlayerModel>(), player);
	}

	IPlayer& serverPlayer;
};
}