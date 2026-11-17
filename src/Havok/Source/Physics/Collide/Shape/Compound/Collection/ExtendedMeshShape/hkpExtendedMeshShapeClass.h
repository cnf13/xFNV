#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// The SDK's getVtable for the type registry: hkpExtendedMeshShape's vptr.
const void* getVtablehkpExtendedMeshShape();
// The type info's finish-loaded-object callback (cdecl, 2 stack arguments, 0xD4B300).
void finishLoadedObjecthkpExtendedMeshShape(void* p, int32_t finishing);
void cleanupLoadedObjecthkpExtendedMeshShape(void* p);
// The finish function of hkpExtendedMeshShape::ShapesSubpart's type info.
void finishLoadedObjecthkpExtendedMeshShapeShapesSubpart(void* p, int32_t finishing);

extern hkClass hkpExtendedMeshShapeShapesSubpartClass;
extern hkClass hkpExtendedMeshShapeSubpartClass;
extern hkClass hkpExtendedMeshShapeTrianglesSubpartClass;
extern hkClass hkpExtendedMeshShapeClass;
extern hkTypeInfo hkpExtendedMeshShapeTypeInfo;
