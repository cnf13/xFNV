#pragma once

#include <cstdint>

class hkpConstraintData;
class hkTransform;
class hkVector4;

// Reads and writes the pivots of a constraint's data whatever its type, looking through breakable and malleable
// wrappers.
class hkpConstraintDataUtils {
public:
	// Sets the pivot of body aiIndex.
	static void setPivot(hkpConstraintData* apData, const hkVector4& arPivot, int32_t aiIndex);
	// The frame of body aiIndex: the identity, then the
	// constraint's local transform or pivot when its type has one.
	static void getPivotTransform(const hkpConstraintData* apData, int32_t aiIndex, hkTransform& arPivot);
	// Sets the frame of body aiIndex: the constraint's
	// local transform, or its pivot from the frame's translation, when its type has one.
	static void setPivotTransform(hkpConstraintData* apData, const hkTransform& arPivot, int32_t aiIndex);
};
