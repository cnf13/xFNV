#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/crtdefs.h"
#include "SDK/MSVC/direct.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/errno.h"
#include "SDK/MSVC/io.h"
#include "SDK/MSVC/malloc.h"
#include "SDK/MSVC/mbstring.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdio.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"
#include "SDK/MSVC/tchar.h"
#include "SDK/MSVC/time.h"
#include "SDK/Windows/winbase.h"
#include "SDK/Windows/winsock2.h"

#include <cctype>
#include <cmath>
#include <cstring>
#include <new>

// A custom I/O procedure of an MMIOINFO (LPMMIOPROC). Windows declares lpmmioinfo as LPSTR; it is the MMIOINFO the
// procedure works on. lParam1 and lParam2 are LPARAMs and the result an LRESULT: pointer-sized (MMIOM_OPEN passes the
// file name in lParam1; winmm reads the whole result).
struct _MMIOINFO;
// XOVERLAPPED: the state of an asynchronous operation the Xbox 360 XDK's calls take. XGamerProfile keeps one as
// m_EventData.
struct _XOVERLAPPED;
// WINMM.dll: the multimedia file I/O BSWin32GameSound::Open reads wave files with. HMMIO is an opaque handle;
// MMRESULT is a UINT.
struct HMMIO__;

// Win32 imports. ole32.dll's CoInitialize (HRESULT result).
using CoInitialize_t = int32_t(WINAPI*)(void* pvReserved);
XFNV_IMPORT(CoInitialize_t, CoInitialize, 0xFDF454);
// ole32.dll's CoInitializeEx (HRESULT result).
using CoInitializeEx_t = int32_t(WINAPI*)(void* pvReserved, uint32_t dwCoInit);
XFNV_IMPORT(CoInitializeEx_t, CoInitializeEx, 0xFDF458);
// ole32.dll's CoCreateInstance (HRESULT result).
using CoCreateInstance_t =
    int32_t(WINAPI*)(const GUID* rclsid, IUnknown* pUnkOuter, uint32_t dwClsContext, const GUID* riid, void** ppv);
XFNV_IMPORT(CoCreateInstance_t, CoCreateInstance, 0xFDF46C);
// ole32.dll's CoUninitialize.
using CoUninitialize_t = void(WINAPI*)();
XFNV_IMPORT(CoUninitialize_t, CoUninitialize, 0xFDF464);
// ole32.dll's CoFreeUnusedLibraries.
using CoFreeUnusedLibraries_t = void(WINAPI*)();
XFNV_IMPORT(CoFreeUnusedLibraries_t, CoFreeUnusedLibraries, 0xFDF468);
// ULARGE_INTEGER: a 64-bit value and its two halves, LowPart first. TESFile::MastersChanged compares each master file's
// LowPart and HighPart with WIN32_FIND_DATAA's nFileSizeLow and nFileSizeHigh.
union _ULARGE_INTEGER {
    struct {
        uint32_t LowPart;
        uint32_t HighPart;
    } u;

    uint64_t QuadPart;
};
// XINPUT1_3.dll, imported by ordinal 2. The SDK's controller state (16 bytes).
struct _XINPUT_GAMEPAD {
    uint16_t wButtons;
    uint8_t bLeftTrigger;
    uint8_t bRightTrigger;
    int16_t sThumbLX;
    int16_t sThumbLY;
    int16_t sThumbRX;
    int16_t sThumbRY;
};
struct _XINPUT_STATE {
    uint32_t dwPacketNumber;
    _XINPUT_GAMEPAD Gamepad;
};
// The SDK's motor speeds XInputSetState takes (4 bytes).
struct _XINPUT_VIBRATION {
    uint16_t wLeftMotorSpeed;
    uint16_t wRightMotorSpeed;
};

// Formats into pcDest (at most stDestSize bytes, always terminated) with vsnprintf; returns its result, or 1 when
// stDestSize is 0, as the game does. C varargs pass floats as double.
int32_t NiSprintf(char* pcDest, size_t stDestSize, const char* pcFormat, ...);
// Interlocked increment and decrement; each returns the new value.
int32_t NiAtomicIncrement(int32_t& aiValue);
uint32_t NiAtomicIncrement(uint32_t& auiValue);
int32_t NiAtomicDecrement(int32_t& aiValue);
uint32_t NiAtomicDecrement(uint32_t& auiValue);
// Case-insensitive compare (the CRT's _stricmp).
int32_t NiStricmp(const char* s1, const char* s2);
// strcpy_s into pcDest of stDestSize bytes.
char* NiStrcpy(char* pcDest, size_t stDestSize, const char* pcSrc);
// strncpy_s of at most stCount characters of pcSrc into pcDest of stDestSize bytes; returns pcDest.
char* NiStrncpy(char* pcDest, size_t stDestSize, const char* pcSrc, size_t stCount);
uint32_t XInputGetState(uint32_t dwUserIndex, _XINPUT_STATE* pXInputState);
int32_t NiStrnicmp(const char* s1, const char* s2, uint32_t n);
float NiGetCurrentTimeInSec();

using XOverlappedCompletionRoutine =
    void (*)(uint32_t dwErrorCode, uint32_t dwNumberOfBytesTransferred, _XOVERLAPPED* lpOverlapped);
struct _XOVERLAPPED {
    uint32_t InternalLow;
    uint32_t InternalHigh;
    uint32_t InternalContext;
    void* hEvent;
    XOverlappedCompletionRoutine pCompletionRoutine;
    uint32_t dwCompletionContext;
    uint32_t dwExtendedError;
};
// The XInputGetState export of XINPUT1_3.dll, imported by ordinal 2.
using XInputGetState_t = uint32_t(WINAPI*)(uint32_t dwUserIndex, _XINPUT_STATE* pState);
namespace xfnv::imports {
    XFNV_IMPORT(XInputGetState_t, XInputGetState, 0xFDF394);
} // namespace xfnv::imports
// XInputSetState from XINPUT1_3.dll, imported by ordinal 3.
using imp_XInputSetState_t = uint32_t(WINAPI*)(uint32_t dwUserIndex, _XINPUT_VIBRATION* pVibration);
namespace xfnv::imports {
    XFNV_IMPORT(imp_XInputSetState_t, XInputSetState, 0xFDF398);
} // namespace xfnv::imports
XFNV_IMPORT_AS(imp_XInputSetState_t, imp_XInputSetState, 0xFDF398, xfnv::imports::XInputSetState);
uint32_t XInputSetState(uint32_t dwUserIndex, _XINPUT_VIBRATION* pVibration);
// ole32.dll CoTaskMemFree (LPVOID).
using CoTaskMemFree_t = void(WINAPI*)(void* pv);
XFNV_IMPORT(CoTaskMemFree_t, CoTaskMemFree, 0xFDF45C);
// ole32.dll CoTaskMemAlloc (SIZE_T).
using CoTaskMemAlloc_t = void*(WINAPI*)(size_t cb);
XFNV_IMPORT(CoTaskMemAlloc_t, CoTaskMemAlloc, 0xFDF460);
// The SDK's OVERLAPPED (0x14 bytes): the state of an asynchronous file operation. Internal and InternalHigh are
// ULONG_PTR; Offset and OffsetHigh share their words with Pointer.
struct _OVERLAPPED {
    uintptr_t Internal;
    uintptr_t InternalHigh;

    union {
        struct {
            uint32_t Offset;
            uint32_t OffsetHigh;
        } s;

        void* Pointer;
    } u;

    void* hEvent;
};
// The SDK's SECURITY_ATTRIBUTES (0xC bytes), CreateFileA's lpSecurityAttributes.
struct _SECURITY_ATTRIBUTES {
    uint32_t nLength;
    void* lpSecurityDescriptor;
    int32_t bInheritHandle;
};
// KERNEL32.dll file reads (HANDLE is void*, DWORD uint32_t, BOOL int32_t): TESDataHandler::CheckForNiRawImageData
// and NiD3DShaderProgramCreatorObj::LoadShaderCodeFromFile open, size and read a file with them.
using ReadFile_t = int32_t(WINAPI*)(
    void* hFile,
    void* lpBuffer,
    uint32_t nNumberOfBytesToRead,
    uint32_t* lpNumberOfBytesRead,
    _OVERLAPPED* lpOverlapped
);
using GetFileSize_t = uint32_t(WINAPI*)(void* hFile, uint32_t* lpFileSizeHigh);
using CreateFileA_t = void*(WINAPI*)(const char* lpFileName,
                                     uint32_t dwDesiredAccess,
                                     uint32_t dwShareMode,
                                     _SECURITY_ATTRIBUTES* lpSecurityAttributes,
                                     uint32_t dwCreationDisposition,
                                     uint32_t dwFlagsAndAttributes,
                                     void* hTemplateFile);
XFNV_IMPORT(ReadFile_t, ReadFile, 0xFDF080);
XFNV_IMPORT(GetFileSize_t, GetFileSize, 0xFDF084);
XFNV_IMPORT(CreateFileA_t, CreateFileA, 0xFDF088);
// An exception registration record, one node of the thread's exception handler chain. Handler is never called, so it
// stays an untyped pointer to the handler.
struct _EXCEPTION_REGISTRATION_RECORD {
    _EXCEPTION_REGISTRATION_RECORD* Next;
    void* Handler;
};
// The head of the thread environment block, holding the exception chain and the thread's stack range.
// bhkCharacterListenerSpell::contactPointAddedCallback saves NtCurrentTeb()->NtTib.ExceptionList and restores it;
// Projectile::InitHavok reads it too.
struct _NT_TIB {
    _EXCEPTION_REGISTRATION_RECORD* ExceptionList;
    void* StackBase;
    void* StackLimit;
    void* SubSystemTib;

    union {
        void* FiberData;
        uint32_t Version;
    };

    void* ArbitraryUserPointer;
    _NT_TIB* Self;
};
// USER32.dll ShowCursor (BOOL bShow) -> int, the display counter.
using ShowCursor_t = int32_t(WINAPI*)(int32_t bShow);
XFNV_IMPORT(ShowCursor_t, ShowCursor, 0xFDF2AC);
using HMMIO = HMMIO__*;
using LPMMIOPROC = intptr_t(CALLBACK*)(_MMIOINFO* lpmmioinfo, uint32_t uMsg, intptr_t lParam1, intptr_t lParam2);
// An open multimedia file's I/O state: its I/O procedure and buffer pointers. The Windows SDK declares it with
// 1-byte packing (mmsystem.h): nothing changes on 32 bits, but the 64-bit winmm.dll has `htask` unaligned at +0x14
// and the buffer pointers 8 bytes lower than a naturally aligned struct would put them, so the packing is part of
// the layout the game and winmm share.
#pragma pack(push, 1)
struct _MMIOINFO {
    // The pointer the I/O procedure keeps in adwInfo.
    void* GetProcData() const {
        void* pData;
        std::memcpy(&pData, adwInfo, sizeof(pData));
        return pData;
    }

    void SetProcData(void* apData) {
        std::memcpy(adwInfo, &apData, sizeof(apData));
    }

    uint32_t dwFlags;
    uint32_t fccIOProc;
    LPMMIOPROC pIOProc;
    uint32_t wErrorRet;
    void* htask;
    int32_t cchBuffer;
    char* pchBuffer;
    char* pchNext;
    char* pchEndRead;
    char* pchEndWrite;
    int32_t lBufOffset;
    int32_t lDiskOffset;
    // The I/O procedure's own words. mmio_IOProc keeps its BSFile pointer here: in adwInfo[0] on 32
    // bits, across adwInfo[0] and adwInfo[1] on 64 (GetProcData/SetProcData).
    uint32_t adwInfo[3];
    uint32_t dwReserved1;
    uint32_t dwReserved2;
    HMMIO hmmio;
};
#pragma pack(pop)
static_assert(sizeof(void*) <= sizeof(uint32_t[3]), "the I/O procedure's pointer fits adwInfo");
static_assert(
    sizeof(void*) != 8 || (offsetof(_MMIOINFO, pchNext) == 0x28 && sizeof(_MMIOINFO) == 0x64),
    "the 64-bit winmm layout"
);
// A RIFF chunk: its id, size, form type and where its data starts.
struct _MMCKINFO {
    uint32_t ckid;
    uint32_t cksize;
    uint32_t fccType;
    uint32_t dwDataOffset;
    uint32_t dwFlags;
};
using mmioOpenA_t = HMMIO(WINAPI*)(char* pszFileName, _MMIOINFO* pmmioinfo, uint32_t fdwOpen);
using mmioDescend_t =
    uint32_t(WINAPI*)(HMMIO hmmio, _MMCKINFO* pmmcki, const _MMCKINFO* pmmckiParent, uint32_t fuDescend);
using mmioClose_t = uint32_t(WINAPI*)(HMMIO hmmio, uint32_t fuClose);
using mmioRead_t = int32_t(WINAPI*)(HMMIO hmmio, char* pch, int32_t cch);
using mmioAscend_t = uint32_t(WINAPI*)(HMMIO hmmio, _MMCKINFO* pmmcki, uint32_t fuAscend);
using mmioGetInfo_t = uint32_t(WINAPI*)(HMMIO hmmio, _MMIOINFO* pmmioinfo, uint32_t fuInfo);
using mmioAdvance_t = uint32_t(WINAPI*)(HMMIO hmmio, _MMIOINFO* pmmioinfo, uint32_t fuAdvance);

inline char bFirst = 1;
inline LARGE_INTEGER initial{};
inline LARGE_INTEGER freq{};

XFNV_IMPORT(mmioOpenA_t, mmioOpenA, 0xFDF318);
XFNV_IMPORT(mmioDescend_t, mmioDescend, 0xFDF31C);
XFNV_IMPORT(mmioClose_t, mmioClose, 0xFDF320);
XFNV_IMPORT(mmioRead_t, mmioRead, 0xFDF324);
XFNV_IMPORT(mmioAscend_t, mmioAscend, 0xFDF328);
XFNV_IMPORT(mmioGetInfo_t, mmioGetInfo, 0xFDF32C);
XFNV_IMPORT(mmioAdvance_t, mmioAdvance, 0xFDF330);
