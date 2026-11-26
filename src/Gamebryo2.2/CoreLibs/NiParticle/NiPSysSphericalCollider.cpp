#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysSphericalCollider.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleInfo.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysCollider.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysColliderManager.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysSpawnModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

#include <cmath>
#include <limits>

NiRTTI NiPSysSphericalCollider::ms_RTTI("NiPSysSphericalCollider", &NiPSysCollider::ms_RTTI);

NiPSysSphericalCollider::NiPSysSphericalCollider(
    float fBounce,
    bool bSpawnOnCollide,
    bool bDieOnCollide,
    NiPSysSpawnModifier* pkSpawnModifier,
    NiAVObject* pkColliderObj,
    float fRadius
)
  : NiPSysCollider(fBounce, bSpawnOnCollide, bDieOnCollide, pkSpawnModifier), m_pkColliderObj(pkColliderObj),
    m_fRadius(1.0f), m_fRadiusSq(1.0f), m_kPosition(NiPoint3::ZERO) {
    if (fRadius >= 0.0) {
        m_fRadius = fRadius;
    }
    m_kLastPSysTransform.m_Translate.x = -std::numeric_limits<float>::max();
    m_kLastColliderTransform.m_Translate.x = -std::numeric_limits<float>::max();
    m_kLastPSysTransform.m_Translate.y = -std::numeric_limits<float>::max();
    m_kLastColliderTransform.m_Translate.y = -std::numeric_limits<float>::max();
    m_kLastPSysTransform.m_Translate.z = -std::numeric_limits<float>::max();
    m_kLastColliderTransform.m_Translate.z = -std::numeric_limits<float>::max();
    m_kLastPSysTransform.m_Rotate = NiMatrix3::ZERO;
    m_kLastPSysTransform.m_fScale = -std::numeric_limits<float>::max();
    m_kLastColliderTransform.m_fScale = -std::numeric_limits<float>::max();
    m_kLastColliderTransform.m_Rotate = m_kLastPSysTransform.m_Rotate;
}

NiPSysSphericalCollider::~NiPSysSphericalCollider() {}

NiObject* NiPSysSphericalCollider::CreateClone(NiCloningProcess& kCloning) {
    NiPSysSphericalCollider* pkObject;
    pkObject = new NiPSysSphericalCollider(1.0f, false, false, nullptr, nullptr, 1.0f);
    CopyMembers(pkObject, kCloning);
    pkObject->m_fRadius = m_fRadius;
    return pkObject;
}

// The radius is only in files from 10.0.1.19 on; older files keep the default.
void NiPSysSphericalCollider::LoadBinary(NiStream& kStream) {
    NiPSysCollider::LoadBinary(kStream);
    kStream.ReadLinkID();
    if (kStream.GetFileVersion() >= NiStream::GetVersion(10, 0, 1, 19)) {
        NiStreamLoadBinary(kStream, m_fRadius);
    }
}

void NiPSysSphericalCollider::LinkObject(NiStream& kStream) {
    NiPSysCollider::LinkObject(kStream);
    m_pkColliderObj = static_cast<NiAVObject*>(kStream.GetObjectFromLinkID());
}

bool NiPSysSphericalCollider::RegisterStreamables(NiStream& kStream) {
    return NiPSysCollider::RegisterStreamables(kStream);
}

void NiPSysSphericalCollider::SaveBinary(NiStream& kStream) {
    NiPSysCollider::SaveBinary(kStream);
    kStream.SaveLinkID(m_pkColliderObj);
    NiStreamSaveBinary(kStream, m_fRadius);
}

// The collider objects are compared with IsEqual.
bool NiPSysSphericalCollider::IsEqual(NiObject* pkObject) {
    if (!NiPSysCollider::IsEqual(pkObject)) {
        return false;
    }

    NiPSysSphericalCollider* pkOther = static_cast<NiPSysSphericalCollider*>(pkObject);
    if ((m_pkColliderObj && !pkOther->m_pkColliderObj) || (!m_pkColliderObj && pkOther->m_pkColliderObj)) {
        return false;
    }

    if (m_pkColliderObj && pkOther->m_pkColliderObj && !m_pkColliderObj->IsEqual(pkOther->m_pkColliderObj)) {
        return false;
    }

    if (m_fRadius != pkOther->m_fRadius) {
        return false;
    }

    return true;
}

void NiPSysSphericalCollider::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiPSysCollider::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString(
        "Collider Object",
        m_pkColliderObj ? static_cast<const char*>(m_pkColliderObj->m_kName) : "None"
    ));
    pkStrings->Add(NiGetViewerString("Radius", m_fRadius));
}

void NiPSysSphericalCollider::ProcessClone(NiCloningProcess& kCloning) {
    NiPSysCollider::ProcessClone(kCloning);

    NiObject* pkObject = nullptr;
    kCloning.m_pkCloneMap->GetAt(this, pkObject);
    NiPSysSphericalCollider* pkClone = static_cast<NiPSysSphericalCollider*>(pkObject);

    if (m_pkColliderObj) {
        if (kCloning.m_pkCloneMap->GetAt(m_pkColliderObj, pkObject)) {
            pkClone->m_pkColliderObj = static_cast<NiAVObject*>(pkObject);
        }
        else {
            pkClone->m_pkColliderObj = m_pkColliderObj;
        }
    }
}

// A particle further than 0.001 (in squared distance) from the sphere's surface is inside or outside it; one on
// the surface never collides here. The step is intersected with the sphere through the closest point of the
// velocity line: an inside particle hits the far side, an outside one moving towards the centre the near side. A
// hit before the current collision time becomes the new collision. The rest of the list is asked last, and its
// collider wins.
NiPSysCollider*
NiPSysSphericalCollider::Resolve(float fInitialTime, float& fCollisionTime, NiPSysData* pkData, int32_t usCurParticle) {
    NiPoint3& kPosition = pkData->m_pkVertex[static_cast<uint16_t>(usCurParticle)];
    NiParticleInfo* pkInfo = &pkData->m_pkParticleInfo[static_cast<uint16_t>(usCurParticle)];
    bool bCollision = false;

    NiPoint3 kDiff = kPosition - m_kPosition;
    float fSqrDist = kDiff.SqrLength();
    float fDist = fSqrDist - m_fRadiusSq;
    bool bInside = -fDist > 0.001f;
    if (bInside || fDist > 0.001f) {
        NiPoint3& kVelocity = pkInfo->m_kVelocity;
        float fDot = kDiff.Dot(kVelocity);
        float fB = -fDot;
        if (bInside || fB > 0.0f) {
            float fSqrSpeed = kVelocity.SqrLength();
            float fClosestTime = fB / fSqrSpeed;
            NiPoint3 kClosest = kPosition + kVelocity * fClosestTime;
            NiPoint3 kClosestDiff = kClosest - m_kPosition;
            float fClosestSqrDist = kClosestDiff.SqrLength();
            if (bInside || fClosestSqrDist < m_fRadiusSq) {
                float fRemainder = m_fRadiusSq - fClosestSqrDist;
                float fQuotient = fRemainder / fSqrSpeed;
                float fRoot = std::sqrt(fQuotient);
                float fTime;
                if (bInside) {
                    fTime = fRoot + fClosestTime;
                }
                else {
                    fTime = fClosestTime - fRoot;
                }

                if (fCollisionTime - fInitialTime > fTime) {
                    bCollision = true;
                    float fNewTime = fTime + fInitialTime;
                    fCollisionTime = fNewTime;
                    m_fCollisionTime = fNewTime;
                    m_kCollisionPoint = kPosition + kVelocity * fTime;
                }
            }
        }
    }

    NiPSysCollider* pkCollider = NiPSysCollider::Resolve(fInitialTime, fCollisionTime, pkData, usCurParticle);
    if (pkCollider) {
        return pkCollider;
    }

    return bCollision ? this : nullptr;
}

// Reflects the velocity about the surface normal at the collision point and damps it by the bounce.
void NiPSysSphericalCollider::Update(float fTime, NiPSysData* pkData, int32_t usCurParticle) {
    NiPoint3& kVelocity = pkData->m_pkParticleInfo[static_cast<uint16_t>(usCurParticle)].m_kVelocity;
    NiPoint3 kDiff = m_kCollisionPoint - m_kPosition;
    float fInvRadius = 1.0f / m_fScaledRadius;
    NiPoint3 kNormal;
    kNormal.x = kDiff.x * fInvRadius;
    kNormal.y = kDiff.y * fInvRadius;
    kNormal.z = kDiff.z * fInvRadius;
    float fDot = kVelocity.Dot(kNormal);
    kVelocity -= kNormal * fDot * 2.0f;
    kVelocity *= m_fBounce;
    NiPSysCollider::Update(fTime, pkData, usCurParticle);
}

// Moves the sphere into the particle system's space when either transform changed since the last call. The first
// compare reads the collider object without a null check; a missing object later counts as the identity.
void NiPSysSphericalCollider::SetResolveConstants() {
    if (m_kLastColliderTransform != m_pkColliderObj->m_kWorld ||
        m_kLastPSysTransform != m_pkManager->m_pkTarget->m_kWorld) {
        NiTransform kColliderToWorld;
        if (m_pkColliderObj) {
            kColliderToWorld = m_pkColliderObj->m_kWorld;
        }
        else {
            kColliderToWorld.MakeIdentity();
        }

        NiTransform kPSysToWorld = m_pkManager->m_pkTarget->m_kWorld;
        NiTransform kWorldToPSys;
        kPSysToWorld.Invert(kWorldToPSys);
        NiTransform kColliderToPSys = kWorldToPSys * kColliderToWorld;

        float fScaledRadius = m_fRadius * kColliderToPSys.m_fScale;
        m_kPosition = kColliderToPSys.m_Translate;
        m_fScaledRadius = fScaledRadius;
        m_fRadiusSq = fScaledRadius * fScaledRadius;
        m_kLastPSysTransform = kPSysToWorld;
        m_kLastColliderTransform = kColliderToWorld;
    }

    if (m_spNext) {
        m_spNext->SetResolveConstants();
    }
}

NiObject* NiPSysSphericalCollider::CreateObject() {
    return new NiPSysSphericalCollider(1.0, false, false, nullptr, nullptr, 1.0);
}
