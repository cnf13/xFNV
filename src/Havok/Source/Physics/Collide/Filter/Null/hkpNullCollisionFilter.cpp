#include "Havok/Source/Physics/Collide/Filter/Null/hkpNullCollisionFilter.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollidableCollidableFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpRayCollidableFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpRayShapeCollectionFilter.h"
#include "Havok/Source/Physics/Collide/Filter/hkpShapeCollectionFilter.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/Modifiers/hkpRemoveTerminalsMoppModifierClass.h"

hkClass hkpNullCollisionFilterClass("hkpNullCollisionFilter", &hkpCollisionFilterClass, 48, nullptr, 0, nullptr, 0,
	nullptr, 0, nullptr, nullptr, 0, 0);
hkTypeInfo hkpNullCollisionFilterTypeInfo = {"hkpNullCollisionFilter", finishLoadedObjecthkpNullCollisionFilter,
	cleanupLoadedObjecthkpNullCollisionFilter, getVtablehkpNullCollisionFilter(), &typeid(hkpNullCollisionFilter),
	sizeof(hkpNullCollisionFilter)};

hkpNullCollisionFilter::~hkpNullCollisionFilter() {}

// Every overload lets the pair collide; the bodies are the header's.
const hkClass& hkpNullCollisionFilter::staticClass() {
	return hkpNullCollisionFilterClass;
}
