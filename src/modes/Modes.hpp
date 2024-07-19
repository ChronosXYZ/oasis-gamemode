#pragma once

#include <ctime>
#include <string>
#include <unordered_map>
#include <variant>

namespace Modes
{
typedef std::variant<int, float, std::string, bool, std::time_t, unsigned int>
	PrimitiveType;
typedef std::unordered_map<std::string, PrimitiveType> JoinData;

enum class Mode
{
	Freeroam,
	Deathmatch,
	X1,
	Duel,
	Derby,
	PTP,
	CnR,
	None
};

inline const std::string getModeShortName(Mode mode)
{
	switch (mode)
	{
	case Mode::Freeroam:
		return "FR";
	case Mode::Deathmatch:
		return "DM";
	case Mode::X1:
		return "X1";
	case Mode::Derby:
		return "DB";
	case Mode::PTP:
		return "PTP";
	case Mode::CnR:
		return "CNR";
	case Mode::None:
		return "";
	case Mode::Duel:
		return "DUEL";
	}
	return "";
};

inline const std::string getModeColor(Mode mode)
{
	switch (mode)
	{
	case Mode::Freeroam:
		return "A9C4E4";
	case Mode::Deathmatch:
		return "FF9933";
	case Mode::X1:
		return "FF0000";
	case Mode::Derby:
		return "FFFFFF";
	case Mode::PTP:
		return "FFFFFF";
	case Mode::CnR:
		return "FFFFFF";
	case Mode::None:
		return "FFFFFF";
	case Mode::Duel:
		return "FFFFFF";
	}
	return "FFFFFF";
}
}