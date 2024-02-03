#pragma once

#include <string>
#include <vector>

namespace Core::Commands
{
struct CommandInfo
{
	std::vector<std::string> args;
	std::string description;
	std::string category;
};
}