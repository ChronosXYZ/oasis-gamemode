#pragma once

#include <string>

namespace Core
{
struct AdminData
{
	unsigned short level;
	std::string passwordHash;
};
}