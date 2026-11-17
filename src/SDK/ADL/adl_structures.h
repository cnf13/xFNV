#pragma once

#include <stddef.h>
#include <stdint.h>
#include "SDK/Windows/windef.h"

// The AMD Display Library (ADL) SDK's structures and entry-point types, as AMD's Eyefinity sample code linked into
// the exe (atiEyefinityGetConfigInfo, its atiEyefinityGetConfigInfo) uses them. None of it is in the PDB; the names are
// the ADL SDK's (adl_structures.h, and the entry-point typedefs of its samples). The exe loads atiadlxx.dll
// (atiadlxy.dll) and resolves each entry point with GetProcAddress; all of them are cdecl.

// One adapter's description (IDA struct AdapterInfo, 0x624 bytes: the stride of the exe's array; it matches
// iAdapterIndex +0x4 and iOSDisplayIndex +0x620 against the Windows display index).
struct AdapterInfo {
    int32_t iSize;
    int32_t iAdapterIndex;
    char strUDID[256];
    int32_t iBusNumber;
    int32_t iDeviceNumber;
    int32_t iFunctionNumber;
    int32_t iVendorID;
    char strAdapterName[256];
    char strDisplayName[256];
    int32_t iPresent;
    int32_t iExist;
    char strDriverPath[256];
    char strDriverPathExt[256];
    char strPNPString[256];
    int32_t iOSDisplayIndex;
};

// A display's identity.
struct ADLDisplayID {
    int32_t iDisplayLogicalIndex;
    int32_t iDisplayPhysicalIndex;
    int32_t iDisplayLogicalAdapterIndex;
    int32_t iDisplayPhysicalAdapterIndex;
};

// A display mode (iXRes +0x1C, iYRes +0x20 and iOrientation +0x2C are the exe's reads).
struct ADLMode {
    int32_t iAdapterIndex;
    ADLDisplayID displayID;
    int32_t iXPos;
    int32_t iYPos;
    int32_t iXRes;
    int32_t iYRes;
    int32_t iColourDepth;
    float fRefreshRate;
    int32_t iOrientation;
    int32_t iModeFlag;
    int32_t iModeMask;
    int32_t iModeValue;
};

// A display map of ADL_Display_DisplayMapConfig_Get; the exe only frees the array.
struct ADLDisplayMap {
    int32_t iDisplayMapIndex;
    ADLMode displayMode;
    int32_t iNumDisplayTarget;
    int32_t iFirstDisplayTargetArrayIndex;
    int32_t iDisplayMapMask;
    int32_t iDisplayMapValue;
};

// A display target of a display map (0x1C bytes: the exe's stride; it reads displayID.iDisplayLogicalIndex +0x0).
struct ADLDisplayTarget {
    ADLDisplayID displayID;
    int32_t iDisplayMapIndex;
    int32_t iDisplayTargetMask;
    int32_t iDisplayTargetValue;
};

// An SLS (Eyefinity) grid.
struct ADLSLSGrid {
    int32_t iAdapterIndex;
    int32_t iSLSGridIndex;
    int32_t iSLSGridRow;
    int32_t iSLSGridColumn;
    int32_t iSLSGridMask;
    int32_t iSLSGridValue;
};

// An SLS map (grid.iSLSGridRow +0x10 and grid.iSLSGridColumn +0x14 are the exe's reads).
struct ADLSLSMap {
    int32_t iAdapterIndex;
    int32_t iSLSMapIndex;
    ADLSLSGrid grid;
    int32_t iSurfaceMapIndex;
    int32_t iOrientation;
    int32_t iSLSMapMask;
    int32_t iSLSMapValue;
    int32_t unk30[8];
};

// A display of an SLS map (0x70 bytes: the exe's stride; displayTarget.displayID.iDisplayLogicalIndex +0x8,
// iSLSGridPositionX +0x24 and iSLSGridPositionY +0x28 are its reads).
struct ADLSLSTarget {
    int32_t iAdapterIndex;
    int32_t iSLSMapIndex;
    ADLDisplayTarget displayTarget;
    int32_t iSLSGridPositionX;
    int32_t iSLSGridPositionY;
    ADLMode viewSize;
    int32_t iSLSTargetMask;
    int32_t iSLSTargetValue;
};

// A native SLS mode (0x50 bytes: the exe's stride; displayMode.iXRes +0x28 and iYRes +0x2C are its reads).
struct ADLSLSMode {
    int32_t iAdapterIndex;
    int32_t iSLSMapIndex;
    int32_t iSLSModeIndex;
    ADLMode displayMode;
    int32_t iSLSNativeModeMask;
    int32_t iSLSNativeModeValue;
};

// A bezel or transient SLS mode (0x58 bytes: the exe's stride; iSLSModeIndex +0x8 and displayMode's resolution
// +0x28/+0x2C are its reads).
struct ADLBezelTransientMode {
    int32_t iAdapterIndex;
    int32_t iSLSMapIndex;
    int32_t iSLSModeIndex;
    ADLMode displayMode;
    int32_t iNumBezelOffset;
    int32_t iFirstBezelOffsetArrayIndex;
    int32_t iSLSBezelTransientModeMask;
    int32_t iSLSBezelTransientModeValue;
};

// A display's bezel offset in a bezel mode (0x34 bytes: the exe's stride; displayID +0x8, iBezelModeIndex +0x18 and
// iBezelOffsetX..iDisplayHeight +0x1C..+0x28 are its reads).
struct ADLSLSOffset {
    int32_t iAdapterIndex;
    int32_t iSLSMapIndex;
    ADLDisplayID displayID;
    int32_t iBezelModeIndex;
    int32_t iBezelOffsetX;
    int32_t iBezelOffsetY;
    int32_t iDisplayWidth;
    int32_t iDisplayHeight;
    int32_t iBezelOffsetMask;
    int32_t iBezelffsetValue;
};

// ADL's allocator callback (stdcall). The SDK's takes an int; uint32_t here, as the exe's callback ADLMainMemoryAlloc
// (Unplaced.h) is declared.
using ADL_MAIN_MALLOC_CALLBACK = void*(WINAPI*)(uint32_t);

// The entry points the Eyefinity sample resolves, cdecl as the exe calls them (`add esp` after each call).
using ADL_MAIN_CONTROL_CREATE = int32_t (*)(ADL_MAIN_MALLOC_CALLBACK callback, int32_t iEnumConnectedAdapters);
using ADL_MAIN_CONTROL_DESTROY = int32_t (*)();
using ADL_ADAPTER_NUMBEROFADAPTERS_GET = int32_t (*)(int32_t* lpNumAdapters);
using ADL_ADAPTER_ADAPTERINFO_GET = int32_t (*)(AdapterInfo* lpInfo, int32_t iInputSize);
using ADL_DISPLAY_DISPLAYMAPCONFIG_GET = int32_t (*)(
    int32_t iAdapterIndex,
    int32_t* lpNumDisplayMap,
    ADLDisplayMap** lppDisplayMap,
    int32_t* lpNumDisplayTarget,
    ADLDisplayTarget** lppDisplayTarget,
    int32_t iOptions
);
using ADL_DISPLAY_SLSMAPINDEX_GET = int32_t (*)(
    int32_t iAdapterIndex,
    int32_t iADLNumDisplayTarget,
    ADLDisplayTarget* lpDisplayTarget,
    int32_t* lpSLSMapIndex
);
using ADL_DISPLAY_SLSMAPCONFIG_GET = int32_t (*)(
    int32_t iAdapterIndex,
    int32_t iSLSMapIndex,
    ADLSLSMap* lpSLSMap,
    int32_t* lpNumSLSTarget,
    ADLSLSTarget** lppSLSTarget,
    int32_t* lpNumNativeMode,
    ADLSLSMode** lppNativeMode,
    int32_t* lpNumBezelMode,
    ADLBezelTransientMode** lppBezelMode,
    int32_t* lpNumTransientMode,
    ADLBezelTransientMode** lppTransientMode,
    int32_t* lpNumSLSOffset,
    ADLSLSOffset** lppSLSOffset,
    int32_t iOption
);
using ADL_DISPLAY_MODES_GET =
    int32_t (*)(int32_t iAdapterIndex, int32_t iDisplayIndex, int32_t* lpNumModes, ADLMode** lppModes);
