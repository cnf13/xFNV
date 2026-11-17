#include "BSShader/SpeedTreeBillboardShaderProperty.h"

#include "BSShader/BSShaderPPLightingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI
    SpeedTreeBillboardShaderProperty::ms_RTTI("SpeedTreeBillboardShaderProperty", &BSShaderPPLightingProperty::ms_RTTI);

SpeedTreeBillboardShaderProperty::SpeedTreeBillboardShaderProperty() {
    iShader = 1;
    iShaderPropertyType = 11;
}

SpeedTreeBillboardShaderProperty::~SpeedTreeBillboardShaderProperty() {}

NiObject* SpeedTreeBillboardShaderProperty::CreateClone(NiCloningProcess& kCloning) {
    void* pMemory = NiMemObject::operator new(sizeof(SpeedTreeBillboardShaderProperty));
    SpeedTreeBillboardShaderProperty* pClone;
    if (pMemory) {
        // EXE-INLINED 0xBA8630
        pClone = ::new (pMemory) SpeedTreeBillboardShaderProperty;
    }
    else {
        pClone = nullptr;
    }
    CopyMembers(pClone, kCloning);
    return pClone;
}

void SpeedTreeBillboardShaderProperty::LoadBinary(NiStream& kStream) {
    BSShaderPPLightingProperty::LoadBinary(kStream);
}

void SpeedTreeBillboardShaderProperty::LinkObject(NiStream& kStream) {
    BSShaderPPLightingProperty::LinkObject(kStream);
}

bool SpeedTreeBillboardShaderProperty::RegisterStreamables(NiStream& kStream) {
    return BSShaderPPLightingProperty::RegisterStreamables(kStream);
}

void SpeedTreeBillboardShaderProperty::SaveBinary(NiStream& kStream) {
    BSShaderPPLightingProperty::SaveBinary(kStream);
}

bool SpeedTreeBillboardShaderProperty::IsEqual(NiObject* pkObject) {
    // The base result is discarded and a constant false is returned, as the game does.
    BSShaderPPLightingProperty::IsEqual(pkObject);
    return false;
}

void SpeedTreeBillboardShaderProperty::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    BSShaderPPLightingProperty::GetViewerStrings(apStrings);
    apStrings->Add(NiGetViewerString(SpeedTreeBillboardShaderProperty::ms_RTTI.GetName()));
}

BSShaderProperty* SpeedTreeBillboardShaderProperty::ClarifyShader(NiGeometry*, bool, bool) {
    // NiAVObject::UpdateProperties is not called when &apGeometry->m_PropertyState is null, because that address is
    // never null.
    if (!spTextures[0]->m_pObject) {
        iShader = 0;
    }
    return nullptr;
}

void SpeedTreeBillboardShaderProperty::CopyMembers(
    SpeedTreeBillboardShaderProperty* pDest,
    NiCloningProcess& arCloneData
) {
    BSShaderLightingProperty::CopyMembers(pDest, arCloneData);
    CopyToMembers(pDest);
}
