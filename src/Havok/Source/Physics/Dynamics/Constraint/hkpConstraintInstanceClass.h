#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Reconstruction.hpp"

#include <stdint.h>

void cleanupLoadedObjecthkpConstraintInstance(void* p);

extern hkClass hkpConstraintInstanceSmallArraySerializeOverrideTypeClass;
extern hkClass hkpConstraintInstanceClass;
extern hkTypeInfo hkpConstraintInstanceTypeInfo;
