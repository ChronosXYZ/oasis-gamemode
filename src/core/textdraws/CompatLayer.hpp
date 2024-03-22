#pragma once

#include "player.hpp"
#include "types.hpp"
#include <Server/Components/TextDraws/textdraws.hpp>
#include <magic_enum/magic_enum.hpp>
#include <string>

inline IPlayerTextDrawData* getTextDrawManager(IPlayer& player)
{
	return queryExtension<IPlayerTextDrawData>(player);
}

inline IPlayerTextDraw* CreatePlayerTextDraw(IPlayer& player, float x, float y, std::string text)
{
	return getTextDrawManager(player)->create(Vector2(x, y), text);
}

inline void PlayerTextDrawFont(IPlayer& player, IPlayerTextDraw* txd, int font)
{
	txd->setStyle(magic_enum::enum_cast<TextDrawStyle>(font).value());
}

inline void PlayerTextDrawLetterSize(IPlayer& player, IPlayerTextDraw* txd, float width, float height)
{
	txd->setLetterSize(Vector2(width, height));
}

inline void PlayerTextDrawAlignment(IPlayer& player, IPlayerTextDraw* txd, int alignment)
{
	txd->setAlignment(magic_enum::enum_cast<TextDrawAlignmentTypes>(alignment).value());
}

inline void PlayerTextDrawColor(IPlayer& player, IPlayerTextDraw* txd, int color)
{
	txd->setColour(Colour::FromRGBA(color));
}

inline void PlayerTextDrawSetShadow(IPlayer& player, IPlayerTextDraw* txd, int shadowSize)
{
	txd->setShadow(shadowSize);
}

inline void PlayerTextDrawSetOutline(IPlayer& player, IPlayerTextDraw* txd, int outlineSize)
{
	txd->setOutline(outlineSize);
}

inline void PlayerTextDrawBackgroundColor(IPlayer& player, IPlayerTextDraw* txd, int color)
{
	txd->setBackgroundColour(Colour::FromRGBA(color));
}

inline void PlayerTextDrawSetProportional(IPlayer& player, IPlayerTextDraw* txd, bool proportional)
{
	txd->setProportional(proportional);
}

inline void PlayerTextDrawUseBox(IPlayer& player, IPlayerTextDraw* txd, int use)
{
	txd->useBox(use);
}

inline void PlayerTextDrawBoxColor(IPlayer& player, IPlayerTextDraw* txd, int color)
{
	txd->setBoxColour(Colour::FromRGBA(color));
}

inline void PlayerTextDrawTextSize(IPlayer& player, IPlayerTextDraw* txd, float width, float height)
{
	txd->setTextSize(Vector2(width, height));
}