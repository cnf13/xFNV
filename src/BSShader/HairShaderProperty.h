#pragma once

#include "BSShader/BSShaderPPLightingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"

// The per-pixel lit hair shader property: up to three hair lights, a tint color and a height map.
class HairShaderProperty : public BSShaderPPLightingProperty {
public:
    HairShaderProperty();
    ~HairShaderProperty() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject*) override {
        return false;
    }
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    bool SetupGeometry(NiGeometry* apGeometry) override;
    BSShaderProperty::RenderPassArray* GetRenderPasses(
        NiGeometry* apGeom,
        int32_t aiEnabledPasses,
        uint16_t& asCount,
        uint32_t auiRenderMode,
        BSShaderAccumulator* apAccumulator,
        int32_t abUpdateList
    ) override;
    uint16_t GetNumberofPasses(NiGeometry*) override {
        return 0;
    }

    // Copies this property's members into pDest.
    void CopyMembers(HairShaderProperty* pDest, NiCloningProcess& arCloneData);

    // The stream's factory: a property in a 0x180-byte engine block.
    static NiObject* CreateObject();

    NiPointer<ShadowSceneLight> spPrimaryLight;
    NiPointer<ShadowSceneLight> spPreviousPrimaryLight;
    NiPoint3 pHairLightPos[3];
    float fHairLightRadius[3];
    NiColorA pHairLightColor[3];
    // Hides BSShaderPPLightingProperty::pHairTintColor.
    NiColorA pHairTintColor;
    NiPointer<NiTexture> spHeightMap;

    static NiRTTI ms_RTTI;
};
