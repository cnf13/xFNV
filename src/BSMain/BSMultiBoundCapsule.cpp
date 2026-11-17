#include "BSMain/BSMultiBoundCapsule.h"

#include "BSMain/BSMultiBound.h"
#include "BSMain/BSMultiBoundShape.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI BSMultiBoundCapsule::ms_RTTI("BSMultiBoundCapsule", &BSMultiBoundShape::ms_RTTI);

BSMultiBoundCapsule::BSMultiBoundCapsule() : BSMultiBoundShape() {}

// QDebugGeomLineSize (0x9B3120) and QDebugGeomShapeSize (0x9CBB10) are constant stubs shared by many unrelated
// classes; they stay with the exe.
BSMultiBoundCapsule::~BSMultiBoundCapsule() {}

// The exe's cleanup-only C++ EH frame is left out.
NiObject* BSMultiBoundCapsule::CreateClone(NiCloningProcess& arCloneData) {
	BSMultiBoundCapsule* v3;
	BSMultiBoundCapsule* apMemory =
		static_cast<BSMultiBoundCapsule*>(NiMemObject::operator new(sizeof(BSMultiBoundCapsule)));
	if (apMemory) {
		v3 = ::new (apMemory) BSMultiBoundCapsule;
	} else {
		v3 = 0;
	}
	CopyMembers(v3, arCloneData);
	return v3;
}

void BSMultiBoundCapsule::LoadBinary(NiStream& arStream) {
	BSMultiBoundShape::LoadBinary(arStream);
	PointA.LoadBinary(arStream);
	PointB.LoadBinary(arStream);
	// The exe reads Radius through its one out-of-line 4-byte transfer, the uint32_t instantiation (0xA26DF0), which
	// copies 4 raw bytes to the address it is given: the exe treats Radius as untyped memory there.
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
	// The exe writes Radius through its 4-byte transfer copy 0xA3DA50, registered as NiAnimationKey::SaveBinary
	// (which writes the key's only member, the float m_fTime).
	NiAnimationKey::SaveBinary(arStream, reinterpret_cast<NiAnimationKey*>(&Radius));
}

// The exe inlines NiPoint3::operator== and combines the three results with a bitwise and, without short-circuit.
bool BSMultiBoundCapsule::IsEqual(NiObject* apObject) {
	if (!BSMultiBoundShape::IsEqual(apObject))
		return false;

	BSMultiBoundCapsule* pkOther = static_cast<BSMultiBoundCapsule*>(apObject);
	// EXE-INLINED 0x4390C0
	bool bPointA = PointA.x == pkOther->PointA.x && PointA.y == pkOther->PointA.y && PointA.z == pkOther->PointA.z;
	// EXE-INLINED 0x4390C0
	bool bPointB = PointB.x == pkOther->PointB.x && PointB.y == pkOther->PointB.y && PointB.z == pkOther->PointB.z;
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

// QMultiBoundRadius: same function as TESGlobal::GetValue (0x526AC0), left with the exe as a shared stub.
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
	void* v2 = NiMemObject::operator new(sizeof(BSMultiBoundCapsule));
	if (v2) {
		return ::new (v2) BSMultiBoundCapsule;
	} else {
		return nullptr;
	}
}
