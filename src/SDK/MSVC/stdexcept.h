#pragma once

#include <stdexcept>
#include <stdint.h>
#include <string>
#include "SDK/MSVC/ehdata.h"
#include "SDK/MSVC/exception.h"
#include "SDK/MSVC/iosfwd.h"

// rt: runtime support the game relies on. Here the VC9 CRT's runtime_error (<stdexcept>, statically linked) with the
// exe's layout: the base of SpeedTree's IdvFileError. logic_error and the classes derived from it are std's.
namespace rt {

    // The CRT's std::runtime_error (PDB: std::runtime_error; IDA: std::runtime_error): an exception holding its message
    // as a string at +0xC, as logic_error does. Vtable slots: the scalar deleting destructor (0xACA220) and
    // runtime_error's what (0xACA1E0, the string's characters). Real C++ constructors and destructor, as exception's
    // (exception.h): a C++ `throw` needs them in the ThrowInfo.
    class runtime_error : public exception {
    public:
        // The exception constructor, runtime_error's vftable, then the string emptied and assigned from _Message
        // (thiscall, ret 4, 0xB096A0).
        explicit runtime_error(const std::string& _Message);
        // The exception copy, runtime_error's vftable, then the string copy (thiscall, ret 4, 0xACA270).
        runtime_error(const runtime_error& _That);
        // runtime_error's vftable, the string freed, then ~exception. Inline (defined below the class): the exe has no
        // out-of-line copy of its own, it is inlined into ~IdvFileError (0xB09780) and the deleting destructors
        // (IdvFileError's 0xB09730), so an out-of-line src copy would be a call the exe does not make.
        ~runtime_error() override;
        // what, vtable slot 1: _Str's characters.
        const char* what() const override;

        std::string _Str;

    public:
        // The scalar deleting destructor (thiscall, ret 4; 0xACA220): ~runtime_error inlined, then ourFree.
        static rt::runtime_error* DeletingDestruct(rt::runtime_error* apThis, uint32_t auiFlags);
    };

    // IDA name _TI2?AVruntime_error@std@@; not in the PDB. The compiler's ThrowInfo for runtime_error, which the exe's
    // throws of it pass to _CxxThrowException.
    extern _s__ThrowInfo runtime_error_ThrowInfo;

    inline runtime_error::~runtime_error() {}

} // namespace rt
