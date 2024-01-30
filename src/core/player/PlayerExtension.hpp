#pragma once

#include "PlayerModel.hpp"
#include <player.hpp>
#include <component.hpp>
#include <memory>
#include <sdk.hpp>
#include <thread>
#include <chrono>

#define DELAYED_KICK_INTERVAL_MS 500

namespace Core
{
class OasisPlayerExt : public IExtension
{
private:
	std::shared_ptr<PlayerModel> _playerData = nullptr;
	IPlayer& serverPlayer;

public:
	PROVIDE_EXT_UID(0xBE727855C7D51E32)
	OasisPlayerExt(std::shared_ptr<PlayerModel> data, IPlayer& player)
		: serverPlayer(player)
	{
		_playerData = data;
	}

	std::shared_ptr<PlayerModel> getPlayerData()
	{
		return this->_playerData;
	}

	void delayedKick()
	{
		std::thread([&]()
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(DELAYED_KICK_INTERVAL_MS));
				serverPlayer.kick();
			})
			.detach();
	}

	void setFacingAngle(float angle)
	{
		auto rot = serverPlayer.getRotation().ToEuler();
		rot.z = angle;
		serverPlayer.setRotation(rot);
	}

	void freeExtension() override
	{
		_playerData.reset();
	}

	void reset() override
	{
		_playerData.reset();
	}
};
}