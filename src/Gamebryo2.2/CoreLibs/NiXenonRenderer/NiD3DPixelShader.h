#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgram.h"
#include "Reconstruction.hpp"

class NiDX9Renderer;

// A compiled D3D pixel shader.
class NiD3DPixelShader : public NiD3DShaderProgram {
public:
	NiD3DPixelShader(NiDX9Renderer* apRenderer);
	~NiD3DPixelShader() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	bool SetShaderConstant(NiShaderConstantMapEntry* pkEntry, const void* pvDataSource,
		uint32_t uiRegisterCount) override;
	bool SetShaderConstantArray(NiShaderConstantMapEntry* pkEntry, const void* pvDataSource, uint32_t uiNumEntries,
		uint32_t uiRegistersPerEntry, uint16_t* pusReorderArray) override;
	virtual IDirect3DPixelShader9* GetShaderHandle() const { return m_hShader; }
	virtual void SetShaderHandle(IDirect3DPixelShader9* hShader) { m_hShader = hShader; }
	virtual void DestroyRendererData();
	// The callers discard eax (0xE814D3, 0xBC4667, 0xBB96A3...).
	virtual void RecreateRendererData();

	IDirect3DPixelShader9* m_hShader;

	static NiRTTI ms_RTTI;
};

NiSmartPointer(NiD3DPixelShader);

inline constexpr fnvd::Vtable NiD3DPixelShader_vftable{};
