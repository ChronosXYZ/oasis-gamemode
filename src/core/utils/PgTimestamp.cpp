#include "PgTimestamp.hpp"
#include <bits/chrono.h>
#include <chrono>

namespace Core::Utils::SQL
{
timestamp from_iso8601_str(const std::string& s)
{
	timestamp ts;
	if (!from_iso8601_str(s, ts))
		throw std::invalid_argument { "failed to parse " + s
			+ " as an ISO 8601 timestamp" };
	return ts;
}

bool from_iso8601_str(const std::string& s, timestamp& ts)
{
	std::istringstream stream { s };
	stream >> date::parse("%F %T", ts);
	return !stream.fail();
}

std::string to_iso8601_str(const timestamp& ts)
{
	return date::format("%F %T", ts);
}

std::string to_http_ts_str(const timestamp& ts)
{
	std::stringstream weekday_abbreviation;
	weekday_abbreviation << static_cast<iso_week::year_weeknum_weekday>(
		std::chrono::time_point_cast<date::days>(ts))
								.weekday();

	return (weekday_abbreviation.str()
		// timestamps serialize to UTC/GMT by default
		+ date::format(" %d-%m-%Y %H:%M:%S GMT",
			std::chrono::time_point_cast<std::chrono::seconds>(ts)));
}

timestamp from_unix_time(unsigned int unix_time)
{
	return timestamp { std::chrono::duration_cast<std::chrono::microseconds>(
		std::chrono::seconds { unix_time }) };
}

unsigned int to_unix_time(const timestamp& ts)
{
	return std::chrono::duration_cast<std::chrono::seconds>(
		ts.time_since_epoch())
		.count();
}

timestamp get_current_timestamp()
{
	using namespace std::chrono;
	return timestamp { duration_cast<microseconds>(
		system_clock::now().time_since_epoch()) };
}
}