#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"

// Helpers that grow, shrink and edit the atom of a simple contact constraint, whose contact points and their
// properties follow it in memory.
class hkpSimpleContactConstraintAtomUtil {
public:
    // A new atom with room for numReservedContactPoints points and the user datas of each
    // body.
    static hkpSimpleContactConstraintAtom* allocateAtom(
        int32_t numReservedContactPoints,
        int32_t numExtraUserDatasA,
        int32_t numExtraUserDatasB,
        int32_t maxNumContactPoints
    );
    // The block size the heap hands out for
    // an atom of size bytes (the next of 16, 32, 48, 64, 96, ...), which blockFree takes back.
    static int32_t getAllocatedAtomSize(int32_t size);

    static void deallocateAtom(hkpSimpleContactConstraintAtom* atom) {
        const int32_t allocatedSize = getAllocatedAtomSize(atom->m_sizeOfAllAtoms);
        hkMemoryRouter::getInstance().heap().blockFree(atom, allocatedSize);
    }

    // Removes the contact point at index and moves the later points and properties down.
    static void removeAtAndCopy(hkpSimpleContactConstraintAtom* atom, int32_t index);
    // Reallocates the atom when more than numFreeElemsLeft reserved points are free; sets
    // atomReallocated (the SDK's hkPadSpu<hkBool>, an hkBool on PC) and returns the atom to use.
    static hkpSimpleContactConstraintAtom* optimizeCapacity(
        hkpSimpleContactConstraintAtom* oldAtom_mightGetDeallocated,
        int32_t numFreeElemsLeft,
        hkBool& atomReallocated
    );
    // Copies src's info, contact points and properties into dst.
    static void copyContents(hkpSimpleContactConstraintAtom* dst, const hkpSimpleContactConstraintAtom* src);
    // Adds a contact point, reallocating a full atom.
    // Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
    static hkpSimpleContactConstraintAtom*
    expandOne(hkpSimpleContactConstraintAtom* oldAtom_mightGetDeallocated, hkPadSpu<hkBool>& atomReallocated);
};
