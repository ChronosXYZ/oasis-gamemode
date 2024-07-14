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
	Derby,
	PTP,
	CnR,
	None
};
}