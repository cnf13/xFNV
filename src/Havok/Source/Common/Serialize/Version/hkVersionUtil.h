#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Copier/hkObjectCopier.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkObjectUpdateTracker;

// Helpers of the versioning patches (the SDK's and PDB's namespace).
namespace hkVersionUtil {
// Writes the new class's default value of every member the old class does not have.
void copyDefaults(void* obj, const hkClass& oldClass, const hkClass& newClass);

// Copies the member oldName of oldObj into the member newName of newObj when both exist.
void renameMember(hkVariant& oldObj, const char* oldName, hkVariant& newObj, const char* newName);
// Inferred type: return: char* from 0xd48f58 (passed to hkString::strCmp 1)
char* getCurrentVersion();
} // namespace hkVersionUtil

namespace hkVersionUtil {
// Copies every object of objectsIn as its class in objectsOut into one chunk the tracker owns; returns the chunk.
void* copyObjects(const hkArrayBase<hkVariant>& objectsIn, hkArray<hkVariant, hkContainerHeapAllocator>& objectsOut,
	hkObjectUpdateTracker& tracker);
}  // namespace hkVersionUtil

// The object copier the versioning's object copy (0xDC12C0) builds on the stack: the PDB's `anonymous
// namespace'::PackfileObjectCopier of hkVersionUtil.cpp, an hkObjectCopier with no members of its own (its
// constructor builds the base and stores only the vtable) that also accepts the member type pairs packfiles store
// for one another. The exe's vtable 0x10D6ACC keeps hkObjectCopier's deleting destructor (slot 0), copyObject (3) and
// lookupClass (5).
class PackfileObjectCopier : public hkObjectCopier {
public:
	PackfileObjectCopier(const hkStructureLayout& layoutIn, const hkStructureLayout& layoutOut);

protected:
	// hkObjectCopier's test, or src's and dst's types (+0xC) in a pair this copier also
	// allows (0x1A with 0x16, 0x1D with 0x21, 0x1C with a 0x14 whose struct class has the name of the class at
	// 0x1268748), the subtypes (+0xD) alike (thiscall, ret 8, all of eax 0 or 1, 0xDC1100).
	int32_t areMembersCompatible(const hkClassMember& src, const hkClassMember& dst) override;
};
inline constexpr fnvd::Vtable PackfileObjectCopier_vftable{};

namespace hkVersionUtil {
// Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
void recomputeClassMemberOffsets(hkClass* const* classes, int32_t classVersion);
}  // namespace hkVersionUtil
