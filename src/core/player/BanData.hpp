#pragma once

#include <ctime>
#include <string>

namespace Core
{
struct Ban
{
	std::string reason;
	std::string by;
	std::time_t expiresAt;
};
}