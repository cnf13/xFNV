#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"

// What a serialization operation reports the objects it allocates, points at and replaces to. Every virtual is pure:
// the caller passes its own tracker. hkVersionUtil::copyObjects is handed a tracker and its worker calls addChunk and
// objectPointedBy.
class hkObjectUpdateTracker : public hkReferencedObject {
public:
	// An allocation the copy made.
	virtual void addAllocation(void* ptr) = 0;
	// A chunk the copy allocated, of a size and memory class; memoryClass is an int32_t because HK_MEMORY_CLASS is not
	// declared (the copy passes HK_MEMORY_CLASS_EXPORT, 5).
	virtual void addChunk(void* ptr, int32_t size, int32_t memoryClass) = 0;
	// An object that points at another one.
	virtual void objectPointedBy(void* obj, void* pointedBy) = 0;
	// A copied object that replaces another.
	virtual void replaceObject(void* oldObject, void* newObject, const hkClass* klass) = 0;
	// An object the copy finishes with, by name.
	virtual void addFinish(void* ptr, const char* name) = 0;
	virtual void removeFinish(void* ptr) = 0;
};
