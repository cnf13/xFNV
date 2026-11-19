#include "BSMain/BSMultiBoundSphere.h"

#include "BSMain/BSMultiBound.h"
#include "BSMain/BSMultiBoundAABB.h"
#include "BSMain/BSMultiBoundOBB.h"
#include "BSMain/BSMultiBoundShape.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustumPlanes.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI BSMultiBoundSphere::ms_RTTI("BSMultiBoundSphere", &BSMultiBoundShape::ms_RTTI);

BSMultiBoundSphere::BSMultiBoundSphere() : BSMultiBoundShape() {}

BSMultiBoundSphere::BSMultiBoundSphere(NiPoint3* apCenter, float afRadius)
  : BSMultiBoundShape(), Center(*apCenter), Radius(afRadius) {}

NiObject* BSMultiBoundSphere::CreateClone(NiCloningProcess& arCloneData) {
    BSMultiBoundSphere* v3;
    v3 = new BSMultiBoundSphere;
    CopyMembers(v3, arCloneData);
    return v3;
}

// Radius is read through the uint32_t transfer, which treats the field as untyped 4 bytes.
void BSMultiBoundSphere::LoadBinary(NiStream& arStream) {
    BSMultiBoundShape::LoadBinary(arStream);
    Center.LoadBinary(arStream);
    NiStreamLoadBinary<uint32_t>(arStream, reinterpret_cast<uint32_t&>(Radius));
}

void BSMultiBoundSphere::LinkObject(NiStream& arStream) {
    BSMultiBoundShape::LinkObject(arStream);
}

bool BSMultiBoundSphere::RegisterStreamables(NiStream& arStream) {
    return BSMultiBoundShape::RegisterStreamables(arStream);
}

// Radius is written through NiAnimationKey::SaveBinary, which treats the field as untyped 4 bytes.
void BSMultiBoundSphere::SaveBinary(NiStream& arStream) {
    BSMultiBoundShape::SaveBinary(arStream);
    Center.SaveBinary(arStream);
    NiAnimationKey::SaveBinary(arStream, reinterpret_cast<NiAnimationKey*>(&Radius));
}

bool BSMultiBoundSphere::IsEqual(NiObject* apObject) {
    if (!BSMultiBoundShape::IsEqual(apObject)) {
        return false;
    }

    BSMultiBoundSphere* pkOther = static_cast<BSMultiBoundSphere*>(apObject);
    bool bCenter = pkOther->Center == Center;
    return bCenter & (pkOther->Radius == Radius);
}

void BSMultiBoundSphere::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    BSMultiBoundShape::GetViewerStrings(apStrings);
    apStrings->Add(NiGetViewerString(BSMultiBoundSphere::ms_RTTI.GetName()));
    apStrings->Add(Center.GetViewerString("Center"));
    apStrings->Add(NiGetViewerString("Radius", Radius));
}

BSMultiBoundShape::BSMBIntersectResult BSMultiBoundSphere::CheckBound(const NiBound& aTargetBound) {
    NiPoint3 kDelta = Center - aTargetBound.m_kCenter;
    float fDist = kDelta.Length();
    if (fDist - (Radius - aTargetBound.m_fRadius) > 0.0) {
        return static_cast<BSMBIntersectResult>(fDist - (Radius + aTargetBound.m_fRadius) <= 0.0);
    }
    return BS_INTERSECT_CONTAINSTARGET;
}

// Containment only: every corner of a box, or the whole of a sphere, must lie inside; capsules are never tested.
BSMultiBoundShape::BSMBIntersectResult BSMultiBoundSphere::CheckBound(const BSMultiBound& aTargetBound) {
    BSMBShapeType eType = aTargetBound.spShape->QType();
    if (eType == BS_SHAPE_AABB) {
        NiPoint3 akVertices[8];
        static_cast<BSMultiBoundAABB*>(aTargetBound.spShape.m_pObject)->GetVertices(akVertices);
        float fSqrRadius = Radius * Radius;
        for (int32_t i = 0; i < 8; ++i) {
            NiPoint3 kDelta = Center - akVertices[i];
            float fSqrDist = kDelta.SqrLength();
            if (fSqrRadius < fSqrDist) {
                return BS_INTERSECT_NONE;
            }
        }
        return BS_INTERSECT_CONTAINSTARGET;
    }
    if (eType == BS_SHAPE_OBB) {
        NiPoint3 akVertices[8];
        static_cast<BSMultiBoundOBB*>(aTargetBound.spShape.m_pObject)->GetVertices(akVertices);
        float fSqrRadius = Radius * Radius;
        for (int32_t i = 0; i < 8; ++i) {
            NiPoint3 kDelta = Center - akVertices[i];
            float fSqrDist = kDelta.SqrLength();
            if (fSqrRadius < fSqrDist) {
                return BS_INTERSECT_NONE;
            }
        }
        return BS_INTERSECT_CONTAINSTARGET;
    }
    if (eType == BS_SHAPE_SPHERE) {
        BSMultiBoundSphere* pkSphere = static_cast<BSMultiBoundSphere*>(aTargetBound.spShape.m_pObject);
        NiPoint3 kDelta = Center - pkSphere->Center;
        float fDist = kDelta.Length();
        if (fDist + pkSphere->Radius - Radius <= 0.0) {
            return BS_INTERSECT_CONTAINSTARGET;
        }
    }
    return BS_INTERSECT_NONE;
}

bool BSMultiBoundSphere::WithinFrustum(const NiFrustumPlanes& aPlanes) {
    NiBound kBound;
    kBound.m_kCenter = Center;
    kBound.m_fRadius = Radius;
    for (int32_t i = 0; i < 6; ++i) {
        if ((aPlanes.m_uiActivePlanes & (1 << i)) != 0 &&
            kBound.WhichSide(aPlanes.m_akCullingPlanes[i]) == NiPlane::NEGATIVE_SIDE) {
            return false;
        }
    }
    return true;
}

bool BSMultiBoundSphere::CompletelyWithinFrustum(const NiFrustumPlanes& aPlanes) {
    NiBound kBound;
    kBound.m_kCenter = Center;
    kBound.m_fRadius = Radius;
    for (int32_t i = 0; i < 6; ++i) {
        if ((aPlanes.m_uiActivePlanes & (1 << i)) != 0 &&
            kBound.WhichSide(aPlanes.m_akCullingPlanes[i]) != NiPlane::POSITIVE_SIDE) {
            return false;
        }
    }
    return true;
}

void BSMultiBoundSphere::GetNiBound(NiBound& aBound) {
    aBound.m_kCenter = Center;
    aBound.m_fRadius = Radius;
}

bool BSMultiBoundSphere::QPointWithin(NiPoint3& aPoint) {
    NiPoint3 kDelta = aPoint - Center;
    float fDist = kDelta.Length();
    return Radius > fDist;
}

void BSMultiBoundSphere::SetRadius(float afRadius) {
    Radius = afRadius;
}

void BSMultiBoundSphere::CopyMembers(BSMultiBoundSphere* pDest, NiCloningProcess& arCloneData) {
    BSMultiBoundShape::CopyMembers(pDest, arCloneData);
    pDest->SetCenter(Center);
    pDest->Radius = Radius;
}

NiObject* BSMultiBoundSphere::CreateObject() {
    return new BSMultiBoundSphere;
}
