#pragma once

#include "Reconstruction.hpp"
#include <stdint.h>

// The NVIDIA driver API's static library (nvapi.lib) as TESRenderer::Init uses it. None of it is in the PDB; the
// names are the NVAPI SDK's. Each entry point is a 6-byte `jmp [slot]` through a pointer NvAPI_Initialize fills.

// NvAPI_Status: 0 is NVAPI_OK, failures are negative.
enum NvAPI_Status : int32_t {
	NVAPI_OK = 0,
};

// An opaque handle to a display the driver drives.
struct NvDisplayHandle__;
using NvDisplayHandle = NvDisplayHandle__*;

// The resolved entry point of NvAPI_EnumNvidiaDisplayHandle (cdecl). Not named in the PDB or IDA; name chosen here.
using NvAPI_EnumNvidiaDisplayHandle_t = NvAPI_Status(*)(uint32_t thisEnum, NvDisplayHandle* pNvDispHandle);

// The handle of the thisEnum-th NVIDIA display in pNvDispHandle; fails once thisEnum passes the last one. IDA name
// _sub_9FEAD6; name from the NVAPI SDK, matched by its use (TESRenderer::Init enumerates up to 128 displays with it
// right after NvAPI_GetDisplayDriverVersion).
NvAPI_Status NvAPI_EnumNvidiaDisplayHandle(uint32_t thisEnum, NvDisplayHandle* pNvDispHandle);

// The driver's version and build strings (NVAPI SDK; NV_DISPLAY_DRIVER_VERSION_VER is sizeof | 1 << 16, 0x1008C).
struct NV_DISPLAY_DRIVER_VERSION {
	uint32_t version;
	uint32_t drvVersion;
	uint32_t bldChangeListNum;
	char szBuildBranchString[64];
	char szAdapterString[64];
};

// The SLI state of a Direct3D device (NVAPI SDK; NV_GET_CURRENT_SLI_STATE_VER is sizeof | 1 << 16, 0x1001C).
struct NV_GET_CURRENT_SLI_STATE {
	uint32_t version;
	uint32_t maxNumAFRGroups;
	uint32_t numAFRGroups;
	uint32_t currentAFRIndex;
	uint32_t nextFrameAFRIndex;
	uint32_t previousFrameAFRIndex;
	uint32_t bIsCurAFRGroupNew;
};

// Opaque handles to a physical and a logical GPU.
struct NvPhysicalGpuHandle__;
using NvPhysicalGpuHandle = NvPhysicalGpuHandle__*;
struct NvLogicalGpuHandle__;
using NvLogicalGpuHandle = NvLogicalGpuHandle__*;
struct IUnknown;

// The resolved entry points TESRenderer::Init calls (cdecl; NvAPI_Initialize fills the slots). Not named in the PDB or
// IDA; names chosen here.
using NvAPI_GetDisplayDriverVersion_t = NvAPI_Status(*)(NvDisplayHandle hNvDisplay,
	NV_DISPLAY_DRIVER_VERSION* pVersion);
using NvAPI_EnumLogicalGPUs_t = NvAPI_Status(*)(NvLogicalGpuHandle* nvGPUHandle, uint32_t* pGpuCount);
using NvAPI_EnumPhysicalGPUs_t = NvAPI_Status(*)(NvPhysicalGpuHandle* nvGPUHandle, uint32_t* pGpuCount);
using NvAPI_D3D_GetCurrentSLIState_t = NvAPI_Status(*)(IUnknown* pDevice,
	NV_GET_CURRENT_SLI_STATE* pSliState);
extern int32_t g_NvAPI_Initialized;
extern int32_t g_NvAPI_InitStatus;
// One entry of the interface table NvAPI_Initialize fills (0x11A6228: 360 entries, then a zero id ends it): the entry
// point the 6-byte `jmp [slot]` stub of one function jumps through, the not-implemented stub 0x9FEA24 until
// nvapi_QueryInterface resolves the interface id after it. Not named in the PDB or IDA; names chosen here. The entry
// is a function pointer, the stub's type (the image holds the stub's address), cast to each entry point's type at
// its call; nvapi_QueryInterface's void* is cast to it when stored.
using NvAPI_Entry_t = int32_t(*)();
struct NvAPI_InterfaceEntry {
	NvAPI_Entry_t pEntry;
	uint32_t uiId;
};
using NvAPI_InterfaceTable_t = NvAPI_InterfaceEntry[361];
extern NvAPI_InterfaceTable_t g_NvAPI_Interfaces;

// The table's entries the reconstructed entry points jump through, by index (their slot addresses after each).
// Names chosen here.
enum NvAPI_InterfaceIndex : uint32_t {
	NvAPI_Index_GetDisplayDriverVersion = 2,
	NvAPI_Index_EnumNvidiaDisplayHandle = 3,
	NvAPI_Index_EnumPhysicalGPUs = 5,
	NvAPI_Index_EnumLogicalGPUs = 6,
	NvAPI_Index_D3D_GetCurrentSLIState = 46,
	// NvAPI_D3D9_AliasPrimaryFromDevice's, interface id 0x7C20C5BE.
	NvAPI_Index_D3D9_AliasPrimaryFromDevice = 74,
};

// nvapi.dll's nvapi_QueryInterface(id): the entry point of an interface id, or null (cdecl; NVAPI SDK).
using nvapi_QueryInterface_t = void*(*)(uint32_t id);
// The driver's NvAPI_Initialize entry point, interface id 0x150E828 (NVAPI SDK).
constexpr uint32_t NvAPI_Id_Initialize = 0x150E828;
using NvAPI_Initialize_t = NvAPI_Status(*)();

// The driver version of hNvDisplay (0 for the default display) into *pVersion. IDA name _sub_9FEAD0; name from the
// NVAPI SDK, matched by its use (the 0x8C-byte versioned struct TESRenderer::Init passes right after
// NvAPI_Initialize).
NvAPI_Status NvAPI_GetDisplayDriverVersion(NvDisplayHandle hNvDisplay, NV_DISPLAY_DRIVER_VERSION* pVersion);
// The logical GPUs (up to 64 handles) and their count. IDA name _sub_9FEAE8; name from the NVAPI SDK, matched by its
// use (called first, with a 64-handle array and a count; fewer logical than physical GPUs means SLI).
NvAPI_Status NvAPI_EnumLogicalGPUs(NvLogicalGpuHandle* nvGPUHandle, uint32_t* pGpuCount);
// The physical GPUs (up to 64 handles) and their count. IDA name _sub_9FEAE2; name from the NVAPI SDK, matched as
// above (called second).
NvAPI_Status NvAPI_EnumPhysicalGPUs(NvPhysicalGpuHandle* nvGPUHandle, uint32_t* pGpuCount);
// The SLI state of the Direct3D object pDevice. IDA name _sub_9FEBD8; name from the NVAPI SDK, matched by its use
// (the Direct3D object and the 0x1C-byte versioned struct).
NvAPI_Status NvAPI_D3D_GetCurrentSLIState(IUnknown* pDevice, NV_GET_CURRENT_SLI_STATE* pSliState);
// Loads nvapi.dll and fills the entry point slots; NVAPI_OK once it has (cdecl, 0x9FEA2A; it saves every register
// with pushal, so it takes none). IDA name; name from the NVAPI SDK.
NvAPI_Status NvAPI_Initialize();
// Not named in the PDB or IDA; name chosen here (IDA: j__NvAPI_Stub_53, the first of the run of 6-byte entry stubs
// 0x9FEC80-0x9FF2E0 it takes for one function). An NVAPI entry point no exe code calls: `jmp [0x11A6478]`, the slot
// NvAPI_Initialize fills for interface id 0x7C20C5BE (the dword after it), else the not-implemented stub 0x9FEA24.
NvAPI_Status NvAPI_D3D9_AliasPrimaryFromDevice();

int32_t NvAPI_Stub();

extern int32_t g_NvAPI_StubStatus;
