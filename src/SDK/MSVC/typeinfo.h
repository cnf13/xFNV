#pragma once

#include "SDK/MSVC/ehdata.h"
#include "SDK/MSVC/exception.h"

// rt: runtime support the game relies on. Here the VC9 CRT's bad_cast (<typeinfo>), with rt::exception's layout.
namespace rt {

    // The CRT's std::bad_cast (IDA: std::bad_cast): an exception with no members of its own and its own vtable.
    class bad_cast : public exception {
    public:
        // The exception with _Message's text.
        explicit bad_cast(const char* _Message) : exception(_Message) {}
    };

    // IDA name _TI2?AVbad_cast@std@@; not in the PDB. The compiler's ThrowInfo for std::bad_cast, which the exe's
    // throws of it pass to _CxxThrowException.
    extern _s__ThrowInfo bad_cast_ThrowInfo;

} // namespace rt
