#pragma once

class hkpSimulationIsland;
struct hkStepInfo;

// The interface the world reports the end of a simulation island's collide step to; the world keeps the listeners in
// hkpWorld::m_islandPostCollideListeners and hkpWorldCallbackUtil::fireIslandPostCollideCallback calls
// postCollideCallback on each with the island and the step. It holds no state.
class hkpIslandPostCollideListener {
public:
	virtual ~hkpIslandPostCollideListener();

	virtual void postCollideCallback(hkpSimulationIsland* island, const hkStepInfo& stepInfo) = 0;
};
