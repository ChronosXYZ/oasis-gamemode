#pragma once

#include "PlayerModel.hpp"
#include "TextDrawManager.hpp"
#include "../textdraws/Notification.hpp"
#include "../../modes/Modes.hpp"

#include <types.hpp>
#include <Server/Components/Timers/timers.hpp>

#include <fmt/printf.h>
#include <player.hpp>
#include <component.hpp>
#include <sdk.hpp>

#include <memory>
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
	std::shared_ptr<TextDrawManager> _textDrawManager = nullptr;
	IPlayer& _player;
	ITimersComponent* _timerManager;

public:
	PROVIDE_EXT_UID(OASIS_PLAYER_EXT_UID)

	OasisPlayerExt(std::shared_ptr<PlayerModel> data, IPlayer& player,
		ITimersComponent* timerManager);

	std::shared_ptr<PlayerModel> getPlayerData();
	std::shared_ptr<TextDrawManager> getTextDrawManager();

	void delayedKick();
	void setFacingAngle(float angle);
	void sendErrorMessage(const std::string& message);
	void sendInfoMessage(const std::string& message);
	void sendTranslatedMessage(const std::string& message);
	void showNotification(const std::string& notification,
		TextDraws::NotificationPosition position, unsigned int seconds = 3,
		unsigned int notificationSound = 0);
	template <typename... T>
	inline void sendTranslatedMessageFormatted(
		const std::string& message, const T&... args)
	{
		_player.sendClientMessage(
			Colour::White(), fmt::sprintf(_(message, _player), args...));
	}

	const std::string getIP();
	float getVehicleSpeed();
	bool isInMode(Modes::Mode mode);
	bool isInAnyMode();
	bool isAuthorized();
	const Modes::Mode& getMode();

	void freeExtension() override;
	void reset() override;
};

inline static OasisPlayerExt* getPlayerExt(IPlayer& player)
{
	return queryExtension<OasisPlayerExt>(player);
};

inline static std::shared_ptr<PlayerModel> getPlayerData(IPlayer& player)
{
	return getPlayerExt(player)->getPlayerData();
};
}