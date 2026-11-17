#include "Havok/Source/Physics/Collide/Agent3/Machine/1n/hkpAgent1nTrack.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeGroupClass.h"

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpAgent1nSectorTypeInfo = {"hkpAgent1nSector", finishLoadedObjecthkpAgent1nSector,
	cleanupLoadedObjecthkpAgent1nSector, nullptr, &typeid(hkpAgent1nSector), sizeof(hkpAgent1nSector)};

// The reflected members of hkpAgent1nSector.
using hkpAgent1nSectorClass_MembersArray = const hkClassMember[5];
static hkpAgent1nSectorClass_MembersArray hkpAgent1nSectorClass_Members{
	{"bytesAllocated", nullptr, nullptr, {hkClassMember::TYPE_UINT32}, {hkClassMember::TYPE_VOID}, 0, {0},
		offsetof(hkpAgent1nSector, m_bytesAllocated), nullptr},
	{"pad0", nullptr, nullptr, {hkClassMember::TYPE_UINT32}, {hkClassMember::TYPE_VOID}, 0, {0},
		offsetof(hkpAgent1nSector, m_pad0), nullptr},
	{"pad1", nullptr, nullptr, {hkClassMember::TYPE_UINT32}, {hkClassMember::TYPE_VOID}, 0, {0},
		offsetof(hkpAgent1nSector, m_pad1), nullptr},
	{"pad2", nullptr, nullptr, {hkClassMember::TYPE_UINT32}, {hkClassMember::TYPE_VOID}, 0, {0},
		offsetof(hkpAgent1nSector, m_pad2), nullptr},
	{"data", nullptr, nullptr, {hkClassMember::TYPE_UINT8}, {hkClassMember::TYPE_VOID}, 496, {0},
		offsetof(hkpAgent1nSector, m_data), nullptr}};
hkClass hkpAgent1nSectorClass("hkpAgent1nSector", nullptr, sizeof(hkpAgent1nSector), nullptr, 0, nullptr, 0,
	hkpAgent1nSectorClass_Members, 5, nullptr, nullptr, 0, 0);

const hkClass& hkpAgent1nSector::staticClass() {
	return hkpAgent1nSectorClass;
}
