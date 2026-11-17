#pragma once

#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgramCreator.h"
#include "Reconstruction.hpp"

struct ID3DXBuffer;
struct ID3DXConstantTable;

// The shader program creator for HLSL source; registered with NiD3DShaderProgramFactory by _SDMInit.
class NiD3DShaderProgramCreatorHLSL : public NiD3DShaderProgramCreator {
public:
	NiD3DShaderProgramCreatorHLSL();
	~NiD3DShaderProgramCreatorHLSL() override;

	uint32_t GetShaderCreationFlags() const override { return m_uiShaderCreationFlags; }
	void SetShaderCreationFlags(uint32_t uiFlags) override { m_uiShaderCreationFlags = uiFlags; }
	// The exe passes the caller's whole dword on; bool would be zero-extended. PDB: bool bSoftwareVP, bool
	// bRecoverable.
	NiD3DVertexShader* CreateVertexShaderFromFile(const char* pcFileName, const char* pcShaderName,
		const char* pcEntryPoint, const char* pcShaderTarget, IDirect3DVertexDeclaration9* hDecl, uint32_t uiUsage,
		int32_t bSoftwareVP, int32_t bRecoverable) override;
	// The exe passes the caller's whole dword on; bool would be zero-extended. PDB: bool bSoftwareVP, bool
	// bRecoverable.
	NiD3DVertexShader* CreateVertexShaderFromBuffer(const void* pvBuffer, uint32_t uiBufferSize,
		const char* pcShaderName, const char* pcEntryPoint, const char* pcShaderTarget,
		IDirect3DVertexDeclaration9* hDecl, uint32_t uiUsage, int32_t bSoftwareVP, int32_t bRecoverable) override;
	// The exe passes the caller's whole dword on; bool would be zero-extended. PDB: bool bRecoverable.
	NiD3DPixelShader* CreatePixelShaderFromFile(const char* pcFileName, const char* pcShaderName,
		const char* pcEntryPoint, const char* pcShaderTarget, int32_t bRecoverable) override;
	// The exe passes the caller's whole dword on; bool would be zero-extended. PDB: bool bRecoverable.
	NiD3DPixelShader* CreatePixelShaderFromBuffer(const void* pvBuffer, uint32_t uiBufferSize, const char* pcShaderName,
		const char* pcEntryPoint, const char* pcShaderTarget, int32_t bRecoverable) override;
	bool RecreateVertexShader(NiD3DVertexShader* pkVertexShader) override;
	bool RecreatePixelShader(NiD3DPixelShader* pkPixelShader) override;
	// The exe puts the two D3DXBuffer entries after the overrides (slots 9 and 10); the Xbox PDB's declaration order
	// differs. IDA names slot 9 CreateVertexShaderFromBuffer. bSoftwareVP and bRecoverable, here and below: the exe
	// passes the caller's whole dword on; bool would be zero-extended (to CreateVertexShader and
	// NiShaderFactory::ReportError). PDB: bool bSoftwareVP, bool bRecoverable.
	virtual NiD3DVertexShader* CreateVertexShaderFromD3DXBuffer(ID3DXBuffer* pkBuffer, uint32_t uiBufferSize,
		const char* pcShaderName, const char* pcEntryPoint, const char* pcShaderTarget,
		IDirect3DVertexDeclaration9* hDecl, uint32_t uiUsage, int32_t bSoftwareVP, int32_t bRecoverable);
	virtual NiD3DPixelShader* CreatePixelShaderFromD3DXBuffer(ID3DXBuffer* pkBuffer, uint32_t uiBufferSize,
		const char* pcShaderName, const char* pcEntryPoint, const char* pcShaderTarget, int32_t bRecoverable);

	// Compiles HLSL source into shader code and its constant table; false on failure (thiscall, ret 0x20, 0xE964A0).
	// The exe passes the caller's whole dword on; bool would be zero-extended. PDB: bool bRecoverable.
	bool LoadShaderCodeFromBuffer(const void* pvBuffer, uint32_t uiBufferSize, const char* pcEntryPoint,
		const char* pcShaderTarget, void*& pvCode, uint32_t& uiCodeSize, ID3DXConstantTable*& pkConstantTable,
		int32_t bRecoverable);
	// The exe passes the caller's whole dword on; bool would be zero-extended. PDB: bool
	// bRecoverable.
	bool LoadShaderCodeFromFile(const char* pcFileName, const char* pcEntryPoint, const char* pcShaderTarget,
		void*& pvCode, uint32_t& uiCodeSize, ID3DXConstantTable*& pkConstantTable, int32_t bRecoverable);

	static void _SDMInit();

	uint32_t m_uiShaderCreationFlags;

	// The creator _SDMInit publishes for the "hlsl" extension.
	static inline NiD3DShaderProgramCreatorHLSL* ms_pkCreator;
};
inline constexpr fnvd::Vtable NiD3DShaderProgramCreatorHLSL_vftable{};
