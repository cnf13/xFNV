#include "Gamebryo2.2/CoreLibs/NiMain/NiSpotLight.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiSpotLight::ms_RTTI("NiSpotLight", &NiPointLight::ms_RTTI);

NiSpotLight::NiSpotLight()
  : m_kWorldDir(1.0f, 0.0f, 0.0f), m_fOuterSpotAngle(0.0f), m_fInnerSpotAngle(0.0f), m_fSpotExponent(1.0f) {
    m_ucEffectType = NiDynamicEffect::SPOT_LIGHT;
}

NiSpotLight::~NiSpotLight() {}

const NiRTTI* NiSpotLight::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* NiSpotLight::CreateClone(NiCloningProcess& kCloning) {
    NiSpotLight* v4 = new NiSpotLight;
    CopyMembers(v4, kCloning);
    return v4;
}

// Files before 20.2.0.5 carry no inner angle; it keeps its current value.
void NiSpotLight::LoadBinary(NiStream& stream) {
    NiPointLight::LoadBinary(stream);
    NiStreamLoadBinary(stream, m_fOuterSpotAngle);
    if (stream.GetFileVersion() >= NiStream::GetVersion(20, 2, 0, 5)) {
        NiStreamLoadBinary(stream, m_fInnerSpotAngle);
    }
    NiStreamLoadBinary(stream, m_fSpotExponent);
}

void NiSpotLight::LinkObject(NiStream& stream) {
    NiPointLight::LinkObject(stream);
}

bool NiSpotLight::RegisterStreamables(NiStream& stream) {
    return NiPointLight::RegisterStreamables(stream);
}

void NiSpotLight::SaveBinary(NiStream& stream) {
    NiPointLight::SaveBinary(stream);
    NiStreamSaveBinary(stream, m_fOuterSpotAngle);
    NiStreamSaveBinary(stream, m_fInnerSpotAngle);
    NiStreamSaveBinary(stream, m_fSpotExponent);
}

bool NiSpotLight::IsEqual(NiObject* pObject) {
    if (!NiPointLight::IsEqual(pObject)) {
        return false;
    }

    NiSpotLight* pkLight = static_cast<NiSpotLight*>(pObject);
    return m_fOuterSpotAngle == pkLight->m_fOuterSpotAngle && m_fSpotExponent == pkLight->m_fSpotExponent &&
        m_fInnerSpotAngle == pkLight->m_fInnerSpotAngle;
}

void NiSpotLight::GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) {
    NiPointLight::GetViewerStrings(pStrings);
    pStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pStrings->Add(m_kWorldDir.GetViewerString("m_kWorldDir"));
    pStrings->Add(NiGetViewerString("m_fOuterSpotAngle", m_fOuterSpotAngle));
    pStrings->Add(NiGetViewerString("m_fInnerSpotAngle", m_fInnerSpotAngle));
    pStrings->Add(NiGetViewerString("m_fSpotExponent", m_fSpotExponent));
}

// The light points along the world rotation's first column.
void NiSpotLight::UpdateWorldData(NiUpdateData& UpdateData) {
    NiPointLight::UpdateWorldData(UpdateData);
    m_kWorldDir.x = m_kWorld.m_Rotate.m_pEntry[0][0];
    m_kWorldDir.y = m_kWorld.m_Rotate.m_pEntry[1][0];
    m_kWorldDir.z = m_kWorld.m_Rotate.m_pEntry[2][0];
    m_uiRevID++;
}

void NiSpotLight::CopyMembers(NiSpotLight* pDest, NiCloningProcess& kCloning) {
    NiPointLight::CopyMembers(pDest, kCloning);
    pDest->m_kWorldDir = m_kWorldDir;
    pDest->m_fOuterSpotAngle = m_fOuterSpotAngle;
    pDest->m_fInnerSpotAngle = m_fInnerSpotAngle;
    pDest->m_fSpotExponent = m_fSpotExponent;
}

NiObject* NiSpotLight::CreateObject() {
    return new NiSpotLight;
}
