#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDynamicEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiLight::ms_RTTI("NiLight", &NiDynamicEffect::ms_RTTI);

NiLight::NiLight()
  : m_fDimmer(1.0f), m_kAmb(1.0f, 1.0f, 1.0f), m_kDiff(1.0f, 1.0f, 1.0f), m_kSpec(1.0f, 1.0f, 1.0f),
    m_pvRendererData(nullptr) {}

// The renderer drops what it holds for the light.
NiLight::~NiLight() {
    if (NiRenderer::ms_pkRenderer) {
        NiRenderer::ms_pkRenderer->PurgeEffect(this);
    }
}

const NiRTTI* NiLight::GetRTTI() const {
    return &ms_RTTI;
}

void NiLight::LoadBinary(NiStream& stream) {
    NiDynamicEffect::LoadBinary(stream);
    NiStreamLoadBinary(stream, m_fDimmer);
    m_kAmb.LoadBinary(stream);
    m_kDiff.LoadBinary(stream);
    m_kSpec.LoadBinary(stream);
}

void NiLight::LinkObject(NiStream& stream) {
    NiDynamicEffect::LinkObject(stream);
}

// NiScreenSpaceCamera's slot is the same function.
bool NiLight::RegisterStreamables(NiStream& stream) {
    return NiDynamicEffect::RegisterStreamables(stream);
}

void NiLight::SaveBinary(NiStream& stream) {
    NiDynamicEffect::SaveBinary(stream);
    NiStreamSaveBinary(stream, m_fDimmer);
    m_kAmb.SaveBinary(stream);
    m_kDiff.SaveBinary(stream);
    m_kSpec.SaveBinary(stream);
}

bool NiLight::IsEqual(NiObject* pObject) {
    if (!NiDynamicEffect::IsEqual(pObject)) {
        return false;
    }

    NiLight* pkLight = static_cast<NiLight*>(pObject);
    if (m_fDimmer != pkLight->m_fDimmer) {
        return false;
    }
    if (m_kAmb != pkLight->m_kAmb) {
        return false;
    }
    if (m_kDiff != pkLight->m_kDiff) {
        return false;
    }
    if (m_kSpec != pkLight->m_kSpec) {
        return false;
    }
    return true;
}

void NiLight::GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) {
    NiDynamicEffect::GetViewerStrings(pStrings);
    pStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pStrings->Add(NiGetViewerString("m_fDimmer", m_fDimmer));
    pStrings->Add(m_kAmb.GetViewerString("m_kAmb"));
    pStrings->Add(m_kDiff.GetViewerString("m_kDiff"));
    pStrings->Add(m_kSpec.GetViewerString("m_kSpec"));
}

void NiLight::SetAmbientColor(const NiColor& color) {
    m_kAmb = color;
    NiDynamicEffect::IncRevisionID();
}

void NiLight::SetDiffuseColor(const NiColor& color) {
    m_kDiff = color;
    NiDynamicEffect::IncRevisionID();
}

void NiLight::SetDimmer(float fDimmer) {
    m_fDimmer = fDimmer;
    NiDynamicEffect::IncRevisionID();
}

void NiLight::SetSpecularColor(const NiColor& color) {
    m_kSpec = color;
    NiDynamicEffect::IncRevisionID();
}

void NiLight::CopyMembers(NiLight* pDest, NiCloningProcess& kCloning) {
    NiDynamicEffect::CopyMembers(pDest, kCloning);
    pDest->m_fDimmer = m_fDimmer;
    pDest->m_kAmb = m_kAmb;
    pDest->m_kDiff = m_kDiff;
    pDest->m_kSpec = m_kSpec;
}
