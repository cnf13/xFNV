#include "BSMain/BSParentVelocityModifier.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleInfo.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

NiRTTI BSParentVelocityModifier::ms_RTTI("BSParentVelocityModifier", &NiPSysModifier::ms_RTTI);

// fTimeLast is deliberately left unset.
BSParentVelocityModifier::BSParentVelocityModifier()
  : fLastTimeDelta(0.0f), fDamping(0.0f), pntLast(NiPoint3::ZERO), velLast(NiPoint3::ZERO) {
    m_uiOrder = 3000;
}

// The base takes the name as a temporary NiFixedString, at order 3000; fTimeLast is deliberately left unset.
BSParentVelocityModifier::BSParentVelocityModifier(const char* apcName)
  : NiPSysModifier(NiFixedString(apcName), 3000u), fLastTimeDelta(0.0f), fDamping(0.0f), pntLast(NiPoint3::ZERO),
    velLast(NiPoint3::ZERO) {}

BSParentVelocityModifier::~BSParentVelocityModifier() {}

NiObject* BSParentVelocityModifier::CreateClone(NiCloningProcess& arCloneData) {
    BSParentVelocityModifier* v3;
    v3 = new BSParentVelocityModifier;
    CopyMembers(v3, arCloneData);
    return v3;
}

void BSParentVelocityModifier::LoadBinary(NiStream& arStream) {
    NiPSysModifier::LoadBinary(arStream);

    // The float is loaded through the 4-byte transfer NiStreamLoadBinary<uint32_t>, which moves four untyped bytes, so
    // the field's address is passed as its argument.
    NiStreamLoadBinary<uint32_t>(arStream, reinterpret_cast<uint32_t&>(fDamping));
}

void BSParentVelocityModifier::LinkObject(NiStream& arStream) {
    NiPSysModifier::LinkObject(arStream);
}

bool BSParentVelocityModifier::RegisterStreamables(NiStream& arStream) {
    return NiPSysModifier::RegisterStreamables(arStream);
}

void BSParentVelocityModifier::SaveBinary(NiStream& arStream) {
    NiPSysModifier::SaveBinary(arStream);

    // The float is saved through NiAnimationKey::SaveBinary, which writes four untyped bytes at the key's start: the
    // field's address is passed as the key.
    NiAnimationKey::SaveBinary(arStream, reinterpret_cast<NiAnimationKey*>(&fDamping));
}

bool BSParentVelocityModifier::IsEqual(NiObject* apObject) {
    bool bEqual = NiPSysModifier::IsEqual(apObject);
    if (bEqual) {
        BSParentVelocityModifier* pkOther = static_cast<BSParentVelocityModifier*>(apObject);
        bEqual = pkOther->fDamping == fDamping;
    }
    return bEqual;
}

void BSParentVelocityModifier::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    NiPSysModifier::GetViewerStrings(apStrings);

    apStrings->Add(NiGetViewerString(BSParentVelocityModifier::ms_RTTI.GetName()));
    apStrings->Add(NiGetViewerString("Damping", fDamping));
}

// Measures how far the parent moved since the last update and adds the change of its damped velocity to every active
// particle. Without a parent, or on the first update after a reset, the history is restarted instead.
void BSParentVelocityModifier::Update(float afTime, NiPSysData* apData) {
    uint16_t usActive = apData->m_usActive;
    if (usActive == 0) {
        return;
    }

    NiNode* pkParent = m_pkTarget->m_pkParent;
    if (pkParent == nullptr) {
        HandleReset();
        return;
    }

    if (pntLast == NiPoint3::ZERO) {
        velLast = NiPoint3::ZERO;
        fTimeLast = afTime;
        pntLast = pkParent->m_kWorld.m_Translate;
    }

    uint32_t uiCount = apData->m_usActive;
    NiPoint3 kPos = pkParent->m_kWorld.m_Translate;
    float fDelta = afTime - fTimeLast;
    if (fDelta != 0.0) {
        NiPoint3 kVel = (fDamping * (kPos - pntLast)) / fDelta;
        NiPoint3 kDiff = kVel - velLast;
        if (kDiff != NiPoint3::ZERO) {
            NiParticleInfo* pkInfo = apData->m_pkParticleInfo;
            while (uiCount--) {
                pkInfo->m_kVelocity += kDiff;
                ++pkInfo;
            }
        }
        velLast = kVel;
    }
    fLastTimeDelta = fDelta;
    fTimeLast = afTime;
    pntLast = kPos;
}

// A new particle starts with the parent's velocity, and its position is wound back by that velocity over the last
// time step.
void BSParentVelocityModifier::Initialize(NiPSysData* apData, uint16_t aiNewParticle) {
    NiParticleInfo* pkInfo = apData->m_pkParticleInfo + aiNewParticle;
    pkInfo->m_kVelocity += velLast;

    NiPoint3* pkVertices = apData->m_pkVertex;
    pkVertices[aiNewParticle] -= velLast * fLastTimeDelta;
}

void BSParentVelocityModifier::HandleReset() {
    pntLast = NiPoint3::ZERO;
}

void BSParentVelocityModifier::CopyMembers(BSParentVelocityModifier* apDest, NiCloningProcess& arCloneData) {
    NiPSysModifier::CopyMembers(apDest, arCloneData);
    apDest->fDamping = fDamping;
}

NiObject* BSParentVelocityModifier::CreateObject() {
    return new BSParentVelocityModifier;
}
