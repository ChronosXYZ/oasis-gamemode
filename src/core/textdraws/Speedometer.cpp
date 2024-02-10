#include "Speedometer.hpp"

#include <Server/Components/TextDraws/textdraws.hpp>
#include <fmt/printf.h>
#include <math.h>
#include <player.hpp>
#include <component.hpp>

namespace Core::TextDraws
{

Speedometer::Speedometer(IPlayer& player)
	: _playerTextDrawData(queryExtension<IPlayerTextDrawData>(player))
{

	_speedbar = _playerTextDrawData->create(Vector2(140.0000, 375.5000), "IIIIIIIIIIIIIIIIIIIIIIIIIIIIII");
	_speedbar->setStyle(TextDrawStyle_1);
	_speedbar->setLetterSize(Vector2(0.2700, 1.2000));
	_speedbar->setColour(Colour::FromRGBA(-154));
	_speedbar->setShadow(0);
	_speedbar->setOutline(0);
	_speedbar->setBackgroundColour(Colour::FromRGBA(51));
	_speedbar->setProportional(true);
	_speedbar->setTextSize(Vector2(0.0, 0.0));

	_currentSpeedBar = _playerTextDrawData->create(Vector2(140.0000, 375.5000), "_");
	_currentSpeedBar->setStyle(TextDrawStyle_1);
	_currentSpeedBar->setLetterSize(Vector2(0.2700, 1.2000));
	_currentSpeedBar->setColour(Colour::FromRGBA(0xFF0000FF));
	_currentSpeedBar->setShadow(0);
	_currentSpeedBar->setOutline(0);
	_currentSpeedBar->setBackgroundColour(Colour::FromRGBA(51));
	_currentSpeedBar->setProportional(true);
	_currentSpeedBar->setTextSize(Vector2(0.0, 0.0));

	_speedLabel = _playerTextDrawData->create(Vector2(145.0000, 359.0000), "_");
	_speedLabel->setStyle(TextDrawStyle_2);
	_speedLabel->setLetterSize(Vector2(0.1999, 1.0000));
	_speedLabel->setColour(Colour::FromRGBA(855638271));
	_speedLabel->setShadow(0);
	_speedLabel->setOutline(1);
	_speedLabel->setBackgroundColour(Colour::FromRGBA(-239));
	_speedLabel->setProportional(true);
	_speedLabel->setTextSize(Vector2(290.0, 0.0));
}

void Speedometer::update(float speedFloat)
{
	std::string speedBar;

	unsigned int speed = roundf(speedFloat);

	if (speed > 0.0 && speed <= 10.0)
		speedBar = "I";
	else if (speed > 10 && speed <= 20)
		speedBar = "I";
	else if (speed > 20 && speed <= 30)
		speedBar = "II";
	else if (speed > 30 && speed <= 40)
		speedBar = "III";
	else if (speed > 40 && speed <= 50)
		speedBar = "IIII";
	else if (speed > 50 && speed <= 60)
		speedBar = "IIIII";
	else if (speed > 60 && speed <= 70)
		speedBar = "IIIIII";
	else if (speed > 70 && speed <= 80)
		speedBar = "IIIIIII";
	else if (speed > 80 && speed <= 85)
		speedBar = "IIIIIIII";
	else if (speed > 85 && speed <= 90)
		speedBar = "IIIIIIIII";
	else if (speed > 90 && speed <= 95)
		speedBar = "IIIIIIIIII";
	else if (speed > 95 && speed <= 100)
		speedBar = "IIIIIIIIIII";
	else if (speed > 100 && speed <= 110)
		speedBar = "IIIIIIIIIIII";
	else if (speed > 110 && speed <= 120)
		speedBar = "IIIIIIIIIIIII";
	else if (speed > 120 && speed <= 130)
		speedBar = "IIIIIIIIIIIIII";
	else if (speed > 130 && speed <= 140)
		speedBar = "IIIIIIIIIIIIIII";
	else if (speed > 140 && speed <= 150)
		speedBar = "IIIIIIIIIIIIIIII";
	else if (speed > 150 && speed <= 155)
		speedBar = "IIIIIIIIIIIIIIIII";
	else if (speed > 155 && speed <= 160)
		speedBar = "IIIIIIIIIIIIIIIIII";
	else if (speed > 160 && speed <= 165)
		speedBar = "IIIIIIIIIIIIIIIIIII";
	else if (speed > 165 && speed <= 170)
		speedBar = "IIIIIIIIIIIIIIIIIIII";
	else if (speed > 170 && speed <= 175)
		speedBar = "IIIIIIIIIIIIIIIIIIIII";
	else if (speed > 175 && speed <= 180)
		speedBar = "IIIIIIIIIIIIIIIIIIIIII";
	else if (speed > 180 && speed <= 185)
		speedBar = "IIIIIIIIIIIIIIIIIIIIIII";
	else if (speed > 185 && speed <= 190)
		speedBar = "IIIIIIIIIIIIIIIIIIIIIIII";
	else if (speed > 190 && speed <= 195)
		speedBar = "IIIIIIIIIIIIIIIIIIIIIIIII";
	else if (speed > 195 && speed <= 200)
		speedBar = "IIIIIIIIIIIIIIIIIIIIIIIIII";
	else if (speed > 200 && speed <= 210)
		speedBar = "IIIIIIIIIIIIIIIIIIIIIIIIIII";
	else if (speed > 210 && speed <= 220)
		speedBar = "IIIIIIIIIIIIIIIIIIIIIIIIIIII";
	else if (speed > 220 && speed <= 230)
		speedBar = "IIIIIIIIIIIIIIIIIIIIIIIIIIIII";
	else if (speed > 230)
		speedBar = "IIIIIIIIIIIIIIIIIIIIIIIIIIIIII";

	unsigned int speedBarColor;
	if (speed <= 80)
		speedBarColor
			= 0x00FF00FF;
	else if (speed > 80 && speed <= 145)
		speedBarColor
			= 0xFFFF00FF;
	else if (speed > 145)
		speedBarColor
			= 0xFF0000FF;
	_currentSpeedBar->setColour(Colour::FromRGBA(speedBarColor));
	_currentSpeedBar->setText(speedBar);
	_currentSpeedBar->show();

	auto speedLabelText = fmt::sprintf("_~n~~r~%d ~w~KM/H", speed);
	_speedLabel->setText(speedLabelText);
	_speedLabel->show();
}

void Speedometer::show()
{
	_speedbar->show();
	_currentSpeedBar->show();
	_speedLabel->show();
}

void Speedometer::hide()
{
	_speedbar->hide();
	_currentSpeedBar->hide();
	_speedLabel->hide();
}

void Speedometer::destroy()
{
	_playerTextDrawData->release(_speedbar->getID());
	_playerTextDrawData->release(_currentSpeedBar->getID());
	_playerTextDrawData->release(_speedLabel->getID());
}

}