#pragma once

#include "BSShader/BSShader.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"

class NiD3DPixelShader;
class NiD3DVertexShader;

class TileShader : public BSShader {
public:
    TileShader(NiDX9ShaderDeclaration* apShaderDecl, NiDX9ShaderDeclaration* apShaderDeclVC);
    ~TileShader() override;

    const NiRTTI* GetRTTI() const override;
    bool Initialize() override;
    void SetupGeometryTextures(const NiPropertyState* apState) override;
    void SetupGeometryConstants(const NiPropertyState* apState) override;
    void PostGeometry(const NiPropertyState* apState) override;
    void RecreateRendererData() override;
    void InitPass(uint32_t auiCurrentPass) override;
    void SetupPassShaders(uint32_t auiCurrentPass) override {}
    void SetupPassConstants(uint32_t auiCurrentPass) override;
    void CreateConstantMaps() override;
    void ReloadShaders() override;

protected:
    virtual void LoadShaders();
    virtual void LoadVertexShaders();
    virtual void LoadPixelShaders();
    virtual bool LoadStagesAndPasses();
    virtual bool LoadTextures();

public:
    static BSShader* CreateShader();

    NiPointer<NiD3DPass> spTileShaderPass;
    NiPointer<NiD3DVertexShader> spTileVertexShader[3];
    NiPointer<NiD3DPixelShader> spTilePixelShader[3];
    NiD3DShaderDeclaration* spShaderDecl;
    NiD3DShaderDeclaration* spShaderDeclVC;

    static NiRTTI ms_RTTI;
    // The shader constant holding 1/gamma in x, which SetupPassConstants writes.
    static inline D3DXVECTOR4 Gamma{};
    // The "tintcolor" pixel and "TexScroll" vertex shader constants (set up in CreateConstantMaps).
    static inline D3DXVECTOR4 TintColor{};
    static inline D3DXVECTOR4 TexScroll{};
};
