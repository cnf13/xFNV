#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Reconstruction.hpp"

class NiMaterialProperty;

// The material the render state last sent to the device.
class NiDX9MaterialManager : public NiMemObject {
public:
	struct MaterialEntry : public NiMemObject {
		// IDA names it MaterialEntry::MaterialEntry; not in the PDB. It fills the D3D material
		// from the property and returns whether the entry changed; the exe calls it both for the construction (after
		// m_uiRevID is cleared) and for the reference source's Update (thiscall, ret 4, 0xE93FB0).
		bool Update(const NiMaterialProperty* apMaterial);

		D3DMATERIAL9 m_kD3DMat;
		uint32_t m_uiRevID;
	};

	~NiDX9MaterialManager();

	void PurgeMaterial(NiMaterialProperty* apMaterial);

	// Makes the entry of apMaterial (a new one when it has no renderer data) the current material and updates it.
	// Inferred type: apMaterial: NiMaterialProperty* from 0xe87991 (parameter 0 of NiD3DRenderState::ApplyMaterial),
	// its members used at 0xe940d8
	void SetCurrentMaterial(const NiMaterialProperty* apMaterial);

	void UpdateToDevice(IDirect3DDevice9* apDevice);

	// Forgets the current material, makes the default material current and marks it
	// changed.
	void Initialize();

	MaterialEntry* m_pkCurrentMaterial;
	bool m_bChanged;
};
