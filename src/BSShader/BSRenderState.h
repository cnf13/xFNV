#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStencilProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiZBufferProperty.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// The render states BSRenderState locks; the index of each one's lock count in BSRenderState::iLock.
enum BSRENDERSTATE : uint32_t {
	BSRS_ZENABLE = 0,
	BSRS_ZWRITEENABLE = 1,
	BSRS_ZFUNC = 2,
	BSRS_DEPTHBIAS = 3,
	BSRS_ALPHATESTENABLE = 4,
	BSRS_ALPHATESTFUNCREF = 5,
	BSRS_ALPHABLENDENABLE = 6,
	BSRS_ALPHABLENDFUNC = 7,
	BSRS_ALPHABLENDOP = 8,
	BSRS_STENCILENABLE = 9,
	BSRS_STENCILFAILZFAILPASS = 10,
	BSRS_STENCILFUNCREFMASK = 11,
	BSRS_STENCILWRITEMASK = 12,
	BSRS_SCISSORENABLE = 13,
	BSRS_SCISSORRECT = 14,
	BSRS_CLIPPLANEENABLE = 15,
	BSRS_COLORWRITEENABLE = 16,
	BSRS_COLORWRITEENABLEMRT = 17,
	BSRS_WIREFRAMEENABLE = 18,
	BSRS_CULLMODE = 19,
	BSRS_ALPHATOCOVERAGE = 20,
	BSRS_COUNT = 21,
};

// How a Set* call changes the state's lock count (added to it).
enum BSRENDERSTATE_LOCK : uint32_t {
	BSRS_NOLOCK = 0,
	BSRS_LOCK = 1,
};

// The game's cached Direct3D render states: each state has a lock count, and Restore* releases aeLock locks before it
// sets the state back to its default.
class BSRenderState {
public:
	// The lock count of each state. Length unknown (not in the PDB): at least 21, since RenderNextRenderPass
	// reads [20].
	using LockArray = uint32_t[21];
	// The BSRENDERSTATE whose lock count guards each D3DRENDERSTATETYPE (0xFFFFFFFF: none). IDA name; not in the
	// PDB. Setup fills it.
	using D3D2BSRSArray = uint32_t[255];

	// IDA's GPUVendor; not in the PDB. The values of eGPUVendor.
	enum GPUVendor : uint32_t {
		Unk = 0,
		Nvidia = 1,
		AMD = 2,
		Intel = 3,
	};

	// The D3DCMPFUNC of each test function and the D3DBLEND of each alpha function. PDB: file-local
	// NiToD3DAlphaTestCmpFunc and NiToD3DAlphaFunc; NiToD3DAlphaFunc has one entry per NiAlphaProperty::AlphaFunction
	// (11: the exe's table runs from 0x11AD8A8 to NiToD3DStencilAction at 0x11AD8D4).
	using D3DMappingArray = uint32_t[8];
	using D3DBlendArray = uint32_t[11];

	// Each is cdecl: subtracts aeLock from the state's lock count and sets the default value.
	static void RestoreZEnable(BSRENDERSTATE_LOCK aeLock);
	static void RestoreZWriteEnable(BSRENDERSTATE_LOCK aeLock);
	static void RestoreZFunc(BSRENDERSTATE_LOCK aeLock);
	static void RestoreAlphaTestEnable(BSRENDERSTATE_LOCK aeLock);
	static void RestoreAlphaTestFuncRef(BSRENDERSTATE_LOCK aeLock);
	static void RestoreAlphaBlendEnable(BSRENDERSTATE_LOCK aeLock);
	static void RestoreAlphaBlendFunc(BSRENDERSTATE_LOCK aeLock);
	static void RestoreColorWriteEnable(BSRENDERSTATE_LOCK aeLock);
	// Sets the color write mask and adds aeLock locks.
	static void SetColorWriteEnable(uint32_t auiMask, BSRENDERSTATE_LOCK aeLock);
	static void SetColorWriteEnableMrt(uint32_t auiMask1, uint32_t auiMask2, uint32_t auiMask3,
		BSRENDERSTATE_LOCK aeLock);
	static void RestoreStencilEnable(BSRENDERSTATE_LOCK aeLock);
	static void RestoreStencilAction(BSRENDERSTATE_LOCK aeLock);
	static void RestoreStencilFunc(BSRENDERSTATE_LOCK aeLock);
	static void RestoreStencilWriteMask(BSRENDERSTATE_LOCK aeLock);
	static void RestoreScissorEnable(BSRENDERSTATE_LOCK aeLock);
	static void RestoreScissorRect(BSRENDERSTATE_LOCK aeLock);
	static void RestoreUserClipPlaneEnables(BSRENDERSTATE_LOCK aeLock);
	static void RestoreDepthBias(BSRENDERSTATE_LOCK aeLock);
	static void RestoreWireframeEnable(BSRENDERSTATE_LOCK aeLock);
	static void RestoreAlphaToCoverageEnable(BSRENDERSTATE_LOCK aeLock);
	// Each is cdecl: sets the state and adds aeLock locks. SetAlphaToCoverageEnable is not reconstructed yet.
	static void SetCullMode(NiStencilProperty::DrawMode aeDrawMode, BSRENDERSTATE_LOCK aeLock);
	static void SetAlphaTestEnable(bool abEnabled, BSRENDERSTATE_LOCK aeLock);
	static void SetAlphaToCoverageEnable(bool abEnabled, BSRENDERSTATE_LOCK aeLock);

	static void Setup();
	static void SetRenderState(uint32_t auiRenderState, uint32_t auiValue, BSRENDERSTATE_LOCK aeLock);
	static void SetZEnable(bool abEnabled, BSRENDERSTATE_LOCK aeLock);
	static void SetZWriteEnable(bool abEnabled, BSRENDERSTATE_LOCK aeLock);
	static void SetZFunc(NiZBufferProperty::TestFunction aeZFunc, BSRENDERSTATE_LOCK aeLock);
	static void SetAlphaTestFuncRef(NiAlphaProperty::TestFunction aeFunc, uint32_t auiRef, BSRENDERSTATE_LOCK aeLock);
	static void SetAlphaBlendEnable(bool abEnabled, BSRENDERSTATE_LOCK aeLock);
	static void SetAlphaBlendFunc(NiAlphaProperty::AlphaFunction aeSrcRGB, NiAlphaProperty::AlphaFunction aeDestRGB,
		BSRENDERSTATE_LOCK aeLock);
	static void SetStencilEnable(bool abEnabled, BSRENDERSTATE_LOCK aeLock);
	static void SetStencilWriteMask(uint32_t auiWriteMask, BSRENDERSTATE_LOCK aeLock);
	static void SetScissorRect(int32_t aiLeft, int32_t aiTop, int32_t aiRight, int32_t aiBottom,
		BSRENDERSTATE_LOCK aeLock);
	// Restores auiRenderState (a D3DRENDERSTATETYPE): through its Restore* function when BSRenderState tracks it, else
	// to the render state cache's previous value.
	static void RestoreRenderState(uint32_t auiRenderState, BSRENDERSTATE_LOCK aeLock);
	// Returns eGPUVendor.
	static uint32_t GetGPUVendor();

	// Each is cdecl: sets the state and adds aeLock locks.
	static void SetStencilAction(NiStencilProperty::Action aeFail, NiStencilProperty::Action aeZFail,
		NiStencilProperty::Action aePass, BSRENDERSTATE_LOCK aeLock);
	static void SetStencilFunc(NiStencilProperty::TestFunc aeFunc, uint32_t auiRef, uint32_t auiTestMask,
		BSRENDERSTATE_LOCK aeLock);
	static void SetScissorEnable(bool abEnabled, BSRENDERSTATE_LOCK aeLock);
	static void SetUserClipPlaneEnables(bool abPlane0, bool abPlane1, bool abPlane2, bool abPlane3, bool abPlane4,
		bool abPlane5, BSRENDERSTATE_LOCK aeLock);
	static void SetDepthBias(float afDepthBias, BSRENDERSTATE_LOCK aeLock);
	static void SetWireframeEnable(bool abEnable, BSRENDERSTATE_LOCK aeLock);
	static void RestoreColorWriteEnableMrt(BSRENDERSTATE_LOCK aeLock);
	static void UnlockRenderState(BSRENDERSTATE aeState);

	static inline LockArray iLock{};
	static inline D3D2BSRSArray uiD3D2BSRS{};
	// The GPU vendor GetGPUVendor returns. IDA name; not in the PDB.
	static inline uint32_t eGPUVendor;
	static inline D3DMappingArray NiToD3DAlphaTestCmpFunc{8, 2, 3, 4, 5, 6, 7, 1};
	static inline D3DBlendArray NiToD3DAlphaFunc{2, 1, 3, 4, 9, 10, 5, 6, 7, 8, 11};
};

// The D3DSTENCILOP of each NiStencilProperty::Action; SetStencilAction indexes it with the action (0xB980DE) and
// passes the entry as SetRenderState's DWORD value. PDB: a file-local `_D3DSTENCILOP NiToD3DStencilAction[6]`; src
// declares no D3DSTENCILOP enum, so the entries are the D3D DWORDs.
using NiToD3DStencilActionArray = uint32_t[NiStencilProperty::ACTION_MAX];
inline NiToD3DStencilActionArray NiToD3DStencilAction{1, 2, 3, 7, 8, 6};
