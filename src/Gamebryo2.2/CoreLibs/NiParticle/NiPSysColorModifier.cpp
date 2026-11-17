#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysColorModifier.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiColorData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiColorKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticleInfo.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"

NiRTTI NiPSysColorModifier::ms_RTTI("NiPSysColorModifier", &NiPSysModifier::ms_RTTI);

NiPSysColorModifier::NiPSysColorModifier()
  : NiPSysModifier(), m_spColorData(), m_fLoKeyTime(0.0f), m_fHiKeyTime(0.0f) {}

NiPSysColorModifier::~NiPSysColorModifier() {}

NiObject* NiPSysColorModifier::CreateClone(NiCloningProcess& kCloning) {
    void* v3 = NiMemObject::operator new(sizeof(NiPSysColorModifier));
    NiPSysColorModifier* v4 = static_cast<NiPSysColorModifier*>(v3);
    if (v3) {
        ::new (v3) NiPSysColorModifier;
    }
    else {
        v4 = nullptr;
    }
    NiPSysModifier::CopyMembers(v4, kCloning);
    v4->SetColorData(m_spColorData.m_pObject);
    return v4;
}

void NiPSysColorModifier::LoadBinary(NiStream& kStream) {
    NiPSysModifier::LoadBinary(kStream);
    kStream.ReadLinkID();
}

// The link id is cast without a type check, as the game does.
void NiPSysColorModifier::LinkObject(NiStream& kStream) {
    NiPSysModifier::LinkObject(kStream);
    SetColorData(static_cast<NiColorData*>(kStream.GetObjectFromLinkID()));
}

bool NiPSysColorModifier::RegisterStreamables(NiStream& kStream) {
    if (!NiPSysModifier::RegisterStreamables(kStream)) {
        return false;
    }

    if (m_spColorData) {
        m_spColorData->RegisterStreamables(kStream);
    }
    return true;
}

void NiPSysColorModifier::SaveBinary(NiStream& kStream) {
    NiPSysModifier::SaveBinary(kStream);
    kStream.SaveLinkID(m_spColorData);
}

// The color data is compared without a null test, as the game does.
bool NiPSysColorModifier::IsEqual(NiObject* pkObject) {
    if (!NiPSysModifier::IsEqual(pkObject)) {
        return false;
    }

    NiPSysColorModifier* pkOther = static_cast<NiPSysColorModifier*>(pkObject);
    return m_spColorData->IsEqual(pkOther->m_spColorData);
}

// The color data adds its own strings; it is not tested for null.
void NiPSysColorModifier::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiPSysModifier::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(NiPSysColorModifier::ms_RTTI.GetName()));
    m_spColorData->GetViewerStrings(pkStrings);
}

// Each particle's color is the key value at its age over its life span, clamped to the key time range. Every
// particle samples from the first key (the last index restarts at 0). Without a color array nothing is done; the
// color data is not tested for null.
void NiPSysColorModifier::Update(float fTime, NiPSysData* pkData) {
    NiColorA* pkColor = pkData->m_pkColor;
    NiParticleInfo* pkInfo = pkData->m_pkParticleInfo;
    if (!pkColor) {
        return;
    }

    uint16_t usActive = pkData->m_usActive;
    uint32_t uiNumKeys = m_spColorData->m_uiNumKeys;
    NiColorKey::KeyType eType = m_spColorData->m_eType;
    uint8_t ucKeySize = m_spColorData->m_ucKeySize;
    NiColorKey* pkKeys = m_spColorData->m_pkKeys;
    for (uint16_t i = 0; i < usActive; i++) {
        float fTime = pkInfo[i].m_fAge / pkInfo[i].m_fLifeSpan;
        fTime = fTime > m_fLoKeyTime ? fTime : m_fLoKeyTime;
        fTime = fTime < m_fHiKeyTime ? fTime : m_fHiKeyTime;
        uint32_t uiLastIdx = 0;
        pkColor[i] = NiColorKey::GenInterp(fTime, pkKeys, eType, uiNumKeys, uiLastIdx, ucKeySize);
    }
}

void NiPSysColorModifier::SetColorData(NiColorData* pkColorData) {
    m_spColorData = pkColorData;
    ResetKeyTimeExtrema();
}

void NiPSysColorModifier::ResetKeyTimeExtrema() {
    m_fLoKeyTime = 0.0f;
    m_fHiKeyTime = 0.0f;
    if (m_spColorData) {
        uint32_t uiNumKeys = m_spColorData->m_uiNumKeys;
        uint8_t ucKeySize = m_spColorData->m_ucKeySize;
        NiColorKey* pkKeys = m_spColorData->m_pkKeys;
        if (uiNumKeys) {
            m_fLoKeyTime = pkKeys->GetKeyAt(0, ucKeySize)->m_fTime;
            m_fHiKeyTime = pkKeys->GetKeyAt(uiNumKeys - 1, ucKeySize)->m_fTime;
        }
    }
}

NiObject* NiPSysColorModifier::CreateObject() {
    NiPSysColorModifier* pkObject =
        static_cast<NiPSysColorModifier*>(NiMemObject::operator new(sizeof(NiPSysColorModifier)));
    if (!pkObject) {
        return 0;
    }
    return ::new (pkObject) NiPSysColorModifier;
}
