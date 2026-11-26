#include "BSMain/BSMultiBoundCapsule.h"

#include "BSMain/BSMultiBound.h"
#include "BSMain/BSMultiBoundShape.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI BSMultiBoundCapsule::ms_RTTI("BSMultiBoundCapsule", &BSMultiBoundShape::ms_RTTI);

BSMultiBoundCapsule::BSMultiBoundCapsule() : BSMultiBoundShape() {}

BSMultiBoundCapsule::~BSMultiBoundCapsule() {}

NiObject* BSMultiBoundCapsule::CreateClone(NiCloningProcess& arCloneData) {
    BSMultiBoundCapsule* v3;
    v3 = new BSMultiBoundCapsule;
    CopyMembers(v3, arCloneData);
    return v3;
}

void BSMultiBoundCapsule::LoadBinary(NiStream& arStream) {
    BSMultiBoundShape::LoadBinary(arStream);
    PointA.LoadBinary(arStream);
    PointB.LoadBinary(arStream);
    // Radius is read through the uint32_t load, which copies 4 raw bytes to the address it is given, treating Radius as
    // untyped memory.
    NiStreamLoadBinary<uint32_t>(arStream, reinterpret_cast<uint32_t&>(Radius));
}

void BSMultiBoundCapsule::LinkObject(NiStream& arStream) {
    BSMultiBoundShape::LinkObject(arStream);
}

bool BSMultiBoundCapsule::RegisterStreamables(NiStream& arStream) {
    return BSMultiBoundShape::RegisterStreamables(arStream);
}

void BSMultiBoundCapsule::SaveBinary(NiStream& arStream) {
    BSMultiBoundShape::SaveBinary(arStream);
    PointA.SaveBinary(arStream);
    PointB.SaveBinary(arStream);
    // Radius is written through NiAnimationKey::SaveBinary, which writes the key's only member, the float m_fTime, as a
    // 4-byte copy.
    NiAnimationKey::SaveBinary(arStream, reinterpret_cast<NiAnimationKey*>(&Radius));
}

bool BSMultiBoundCapsule::IsEqual(NiObject* apObject) {
    if (!BSMultiBoundShape::IsEqual(apObject)) {
        return false;
    }

    BSMultiBoundCapsule* pkOther = static_cast<BSMultiBoundCapsule*>(apObject);
    bool bPointA = PointA == pkOther->PointA;
    bool bPointB = PointB == pkOther->PointB;
    bool bRadius = Radius == pkOther->Radius;
    return bPointA & bPointB & bRadius;
}

void BSMultiBoundCapsule::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    BSMultiBoundShape::GetViewerStrings(apStrings);
    apStrings->Add(NiGetViewerString(BSMultiBoundCapsule::ms_RTTI.GetName()));
    apStrings->Add(PointA.GetViewerString("PointA"));
    apStrings->Add(PointB.GetViewerString("PointB"));
    apStrings->Add(NiGetViewerString("Radius", Radius));
}

BSMultiBoundShape::BSMBIntersectResult BSMultiBoundCapsule::CheckBound(const NiBound&) {
    return BS_INTERSECT_NONE;
}

// The target's type is queried and ignored: capsule tests are unimplemented in this build.
BSMultiBoundShape::BSMBIntersectResult BSMultiBoundCapsule::CheckBound(const BSMultiBound& aTargetBound) {
    aTargetBound.spShape->QType();
    return BS_INTERSECT_NONE;
}

void BSMultiBoundCapsule::CopyMembers(BSMultiBoundCapsule* pDest, NiCloningProcess& arCloneData) {
    BSMultiBoundShape::CopyMembers(pDest, arCloneData);
    pDest->PointA = PointA;
    pDest->PointB = PointB;
    pDest->Radius = Radius;
}

NiObject* BSMultiBoundCapsule::CreateObject() {
    return new BSMultiBoundCapsule;
}
