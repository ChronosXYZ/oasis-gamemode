#pragma once

#include <date/date.h>
#include <memory>
#include <optional>
#include <string>
#include <pqxx/pqxx>
#include <variant>

#include "AdminData.hpp"
#include "BanData.hpp"
#include "../utils/PgTimestamp.hpp"
#include "../../constants.hpp"

using namespace std;

namespace Core
{
typedef std::variant<int, float, std::string, bool> PrimitiveType;
struct PlayerModel
{
	unsigned long userId;
	string name;
	string passwordHash;
	string language = consts::LANGUAGE_CODE_NAME.at(0);
	string email;
	string lastIP;
	unsigned short lastSkinId;
	Utils::SQL::timestamp lastLoginAt;
	Utils::SQL::timestamp registrationDate;

	unique_ptr<Ban> ban;
	unique_ptr<AdminData> adminData;
	std::unordered_map<std::string, PrimitiveType> tempData;

	PlayerModel() = default;

	void updateFromRow(const pqxx::row& row)
	{
		userId = row["id"].as<unsigned long>();
		name = row["name"].c_str();
		passwordHash = row["password_hash"].c_str();
		language = row["language"].c_str();
		email = row["email"].c_str();
		lastIP = row["last_ip"].c_str();
		lastSkinId = row["last_skin_id"].as<unsigned short>();
		lastLoginAt = row["last_login_at"].as<Utils::SQL::timestamp>();
		registrationDate = row["registration_date"].as<Utils::SQL::timestamp>();

		if (!row["ban_expires_at"].is_null())
		{
			ban = make_unique<Ban>();
			ban->expiresAt = row["ban_expires_at"].as<std::time_t>();
			ban->by = row["banned_by"].as<string>();
			ban->reason = row["ban_reason"].as<string>();
		}

		adminData = make_unique<AdminData>();
		if (!row["admin_level"].is_null())
			adminData->level = row["admin_level"].as<unsigned short>();
		else
			adminData->level = 0;
		if (!row["admin_pass_hash"].is_null())
			adminData->passwordHash = row["admin_pass_hash"].as<string>();
	}

	void setTempData(const std::string& key, const PrimitiveType& value)
	{
		tempData[key] = value;
	}

	void deleteTempData(const std::string& key)
	{
		if (tempData.contains(key))
		{
			tempData.erase(key);
		}
	}

	std::optional<const PrimitiveType> getTempData(const std::string& key)
	{
		if (tempData.contains(key))
		{
			return tempData.at(key);
		}
		return {};
	}
};

}