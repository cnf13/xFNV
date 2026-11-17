#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"

// A contact point an agent produced: the point and the id the contact manager gave it.
struct alignas(16) hkpProcessCdPoint {
	hkContactPoint m_contact;
	hkPadSpu<uint32_t> m_contactPointId;
};

int32_t contactPointIsValid(const hkpProcessCdPoint* point, float a2, const float* a3, float a4,
	const hkContactPoint* reference);
