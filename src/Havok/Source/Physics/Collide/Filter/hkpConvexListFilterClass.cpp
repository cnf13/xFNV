#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Physics/Collide/Filter/hkpConvexListFilter.h"

// The reflection data of hkpConvexListFilter: the names of ConvexListCollisionType's values, the enum, the enum by
// pointer (file-local data), then the class record: parent hkReferencedObject, no members.
using hkpConvexListFilterConvexListCollisionTypeEnumItemsArray = const hkClassEnum::Item[3];
static hkpConvexListFilterConvexListCollisionTypeEnumItemsArray hkpConvexListFilterConvexListCollisionTypeEnumItems{
	{hkpConvexListFilter::TREAT_CONVEX_LIST_AS_NORMAL, "TREAT_CONVEX_LIST_AS_NORMAL"},
	{hkpConvexListFilter::TREAT_CONVEX_LIST_AS_LIST, "TREAT_CONVEX_LIST_AS_LIST"},
	{hkpConvexListFilter::TREAT_CONVEX_LIST_AS_CONVEX, "TREAT_CONVEX_LIST_AS_CONVEX"}};
using hkpConvexListFilterEnumsArray = const hkClassEnum[1];
static hkpConvexListFilterEnumsArray hkpConvexListFilterEnums{
	{"ConvexListCollisionType", hkpConvexListFilterConvexListCollisionTypeEnumItems, 3, nullptr, {0}}};
static const hkClassEnum* hkpConvexListFilterConvexListCollisionTypeEnum = &hkpConvexListFilterEnums[0];

hkClass hkpConvexListFilterClass("hkpConvexListFilter", &hkReferencedObjectClass, 8, nullptr, 0,
	hkpConvexListFilterEnums, 1, nullptr, 0, nullptr, nullptr, 0, 0);
