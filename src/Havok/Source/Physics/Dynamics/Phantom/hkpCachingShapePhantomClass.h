#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Reconstruction.hpp"

const void* getVtablehkpCachingShapePhantom();
// The class registry's finish-loaded-object callback.
void finishLoadedObjecthkpCachingShapePhantom(void* p, int32_t finishing);
void cleanupLoadedObjecthkpCachingShapePhantom(void* p);

extern hkClass hkpCachingShapePhantomClass;

extern hkTypeInfo hkpCachingShapePhantomTypeInfo;
