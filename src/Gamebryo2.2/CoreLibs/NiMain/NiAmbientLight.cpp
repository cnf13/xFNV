#include "Gamebryo2.2/CoreLibs/NiMain/NiAmbientLight.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiAmbientLight::ms_RTTI("NiAmbientLight", &NiLight::ms_RTTI);

NiAmbientLight::NiAmbientLight() {
    m_ucEffectType = NiDynamicEffect::AMBIENT_LIGHT;
}

NiAmbientLight::~NiAmbientLight() {}

const NiRTTI* NiAmbientLight::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* NiAmbientLight::CreateClone(NiCloningProcess& kCloning) {
    NiAmbientLight* v3 = new NiAmbientLight;
    CopyMembers(v3, kCloning);
    return v3;
}

// NiDirectionalLight's slot is the same function.
void NiAmbientLight::LoadBinary(NiStream& stream) {
    NiLight::LoadBinary(stream);
}

// NiDirectionalLight's and NiPointLight's slots are the same function.
void NiAmbientLight::LinkObject(NiStream& stream) {
    NiLight::LinkObject(stream);
}

// NiDirectionalLight's and NiPointLight's slots are the same function.
bool NiAmbientLight::RegisterStreamables(NiStream& stream) {
    return NiLight::RegisterStreamables(stream);
}

// NiDirectionalLight's slot is the same function.
void NiAmbientLight::SaveBinary(NiStream& stream) {
    NiLight::SaveBinary(stream);
}

// NiDirectionalLight's slot is the same function.
bool NiAmbientLight::IsEqual(NiObject* pObject) {
    return NiLight::IsEqual(pObject);
}

void NiAmbientLight::GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) {
    NiLight::GetViewerStrings(pStrings);
    pStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

NiObject* NiAmbientLight::CreateObject() {
    return new NiAmbientLight;
}
