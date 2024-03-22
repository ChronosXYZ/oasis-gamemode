#include "DeathmatchTimer.hpp"

#include "../../../core/textdraws/CompatLayer.hpp"
#include "Server/Components/TextDraws/textdraws.hpp"
#include "component.hpp"

#include <chrono>
#include <fmt/printf.h>
#include <player.hpp>

#include <algorithm>
#include <string>

namespace Modes::Deathmatch::TextDraws
{
DeathmatchTimer::DeathmatchTimer(IPlayer& player)
	: _txdManager(queryExtension<IPlayerTextDrawData>(player))
{
	dmmode_PTD[0] = CreatePlayerTextDraw(player, 561.5000, 413.5000, "_"); //"~w~Starboy (456) ~r~vs. ~w~Pukbot (321)");
	PlayerTextDrawFont(player, dmmode_PTD[0], 2);
	PlayerTextDrawLetterSize(player, dmmode_PTD[0], 0.1399, 0.9999);
	PlayerTextDrawAlignment(player, dmmode_PTD[0], 2);
	PlayerTextDrawColor(player, dmmode_PTD[0], 255);
	PlayerTextDrawSetShadow(player, dmmode_PTD[0], 0);
	PlayerTextDrawSetOutline(player, dmmode_PTD[0], 1);
	PlayerTextDrawBackgroundColor(player, dmmode_PTD[0], 0);
	PlayerTextDrawSetProportional(player, dmmode_PTD[0], true);
	PlayerTextDrawUseBox(player, dmmode_PTD[0], true);
	PlayerTextDrawBoxColor(player, dmmode_PTD[0], 855638186);
	PlayerTextDrawTextSize(player, dmmode_PTD[0], 637.0000, 150.5000);

	dmmode_PTD[1] = CreatePlayerTextDraw(player, 561.5000, 426.5000, "White bg");
	PlayerTextDrawFont(player, dmmode_PTD[1], 2);
	PlayerTextDrawLetterSize(player, dmmode_PTD[1], 0.1699, 0.4999);
	PlayerTextDrawAlignment(player, dmmode_PTD[1], 2);
	PlayerTextDrawColor(player, dmmode_PTD[1], 0);
	PlayerTextDrawSetShadow(player, dmmode_PTD[1], 0);
	PlayerTextDrawSetOutline(player, dmmode_PTD[1], -1);
	PlayerTextDrawBackgroundColor(player, dmmode_PTD[1], 0);
	PlayerTextDrawSetProportional(player, dmmode_PTD[1], true);
	PlayerTextDrawUseBox(player, dmmode_PTD[1], true);
	PlayerTextDrawBoxColor(player, dmmode_PTD[1], -137);
	PlayerTextDrawTextSize(player, dmmode_PTD[1], 637.0000, 150.5000);

	dmmode_PTD[2] = CreatePlayerTextDraw(player, 486.0000, 423.5000, "___Kills_____Deaths___Ratio____Damage____Time_");
	PlayerTextDrawFont(player, dmmode_PTD[2], 2);
	PlayerTextDrawLetterSize(player, dmmode_PTD[2], 0.1699, 1.0999);
	PlayerTextDrawColor(player, dmmode_PTD[2], 255);
	PlayerTextDrawSetShadow(player, dmmode_PTD[2], 0);
	PlayerTextDrawSetOutline(player, dmmode_PTD[2], -1);
	PlayerTextDrawBackgroundColor(player, dmmode_PTD[2], 0);
	PlayerTextDrawSetProportional(player, dmmode_PTD[2], true);
	PlayerTextDrawTextSize(player, dmmode_PTD[2], 637.0000, 720.5000);

	dmmode_PTD[3] = CreatePlayerTextDraw(player, 561.5000, 435.5000, "_");
	PlayerTextDrawFont(player, dmmode_PTD[3], 2);
	PlayerTextDrawLetterSize(player, dmmode_PTD[3], 0.2199, 0.8999);
	PlayerTextDrawAlignment(player, dmmode_PTD[3], 2);
	PlayerTextDrawColor(player, dmmode_PTD[3], -1);
	PlayerTextDrawSetShadow(player, dmmode_PTD[3], 0);
	PlayerTextDrawSetOutline(player, dmmode_PTD[3], -1);
	PlayerTextDrawBackgroundColor(player, dmmode_PTD[3], 187);
	PlayerTextDrawSetProportional(player, dmmode_PTD[3], true);
	PlayerTextDrawUseBox(player, dmmode_PTD[3], true);
	PlayerTextDrawBoxColor(player, dmmode_PTD[3], 102);
	PlayerTextDrawTextSize(player, dmmode_PTD[3], 0.0000, 150.5000);

	dmmode_PTD[4] = CreatePlayerTextDraw(player, 561.5000, 441.5000, "_");
	PlayerTextDrawFont(player, dmmode_PTD[4], 2);
	PlayerTextDrawLetterSize(player, dmmode_PTD[4], 0.2199, 0.1999);
	PlayerTextDrawAlignment(player, dmmode_PTD[4], 2);
	PlayerTextDrawColor(player, dmmode_PTD[4], -1);
	PlayerTextDrawSetShadow(player, dmmode_PTD[4], 0);
	PlayerTextDrawSetOutline(player, dmmode_PTD[4], -1);
	PlayerTextDrawBackgroundColor(player, dmmode_PTD[4], 187);
	PlayerTextDrawSetProportional(player, dmmode_PTD[4], true);
	PlayerTextDrawUseBox(player, dmmode_PTD[4], true);
	PlayerTextDrawBoxColor(player, dmmode_PTD[4], 153);
	PlayerTextDrawTextSize(player, dmmode_PTD[4], 0.0000, 150.5000);

	dmmode_PTD[5] = CreatePlayerTextDraw(player, 487.5000, 436.5000, "strip L");
	PlayerTextDrawFont(player, dmmode_PTD[5], 2);
	PlayerTextDrawLetterSize(player, dmmode_PTD[5], 0.2199, 0.2999);
	PlayerTextDrawAlignment(player, dmmode_PTD[5], 2);
	PlayerTextDrawColor(player, dmmode_PTD[5], 0);
	PlayerTextDrawSetShadow(player, dmmode_PTD[5], 0);
	PlayerTextDrawSetOutline(player, dmmode_PTD[5], 0);
	PlayerTextDrawBackgroundColor(player, dmmode_PTD[5], 187);
	PlayerTextDrawSetProportional(player, dmmode_PTD[5], true);
	PlayerTextDrawUseBox(player, dmmode_PTD[5], true);
	PlayerTextDrawBoxColor(player, dmmode_PTD[5], -872415079);
	PlayerTextDrawTextSize(player, dmmode_PTD[5], -40.0000, 0.5000);

	dmmode_PTD[6] = CreatePlayerTextDraw(player, 516.5000, 436.5000, "strip L1");
	PlayerTextDrawFont(player, dmmode_PTD[6], 2);
	PlayerTextDrawLetterSize(player, dmmode_PTD[6], 0.2199, 0.2999);
	PlayerTextDrawAlignment(player, dmmode_PTD[6], 2);
	PlayerTextDrawColor(player, dmmode_PTD[6], 0);
	PlayerTextDrawSetShadow(player, dmmode_PTD[6], 0);
	PlayerTextDrawSetOutline(player, dmmode_PTD[6], 0);
	PlayerTextDrawBackgroundColor(player, dmmode_PTD[6], 187);
	PlayerTextDrawSetProportional(player, dmmode_PTD[6], true);
	PlayerTextDrawUseBox(player, dmmode_PTD[6], true);
	PlayerTextDrawBoxColor(player, dmmode_PTD[6], -1145324647);
	PlayerTextDrawTextSize(player, dmmode_PTD[6], -40.0000, 0.5000);

	dmmode_PTD[7] = CreatePlayerTextDraw(player, 546.5000, 436.5000, "strip L2");
	PlayerTextDrawFont(player, dmmode_PTD[7], 2);
	PlayerTextDrawLetterSize(player, dmmode_PTD[7], 0.2199, 0.2999);
	PlayerTextDrawAlignment(player, dmmode_PTD[7], 2);
	PlayerTextDrawColor(player, dmmode_PTD[7], 0);
	PlayerTextDrawSetShadow(player, dmmode_PTD[7], 0);
	PlayerTextDrawSetOutline(player, dmmode_PTD[7], 0);
	PlayerTextDrawBackgroundColor(player, dmmode_PTD[7], 187);
	PlayerTextDrawSetProportional(player, dmmode_PTD[7], true);
	PlayerTextDrawUseBox(player, dmmode_PTD[7], true);
	PlayerTextDrawBoxColor(player, dmmode_PTD[7], -1145324647);
	PlayerTextDrawTextSize(player, dmmode_PTD[7], -40.0000, 0.5000);

	dmmode_PTD[8] = CreatePlayerTextDraw(player, 576.5000, 436.5000, "strip R2");
	PlayerTextDrawFont(player, dmmode_PTD[8], 2);
	PlayerTextDrawLetterSize(player, dmmode_PTD[8], 0.2199, 0.2999);
	PlayerTextDrawAlignment(player, dmmode_PTD[8], 2);
	PlayerTextDrawColor(player, dmmode_PTD[8], 0);
	PlayerTextDrawSetShadow(player, dmmode_PTD[8], 0);
	PlayerTextDrawSetOutline(player, dmmode_PTD[8], 0);
	PlayerTextDrawBackgroundColor(player, dmmode_PTD[8], 187);
	PlayerTextDrawSetProportional(player, dmmode_PTD[8], true);
	PlayerTextDrawUseBox(player, dmmode_PTD[8], true);
	PlayerTextDrawBoxColor(player, dmmode_PTD[8], -1145324647);
	PlayerTextDrawTextSize(player, dmmode_PTD[8], -40.0000, 0.5000);

	dmmode_PTD[9] = CreatePlayerTextDraw(player, 606.5000, 436.5000, "strip R1");
	PlayerTextDrawFont(player, dmmode_PTD[9], 2);
	PlayerTextDrawLetterSize(player, dmmode_PTD[9], 0.2199, 0.2999);
	PlayerTextDrawAlignment(player, dmmode_PTD[9], 2);
	PlayerTextDrawColor(player, dmmode_PTD[9], 0);
	PlayerTextDrawSetShadow(player, dmmode_PTD[9], 0);
	PlayerTextDrawSetOutline(player, dmmode_PTD[9], 0);
	PlayerTextDrawBackgroundColor(player, dmmode_PTD[9], 187);
	PlayerTextDrawSetProportional(player, dmmode_PTD[9], true);
	PlayerTextDrawUseBox(player, dmmode_PTD[9], true);
	PlayerTextDrawBoxColor(player, dmmode_PTD[9], -1145324647);
	PlayerTextDrawTextSize(player, dmmode_PTD[9], -40.0000, 0.5000);

	dmmode_PTD[10] = CreatePlayerTextDraw(player, 635.5000, 436.5000, "strip R");
	PlayerTextDrawFont(player, dmmode_PTD[10], 2);
	PlayerTextDrawLetterSize(player, dmmode_PTD[10], 0.2199, 0.2999);
	PlayerTextDrawAlignment(player, dmmode_PTD[10], 2);
	PlayerTextDrawColor(player, dmmode_PTD[10], 0);
	PlayerTextDrawSetShadow(player, dmmode_PTD[10], 0);
	PlayerTextDrawSetOutline(player, dmmode_PTD[10], 0);
	PlayerTextDrawBackgroundColor(player, dmmode_PTD[10], 187);
	PlayerTextDrawSetProportional(player, dmmode_PTD[10], true);
	PlayerTextDrawUseBox(player, dmmode_PTD[10], true);
	PlayerTextDrawBoxColor(player, dmmode_PTD[10], -1157627751);
	PlayerTextDrawTextSize(player, dmmode_PTD[10], -40.0000, 0.5000);

	dmmode_PTD[11] = CreatePlayerTextDraw(player, 501.5000, 433.5000, "_"); //"50000");
	PlayerTextDrawFont(player, dmmode_PTD[11], 2);
	PlayerTextDrawLetterSize(player, dmmode_PTD[11], 0.1599, 1.1000);
	PlayerTextDrawAlignment(player, dmmode_PTD[11], 2);
	PlayerTextDrawColor(player, dmmode_PTD[11], -35);
	PlayerTextDrawSetShadow(player, dmmode_PTD[11], 0);
	PlayerTextDrawSetOutline(player, dmmode_PTD[11], 1);
	PlayerTextDrawBackgroundColor(player, dmmode_PTD[11], 0);
	PlayerTextDrawSetProportional(player, dmmode_PTD[11], true);
	PlayerTextDrawTextSize(player, dmmode_PTD[11], 0.0000, 0.0000);

	dmmode_PTD[12] = CreatePlayerTextDraw(player, 531.5000, 433.5000, "_"); //"50000");
	PlayerTextDrawFont(player, dmmode_PTD[12], 2);
	PlayerTextDrawLetterSize(player, dmmode_PTD[12], 0.1599, 1.1000);
	PlayerTextDrawAlignment(player, dmmode_PTD[12], 2);
	PlayerTextDrawColor(player, dmmode_PTD[12], -35);
	PlayerTextDrawSetShadow(player, dmmode_PTD[12], 0);
	PlayerTextDrawSetOutline(player, dmmode_PTD[12], 1);
	PlayerTextDrawBackgroundColor(player, dmmode_PTD[12], 0);
	PlayerTextDrawSetProportional(player, dmmode_PTD[12], true);
	PlayerTextDrawTextSize(player, dmmode_PTD[12], 0.0000, 0.0000);

	dmmode_PTD[13] = CreatePlayerTextDraw(player, 561.5000, 433.5000, "_"); //"50000");
	PlayerTextDrawFont(player, dmmode_PTD[13], 2);
	PlayerTextDrawLetterSize(player, dmmode_PTD[13], 0.1599, 1.1000);
	PlayerTextDrawAlignment(player, dmmode_PTD[13], 2);
	PlayerTextDrawColor(player, dmmode_PTD[13], -35);
	PlayerTextDrawSetShadow(player, dmmode_PTD[13], 0);
	PlayerTextDrawSetOutline(player, dmmode_PTD[13], 1);
	PlayerTextDrawBackgroundColor(player, dmmode_PTD[13], 0);
	PlayerTextDrawSetProportional(player, dmmode_PTD[13], true);
	PlayerTextDrawTextSize(player, dmmode_PTD[13], 0.0000, 0.0000);

	dmmode_PTD[14] = CreatePlayerTextDraw(player, 591.5000, 433.5000, "_"); //"50000");
	PlayerTextDrawFont(player, dmmode_PTD[14], 2);
	PlayerTextDrawLetterSize(player, dmmode_PTD[14], 0.1599, 1.1000);
	PlayerTextDrawAlignment(player, dmmode_PTD[14], 2);
	PlayerTextDrawColor(player, dmmode_PTD[14], -35);
	PlayerTextDrawSetShadow(player, dmmode_PTD[14], 0);
	PlayerTextDrawSetOutline(player, dmmode_PTD[14], 1);
	PlayerTextDrawBackgroundColor(player, dmmode_PTD[14], 0);
	PlayerTextDrawSetProportional(player, dmmode_PTD[14], true);
	PlayerTextDrawTextSize(player, dmmode_PTD[14], 0.0000, 0.0000);

	dmmode_PTD[15] = CreatePlayerTextDraw(player, 621.5000, 433.5000, "_"); //"50000");
	PlayerTextDrawFont(player, dmmode_PTD[15], 2);
	PlayerTextDrawLetterSize(player, dmmode_PTD[15], 0.1599, 1.1000);
	PlayerTextDrawAlignment(player, dmmode_PTD[15], 2);
	PlayerTextDrawColor(player, dmmode_PTD[15], -35);
	PlayerTextDrawSetShadow(player, dmmode_PTD[15], 0);
	PlayerTextDrawSetOutline(player, dmmode_PTD[15], 1);
	PlayerTextDrawBackgroundColor(player, dmmode_PTD[15], 0);
	PlayerTextDrawSetProportional(player, dmmode_PTD[15], true);
	PlayerTextDrawTextSize(player, dmmode_PTD[15], 0.0000, 0.0000);
}

void DeathmatchTimer::show()
{
	std::for_each(dmmode_PTD.cbegin(), dmmode_PTD.cend(), [](IPlayerTextDraw* x)
		{
			x->show();
		});
}

void DeathmatchTimer::hide()
{
	std::for_each(dmmode_PTD.cbegin(), dmmode_PTD.cend(), [](IPlayerTextDraw* x)
		{
			x->hide();
		});
}

void DeathmatchTimer::destroy()
{
	std::for_each(dmmode_PTD.cbegin(), dmmode_PTD.cend(), [&](IPlayerTextDraw* x)
		{
			this->_txdManager->release(x->getID());
		});
}

void DeathmatchTimer::update(std::string header, int kills, int deaths, float damage, int seconds)
{
	dmmode_PTD[0]->setText(header);
	dmmode_PTD[11]->setText(std::to_string(kills));
	dmmode_PTD[12]->setText(std::to_string(deaths));

	float ratio = float(kills);
	if (deaths != 0)
	{
		ratio = float(kills) / float(deaths);
	}
	dmmode_PTD[13]->setText(fmt::sprintf("%.1f", ratio));

	std::string damageText = fmt::sprintf("%.1f", damage);
	if (damage > 1000.0)
	{
		damageText = fmt::sprintf("%.2fk", damage);
	}
	dmmode_PTD[14]->setText(damageText);

	std::chrono::hh_mm_ss time { std::chrono::seconds(seconds) };
	dmmode_PTD[15]->setText(fmt::sprintf("%02d:%02d", time.minutes().count(), time.seconds().count()));

	this->show();
}
}