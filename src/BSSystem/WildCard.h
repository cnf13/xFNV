#pragma once

#include "Reconstruction.hpp"

// The file-name wildcard matcher: a static class of pattern functions with one shared flag word (bit 0 case
// sensitive, 1 compare the path, 2 compare the extension, 3 skip leading spaces). No instances.
class WildCard {
public:
	// Whether apString matches apPattern: copies both through MakeString, then RecMatch.
	static bool Match(const char* apString, const char* apPattern);
	// The recursive matcher over the already normalised strings: 1 for a match, 0 for none, 2
	// when no later start can match either.
	static int32_t RecMatch(const char* apString, const char* apPattern);
	// Copies apSrc to apDest (260 bytes) with the leading spaces skipped and the path and
	// extension kept or dropped according to the flags (cdecl, 0xAF4070). IDA's name; the PDB's is MakeString.
	static void MakeString(char* apDest, const char* apSrc);
	static bool IsWild(const char* apPattern);

	static inline int32_t iFlags;
};
