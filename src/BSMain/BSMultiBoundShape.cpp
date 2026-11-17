#include "BSMain/BSMultiBoundShape.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"

NiRTTI BSMultiBoundShape::ms_RTTI("BSMultiBoundShape", &NiObject::ms_RTTI);

// QPointWithin (0x8D0370, a shared return-false stub) and SetCenter (0x4534F0, a shared empty stub) stay with the
// exe; the constructor (0xC4F980) is not reconstructed because the class is abstract.
BSMultiBoundShape::BSMultiBoundShape() : NiObject() {}

// Inlined into every shape's destructor; the exe has no copy of its own.
BSMultiBoundShape::~BSMultiBoundShape() {}

// The abstract base clones to itself.
NiObject* BSMultiBoundShape::CreateClone(NiCloningProcess&) {
	return this;
}

void BSMultiBoundShape::LoadBinary(NiStream& arStream) {
	NiObject::LoadBinary(arStream);
}

void BSMultiBoundShape::LinkObject(NiStream& arStream) {
	NiObject::LinkObject(arStream);
}

bool BSMultiBoundShape::RegisterStreamables(NiStream& arStream) {
	return NiObject::RegisterStreamables(arStream);
}

void BSMultiBoundShape::SaveBinary(NiStream& arStream) {
	NiObject::SaveBinary(arStream);
}

bool BSMultiBoundShape::IsEqual(NiObject* apObject) {
	return NiObject::IsEqual(apObject);
}

// The shape adds no strings, not even its title.
void BSMultiBoundShape::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
	NiObject::GetViewerStrings(apStrings);
}

void BSMultiBoundShape::CopyMembers(BSMultiBoundShape* pDest, NiCloningProcess& arCloneData) {
	NiObject::CopyMembers(pDest, arCloneData);
	pDest->kCullResult = kCullResult;
}
