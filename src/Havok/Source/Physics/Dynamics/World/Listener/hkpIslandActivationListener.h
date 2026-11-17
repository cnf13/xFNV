#pragma once

class hkpSimulationIsland;

// The interface the world reports a simulation island going to sleep or waking up to. It holds no state.
class hkpIslandActivationListener {
public:
    hkpIslandActivationListener();
    virtual ~hkpIslandActivationListener();

    virtual void islandActivatedCallback(hkpSimulationIsland* apIsland) = 0;
    virtual void islandDeactivatedCallback(hkpSimulationIsland* apIsland) = 0;

    // DeletingDestruct's free (ourFree).
    static void operator delete(void* apMemory);
};
