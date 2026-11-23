#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"

#include <stdint.h>

// The type infos' finish callbacks, defined in hkpStorageExtendedMeshShapeClass.cpp.
void finishLoadedObjecthkpStorageExtendedMeshShapeMeshSubpartStorage(void* p, int32_t finishing);
void finishLoadedObjecthkpStorageExtendedMeshShapeShapeSubpartStorage(void* p, int32_t finishing);
void finishLoadedObjecthkpStorageExtendedMeshShape(void* p, int32_t finishing);
void cleanupLoadedObjecthkpStorageExtendedMeshShapeMeshSubpartStorage(void* p);
const void* getVtablehkpStorageExtendedMeshShapeMeshSubpartStorage();
void cleanupLoadedObjecthkpStorageExtendedMeshShapeShapeSubpartStorage(void* p);
const void* getVtablehkpStorageExtendedMeshShapeShapeSubpartStorage();
void cleanupLoadedObjecthkpStorageExtendedMeshShape(void* p);

extern hkClass hkpStorageExtendedMeshShapeMeshSubpartStorageClass;
extern hkTypeInfo hkpStorageExtendedMeshShapeMeshSubpartStorageTypeInfo;
extern hkClass hkpStorageExtendedMeshShapeShapeSubpartStorageClass;
extern hkTypeInfo hkpStorageExtendedMeshShapeShapeSubpartStorageTypeInfo;
extern hkClass hkpStorageExtendedMeshShapeClass;
extern hkTypeInfo hkpStorageExtendedMeshShapeTypeInfo;
