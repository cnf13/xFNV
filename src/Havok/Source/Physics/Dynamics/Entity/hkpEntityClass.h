#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Reconstruction.hpp"

const void* getVtablehkpEntity();
// The type info's finish-loaded-object callback for hkpEntity.
void finishLoadedObjecthkpEntity(void* p, int32_t finishing);
// The type info's cleanup callback of hkpEntity::ExtendedListeners.
void cleanupLoadedObjecthkpEntityExtendedListeners(void* p);
void cleanupLoadedObjecthkpEntity(void* p);
hkClass* getClassType();

extern hkClass hkpEntitySpuCollisionCallbackClass;
extern hkClass hkpEntityExtendedListenersClass;
extern hkClass hkpEntitySmallArraySerializeOverrideTypeClass;
extern hkClass hkpEntityClass;
extern hkTypeInfo hkpEntityTypeInfo;
