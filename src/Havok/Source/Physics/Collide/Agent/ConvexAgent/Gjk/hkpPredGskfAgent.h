#pragma once

#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/Gjk/hkpGskfAgent.h"

#include <stdint.h>

class hkpCdBody;
class hkpCollisionInput;
class hkpContactMgr;

// The predictive GSK agent between two convex shapes: hkpGskfAgent with its own processCollision.
class hkpPredGskfAgent : public hkpGskfAgent {
public:
    hkpPredGskfAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB, hkpContactMgr* mgr)
      : hkpGskfAgent(bodyA, bodyB, mgr) {}

    void processCollision(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpProcessCollisionInput& input,
        hkpProcessCollisionOutput& result
    ) override;

    // Registers the agent for a convex shape against a convex shape.
    static void registerAgent(hkpCollisionDispatcher* dispatcher);
    // Havok SDK signature (hkpCollide.lib, tied by bytes,reloc).
    static hkpCollisionAgent* createPredGskfAgent(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpCollisionInput& input,
        hkpContactMgr* mgr
    );
};
