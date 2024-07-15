#pragma once

#include "ITextDrawWrapper.hpp"
#include "Server/Components/Timers/timers.hpp"

#include <Server/Components/TextDraws/textdraws.hpp>
#include <chrono>
#include <player.hpp>

namespace Core::TextDraws
{
using namespace std::string_literals;

enum class NotificationPosition
{
	Top,
	Left,
	Right
};

class Notification : public ITextDrawWrapper
{
	IPlayerTextDrawData* playerTextDrawData;
	IPlayer& player;
	ITimersComponent* timersComponent;

	IPlayerTextDraw* topLabel;
	std::optional<ITimer*> showTimer;

public:
	Notification(IPlayer& player, ITimersComponent* timersComponent);
	~Notification();

	void show() override;
	void hide() override;
	void destroy() override;

	void show(std::string text, NotificationPosition position,
		unsigned int notificationSound = 0, unsigned int seconds = 3);

	inline static const auto NAME = "NOTIFICATION"s;
};
}