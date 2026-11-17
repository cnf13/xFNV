#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// Havok SDK signature (hkaRagdoll.lib, tied by reloc).
const void* getVtablehkaRagdollInstance();
void cleanupLoadedObjecthkaRagdollInstance(void* p);
void cleanupLoadedObjecthkaSkeletonMapper(void* p);

extern hkClass hkaSkeletonMapperClass;
extern hkTypeInfo hkaSkeletonMapperTypeInfo;
