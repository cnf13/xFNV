#pragma once

#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdBodyPairCollector.h"
#include "Havok/Source/Physics/Collide/Query/Collector/BodyPairCollector/hkpRootCdBodyPair.h"

class hkpCdBody;

// Keeps the first body pair a query finds, as root collidables and shape keys, and stops the query (addCdBodyPair sets
// m_earlyOut).
class hkpFirstCdBodyPairCollector : public hkpCdBodyPairCollector {
public:
	~hkpFirstCdBodyPairCollector() override;

protected:
	void addCdBodyPair(const hkpCdBody& bodyA, const hkpCdBody& bodyB) override;

public:
	hkpRootCdBodyPair m_cdBodyPair;
};
