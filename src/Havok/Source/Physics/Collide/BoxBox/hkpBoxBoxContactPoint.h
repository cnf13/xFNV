#pragma once

#include <cstdint>

// A box-box manifold point: the two features that made it and its contact point id.
class hkpFeatureContactPoint {
public:
	uint8_t m_featureIdA;
	uint8_t m_featureIdB;
	// hkContactPointId (an unsigned short).
	uint16_t m_contactPointId;
};
