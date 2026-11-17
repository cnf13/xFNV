#pragma once

#include "Havok/Source/Physics/Collide/Agent/Query/hkpCdBodyPairCollector.h"
#include "Havok/Source/Physics/Collide/Query/Collector/BodyPairCollector/hkpRootCdBodyPair.h"
#include "Reconstruction.hpp"

class hkpCdBody;

// Keeps the first body pair a query finds, as root collidables and shape keys, and stops the query (addCdBodyPair,
// 0xD219E0, sets m_earlyOut). The exe has no vtable for this class: nothing refers to addCdBodyPair, which the map
// names, and it has no RTTI.
class hkpFirstCdBodyPairCollector : public hkpCdBodyPairCollector {
public:
	~hkpFirstCdBodyPairCollector() override;

protected:
	void addCdBodyPair(const hkpCdBody& bodyA, const hkpCdBody& bodyB) override;

public:
	hkpRootCdBodyPair m_cdBodyPair;
};
