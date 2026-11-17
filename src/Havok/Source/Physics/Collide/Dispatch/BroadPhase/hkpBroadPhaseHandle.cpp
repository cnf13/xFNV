#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpBroadPhaseHandle.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeGroupClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpBroadPhaseHandleTypeInfo = {"hkpBroadPhaseHandle", finishLoadedObjecthkpBroadPhaseHandle,
	cleanupLoadedObjecthkpBroadPhaseHandle, nullptr, &typeid(hkpBroadPhaseHandle), sizeof(hkpBroadPhaseHandle)};

// Not named in the PDB or IDA; name chosen here (the SDK's class-file name). The reflected members of
// hkpBroadPhaseHandle, as the exe's .rdata holds them.
using hkpBroadPhaseHandleClass_MembersArray = const hkClassMember[1];
static hkpBroadPhaseHandleClass_MembersArray hkpBroadPhaseHandleClass_Members{
	{"id", nullptr, nullptr, {hkClassMember::TYPE_UINT32}, {hkClassMember::TYPE_VOID}, 0,
		{hkClassMember::SERIALIZE_IGNORED}, offsetof(hkpBroadPhaseHandle, m_id), nullptr}};
hkClass hkpBroadPhaseHandleClass("hkpBroadPhaseHandle", nullptr, sizeof(hkpBroadPhaseHandle), nullptr, 0, nullptr, 0,
	hkpBroadPhaseHandleClass_Members, 1, nullptr, nullptr, 0, 0);

const hkClass& hkpBroadPhaseHandle::staticClass() {
	return hkpBroadPhaseHandleClass;
}
