#include "Havok/Source/Physics/Dynamics/Constraint/Atom/hkpConstraintAtomUtil.h"

#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Contact/hkpContactPointProperties.h"

// Allocates an atom with room for numReservedContactPoints contact points and their properties (each an
// hkpContactPointProperties, 20 bytes in the exe, plus a UserData per user data), rounded up to 16 bytes, from the
// heap's block allocator in getAllocatedAtomSize's bucket. The exe loads the heap into ecx before the
// getAllocatedAtomSize call and relies on that call leaving ecx intact.
hkpSimpleContactConstraintAtom* hkpSimpleContactConstraintAtomUtil::allocateAtom(int32_t numReservedContactPoints,
	int32_t numExtraUserDatasA, int32_t numExtraUserDatasB, int32_t maxNumContactPoints) {
	const int32_t striding =
		static_cast<int32_t>(sizeof(hkpContactPointProperties::UserData)) * (numExtraUserDatasA + numExtraUserDatasB) +
		static_cast<int32_t>(sizeof(hkpContactPointProperties));
	const int32_t size = (static_cast<int32_t>(sizeof(hkpSimpleContactConstraintAtom)) + 15 +
							 numReservedContactPoints * (static_cast<int32_t>(sizeof(hkContactPoint)) + striding)) &
		~15;
	hkMemoryAllocator* heap = hkMemoryRouter::getInstance().m_heap;
	hkpSimpleContactConstraintAtom* atom = static_cast<hkpSimpleContactConstraintAtom*>(heap
			->blockAlloc(hkpSimpleContactConstraintAtomUtil::getAllocatedAtomSize(size)));
	atom->m_numUserDatasForBodyA = static_cast<uint8_t>(numExtraUserDatasA);
	atom->m_maxNumContactPoints = static_cast<uint16_t>(maxNumContactPoints);
	atom->m_numUserDatasForBodyB = static_cast<uint8_t>(numExtraUserDatasB);
	atom->m_sizeOfAllAtoms = static_cast<uint16_t>(size);
	atom->m_numReservedContactPoints = static_cast<uint16_t>(numReservedContactPoints);
	atom->m_contactPointPropertiesStriding = static_cast<uint8_t>(striding);
	atom->m_info.m_index = 3;
	atom->m_info.m_flags = 0;
	atom->m_info.m_data[0] = 0;
	atom->m_info.m_data[1] = 0;
	atom->m_info.m_data[2] = 0;
	atom->m_info.m_data[3] = 0;
	atom->m_info.m_data[4] = 0;
	atom->m_info.m_data[5] = 0;
	atom->m_info.m_data[6] = 0;
	// EXE-INLINED none: hkEnum<hkpConstraintAtom::AtomType, uint16_t>::operator=
	atom->m_type.m_storage = hkpConstraintAtom::TYPE_CONTACT;
	atom->m_numContactPoints = 0;
	return atom;
}

// KEEP: the exe body writes only eax, and allocateAtom keeps the heap in ecx and its vtable in edx across the call,
// which clang does not guarantee: it reads [edx + 4] at 0xD92549 and calls blockAlloc with that ecx at 0xD9254C after
// the call at 0xD92540.
int32_t hkpSimpleContactConstraintAtomUtil::getAllocatedAtomSize(int32_t size) {
	if (size <= 16) {
		return 16;
	}
	if (size <= 32) {
		return 32;
	}
	if (size <= 48) {
		return 48;
	}
	if (size <= 64) {
		return 64;
	}
	if (size <= 96) {
		return 96;
	}
	if (size <= 128) {
		return 128;
	}
	if (size <= 160) {
		return 160;
	}
	if (size <= 192) {
		return 192;
	}
	if (size <= 256) {
		return 256;
	}
	if (size <= 320) {
		return 320;
	}
	if (size <= 512) {
		return 512;
	}
	if (size <= 640) {
		return 640;
	}
	if (size <= 1024) {
		return 1024;
	}
	if (size <= 2048) {
		return 2048;
	}
	if (size <= 0x1000) {
		return 0x1000;
	}
	if (size <= 0x2000) {
		return 0x2000;
	}
	return size;
}

hkpSimpleContactConstraintAtom*
hkpSimpleContactConstraintAtomUtil::optimizeCapacity(hkpSimpleContactConstraintAtom* oldAtom_mightGetDeallocated,
	int32_t numFreeElemsLeft, hkBool& atomReallocated) {
	int32_t m_numReservedContactPoints = oldAtom_mightGetDeallocated->m_numReservedContactPoints;
	if (2 * (numFreeElemsLeft + oldAtom_mightGetDeallocated->m_numContactPoints) > m_numReservedContactPoints) {
		return oldAtom_mightGetDeallocated;
	}
	// The reserved count is a zero-extended uint16_t, so the halving is the same signed or unsigned.
	hkpSimpleContactConstraintAtom* Atom =
		hkpSimpleContactConstraintAtomUtil::allocateAtom(m_numReservedContactPoints >> 1,
			oldAtom_mightGetDeallocated->m_numUserDatasForBodyA, oldAtom_mightGetDeallocated->m_numUserDatasForBodyB,
			oldAtom_mightGetDeallocated->m_maxNumContactPoints);
	hkpSimpleContactConstraintAtomUtil::copyContents(Atom, oldAtom_mightGetDeallocated);
	int32_t AllocatedAtomSize =
		hkpSimpleContactConstraintAtomUtil::getAllocatedAtomSize(oldAtom_mightGetDeallocated->m_sizeOfAllAtoms);
	hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
	Instance->m_heap->blockFree(oldAtom_mightGetDeallocated, AllocatedAtomSize);
	atomReallocated.m_bool = 1;
	return Atom;
}

// Copies src's info, contact points and their properties into dst, whose reserved point count may differ (the
// properties start after the reserved points). The point count is reloaded on each step, as the exe does. The
// properties are the atom's untyped stream memory, stepped through by src's striding in bytes, as the SDK does.
void hkpSimpleContactConstraintAtomUtil::copyContents(hkpSimpleContactConstraintAtom* dst,
	const hkpSimpleContactConstraintAtom* src) {
	dst->m_info = src->m_info;
	dst->m_numContactPoints = src->m_numContactPoints;
	const int32_t striding = src->m_contactPointPropertiesStriding;
	hkContactPoint* dstCp = dst->getContactPoints();
	char* dstCpp = reinterpret_cast<char*>(dst->getContactPointPropertiesStream(0));
	const hkContactPoint* srcCp = src->getContactPoints();
	const char* srcCpp = reinterpret_cast<const char*>(src->getContactPointPropertiesStream(0));
	for (int32_t i = 0; i < src->m_numContactPoints; i++) {
		dstCp[i] = srcCp[i];
		hkString::memCpy4(dstCpp, srcCpp, striding >> 2);
		dstCpp += striding;
		srcCpp += striding;
	}
}

// Makes room for one more contact point: a full atom is replaced by one with twice its points (at least 4, at most
// m_maxNumContactPoints) and freed, and atomReallocated set. The new point's properties are zeroed.
hkpSimpleContactConstraintAtom*
hkpSimpleContactConstraintAtomUtil::expandOne(hkpSimpleContactConstraintAtom* oldAtom_mightGetDeallocated,
	hkPadSpu<hkBool>& atomReallocated) {
	hkpSimpleContactConstraintAtom* atom = oldAtom_mightGetDeallocated;
	const int32_t numContactPoints = oldAtom_mightGetDeallocated->m_numContactPoints;
	if (numContactPoints >= oldAtom_mightGetDeallocated->m_numReservedContactPoints) {
		int32_t newSize = numContactPoints > 2 ? numContactPoints : 2;
		newSize += newSize;
		const int32_t maxNumContactPoints = oldAtom_mightGetDeallocated->m_maxNumContactPoints;
		if (newSize >= maxNumContactPoints) {
			newSize = maxNumContactPoints;
		}
		atom = hkpSimpleContactConstraintAtomUtil::allocateAtom(newSize,
			oldAtom_mightGetDeallocated->m_numUserDatasForBodyA, oldAtom_mightGetDeallocated->m_numUserDatasForBodyB,
			maxNumContactPoints);
		hkpSimpleContactConstraintAtomUtil::copyContents(atom, oldAtom_mightGetDeallocated);
		const int32_t allocatedSize =
			hkpSimpleContactConstraintAtomUtil::getAllocatedAtomSize(oldAtom_mightGetDeallocated->m_sizeOfAllAtoms);
		hkMemoryRouter::getInstance().m_heap->blockFree(oldAtom_mightGetDeallocated, allocatedSize);
		// EXE-INLINED none: hkPadSpu<hkBool>::operator=
		atomReallocated.m_storage = true;
	}
	atom->m_numContactPoints = static_cast<uint16_t>(numContactPoints + 1);
	// The SDK's HK_HINT_SIZE16: the exe multiplies the index and the striding as 16-bit signed values.
	hkString::memSet4(atom->getContactPointPropertiesStream(static_cast<int16_t>(numContactPoints)), 0,
		atom->getContactPointPropertiesStriding() >> 2);
	return atom;
}
