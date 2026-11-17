#pragma once

#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/Gjk/hkpGskBaseAgent.h"
#include "Havok/Source/Physics/Collide/Agent/ConvexAgent/Gjk/hkpGskManifold.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"

class hkpCdBody;
class hkpCollisionInput;
class hkpContactMgr;
class hkpProcessCollisionInput;
class hkpProcessCollisionOutput;

int32_t calcEdgeEdgeClosestPoints(const uint8_t* simplex, hkVector4* out, const hkVector4* vertices);

// The GSK agent between two convex shapes that keeps a contact manifold between frames.
class hkpGskfAgent : public hkpGskBaseAgent {
public:
    hkpGskfAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB, hkpContactMgr* mgr);
    ~hkpGskfAgent() override;

    void processCollision(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpProcessCollisionInput& input,
        hkpProcessCollisionOutput& result
    ) override;
    void cleanup(hkpConstraintOwner& constraintOwner) override;
    void removePoint(uint16_t idToRemove) override;
    void commitPotential(uint16_t idToCommit) override;
    void createZombie(uint16_t idTobecomeZombie) override;

    // The GSK collision at the step's end, without the TIM early out.
    void processCollisionNoTim(
        const hkpCdBody& bodyA,
        const hkpCdBody& bodyB,
        const hkpProcessCollisionInput& input,
        hkpProcessCollisionOutput& output
    );

    // Havok SDK signature (hkpCollide.lib, tied by bytes,reloc). A new agent with a manifold where there is a
    // contact manager, else the base agent.
    static hkpCollisionAgent*
    createGskfAgent(const hkpCdBody& bodyA, const hkpCdBody& bodyB, const hkpCollisionInput& input, hkpContactMgr* mgr);
    // Havok SDK signature (hkpCollide.lib, tied by reloc).
    static void initAgentFunc(hkpCollisionDispatcher::AgentFuncs& af);
    // Havok SDK signature (hkpCollide.lib, tied by bytes).
    static void registerAgent(hkpCollisionDispatcher* dispatcher);

    hkpGskManifold m_manifold;
    uint8_t padding74[12];
};
