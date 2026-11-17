#pragma once

#include "Reconstruction.hpp"

template <typename T> class hkSmallArray;
struct hkpConstraintBrokenEvent;
class hkpConstraintInstance;
class hkpConstraintListener;
struct hkpConstraintRepairedEvent;

// The notifications a constraint runs when it changes: each tells every listener the constraint holds. The listeners
// themselves stay with the exe. The PDB places these in Constraint\Util\hkpConstraintCallbackUtil.cpp.
class hkpConstraintCallbackUtil {
public:
	// Tells apConstraint's listeners that it was added to a world.
	static void fireConstraintAdded(hkpConstraintInstance* constraint);
	// Tells apConstraint's listeners that it was removed from its world.
	static void fireConstraintRemoved(hkpConstraintInstance* constraint);
	// Tells apConstraint's listeners that it is being deleted.
	static void fireConstraintDeleted(hkpConstraintInstance* constraint);
	// Tells the broken constraint's listeners about the break.
	static void fireConstraintBroken(const hkpConstraintBrokenEvent& event);
	// Tells the repaired constraint's listeners about the repair.
	static void fireConstraintRepaired(const hkpConstraintRepairedEvent& event);
	// Removes the null entries from a listener array, keeping the order (the PDB's
	// cleanupNullPointers, a file-static template in Havok's source; this instance takes the array in eax and
	// returns it there: usercall, ret 0, 0xD85D40).
	static void cleanupNullPointers(hkSmallArray<hkpConstraintListener*>& arArray);
};
