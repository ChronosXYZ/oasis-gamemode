#pragma once

#include <regex>
#include <string>
#include <vector>
#include <sstream>

namespace Utils
{
namespace Strings
{
	inline std::vector<std::string> split(std::string str, char separator)
	{
		std::vector<std::string> separatedStrings;
		int startIndex = 0, endIndex = 0;
		for (int i = 0; i <= str.size(); i++)
		{
			if (str[i] == separator || i == str.size())
			{
				endIndex = i;
				std::string temp;
				temp.append(str, startIndex, endIndex - startIndex);
				separatedStrings.push_back(temp);
				startIndex = endIndex + 1;
			}
		}
		return separatedStrings;
	};

	template <typename Numeric>
	inline bool isNumber(const std::string& s)
	{
		std::istringstream iss(s);
		Numeric f;
		iss >> std::noskipws >> f; // noskipws considers leading whitespace invalid
		// Check the entire string was consumed and if either failbit or badbit is set
		return iss.eof() && !iss.fail();
	}

	inline std::vector<std::smatch> findAllMatches(const std::regex& regexp, const std::string& str)
	{
		std::vector<std::smatch> matches;
		std::sregex_iterator iter(str.begin(), str.end(), regexp);
		std::sregex_iterator end;
		for (std::sregex_iterator i = iter; i != end; ++i)
		{
			matches.push_back(*i);
		}
		return matches;
	}
}
}