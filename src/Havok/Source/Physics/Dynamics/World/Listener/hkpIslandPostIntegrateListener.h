#pragma once

class hkpSimulationIsland;
struct hkStepInfo;

// The interface an island reports its post-integrate step to. It holds no state.
// hkpWorldCallbackUtil::fireIslandPostIntegrateCallback calls postIntegrateCallback with the island and the step.
class hkpIslandPostIntegrateListener {
public:
    virtual ~hkpIslandPostIntegrateListener();

    virtual void postIntegrateCallback(hkpSimulationIsland* island, const hkStepInfo& stepInfo) = 0;
};
