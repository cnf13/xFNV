#pragma once

#include "BSShader/BSShader.h"
#include "BSShader/BSShaderManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"

class NiD3DPass;
class NiD3DPixelShader;
class NiD3DVertexShader;
class NiDX9ShaderDeclaration;
class NiTriShape;

// The shader of laser and plasma beams: a core color with a glow, animated by a timer.
class BeamShader : public BSShader {
public:
    enum VertexConstants : int32_t {
        PAS_VC_WORLDVIEWTRANSFORM = 0x0,
        PAS_VC_VARS = 0x4,
        PAS_VC_EYEPOS = 0x8,
    };

    enum PixelConstants : int32_t {
        BS_PS_CORECOLOR = 0x0,
        BS_PS_GLOWCOLOR = 0x1,
    };

    BeamShader(NiDX9ShaderDeclaration* apShaderDecl);
    ~BeamShader() override;

    const NiRTTI* GetRTTI() const override;
    bool Initialize() override;
    void RecreateRendererData() override;
    void CreateConstantMaps() override;
    void ReloadShaders() override;
    void ClearPassStages() override;
    void SetCurrentPass(uint32_t aPass, bool aFirstPass) override;
    virtual void LoadShaders();
    virtual bool LoadStagesAndPasses();

    // Sets up the beam pass's stages.
    void PresetStages();

    // Releases spBaseGeometry.
    static void Free();
    static BSShader* CreateShader();

    BSShaderManager::eBSSMPassEnum ePass;
    float fTimer;
    float fVars[16];
    NiPointer<NiD3DPass> spBeamPass;
    NiPointer<NiD3DVertexShader> spBeamVertexShader;
    NiPointer<NiD3DPixelShader> spBeamPixelShader;

    static NiRTTI ms_RTTI;
    // The beams' shared geometry, released at exit.
    static NiPointer<NiTriShape> spBaseGeometry;
};
