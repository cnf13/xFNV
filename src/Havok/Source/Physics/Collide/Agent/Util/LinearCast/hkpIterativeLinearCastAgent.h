#pragma once

#include "Havok/Source/Physics/Collide/Agent/hkpCollisionAgent.h"

// The base of the convex agents that do linear casts by iterating closest point queries. It adds no data to
// hkpCollisionAgent.
class hkpIterativeLinearCastAgent : public hkpCollisionAgent {
public:
    hkpIterativeLinearCastAgent(hkpContactMgr* contactMgr) : hkpCollisionAgent(contactMgr) {}

    // The linear cast of an agent instance: the closest points come from the agent's own getClosestPoints, then the
    // fraction along the path is refined.
    void linearCast(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpLinearCastCollisionInput& input,
        hkpCdPointCollector& collector,
        hkpCdPointCollector* startCollector
    ) override;

    // The same cast as the dispatcher's agent2 table calls it, with the closest-points function taken from that table.
    static void staticLinearCast(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpLinearCastCollisionInput& input,
        hkpCdPointCollector& collector,
        hkpCdPointCollector* startCollector
    );
};
