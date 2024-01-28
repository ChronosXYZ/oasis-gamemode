#include <pqxx/pqxx>
#include <date/date.h>
#include "iso_week.h"
#include <chrono>
#include <string>
#include <iomanip>
#include <sstream>
#include <stdexcept> // std::invalid_argument

namespace Utils::SQL
{
using timestamp = std::chrono::sys_time<std::chrono::microseconds>;

timestamp from_iso8601_str(const std::string&);
bool from_iso8601_str(const std::string&, timestamp&);
std::string to_iso8601_str(const timestamp&);
std::string to_http_ts_str(const timestamp&);

timestamp from_unix_time(unsigned int);
unsigned int to_unix_time(const timestamp&);
timestamp get_current_timestamp();
}

namespace pqxx
{
template <>
struct nullness<Utils::SQL::timestamp> : no_null<Utils::SQL::timestamp>
{
};

template <>
struct string_traits<Utils::SQL::timestamp>
{
	using subject_type = Utils::SQL::timestamp;
	static constexpr bool converts_to_string { true };
	static constexpr bool converts_from_string { true };

	static char* into_buf(char* begin, char* end, Utils::SQL::timestamp const& value)
	{
		auto str = Utils::SQL::to_iso8601_str(value);
		if (internal::cmp_greater_equal(std::size(str), end - begin))
			throw conversion_overrun {
				"Could not convert timestamp to string: too long for buffer."
			};
		// Include the trailing zero.
		str.copy(begin, std::size(str));
		begin[std::size(str)] = '\0';
		return begin + std::size(str) + 1;
	}

	static zview to_buf(char* begin, char* end, std::string const& value)
	{
		return generic_to_buf(begin, end, value);
	}

	static Utils::SQL::timestamp from_string(std::string_view text)
	{
		auto str = Utils::SQL::from_iso8601_str(std::string(text));
		return str;
		// throw argument_error {
		// 	"Failed conversion to "
		// 	+ static_cast<std::string>(name())
		// 	+ ": '"
		// 	+ static_cast<std::string>(str)
		// 	+ "'"
		// };
	}

	static std::string to_string(const Utils::SQL::timestamp& ts)
	{
		return Utils::SQL::to_iso8601_str(ts);
	}

	static std::size_t size_buffer(Utils::SQL::timestamp const& value) noexcept
	{
		return Utils::SQL::to_iso8601_str(value).length() + 1;
	}
};
}
