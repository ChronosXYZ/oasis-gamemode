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
	topNotification = CreatePlayerTextDraw(
		player, 319.0000, 16.0000, "_"); //"Starboy~n~~w~1");
	PlayerTextDrawFont(player, topNotification, 2);
	PlayerTextDrawLetterSize(player, topNotification, 0.3199, 1.6000);
	PlayerTextDrawAlignment(player, topNotification, 2);
	PlayerTextDrawColor(player, topNotification, -301989633);
	PlayerTextDrawSetShadow(player, topNotification, 0);
	PlayerTextDrawSetOutline(player, topNotification, 1);
	PlayerTextDrawBackgroundColor(player, topNotification, 255);
	PlayerTextDrawSetProportional(player, topNotification, true);
	PlayerTextDrawTextSize(player, topNotification, 0.0000, 0.0000);

	bottomNotification
		= CreatePlayerTextDraw(player, 320.000000, 385.000000, "_");

	PlayerTextDrawFont(player, bottomNotification, 1);
	PlayerTextDrawLetterSize(player, bottomNotification, 0.487500, 1.950000);
	PlayerTextDrawTextSize(player, bottomNotification, 401.000000, 341.500000);
	PlayerTextDrawSetOutline(player, bottomNotification, 1);
	PlayerTextDrawSetShadow(player, bottomNotification, 0);
	PlayerTextDrawAlignment(player, bottomNotification, 2);
	PlayerTextDrawColor(player, bottomNotification, -1);
	PlayerTextDrawBackgroundColor(player, bottomNotification, 255);
	PlayerTextDrawBoxColor(player, bottomNotification, 50);
	PlayerTextDrawUseBox(player, bottomNotification, 0);
	PlayerTextDrawSetProportional(player, bottomNotification, 1);

	this->showTimers = {
		{ NotificationPosition::Top, {} },
		{ NotificationPosition::Bottom, {} },
		{ NotificationPosition::Left, {} },
		{ NotificationPosition::Right, {} },
	};
}

Notification::~Notification()
{
	for (auto [position, timer] : this->showTimers)
	{
		if (timer)
		{
			timer.value()->kill();
			timer.reset();
		}
	}
}

void Notification::show(std::string text, NotificationPosition position,
	unsigned int notificationSound, unsigned int seconds)
{
	if (this->showTimers.at(position))
		this->showTimers.at(position).value()->kill();

	switch (position)
	{
	case NotificationPosition::Top:
	case NotificationPosition::Left:
	case NotificationPosition::Right:
	{
		topNotification->setText(text);
		topNotification->show();
		break;
	}
	case NotificationPosition::Bottom:
	{
		this->bottomNotification->setText(text);
		this->bottomNotification->show();
		break;
	}
	}

	if (notificationSound != 0)
		player.playSound(notificationSound, Vector3(0.0, 0.0, 0.0));

	this->showTimers[position] = this->timersComponent->create(
		new Impl::SimpleTimerHandler(
			[this, position]()
			{
				this->hide(position);
				this->showTimers[position].reset();
			}),
		Seconds(seconds), false);
}

void Notification::hide(NotificationPosition position)
{
	switch (position)
	{
	case NotificationPosition::Top:
	{
		this->topNotification->hide();
		break;
	}
	case NotificationPosition::Left:
	case NotificationPosition::Right:
	case NotificationPosition::Bottom:
	{
		this->bottomNotification->hide();
		break;
	}
	}
}

void Notification::show() { }

void Notification::hide() { this->topNotification->hide(); }

void Notification::destroy()
{
	this->playerTextDrawData->release(this->topNotification->getID());
	this->playerTextDrawData->release(this->bottomNotification->getID());
}
}