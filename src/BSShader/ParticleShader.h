#pragma once

#include "BSShader/BSShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Xenon/NiPoint4.h"

#include <stdint.h>

class NiD3DPixelShader;
class NiD3DVertexShader;
class NiDX9ShaderDeclaration;
class NiTriShape;
class ParticleShaderProperty;

class ParticleShader : public BSShader {
public:
    class ParticleData;

    ParticleShader(NiDX9ShaderDeclaration* apShaderDecl);
    ~ParticleShader() override;

    const NiRTTI* GetRTTI() const override;
    bool Initialize() override;
    void SetupGeometryTextures(const NiPropertyState* apState) override;
    void SetupGeometryConstants(const NiPropertyState* apState) override;
    void SetupGeometryAlphaBlending(const NiPropertyState* apState) override;
    void SetupGeometryRenderStates(const NiPropertyState* apState, bool abFirstPass) override;
    void RecreateRendererData() override;
    void InitPass(uint32_t auiCurrentPass) override;
    void SetupPassConstants(uint32_t auiCurrentPass) override;
    void CreateConstantMaps() override;
    void ReloadShaders() override;
    void ClearPassStages() override;
    void SetCurrentPass(uint32_t aPass, bool aFirstPass) override;

protected:
    virtual void LoadShaders();
    // Creates the missing passes and presets their stages; always true.
    virtual bool LoadStagesAndPasses();

    // Sets up the passes' texture stages and render states; called by LoadStagesAndPasses.
    void PresetStages();

public:
    // The instance count one batch holds: 26 at shader level below 2, else 78.
    static uint32_t GetMaxInstanceSize();
    // Starts the particle shader property of apObject under apRoot.
    static ParticleShaderProperty* Start(NiAVObject* apRoot, NiAVObject* apObject);
    // Stops apProp on apObject.
    static void Stop(NiAVObject* apObject, ParticleShaderProperty* apProp);
    // Releases the shared particle geometry.
    static void Free();
    // Recomputes the FOV-dependent particle scale statics from the
    // renderer's viewport.
    static void UpdateFov();
    static NiGeometry* GetParticleGeometry();
    static BSShader* CreateShader();

    BSShaderManager::eBSSMPassEnum ePass;
    ParticleData* pParticleData;
    NiPointer<NiD3DPass> spParticlePass;
    NiPointer<NiD3DPass> spParticlePrePass;
    NiPointer<NiD3DVertexShader> spParticleVertexShader;
    NiPointer<NiD3DPixelShader> spParticlePixelShader;
    NiPoint4 fVars[5];
    NiPoint3 Velocity;
    NiPoint3 Acceleration;
    NiPoint4 Wind;
    NiColorA Color1;
    NiColorA Color2;
    NiColorA Color3;

    static NiRTTI ms_RTTI;
    static NiPointer<NiTriShape> spBaseGeometry;
    // The cached GetMaxInstanceSize result (0 until first computed).
    static inline uint32_t uiMaxInstanceSize;
    // The vertex constant "Scale Adjustment".
    static inline NiPoint4 ScaleAdjust{1.0f, 1.0f, 1.0f, 1.0f};
    // The particle size scale for the current FOV, set by BSShaderManager::SetFOV.
    static inline float fFOVAdjust = 1.0f;
    static inline int32_t iNumParticles;
};

// One particle's instance constants (0x30 bytes, three registers).
class ParticleShader::ParticleData {
public:
    float fXPos;
    float fYPos;
    float fZPos;
    float fBirthTime;
    float fXVel;
    float fYVel;
    float fZVel;
    float fLifeAdjust;
    float fRotationStart;
    float fRotationSpeed;
    float fSpeedMult;
    float fTexCoords;
};
