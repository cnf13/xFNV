#include "SDK/FaceGen/algs.h"
#include "BSCore/MemoryManager.h"
#include "Reconstruction.hpp"
#include "SDK/FaceGen/StrContainers.h"
#include "SDK/FaceGen/algs.hpp"
#include "SDK/FaceGen/assert.hpp"
#include "SDK/FaceGen/lastError.hpp"
#include "SDK/MSVC/crtdefs.h"
#include "SDK/MSVC/exception.h"
#include <fstream>
#include <ios>
#include "SDK/MSVC/iosfwd.h"
#include <istream>
#include <cmath>
#include <sstream>
#include "SDK/MSVC/stdexcept.h"
#include "SDK/MSVC/stdio.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"
#include <vector>
#include <locale>
#include <string>
#include <memory>
#include <map>
#include <stdint.h>

// Opens fname for reading ("r", the exe's literal at 0x1082CB8) and closes it again; c_str() is inlined.
bool fr2CheckFileExists(const std::string& fname)
{
	const char* pcName = fname.c_str();
	std::FILE* f = rt::fopen(pcName, "r");
	if (!f) {
		return false;
	}
	std::fclose(f);
	return true;
}


// Every operator+ is the exe's out-of-line call into a temporary, each destroyed (_Tidy(true, 0) inlined) in the exe's
// order; the count is printed with sprintf into a 12-byte frame buffer. The result is the local copied into the
// caller's string (the copy constructor inlined: _Tidy(false, 0), assign(local, 0, npos)), then the local destroyed.
// The exe's cleanup-only EH frame is left out.
std::string* futComposeNewFname(std::string* _Result, const std::string& path,
	const std::string& outRoot, const std::string& ext)
{
	std::string sName = outRoot + "." + ext;
	int32_t iCount = 0;
	while (fr2CheckFileExists(path + sName)) {
		++iCount;
		sName = outRoot + std::to_string(iCount) + "." + ext;
	}
	::new (_Result) std::string(sName);
	return _Result;
}

// The kept characters are appended one at a time as a one-character C string (strlen and append out of line); the
// character is read through operator[] (its check _Off <= size()), inlined. The result is the local copied into the
// caller's string (the copy constructor inlined: _Tidy(false, 0), assign(local, 0, npos)), then the local destroyed.
// The exe's cleanup-only EH frame is left out.
std::string* futRemoveBadFnameCharacters(std::string* _Result, const std::string& fname)
{
	std::string sOut("");
	char acSource[2];
	acSource[0] = 0;
	acSource[1] = 0;
	for (uint32_t i = 0; i < fname.size(); ++i) {
		acSource[0] = fname[i];
		if (acSource[0] == '*' || acSource[0] == '?' || acSource[0] == '"' || acSource[0] == '<' ||
			acSource[0] == '>' || acSource[0] == '|' || acSource[0] == ':' || acSource[0] == '\\' ||
			acSource[0] == '/' || acSource[0] == ' ') {
			continue;
		}
		sOut.append(acSource, std::strlen(acSource));
	}
	::new (_Result) std::string(sOut);
	return _Result;
}

// A NaN argument is reported. An infinite or NaN sum becomes the largest float of the sum's sign; the sign test
// compares the sum with its fabs, so a NaN sum gives -FLT_MAX.
template <>
float fr2AddSaturated<float>(float val1, float val2)
{
	if (std::isnan(val1)) {
		fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\algs.cpp", 434);
	}
	if (std::isnan(val2)) {
		fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\algs.cpp", 435);
	}
	float fSum = val1 + val2;
	if (std::isfinite(fSum)) {
		return fSum;
	}
	float fAbs = std::fabs(static_cast<double>(fSum));
	if (fSum == fAbs) {
		return 3.4028235e+38f;
	}
	return -3.4028235e+38f;
}

// The sum clamped to [-32768, 32767]; the exe then returns only the result's low byte, sign-extended.
template <>
int16_t fr2AddSaturated<int16_t>(int16_t val1, int16_t val2)
{
	int32_t iSum = val2 + val1;
	iSum = iSum < 32767 ? iSum : 32767;
	iSum = iSum > -32768 ? iSum : -32768;
	return static_cast<int16_t>(static_cast<int8_t>(iSum));
}

uint32_t fr2Log2Floor(uint32_t a)
{
	if (!a) {
		// "D:\_Fallout3\Platforms\Common\Code\SDK\FaceGen\algs.cpp"
		fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\algs.cpp", 39);
	}
	uint32_t uiBits = 0;
	do {
		a >>= 1;
		++uiBits;
	} while (a);
	return uiBits - 1;
}

uint32_t fr2Log2Ceil(uint32_t a) {
	if (a == ((uint32_t)((int32_t)(1 << fr2Log2Floor(a))))) {
		return fr2Log2Floor(a);
	} else {
		return fr2Log2Floor(a) + 1;
	}
}

void futChooseTwoLabels(uint32_t a1, int32_t a2, uint32_t* a3, uint32_t* a4) {
	if (!a1) {
		fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\algs.cpp", 83);
	}
	int v4 = 0;
	for (*a3 = 0; *a3 < a1; ++*a3) {
		for (*a4 = *a3 + 1; *a4 < a1; ++*a4) {
			if (a2 == v4) {
				return;
			}
			++v4;
		}
	}
	fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\algs.cpp", 96);
}

// The exe's cleanup-only C++ EH frame is left out. The callee destroys the by-value a1 on each return (MSVC).
char fr2FileNameSplit(std::basic_string<char, std::char_traits<char>, std::allocator<char>> a1, std::basic_string<char, std::char_traits<char>, std::allocator<char>>* arg1C, std::basic_string<char, std::char_traits<char>, std::allocator<char>>* a3, std::basic_string<char, std::char_traits<char>, std::allocator<char>>* a4) {
	size_t* v6;
	std::basic_string<char, std::char_traits<char>, std::allocator<char>> v14;
	std::basic_string<char, std::char_traits<char>, std::allocator<char>> v15;
	std::basic_string<char, std::char_traits<char>, std::allocator<char>> _Result;
	size_t v17;
	size_t v18;
	size_t v21;
	size_t v23;
	size_t Mysize = a1.size();
	size_t Size = std::strlen("\\");
	v23 = a1.rfind("\\", std::string::npos, Size);
	size_t _Count = std::strlen("/");
	v21 = a1.rfind("/", std::string::npos, _Count);
	size_t v11 = std::strlen(":");
	v18 = a1.rfind(":", std::string::npos, v11);
	if (v23 == -1) {
		v23 = 0;
	} else {
		++v23;
	}
	if (v21 == -1) {
		v21 = 0;
	} else {
		++v21;
	}
	if (v18 == -1) {
		v18 = 0;
	} else {
		++v18;
	}
	if (v23 >= v21) {
		v6 = &v23;
	} else {
		v6 = &v21;
	}
	v17 = *v6;
	if (v17 && v18 > v17) {
		return 0;
	} else {
		size_t* v5;
		size_t v10;
		size_t v19;
		size_t v22;
		if (v17 >= v18) {
			v5 = &v17;
		} else {
			v5 = &v18;
		}
		v17 = *v5;
		if (v17 == Mysize || ((v10 = std::strlen("."), (v22 = a1.rfind(".", std::string::npos, v10), v22 != -1)) ? v19 = v22 + 1 : (v22 = Mysize, v19 = Mysize), v22 == v17)) {
			return 0;
		} else {
			if (v22 < v17) {
				v22 = Mysize;
			}
			if (v17 >= v22) {
				fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\algs.cpp", 170);
			}
			*arg1C = a1.substr(0, v17);
			*a3 = a1.substr(v17, v22 - v17);
			*a4 = a1.substr(v19, Mysize - v19);
			return 1;
		}
	}
}

// The path after its root: past a drive's "X:" (a drive-relative "X:name" returns the name), and for a UNC path
// ("\\server\share\...") past the server and share; null for no path, an empty one or nothing after the root. The
// separators are the exe's literal "\\/" at 0x10A2F88.
char* futPathFindNextComponent(char* Str)
{
	char* pcRest = nullptr;
	if (!Str) {
		return pcRest;
	}
	if (!*Str) {
		return pcRest;
	}
	char* pcStart = Str;
	char* pcColon = std::strchr(Str, ':');
	if (pcColon) {
		pcStart = pcColon + 1;
		if (*pcStart != '\\' && *pcStart != '/') {
			return pcStart;
		}
	}
	char* pcSeparator = std::strpbrk(pcStart, "\\/");
	if (pcSeparator) {
		pcRest = pcSeparator + 1;
		if (pcRest == Str + 1 && std::strpbrk(pcRest, "\\/") == pcRest) {
			++pcRest;
			pcRest = std::strpbrk(pcRest, "\\/");
			if (pcRest) {
				++pcRest;
				pcRest = std::strpbrk(pcRest, "\\/");
			}
			if (pcRest) {
				++pcRest;
			}
		}
	}
	if (pcRest && !*pcRest) {
		pcRest = nullptr;
	}
	return pcRest;
}

// Appends the bytes of arBytes to arHex as upper-case hex digit pairs, a space before every second byte after the
// first; arHex is cleared first. clear and push_back are called, operator[] (with its range check) and size()
// inlined.
void futStrToHexStr(const std::vector<char, std::allocator<char>>& arBytes,
	std::vector<char, std::allocator<char>>& arHex)
{
	arHex.clear();
	for (int32_t i = 0; static_cast<uint32_t>(i) < arBytes.size(); ++i) {
		if (i > 0 && i % 2 == 0) {
			char cSpace = ' ';
			arHex.push_back(cSpace);
		}
		// The high nibble is read zero-extended, the low one sign-extended (movzx/sar, movsx/and).
		uint8_t ucHigh = static_cast<uint8_t>(static_cast<uint8_t>(arBytes[i]) >> 4);
		uint8_t ucLow = static_cast<uint8_t>(arBytes[i] & 0xF);
		char cHigh;
		if (ucHigh < 10) {
			cHigh = static_cast<char>(ucHigh + '0');
		} else {
			cHigh = static_cast<char>(ucHigh + 'A' - 10);
		}
		char cLow;
		if (ucLow < 10) {
			cLow = static_cast<char>(ucLow + '0');
		} else {
			cLow = static_cast<char>(ucLow + 'A' - 10);
		}
		arHex.push_back(cHigh);
		arHex.push_back(cLow);
	}
}

// Two hex digits per byte, upper or lower case, spaces between bytes skipped; a character that is no digit counts as
// 0. Stops at the last character or at a 0 in either digit. The second digit's '0'-'9' case reads the character
// through the out-of-line operator[], the others inline.
void futHexStrToStr(std::vector<char, std::allocator<char>>& arHex,
	std::vector<char, std::allocator<char>>& arBytes) {
	arBytes.clear();
	int32_t iSize = static_cast<int32_t>(arHex.size());
	for (int32_t i = 0; i < iSize; ++i) {
		if (i + 1 >= iSize) {
			break;
		}
		if (arHex[i] == ' ') {
			continue;
		}
		if (arHex[i] == '\0' || arHex[i + 1] == '\0') {
			break;
		}
		uint8_t ucByte = 0;
		if (arHex[i] >= 'A' && arHex[i] <= 'F') {
			ucByte = arHex[i] - 'A' + 10;
		} else if (arHex[i] >= 'a' && arHex[i] <= 'f') {
			ucByte = arHex[i] - 'a' + 10;
		} else if (arHex[i] >= '0' && arHex[i] <= '9') {
			ucByte = arHex[i] - '0';
		}
		ucByte <<= 4;
		++i;
		if (arHex[i] >= 'A' && arHex[i] <= 'F') {
			ucByte = ucByte + arHex[i] - 'A' + 10;
		} else if (arHex[i] >= 'a' && arHex[i] <= 'f') {
			ucByte = ucByte + arHex[i] - 'a' + 10;
		} else if (arHex[i] >= '0' && arHex[i] <= '9') {
			ucByte = ucByte + arHex[i] - '0';
		}
		char cByte = static_cast<char>(ucByte);
		arBytes.push_back(cByte);
	}
}

// Reads "key value" lines from a1 into the map a2, at most a4 of them when a4 is not 0: each key with >>, the
// spaces and tabs after it skipped, the rest of the line with getline; a key equal to a3 is skipped unless a3 is
// empty. Returns 1 unless the stream failed before its end. The stream state is read inline through the vbtable at
// each test (`[a1 + [[a1]+4] + 8]`), as _Myios() does. The exe's cleanup-only C++ EH frame is left out.
char FutStreamPairParser(std::basic_istream<char, std::char_traits<char>>* a1, std::map<std::string, std::string>* a2,
	std::basic_string<char, std::char_traits<char>, std::allocator<char>>* a3, int32_t a4) {
	std::string arStr;
	int32_t v17 = 0;
	while (!a1->eof() && !a1->fail()) {
		if (a4 && v17 >= a4) {
			break;
		}
		*a1 >> arStr;
		if (a1->eof() || a1->fail()) {
			break;
		}
		while (!a1->eof() && !a1->fail()) {
			char v15 = static_cast<char>(a1->peek());
			if (v15 == '\n' || a1->eof() || a1->fail() || (v15 != ' ' && v15 != '\t')) {
				break;
			}
			a1->get();
		}
		std::string v16;
		if (!a1->eof() && !a1->fail()) {
			std::getline(*a1, v16, a1->widen('\n'));
		}
		if (arStr.compare(*a3) || a3->empty()) {
			a2->insert(std::pair<const std::string, std::string>(arStr, v16));
			++v17;
		}
	}
	return !a1->fail() || a1->eof() ? 1 : 0;
}

// FutStreamPairParser's reading into the vector of pairs a2 (push_back of each key and value) in place of a map. The exe's
// cleanup-only C++ EH frame is left out.
char FutStreamPairParserVec(std::basic_istream<char, std::char_traits<char>>* a1, std::vector<std::pair<std::basic_string<char, std::char_traits<char>, std::allocator<char>>, std::basic_string<char, std::char_traits<char>, std::allocator<char>>>, std::allocator<std::pair<std::basic_string<char, std::char_traits<char>, std::allocator<char>>, std::basic_string<char, std::char_traits<char>, std::allocator<char>>>>>* a2, std::basic_string<char, std::char_traits<char>, std::allocator<char>>* a3, int32_t a4) {
	std::string arStr;
	int32_t v15 = 0;
	while (!a1->eof() && !a1->fail()) {
		if (a4 && v15 >= a4) {
			break;
		}
		*a1 >> arStr;
		if (a1->eof() || a1->fail()) {
			break;
		}
		while (!a1->eof() && !a1->fail()) {
			char v13 = static_cast<char>(a1->peek());
			if (v13 == '\n' || a1->eof() || a1->fail() || (v13 != ' ' && v13 != '\t')) {
				break;
			}
			a1->get();
		}
		std::string apMemory;
		if (!a1->eof() && !a1->fail()) {
			std::getline(*a1, apMemory, a1->widen('\n'));
		}
		if (arStr.compare(*a3) || a3->empty()) {
			a2->push_back(std::pair<std::string, std::string>(arStr, apMemory));
			++v15;
		}
	}
	return !a1->fail() || a1->eof() ? 1 : 0;
}

// The exe's cleanup-only C++ EH frame is left out. The istringstream is a complete object, the class part with its
// basic_ios virtual base right after it (+0x50): constructed with aiMostDerived = 1, destroyed by the class's
// Destruct, which the exe enters with the ios subobject, then by ~basic_ios on the ios.
char FutBufferPairParser(char* a1, std::basic_string<char, std::char_traits<char>, std::allocator<char>>* a2,
	std::map<std::string, std::string>* a3)
{
	if (!a1) {
		futSetLastError(FUTERR_SEE_TEXT, "Input buffer is empty.");
		return 0;
	}
	std::istringstream kStream(std::string(a1), std::ios_base::in);
	if (kStream.fail()) {
		futSetLastError(FUTERR_SEE_TEXT, "Unable to create string stream for buffer.");
		return 0;
	}
	a3->clear();
	// A non-empty name a2: a first pass, after which the map must hold a2.
	if (!a2->empty()) {
		std::string kSeparator("//");
		if (!FutStreamPairParser(&kStream, a3, &kSeparator, 1)) {
			futSetLastError(FUTERR_SEE_TEXT, "Unable to parse string stream.");
			return 0;
		}
		if (a3->find(*a2) == a3->end()) {
			futSetLastError(FUTERR_FILE_INCORRECT_TYPE, "string stream buffer");
			return 0;
		}
	}
	a3->clear();
	std::string kSeparator("//");
	if (!FutStreamPairParser(&kStream, a3, &kSeparator, 0)) {
		futSetLastError(FUTERR_FILE_NOT_READABLE, "string stream buffer");
		return 0;
	}
	return 1;
}

// The exe's cleanup-only C++ EH frame is left out. The ifstream is a complete object, the class part with its
// basic_ios virtual base right after it (+0x58): constructed with aiMostDerived = 1, destroyed by the class's
// Destruct, which the exe enters with the ios subobject, then by ~basic_ios on the ios. kUnused is a string the exe
// default-constructs (inlined) and destroys on every return without using it.
// The exe's share flag (_SH_DENYNO) has no standard form.
char FutFilePairParser(std::basic_string<char, std::char_traits<char>, std::allocator<char>>* a1,
	std::basic_string<char, std::char_traits<char>, std::allocator<char>>* arg4,
	std::map<std::string, std::string>* a3)
{
	std::ifstream kStream(a1->c_str(), std::ios_base::in);
	if (kStream.fail()) {
		futSetLastError(FUTERR_FILE_NOT_READABLE, *a1);
		return 0;
	}
	a3->clear();
	std::string kSeparator("//");
	if (!FutStreamPairParser(&kStream, a3, &kSeparator, 1)) {
		futSetLastError(FUTERR_FILE_NOT_READABLE, *a1);
		return 0;
	}
	if (a3->find(*arg4) == a3->end()) {
		futSetLastError(FUTERR_FILE_INCORRECT_TYPE, *a1);
		return 0;
	}
	a3->clear();
	std::string kSeparator2("//");
	if (!FutStreamPairParser(&kStream, a3, &kSeparator2, 0)) {
		futSetLastError(FUTERR_FILE_NOT_READABLE, *a1);
		return 0;
	}
	return 1;
}

// The exe's cleanup-only C++ EH frame is left out. The ifstream is a complete object, the class part with its
// basic_ios virtual base right after it (+0x58): constructed with aiMostDerived = 1, destroyed by the class's
// Destruct, which the exe enters with the ios subobject, then by ~basic_ios on the ios. kUnused is a string the exe
// default-constructs (inlined) and destroys on every return without using it.
// The exe's share flag (_SH_DENYNO) has no standard form; resize(0, empty pair) is clear().
char FutFilePairParser(std::basic_string<char, std::char_traits<char>, std::allocator<char>>* a1,
	const std::basic_string<char, std::char_traits<char>, std::allocator<char>>* arg4,
	std::vector<std::pair<std::string, std::string>, std::allocator<std::pair<std::string, std::string>>>*
		a3)
{
	std::ifstream kStream(a1->c_str(), std::ios_base::in);
	if (kStream.fail()) {
		futSetLastError(FUTERR_FILE_NOT_READABLE, *a1);
		return 0;
	}
	a3->clear();
	std::string kSeparator("//");
	if (!FutStreamPairParserVec(&kStream, a3, &kSeparator, 1)) {
		futSetLastError(FUTERR_FILE_NOT_READABLE, *a1);
		return 0;
	}
	// The first pair's name must be arg4. A file with no pair goes to operator[]'s range check in the exe
	// (_invalid_parameter_noinfo); the standalone build reports it through at() (std::out_of_range).
	if (a3->at(0).first.compare(*arg4)) {
		futSetLastError(FUTERR_FILE_INCORRECT_TYPE, *a1);
		return 0;
	}
	a3->clear();
	std::string kSeparator2("//");
	if (!FutStreamPairParserVec(&kStream, a3, &kSeparator2, 0)) {
		futSetLastError(FUTERR_FILE_NOT_READABLE, *a1);
		return 0;
	}
	return 1;
}

// The exe's cleanup-only C++ EH frame is left out. The ifstream is a complete object, the class part with its
// basic_ios virtual base right after it (+0x58): constructed with aiMostDerived = 1, destroyed by the class's
// Destruct, which the exe enters with the ios subobject, then by ~basic_ios on the ios (inlined on the success path).
// Each line is split at the commas outside double quotes; a line whose fields are all blanks and tabs is dropped.
// The exe's share flag (_SH_DENYNO) has no standard form.
char futLoadCsvFile(std::basic_string<char, std::char_traits<char>, std::allocator<char>>* a1,
	std::vector<std::vector<std::string, std::allocator<std::string>>,
		std::allocator<std::vector<std::string, std::allocator<std::string>>>>* arg4)
{
	using StrVector = std::vector<std::string, std::allocator<std::string>>;
	StrVector kRow;
	arg4->clear();
	if (!fr2CheckFileExists(*a1)) {
		futSetLastError(FUTERR_FILE_NOT_FOUND, *a1);
		return 0;
	}
	std::ifstream kStream(a1->c_str(), std::ios_base::in);
	if (kStream.fail()) {
		futSetLastError(FUTERR_FILE_NOT_READABLE, *a1);
		return 0;
	}
	while (!kStream.eof() && !kStream.fail()) {
		std::string kLine;
		std::getline(kStream, kLine, kStream.widen('\n'));
		if (!kStream.fail()) {
			const char* pCursor = kLine.c_str();
			bool bBlank = true;
			kRow.clear();
			while (*pCursor != '\n' && *pCursor) {
				std::string kField;
				bool bQuoted = false;
				if (*pCursor == '"') {
					bQuoted = true;
					++pCursor;
				}
				while ((*pCursor != ',' || bQuoted) && (*pCursor && *pCursor != '\n')) {
					if (*pCursor == '"') {
						bQuoted = !bQuoted;
					} else {
						if (*pCursor != ' ' && *pCursor != '\t') {
							bBlank = false;
						}
						kField.push_back(*pCursor);
					}
					++pCursor;
				}
				if (*pCursor) {
					++pCursor;
				}
				kRow.push_back(kField);
			}
			if (!bBlank) {
				arg4->push_back(kRow);
			}
		}
	}
	if (kStream.eof() || !kStream.fail()) {
		return 1;
	}
	futSetLastError(FUTERR_FILE_CORRUPT, *a1);
	return 0;
}

// Writes arg4's rows to the file a1 names as CSV: each cell quoted, cells separated by commas, each row ended by a
// newline and two flushes. The ofstream local is a complete object (ofstream_complete), which the exe builds with the
// constructor's aiMostDerived = 1 and tears down with ~basic_ofstream (0xACDEB0) on its ios subobject, then
// ~basic_ios (0xACA4B0) there. The sizes are size(): the exe divides each byte span with idiv (0x18 for the rows,
// 0x1C for a row's strings). The exe's cleanup-only C++ EH frame is left out.
// The exe's share flag (_SH_DENYNO) has no standard form; ofstream's close() sets failbit when the file buffer's
// close fails, as the hybrid body does by hand.
char futSaveCsvFile(std::basic_string<char, std::char_traits<char>, std::allocator<char>>* a1, std::vector<std::vector<std::basic_string<char, std::char_traits<char>, std::allocator<char>>, std::allocator<std::basic_string<char, std::char_traits<char>, std::allocator<char>>>>, std::allocator<std::vector<std::basic_string<char, std::char_traits<char>, std::allocator<char>>, std::allocator<std::basic_string<char, std::char_traits<char>, std::allocator<char>>>>>>* arg4)
{
	if (!arg4->size()) {
		futSetLastError(FUTERR_SEE_TEXT, "CSV input is empty.");
		return 0;
	}
	std::ofstream kStream(a1->c_str(), std::ios_base::out);
	if (kStream.fail()) {
		futSetLastError(FUTERR_FILE_NOT_WRITEABLE, *a1);
		return 0;
	}
	for (uint32_t i = 0; i < arg4->size(); ++i) {
		uint32_t v24 = static_cast<uint32_t>((*arg4)[i].size());
		for (uint32_t j = 0; j < v24; ++j) {
			kStream << "\"" << (*arg4)[i][j] << "\"";
			if (j == v24 - 1) {
				kStream.put('\n');
				kStream.flush();
				kStream.flush();
			} else {
				kStream << ",";
			}
		}
		if (kStream.fail()) {
			futSetLastError(FUTERR_FILE_WRITE_ERROR, *a1);
			kStream.close();
			return 0;
		}
	}
	kStream.close();
	return 1;
}

uint8_t* UninitFillN(uint8_t* a1, size_t a2, const uint8_t* a3) {
	for (size_t i = 0; i < a2; ++i) {
		a1[i] = *a3;
	}
	return &a1[a2];
}

// KEEP, unregistered (an uninitialised read): VC9's _Copy_opt for chars. The exe copies never-written tag bytes of its
// frame ([ebp-4] into [ebp-2] at 0xAD511B, [ebp-3] into [ebp-0x21] at 0xAD5133).
char* CopyOpt(const char* _First, const char* _Last, char* _Dest)
{
	ptrdiff_t _Count = _Last - _First;
	char* _Result = _Dest + _Count;
	if (_Count > 0) {
		memmove_s(_Dest, static_cast<size_t>(_Count), _First, static_cast<size_t>(_Count));
	}
	return _Result;
}

// KEEP, unregistered (an uninitialised read): VC9's _Copy_opt over vector<string>. The exe copies never-written tag
// bytes of its frame ([ebp-4] into [ebp-2] at 0xAD518E, [ebp-3] into [ebp-0x165] at 0xAD51A9). It computes the result
// first (cdq; idiv 0x18 on the byte distance: truncated, not an exact pointer subtraction); each element goes
// through vector<string>'s operator= (0xAD61E0).
StrVector* CopyOpt(const StrVector* _First, const StrVector* _Last, StrVector* _Dest)
{
	StrVector* _Result = _Dest + (_Last - _First);
	for (; _First != _Last; ++_First, ++_Dest) {
		*_Dest = *_First;
	}
	return _Result;
}

// KEEP, unregistered (an uninitialised read): VC9's _Move_backward over strings with the swap tag. The exe copies
// never-written tag bytes of its own frame ([ebp-4] into [ebp-2] at 0xAD525E, [ebp-3] into [ebp-0xBD] at 0xAD5279).
// The result is computed before the loop: _Dest less the signed division of the raw byte distance (cdq; idiv 0x1C;
// the pointers read as plain addresses, which a pointer subtraction would assume to be an exact multiple); each step
// swaps through string::swap (0xAD6BB0).
std::string* UncheckedMoveBackward(std::string* _First, std::string* _Last, std::string* _Dest)
{
	std::string* _Result = _Dest - static_cast<ptrdiff_t>(
		reinterpret_cast<uintptr_t>(_Last) - reinterpret_cast<uintptr_t>(_First))
		/ static_cast<ptrdiff_t>(sizeof(std::string));
	while (_First != _Last) {
		--_Last;
		--_Dest;
		_Dest->swap(*_Last);
	}
	return _Result;
}

// <xmemory>'s _Allocate of strings, out of line.
std::string* _Allocate(uint32_t _Count)
{
	XFNV_INLINED_LOCAL(rt::bad_alloc, kException, std::nullopt, std::nullopt);
	if (_Count) {
		if (static_cast<size_t>(-1) / _Count < sizeof(std::string)) {
			// EXE-INLINED 0x44D6C0
			const char* pMessage = nullptr;
			// Natively bad_alloc's own constructor, which stores its own vtable.
			::new (static_cast<void*>(&kException)) rt::bad_alloc(pMessage);
			throw kException;
		}
	} else {
		_Count = 0;
	}
	return static_cast<std::string*>(ourMalloc(sizeof(std::string) * _Count));
}
