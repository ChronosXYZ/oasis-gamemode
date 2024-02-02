#pragma once

#include "utils/Singleton.hpp"

#include <cmrc/cmrc.hpp>

#include <optional>
#include <unordered_map>

CMRC_DECLARE(oasis);

namespace Core
{
class SQLQueryManager : public Singleton<SQLQueryManager>
{
	inline static const std::string SQL_QUERIES_DIR = "resources/sql/";

	std::unordered_map<std::string, const std::string> _queries;

public:
	SQLQueryManager();

	std::optional<const std::string> getQueryByName(const std::string& name);
};
}