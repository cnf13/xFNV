#pragma once

#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastCollectorOutput.h"
#include "Reconstruction.hpp"

// A ray cast's result for a shape hierarchy: the collector's output plus the shape keys of the hierarchy levels
// down to the shape that was hit.
// sizeof is not asserted: the 16-byte alignment of the base makes it 0x50, which only the exe's allocations of the
// class could confirm.
struct hkpShapeRayCastOutput : hkpShapeRayCastCollectorOutput {
	// The depth of the shape hierarchy a ray cast can report.
	enum {
		MAX_HIERARCHY_DEPTH = 8
	};

	hkpShapeRayCastOutput();
	hkpShapeRayCastOutput(const hkpShapeRayCastOutput* apOther);

	void reset();
	// Copies the collector output, the shape keys and the key index (defined in Projectile.cpp, as in the PDB).
	hkpShapeRayCastOutput& operator=(const hkpShapeRayCastOutput& __that);
	void reset_0();

	uint32_t m_shapeKeys[8];
	int32_t m_shapeKeyIndex;
};

// What a ray bundle cast against a shape reports: one ray cast's result per ray of the bundle, in its order (as the
// SDK's header declares it here). Its methods (the constructors, reset, operator=) are not reconstructed yet.
struct hkpShapeRayBundleCastOutput {
	hkpShapeRayBundleCastOutput();

	int32_t reset();

	hkpShapeRayCastOutput m_outputs[4];
};
