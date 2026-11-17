#pragma once

// The FaceGen SDK's string containers (StrContainers.cpp): the element types under one name each and the free
// out-of-line helpers of their VC9 instances, declared here so that other files' bodies can call them and the
// bodies of the helpers themselves can be defined with qualified names.

#include <functional>
#include <map>
#include <memory>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

// The element types and the map's traits under one name each: a macro argument can't hold the templates' commas.
using StrPair = std::pair<std::string, std::string>;
using StrVector = std::vector<std::string, std::allocator<std::string>>;

// pair<string, string>'s destructor.
void _StrPair_Destruct(StrPair* apThis, uint32_t auiFlags);
// Not named in the PDB or IDA; names chosen here. stdext::unchecked_uninitialized_copy (cdecl, 0xAD6B10 / 0xAD6B60).
// Inferred type: return: std::pair<std::basic_string<char, std::char_traits<char>, std::allocator<char>>,
// std::basic_string<char, std::char_traits<char>, std::allocator<char>>>* from 0xad6b4f (result of
// std::_Uninit_copy<std::pair<std::basic_string<char, std::char_traits<char>, std::allocator<char>>,
// std::basic_string<char, std::char_traits<char>, std::allocator<char>>>>), its members used at 0xad1e7c
StrPair* _StrPair_UninitCopy(StrPair* _First, StrPair* _Last, StrPair* _Dest, std::allocator<StrPair>& _Al);
StrVector* _StrVector_UninitCopy(StrVector* _First, StrVector* _Last, StrVector* _Dest, std::allocator<StrVector>& _Al);
// Not named in the PDB or IDA; names chosen here. stdext::unchecked_uninitialized_move (cdecl, 0xAD5F80 / 0xAD6040).
StrPair* _StrPair_UninitMove(StrPair* _First, StrPair* _Last, StrPair* _Dest, std::allocator<StrPair>& _Al);
StrVector* _StrVector_UninitMove(StrVector* _First, StrVector* _Last, StrVector* _Dest, std::allocator<StrVector>& _Al);
// Not named in the PDB or IDA; names chosen here. The in-place backward move (cdecl, ret 0, 0xAD6000 / 0xAD60C0),
// VC9's _Move_backward_opt: _IterCat is the iterator category tag's byte, not the allocator.
void _StrPair_MoveBackward(
    StrPair* _First,
    StrPair* _Last,
    StrPair* _Dest,
    uint8_t _IterCat,
    uint8_t _Cat,
    uint8_t _Checked
);
void _StrVector_MoveBackward(
    StrVector* _First,
    StrVector* _Last,
    StrVector* _Dest,
    uint8_t _IterCat,
    uint8_t _Cat,
    uint8_t _Checked
);
// Not named in the PDB or IDA; names chosen here. operator= of the pair and the string vector (thiscall, ret 4,
// 0xAD6190 / 0xAD61E0); the vector's returns *apThis in eax.
void _StrPair_Assign(StrPair* apThis, const StrPair& arVal);
StrVector& _StrVector_Assign(StrVector* apThis, const StrVector& arVal);
// Destroys the vector<string> elements [_First, _Last).
void _StrVector_DestroyRange(
    std::vector<StrVector, std::allocator<StrVector>>* apThis,
    StrVector* _First,
    StrVector* _Last
);

std::basic_string<char, std::char_traits<char>, std::allocator<char>>* UncheckedCopyBackward(
    std::basic_string<char, std::char_traits<char>, std::allocator<char>>* a1,
    std::basic_string<char, std::char_traits<char>, std::allocator<char>>* a2,
    std::basic_string<char, std::char_traits<char>, std::allocator<char>>* a3
);

// Not named in the PDB or IDA; name chosen here. VC9's _Copy_backward_opt over pair<string, string> (cdecl, ret 0):
// each pair of [a1, arg4), last first, assigned to the pair before the previous one from a3, returning a3 - (arg4 -
// a1). The exe's asm steps 0x38-byte pairs back (0xAD685B) and assigns the strings at +0 and +0x1C.
std::pair<std::string, std::string>* CopyBackwardOpt(
    std::pair<std::string, std::string>* a1,
    std::pair<std::string, std::string>* arg4,
    std::pair<std::string, std::string>* a3
);
