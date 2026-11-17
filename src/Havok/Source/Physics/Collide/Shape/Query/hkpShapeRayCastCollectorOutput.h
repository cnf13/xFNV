#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

// What a ray cast against a shape reports to its collector: the surface normal at the hit, the fraction of the ray
// travelled and a shape-specific extra value.
struct hkpShapeRayCastCollectorOutput {
	hkpShapeRayCastCollectorOutput();
	hkpShapeRayCastCollectorOutput(const hkpShapeRayCastCollectorOutput* a1);

	// The copy assignment (thiscall, ret 4, 0x9C1B00). Its body is m_normal's copy through the
	// exe's out-of-line hkVector4 copy (0x4A3F10), then the other fields; src's implicit hkVector4 copy is inline,
	// so the body waits for the SWEEP-INLINE 0x4A3F10 conversion.
	hkpShapeRayCastCollectorOutput& operator=(const hkpShapeRayCastCollectorOutput& arOther);
	// Whether the ray hit (m_hitFraction below one); the hkBool comes back through a hidden
	// pointer.
	hkBool hasHit() const;
	hkpShapeRayCastCollectorOutput* hkpShapeRayCastCollectorOutput_();

	hkVector4 m_normal;
	float m_hitFraction;
	int32_t m_extraInfo;
	int32_t m_pad[2];
};
