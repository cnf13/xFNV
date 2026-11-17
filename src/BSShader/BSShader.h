#pragma once

#include "BSShader/BSShaderManager.h"
// The base of Bethesda's shaders, set on each geometry by its shader property.
// Pass identifiers and shader types use their PC 32-bit storage.
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DDefaultShader.h"

#include <stdint.h>

class NiD3DPixelShader;
class NiD3DVertexShader;
class NiDX9ShaderDeclaration;

struct BSShaderMacro {
	const char* strPath;
	D3DXMACRO kMacros[9];
};

class BSShader : public NiD3DDefaultShader {
public:
	BSShader();
	~BSShader() override;

	bool Initialize() override;
	bool SetupGeometry(NiGeometry* apGeometry) override;
	void SetupGeometryAlphaBlending(const NiPropertyState* apState) override;
	void SetupGeometryAlphaTesting(const NiPropertyState* apState) override;
	void SetupGeometryRenderStates(const NiPropertyState* apState, bool abFirstPass) override;
	void RecreateRendererData() override;

	virtual void InitPass(uint32_t auiCurrentPass) {}
	virtual void SetupPassShaders(uint32_t auiCurrentPass);
	virtual void SetupPassTextures(uint32_t auiCurrentPass) {}
	virtual void SetupPassConstants(uint32_t auiCurrentPass) {}
	virtual void SetupPassRenderStates(uint32_t auiCurrentPass);
	virtual void RestorePassRenderStates(uint32_t auiCurrentPass);
	virtual void ApplyTextureStages();
	virtual void ApplyTextureStagesFlags();
	virtual void SetupGeometryNonFirstPass(const NiPropertyState* apState);
	virtual void RestoreGeometryNonFirstPass();
	using NiD3DDefaultShader::GetShaderDecl;
	virtual NiD3DShaderDeclaration* GetShaderDecl(NiGeometry* apGeometry, BSShaderProperty* apProperty);
	virtual void CreateConstantMaps() {}
	virtual void ReloadShaders() {}
	virtual void ClearPassStages();
	virtual void ClearPass(NiD3DPass* apPass);

protected:
	virtual bool CreateShaders();
	virtual bool CreateStagesAndPasses();
	virtual bool CreateTextures();

public:
	virtual void SetCurrentPass(uint32_t aPass, bool aFirstPass);

	// Returns iShaderType.
	int32_t GetShaderType();
	// Sets vertex shader constant c25 from the first scene light's direction turned by
	// apMatrix.
	void SetLightDirection(D3DXMATRIX* apMatrix);
	// Loads the vertex shader apCacheName: from the loaded shader package, else from the compiled-shader cache, else
	// compiled from the HLSL file apFilename with apMacro (8 entries, "HDR" appended under HDR) for apTarget, writing
	// the cache file. The body never reads `this`.
	NiD3DVertexShader* CompileVertexShaderHLSL(const char* apFilename, D3DXMACRO* apMacro, const char* apTarget,
		const char* apCacheName);
	// The pixel shader form of CompileVertexShaderHLSL; a "ps_2" target is replaced by the manager's pixel shader
	// target.
	NiD3DPixelShader* CompilePixelShaderHLSL(const char* apFilename, D3DXMACRO* apMacro, const char* apTarget,
		const char* apCacheName);

	static NiPoint3* GetCameraWorldTranslate();
	static void SetupDefaultStage(NiD3DTextureStage* apStage, uint32_t aiStageNum, uint32_t aiAddressValue,
		uint32_t aiMipFilter, bool abAniso);
	// Joins the shader path with apFile's own directory into apResult.
	static void MakePath(const char* apFile, char* apResult);
	// Builds the compiled shader's path for apFilename into apResult: the shader cache directory, apCacheName and the
	// extension apExtension ("vso"/"pso").
	static void CreateShaderPath(char* apResult, const char* apFilename, const char* apCacheName,
		const char* apExtension);
	static NiSourceTexture* GetDefNormalMap();

	NiPointer<NiD3DPass> spPass;
	// SetCurrentPass stores the pass number.
	BSShaderManager::eBSSMPassEnum ePass;
	bool bFirstPass;
	uint8_t padding65[3];
	uint32_t iShaderType;

	// Whether the HLSL sources are checked against the compiled-shader cache, and compiled when it has no newer copy,
	// for a shader the shader package lacks.
	static inline bool bCompileNewer = true;
	static inline int16_t sShadowAlphaTestRef = -1;
};

NiSmartPointer(BSShader);

// The include handler BSShader passes to the D3DX shader compiler.
class ShaderIncludes : public ID3DXInclude {
public:
	explicit ShaderIncludes(const char* apShaderFileName);

	// Reads the included shader file into a MemoryManager block.
	HRESULT STDMETHODCALLTYPE Open(uint32_t IncludeType, const char* pFileName, const void* pParentData,
		const void** ppData, uint32_t* pBytes) override;
	// Frees the data Open allocated.
	HRESULT STDMETHODCALLTYPE Close(const void* pData) override;

	const char* pShaderFileName;
};

using uiEnabledFlags_t = int32_t[16];
inline uiEnabledFlags_t uiEnabledFlags{1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768};
