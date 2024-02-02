#include "SQLQueryManager.hpp"

#include <cmrc/cmrc.hpp>

namespace Core
{
SQLQueryManager::SQLQueryManager()
{
	auto fs = cmrc::oasis::get_filesystem();
	for (const auto& entry : fs.iterate_directory(SQL_QUERIES_DIR))
	{
		auto queryFile = fs.open(SQL_QUERIES_DIR + entry.filename());
		auto queryText = std::string(queryFile.begin(), queryFile.end());
		auto queryName = entry.filename().substr(0, entry.filename().length() - 4); // remove .sql suffix
		this->_queries.insert(make_pair(queryName, queryText));
	}
}

std::optional<const std::string> SQLQueryManager::getQueryByName(const std::string& name)
{
	if (this->_queries.contains(std::string(name)))
	{
		return this->_queries[name];
	}
	return {};
}
}