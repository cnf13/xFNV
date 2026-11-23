#pragma once

#include "BSShader/BSShader.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/DistantLODGroup.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Xenon/NiPoint4.h"

class NiD3DPixelShader;
class NiD3DVertexShader;

class DistantLODShader : public BSShader {
public:
    DistantLODShader(NiDX9ShaderDeclaration* apShaderDecl);
    ~DistantLODShader() override;

    const NiRTTI* GetRTTI() const override;
    void RecreateRendererData() override;
    void CreateConstantMaps() override;
    void ReloadShaders() override;
    void ClearPassStages() override;
    void SetCurrentPass(uint32_t aPass, bool aFirstPass) override;

protected:
    virtual void LoadShaders();
    virtual bool LoadStagesAndPasses();

    void PresetStages();

public:
    static BSShader* CreateShader();

    NiPointer<NiD3DPass> spPass;
    float fWindTimer;
    BSShaderManager::eBSSMPassEnum ePass;
    bool bFirstPass;
    NiPointer<NiD3DVertexShader> spVertexShaders[4];
    NiPointer<NiD3DPixelShader> spPixelShaders[2];
    // iInstanceDataCount instances, allocated by the constructor, handed to the "instance data" entry.
    DistantLODGroup::DistantLODInstanceData* pDistantLODInstanceData;
    NiShaderConstantMapEntry* pDistantLODInstanceDataEntry;
    uint16_t iInstanceDataCount;
    NiColorA pfAmbientColor;
    NiPoint4 pfDiffuseDir;
    NiColorA pfDiffuseColor;
    NiColorA pfAlphaTestRef;
    NiPoint4 pEyeVector;
    NiPoint4 pEyePosition;

    static NiRTTI ms_RTTI;
};
