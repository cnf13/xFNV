#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Reconstruction.hpp"

class NiDX9Renderer;

// Caches the vertex and pixel shader constant registers (float, int and bool banks) with their dirty ranges and
// writes the changed ones to the device. Not in the Xbox pdb: the PDB has the Xbox class NiD3DShaderConstantManager,
// whose members these are (IDA's PC class derives from NiRefObject directly and has them all).
// Only the layout; the methods are not reconstructed yet.
class NiDX9ShaderConstantManager : public NiRefObject {
public:
	// The register counts from the device caps.
	NiDX9ShaderConstantManager(NiDX9Renderer* pkD3DRenderer, _D3DCAPS9* kD3DCaps9);
	// The exe's NiD3DShaderConstantManager part: no registers, the renderer's device (AddRef'd).
	NiDX9ShaderConstantManager(NiDX9Renderer* pkD3DRenderer);
	~NiDX9ShaderConstantManager() override;

	// Does nothing.
	virtual void CommitChanges() {}

	static NiDX9ShaderConstantManager* Create(NiDX9Renderer* pkRenderer, _D3DCAPS9* kD3DCaps9);

	// Float registers; the saved copy is what RestoreVertexShaderConstantF and friends copy back.
	float* m_pfFloatVSConstantData;
	float* m_pfSavedFloatVSConstantData;
	float* m_pfFloatPSConstantData;
	float* m_pfSavedFloatPSConstantData;
	uint32_t m_uiFirstDirtyFloatVSReg;
	uint32_t m_uiFirstCleanFloatVSReg;
	uint32_t m_uiFirstDirtyFloatPSReg;
	uint32_t m_uiFirstCleanFloatPSReg;
	uint32_t m_uiNumFloatVSConstants;
	uint32_t m_uiNumFloatPSConstants;
	// Integer registers.
	int32_t* m_piIntVSConstantData;
	int32_t* m_piSavedIntVSConstantData;
	int32_t* m_piIntPSConstantData;
	int32_t* m_piSavedIntPSConstantData;
	uint32_t m_uiFirstDirtyIntVSReg;
	uint32_t m_uiFirstCleanIntVSReg;
	uint32_t m_uiFirstDirtyIntPSReg;
	uint32_t m_uiFirstCleanIntPSReg;
	uint32_t m_uiNumIntVSConstants;
	uint32_t m_uiNumIntPSConstants;
	// Bool registers (BOOL is a 32-bit int).
	int32_t* m_pbBoolVSConstantData;
	int32_t* m_pbSavedBoolVSConstantData;
	int32_t* m_pbBoolPSConstantData;
	int32_t* m_pbSavedBoolPSConstantData;
	uint32_t m_uiFirstDirtyBoolVSReg;
	uint32_t m_uiFirstCleanBoolVSReg;
	uint32_t m_uiFirstDirtyBoolPSReg;
	uint32_t m_uiFirstCleanBoolPSReg;
	uint32_t m_uiNumBoolVSConstants;
	uint32_t m_uiNumBoolPSConstants;
	// The renderer's device, AddRef'd by the constructor.
	IDirect3DDevice9* m_pkD3DDevice;
	// IDA: NiD3DRenderer*, a typedef of NiDX9Renderer.
	NiDX9Renderer* m_pkD3DRenderer;
	bool m_bSoftwareOnlyVP;
};
inline constexpr fnvd::Vtable NiDX9ShaderConstantManager_vftable{};
