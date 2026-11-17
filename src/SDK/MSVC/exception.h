#pragma once

#include "SDK/MSVC/ehdata.h"

// rt: runtime support the game relies on. Here the VC9 CRT's exception classes, statically linked: exception
// (<exception>) and bad_alloc (<new>), with the exe's layouts, thrown and caught by name; std's classes have neither
// that layout nor exception's constructor from a text. <stdexcept>'s runtime_error is in stdexcept.h.
namespace rt {

    // The CRT's std::exception (IDA: std::exception; the PDB has no entry for it). The what() text is held by pointer
    // and freed by the destructor when _m_doFree is set. Real C++ constructors and destructor, not static
    // Construct/Destruct: a C++ `throw` needs C++ constructors and destructors in the ThrowInfo (the user's choice of
    // real C++ EH, 2026-11-04; Main's ruling for exception, runtime_error and IdvFileError only). Each writes exactly
    // its exe level, the exe's vftable included.
    class exception {
    public:
        // No text.
        exception();
        // A malloc'd copy of _What's text, which the destructor frees.
        explicit exception(const char* const& _What);
        // The copy: a malloc'd copy of the text when _That owns its text, else the same pointer.
        exception(const exception& _That);
        // Frees the text when it is owned.
        virtual ~exception();
        // The text, or "Unknown exception" without one.
        virtual const char* what() const;

        const char* _m_what;
        int32_t _m_doFree;
    };

    // Not named in the PDB or IDA; name chosen here. The text what() returns for an exception without one; the exe
    // returns this address.
    using UnknownExceptionText = const char[18];
    extern UnknownExceptionText exception_UnknownText;

    // The CRT's std::bad_alloc (IDA: std::bad_alloc; the PDB lists its members, not its layout): an exception with no
    // members of its own and its own vtable.
    class bad_alloc : public exception {
    public:
    public:
        static rt::bad_alloc* DeletingDestruct(rt::bad_alloc* apThis, uint32_t auiFlags);
        // The native constructors Construct stands for: exception(const char* const&) with _Message.
        bad_alloc() = default;
        explicit bad_alloc(const char* _Message) : exception(_Message) {}
        // exception's copy.
        bad_alloc(const bad_alloc& _That);
        ~bad_alloc() override;
    };

    // IDA name ::TI2?AVbad_alloc@std@@; not in the PDB. The compiler's ThrowInfo for bad_alloc (its unwind 0x44D6F0,
    // its catchable types at 0x1140F08), which the exe's allocation failure paths pass to _CxxThrowException
    // (0xEC7894).
    extern _s__ThrowInfo TI2_AVbad_alloc_std;

} // namespace rt
