#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysBombModifier.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticleInfo.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

#include <cmath>

NiRTTI NiPSysBombModifier::ms_RTTI("NiPSysBombModifier", &NiPSysModifier::ms_RTTI);

NiPSysBombModifier::NiPSysBombModifier()
  : NiPSysModifier(), m_pkBombObj(nullptr), m_kBombAxis(NiPoint3_UNIT_X), m_fDecay(0.0f), m_fDeltaV(0.0f),
    m_eDecayType(NiPSysBombModifier::NONE), m_eSymmType(NiPSysBombModifier::SPHERICAL) {}

NiPSysBombModifier::~NiPSysBombModifier() {}

NiObject* NiPSysBombModifier::CreateClone(NiCloningProcess& kCloning) {
    NiPSysBombModifier* pkObject =
        static_cast<NiPSysBombModifier*>(NiMemObject::operator new(sizeof(NiPSysBombModifier)));
    NiPSysBombModifier* pkObject_2 = pkObject;
    if (pkObject) {
        ::new (pkObject) NiPSysBombModifier;
    }
    else {
        pkObject_2 = 0;
    }
    CopyMembers(pkObject_2, kCloning);
    pkObject_2->m_kBombAxis = m_kBombAxis;
    pkObject_2->m_fDecay = m_fDecay;
    pkObject_2->m_fDeltaV = m_fDeltaV;
    pkObject_2->m_eDecayType = m_eDecayType;
    pkObject_2->m_eSymmType = m_eSymmType;
    return pkObject_2;
}

// The decay type and the symmetry type are each read into a local and stored; a short read leaves the local unwritten,
// so an uninitialised value is stored, as the game does.
void NiPSysBombModifier::LoadBinary(NiStream& kStream) {
    NiPSysModifier::LoadBinary(kStream);

    kStream.ReadLinkID();
    m_kBombAxis.LoadBinary(kStream);
    NiStreamLoadBinary(kStream, m_fDecay);
    NiStreamLoadBinary(kStream, m_fDeltaV);
    uint32_t uiDecayType;
    NiStreamLoadBinary(kStream, uiDecayType);
    m_eDecayType = static_cast<DecayType>(uiDecayType);
    uint32_t uiSymmType;
    NiStreamLoadBinary(kStream, uiSymmType);
    m_eSymmType = static_cast<SymmType>(uiSymmType);
}

void NiPSysBombModifier::LinkObject(NiStream& kStream) {
    NiPSysModifier::LinkObject(kStream);

    m_pkBombObj = static_cast<NiAVObject*>(kStream.GetObjectFromLinkID());
}

bool NiPSysBombModifier::RegisterStreamables(NiStream& kStream) {
    return NiPSysModifier::RegisterStreamables(kStream);
}

void NiPSysBombModifier::SaveBinary(NiStream& kStream) {
    NiPSysModifier::SaveBinary(kStream);

    kStream.SaveLinkID(m_pkBombObj);
    m_kBombAxis.SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_fDecay);
    NiStreamSaveBinary(kStream, m_fDeltaV);
    NiStreamSaveBinary(kStream, m_eDecayType);
    NiStreamSaveBinary(kStream, m_eSymmType);
}

bool NiPSysBombModifier::IsEqual(NiObject* pkObject) {
    if (!NiPSysModifier::IsEqual(pkObject)) {
        return false;
    }

    NiPSysBombModifier* pkOther = static_cast<NiPSysBombModifier*>(pkObject);
    if ((m_pkBombObj && !pkOther->m_pkBombObj) || (!m_pkBombObj && pkOther->m_pkBombObj)) {
        return false;
    }
    if (m_pkBombObj && pkOther->m_pkBombObj && !m_pkBombObj->IsEqual(pkOther->m_pkBombObj)) {
        return false;
    }
    if (m_kBombAxis != pkOther->m_kBombAxis) {
        return false;
    }
    if (m_fDecay != pkOther->m_fDecay) {
        return false;
    }
    if (m_fDeltaV != pkOther->m_fDeltaV) {
        return false;
    }
    if (m_eDecayType != pkOther->m_eDecayType) {
        return false;
    }
    if (m_eSymmType != pkOther->m_eSymmType) {
        return false;
    }
    return true;
}

void NiPSysBombModifier::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiPSysModifier::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(NiPSysBombModifier::ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("Bomb Object", m_pkBombObj ? (const char*)m_pkBombObj->m_kName : "None"));
    pkStrings->Add(m_kBombAxis.GetViewerString("Bomb Axis"));
    pkStrings->Add(NiGetViewerString("Decay", m_fDecay));
    pkStrings->Add(NiGetViewerString("DeltaV", m_fDeltaV));

    const char* pcDecayType;
    switch (m_eDecayType) {
        case NONE: pcDecayType = "NONE"; break;
        case LINEAR: pcDecayType = "LINEAR"; break;
        case EXPONENTIAL: pcDecayType = "EXPONENTIAL"; break;
        default: pcDecayType = "Unknown"; break;
    }
    pkStrings->Add(NiGetViewerString("Decay Type", pcDecayType));

    const char* pcSymmType;
    switch (m_eSymmType) {
        case SPHERICAL: pcSymmType = "SPHERICAL"; break;
        case CYLINDRICAL: pcSymmType = "CYLINDRICAL"; break;
        case PLANAR: pcSymmType = "PLANAR"; break;
        default: pcSymmType = "Unknown"; break;
    }
    pkStrings->Add(NiGetViewerString("Symmetry Type", pcSymmType));
}

// Unlike the field modifiers' ProcessClone, the bomb object is looked up in the clone map even when it is null.
void NiPSysBombModifier::ProcessClone(NiCloningProcess& kCloning) {
    NiPSysModifier::ProcessClone(kCloning);

    NiObject* pkObject = nullptr;
    kCloning.m_pkCloneMap->GetAt(this, pkObject);
    NiPSysBombModifier* pkClone = static_cast<NiPSysBombModifier*>(pkObject);
    if (kCloning.m_pkCloneMap->GetAt(m_pkBombObj, pkObject)) {
        pkClone->m_pkBombObj = static_cast<NiAVObject*>(pkObject);
    }
    else {
        pkClone->m_pkBombObj = m_pkBombObj;
    }
}

// Pushes each particle updated before afTime away from the bomb: DeltaV per second, scaled by the decay over the
// distance; particles beyond the decay distance are left alone unless the decay type is NONE. For a symmetry type other
// than the three, the push direction is left unwritten, so the particle gets the previous particle's direction, or
// uninitialized bytes for the first particle, as the game does.
void NiPSysBombModifier::Update(float fTime, NiPSysData* pkData) {
    NiPoint3 kTranslate = NiPoint3_ZERO;
    NiPoint3 kDir = m_kBombAxis;
    if (pkData->m_usActive && m_pkBombObj) {
        // The bomb object's transform in the particle system's space.
        NiTransform kBombXform = m_pkBombObj->m_kWorld;
        NiTransform kSystemXform = m_pkTarget->m_kWorld;
        NiTransform kInvSystemXform;
        kSystemXform.Invert(kInvSystemXform);
        NiTransform kTrans = kInvSystemXform * kBombXform;
        kTranslate = kTrans.m_Translate;
        kDir = kTrans.m_Rotate * m_kBombAxis;
        kDir.Unitize();
    }

    NiPoint3 kPush;
    for (uint16_t i = 0; i < pkData->m_usActive; i++) {
        NiParticleInfo* pkInfo = &pkData->m_pkParticleInfo[i];
        float fLastUpdate = pkInfo->m_fLastUpdate;
        if (!(fLastUpdate < fTime)) {
            continue;
        }

        float fDeltaT = fTime - fLastUpdate;
        NiPoint3 kDiff = pkData->m_pkVertex[i] - kTranslate;
        float fDist = std::sqrt(kDiff.SqrLength());
        DecayType eDecayType = m_eDecayType;
        // a NaN distance counts as beyond the decay distance
        if (eDecayType != NONE && !(m_fDecay >= fDist)) {
            continue;
        }

        switch (m_eSymmType) {
            case SPHERICAL: {
                // away from the bomb's centre, no test for a zero distance
                float fInvDist = 1.0f / fDist;
                kPush = kDiff * fInvDist;
                break;
            }
            case CYLINDRICAL: {
                // away from the bomb's axis
                float fDot = kDiff.Dot(kDir);
                kPush = kDiff - kDir * fDot;
                fDist = std::sqrt(kPush.SqrLength());
                if (fDist != 0.0f) {
                    float fInvDist = 1.0f / fDist;
                    kPush *= fInvDist;
                }
                break;
            }
            case PLANAR: {
                // away from the plane through the bomb, normal to its axis
                float fDot = kDiff.Dot(kDir);
                if (fDot < 0.0f) {
                    fDist = -fDot;
                    kPush = -kDir;
                }
                else {
                    fDist = fDot;
                    kPush = kDir;
                }
                break;
            }
        }

        float fDecay = 1.0f;
        if (eDecayType == LINEAR) {
            fDecay = (m_fDecay - fDist) / m_fDecay;
        }
        else if (eDecayType == EXPONENTIAL) {
            float fExponent = -fDist / m_fDecay;
            fDecay = std::exp(static_cast<double>(fExponent));
        }
        fDeltaT = m_fDeltaV * fDecay * fDeltaT;
        pkInfo->m_kVelocity += kPush * fDeltaT;
    }
}

NiObject* NiPSysBombModifier::CreateObject() {
    NiPSysBombModifier* pkObject =
        static_cast<NiPSysBombModifier*>(NiMemObject::operator new(sizeof(NiPSysBombModifier)));
    if (!pkObject) {
        return 0;
    }
    return ::new (pkObject) NiPSysBombModifier;
}
