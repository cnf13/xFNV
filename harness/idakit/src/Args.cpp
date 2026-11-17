#include "idakit/Command.hpp"

#include "idakit/Database.hpp"

#include <algorithm>

namespace idakit
{

Args::Args(const std::vector<std::string>& words, const std::vector<std::string>& flags)
{
	bool options = true;
	for (size_t k = 0; k < words.size(); ++k)
	{
		const std::string& word = words[k];
		if (options && word == "--")
		{
			options = false;
			continue;
		}
		if (!options || word.rfind("--", 0) != 0)
		{
			m_positional.push_back(word);
			continue;
		}
		std::string name = word.substr(2);
		if (std::find(flags.begin(), flags.end(), name) != flags.end())
		{
			m_flags.push_back(name);
			continue;
		}
		if (k + 1 >= words.size())
			throw Error("idakit: --" + name + " needs a value");
		m_options[name].push_back(words[++k]);
	}
}

std::optional<std::string> Args::option(const std::string& name) const
{
	auto found = m_options.find(name);
	if (found == m_options.end())
		return std::nullopt;
	return found->second.back();
}

std::vector<std::string> Args::values(const std::string& name) const
{
	auto found = m_options.find(name);
	return found == m_options.end() ? std::vector<std::string>() : found->second;
}

std::string Args::required(const std::string& name) const
{
	auto value = option(name);
	if (!value)
		throw Error("idakit: --" + name + " is required");
	return *value;
}

bool Args::flag(const std::string& name) const
{
	return std::find(m_flags.begin(), m_flags.end(), name) != m_flags.end();
}

}  // namespace idakit
