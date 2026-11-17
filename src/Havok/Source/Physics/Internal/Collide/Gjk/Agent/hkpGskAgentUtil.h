#pragma once

class hkpAgent3Input;
struct hkpAgentEntry;
class hkpGskCache;
class hkpGskManifold;
class hkpProcessCollisionOutput;
class hkVector4;

// Runs GSK on the two convex bodies, welds the closest point, updates the manifold and writes the contact points to
// output. agentData is the agent's data, which starts with its hkpGskCache.
void hkGskAgentUtil_processCollisionNoTim(
    const hkpAgent3Input& input,
    hkpAgentEntry* entry,
    void* agentData,
    hkpGskCache& gskCache,
    hkpGskManifold& gskManifold,
    hkVector4& separatingNormal,
    hkpProcessCollisionOutput& output
);
