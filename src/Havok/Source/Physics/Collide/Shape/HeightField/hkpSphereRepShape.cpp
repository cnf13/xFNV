#include "Havok/Source/Physics/Collide/Shape/HeightField/hkpSphereRepShape.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"

// The class record of hkpSphereRepShape, as the SDK's generated file has it (the SDK has no .cpp of the class: this
// file holds its reflection data). An abstract shape: parent hkpShape, no members of its own.
hkClass hkpSphereRepShapeClass("hkpSphereRepShape", &hkpShapeClass, 16, nullptr, 0, nullptr, 0, nullptr, 0, nullptr,
	nullptr, 0, 0);
