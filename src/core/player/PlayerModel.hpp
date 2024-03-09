#pragma once

#include "AdminData.hpp"
#include "BanData.hpp"
#include "../utils/PgTimestamp.hpp"
#include "../utils/Localization.hpp"

#include <Server/Components/Timers/timers.hpp>

#include <cstddef>
#include <ctime>
#include <date/date.h>
#include <memory>
#include <optional>
#include <string>
#include <pqxx/pqxx>
#include <string_view>
#include <variant>

namespace Core
{
typedef std::variant<int, float, std::string, bool, std::size_t, std::time_t> PrimitiveType;
struct PlayerModel
{
	unsigned long userId;
	std::string name;
	std::string passwordHash;
	std::string language = Localization::LANGUAGE_CODE_NAMES.at(0);
	std::string email;
	std::string lastIP;
	unsigned short lastSkinId;
	Utils::SQL::timestamp lastLoginAt;
	Utils::SQL::timestamp registrationDate;

	std::unique_ptr<Ban> ban;
	std::unique_ptr<AdminData> adminData;
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
			ban = std::make_unique<Ban>();
			ban->expiresAt = row["ban_expires_at"].as<std::time_t>();
			ban->by = row["banned_by"].as<std::string>();
			ban->reason = row["ban_reason"].as<std::string>();
		}

		adminData = std::make_unique<AdminData>();
		if (!row["admin_level"].is_null())
			adminData->level = row["admin_level"].as<unsigned short>();
		else
			adminData->level = 0;
		if (!row["admin_pass_hash"].is_null())
			adminData->passwordHash = row["admin_pass_hash"].as<std::string>();
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

	template <typename T>
	std::optional<const T> getTempData(const std::string& key)
	{
		if (tempData.contains(key))
		{
			auto data = tempData.at(key);
			return std::get<T>(data);
		}
		return {};
	}
};
}