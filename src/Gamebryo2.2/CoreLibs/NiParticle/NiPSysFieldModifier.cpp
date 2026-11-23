#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysFieldModifier.h"

#include "BSMain/BSParentVelocityModifier.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"

NiRTTI NiPSysFieldModifier::ms_RTTI("NiPSysFieldModifier", &NiPSysModifier::ms_RTTI);

NiPSysFieldModifier::NiPSysFieldModifier()
  : m_pkFieldObj(nullptr), m_fMagnitude(0.0f), m_fAttenuation(0.0f), m_bUseMaxDistance(false), m_fMaxDistance(0.0f),
    m_fMaxDistanceSqr(0.0f) {}

// The squared maximum distance is computed here and never streamed.
void NiPSysFieldModifier::LoadBinary(NiStream& kStream) {
    NiPSysModifier::LoadBinary(kStream);

    kStream.ReadLinkID();
    NiStreamLoadBinary(kStream, m_fMagnitude);
    NiStreamLoadBinary(kStream, m_fAttenuation);
    uint8_t ucUseMaxDistance;
    NiStreamLoadBinary(kStream, ucUseMaxDistance);
    m_bUseMaxDistance = ucUseMaxDistance != 0;
    NiStreamLoadBinary(kStream, m_fMaxDistance);
    m_fMaxDistanceSqr = m_fMaxDistance * m_fMaxDistance;
}

// NiPSysBombModifier, NiPSysDragModifier and NiPSysGravityModifier use the same function.
void NiPSysFieldModifier::LinkObject(NiStream& kStream) {
    NiPSysModifier::LinkObject(kStream);

    m_pkFieldObj = static_cast<NiAVObject*>(kStream.GetObjectFromLinkID());
}

bool NiPSysFieldModifier::RegisterStreamables(NiStream& kStream) {
    return NiPSysModifier::RegisterStreamables(kStream);
}

void NiPSysFieldModifier::SaveBinary(NiStream& kStream) {
    NiPSysModifier::SaveBinary(kStream);

    kStream.SaveLinkID(m_pkFieldObj);
    NiStreamSaveBinary(kStream, m_fMagnitude);
    NiStreamSaveBinary(kStream, m_fAttenuation);
    uint8_t ucUseMaxDistance = m_bUseMaxDistance;
    NiStreamSaveBinary(kStream, ucUseMaxDistance);
    NiStreamSaveBinary(kStream, m_fMaxDistance);
}

// The squared maximum distance is not compared.
bool NiPSysFieldModifier::IsEqual(NiObject* pkObject) {
    if (!NiPSysModifier::IsEqual(pkObject)) {
        return false;
    }

    NiPSysFieldModifier* pkOther = static_cast<NiPSysFieldModifier*>(pkObject);
    if ((m_pkFieldObj && !pkOther->m_pkFieldObj) || (!m_pkFieldObj && pkOther->m_pkFieldObj)) {
        return false;
    }
    if (m_pkFieldObj && pkOther->m_pkFieldObj && !m_pkFieldObj->IsEqual(pkOther->m_pkFieldObj)) {
        return false;
    }
    if (m_fMagnitude != pkOther->m_fMagnitude) {
        return false;
    }
    if (m_fAttenuation != pkOther->m_fAttenuation) {
        return false;
    }
    if (m_bUseMaxDistance != pkOther->m_bUseMaxDistance) {
        return false;
    }
    if (m_fMaxDistance != pkOther->m_fMaxDistance) {
        return false;
    }
    return true;
}

void NiPSysFieldModifier::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiPSysModifier::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(NiPSysFieldModifier::ms_RTTI.GetName()));
    pkStrings->Add(
        NiGetViewerString("Field Object", m_pkFieldObj ? static_cast<const char*>(m_pkFieldObj->m_kName) : "None")
    );
    pkStrings->Add(NiGetViewerString("Magnitude", m_fMagnitude));
    pkStrings->Add(NiGetViewerString("Attenuation", m_fAttenuation));
    pkStrings->Add(NiGetViewerString("Use Max Distance", m_bUseMaxDistance));
    pkStrings->Add(NiGetViewerString("Max Distance", m_fMaxDistance));
}

// NiPSysDragModifier and NiPSysGravityModifier use the same function.
void NiPSysFieldModifier::ProcessClone(NiCloningProcess& kCloning) {
    NiPSysModifier::ProcessClone(kCloning);

    NiObject* pkObject = nullptr;
    kCloning.m_pkCloneMap->GetAt(this, pkObject);
    NiPSysFieldModifier* pkClone = static_cast<NiPSysFieldModifier*>(pkObject);
    if (m_pkFieldObj) {
        if (kCloning.m_pkCloneMap->GetAt(m_pkFieldObj, pkObject)) {
            pkClone->m_pkFieldObj = static_cast<NiAVObject*>(pkObject);
        }
        else {
            pkClone->m_pkFieldObj = m_pkFieldObj;
        }
    }
}

void NiPSysFieldModifier::CopyMembers(NiPSysFieldModifier* pkDest, NiCloningProcess& kCloning) {
    NiPSysModifier::CopyMembers(pkDest, kCloning);
    pkDest->m_pkFieldObj = m_pkFieldObj;
    pkDest->m_fMagnitude = m_fMagnitude;
    pkDest->m_fAttenuation = m_fAttenuation;
    pkDest->m_bUseMaxDistance = m_bUseMaxDistance;
    pkDest->m_fMaxDistance = m_fMaxDistance;
    pkDest->m_fMaxDistanceSqr = m_fMaxDistanceSqr;
}
