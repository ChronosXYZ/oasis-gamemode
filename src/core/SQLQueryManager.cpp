#include "SQLQueryManager.hpp"
#include <utility>

using namespace Core;
using namespace std;

SQLQueryManager::SQLQueryManager()
{
	auto fs = cmrc::oasis::get_filesystem();
	for (const auto& entry : fs.iterate_directory(SQL_QUERIES_DIR))
	{
		auto queryFile = fs.open(SQL_QUERIES_DIR + entry.filename());
		auto queryText = string(queryFile.begin(), queryFile.end());
		auto queryName = entry.filename().substr(0, entry.filename().length() - 4); // remove .sql suffix
		this->_queries.insert(make_pair(queryName, queryText));
	}
}

optional<const string> SQLQueryManager::getQueryByName(const string& name)
{
	if (this->_queries.contains(name))
	{
		return this->_queries[name];
	}
	return {};
}