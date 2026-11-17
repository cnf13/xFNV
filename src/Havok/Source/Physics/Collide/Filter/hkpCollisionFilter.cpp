#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollidableCollidableFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpRayCollidableFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpRayShapeCollectionFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpShapeCollectionFilter.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"

// The class record of hkpCollisionFilter, as the SDK's generated file has it. Names of tables the PDB does not name
// are the Havok SDK's.
// The hkpFilterType enum's items and the enum (.rdata), and the enum by pointer (.data; the PDB's name).
using hkpCollisionFilterhkpFilterTypeEnumItemsArray = const hkClassEnum::Item[7];
static hkpCollisionFilterhkpFilterTypeEnumItemsArray hkpCollisionFilterhkpFilterTypeEnumItems{
	{hkpCollisionFilter::HK_FILTER_UNKNOWN, "HK_FILTER_UNKNOWN"},
	{hkpCollisionFilter::HK_FILTER_NULL, "HK_FILTER_NULL"}, {hkpCollisionFilter::HK_FILTER_GROUP, "HK_FILTER_GROUP"},
	{hkpCollisionFilter::HK_FILTER_LIST, "HK_FILTER_LIST"}, {hkpCollisionFilter::HK_FILTER_CUSTOM, "HK_FILTER_CUSTOM"},
	{hkpCollisionFilter::HK_FILTER_PAIR, "HK_FILTER_PAIR"},
	{hkpCollisionFilter::HK_FILTER_CONSTRAINT, "HK_FILTER_CONSTRAINT"}};
using hkpCollisionFilterEnumsArray = const hkClassEnum[1];
static hkpCollisionFilterEnumsArray hkpCollisionFilterEnums{
	{"hkpFilterType", hkpCollisionFilterhkpFilterTypeEnumItems, 7, nullptr, 0}};
static const hkClassEnum* hkpCollisionFilterhkpFilterTypeEnum = &hkpCollisionFilterEnums[0];
// The PDB's name. The record's member table (.data): its type member's enum is null in the image: the exe's
// `dynamic initializer for 'hkpCollisionFilterClass_Members'' (0xFC08F0) stores it (below); the line holds it itself.
using hkpCollisionFilterClass_MembersArray = hkClassMember[3];
static hkpCollisionFilterClass_MembersArray hkpCollisionFilterClass_Members{
	{"prepad", nullptr, nullptr, {hkClassMember::TYPE_UINT32}, {hkClassMember::TYPE_VOID}, 2, 0, 0x18, nullptr},
	{"type", nullptr, &hkpCollisionFilterEnums[0], {hkClassMember::TYPE_ENUM}, {hkClassMember::TYPE_UINT32}, 0, 0, 0x20,
		nullptr},
	{"postpad", nullptr, nullptr, {hkClassMember::TYPE_UINT32}, {hkClassMember::TYPE_VOID}, 3, 0, 0x24, nullptr}};
// The exe's record passes four implemented interfaces with no interface table.
hkClass hkpCollisionFilterClass("hkpCollisionFilter", &hkReferencedObjectClass, 48, nullptr, 4, hkpCollisionFilterEnums,
	1, hkpCollisionFilterClass_Members, 3, nullptr, nullptr, 0, 0);

hkpCollisionFilter::hkpCollisionFilter() {
	m_type.m_storage = HK_FILTER_UNKNOWN;
}

hkpCollisionFilter::~hkpCollisionFilter() {}

// Every hit counts: the whole capacity is kept.
int32_t hkpCollisionFilter::numShapeKeyHitsLimitBreached(const hkpCollisionInput& input, const hkpCdBody& bodyA,
	const hkpCdBody& bodyB, const hkpBvTreeShape* bvTreeShapeB, hkAabb& aabb, uint32_t* shapeKeysInOut,
	int32_t shapeKeysCapacity) const {
	return shapeKeysCapacity;
}
