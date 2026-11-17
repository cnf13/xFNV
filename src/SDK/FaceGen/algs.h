#pragma once

#include "SDK/MSVC/iosfwd.h"
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <istream>
#include <stdint.h>

// Inferred type: a1: uint32_t: unsigned uses from 0xac69d2
void futChooseTwoLabels(uint32_t a1, int32_t a2, uint32_t* a3, uint32_t* a4);

char fr2FileNameSplit(std::basic_string<char, std::char_traits<char>, std::allocator<char>> a1,
	std::basic_string<char, std::char_traits<char>, std::allocator<char>>* arg1C,
	std::basic_string<char, std::char_traits<char>, std::allocator<char>>* a3,
	std::basic_string<char, std::char_traits<char>, std::allocator<char>>* a4);

// The path past its root (drive, UNC server and share), or null.
char* futPathFindNextComponent(char* Str);

// The exe leaves arBytes.size() in eax; it has no callers.
void futStrToHexStr(const std::vector<char, std::allocator<char>>& arBytes,
	std::vector<char, std::allocator<char>>& arHex);

// The hex text arHex decoded into arBytes, two digits per byte, spaces skipped, up to a 0 character. arHex is the
// vector<char> whose operator[] (0xACAD30, non-const) the exe calls on [esp+4]; arBytes the vector<char> it clears
// (0xACAE90) and appends to (0xACAD60). The exe sets no eax before its ret; it has no callers.
void futHexStrToStr(std::vector<char, std::allocator<char>>& arHex,
	std::vector<char, std::allocator<char>>& arBytes);

// Reads key/value lines from the stream a1 into
// the string map a2 (AC8390 and AC87B0 pass their map on). a1 is the input stream: AC8390 passes an istringstream,
// AC87B0 an ifstream.
// Inferred type: a4: int32_t: signed uses from 0xac7b55
char FutStreamPairParser(std::basic_istream<char, std::char_traits<char>>* a1,
	std::map<std::string, std::string>* a2,
	std::basic_string<char, std::char_traits<char>, std::allocator<char>>* a3, int32_t a4);

// Reads key/value lines from the stream a1 into
// the vector of string pairs a2 (AC8C50 passes its vector on). a1 is the input stream: the body calls basic_istream's
// get, peek, getline and operator>> on it.
// Inferred type: a4: int32_t: signed uses from 0xac8005
char FutStreamPairParserVec(std::basic_istream<char, std::char_traits<char>>* a1,
	std::vector<std::pair<std::string, std::string>, std::allocator<std::pair<std::string, std::string>>>*
		a2,
	std::basic_string<char, std::char_traits<char>, std::allocator<char>>* a3, int32_t a4);

// Not named in the PDB or IDA; name chosen here. Parses the text buffer a1 into the string map
// a3: the exe calls the map's _Tree members on a3 (clear, find) and passes it to FutStreamPairParser.
// Inferred type: a1: char* from 0xac83f7 (passed to std::basic_string<char, std::char_traits<char>,
// std::allocator<char>>::Construct 1)
char FutBufferPairParser(char* a1, std::basic_string<char, std::char_traits<char>, std::allocator<char>>* a2,
	std::map<std::string, std::string>* a3);

// Reads the file named a1 into the string map
// a3, as FutBufferPairParser does for a buffer.
char FutFilePairParser(std::basic_string<char, std::char_traits<char>, std::allocator<char>>* a1,
	std::basic_string<char, std::char_traits<char>, std::allocator<char>>* arg4,
	std::map<std::string, std::string>* a3);

// Not named in the PDB or IDA; name chosen here. Reads the file named a1 into the vector of
// string pairs a3: the exe resizes a3 to 0, sizes it by its 56-byte elements (_Myfirst +0xC, _Mylast +0x10) and
// compares the first element's first string with the string arg4.
char FutFilePairParser(std::basic_string<char, std::char_traits<char>, std::allocator<char>>* a1,
	const std::basic_string<char, std::char_traits<char>, std::allocator<char>>* arg4,
	std::vector<std::pair<std::string, std::string>, std::allocator<std::pair<std::string, std::string>>>*
		a3);

// Not named in the PDB or IDA; name chosen here. Reads the comma-separated file named a1 into arg4, one vector of
// field strings per line that has a non-blank field: the exe calls clear and push_back of the vector of string vectors
// on arg4 (0xAC915B, 0xAC95E0).
char futLoadCsvFile(std::basic_string<char, std::char_traits<char>, std::allocator<char>>* a1,
	std::vector<std::vector<std::string, std::allocator<std::string>>,
		std::allocator<std::vector<std::string, std::allocator<std::string>>>>* arg4);

// Not named in the PDB or IDA; name chosen here. Writes the rows arg4 to the file named a1,
// each string quoted: the exe walks arg4 by its 24-byte vector elements (_Myfirst +0xC, _Mylast +0x10) and each row
// by its 28-byte strings.
char futSaveCsvFile(std::basic_string<char, std::char_traits<char>, std::allocator<char>>* a1,
	std::vector<std::vector<std::string, std::allocator<std::string>>,
		std::allocator<std::vector<std::string, std::allocator<std::string>>>>* arg4);

// Not named in the PDB or IDA; name chosen here. A byte vector's _Ufill (thiscall in the exe, ecx unused, ret 0xC):
// fill_n of a2 bytes through memset_ (0xAC3300), then a1 + a2.
uint8_t* UninitFillN(uint8_t* a1, size_t a2, const uint8_t* a3);

// Not named in the PDB or IDA; name chosen here. VC9's _Copy_opt over chars (vector<char>::erase, 0xACEFD0): one
// memmove_s of [_First, _Last) to _Dest; returns _Dest's end (cdecl, ret 0).
char* CopyOpt(const char* _First, const char* _Last, char* _Dest);

// Not named in the PDB or IDA; name chosen here. VC9's _Copy_opt over vector<string> elements
// (vector<vector<string>>::erase, 0xAD0110): each assigned through 0xAD61E0; returns _Dest's end (cdecl, ret 0).
std::vector<std::string, std::allocator<std::string>>* CopyOpt(
	const std::vector<std::string, std::allocator<std::string>>* _First,
	const std::vector<std::string, std::allocator<std::string>>* _Last,
	std::vector<std::string, std::allocator<std::string>>* _Dest);

// VC9's _Move_backward over strings with the swap tag:
// swaps [_First, _Last) into the slots ending at _Dest, last first; returns the start of the moved range.
std::string* UncheckedMoveBackward(std::string* _First, std::string* _Last, std::string* _Dest);

// <xmemory>'s _Allocate of strings, out of line: ourMalloc of _Count
// strings; throws bad_alloc when the size overflows.
std::string* _Allocate(uint32_t _Count);
