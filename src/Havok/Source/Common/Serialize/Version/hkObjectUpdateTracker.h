#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Reconstruction.hpp"

// What a serialization operation reports the objects it allocates, points at and replaces to. Every virtual is pure:
// the caller passes its own tracker. The PDB's vftable offsets are 12, 16, 20, 24, 28 and 32, after
// hkReferencedObject's slots; hkVersionUtil::copyObjects (0xDC15A0) is handed a tracker and its worker (0xDC12C0)
// calls addChunk (offset 16) and objectPointedBy (offset 20).
class hkObjectUpdateTracker : public hkReferencedObject {
public:
	// An allocation the copy made.
	virtual void addAllocation(void* ptr) = 0;
	// A chunk the copy allocated, of a size and memory class; the class is the PDB's HK_MEMORY_CLASS, which src does
	// not declare (the copy passes HK_MEMORY_CLASS_EXPORT, 5).
	virtual void addChunk(void* ptr, int32_t size, int32_t memoryClass) = 0;
	// An object that points at another one.
	virtual void objectPointedBy(void* obj, void* pointedBy) = 0;
	// A copied object that replaces another.
	virtual void replaceObject(void* oldObject, void* newObject, const hkClass* klass) = 0;
	// An object the copy finishes with, by name.
	virtual void addFinish(void* ptr, const char* name) = 0;
	virtual void removeFinish(void* ptr) = 0;
};
