#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

// The construction data every sampled heightfield shape takes: the scale of one sample and the grid resolution, and
// the height range. It has no vtable. The hkVector4 makes it 16-aligned (size 0x30): hkBSHeightFieldShapeCinfo's
// constructor and CalcMinMax put its first member at 0x30.
class hkpSampledHeightFieldBaseCinfo {
public:
	hkpSampledHeightFieldBaseCinfo();

	hkVector4 m_scale;
	int32_t m_xRes;
	int32_t m_zRes;
	float m_minHeight;
	float m_maxHeight;
	hkBool m_useProjectionBasedHeight;
};
