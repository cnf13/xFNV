#pragma once

#include "BSShader/BSRenderedTexture.h"
#include "BSShader/BSShaderProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Xenon/NiPoint4.h"

class NiGeometry;
class NiLight;

// The property that selects the water shader for a water surface.
class WaterShaderProperty : public BSShaderProperty {
public:
    WaterShaderProperty();
    ~WaterShaderProperty() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    BSShaderProperty::RenderPassArray* GetRenderPasses(
        NiGeometry* apGeom,
        int32_t aiEnabledPasses,
        uint16_t& asCount,
        uint32_t auiRenderMode,
        BSShaderAccumulator* apAccumulator,
        int32_t abUpdateList
    ) override;
    void GetTextureUse(void (*apTextureUseCB)(NiTexture*, uint32_t, void*), void* apParam) override;

protected:
    // Copies the water members into pDest while cloning.
    void CopyMembers(WaterShaderProperty* pDest, NiCloningProcess& arCloneData);

public:
    BSShaderProperty::RenderPass* GetWaterPass(NiGeometry* apGeometry);
    void Copy(WaterShaderProperty* pDest);

    static NiObject* CreateObject();

    bool bDisplacement;
    bool bLOD;
    bool bFullReflections;
    bool bDepth;
    int32_t uwTexOffsetX;
    int32_t uwTexOffsetY;
    float fBlendRadius;
    float fBlendNormalsAmount;
    float fFogFar;
    float fFogRange;
    bool bIsMoving;
    bool bInWater;
    bool bUnderWater;
    bool bUpdateConstants;
    bool bReflections;
    bool bRefractions;
    bool bObjectTexCoords;
    bool bSpecularLighting;
    uint32_t iStencilMask;
    NiColorA pShallowColor;
    NiColorA pDeepColor;
    NiColorA pReflectionColor;
    // sun power, reflectivity, opacity, distortion
    NiPoint4 pVarAmounts;
    NiPoint4 pBlendRadius;
    NiPoint4 pDepthFalloff;
    NiPoint4 pDepthOffset;
    NiPoint4 pFresnelRI;
    NiColorA fTile;
    float fFresnelAmount;
    float fNoiseScale;
    float fFogAmount;
    float fUVScale;
    NiTPointerList<NiLight*> LightList;
    NiPointer<NiTexture> spNoiseHeightMap;
    NiPointer<BSRenderedTexture> spNoiseNormalMap;
    NiPointer<BSRenderedTexture> spReflectionMap;
    NiPointer<BSRenderedTexture> spRefractionMap;
    NiPointer<BSRenderedTexture> spDepthMap;
    NiPointer<NiRenderedTexture> spDisplacementNormalMap;
    BSShaderProperty::RenderPass* pWaterPass;

    static NiRTTI ms_RTTI;
    static inline bool bEnabled = true;
};
