#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVertexColorProperty.h"
#include "Reconstruction.hpp"

class NiDX9RenderState;
class NiDynamicEffectState;
class NiLight;

// Sets the device's fixed-function lights from a geometry's dynamic effects. The layout is Gamebryo 2.2's
// NiDX9LightManager.h (the PDB lacks the class: the Xbox has no DX9 renderer), as the exe's constructor (0xE8BAE0)
// builds it: the light map at +0x0 (hash size 37), the device light list at +0x10, the device (AddRef'd) at +0x1C,
// the render state at +0x20, -1 as the invalid state at +0x24, the next free index at +0x38 and the 512-byte index
// bit set at +0x3C; NiDX9Renderer::Initialize (0xE72E60) allocates 0x23C bytes.
class NiDX9LightManager : public NiMemObject {
public:
	// One device light: the D3D9 light a NiLight produced, the NiLight revision it was built from, its device light
	// index and two flags (Gamebryo 2.2's NiDX9LightManager.h, which the PDB lacks: the Xbox has no DX9 renderer).
	// 0x74 bytes, as the exe's code has them: the constructor (0xE8B4B0) writes m_uiRevID 0 at +0x68, m_iIndex at
	// +0x6C and both flags 0 at +0x70/+0x71; Update (0xE8B250) rebuilds m_kD3DLight when m_uiRevID differs from the
	// NiLight's; UpdateLights (0xE8B740) reads m_iIndex and the flags; the frees push 0x74 (0xE8B7D8's
	// operator delete(m_val, 116u) and the allocation at 0xE8B740). Public because the map instances' DeletingDestruct
	// specializations below are declared at namespace scope.
	class LightEntry : public NiMemObject {
	public:
		LightEntry(const NiLight& kLight, int32_t iIndex);

		// Gamebryo 2.2's LightEntry::Update: when kLight's revision (+0xA8) differs from m_uiRevID, rebuilds
		// m_kD3DLight from the light by its type (+0x9D) and returns true; else false (thiscall, ret 4, 0xE8B250; the
		// callers are UpdateLights 0xE8B740 and the constructor 0xE8B4B0).
		bool Update(const NiLight& kLight);

		D3DLIGHT9 m_kD3DLight;
		uint32_t m_uiRevID;
		int32_t m_iIndex;
		bool m_bNeedsRemoval;
		bool m_bActive;
	};

	NiDX9LightManager(NiDX9RenderState* pkRenderState, IDirect3DDevice9* pkD3DDevice9);
	~NiDX9LightManager();

	// thiscall, 0xE8BB70. Sets the lighting and material-source render states for a texturing and vertex colour
	// property and, when the state or the ambient-diffuse mode changed, the device lights (UpdateLights).
	void SetState(const NiDynamicEffectState* pkState, const NiTexturingProperty* pkTex,
		const NiVertexColorProperty* pkColor);
	// thiscall, 0xE8B560. Disables the device lights enabled during the last frame.
	void NextFrame();
	// thiscall, 0xE8BD40. Forgets apLight and frees its device light index.
	void RemoveLight(NiLight* apLight);
	// thiscall, 0xE8BE20. Disables and forgets every device light (before a device reset).
	void ReleaseLights();
	// thiscall, 0xE8B170. Sets the device's lighting render states again.
	void InitRenderModes();
	// Gamebryo 2.2's GetNextAvailableIndex; not in the PDB. Takes the first clear bit of
	// m_auiValidIndices from m_uiNextAvailableIndex on (wrapping at 4096), sets it and returns its index, or -1
	// (thiscall, ret 0, 0xE8B1F0).
	uint32_t GetNextAvailableIndex();
	// IDA name; not in the PDB. thiscall, ret 4, 0xE8B740 (SetState's call at 0xE8BD05 passes its pkState).
	void UpdateLights(const NiDynamicEffectState* pkState);

	NiTPointerMap<NiLight*, LightEntry*> m_kLights;
	NiTPointerList<NiLight*> m_kDeviceLights;
	IDirect3DDevice9* m_pkD3DDevice9;
	NiDX9RenderState* m_pkRenderState;
	// Gamebryo: const NiDynamicEffectState* const; the exe's constructor stores it (-1), as Construct does here.
	const NiDynamicEffectState* m_pkInvalidState;
	const NiDynamicEffectState* m_pkLastState;
	bool m_bLastAmbientDiffuse;
	bool m_bAmbientDiffuse;
	NiTexturingProperty::ApplyMode m_eApply;
	NiVertexColorProperty::SourceVertexMode m_eSourceMode;
	uint32_t m_uiNextAvailableIndex;
	// One bit per device light index (LIGHTINDEXARRAYSIZE 128 words).
	uint32_t m_auiValidIndices[128];

	static float ms_fMaxRange;
};
