#include "idakit/Address.hpp"

#include "idakit/Database.hpp"

#include <cstdio>
#include <fstream>
#include <set>
#include <sstream>

namespace idakit
{

namespace
{

uint64_t parseHex(const std::string& word)
{
	std::string digits = word.rfind("0x", 0) == 0 || word.rfind("0X", 0) == 0 ? word.substr(2) : word;
	if (digits.empty() || digits.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos)
		throw Error("idakit: not a hex address: " + word);
	return std::stoull(digits, nullptr, 16);
}

}  // namespace

std::vector<uint64_t> parseAddresses(const std::vector<std::string>& items)
{
	std::vector<uint64_t> out;
	std::set<uint64_t> seen;
	auto add = [&](const std::string& word) {
		uint64_t address = parseHex(word);
		if (seen.insert(address).second)
			out.push_back(address);
	};
	for (const std::string& item : items)
	{
		if (item.empty() || item[0] != '@')
		{
			add(item);
			continue;
		}
		std::ifstream file(item.substr(1));
		if (!file)
			throw Error("idakit: cannot read " + item.substr(1));
		std::string line;
		while (std::getline(file, line))
		{
			line = line.substr(0, line.find('#'));
			std::istringstream words(line);
			std::string word;
			if (words >> word)
				add(word);
		}
	}
	return out;
}

std::string hex8(uint64_t address)
{
	char text[32];
	std::snprintf(text, sizeof(text), "%08llX", static_cast<unsigned long long>(address));
	return text;
}

std::string hexText(uint64_t address)
{
	char text[32];
	std::snprintf(text, sizeof(text), "0x%llx", static_cast<unsigned long long>(address));
	return text;
}

}  // namespace idakit
