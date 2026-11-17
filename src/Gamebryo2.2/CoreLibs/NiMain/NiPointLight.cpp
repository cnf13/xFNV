#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDynamicEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiPointLight::ms_RTTI("NiPointLight", &NiLight::ms_RTTI);

NiPointLight::NiPointLight() : m_fAtten0(0.0f), m_fAtten1(1.0f), m_fAtten2(0.0f) {
    m_ucEffectType = NiDynamicEffect::POINT_LIGHT;
}

NiPointLight::~NiPointLight() {}

const NiRTTI* NiPointLight::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* NiPointLight::CreateClone(NiCloningProcess& kCloning) {
    NiPointLight* pkObject_2 = static_cast<NiPointLight*>(NiMemObject::operator new(sizeof(NiPointLight)));
    if (pkObject_2) {
        ::new (pkObject_2) NiPointLight;
    }
    else {
        pkObject_2 = nullptr;
    }
    NiLight::CopyMembers(pkObject_2, kCloning);
    pkObject_2->m_fAtten0 = m_fAtten0;
    pkObject_2->m_fAtten1 = m_fAtten1;
    pkObject_2->m_fAtten2 = m_fAtten2;
    return pkObject_2;
}

void NiPointLight::LoadBinary(NiStream& stream) {
    NiLight::LoadBinary(stream);
    NiStreamLoadBinary(stream, m_fAtten0);
    NiStreamLoadBinary(stream, m_fAtten1);
    NiStreamLoadBinary(stream, m_fAtten2);
}

void NiPointLight::LinkObject(NiStream& stream) {
    NiLight::LinkObject(stream);
}

bool NiPointLight::RegisterStreamables(NiStream& stream) {
    return NiLight::RegisterStreamables(stream);
}

void NiPointLight::SaveBinary(NiStream& stream) {
    NiLight::SaveBinary(stream);
    NiStreamSaveBinary(stream, m_fAtten0);
    NiStreamSaveBinary(stream, m_fAtten1);
    NiStreamSaveBinary(stream, m_fAtten2);
}

bool NiPointLight::IsEqual(NiObject* pObject) {
    if (!NiLight::IsEqual(pObject)) {
        return false;
    }

    NiPointLight* pkLight = static_cast<NiPointLight*>(pObject);
    return m_fAtten0 == pkLight->m_fAtten0 && m_fAtten1 == pkLight->m_fAtten1 && m_fAtten2 == pkLight->m_fAtten2;
}

void NiPointLight::GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) {
    NiLight::GetViewerStrings(pStrings);
    pStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pStrings->Add(NiGetViewerString("m_fAtten0", m_fAtten0));
    pStrings->Add(NiGetViewerString("m_fAtten1", m_fAtten1));
    pStrings->Add(NiGetViewerString("m_fAtten2", m_fAtten2));
}

// Position changes reach the renderer through the revision only.
void NiPointLight::UpdateWorldData(NiUpdateData& UpdateData) {
    NiAVObject::UpdateWorldData(UpdateData);
    m_uiRevID++;
}

void NiPointLight::SetConstantAttenuation(float fC) {
    m_fAtten0 = fC;
    NiDynamicEffect::IncRevisionID();
}

void NiPointLight::SetLinearAttenuation(float fL) {
    m_fAtten1 = fL;
    NiDynamicEffect::IncRevisionID();
}

void NiPointLight::SetQuadraticAttenuation(float fQ) {
    m_fAtten2 = fQ;
    NiDynamicEffect::IncRevisionID();
}

void NiPointLight::CopyMembers(NiPointLight* pDest, NiCloningProcess& kCloning) {
    NiLight::CopyMembers(pDest, kCloning);
    pDest->m_fAtten0 = m_fAtten0;
    pDest->m_fAtten1 = m_fAtten1;
    pDest->m_fAtten2 = m_fAtten2;
}

NiObject* NiPointLight::CreateObject() {
    void* pkObject = NiMemObject::operator new(sizeof(NiPointLight));
    if (!pkObject) {
        return nullptr;
    }
    return ::new (pkObject) NiPointLight;
}
