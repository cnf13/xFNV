#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Reconstruction.hpp"

class hkStreamReader;
class hkStreamWriter;
class hkStructureLayout;

// The visual debugger's object streaming: objects are written with their local and global pointer fixups and read
// back. Everything in it is static.
class hkObjectSerialize {
public:
	// A pointer inside the object to another place in the same object.
	struct LocalFixup {
		int32_t fromOffset;
		int32_t toOffset;
	};

	// A pointer inside the object to another object, by that object's id and class id.
	struct GlobalFixup {
		int32_t fromOffset;
		uint64_t toID;
		uint64_t toClassID;
	};

	static uint64_t readObject(hkStreamReader* s, void*& data, uint32_t& dataSize,
		hkArray<hkObjectSerialize::GlobalFixup, hkContainerHeapAllocator>& globalFixups, uint64_t& klassID);
	// Havok SDK signature (hkVisualize.lib, tied by bytes,reloc).
	static int32_t writeObject(hkStreamWriter* s, const hkStructureLayout& destLayout, uint64_t dataID,
		const void* data, const hkClass& klass,
		hkArray<hkObjectSerialize::GlobalFixup, hkContainerHeapAllocator>& globalFixups);
};
