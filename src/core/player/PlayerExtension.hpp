#pragma once

#include "PlayerModel.hpp"

#include <fmt/core.h>
#include <player.hpp>
#include <component.hpp>
#include <memory>
#include <sdk.hpp>
#include <thread>
#include <chrono>

#define DELAYED_KICK_INTERVAL_MS 500
#define OASIS_PLAYER_EXT_UID 0xBE727855C7D51E32

namespace Core::Player
{
class OasisPlayerExt : public IExtension
{
private:
	std::shared_ptr<PlayerModel> _playerData = nullptr;
	IPlayer& _player;

public:
	PROVIDE_EXT_UID(OASIS_PLAYER_EXT_UID)

	OasisPlayerExt(std::shared_ptr<PlayerModel> data, IPlayer& player);

	std::shared_ptr<PlayerModel> getPlayerData();

	void delayedKick();
	void setFacingAngle(float angle);
	void sendErrorMessage(const std::string& message);
	void sendInfoMessage(const std::string& message);
	const std::string getIP();

	void freeExtension() override;
	void reset() override;
};

inline static OasisPlayerExt* getPlayerExt(IPlayer& player)
{
	return queryExtension<OasisPlayerExt>(player);
};
}