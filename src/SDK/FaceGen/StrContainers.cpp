// The FaceGen SDK's string containers: the exe's out-of-line VC9 instances of map<string, string>,
// vector<pair<string, string>> and vector<vector<string>>. None of their addresses has a PDB name, so the source file
// the PDB would name is not known; the bodies are written, and the reconstructed ones registered, here, over the
// standard containers; the free helpers are declared in StrContainers.h.

#include "SDK/FaceGen/StrContainers.h"
#include <functional>
#include <map>
#include <memory>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>
#include "BSCore/MemoryManager.h"
#include "SDK/MSVC/crtdefs.h"
#include "SDK/MSVC/ehdata.h"
#include "SDK/MSVC/exception.h"
#include "SDK/MSVC/iosfwd.h"
#include "SDK/MSVC/stdexcept.h"

// The exe's own instances of VC9 library code, hybrid only: the standalone build uses the standard library's.
// vector<string>'s operator=, which algs.cpp's element copies (CopyOpt) call: the standard library's.
StrVector& _StrVector_Assign(StrVector* apThis, const StrVector& arVal) {
    *apThis = arVal;
    return *apThis;
}
using vector_pair_basic_string_char_basic_string_char = std::vector<
    std::pair<
        std::basic_string<char, std::char_traits<char>, std::allocator<char>>,
        std::basic_string<char, std::char_traits<char>, std::allocator<char>>>,
    std::allocator<std::pair<
        std::basic_string<char, std::char_traits<char>, std::allocator<char>>,
        std::basic_string<char, std::char_traits<char>, std::allocator<char>>>>>;
using vector_vector_basic_string_char = std::vector<
    std::vector<
        std::basic_string<char, std::char_traits<char>, std::allocator<char>>,
        std::allocator<std::basic_string<char, std::char_traits<char>, std::allocator<char>>>>,
    std::allocator<std::vector<
        std::basic_string<char, std::char_traits<char>, std::allocator<char>>,
        std::allocator<std::basic_string<char, std::char_traits<char>, std::allocator<char>>>>>>;
