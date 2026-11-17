#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"

#include <stdint.h>

// getVtable for the type registry: returns hkpExtendedMeshShape's vptr.
const void* getVtablehkpExtendedMeshShape();
// The type info's finish-loaded-object callback.
void finishLoadedObjecthkpExtendedMeshShape(void* p, int32_t finishing);
void cleanupLoadedObjecthkpExtendedMeshShape(void* p);
// The finish function of hkpExtendedMeshShape::ShapesSubpart's type info.
void finishLoadedObjecthkpExtendedMeshShapeShapesSubpart(void* p, int32_t finishing);

extern hkClass hkpExtendedMeshShapeShapesSubpartClass;
extern hkClass hkpExtendedMeshShapeSubpartClass;
extern hkClass hkpExtendedMeshShapeTrianglesSubpartClass;
extern hkClass hkpExtendedMeshShapeClass;
extern hkTypeInfo hkpExtendedMeshShapeTypeInfo;
