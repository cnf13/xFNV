#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Copier/hkObjectCopier.h"

#include <stdint.h>

class hkObjectUpdateTracker;

// Helpers of the versioning patches.
namespace hkVersionUtil {
// Writes the new class's default value of every member the old class does not have.
void copyDefaults(void* obj, const hkClass& oldClass, const hkClass& newClass);

// Copies the member oldName of oldObj into the member newName of newObj when both exist.
void renameMember(hkVariant& oldObj, const char* oldName, hkVariant& newObj, const char* newName);
char* getCurrentVersion();
} // namespace hkVersionUtil

namespace hkVersionUtil {
// Copies every object of objectsIn as its class in objectsOut into one chunk the tracker owns; returns the chunk.
void* copyObjects(const hkArrayBase<hkVariant>& objectsIn, hkArray<hkVariant, hkContainerHeapAllocator>& objectsOut,
	hkObjectUpdateTracker& tracker);
}  // namespace hkVersionUtil

// The object copier the versioning's object copy builds on the stack: an hkObjectCopier with no members of its own that
// also accepts the member type pairs packfiles store for one another.
class PackfileObjectCopier : public hkObjectCopier {
public:
	PackfileObjectCopier(const hkStructureLayout& layoutIn, const hkStructureLayout& layoutOut);

protected:
	// Returns 1 if hkObjectCopier's test passes, or if src's and dst's types are in a pair this copier also allows
	// (0x1A with 0x16, 0x1D with 0x21, 0x1C with a 0x14 whose struct class has a specific class's name), the subtypes
	// alike; otherwise 0.
	int32_t areMembersCompatible(const hkClassMember& src, const hkClassMember& dst) override;
};

namespace hkVersionUtil {
// Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
void recomputeClassMemberOffsets(hkClass* const* classes, int32_t classVersion);
} // namespace hkVersionUtil
