#include "BSShader/SpeedTreeBranchShaderProperty.h"

#include "BSShader/BSShaderProperty.h"
#include "BSShader/SpeedTreeShaderProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAdditionalGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"

NiRTTI SpeedTreeBranchShaderProperty::ms_RTTI(
    "SpeedTreeBranchShaderProperty",
    &SpeedTreeShaderPPLightingProperty::ms_RTTI
);

SpeedTreeBranchShaderProperty::SpeedTreeBranchShaderProperty(STSPData* apData)
  : SpeedTreeShaderPPLightingProperty(apData) {
    iShader = 4;
    iShaderPropertyType = 10;
}

SpeedTreeBranchShaderProperty::~SpeedTreeBranchShaderProperty() {}

NiObject* SpeedTreeBranchShaderProperty::CreateClone(NiCloningProcess& arCloneData) {
    SpeedTreeBranchShaderProperty* pprop = new SpeedTreeBranchShaderProperty(GetData());
    CopyMembers(pprop, arCloneData);
    return pprop;
}

void SpeedTreeBranchShaderProperty::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    SpeedTreeShaderPPLightingProperty::GetViewerStrings(apStrings);
}

NiAdditionalGeometryData* SpeedTreeBranchShaderProperty::CreateRendererSpecificProperty(NiGeometry* apGeometry) {
    NiAdditionalGeometryData* pdata;
    if (!apGeometry) {
        return nullptr;
    }
    uint16_t m_usVertices = apGeometry->m_spModelData.m_pObject->m_usVertices;
    if (m_usVertices > GetVertexCount()) {
        return nullptr;
    }
    uint16_t usverts = apGeometry->m_spModelData.m_pObject->m_usVertices;
    pdata = new NiAdditionalGeometryData(usverts, 1u);
    pdata->SetDataBlockCount(1u);
    // SetDataBlock takes untyped bytes: the wind vectors are handed over as raw memory (as SpeedTreeLeafShaderProperty
    // does).
    uint8_t* v7 = reinterpret_cast<uint8_t*>(GetWindInfo());
    pdata->SetDataBlock(0, v7, 16 * usverts, false);
    pdata->SetDataStream(0, 0, 0, 4u, usverts, 16u, 16u);
    return pdata;
}

SpeedTreeBranchShaderProperty* SpeedTreeBranchShaderProperty::CreateCopy() {
    return new SpeedTreeBranchShaderProperty(GetData());
}

void SpeedTreeBranchShaderProperty::CopyMembers(SpeedTreeBranchShaderProperty* apDest, NiCloningProcess& arCloneData) {
    BSShaderLightingProperty::CopyMembers(apDest, arCloneData);
    CopyToMembers(apDest);
}
