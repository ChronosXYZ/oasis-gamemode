#pragma once

#include "ITextDrawWrapper.hpp"
#include "Server/Components/TextDraws/textdraws.hpp"
#include "player.hpp"

#include <component.hpp>

#include <string>

namespace Core::TextDraws
{
using namespace std::string_literals;

class ServerLogo : public ITextDrawWrapper
{
	inline static const auto NAME = "SERVER_LOGO"s;

	IPlayerTextDrawData* _playerTextDrawData;

	IPlayerTextDraw* _topLabel;
	IPlayerTextDraw* _bottomLabel;
	IPlayerTextDraw* _box;
	IPlayerTextDraw* _website;

public:
	ServerLogo(IPlayer& player, const std::string& name, const std::string& bottomLabel, const std::string& website);

	void show() override;
	void hide() override;
	void destroy() override;
	const std::string& name() override;
};
}