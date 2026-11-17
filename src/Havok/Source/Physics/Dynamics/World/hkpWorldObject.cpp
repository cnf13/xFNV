#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Thread/CriticalSection/hkCriticalSection.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Reconstruction.hpp"

static void cleanupLoadedObjecthkpPropertyValue(void* p);
static void cleanupLoadedObjecthkpProperty(void* p);
static void finishLoadedObjecthkpProperty(void* p, int32_t finishing);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpPropertyValueTypeInfo = {"hkpPropertyValue", nullptr, cleanupLoadedObjecthkpPropertyValue, nullptr,
	&typeid(hkpPropertyValue), sizeof(hkpPropertyValue)};
hkTypeInfo hkpPropertyTypeInfo = {"hkpProperty", finishLoadedObjecthkpProperty, cleanupLoadedObjecthkpProperty, nullptr,
	&typeid(hkpProperty), sizeof(hkpProperty)};

// Not named in the PDB or IDA; names chosen here (the SDK's class-file names). The reflected enums of
// hkpWorldObject, as the exe's .rdata holds them.
using hkpWorldObjectMtChecksEnumItemsArray = const hkClassEnum::Item[2];
static hkpWorldObjectMtChecksEnumItemsArray hkpWorldObjectMtChecksEnumItems{
	{hkpWorldObject::MULTI_THREADING_CHECKS_ENABLE, "MULTI_THREADING_CHECKS_ENABLE"},
	{hkpWorldObject::MULTI_THREADING_CHECKS_IGNORE, "MULTI_THREADING_CHECKS_IGNORE"}};
using hkpWorldObjectBroadPhaseTypeEnumItemsArray = const hkClassEnum::Item[5];
static hkpWorldObjectBroadPhaseTypeEnumItemsArray hkpWorldObjectBroadPhaseTypeEnumItems{
	{hkpWorldObject::BROAD_PHASE_INVALID, "BROAD_PHASE_INVALID"},
	{hkpWorldObject::BROAD_PHASE_ENTITY, "BROAD_PHASE_ENTITY"},
	{hkpWorldObject::BROAD_PHASE_PHANTOM, "BROAD_PHASE_PHANTOM"},
	{hkpWorldObject::BROAD_PHASE_BORDER, "BROAD_PHASE_BORDER"},
	{hkpWorldObject::BROAD_PHASE_MAX_ID, "BROAD_PHASE_MAX_ID"}};
using hkpWorldObjectEnumsArray = const hkClassEnum[2];
static hkpWorldObjectEnumsArray hkpWorldObjectEnums{{"MtChecks", hkpWorldObjectMtChecksEnumItems, 2, nullptr, {0}},
	{"BroadPhaseType", hkpWorldObjectBroadPhaseTypeEnumItems, 5, nullptr, {0}}};
hkpWorldObject::MembersArray hkpWorldObject::Members{{"world", nullptr, nullptr, hkClassMember::TYPE_POINTER,
														 hkClassMember::TYPE_VOID, 0, hkClassMember::SERIALIZE_IGNORED,
														 0x8, nullptr},
	{"userData", nullptr, nullptr, hkClassMember::TYPE_ULONG, hkClassMember::TYPE_VOID, 0, 0, 0xC, nullptr},
	{"collidable", &hkpLinkedCollidableClass, nullptr, hkClassMember::TYPE_STRUCT, hkClassMember::TYPE_VOID, 0, 0, 0x10,
		nullptr},
	{"multiThreadCheck", &hkMultiThreadCheckClass, nullptr, hkClassMember::TYPE_STRUCT, hkClassMember::TYPE_VOID, 0, 0,
		0x6C, nullptr},
	{"name", nullptr, nullptr, hkClassMember::TYPE_STRINGPTR, hkClassMember::TYPE_VOID, 0, 0, 0x74, nullptr},
	{"properties", &hkpPropertyClass, nullptr, hkClassMember::TYPE_ARRAY, hkClassMember::TYPE_STRUCT, 0, 0, 0x78,
		nullptr},
	{"aiData", nullptr, nullptr, hkClassMember::TYPE_POINTER, hkClassMember::TYPE_VOID, 0,
		hkClassMember::SERIALIZE_IGNORED, 0x84, nullptr},
	{"treeData", nullptr, nullptr, hkClassMember::TYPE_POINTER, hkClassMember::TYPE_VOID, 0,
		hkClassMember::SERIALIZE_IGNORED, 0x88, nullptr}};

// Not named in the PDB or IDA; names chosen here (the SDK's class-file names). The reflected members of
// hkpPropertyValue and hkpProperty, as the exe's .rdata holds them.
using hkpPropertyValueClass_MembersArray = const hkClassMember[1];
static hkpPropertyValueClass_MembersArray hkpPropertyValueClass_Members{
	{"data", nullptr, nullptr, hkClassMember::TYPE_UINT64, hkClassMember::TYPE_VOID, 0, 0, 0x0, nullptr}};
using hkpPropertyClass_MembersArray = const hkClassMember[3];
static hkpPropertyClass_MembersArray hkpPropertyClass_Members{
	{"key", nullptr, nullptr, hkClassMember::TYPE_UINT32, hkClassMember::TYPE_VOID, 0, 0, 0x0, nullptr},
	{"alignmentPadding", nullptr, nullptr, hkClassMember::TYPE_UINT32, hkClassMember::TYPE_VOID, 0, 0, 0x4, nullptr},
	{"value", &hkpPropertyValueClass, nullptr, hkClassMember::TYPE_STRUCT, hkClassMember::TYPE_VOID, 0, 0, 0x8,
		nullptr}};
hkClass hkpWorldObjectClass("hkpWorldObject", &hkReferencedObjectClass, 140, nullptr, 0, hkpWorldObjectEnums, 2,
	hkpWorldObject::Members, 8, nullptr, nullptr, 0, 0);
hkClass hkpPropertyValueClass("hkpPropertyValue", nullptr, 8, nullptr, 0, nullptr, 0, hkpPropertyValueClass_Members, 1,
	nullptr, nullptr, 0, 0);
hkClass hkpPropertyClass("hkpProperty", nullptr, 16, nullptr, 0, nullptr, 0, hkpPropertyClass_Members, 3, nullptr,
	nullptr, 0, 0);

static void cleanupLoadedObjecthkpPropertyValue(void* p) {
	;
}

static void cleanupLoadedObjecthkpProperty(void* p) {
	;
}

static void finishLoadedObjecthkpProperty(void* p, int32_t finishing) {
	;
}

const hkClass& hkpPropertyValue::staticClass() {
	return hkpPropertyValueClass;
}

const hkClass& hkpProperty::staticClass() {
	return hkpPropertyClass;
}

// m_treeData starts as the exe's sentinel -1 (no tree node yet), an address value rather than an object.
hkpWorldObject::hkpWorldObject(const hkpShape* a1, hkpWorldObject::BroadPhaseType a2)
	: m_world(nullptr), m_userData(nullptr), m_collidable(a1, static_cast<const hkMotionState*>(nullptr), a2),
	  m_aiData(nullptr), m_treeData(reinterpret_cast<void*>(static_cast<intptr_t>(-1))) {
	m_multiThreadCheck.m_threadId = hkMultiThreadCheck::UNMARKED;
	m_multiThreadCheck.m_markCount = 0x8000;
	m_collidable.setOwner(this);
	if (a1) {
		a1->addReference();
	}
}

// The SDK's finish constructor (thiscall, ret 4): hkReferencedObject's stores nothing; the exe inlines
// hkpLinkedCollidable's (hkpCollidable's with its handle's and bounding volume data's, and a default-constructed
// collision entry array) and hkMultiThreadCheck's default constructor (the release build's 0x8000 mark count), calls
// hkStringPtr's finish constructor on m_name, and m_properties' finish constructor stores nothing.
// hkMultiThreadCheck has no constructor in src: its fields are set here, as the exe does.
hkpWorldObject::hkpWorldObject(hkFinishLoadedObjectFlag flag)
	: hkReferencedObject(flag), m_collidable(flag), m_name(flag), m_properties(flag) {
	m_multiThreadCheck.m_threadId = hkMultiThreadCheck::UNMARKED;
	m_multiThreadCheck.m_markCount = 0x8000;
	if (flag.m_finishing) {
		m_collidable.setOwner(this);
	}
}

// The name, the properties and the collidable's entries free themselves; the shape and the AI data are held.
hkpWorldObject::~hkpWorldObject() {
	if (m_collidable.m_shape) {
		m_collidable.m_shape->removeReference();
	}
	if (m_aiData) {
		m_aiData->removeReference();
	}
}

// The shape, the collision entry array and the property array, then the collidable's child shape AABBs. The two
// arrays report their used and allocated bytes through the collector's array hook; the AABBs through its normal
// chunk hook. apClass is unused.
void hkpWorldObject::calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const {
	collector->addReferencedObject("Shape", m_collidable.m_shape, 0);
	const uint32_t uiCollisionCapacityAndFlags = m_collidable.m_collisionEntries.m_capacityAndFlags;
	if ((static_cast<int32_t>(uiCollisionCapacityAndFlags) >= 0) || collector->m_includeUnmanagedArrayMemory) {
		const int32_t iCollisionAllocated =
			hkStatisticsCollector::getAllocatedSize(m_collidable.m_collisionEntries.m_data,
				8 *
					static_cast<int32_t>(uiCollisionCapacityAndFlags &
						hkArrayBase<hkpLinkedCollidable::CollisionEntry>::CAPACITY_MASK));
		collector->addChunk(hkStatisticsCollector::MEM_ARRAY, "CollAgtPtr", m_collidable.m_collisionEntries.m_data,
			8 * m_collidable.m_collisionEntries.m_size, iCollisionAllocated);
	}
	const uint32_t uiPropertyCapacityAndFlags = m_properties.m_capacityAndFlags;
	if ((static_cast<int32_t>(uiPropertyCapacityAndFlags) >= 0) || collector->m_includeUnmanagedArrayMemory) {
		const int32_t iPropertyAllocated = hkStatisticsCollector::getAllocatedSize(m_properties.m_data,
			16 * static_cast<int32_t>(uiPropertyCapacityAndFlags & hkArrayBase<hkpProperty>::CAPACITY_MASK));
		collector->addChunk(hkStatisticsCollector::MEM_ARRAY, "Properties", m_properties.m_data,
			16 * m_properties.m_size, iPropertyAllocated);
	}
	if (m_collidable.m_boundingVolumeData.m_childShapeAabbs != nullptr) {
		collector->addNormalChunk("ChildShapeAabbs", m_collidable.m_boundingVolumeData.m_childShapeAabbs,
			32 * m_collidable.m_boundingVolumeData.m_capacityChildShapeAabbs, 0);
		collector->addNormalChunk("ChildShapeAabbs", m_collidable.m_boundingVolumeData.m_childShapeKeys,
			4 * m_collidable.m_boundingVolumeData.m_capacityChildShapeAabbs, 0);
	}
}

// An existing key keeps its entry; a new one is appended, growing the array first when it is full. The exe fetches
// the memory router before the full test.
void hkpWorldObject::addProperty(uint32_t key, hkpPropertyValue value) {
	const int32_t iSize = m_properties.m_size;
	for (int32_t i = 0; i < iSize; ++i) {
		if (m_properties.m_data[i].m_key == key) {
			return;
		}
	}
	hkMemoryRouter& rRouter = hkMemoryRouter::getInstance();
	if (m_properties.m_size == (int32_t)(m_properties.m_capacityAndFlags & hkArrayBase<hkpProperty>::CAPACITY_MASK)) {
		hkArrayUtil::_reserveMore(rRouter.heap(), &m_properties, sizeof(hkpProperty));
	}
	hkpProperty* pkProperty = &m_properties.m_data[m_properties.m_size++];
	pkProperty->m_value = value;
	pkProperty->m_key = key;
}

// The found entry's value is returned and the entries after it slide down over it (hkArray's removeAtAndCopy: its
// byte count wraps at 32 bits, as the exe's does).
hkpPropertyValue hkpWorldObject::removeProperty(uint32_t key) {
	const int32_t iSize = m_properties.m_size;
	for (int32_t i = 0; i < iSize; ++i) {
		if (m_properties.m_data[i].m_key == key) {
			hkpPropertyValue kValue = m_properties.m_data[i].m_value;
			m_properties.removeAtAndCopy(i);
			return kValue;
		}
	}
	hkpPropertyValue kValue;
	kValue.m_data = 0;
	return kValue;
}

void hkpWorldObject::editProperty(uint32_t key, hkpPropertyValue value, hkpWorldObject::MtChecks mtCheck) {
	int32_t m_size = m_properties.m_size;
	int32_t v5 = 0;
	if (m_size > 0) {
		hkpProperty* m_data = m_properties.begin();
		for (hkpProperty* i = m_data; i->m_key != key; ++i) {
			if (++v5 >= m_size) {
				return;
			}
		}
		m_data[v5].m_value = value;
	}
}

void hkpWorldObject::markForWriteImpl() {
	;
}

void hkpWorldObject::checkReadWrite() {
	;
}

void hkpWorldObject::checkReadOnly() const {
	;
}

void hkpWorldObject::setTreeData(void* a1) {
	m_treeData = a1;
}

void* hkpWorldObject::getTreeData() const {
	return m_treeData;
}

void hkpWorldObject::unlockProperty(uint32_t a1) {
	hkpWorld* m_world;
	hkCriticalSection* v3;
	m_world = this->m_world;
	if (m_world) {
		v3 = m_world->m_propertyMasterLock;
		if (v3) {
			LeaveCriticalSection(&v3->m_section);
		}
	}
}

void hkpWorldObject::lockProperty(uint32_t key) {
	hkpWorld* m_world = this->m_world;
	if (m_world) {
		hkCriticalSection* m_propertyMasterLock = m_world->m_propertyMasterLock;
		if (m_propertyMasterLock) {
			EnterCriticalSection(&m_propertyMasterLock->m_section);
		}
	}
}

hkaiObjectData* hkpWorldObject::getAiObjectData() {
	return static_cast<hkaiObjectData*>(m_aiData);
}

void hkpWorldObject::setAiObjectData(hkaiObjectData* data) {
	data->addReference();
	hkReferencedObject* m_aiData = this->m_aiData;
	if (m_aiData) {
		m_aiData->removeReference();
	}
	this->m_aiData = data;
}
