#pragma once

class hkpEntity;
class hkpWorld;

// The interface the world calls after each simulation step, and when an inactive entity is moved. It holds no state.
class hkpWorldPostSimulationListener {
public:
	virtual ~hkpWorldPostSimulationListener();

	virtual void postSimulationCallback(hkpWorld* world) = 0;
	virtual void inactiveEntityMovedCallback(hkpEntity* entity) {}

	// DeletingDestruct's free (ourFree).
	static void operator delete(void* apMemory);
};
