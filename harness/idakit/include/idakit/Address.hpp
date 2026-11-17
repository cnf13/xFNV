// Address lists as the harness tools pass them: hex addresses (0x optional) and @FILE (one per line; text after the
// first word or a '#' ignored).
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace idakit
{

// In command-line order, duplicates dropped. Throws Error on a word that is no hex address or an unreadable @FILE.
std::vector<uint64_t> parseAddresses(const std::vector<std::string>& items);

// `%08X`: the cache file stem the Python tools use (00401BF0).
std::string hex8(uint64_t address);

// `0x%x`: Python's hex().
std::string hexText(uint64_t address);

}  // namespace idakit
