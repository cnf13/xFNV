#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysGravityModifier.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticleInfo.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

NiRTTI NiPSysGravityModifier::ms_RTTI("NiPSysGravityModifier", &NiPSysModifier::ms_RTTI);

NiPSysGravityModifier::NiPSysGravityModifier()
  : m_pkGravityObj(nullptr), m_kGravityAxis(NiPoint3_UNIT_X), m_fDecay(0.0f), m_fStrength(1.0f), m_eType(FORCE_PLANAR),
    m_fTurbulence(0.0f), m_fScale(1.0f) {}

NiPSysGravityModifier::~NiPSysGravityModifier() {}

NiObject* NiPSysGravityModifier::CreateClone(NiCloningProcess& kCloning) {
    NiPSysGravityModifier* v4;
    void* v3 = NiMemObject::operator new(sizeof(NiPSysGravityModifier));
    if (v3) {
        v4 = ::new (v3) NiPSysGravityModifier;
    }
    else {
        v4 = nullptr;
    }
    NiPSysModifier::CopyMembers(v4, kCloning);
    v4->m_kGravityAxis.x = m_kGravityAxis.x;
    v4->m_kGravityAxis.y = m_kGravityAxis.y;
    v4->m_kGravityAxis.z = m_kGravityAxis.z;
    v4->m_fDecay = m_fDecay;
    v4->m_fStrength = m_fStrength;
    v4->m_eType = static_cast<NiPSysGravityModifier::ForceType>((uint32_t)m_eType);
    v4->m_fTurbulence = m_fTurbulence;
    v4->m_fScale = m_fScale;
    v4->m_bWorldAligned = (bool)((uint8_t)m_bWorldAligned);
    return v4;
}

// Turbulence and its scale exist from 10.0.1.14; the world-aligned flag only in Bethesda stream versions after 20. A
// short read of the force type leaves m_eType unwritten.
void NiPSysGravityModifier::LoadBinary(NiStream& kStream) {
    NiPSysModifier::LoadBinary(kStream);

    kStream.ReadLinkID();
    m_kGravityAxis.LoadBinary(kStream);
    NiStreamLoadBinary(kStream, m_fDecay);
    NiStreamLoadBinary(kStream, m_fStrength);
    uint32_t uiType;
    NiStreamLoadBinary(kStream, uiType);
    m_eType = static_cast<ForceType>(uiType);
    if (kStream.GetFileVersion() >= NiStream::GetVersion(10, 0, 1, 14)) {
        NiStreamLoadBinary(kStream, m_fTurbulence);
        NiStreamLoadBinary(kStream, m_fScale);
    }
    if (kStream.m_BSStreamHeader.uiVersion > 0x14) {
        NiStreamLoadBinary(kStream, m_bWorldAligned);
    }
}

void NiPSysGravityModifier::LinkObject(NiStream& kStream) {
    NiPSysModifier::LinkObject(kStream);

    m_pkGravityObj = static_cast<NiAVObject*>(kStream.GetObjectFromLinkID());
}

bool NiPSysGravityModifier::RegisterStreamables(NiStream& kStream) {
    return NiPSysModifier::RegisterStreamables(kStream);
}

// Every field is written whatever the version.
void NiPSysGravityModifier::SaveBinary(NiStream& kStream) {
    NiPSysModifier::SaveBinary(kStream);

    kStream.SaveLinkID(m_pkGravityObj);
    m_kGravityAxis.SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_fDecay);
    NiStreamSaveBinary(kStream, m_fStrength);
    NiStreamSaveBinary(kStream, m_eType);
    NiStreamSaveBinary(kStream, m_fTurbulence);
    NiStreamSaveBinary(kStream, m_fScale);
    NiStreamSaveBinary(kStream, m_bWorldAligned);
}

bool NiPSysGravityModifier::IsEqual(NiObject* pkObject) {
    if (!NiPSysModifier::IsEqual(pkObject)) {
        return false;
    }

    NiPSysGravityModifier* pkOther = static_cast<NiPSysGravityModifier*>(pkObject);
    if ((m_pkGravityObj && !pkOther->m_pkGravityObj) || (!m_pkGravityObj && pkOther->m_pkGravityObj)) {
        return false;
    }
    if (m_pkGravityObj && pkOther->m_pkGravityObj && !m_pkGravityObj->IsEqual(pkOther->m_pkGravityObj)) {
        return false;
    }
    if (pkOther->m_kGravityAxis != m_kGravityAxis) {
        return false;
    }
    if (m_fDecay != pkOther->m_fDecay) {
        return false;
    }
    if (m_fStrength != pkOther->m_fStrength) {
        return false;
    }
    if (m_eType != pkOther->m_eType) {
        return false;
    }
    if (m_fTurbulence != pkOther->m_fTurbulence) {
        return false;
    }
    if (m_fScale != pkOther->m_fScale) {
        return false;
    }
    return m_bWorldAligned == pkOther->m_bWorldAligned;
}

void NiPSysGravityModifier::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiPSysModifier::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(NiPSysGravityModifier::ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("Gravity Object", m_pkGravityObj ? (const char*)m_pkGravityObj->m_kName : "None"));
    pkStrings->Add(m_kGravityAxis.GetViewerString("Gravity Axis"));
    pkStrings->Add(NiGetViewerString("Decay", m_fDecay));
    pkStrings->Add(NiGetViewerString("Strength", m_fStrength));
    pkStrings->Add(NiGetViewerString("Turbulence", m_fTurbulence));
    pkStrings->Add(NiGetViewerString("TurbulenceScale", m_fScale));

    const char* pcType;
    switch (m_eType) {
        case FORCE_PLANAR: pcType = "FORCE_PLANAR"; break;
        case FORCE_SPHERICAL: pcType = "FORCE_SPHERICAL"; break;
        default: pcType = "Unknown"; break;
    }
    pkStrings->Add(NiGetViewerString("Force Type", pcType));
    pkStrings->Add(NiGetViewerString("WorldAligned", m_bWorldAligned));
}

void NiPSysGravityModifier::ProcessClone(NiCloningProcess& kCloning) {
    NiPSysModifier::ProcessClone(kCloning);

    NiObject* pkObject = nullptr;
    kCloning.m_pkCloneMap->GetAt(this, pkObject);
    NiPSysGravityModifier* pkClone = static_cast<NiPSysGravityModifier*>(pkObject);
    if (m_pkGravityObj) {
        if (kCloning.m_pkCloneMap->GetAt(m_pkGravityObj, pkObject)) {
            pkClone->m_pkGravityObj = static_cast<NiAVObject*>(pkObject);
        }
        else {
            pkClone->m_pkGravityObj = m_pkGravityObj;
        }
    }
}

// Works in the particle system's space. The axis is taken through the gravity object's local rotation when world
// aligned, else through the gravity-to-system rotation, and unitized. The strength is scaled by the float 1.6 widened
// to double; turbulence is a symmetric random per component (x, y, z draws) times scale * turbulence * 500. Decay
// is exp(-decay * |distance|), the distance being the signed offset along the axis for planar gravity and the
// distance to the gravity object for spherical gravity. Spherical gravity with both decay and turbulence pulls along
// the axis, not toward the object. The active count and the array pointers are read once.
void NiPSysGravityModifier::Update(float fTime, NiPSysData* pkData) {
    if (!pkData->m_usActive) {
        return;
    }
    if (!m_pkGravityObj) {
        return;
    }

    NiTransform kGravity = m_pkGravityObj->m_kWorld;
    NiTransform kPSys = m_pkTarget->m_kWorld;
    NiTransform kInvPSys;
    kPSys.Invert(kInvPSys);
    NiTransform kGravityToPSys = kInvPSys * kGravity;

    NiPoint3 kDir;
    if (m_bWorldAligned) {
        kDir = m_pkGravityObj->m_kLocal.m_Rotate * m_kGravityAxis;
    }
    else {
        kDir = kGravityToPSys.m_Rotate * m_kGravityAxis;
    }
    kDir.Unitize();

    float fStrength = m_fStrength * static_cast<double>(1.6f);
    uint16_t usActive = pkData->m_usActive;

    switch (m_eType) {
        case FORCE_PLANAR:
            if (m_fDecay == 0.0f && m_fTurbulence == 0.0f) {
                NiParticleInfo* pkInfo = pkData->m_pkParticleInfo;
                for (uint16_t i = 0; i < usActive; i++, pkInfo++) {
                    float fScale = (fTime - pkInfo->m_fLastUpdate) * fStrength;
                    float fAddX = kDir.x * fScale;
                    float fAddY = kDir.y * fScale;
                    float fAddZ = kDir.z * fScale;
                    pkInfo->m_kVelocity.x += fAddX;
                    pkInfo->m_kVelocity.y += fAddY;
                    pkInfo->m_kVelocity.z += fAddZ;
                }
            }
            else if (m_fDecay == 0.0f && m_fTurbulence != 0.0f) {
                NiParticleInfo* pkInfo = pkData->m_pkParticleInfo;
                float fTurbulence = m_fScale * m_fTurbulence * 500.0;
                float fGravX = kDir.x * fStrength;
                float fGravY = kDir.y * fStrength;
                float fGravZ = kDir.z * fStrength;
                for (uint16_t i = 0; i < usActive; i++, pkInfo++) {
                    float fRandX = NiSymmetricRandom_6397C0() * fTurbulence;
                    float fRandY = NiSymmetricRandom_6397C0() * fTurbulence;
                    float fRandZ = NiSymmetricRandom_6397C0() * fTurbulence;
                    float fSumX = fGravX + fRandX;
                    float fSumY = fGravY + fRandY;
                    float fSumZ = fGravZ + fRandZ;
                    float fDeltaT = fTime - pkInfo->m_fLastUpdate;
                    float fAddX = fSumX * fDeltaT;
                    float fAddY = fSumY * fDeltaT;
                    float fAddZ = fSumZ * fDeltaT;
                    pkInfo->m_kVelocity.x += fAddX;
                    pkInfo->m_kVelocity.y += fAddY;
                    pkInfo->m_kVelocity.z += fAddZ;
                }
            }
            else if (m_fDecay != 0.0f && m_fTurbulence == 0.0f) {
                NiParticleInfo* pkInfo = pkData->m_pkParticleInfo;
                NiPoint3* pkPosition = pkData->m_pkVertex;
                for (uint16_t i = 0; i < usActive; i++, pkInfo++, pkPosition++) {
                    float fDiffX = kGravityToPSys.m_Translate.x - pkPosition->x;
                    float fDiffY = kGravityToPSys.m_Translate.y - pkPosition->y;
                    float fDiffZ = kGravityToPSys.m_Translate.z - pkPosition->z;
                    float fDist = fDiffX * kDir.x + fDiffY * kDir.y + fDiffZ * kDir.z;
                    float fExponent;
                    if (fDist < 0.0f) {
                        fExponent = fDist * m_fDecay;
                    }
                    else {
                        fExponent = fDist * -m_fDecay;
                    }
                    float fDecay = static_cast<float>(std::exp(fExponent));
                    float fScale = fDecay * fStrength * (fTime - pkInfo->m_fLastUpdate);
                    float fAddX = kDir.x * fScale;
                    float fAddY = kDir.y * fScale;
                    float fAddZ = kDir.z * fScale;
                    pkInfo->m_kVelocity.x += fAddX;
                    pkInfo->m_kVelocity.y += fAddY;
                    pkInfo->m_kVelocity.z += fAddZ;
                }
            }
            else if (m_fDecay != 0.0f && m_fTurbulence != 0.0f) {
                NiParticleInfo* pkInfo = pkData->m_pkParticleInfo;
                NiPoint3* pkPosition = pkData->m_pkVertex;
                float fTurbulence = m_fScale * m_fTurbulence * 500.0;
                float fGravX = kDir.x * fStrength;
                float fGravY = kDir.y * fStrength;
                float fGravZ = kDir.z * fStrength;
                for (uint16_t i = 0; i < usActive; i++, pkInfo++, pkPosition++) {
                    float fRandX = NiSymmetricRandom_6397C0() * fTurbulence;
                    float fRandY = NiSymmetricRandom_6397C0() * fTurbulence;
                    float fRandZ = NiSymmetricRandom_6397C0() * fTurbulence;
                    float fDiffX = kGravityToPSys.m_Translate.x - pkPosition->x;
                    float fDiffY = kGravityToPSys.m_Translate.y - pkPosition->y;
                    float fDiffZ = kGravityToPSys.m_Translate.z - pkPosition->z;
                    float fDist = fDiffX * kDir.x + fDiffY * kDir.y + fDiffZ * kDir.z;
                    float fExponent;
                    if (fDist < 0.0f) {
                        fExponent = fDist * m_fDecay;
                    }
                    else {
                        fExponent = fDist * -m_fDecay;
                    }
                    float fDecay = static_cast<float>(std::exp(fExponent));
                    float fDecayX = fGravX * fDecay;
                    float fDecayY = fGravY * fDecay;
                    float fDecayZ = fGravZ * fDecay;
                    float fSumX = fDecayX + fRandX;
                    float fSumY = fDecayY + fRandY;
                    float fSumZ = fDecayZ + fRandZ;
                    float fDeltaT = fTime - pkInfo->m_fLastUpdate;
                    float fAddX = fSumX * fDeltaT;
                    float fAddY = fSumY * fDeltaT;
                    float fAddZ = fSumZ * fDeltaT;
                    pkInfo->m_kVelocity.x += fAddX;
                    pkInfo->m_kVelocity.y += fAddY;
                    pkInfo->m_kVelocity.z += fAddZ;
                }
            }
            break;
        case FORCE_SPHERICAL:
            if (m_fDecay == 0.0f && m_fTurbulence == 0.0f) {
                NiParticleInfo* pkInfo = pkData->m_pkParticleInfo;
                NiPoint3* pkPosition = pkData->m_pkVertex;
                for (uint16_t i = 0; i < usActive; i++, pkInfo++, pkPosition++) {
                    NiPoint3 kToCenter;
                    kToCenter.x = kGravityToPSys.m_Translate.x - pkPosition->x;
                    kToCenter.y = kGravityToPSys.m_Translate.y - pkPosition->y;
                    kToCenter.z = kGravityToPSys.m_Translate.z - pkPosition->z;
                    kToCenter.Unitize();
                    float fScale = (fTime - pkInfo->m_fLastUpdate) * fStrength;
                    float fAddX = kToCenter.x * fScale;
                    float fAddY = kToCenter.y * fScale;
                    float fAddZ = kToCenter.z * fScale;
                    pkInfo->m_kVelocity.x += fAddX;
                    pkInfo->m_kVelocity.y += fAddY;
                    pkInfo->m_kVelocity.z += fAddZ;
                }
            }
            else if (m_fDecay == 0.0f && m_fTurbulence != 0.0f) {
                NiParticleInfo* pkInfo = pkData->m_pkParticleInfo;
                NiPoint3* pkPosition = pkData->m_pkVertex;
                float fTurbulence = m_fScale * m_fTurbulence * 500.0;
                for (uint16_t i = 0; i < usActive; i++, pkInfo++, pkPosition++) {
                    NiPoint3 kToCenter;
                    kToCenter.x = kGravityToPSys.m_Translate.x - pkPosition->x;
                    kToCenter.y = kGravityToPSys.m_Translate.y - pkPosition->y;
                    kToCenter.z = kGravityToPSys.m_Translate.z - pkPosition->z;
                    kToCenter.Unitize();
                    float fRandX = NiSymmetricRandom_6397C0() * fTurbulence;
                    float fRandY = NiSymmetricRandom_6397C0() * fTurbulence;
                    float fRandZ = NiSymmetricRandom_6397C0() * fTurbulence;
                    float fGravX = kToCenter.x * fStrength;
                    float fGravY = kToCenter.y * fStrength;
                    float fGravZ = kToCenter.z * fStrength;
                    float fSumX = fGravX + fRandX;
                    float fSumY = fGravY + fRandY;
                    float fSumZ = fGravZ + fRandZ;
                    float fDeltaT = fTime - pkInfo->m_fLastUpdate;
                    float fAddX = fSumX * fDeltaT;
                    float fAddY = fSumY * fDeltaT;
                    float fAddZ = fSumZ * fDeltaT;
                    pkInfo->m_kVelocity.x += fAddX;
                    pkInfo->m_kVelocity.y += fAddY;
                    pkInfo->m_kVelocity.z += fAddZ;
                }
            }
            else if (m_fDecay != 0.0f && m_fTurbulence == 0.0f) {
                NiParticleInfo* pkInfo = pkData->m_pkParticleInfo;
                NiPoint3* pkPosition = pkData->m_pkVertex;
                for (uint16_t i = 0; i < usActive; i++, pkInfo++, pkPosition++) {
                    NiPoint3 kToCenter;
                    kToCenter.x = kGravityToPSys.m_Translate.x - pkPosition->x;
                    kToCenter.y = kGravityToPSys.m_Translate.y - pkPosition->y;
                    kToCenter.z = kGravityToPSys.m_Translate.z - pkPosition->z;
                    float fExponent = kToCenter.UnitizeGetLength() * -m_fDecay;
                    float fDecay = static_cast<float>(std::exp(fExponent));
                    float fScale = fDecay * fStrength * (fTime - pkInfo->m_fLastUpdate);
                    float fAddX = kToCenter.x * fScale;
                    float fAddY = kToCenter.y * fScale;
                    float fAddZ = kToCenter.z * fScale;
                    pkInfo->m_kVelocity.x += fAddX;
                    pkInfo->m_kVelocity.y += fAddY;
                    pkInfo->m_kVelocity.z += fAddZ;
                }
            }
            else if (m_fDecay != 0.0f && m_fTurbulence != 0.0f) {
                NiParticleInfo* pkInfo = pkData->m_pkParticleInfo;
                NiPoint3* pkPosition = pkData->m_pkVertex;
                float fTurbulence = m_fScale * m_fTurbulence * 500.0;
                float fGravX = kDir.x * fStrength;
                float fGravY = kDir.y * fStrength;
                float fGravZ = kDir.z * fStrength;
                for (uint16_t i = 0; i < usActive; i++, pkInfo++, pkPosition++) {
                    float fRandX = NiSymmetricRandom_6397C0() * fTurbulence;
                    float fRandY = NiSymmetricRandom_6397C0() * fTurbulence;
                    float fRandZ = NiSymmetricRandom_6397C0() * fTurbulence;
                    NiPoint3 kToCenter;
                    kToCenter.x = kGravityToPSys.m_Translate.x - pkPosition->x;
                    kToCenter.y = kGravityToPSys.m_Translate.y - pkPosition->y;
                    kToCenter.z = kGravityToPSys.m_Translate.z - pkPosition->z;
                    float fExponent = kToCenter.UnitizeGetLength() * -m_fDecay;
                    float fDecay = static_cast<float>(std::exp(fExponent));
                    float fDecayX = fGravX * fDecay;
                    float fDecayY = fGravY * fDecay;
                    float fDecayZ = fGravZ * fDecay;
                    float fSumX = fDecayX + fRandX;
                    float fSumY = fDecayY + fRandY;
                    float fSumZ = fDecayZ + fRandZ;
                    float fDeltaT = fTime - pkInfo->m_fLastUpdate;
                    float fAddX = fSumX * fDeltaT;
                    float fAddY = fSumY * fDeltaT;
                    float fAddZ = fSumZ * fDeltaT;
                    pkInfo->m_kVelocity.x += fAddX;
                    pkInfo->m_kVelocity.y += fAddY;
                    pkInfo->m_kVelocity.z += fAddZ;
                }
            }
            break;
    }
}

NiObject* NiPSysGravityModifier::CreateObject() {
    void* v0 = NiMemObject::operator new(sizeof(NiPSysGravityModifier));
    if (v0) {
        return ::new (v0) NiPSysGravityModifier;
    }
    else {
        return nullptr;
    }
}
