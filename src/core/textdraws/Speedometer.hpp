#pragma once

#include "ITextDrawWrapper.hpp"

#include <player.hpp>
#include <Server/Components/TextDraws/textdraws.hpp>

namespace Core::TextDraws
{
using namespace std::string_literals;
class Speedometer : public ITextDrawWrapper
{
	IPlayerTextDrawData* _playerTextDrawData;

	IPlayerTextDraw* _speedbar;
	IPlayerTextDraw* _currentSpeedBar;
	IPlayerTextDraw* _speedLabel;

public:
	Speedometer(IPlayer& player);

	void update(float speed);

	void show() override;
	void hide() override;
	void destroy() override;

	inline static const auto NAME = "SPEEDOMETER"s;
};
}