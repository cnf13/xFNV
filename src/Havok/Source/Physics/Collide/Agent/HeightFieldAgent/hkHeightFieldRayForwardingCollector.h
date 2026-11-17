#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpRayHitCollector.h"

class hkpCdBody;
class hkpCdPointCollector;

// The height field agent's ray collector for a sphere swept over a height field: each ray hit becomes a contact point
// (the hit moved by m_currentRadius against the world hit normal) handed to m_collector, and the early-out fraction
// follows the collector's.
class hkHeightFieldRayForwardingCollector : public hkpRayHitCollector {
public:
	hkHeightFieldRayForwardingCollector(const hkVector4& path, const hkpCdBody& csBody, hkpCdPointCollector& collector)
		: m_path(path), m_csBody(csBody), m_collector(collector) {
		m_earlyOutHitFraction = 1.0f;
	}

	void addRayHit(const hkpCdBody& cdBody, const hkpShapeRayCastCollectorOutput& hitInfo) override;
	// Slot 1.
	~hkHeightFieldRayForwardingCollector() override;

	hkVector4 m_currentFrom;
	float m_currentRadius;
	hkVector4 m_path;
	const hkpCdBody& m_csBody;
	hkpCdPointCollector& m_collector;
};
