#pragma once

#include "Reconstruction.hpp"

// A box-box manifold point: the two features that made it and its contact point id (Havok SDK
// Physics/Collide/BoxBox/hkpBoxBoxContactPoint.h; 4 bytes, the stride 0xD40180 walks the manifold's points with).
class hkpFeatureContactPoint {
public:
	uint8_t m_featureIdA;
	uint8_t m_featureIdB;
	// hkContactPointId (an unsigned short).
	uint16_t m_contactPointId;
};
