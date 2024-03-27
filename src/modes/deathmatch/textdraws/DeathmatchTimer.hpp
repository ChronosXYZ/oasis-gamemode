#pragma once

#include "../../../core/textdraws/ITextDrawWrapper.hpp"

#include <Server/Components/TextDraws/textdraws.hpp>
#include <player.hpp>

namespace Modes::Deathmatch::TextDraws
{
using namespace std::string_literals;
class DeathmatchTimer : public Core::TextDraws::ITextDrawWrapper
{
	IPlayerTextDrawData* _txdManager;
	std::array<IPlayerTextDraw*, 16> dmmode_PTD;

public:
	DeathmatchTimer(IPlayer& player);

	void show() override;
	void hide() override;
	void destroy() override;

	void update(std::string header, int kills, int deaths, float damage, std::chrono::seconds countdown);

	inline const static auto NAME = "DeathmatchTimer"s;
};
}