#pragma once

#include "BSShader/BSDistantTreeShader.h"
#include "BSShader/BSShaderProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"

// The shader property of the instanced distant trees.
class BSDistantTreeShaderProperty : public BSShaderProperty {
public:
    BSDistantTreeShaderProperty();
    ~BSDistantTreeShaderProperty() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    void PostLinkObject(NiStream& kStream) override;
    bool SetupGeometry(NiGeometry*) override {
        return true;
    }
    BSShaderProperty::RenderPassArray* GetRenderPasses(
        NiGeometry* apGeom,
        int32_t aiEnabledPasses,
        uint16_t& asCount,
        uint32_t auiRenderMode,
        BSShaderAccumulator* apAccumulator,
        int32_t abUpdateList
    ) override;

    // Writes the position, scale and rotation cosine and sine of instance auiInstanceID.
    void SetInstanceData(uint32_t auiInstanceID, NiPoint3 arPos, float fRotZ, float fScale);

protected:
    void CopyMembers(BSDistantTreeShaderProperty* pDest, NiCloningProcess& arCloneData);

public:
    // The stream's object factory: allocates a 0x68-byte property and constructs it.
    static NiObject* CreateObject();

    uint32_t uiInstanceCount;
    // uiInstanceCount instances (position, scale, rotation cosine and sine).
    BSDistantTreeShader::InstanceData* pInstancesA;

    static NiRTTI ms_RTTI;
};
