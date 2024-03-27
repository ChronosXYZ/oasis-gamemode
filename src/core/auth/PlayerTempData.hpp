#pragma once

#include <optional>
#include <string>
namespace Core::Auth
{
struct PlayerTempData
{
	unsigned int loginAttempts;
	std::string plainTextPassword;
};
}