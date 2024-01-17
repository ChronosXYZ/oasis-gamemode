#pragma once

#include <string>

using namespace std;

namespace Core
{

struct PlayerData
{
	// Basic stuff
	unsigned long account_id;
	string name;
	string passwordHash;
	string language;
	string email;
	string lastIP;
	uint lastSkinId;

	bool banned;
	string banReason;
	string banBy;
	unsigned long banExpiresAt;

	// Admin stuff
	unsigned int adminLevel;
	string adminPassHash;
};

}