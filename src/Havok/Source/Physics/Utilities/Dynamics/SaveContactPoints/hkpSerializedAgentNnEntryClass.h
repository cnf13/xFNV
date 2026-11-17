#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Reconstruction.hpp"

#include <stdint.h>

void cleanupLoadedObjecthkpSerializedAgentNnEntry(void* p);
void finishLoadedObjecthkpSerializedTrack1nInfo(void* p, int32_t finishing);
void finishLoadedObjecthkpSerializedSubTrack1nInfo(void* p, int32_t finishing);
void finishLoadedObjecthkpSerializedAgentNnEntry(void* p, int32_t finishing);
// A cleanup callback: destroys the
// hkpSerializedTrack1nInfo at p.
void cleanupLoadedObjecthkpSerializedTrack1nInfo(void* p);
// A cleanup callback: destroys the hkpSerializedTrack1nInfo at p.
void cleanupLoadedObjecthkpSerializedSubTrack1nInfo(void* p);

extern hkClass hkpSerializedSubTrack1nInfoClass;
extern hkClass hkpSerializedTrack1nInfoClass;
extern hkClass hkpSerializedAgentNnEntryClass;
extern hkTypeInfo hkpSerializedAgentNnEntryTypeInfo;
