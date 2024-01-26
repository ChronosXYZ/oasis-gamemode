#pragma once

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

	inline bool isNumber(const std::string& s)
	{
		return !s.empty() && std::ranges::find_if(s.begin(), s.end(), [](unsigned char c)
								 {
									 return !std::isdigit(c);
								 })
			== s.end();
	}

	inline bool isDouble(const std::string& s)
	{
		std::istringstream iss(s);
		double f;
		iss >> std::noskipws >> f; // noskipws considers leading whitespace invalid
		// Check the entire string was consumed and if either failbit or badbit is set
		return iss.eof() && !iss.fail();
	}
}
}