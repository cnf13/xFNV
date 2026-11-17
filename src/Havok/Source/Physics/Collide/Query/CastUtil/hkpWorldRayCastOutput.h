#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastOutput.h"
#include "Reconstruction.hpp"

class hkpCollidable;

// A ray cast's result for the whole world: the shape-level output plus the root collidable that was hit, null while
// nothing was.
// sizeof is not asserted: the base's 16-byte alignment makes it 0x60, which only the exe's allocations of the class
// could confirm.
struct hkpWorldRayCastOutput : hkpShapeRayCastOutput {
	hkpWorldRayCastOutput();
	hkpWorldRayCastOutput(const hkpWorldRayCastOutput& __that);

	// Copies the shape-level output and m_rootCollidable (defined in Projectile.cpp, as in the PDB).
	hkpWorldRayCastOutput& operator=(const hkpWorldRayCastOutput& __that);

	// Whether the cast hit anything: the SDK's inline body; the exe's out-of-line copy (0x5C53F0) is registered in
	// TESScriptFunctions.cpp, as in the PDB.
	hkBool* hasHit(hkBool* arResult) const {
		::new (arResult) hkBool(m_rootCollidable != nullptr);
		return arResult;
	}

	// Clears m_rootCollidable and resets the shape-level output: the SDK's inline body; the exe's out-of-line copy
	// (thiscall, ret 0, 0x62A2B0) is registered in Projectile.cpp.
	void reset() {
		m_rootCollidable = nullptr;
		hkpShapeRayCastOutput::reset_0();
	}

	hkpCollidable* m_rootCollidable;
};
