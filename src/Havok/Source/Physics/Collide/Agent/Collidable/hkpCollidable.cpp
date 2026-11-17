#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"

#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Geometry/LocalFrame/hkLocalFrame.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeGroupClass.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpCollidableBoundingVolumeDataTypeInfo = {"hkpCollidableBoundingVolumeData",
	finishLoadedObjecthkpCollidableBoundingVolumeData, cleanupLoadedObjecthkpCollidableBoundingVolumeData, nullptr,
	&typeid(hkpCollidable::BoundingVolumeData), sizeof(hkpCollidable::BoundingVolumeData)};
hkTypeInfo hkpCollidableTypeInfo = {"hkpCollidable", finishLoadedObjecthkpCollidable, cleanupLoadedObjecthkpCollidable,
	nullptr, &typeid(hkpCollidable), sizeof(hkpCollidable)};

// The reflected members of hkpCollidable::BoundingVolumeData.
using hkpCollidableBoundingVolumeDataClass_MembersArray = const hkClassMember[10];
static hkpCollidableBoundingVolumeDataClass_MembersArray hkpCollidableBoundingVolumeDataClass_Members{
	{"min", nullptr, nullptr, {hkClassMember::TYPE_UINT32}, {hkClassMember::TYPE_VOID}, 3, {0},
		offsetof(hkpCollidable::BoundingVolumeData, m_min), nullptr},
	{"expansionMin", nullptr, nullptr, {hkClassMember::TYPE_UINT8}, {hkClassMember::TYPE_VOID}, 3, {0},
		offsetof(hkpCollidable::BoundingVolumeData, m_expansionMin), nullptr},
	{"expansionShift", nullptr, nullptr, {hkClassMember::TYPE_UINT8}, {hkClassMember::TYPE_VOID}, 0, {0},
		offsetof(hkpCollidable::BoundingVolumeData, m_expansionShift), nullptr},
	{"max", nullptr, nullptr, {hkClassMember::TYPE_UINT32}, {hkClassMember::TYPE_VOID}, 3, {0},
		offsetof(hkpCollidable::BoundingVolumeData, m_max), nullptr},
	{"expansionMax", nullptr, nullptr, {hkClassMember::TYPE_UINT8}, {hkClassMember::TYPE_VOID}, 3, {0},
		offsetof(hkpCollidable::BoundingVolumeData, m_expansionMax), nullptr},
	{"padding", nullptr, nullptr, {hkClassMember::TYPE_UINT8}, {hkClassMember::TYPE_VOID}, 0, {0},
		offsetof(hkpCollidable::BoundingVolumeData, m_padding), nullptr},
	{"numChildShapeAabbs", nullptr, nullptr, {hkClassMember::TYPE_UINT16}, {hkClassMember::TYPE_VOID}, 0,
		{hkClassMember::SERIALIZE_IGNORED}, offsetof(hkpCollidable::BoundingVolumeData, m_numChildShapeAabbs), nullptr},
	{"capacityChildShapeAabbs", nullptr, nullptr, {hkClassMember::TYPE_UINT16}, {hkClassMember::TYPE_VOID}, 0,
		{hkClassMember::SERIALIZE_IGNORED}, offsetof(hkpCollidable::BoundingVolumeData, m_capacityChildShapeAabbs),
		nullptr},
	{"childShapeAabbs", nullptr, nullptr, {hkClassMember::TYPE_POINTER}, {hkClassMember::TYPE_VOID}, 0,
		{hkClassMember::SERIALIZE_IGNORED}, offsetof(hkpCollidable::BoundingVolumeData, m_childShapeAabbs), nullptr},
	{"childShapeKeys", nullptr, nullptr, {hkClassMember::TYPE_POINTER}, {hkClassMember::TYPE_VOID}, 0,
		{hkClassMember::SERIALIZE_IGNORED}, offsetof(hkpCollidable::BoundingVolumeData, m_childShapeKeys), nullptr}};
hkClass hkpCollidableBoundingVolumeDataClass("hkpCollidableBoundingVolumeData", nullptr,
	sizeof(hkpCollidable::BoundingVolumeData), nullptr, 0, nullptr, 0, hkpCollidableBoundingVolumeDataClass_Members, 10,
	nullptr, nullptr, 0, 0);

// The reflection's enum records of hkpCollidable: the names of ForceCollideOntoPpuReasons' values, and the enum (the
// SDK's file-local data).
using hkpCollidableForceCollideOntoPpuReasonsEnumItemsArray = const hkClassEnum::Item[3];
static hkpCollidableForceCollideOntoPpuReasonsEnumItemsArray hkpCollidableForceCollideOntoPpuReasonsEnumItems{
	{hkpCollidable::FORCE_PPU_USER_REQUEST, "FORCE_PPU_USER_REQUEST"},
	{hkpCollidable::FORCE_PPU_SHAPE_REQUEST, "FORCE_PPU_SHAPE_REQUEST"},
	{hkpCollidable::FORCE_PPU_MODIFIER_REQUEST, "FORCE_PPU_MODIFIER_REQUEST"}};
using hkpCollidableEnumsArray = const hkClassEnum[1];
static hkpCollidableEnumsArray hkpCollidableEnums{
	{"ForceCollideOntoPpuReasons", hkpCollidableForceCollideOntoPpuReasonsEnumItems, 3, nullptr, {0}}};
hkpCollidable::MembersArray hkpCollidable::Members{
	{"ownerOffset", nullptr, nullptr, {hkClassMember::TYPE_INT8}, {hkClassMember::TYPE_VOID}, 0,
		{hkClassMember::SERIALIZE_IGNORED}, offsetof(hkpCollidable, m_ownerOffset), nullptr},
	{"forceCollideOntoPpu", nullptr, nullptr, {hkClassMember::TYPE_UINT8}, {hkClassMember::TYPE_VOID}, 0, {0},
		offsetof(hkpCollidable, m_forceCollideOntoPpu), nullptr},
	{"shapeSizeOnSpu", nullptr, nullptr, {hkClassMember::TYPE_UINT16}, {hkClassMember::TYPE_VOID}, 0,
		{hkClassMember::SERIALIZE_IGNORED}, offsetof(hkpCollidable, m_shapeSizeOnSpu), nullptr},
	{"broadPhaseHandle", &hkpTypedBroadPhaseHandleClass, nullptr, {hkClassMember::TYPE_STRUCT},
		{hkClassMember::TYPE_VOID}, 0, {0}, offsetof(hkpCollidable, m_broadPhaseHandle), nullptr},
	{"boundingVolumeData", &hkpCollidableBoundingVolumeDataClass, nullptr, {hkClassMember::TYPE_STRUCT},
		{hkClassMember::TYPE_VOID}, 0, {hkClassMember::SERIALIZE_IGNORED},
		offsetof(hkpCollidable, m_boundingVolumeData), nullptr},
	{"allowedPenetrationDepth", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0},
		offsetof(hkpCollidable, m_allowedPenetrationDepth), nullptr}};
hkClass hkpCollidableClass("hkpCollidable", &hkpCdBodyClass, sizeof(hkpCollidable), nullptr, 0, hkpCollidableEnums, 1,
	hkpCollidable::Members, 6, nullptr, nullptr, 0, 0);

// Only the first corners and the child shape count are cleared; the rest of the volume data is left as the caller
// had it.
hkpCollidable::BoundingVolumeData::BoundingVolumeData() {
	m_numChildShapeAabbs = 0;
	m_capacityChildShapeAabbs = 0;
	m_childShapeAabbs = nullptr;
	m_childShapeKeys = nullptr;
	m_min[0] = 1;
	m_max[0] = 0;
}

hkpCollidable::BoundingVolumeData::BoundingVolumeData(hkFinishLoadedObjectFlag flag) {
	if (flag.m_finishing) {
		m_capacityChildShapeAabbs = 0;
		m_numChildShapeAabbs = 0;
		m_childShapeAabbs = nullptr;
		m_childShapeKeys = nullptr;
		invalidate();
	}
}

void hkpCollidable::BoundingVolumeData::deallocate() {
	hkAabbUint32* m_childShapeAabbs;
	int32_t m_capacityChildShapeAabbs;
	uint32_t* m_childShapeKeys;
	m_childShapeAabbs = this->m_childShapeAabbs;
	m_capacityChildShapeAabbs = (int32_t)this->m_capacityChildShapeAabbs;
	hkMemoryRouter* instance = &hkMemoryRouter::getInstance();
	instance->m_heap->blockFree(m_childShapeAabbs, sizeof(hkAabbUint32) * m_capacityChildShapeAabbs);
	m_childShapeKeys = this->m_childShapeKeys;
	uint32_t v6 = (this->m_capacityChildShapeAabbs + 3) & 0xFFFFFFFCu;
	hkMemoryRouter* v7 = &hkMemoryRouter::getInstance();
	v7->m_heap->blockFree(m_childShapeKeys, (int32_t)(4 * v6));
	this->m_childShapeAabbs = nullptr;
	this->m_childShapeKeys = (uint32_t*)0;
	m_numChildShapeAabbs = (uint16_t)0;
	this->m_capacityChildShapeAabbs = (uint16_t)0;
}

void hkpCollidable::BoundingVolumeData::allocate(int32_t numChildShapes) {
	m_numChildShapeAabbs = numChildShapes;
	m_capacityChildShapeAabbs = numChildShapes;
	if (numChildShapes) {
		hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
		hkAabbUint32* v4 = static_cast<hkAabbUint32*>(Instance->m_heap->blockAlloc(sizeof(hkAabbUint32) *
			static_cast<uint16_t>(numChildShapes)));
		int32_t v5 = m_capacityChildShapeAabbs + 3;
		m_childShapeAabbs = v4;
		hkMemoryRouter* v6 = &hkMemoryRouter::getInstance();
		m_childShapeKeys = static_cast<uint32_t*>(v6->m_heap->blockAlloc(sizeof(uint32_t) * (v5 & 0xFFFFFFFCu)));
	}
}

const hkClass& hkpCollidable::BoundingVolumeData::staticClass() {
	return hkpCollidableBoundingVolumeDataClass;
}

// The same over a transform.
hkpCollidable::hkpCollidable(const hkpShape* shape, const hkTransform* ms, int32_t type)
	: m_ownerOffset(0), m_forceCollideOntoPpu(0), m_shapeSizeOnSpu(0), m_allowedPenetrationDepth(-1.0f) {
	// The transform is taken as const and kept in the body's non-const motion field.
	m_motion.pMotion = const_cast<hkTransform*>(ms);
	m_shape = shape;
	m_parent = nullptr;
	m_shapeKey = 0xFFFFFFFF;
	m_broadPhaseHandle.m_id = 0;
	m_broadPhaseHandle.m_type = static_cast<int8_t>(type);
	m_broadPhaseHandle.m_objectQualityType = -1;
	m_broadPhaseHandle.m_collisionFilterInfo = 0;
	m_broadPhaseHandle.setOwner(this);
}

// The body over the motion state, no parent and no shape key; the handle of type with its owner offset back to the
// collidable; the volume data empty (its own constructor). hkpCdBody and the handles have no constructors: their
// fields are set here, as the Construct above does.
hkpCollidable::hkpCollidable(const hkpShape* shape, const hkMotionState* ms, int32_t type)
	: m_ownerOffset(0), m_forceCollideOntoPpu(0), m_shapeSizeOnSpu(0), m_allowedPenetrationDepth(-1.0f) {
	// The motion state is taken as const and kept in the body's non-const motion field.
	m_motion.pState = const_cast<hkMotionState*>(ms);
	m_shape = shape;
	m_parent = nullptr;
	m_shapeKey = 0xFFFFFFFF;
	m_broadPhaseHandle.m_id = 0;
	m_broadPhaseHandle.m_type = static_cast<int8_t>(type);
	m_broadPhaseHandle.m_objectQualityType = -1;
	m_broadPhaseHandle.m_collisionFilterInfo = 0;
	m_broadPhaseHandle.setOwner(this);
}

// Finish constructor: finishes the volume data, then sets the handle's owner offset when finishing; the loaded bytes
// are kept otherwise.
hkpCollidable::hkpCollidable(hkFinishLoadedObjectFlag flag) : m_boundingVolumeData(flag) {
	if (flag.m_finishing) {
		m_broadPhaseHandle.setOwner(this);
	}
}

// Frees the child shape AABBs and keys.
hkpCollidable::~hkpCollidable() {
	if (m_boundingVolumeData.hasAllocations()) {
		m_boundingVolumeData.deallocate();
	}
}

// Flags a shape whose collections hold more than 100 children; no warning text is emitted.
hkBool hkpCollidable::checkPerformance() {
	hkBool isOk;
	if (!m_shape) {
		isOk.m_bool = false;
		return isOk;
	}
	char ok = 1;
	if (m_shape->m_type.m_storage == HK_SHAPE_LIST) {
		const hkpShapeContainer* pContainer = static_cast<const hkpShapeCollection*>(m_shape);
		if (pContainer->getNumChildShapes() > 100) {
			ok = 0;
		}
	}
	if (m_shape->m_type.m_storage == HK_SHAPE_TRIANGLE_COLLECTION) {
		const hkpShapeContainer* pContainer = static_cast<const hkpShapeCollection*>(m_shape);
		if (pContainer->getNumChildShapes() > 100) {
			ok = 0;
		}
	}
	if (m_shape->m_type.m_storage == HK_SHAPE_COLLECTION) {
		const hkpShapeContainer* pContainer = static_cast<const hkpShapeCollection*>(m_shape);
		if (pContainer->getNumChildShapes() > 100) {
			ok = 0;
		}
	}
	if (m_shape->m_type.m_storage == HK_SHAPE_TRANSFORM) {
		ok = 0;
	}
	isOk.m_bool = ok;
	return isOk;
}

const hkClass& hkpCollidable::staticClass() {
	return hkpCollidableClass;
}

const hkpCollidable* hkpCdBody::getRootCollidable() const {
	const hkpCdBody* body = this;
	while (body->m_parent != nullptr) {
		body = body->m_parent;
	}
	return static_cast<const hkpCollidable*>(body);
}
