#pragma once

#include "PlayerModel.hpp"
#include "TextDrawManager.hpp"
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
#include <utility>

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
	void sendInfoMessage(const std::string& message);

	template <typename... T>
	inline void sendInfoMessage(const std::string& message, T&&... args)
	{
		_player.sendClientMessage(Colour::White(),
			fmt::sprintf("%s %s", _("#LIME#>>#WHITE#", _player),
				fmt::sprintf(_(message, _player), std::forward<T>(args)...)));
	}

	template <typename... T>
	inline void sendErrorMessage(const std::string& message, T&&... args)
	{
		_player.sendClientMessage(Colour::White(),
			fmt::format("{} {}", _("#RED#[ERROR]#WHITE#", _player),
				fmt::sprintf(_(message, _player), std::forward<T>(args)...)));
	}

	void showNotification(const std::string& notification, unsigned int seconds = 3,
		unsigned int notificationSound = 0);
	template <typename... T>
	inline void sendTranslatedMessage(const std::string& message, T&&... args)
	{
		_player.sendClientMessage(Colour::White(),
			fmt::sprintf(_(message, _player), std::forward<T>(args)...));
	}

	template <typename... T>
	inline void sendModeMessage(const std::string& message, T&&... args)
	{
		auto mode = this->getMode();
		this->_player.sendClientMessage(Colour::White(),
			fmt::sprintf("%s {%s}%s{FFFFFF}: %s", _("#LIME#>>#WHITE#", _player),
				Modes::getModeColor(mode), Modes::getModeShortName(mode),
				fmt::sprintf(_(message, _player), std::forward<T>(args)...)));
	}

	template <typename... T>
	inline void sendModeMessage(
		Modes::Mode mode, const std::string& message, T&&... args)
	{
		this->_player.sendClientMessage(Colour::White(),
			fmt::sprintf("%s {%s}%s{FFFFFF}: %s", _("#LIME#>>#WHITE#", _player),
				Modes::getModeColor(mode), Modes::getModeShortName(mode),
				fmt::sprintf(_(message, _player), std::forward<T>(args)...)));
	}

	const std::string getIP();
	float getVehicleSpeed();
	bool isInMode(Modes::Mode mode);
	bool isInAnyMode();
	bool isAuthorized();
	unsigned int getNormalizedColor();
	const Modes::Mode& getMode();

	template <typename... T>
	inline void sendGameText(
		const std::string& message, Milliseconds time, int style, T&&... args)
	{
		this->_player.sendGameText(
			fmt::sprintf(_(message, this->_player), std::forward<T>(args)...),
			time, style);
	}

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