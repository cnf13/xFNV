#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Reconstruction.hpp"

// Havok SDK signature (hkpCollide.lib, tied by reloc,target).
const void* getVtablehkpConvexPieceMeshShape();
void cleanupLoadedObjecthkpConvexPieceMeshShape(void* p);

extern hkClass hkpConvexPieceMeshShapeClass;

extern hkTypeInfo hkpConvexPieceMeshShapeTypeInfo;
