#pragma once

#include <player.hpp>

namespace Modes
{
struct IMode
{
	IMode() { }
	virtual ~IMode() { }

	virtual void onModeJoin(IPlayer& player) = 0;
	virtual void onModeLeave(IPlayer& player) = 0;
};
}