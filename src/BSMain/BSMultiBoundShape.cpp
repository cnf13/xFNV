#include "BSMain/BSMultiBoundShape.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"

NiRTTI BSMultiBoundShape::ms_RTTI("BSMultiBoundShape", &NiObject::ms_RTTI);

BSMultiBoundShape::BSMultiBoundShape() : NiObject() {}

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
