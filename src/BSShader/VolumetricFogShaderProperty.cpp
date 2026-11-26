#include "BSShader/VolumetricFogShaderProperty.h"

#include "BSShader/BSShaderProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI VolumetricFogShaderProperty::ms_RTTI("VolumetricFogShaderProperty", &BSShaderProperty::ms_RTTI);

// The fog is white with a density of 100.
VolumetricFogShaderProperty::VolumetricFogShaderProperty()
  : FogProps(NiColorA::WHITE.r, NiColorA::WHITE.g, NiColorA::WHITE.b, 100.0f) {
    iShaderPropertyType = 22;
}

VolumetricFogShaderProperty::~VolumetricFogShaderProperty() {}

// CopyMembers still runs when the allocation failed, with a null destination; the fog colour is
// copied afterwards, so it dereferences null there too.
NiObject* VolumetricFogShaderProperty::CreateClone(NiCloningProcess& arCloneData) {
    VolumetricFogShaderProperty* pClone = new VolumetricFogShaderProperty;
    CopyMembers(pClone, arCloneData);
    pClone->FogProps = FogProps;
    return pClone;
}

void VolumetricFogShaderProperty::LoadBinary(NiStream& kStream) {
    BSShaderProperty::LoadBinary(kStream);
    FogProps.LoadBinary(kStream);
}

void VolumetricFogShaderProperty::LinkObject(NiStream& kStream) {
    BSShaderProperty::LinkObject(kStream);
}

bool VolumetricFogShaderProperty::RegisterStreamables(NiStream& kStream) {
    return BSShaderProperty::RegisterStreamables(kStream);
}

void VolumetricFogShaderProperty::SaveBinary(NiStream& kStream) {
    BSShaderProperty::SaveBinary(kStream);
    FogProps.SaveBinary(kStream);
}

// The type of pkObject is not checked before the cast.
bool VolumetricFogShaderProperty::IsEqual(NiObject* pkObject) {
    if (static_cast<VolumetricFogShaderProperty*>(pkObject)->FogProps != FogProps) {
        return false;
    }
    return BSShaderProperty::IsEqual(pkObject);
}

void VolumetricFogShaderProperty::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    BSShaderProperty::GetViewerStrings(apStrings);
    apStrings->Add(NiGetViewerString(VolumetricFogShaderProperty::ms_RTTI.GetName()));
    apStrings->Add(FogProps.GetViewerString("FogProps"));
}

void VolumetricFogShaderProperty::CopyMembers(VolumetricFogShaderProperty* pDest, NiCloningProcess& arCloneData) {
    NiShadeProperty::CopyMembers(pDest, arCloneData);
    pDest->ulFlags[0] = ulFlags[0];
    pDest->ulFlags[1] = ulFlags[1];
    pDest->fAlpha = fAlpha;
    double fFadeAlpha = this->fFadeAlpha;
    pDest->iLastRenderPassState = 0;
    pDest->fFadeAlpha = fFadeAlpha;
    pDest->iShader = iShader;
    pDest->fDepthBias = fDepthBias;
}

NiObject* VolumetricFogShaderProperty::CreateObject() {
    return new VolumetricFogShaderProperty;
}
