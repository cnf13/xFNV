#pragma once

// ATL: the exe's VC9 ATL statreg.h (the registrar's script parser), the part the exe has: the registry-script
// keywords the header declares `const LPCTSTR` at namespace scope (ANSI build: `const char* const`). Being const they
// are internal to each file, so every file that includes the header has its own eight pointers in the exe's .rdata,
// pointing to the shared literals; the exe has four such copies (0x107CB28, 0x107CBA0, 0x1093D50, 0x1093EE4), which no
// code reads. The Xbox PDB has no ATL module to confirm them by name.
namespace ATL {

    static const char* const szStringVal = "S";
    static const char* const multiszStringVal = "M";
    static const char* const szDwordVal = "D";
    static const char* const szBinaryVal = "B";
    static const char* const szValToken = "Val";
    static const char* const szForceRemove = "ForceRemove";
    static const char* const szNoRemove = "NoRemove";
    static const char* const szDelete = "Delete";

} // namespace ATL
