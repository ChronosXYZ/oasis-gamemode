#include "Notification.hpp"

#include "CompatLayer.hpp"

#include <Server/Components/Timers/timers.hpp>
#include <Server/Components/Timers/Impl/timers_impl.hpp>

namespace Core::TextDraws
{
Notification::Notification(IPlayer& player, ITimersComponent* timersComponent)
	: playerTextDrawData(queryExtension<IPlayerTextDrawData>(player))
	, timersComponent(timersComponent)
	, player(player)
{
	topLabel = CreatePlayerTextDraw(
		player, 319.0000, 16.0000, "_"); //"Starboy~n~~w~1");
	PlayerTextDrawFont(player, topLabel, 2);
	PlayerTextDrawLetterSize(player, topLabel, 0.3199, 1.6000);
	PlayerTextDrawAlignment(player, topLabel, 2);
	PlayerTextDrawColor(player, topLabel, -301989633);
	PlayerTextDrawSetShadow(player, topLabel, 0);
	PlayerTextDrawSetOutline(player, topLabel, 1);
	PlayerTextDrawBackgroundColor(player, topLabel, 255);
	PlayerTextDrawSetProportional(player, topLabel, true);
	PlayerTextDrawTextSize(player, topLabel, 0.0000, 0.0000);
}

Notification::~Notification()
{
	if (this->showTimer)
	{
		this->showTimer.value()->kill();
		this->showTimer.reset();
	}
}

void Notification::show(std::string text, NotificationPosition position,
	unsigned int notificationSound, unsigned int seconds)
{
	if (this->showTimer)
		this->showTimer.value()->kill();

	switch (position)
	{
	case NotificationPosition::Top:
	case NotificationPosition::Left:
	case NotificationPosition::Right:
	{
		topLabel->setText(text);
		topLabel->show();
		break;
	}
	}

	if (notificationSound != 0)
		player.playSound(notificationSound, Vector3(0.0, 0.0, 0.0));

	this->showTimer
		= this->timersComponent->create(new Impl::SimpleTimerHandler(
											[this]()
											{
												this->hide();
												this->showTimer.reset();
											}),
			Seconds(seconds), false);
}

void Notification::show() { }

void Notification::hide() { topLabel->hide(); }

void Notification::destroy()
{
	this->playerTextDrawData->release(this->topLabel->getID());
}

}