#pragma once

#include <string>
#include <unordered_map>
namespace Utils
{
inline const static std::unordered_map<std::string, const std::string> COLORS {
	{ "WHITE", "FFFFFF" },
	{ "LIGHT_GRAY", "D3D3D3" },
	{ "DEEP_SAFFRON", "FF9933" },
	{ "RED", "FF0000" },
	{ "BRIGHT_BLUE", "0096FF" },
	{ "DRY_HIGHLIGHTER_GREEN", "33AA22" },
	{ "MILLION_GREY", "999999" }
};
}