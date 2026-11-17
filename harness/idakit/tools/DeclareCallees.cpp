// declare --callees: the functions an emit directory's bodies call without a src declaration (spots.tsv's `no src
// declaration of X (ADDR)`), each address once, for declare's own per-address run (docs/IDAKIT.md "idakit declare").
// An address that is no function's start (an import's slot, which src declares as its IAT global) is no callee to
// declare. declare --fold-calls: the calls of a class template instance's member on another type (spots.tsv's root
// `call of O::f on R` at the callee its origin `ret:ADDR` names), each (address, owner, receiver) once.

#include "idakit/Database.hpp"
#include "idakit/Ida.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

namespace idakit
{

// The callees' addresses in order, and the reason each address not taken is passed over. Only (non-empty): those of
// these addresses.
std::vector<uint64_t> calleeAddresses(const std::filesystem::path& dir, const std::vector<uint64_t>& only,
									  std::map<uint64_t, std::string>& passed)
{
	std::ifstream in(dir / "spots.tsv");
	if (!in)
		throw Error("idakit: cannot read " + (dir / "spots.tsv").string());
	static const std::regex reason(R"(^no src declaration of .* \(0x([0-9a-fA-F]+)\)$)");
	std::set<uint64_t> callees;
	std::string line;
	while (std::getline(in, line))
	{
		std::vector<std::string> cells;
		std::istringstream row(line);
		for (std::string cell; std::getline(row, cell, '\t');)
			cells.push_back(cell);
		std::smatch m;
		if (cells.size() < 3 || !std::regex_match(cells[2], m, reason))
			continue;
		uint64_t ea = std::stoull(m[1], nullptr, 16);
		if (!only.empty() && std::find(only.begin(), only.end(), ea) == only.end())
			continue;
		func_t* f = get_func(static_cast<ea_t>(ea));
		if (!f || f->start_ea != static_cast<ea_t>(ea))
			passed[ea] = "no function starts here (an import's slot: src declares its IAT global)";
		else
			callees.insert(ea);
	}
	return {callees.begin(), callees.end()};
}

std::vector<std::tuple<uint64_t, std::string, std::string>> instanceCalls(const std::filesystem::path& dir)
{
	std::ifstream in(dir / "spots.tsv");
	if (!in)
		throw Error("idakit: cannot read " + (dir / "spots.tsv").string());
	static const std::regex reason(R"(^call of (.+<.*>::[^:]+) on (.+)$)"), origin(R"(^ret:0x([0-9a-fA-F]+)$)");
	std::set<std::tuple<uint64_t, std::string, std::string>> calls;
	for (std::string line; std::getline(in, line);)
	{
		std::vector<std::string> cells;
		std::istringstream row(line);
		for (std::string cell; std::getline(row, cell, '\t');)
			cells.push_back(cell);
		std::smatch m, o;
		if (cells.size() < 4 || cells[1] != "1" || !std::regex_match(cells[2], m, reason) ||
			!std::regex_match(cells[3], o, origin))
			continue;
		calls.emplace(std::stoull(o[1], nullptr, 16), m[1].str(), m[2].str());
	}
	return {calls.begin(), calls.end()};
}

}  // namespace idakit
