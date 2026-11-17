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
    // EXE-INLINED 0x439EF0
    float fDiffX = kPoint.x - arCollider.m_kPosition.x;
    float fDiffY = kPoint.y - arCollider.m_kPosition.y;
    float fDiffZ = kPoint.z - arCollider.m_kPosition.z;
    NiPoint3 kXAxis = arCollider.m_kRotation * arCollider.m_kXAxis;
    NiPoint3 kYAxis = arCollider.m_kRotation * arCollider.m_kYAxis;

    // EXE-INLINED 0x4B6190
    float fXDot = fDiffX * kXAxis.x + fDiffY * kXAxis.y + fDiffZ * kXAxis.z;
    // EXE-INLINED 0x45BB20
    float fXPartX = kXAxis.x * fXDot;
    float fXPartY = kXAxis.y * fXDot;
    float fXPartZ = kXAxis.z * fXDot;
    // EXE-INLINED 0x4B6190
    float fYDot = fDiffX * kYAxis.x + fDiffY * kYAxis.y + fDiffZ * kYAxis.z;
    // EXE-INLINED 0x45BB20
    float fYPartX = kYAxis.x * fYDot;
    float fYPartY = kYAxis.y * fYDot;
    float fYPartZ = kYAxis.z * fYDot;

    // EXE-INLINED 0x4A7290
    float fXSqrLength = fXPartX * fXPartX + fXPartY * fXPartY + fXPartZ * fXPartZ;
    if (!(fXSqrLength < arCollider.m_fHalfSqScaledWidth)) {
        return false;
    }

    // EXE-INLINED 0x4A7290
    float fYSqrLength = fYPartX * fYPartX + fYPartY * fYPartY + fYPartZ * fYPartZ;
    return fYSqrLength < arCollider.m_fHalfSqScaledHeight;
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
    m_fHalfSqScaledHeight(0.5f), m_kPlane(NiPoint3_ZERO, 0.0f), m_kPosition(NiPoint3_ZERO),
    m_kRotation(NiMatrix3_IDENTITY) {
    m_kLastColliderTransform.MakeIdentity();
    m_kLastPSysTransform.MakeIdentity();
}

NiPSysPlanarCollider::~NiPSysPlanarCollider() {}

NiObject* NiPSysPlanarCollider::CreateClone(NiCloningProcess& kCloning) {
    NiPSysPlanarCollider* v4;
    void* v3 = NiMemObject::operator new(sizeof(NiPSysPlanarCollider));
    if (v3) {
        v4 = ::new (v3)
            NiPSysPlanarCollider(1.0, false, false, nullptr, nullptr, 1.0, 1.0, NiPoint3_UNIT_X, NiPoint3_UNIT_Y);
    }
    else {
        v4 = nullptr;
    }
    CopyMembers(v4, kCloning);
    v4->m_fWidth = m_fWidth;
    v4->m_fHeight = m_fHeight;
    v4->m_kXAxis.x = m_kXAxis.x;
    v4->m_kXAxis.y = m_kXAxis.y;
    v4->m_kXAxis.z = m_kXAxis.z;
    v4->m_kYAxis.x = m_kYAxis.x;
    v4->m_kYAxis.y = m_kYAxis.y;
    v4->m_kYAxis.z = m_kYAxis.z;
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

// The collider object is cast without a type check, as the game does.
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
        "Collider Object", m_pkColliderObj ? static_cast<const char*>(m_pkColliderObj->m_kName) : "None"
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

    // EXE-INLINED 0x4B6160
    float fPlaneDot =
        m_kPlane.m_kNormal.x * kPosition.x + m_kPlane.m_kNormal.y * kPosition.y + m_kPlane.m_kNormal.z * kPosition.z;
    float fDist = fPlaneDot - m_kPlane.m_fConstant;
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
        // EXE-INLINED 0x45BB20
        float fStepX = fDeltaTime * pkInfo->m_kVelocity.x;
        float fStepY = fDeltaTime * pkInfo->m_kVelocity.y;
        float fStepZ = fDeltaTime * pkInfo->m_kVelocity.z;
        // EXE-INLINED 0x4B6190
        float fStepDist = m_kPlane.m_kNormal.x * fStepX + m_kPlane.m_kNormal.y * fStepY + m_kPlane.m_kNormal.z * fStepZ;
        float fNewDist = fStepDist + fDist;
        float fAbsNewDist = std::fabs(fNewDist);
        if (fAbsNewDist < 0.001f) {
            fNewDist = 0.0f;
        }

        if (fNewDist * fDist < 0.0) {
            float fTime = fDeltaTime * -fDist / fStepDist;
            // Each product is stored as a float before the sum.
            // EXE-INLINED 0x45BB20
            float fMoveX = fTime * pkInfo->m_kVelocity.x;
            float fMoveY = fTime * pkInfo->m_kVelocity.y;
            float fMoveZ = fTime * pkInfo->m_kVelocity.z;
            // EXE-INLINED 0x439E90
            float fPointX = kPosition.x + fMoveX;
            float fPointY = kPosition.y + fMoveY;
            float fPointZ = kPosition.z + fMoveZ;

            // The static is initialized here, once the step is known to cross the plane.
            if (!(sfCollisionEpsilon_guard & 1)) {
                sfCollisionEpsilon_guard |= 1;
                sfCollisionEpsilon = static_cast<float>(std::pow(2.0, -21.0));
            }

            // EXE-INLINED 0x4B6160
            float fPointDot =
                m_kPlane.m_kNormal.x * fPointX + m_kPlane.m_kNormal.y * fPointY + m_kPlane.m_kNormal.z * fPointZ;
            float fPointDist = fPointDot - m_kPlane.m_fConstant;
            if (fPointDist * fDist < 0.0) {
                float fAbsX = std::fabs(fPointX);
                float fAbsY = std::fabs(fPointY);
                float fMax = fAbsX < fAbsY ? fAbsY : fAbsX;
                float fAbsZ = std::fabs(fPointZ);
                if (fMax < fAbsZ) {
                    fMax = fAbsZ;
                }

                float fOffset = fMax * sfCollisionEpsilon;
                // The normal scaled by the offset is stored as floats before the sum, as above.
                // EXE-INLINED 0x45BB20
                float fNudgeX = m_kPlane.m_kNormal.x * fOffset;
                float fNudgeY = m_kPlane.m_kNormal.y * fOffset;
                float fNudgeZ = m_kPlane.m_kNormal.z * fOffset;
                if (fDist < 0.0f) {
                    fPointX = fPointX - fNudgeX;
                    fPointY = fPointY - fNudgeY;
                    fPointZ = fPointZ - fNudgeZ;
                }
                else {
                    fPointX = fPointX + fNudgeX;
                    fPointY = fPointY + fNudgeY;
                    fPointZ = fPointZ + fNudgeZ;
                }
            }

            NiPoint3 kPoint;
            kPoint.x = fPointX;
            kPoint.y = fPointY;
            kPoint.z = fPointZ;
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
    // EXE-INLINED 0x4B6190
    float fDot =
        m_kPlane.m_kNormal.x * kVelocity.x + m_kPlane.m_kNormal.y * kVelocity.y + m_kPlane.m_kNormal.z * kVelocity.z;
    // EXE-INLINED 0x45BB20
    float fNormalX = m_kPlane.m_kNormal.x * fDot;
    float fNormalY = m_kPlane.m_kNormal.y * fDot;
    float fNormalZ = m_kPlane.m_kNormal.z * fDot;
    if (fTime == m_fCollisionTime && fDot < 0.001f) {
        // EXE-INLINED 0x439EF0
        kVelocity.x = kVelocity.x - fNormalX;
        kVelocity.y = kVelocity.y - fNormalY;
        kVelocity.z = kVelocity.z - fNormalZ;
    }
    else {
        float fTwiceX = fNormalX * 2.0f;
        float fTwiceY = fNormalY * 2.0f;
        float fTwiceZ = fNormalZ * 2.0f;
        // EXE-INLINED 0x439EF0
        float fNewX = kVelocity.x - fTwiceX;
        kVelocity.x = fNewX;
        float fNewY = kVelocity.y - fTwiceY;
        kVelocity.y = fNewY;
        float fNewZ = kVelocity.z - fTwiceZ;
        kVelocity.z = fNewZ;
        // EXE-INLINED 0x439180
        float fBounce = m_fBounce;
        kVelocity.x = fNewX * fBounce;
        kVelocity.y = fNewY * fBounce;
        kVelocity.z = fNewZ * fBounce;
    }

    NiPSysCollider::Update(fTime, pkData, usCurParticle);
}

// Rebuilds the plane and the squared half extents when the plane is still unset or either transform changed. The normal
// is the unit cross product of the axes; with a collider object, the position, rotation, normal and extents are moved
// into the particle system's space. Without one the plane stays in particle system space.
void NiPSysPlanarCollider::SetResolveConstants() {
    NiPlane kUnset(NiPoint3_ZERO, 0.0f);
    if ((m_kPlane.m_kNormal.x == kUnset.m_kNormal.x && m_kPlane.m_kNormal.y == kUnset.m_kNormal.y &&
         m_kPlane.m_kNormal.z == kUnset.m_kNormal.z && m_kPlane.m_fConstant == kUnset.m_fConstant) ||
        (m_pkColliderObj &&
         (m_kLastColliderTransform != m_pkColliderObj->m_kWorld ||
          m_kLastPSysTransform != m_pkManager->m_pkTarget->m_kWorld))) {
        m_kPosition = NiPoint3_ZERO;
        m_kRotation = NiMatrix3_IDENTITY;
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
    void* v0 = NiMemObject::operator new(sizeof(NiPSysPlanarCollider));
    if (v0) {
        return ::new (v0)
            NiPSysPlanarCollider(1.0, false, false, nullptr, nullptr, 1.0, 1.0, NiPoint3_UNIT_X, NiPoint3_UNIT_Y);
    }

    return nullptr;
}
