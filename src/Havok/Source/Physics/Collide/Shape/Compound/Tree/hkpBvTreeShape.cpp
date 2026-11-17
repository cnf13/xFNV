#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/hkpBvTreeShape.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"

// hkpBvTreeShape's reflection data.

namespace {
// Per member its offset into this struct (-1: no default), then the defaults.
struct hkpBvTreeShape_DefaultStruct {
	int32_t s_defaultOffsets[1];
	uint8_t m_bvTreeType;
};
} // namespace

// The reflected enum of hkpBvTreeShape.
using hkpBvTreeShapeBvTreeTypeEnumItemsArray = const hkClassEnum::Item[4];
static hkpBvTreeShapeBvTreeTypeEnumItemsArray hkpBvTreeShapeBvTreeTypeEnumItems{
	{hkpBvTreeShape::BVTREE_MOPP, "BVTREE_MOPP"},
	{hkpBvTreeShape::BVTREE_TRISAMPLED_HEIGHTFIELD, "BVTREE_TRISAMPLED_HEIGHTFIELD"},
	{hkpBvTreeShape::BVTREE_USER, "BVTREE_USER"}, {hkpBvTreeShape::BVTREE_MAX, "BVTREE_MAX"}};
using hkpBvTreeShapeEnumsArray = const hkClassEnum[1];
static hkpBvTreeShapeEnumsArray hkpBvTreeShapeEnums{{"BvTreeType", hkpBvTreeShapeBvTreeTypeEnumItems, 4, nullptr, {0}}};

// The BvTreeType enum record, as the member table's bvTreeType row names it.
static const hkClassEnum* hkpBvTreeShapeBvTreeTypeEnum = &hkpBvTreeShapeEnums[0];

// The class record's member table: the bvTreeType row's enum is null in the image.
using hkpBvTreeShapeClass_MembersArray = hkClassMember[1];
static hkpBvTreeShapeClass_MembersArray hkpBvTreeShapeClass_Members{{"bvTreeType", nullptr,
	hkpBvTreeShapeBvTreeTypeEnum, {hkClassMember::TYPE_ENUM}, {hkClassMember::TYPE_UINT8}, 0, {0}, 0x10, nullptr}};

// The defaults the class record names: bvTreeType is BVTREE_USER.
static const hkpBvTreeShape_DefaultStruct hkpBvTreeShape_Default{{0x4}, hkpBvTreeShape::BVTREE_USER};
hkClass hkpBvTreeShapeClass("hkpBvTreeShape", &hkpShapeClass, 0x14, nullptr, 0, hkpBvTreeShapeEnums, 1,
	hkpBvTreeShapeClass_Members, 1, &hkpBvTreeShape_Default, nullptr, 0, 0);
