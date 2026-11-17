#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandle.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeGroupClass.h"

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpTypedBroadPhaseHandleTypeInfo = {"hkpTypedBroadPhaseHandle", finishLoadedObjecthkpTypedBroadPhaseHandle,
	cleanupLoadedObjecthkpTypedBroadPhaseHandle, nullptr, &typeid(hkpTypedBroadPhaseHandle),
	sizeof(hkpTypedBroadPhaseHandle)};

hkpTypedBroadPhaseHandle::MembersArray hkpTypedBroadPhaseHandle::Members{
	{"type", nullptr, nullptr, {hkClassMember::TYPE_INT8}, {hkClassMember::TYPE_VOID}, 0, {0},
		offsetof(hkpTypedBroadPhaseHandle, m_type), nullptr},
	{"ownerOffset", nullptr, nullptr, {hkClassMember::TYPE_INT8}, {hkClassMember::TYPE_VOID}, 0,
		{hkClassMember::SERIALIZE_IGNORED}, offsetof(hkpTypedBroadPhaseHandle, m_ownerOffset), nullptr},
	{"objectQualityType", nullptr, nullptr, {hkClassMember::TYPE_INT8}, {hkClassMember::TYPE_VOID}, 0, {0},
		offsetof(hkpTypedBroadPhaseHandle, m_objectQualityType), nullptr},
	{"collisionFilterInfo", nullptr, nullptr, {hkClassMember::TYPE_UINT32}, {hkClassMember::TYPE_VOID}, 0, {0},
		offsetof(hkpTypedBroadPhaseHandle, m_collisionFilterInfo), nullptr}};
hkClass hkpTypedBroadPhaseHandleClass("hkpTypedBroadPhaseHandle", &hkpBroadPhaseHandleClass,
	sizeof(hkpTypedBroadPhaseHandle), nullptr, 0, nullptr, 0, hkpTypedBroadPhaseHandle::Members, 4, nullptr, nullptr, 0,
	0);

int32_t hkpTypedBroadPhaseHandle::getType() const {
	return m_type;
}

void hkpTypedBroadPhaseHandle::SetCollisionFilterInfo(uint32_t a1) {
	m_collisionFilterInfo = a1;
}

const hkClass& hkpTypedBroadPhaseHandle::staticClass() {
	return hkpTypedBroadPhaseHandleClass;
}
