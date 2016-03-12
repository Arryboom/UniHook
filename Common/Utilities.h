#pragma once
#include <vector>
#include <string>
std::vector<std::string> split(const std::string &s, const std::string& delim)
{
	std::vector<std::string> tokens;
	size_t start = 0;
	size_t end = s.find(delim);

	while (end != std::string::npos)
	{
		tokens.push_back(s.substr(start, end - start));
		start = end + delim.length();
		end = s.find(delim, start);
	}
	tokens.push_back(s.substr(start, end));
	return tokens;
}