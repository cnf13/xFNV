#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Serialize/Util/hkStructureLayout.h"

#include <stdint.h>

class hkOArchive;
class hkRelocationInfo;
class hkStreamWriter;

// Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
hkResult applyDefaultsFor(const hkClass& a1, const hkClassMember& a2, int32_t a3, hkStreamWriter* a4);
int32_t saveCstring(char* a1, int32_t a2, hkOArchive* r_ebx, hkRelocationInfo* r_esi);
// Whether lo <= num < hi.
bool inRange(int32_t num, int32_t lo, int32_t hi);
// A free function of hkObjectCopier.cpp: pads w with zero bytes up to the next multiple of pad (a power of two).
void objectCopierPadUp(hkStreamWriter* w, int32_t pad);
// Writes the nelem hkUlongs at startAddress into oa as elemsize-byte elements (4: as they are; else each widened to 8
// bytes in stack memory).
void writeUlongArray(hkOArchive& oa, int32_t elemsize, int32_t nelem, const hkUlong* startAddress);

// Copies an object of one platform layout into another, writing the converted bytes and the pointers to relocate.
// hkBool32 is spelled int32_t.
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
	int32_t saveBody(const void* dataIn, const hkClass& klassIn, hkOArchive& oa, const hkClass& klassOut);
	void saveExtras(const void* dataIn, const hkClass& klassIn, hkOArchive& dataOut, const hkClass& klassOut,
		int32_t classStart, hkRelocationInfo& fixups, int32_t level);

public:
	// Returns member's C-style array size, or 1 when it is no C-style array; writeZero multiplies sizes by it.
	static int32_t getNumElements(const hkClassMember& member);
	static void writePodArray(int32_t a1, uint32_t a2, hkOArchive* a3, char* a4, int32_t r_eax);

	hkStructureLayout m_layoutIn;
	hkStructureLayout m_layoutOut;
	hkFlags<ObjectCopierFlagBits, uint32_t> m_flags;
	hkBool m_byteSwap;
};
