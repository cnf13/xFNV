#pragma once

#include "BSShader/BSShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVector4.h"

class NiD3DPass;
class NiD3DPixelShader;
class NiD3DVertexShader;

// The shader that draws a SpeedTree model's fronds.
class SpeedTreeFrondShader : public BSShader {
public:
    SpeedTreeFrondShader();
    ~SpeedTreeFrondShader() override;

    const NiRTTI* GetRTTI() const override;
    bool SetupGeometry(NiGeometry* apGeometry) override;
    void CreateConstantMaps() override;
    void ReloadShaders() override;
    void ClearPassStages() override;
    virtual void LoadShaders();

    void LoadStagesAndPasses();

    static void RegisterRTTI();
    static BSShader* CreateShader();

    NiPointer<NiD3DVertexShader> pVertexShader[4];
    NiPointer<NiD3DPixelShader> pPixelShader[2];
    NiPointer<NiD3DPass> sp1StagePass;
    NiVector4 pLightPosition;
    NiVector4 pLightDirection;

    static NiRTTI ms_RTTI;
};
