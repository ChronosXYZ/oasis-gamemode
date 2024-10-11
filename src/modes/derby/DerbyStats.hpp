#pragma once

#include <pqxx/pqxx>

namespace Modes::Derby
{
struct DerbyStats
{
	unsigned int score = 0;
	unsigned int drops = 0;
	unsigned int dropped = 0;
	unsigned int derbiesWon = 0;
	unsigned int derbyRank = 0;

	DerbyStats() = default;

	void updateFromRow(const pqxx::row& row)
	{
		score = row["score"].as<unsigned int>();
		drops = row["drops"].as<unsigned int>();
		dropped = row["dropped"].as<unsigned int>();
		derbiesWon = row["derbies_won"].as<unsigned int>();
		derbyRank = row["derby_rank"].as<unsigned int>();
	}
};
}
