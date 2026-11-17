#pragma once

#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Reconstruction.hpp"

class hkpConstraintInstance;
class hkpConstraintOwner;
class hkpEntity;
class hkpWorld;

// Helpers that add, remove and modify constraints in a world.
class hkpWorldConstraintUtil {
public:
	// The innermost modifier atom of constraint (the last one before the constraint's own atoms), or null without
	// modifiers.
	static hkpModifierConstraintAtom* findLastModifier(hkpConstraintInstance* instance);
	// Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
	static hkpModifierConstraintAtom* findModifier(hkpConstraintInstance* instance, hkpConstraintAtom::AtomType type);
	// Adds the constraint to the world and to the island its entities live in. Kept
	// unregistered: an uninitialised read (the reason is above its body).
	static void addConstraint(hkpWorld* world, hkpConstraintInstance* constraint);
	// Takes the constraint out of its island and its entities, then drops its reference.
	static void removeConstraint(hkpConstraintInstance* constraint);
	// Inserts the modifier atom s into the constraint's chain under constraintOwner.
	static void addModifier(hkpConstraintInstance* instance, hkpConstraintOwner& constraintOwner,
		hkpModifierConstraintAtom* newModifier);
	// Points the atom or modifier that held oldAtom at updatedAtom after the contact atom was reallocated (cdecl,
	// ret 0, 0xD275A0; the size is read as a word).
	static void updateFatherOfMovedAtom(hkpConstraintInstance* instance, const hkpConstraintAtom* oldAtom,
		const hkpConstraintAtom* updatedAtom, int32_t updatedSizeOfAtom);
	static hkpConstraintInstance* getConstraint(const hkpEntity* entityA, const hkpEntity* a2);
	// Unlinks the first modifier of the given type from the constraint's chain, takes its solver info out of the
	// owner's and the constraint's, and frees it.
	static void removeAndDeleteModifier(hkpConstraintInstance* instance, hkpConstraintOwner& constraintOwner,
		hkpConstraintAtom::AtomType type);
};
