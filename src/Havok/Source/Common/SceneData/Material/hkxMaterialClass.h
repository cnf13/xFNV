#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"

#include <stdint.h>

void cleanupLoadedObjecthkxMaterial(void* p);
// Havok SDK signature (hkSceneData.lib, tied by bytes,reloc).
const void* getVtablehkxMaterial();

extern hkClass hkxMaterialTextureStageClass;
extern hkClass hkxMaterialClass;
extern hkTypeInfo hkxMaterialTypeInfo;
