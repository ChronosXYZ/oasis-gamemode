#pragma once

#include <memory>
#include <optional>
#include <string>
#include <pqxx/pqxx>

#include "AdminData.hpp"
#include "BanData.hpp"

using namespace std;

namespace Core
{

struct PlayerModel
{
	unsigned long account_id;
	string name;
	string passwordHash;
	string language;
	string email;
	string lastIP;
	unsigned short lastSkinId;

	unique_ptr<Ban> ban;
	unique_ptr<AdminData> adminData;

	PlayerModel() = default;

	PlayerModel(const pqxx::row& row)
	{
		account_id = row["id"].as<unsigned long>();
		name = row["name"].c_str();
		passwordHash = row["password_hash"].c_str();
		language = row["language"].c_str();
		email = row["email"].c_str();
		lastIP = row["last_ip"].c_str();
		lastSkinId = row["last_skin_id"].as<unsigned short>();

		if (!row["ban_expires_at"].is_null())
		{
			ban = make_unique<Ban>();
			ban->expiresAt = row["ban_expires_at"].as<std::time_t>();
			ban->by = row["banned_by"].as<string>();
			ban->reason = row["ban_reason"].as<string>();
		}

		adminData = make_unique<AdminData>();
		adminData->level = row["admin_level"].as<unsigned short>();
		if (!row["admin_pass_hash"].is_null())
			adminData->passwordHash = row["admin_pass_hash"].as<string>();
	}
};

}