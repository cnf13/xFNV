#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysPlanarCollider.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticleInfo.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysCollider.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysColliderManager.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysSpawnModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

#include <cmath>

// sfCollisionEpsilon is the nudge factor; sfCollisionEpsilon_guard is its guard.
static float sfCollisionEpsilon;
static uint32_t sfCollisionEpsilon_guard;

NiRTTI NiPSysPlanarCollider::ms_RTTI("NiPSysPlanarCollider", &NiPSysCollider::ms_RTTI);

// Whether kPoint lies inside the rectangle: its offset from the centre is projected on both rotated axes, and each
// projection must be shorter than the half extent.
static bool IsInsideRectangle(const NiPSysPlanarCollider& arCollider, const NiPoint3& kPoint) {
    NiPoint3 kDiff = kPoint - arCollider.m_kPosition;
    NiPoint3 kXAxis = arCollider.m_kRotation * arCollider.m_kXAxis;
    NiPoint3 kYAxis = arCollider.m_kRotation * arCollider.m_kYAxis;

    NiPoint3 kXPart = kXAxis * kDiff.Dot(kXAxis);
    NiPoint3 kYPart = kYAxis * kDiff.Dot(kYAxis);

    if (!(kXPart.SqrLength() < arCollider.m_fHalfSqScaledWidth)) {
        return false;
    }

    return kYPart.SqrLength() < arCollider.m_fHalfSqScaledHeight;
}

NiPSysPlanarCollider::NiPSysPlanarCollider(
    float fBounce,
    bool bSpawnOnCollide,
    bool bDieOnCollide,
    NiPSysSpawnModifier* pkSpawnModifier,
    NiAVObject* pkColliderObj,
    float fWidth,
    float fHeight,
    NiPoint3 kXAxis,
    NiPoint3 kYAxis
)
  : NiPSysCollider(fBounce, bSpawnOnCollide, bDieOnCollide, pkSpawnModifier), m_pkColliderObj(pkColliderObj),
    m_fWidth(fWidth), m_fHeight(fHeight), m_kXAxis(kXAxis), m_kYAxis(kYAxis), m_fHalfSqScaledWidth(0.5f),
    m_fHalfSqScaledHeight(0.5f), m_kPlane(NiPoint3::ZERO, 0.0f), m_kPosition(NiPoint3::ZERO),
    m_kRotation(NiMatrix3::IDENTITY) {
    m_kLastColliderTransform.MakeIdentity();
    m_kLastPSysTransform.MakeIdentity();
}

NiPSysPlanarCollider::~NiPSysPlanarCollider() {}

NiObject* NiPSysPlanarCollider::CreateClone(NiCloningProcess& kCloning) {
    NiPSysPlanarCollider* v4;
    v4 = new NiPSysPlanarCollider(1.0, false, false, nullptr, nullptr, 1.0, 1.0, NiPoint3::UNIT_X, NiPoint3::UNIT_Y);
    CopyMembers(v4, kCloning);
    v4->m_fWidth = m_fWidth;
    v4->m_fHeight = m_fHeight;
    v4->m_kXAxis = m_kXAxis;
    v4->m_kYAxis = m_kYAxis;
    return v4;
}

void NiPSysPlanarCollider::LoadBinary(NiStream& kStream) {
    NiPSysCollider::LoadBinary(kStream);
    kStream.ReadLinkID();
    NiStreamLoadBinary(kStream, m_fWidth);
    NiStreamLoadBinary(kStream, m_fHeight);
    m_kXAxis.LoadBinary(kStream);
    m_kYAxis.LoadBinary(kStream);
}

// The collider object is cast without a type check.
void NiPSysPlanarCollider::LinkObject(NiStream& kStream) {
    NiPSysCollider::LinkObject(kStream);
    m_pkColliderObj = static_cast<NiAVObject*>(kStream.GetObjectFromLinkID());
}

// The collider object is not registered: the scene graph owns it.
bool NiPSysPlanarCollider::RegisterStreamables(NiStream& kStream) {
    return NiPSysCollider::RegisterStreamables(kStream);
}

void NiPSysPlanarCollider::SaveBinary(NiStream& kStream) {
    NiPSysCollider::SaveBinary(kStream);
    kStream.SaveLinkID(m_pkColliderObj);
    NiStreamSaveBinary(kStream, m_fWidth);
    NiStreamSaveBinary(kStream, m_fHeight);
    m_kXAxis.SaveBinary(kStream);
    m_kYAxis.SaveBinary(kStream);
}

// The collider objects are compared with IsEqual.
bool NiPSysPlanarCollider::IsEqual(NiObject* pkObject) {
    if (!NiPSysCollider::IsEqual(pkObject)) {
        return false;
    }

    NiPSysPlanarCollider* pkOther = static_cast<NiPSysPlanarCollider*>(pkObject);
    if ((m_pkColliderObj && !pkOther->m_pkColliderObj) || (!m_pkColliderObj && pkOther->m_pkColliderObj)) {
        return false;
    }

    if (m_pkColliderObj && pkOther->m_pkColliderObj && !m_pkColliderObj->IsEqual(pkOther->m_pkColliderObj)) {
        return false;
    }

    if (m_fWidth != pkOther->m_fWidth) {
        return false;
    }

    if (m_fHeight != pkOther->m_fHeight) {
        return false;
    }

    if (m_kXAxis != pkOther->m_kXAxis) {
        return false;
    }

    if (m_kYAxis != pkOther->m_kYAxis) {
        return false;
    }

    return true;
}

void NiPSysPlanarCollider::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiPSysCollider::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString(
        "Collider Object",
        m_pkColliderObj ? static_cast<const char*>(m_pkColliderObj->m_kName) : "None"
    ));
    pkStrings->Add(NiGetViewerString("Width", m_fWidth));
    pkStrings->Add(NiGetViewerString("Height", m_fHeight));
    pkStrings->Add(m_kXAxis.GetViewerString("X-Axis"));
    pkStrings->Add(m_kYAxis.GetViewerString("Y-Axis"));
}

// The clone points at the clone of the collider object, or at the original when it was not cloned. One out variable
// serves both map lookups.
void NiPSysPlanarCollider::ProcessClone(NiCloningProcess& kCloning) {
    NiPSysCollider::ProcessClone(kCloning);

    NiObject* pkObject = nullptr;
    kCloning.m_pkCloneMap->GetAt(this, pkObject);
    NiPSysPlanarCollider* pkClone = static_cast<NiPSysPlanarCollider*>(pkObject);

    if (m_pkColliderObj) {
        if (kCloning.m_pkCloneMap->GetAt(m_pkColliderObj, pkObject)) {
            pkClone->m_pkColliderObj = static_cast<NiAVObject*>(pkObject);
        }
        else {
            pkClone->m_pkColliderObj = m_pkColliderObj;
        }
    }
}

// A particle on the plane (within 0.001) that lies inside the rectangle collides at once, and then the rest of the
// list is not asked. Otherwise the particle collides when its step crosses the plane: the crossing point is nudged
// back to the particle's side by a fraction of its largest coordinate, then tested against the rectangle.
NiPSysCollider*
NiPSysPlanarCollider::Resolve(float fInitialTime, float& fCollisionTime, NiPSysData* pkData, int32_t usCurParticle) {
    NiPoint3& kPosition = pkData->m_pkVertex[static_cast<uint16_t>(usCurParticle)];
    NiParticleInfo* pkInfo = &pkData->m_pkParticleInfo[static_cast<uint16_t>(usCurParticle)];
    bool bCollision = false;

    float fDist = m_kPlane.Distance(kPosition);
    if (fDist < 0.001f && -fDist < 0.001f) {
        if (IsInsideRectangle(*this, kPosition)) {
            fCollisionTime = fInitialTime;
            m_fCollisionTime = fInitialTime;
            m_kCollisionPoint = kPosition;
            return this;
        }
    }
    else {
        float fDeltaTime = fCollisionTime - fInitialTime;
        NiPoint3 kStep = pkInfo->m_kVelocity * fDeltaTime;
        float fStepDist = m_kPlane.m_kNormal.Dot(kStep);
        float fNewDist = fStepDist + fDist;
        float fAbsNewDist = std::fabs(fNewDist);
        if (fAbsNewDist < 0.001f) {
            fNewDist = 0.0f;
        }

        if (fNewDist * fDist < 0.0) {
            float fTime = fDeltaTime * -fDist / fStepDist;
            NiPoint3 kPoint = kPosition + pkInfo->m_kVelocity * fTime;

            // The static is initialized here, once the step is known to cross the plane.
            if (!(sfCollisionEpsilon_guard & 1)) {
                sfCollisionEpsilon_guard |= 1;
                sfCollisionEpsilon = static_cast<float>(std::pow(2.0, -21.0));
            }

            float fPointDist = m_kPlane.Distance(kPoint);
            if (fPointDist * fDist < 0.0) {
                float fAbsX = std::fabs(kPoint.x);
                float fAbsY = std::fabs(kPoint.y);
                float fMax = fAbsX < fAbsY ? fAbsY : fAbsX;
                float fAbsZ = std::fabs(kPoint.z);
                if (fMax < fAbsZ) {
                    fMax = fAbsZ;
                }

                float fOffset = fMax * sfCollisionEpsilon;
                NiPoint3 kNudge = m_kPlane.m_kNormal * fOffset;
                if (fDist < 0.0f) {
                    kPoint.x = kPoint.x - kNudge.x;
                    kPoint.y = kPoint.y - kNudge.y;
                    kPoint.z = kPoint.z - kNudge.z;
                }
                else {
                    kPoint.x = kPoint.x + kNudge.x;
                    kPoint.y = kPoint.y + kNudge.y;
                    kPoint.z = kPoint.z + kNudge.z;
                }
            }

            if (IsInsideRectangle(*this, kPoint)) {
                bCollision = true;
                float fNewTime = fTime + fInitialTime;
                fCollisionTime = fNewTime;
                m_fCollisionTime = fNewTime;
                m_kCollisionPoint = kPoint;
            }
        }
    }

    NiPSysCollider* pkCollider = NiPSysCollider::Resolve(fInitialTime, fCollisionTime, pkData, usCurParticle);
    if (pkCollider) {
        return pkCollider;
    }

    return bCollision ? this : nullptr;
}

// Reflects the velocity about the plane and damps it by the bounce. A particle that collided at this very time while
// barely moving into the plane (normal speed below 0.001) only loses its normal part, so it slides. Each product and
// difference is stored as a float.
void NiPSysPlanarCollider::Update(float fTime, NiPSysData* pkData, int32_t usCurParticle) {
    NiPoint3& kVelocity = pkData->m_pkParticleInfo[static_cast<uint16_t>(usCurParticle)].m_kVelocity;
    float fDot = m_kPlane.m_kNormal.Dot(kVelocity);
    NiPoint3 kNormal = m_kPlane.m_kNormal * fDot;
    if (fTime == m_fCollisionTime && fDot < 0.001f) {
        kVelocity = kVelocity - kNormal;
    }
    else {
        NiPoint3 kTwice(kNormal.x * 2.0f, kNormal.y * 2.0f, kNormal.z * 2.0f);
        kVelocity = kVelocity - kTwice;
        kVelocity *= m_fBounce;
    }

    NiPSysCollider::Update(fTime, pkData, usCurParticle);
}

// Rebuilds the plane and the squared half extents when the plane is still unset or either transform changed. The normal
// is the unit cross product of the axes; with a collider object, the position, rotation, normal and extents are moved
// into the particle system's space. Without one the plane stays in particle system space.
void NiPSysPlanarCollider::SetResolveConstants() {
    NiPlane kUnset(NiPoint3::ZERO, 0.0f);
    if ((m_kPlane.m_kNormal.x == kUnset.m_kNormal.x &&
         m_kPlane.m_kNormal.y == kUnset.m_kNormal.y &&
         m_kPlane.m_kNormal.z == kUnset.m_kNormal.z &&
         m_kPlane.m_fConstant == kUnset.m_fConstant) ||
        (m_pkColliderObj &&
         (m_kLastColliderTransform != m_pkColliderObj->m_kWorld ||
          m_kLastPSysTransform != m_pkManager->m_pkTarget->m_kWorld))) {
        m_kPosition = NiPoint3::ZERO;
        m_kRotation = NiMatrix3::IDENTITY;
        NiPoint3 kNormal;
        kNormal.x = m_kXAxis.y * m_kYAxis.z - m_kXAxis.z * m_kYAxis.y;
        kNormal.y = m_kXAxis.z * m_kYAxis.x - m_kXAxis.x * m_kYAxis.z;
        kNormal.z = m_kXAxis.x * m_kYAxis.y - m_kXAxis.y * m_kYAxis.x;
        kNormal.Unitize();
        float fWidth = m_fWidth;
        float fHeight = m_fHeight;

        if (m_pkColliderObj) {
            NiTransform kColliderToWorld = m_pkColliderObj->m_kWorld;
            NiTransform kPSysToWorld = m_pkManager->m_pkTarget->m_kWorld;
            NiTransform kWorldToPSys;
            kPSysToWorld.Invert(kWorldToPSys);
            NiTransform kColliderToPSys = kWorldToPSys * kColliderToWorld;

            m_kPosition = kColliderToPSys.m_Translate;
            m_kRotation = kColliderToPSys.m_Rotate;
            float fNormalX = kNormal.x;
            float fNormalY = kNormal.y;
            float fNormalZ = kNormal.z;
            kNormal.x = m_kRotation.m_pEntry[0][0] * fNormalX + m_kRotation.m_pEntry[0][1] * fNormalY +
                m_kRotation.m_pEntry[0][2] * fNormalZ;
            kNormal.y = m_kRotation.m_pEntry[1][0] * fNormalX + m_kRotation.m_pEntry[1][1] * fNormalY +
                m_kRotation.m_pEntry[1][2] * fNormalZ;
            kNormal.z = m_kRotation.m_pEntry[2][0] * fNormalX + m_kRotation.m_pEntry[2][1] * fNormalY +
                m_kRotation.m_pEntry[2][2] * fNormalZ;
            kNormal.Unitize();
            fWidth = kColliderToPSys.m_fScale * fWidth;
            fHeight = kColliderToPSys.m_fScale * fHeight;
            m_kLastColliderTransform = kColliderToWorld;
            m_kLastPSysTransform = kPSysToWorld;
        }

        fWidth = fWidth * 0.5;
        fHeight = fHeight * 0.5;
        m_fHalfSqScaledWidth = fWidth * fWidth;
        m_fHalfSqScaledHeight = fHeight * fHeight;
        m_kPlane = NiPlane(kNormal, m_kPosition);
    }

    if (m_spNext) {
        m_spNext->SetResolveConstants();
    }
}

NiObject* NiPSysPlanarCollider::CreateObject() {
    return new NiPSysPlanarCollider(1.0, false, false, nullptr, nullptr, 1.0, 1.0, NiPoint3::UNIT_X, NiPoint3::UNIT_Y);
}
