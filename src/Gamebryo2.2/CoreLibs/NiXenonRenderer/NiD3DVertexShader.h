#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgram.h"
#include "Reconstruction.hpp"

class NiDX9Renderer;

// A compiled D3D vertex shader with its vertex declaration.
class NiD3DVertexShader : public NiD3DShaderProgram {
public:
	NiD3DVertexShader(NiDX9Renderer* apRenderer);
	~NiD3DVertexShader() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	bool SetShaderConstant(NiShaderConstantMapEntry* pkEntry, const void* pvDataSource,
		uint32_t uiRegisterCount) override;
	bool SetShaderConstantArray(NiShaderConstantMapEntry* pkEntry, const void* pvDataSource, uint32_t uiNumEntries,
		uint32_t uiRegistersPerEntry, uint16_t* pusReorderArray) override;
	virtual uint32_t GetUsage() const { return m_uiUsage; }
	virtual void SetUsage(uint32_t uiUsage) { m_uiUsage = uiUsage; }
	virtual IDirect3DVertexShader9* GetShaderHandle() const { return m_hShader; }
	virtual void SetShaderHandle(IDirect3DVertexShader9* hShader) { m_hShader = hShader; }
	virtual IDirect3DVertexDeclaration9* GetVertexDeclaration() const { return m_hDecl; }
	virtual void SetVertexDeclaration(IDirect3DVertexDeclaration9* hDecl) { m_hDecl = hDecl; }
	virtual uint8_t GetSoftwareVertexProcessing() const { return m_bSoftwareVP; }
	virtual void SetSoftwareVertexProcessing(uint8_t bSoftwareVP) { m_bSoftwareVP = bSoftwareVP; }
	virtual void DestroyRendererData();
	// The callers discard eax (0xE814E4, 0xBC4657, 0xBB9682...).
	virtual void RecreateRendererData();

	uint8_t m_bSoftwareVP;
	uint32_t m_uiUsage;
	IDirect3DVertexShader9* m_hShader;
	IDirect3DVertexDeclaration9* m_hDecl;

	static NiRTTI ms_RTTI;
};

NiSmartPointer(NiD3DVertexShader);

inline constexpr fnvd::Vtable NiD3DVertexShader_vftable{};
