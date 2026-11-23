#pragma once
// The crash log of the programs without the exe (CMake option XFNV_CRASH_LOG): what the platform's capture side
// (CaptureWindows.cpp: the exception filter, the threads, the stacks, the file; CapturePosix.cpp: the same on
// signals), the compiler ABI's RTTI side (RttiMsvc.cpp, RttiItanium.cpp) and the shared report (Out.cpp, Report.cpp,
// Annotate.cpp: what a value points to; Identity.cpp: by src's types) share. CMake picks one file of each pair.
// Everything here runs in a broken process: no heap, static buffers only, and every read of memory goes through
// readMemory.

#include <cstddef>
#include <cstdint>

namespace xfnv::crashlog {

    constexpr uint32_t kFaultScanWords = 16384;
    constexpr uint32_t kThreadScanWords = 4096;
    constexpr int32_t kMaxFrames = 64;
    constexpr uintptr_t kWord = sizeof(uintptr_t);

    // The log's text, gathered in a static buffer and handed to writeOut whenever it fills.
    class Out {
    public:
        Out& str(const char* apText);
        // At most auiMax characters of apText, which may be unreadable (copied through readMemory first).
        Out& guardedStr(uintptr_t apText, uint32_t auiMax);
        Out& hex(uint64_t auiValue, int32_t aiDigits = 8);
        Out& dec(int64_t aiValue);
        Out& chr(char acChar);
        Out& nl() {
            return chr('\n');
        }
        void flush();

    private:
        char acBuffer[16384];
        uint32_t uiUsed = 0;
    };

    // The platform's side (CaptureWindows.cpp, CapturePosix.cpp).
    // Copies auiSize bytes at apSource into apDest; false (and nothing faults) when any of them is unreadable.
    bool readMemory(void* apDest, uintptr_t apSource, size_t auiSize);
    // Writes the log's bytes to the open log file.
    void writeOut(const char* apText, uint32_t auiSize);
    // Whether auiAddress lies in executable code of a loaded image.
    bool isCode(uintptr_t auiAddress);
    // The code address as the log names it: "module+0xOFFSET", or a symbol where the platform resolves one in-process.
    void codeAddress(Out& arOut, uintptr_t auiAddress);

    // The program's own image: vtables and RTTI records are trusted only inside it.
    struct Image {
        uintptr_t uiBegin;
        uintptr_t uiEnd;
        bool Contains(uintptr_t auiAddress, size_t auiSize = 1) const {
            return auiAddress >= uiBegin && auiAddress < uiEnd && auiSize <= uiEnd - auiAddress;
        }
    };
    extern Image kImage;

    // The compiler ABI's side (RttiMsvc.cpp, RttiItanium.cpp): an object whose first word is a vtable of the image.
    struct Dynamic {
        // The complete object's address
        uintptr_t uiComplete;
        // The ABI's record of its class (MSVC: the class hierarchy descriptor; Itanium: the type_info)
        uintptr_t uiType;
        // The class's name as the ABI decorates it (".?AVNiNode@@", "6NiNode")
        char acName[160];
    };
    // The dynamic class of the object at auiValue; false unless its first word is a vtable of the image.
    bool dynamicType(uintptr_t auiValue, Dynamic& arOut);
    // The class of the vtable at auiVtable itself (uiComplete is not set); false unless it is one.
    bool vtableType(uintptr_t auiVtable, Dynamic& arOut);
    // Where the base class apClass (a class at namespace scope: "TESForm") of arObject sits; false when it has none, or
    // only virtually.
    bool findBase(const Dynamic& arObject, const char* apClass, uintptr_t& arBase);
    // The class's name as C++ spells it ("Tile::Extra"); names the ABI's form does not reduce simply are printed as
    // they are.
    void className(Out& arOut, const Dynamic& arObject);

    // Identity.cpp: by src's own types.
    // The identity src's types give the object: a few fields after its class name.
    void identity(Out& arOut, const Dynamic& arObject);
    // When auiValue points to the game's thread object (a BSThread) of thread auiThreadId: prints its class after the
    // thread's heading and returns true.
    bool threadObject(Out& arOut, uintptr_t auiValue, uint32_t auiThreadId);

    // Annotate.cpp.
    // What a data pointer points to, on the rest of the current line (and, for an object with an identity, on
    // indented lines after it): an object's dynamic class by its vtable's RTTI with its first words and identity, else
    // a string, else the first words. Prints nothing for an unreadable address.
    void annotateData(Out& arOut, uintptr_t auiValue);
    // What a register or stack word holds, after it on the line: code, a pointer into the thread's own stack, or data.
    void annotateValue(Out& arOut, uintptr_t auiValue, uintptr_t auiStackLow, uintptr_t auiStackHigh);

    // Report.cpp: the parts of a thread's section both platforms print alike.
    struct Register {
        const char* pName;
        uintptr_t uiValue;
    };
    // One line per register, its value annotated.
    void
    registers(Out& arOut, const Register* apRegisters, size_t auiCount, uintptr_t auiStackLow, uintptr_t auiStackHigh);
    // Every word from the stack pointer up to the stack's top that is code or points to readable memory.
    void scan(Out& arOut, uintptr_t auiStackLow, uintptr_t auiStackHigh, uint32_t auiMaxWords);
    // The game's thread object among the stack's first words (its start routine's argument), printed after the heading.
    void threadClass(Out& arOut, uintptr_t auiStackLow, uintptr_t auiStackHigh, uint32_t auiThreadId);
    struct DateTime {
        uint16_t uiYear;
        uint16_t uiMonth;
        uint16_t uiDay;
        uint16_t uiHour;
        uint16_t uiMinute;
        uint16_t uiSecond;
    };
    // "2026-11-15 15:02:03"
    void printTime(Out& arOut, const DateTime& arTime);

} // namespace xfnv::crashlog
