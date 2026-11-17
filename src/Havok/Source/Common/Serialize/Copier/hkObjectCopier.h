#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Serialize/Util/hkStructureLayout.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkOArchive;
class hkRelocationInfo;
class hkStreamWriter;

// Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
hkResult applyDefaultsFor(const hkClass& a1, const hkClassMember& a2, int32_t a3, hkStreamWriter* a4);
// Not reconstructed yet. The PDB's `anonymous namespace'::saveCstring of hkObjectCopier.cpp (usercall: two stack
// arguments, ebx, esi; 0xE03E20).
// Inferred type: a1: char* from 0xe03e81 (passed to hkString::strLen 0); r_ebx: hkOArchive* from 0xe04098 (parameter 2
// of hkObjectCopier::saveExtras); r_esi: hkRelocationInfo* from 0xe04098 (parameter 5 of hkObjectCopier::saveExtras),
// its members used at 0xe03e59
int32_t saveCstring(char* a1, int32_t a2, hkOArchive* r_ebx, hkRelocationInfo* r_esi);
// The PDB's `anonymous namespace'::inRange of hkObjectCopier.cpp: whether lo <= num < hi (cdecl, ret 0, 0xE030E0;
// it sets all of eax to 0 or 1).
bool inRange(int32_t num, int32_t lo, int32_t hi);
// The PDB's objectCopierPadUp, a free function of hkObjectCopier.cpp: pads apWriter with zero bytes up to the next
// multiple of aiAlign (a power of two). The exe passes aiAlign in eax and apWriter in edi (usercall, ret 0, 0xE03360).
void objectCopierPadUp(hkStreamWriter* w, int32_t pad);
// The PDB's writeUlongArray, a free function of hkObjectCopier.cpp: writes the nelem hkUlongs
// at startAddress into oa as elemsize-byte elements (4: as they are; else each widened to 8 bytes in stack memory).
// The exe passes oa and elemsize on the stack, startAddress in ebx and nelem in edi (usercall, ret 0, 0xE03300).
void writeUlongArray(hkOArchive& oa, int32_t elemsize, int32_t nelem, const hkUlong* startAddress);

// Copies an object of one platform layout into another, writing the converted bytes and the pointers to relocate.
// The exe's vtable overrides the destructor and adds slots 3 to 5. hkBool32 is spelled int32_t.
class hkObjectCopier : public hkReferencedObject {
public:
	enum ObjectCopierFlagBits : int32_t {
		FLAG_NONE = 0x0,
		FLAG_APPLY_DEFAULT_IF_SERIALIZE_IGNORED = 0x1,
		FLAG_RESPECT_SERIALIZE_IGNORED = 0x2,
	};

	hkObjectCopier(const hkStructureLayout& layoutIn, const hkStructureLayout& layoutOut, uint32_t flags);
	~hkObjectCopier() override;

	virtual hkResult copyObject(const void* dataIn, const hkClass& klassIn, hkStreamWriter* dataOut,
		const hkClass& klassOut, hkRelocationInfo& reloc);

protected:
	virtual int32_t areMembersCompatible(const hkClassMember& src, const hkClassMember& dst);

private:
	virtual const hkClass* lookupClass(const hkClass& klass) { return &klass; }

public:
	const hkStructureLayout& getLayoutOut() const { return m_layoutOut; }
	const hkStructureLayout& getLayoutIn() const { return m_layoutIn; }

protected:
	void writeZero(hkOArchive& oa, const hkClassMember& memberOut);
	// KEEP, not registered (an uninitialised read: the comment above its body in hkObjectCopier.cpp).
	int32_t saveBody(const void* dataIn, const hkClass& klassIn, hkOArchive& oa, const hkClass& klassOut);
	void saveExtras(const void* dataIn, const hkClass& klassIn, hkOArchive& dataOut, const hkClass& klassOut,
		int32_t classStart, hkRelocationInfo& fixups, int32_t level);

public:
	// Not named in the PDB or IDA; name chosen here. member's C-style array size, or 1 when
	// it is no C-style array; the exe passes member in esi (usercall, ret 0, 0xE03100); writeZero multiplies sizes
	// by it. Public: its usercall registration names it from namespace scope.
	// Inferred type: return: int32_t: signed uses from 0xe03277; member: hkClassMember* from 0xe03270 (parameter 1 of
	// applyDefaultsFor)
	static int32_t getNumElements(const hkClassMember& member);
	// The PDB's `anonymous namespace'::writePodArray of hkObjectCopier.cpp (usercall: ecx, edx, two stack arguments,
	// eax; 0xE03170).
	// Inferred type: a3: hkOArchive* from 0xe037f1 (parameter 2 of hkObjectCopier::saveBody)
	static void writePodArray(int32_t a1, uint32_t a2, hkOArchive* a3, char* a4, int32_t r_eax);

	hkStructureLayout m_layoutIn;
	hkStructureLayout m_layoutOut;
	hkFlags<ObjectCopierFlagBits, uint32_t> m_flags;
	hkBool m_byteSwap;
};
inline constexpr fnvd::Vtable hkObjectCopier_vftable{};
