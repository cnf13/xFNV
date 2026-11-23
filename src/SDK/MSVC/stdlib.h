#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <random>

// The Microsoft-only functions of VC9's <stdlib.h> the game calls, under their own names, and the functions whose
// behaviour the game relies on where the C libraries differ (rt). The standard functions are std:: (<cstdlib>).
//
// The Windows CRT declares and defines every one of the Microsoft-only functions itself (in its <stdlib.h>), with the
// contract written here: a second definition would be a second definition of the CRT's own C function. So on Windows
// the CRT's are used, and elsewhere they are declared here and stdlib.cpp defines them to VC9's rules.
#if !defined(_WIN32)

// _gcvt, 0xECDD16 (IDA's ::gcvt): _gcvt_s into an unbounded apBuffer: adValue with aiDigits significant digits in
// VC9's Fortran G form (fixed, or "d.ddde+ddd" outside 0.1 to 10^aiDigits), trailing zeros dropped but not the point
// ("1."). apBuffer, or null on an error.
char* _gcvt(double adValue, int aiDigits, char* apBuffer);

// _splitpath_s, 0xEC7F1E: splits apPath into drive ("X:"), directory (through the last '/' or '\'), file name and
// extension (from the last '.' after the directory); a null buffer with size 0 skips that part. 0, or EINVAL (a null
// path, or a buffer and its size not both given) / ERANGE (a part does not fit), with every buffer given emptied.
int _splitpath_s(
    const char* apPath,
    char* apDrive,
    size_t astDriveSize,
    char* apDir,
    size_t astDirSize,
    char* apFName,
    size_t astFNameSize,
    char* apExt,
    size_t astExtSize
);

// _makepath_s, 0xEC825B: joins apDrive (its first character and ':'), apDir (a '\' added unless it ends in '/' or
// '\'), apFName and apExt (a '.' added unless it starts with one) into apBuffer; 0, or ERANGE (emptied) when the
// result with its terminator does not fit in astBufferSize.
int _makepath_s(
    char* apBuffer,
    size_t astBufferSize,
    const char* apDrive,
    const char* apDir,
    const char* apFName,
    const char* apExt
);

#endif

// rt: runtime support the game relies on.
namespace rt {

    // VC9's RAND_MAX. The game's code divides rand's results by 32767 and masks them with 0x7FFF: callers rely on the
    // range, not on VC9's sequence.
    constexpr int32_t kRandMax = 32767;

    // rand's engine, one per thread as VC9's CRT keeps its seed per thread;
    // an unseeded thread starts as srand(1) leaves it (C's rule), so runs repeat.
    inline thread_local std::minstd_rand kRandEngine{1};

    // Not reconstructed yet: rand, 0xECADB8. A uniform draw in [0, kRandMax] from this thread's standard engine, not
    // the C library's rand, whose range is not VC9's everywhere (glibc's goes to 2^31 - 1: grass heights went to
    // millions of units).
    inline int32_t rand() {
        return std::uniform_int_distribution<int32_t>(0, kRandMax)(kRandEngine);
    }

    // Not reconstructed yet: srand, 0xECADA6. Reseeds this thread's rand engine.
    inline void srand(uint32_t auiSeed) {
        kRandEngine.seed(auiSeed);
    }

    // wcstombs, 0xECACB2 (_wcstombs_l_helper in the C locale) on the game's 2-byte characters (VC9's wchar_t: char16_t
    // here): each character below 256 is one byte; at most astMaxCount bytes, the terminator included when it fits; the
    // bytes written before it, or the length a null apDest would need. -1 with errno EILSEQ for a character above 255.
    // Defined in stdlib.cpp.
    size_t wcstombs(char* apDest, const char16_t* apSource, size_t astMaxCount);

    // wcstombs_s, 0xECAD86 (cdecl; returns errno_t): wcstombs into a buffer of astSizeInBytes, always terminated; the
    // bytes with the terminator to *apReturnValue. A text that does not fit is ERANGE (emptied), or with astMaxCount
    // kTRUNCATE cut and STRUNCATE returned. Defined in stdlib.cpp.
    int32_t wcstombs_s(
        size_t* apReturnValue,
        char* apDest,
        size_t astSizeInBytes,
        const char16_t* apSource,
        size_t astMaxCount
    );

} // namespace rt
