#pragma once

#include "BSShader/BSShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVector4.h"

class NiD3DPixelShader;
class NiD3DVertexShader;

class BSShaderBloodSplatter : public BSShader {
public:
    struct Constant {
        static inline NiVector4 LightLoc{};
        static inline NiVector4 Ctrl{};
        static inline NiVector4 Alpha{};
    };

    BSShaderBloodSplatter();
    ~BSShaderBloodSplatter() override;

    const NiRTTI* GetRTTI() const override;
    void SetupGeometryTextures(const NiPropertyState* apState) override;
    void SetupGeometryConstants(const NiPropertyState* apState) override;
    void InitPass(uint32_t auiCurrentPass) override;
    void CreateConstantMaps() override;
    void ReloadShaders() override;
    void ClearPassStages() override;

protected:
    // Loads the vertex and pixel shaders.
    void LoadShaders();
    // Creates the passes and sets up their stages; always true.
    bool LoadStagesAndPasses();

public:
    static void SetFlareMult(float afFlareMult);
    static void SetGlobalAlpha(float afAlpha);
    static float GetGlobalAlpha();
    static void SetFlareOffsetScale(float afScale);
    static void ComputeScreenSpaceLightLoc(NiCamera* apCamera);
    static BSShader* CreateShader();

    NiPointer<NiD3DPass> spPass[2];
    NiPointer<NiD3DVertexShader> spVertexShader[2];
    NiPointer<NiD3DPixelShader> spPixelShader[2];

    static inline float fGlobalAlpha = 1.0f;
    static inline float fFlareMult = 1.0f;
    static NiRTTI ms_RTTI;
    static inline float fFlareAlpha;
    static NiPointer<NiTexture> spDefaultFlareTexture;
};
