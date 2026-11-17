#pragma once

#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpIslandActivationListener.h"
#include "Reconstruction.hpp"

class hkpEntity;
class hkpSimulationIsland;
class hkpWorld;

// The game's listener for simulation islands activating and deactivating, which also listens to entities being
// added.
class FOIslandActivationListener : public hkpIslandActivationListener, public hkpEntityListener {
public:
	FOIslandActivationListener();
	~FOIslandActivationListener() override;

	void islandActivatedCallback(hkpSimulationIsland* apIsland) override;
	void islandDeactivatedCallback(hkpSimulationIsland* apIsland) override;
	// hkpEntityListener's vtable at +0x4.
	void entityAddedCallback(hkpEntity* apEntity) override;

	// Puts the listener on apWorld as an island activation and an entity listener.
	void AddToWorld(hkpWorld* apWorld);
	// Takes the listener off apWorld.
	void RemoveFromWorld(hkpWorld* apWorld);

	// DeletingDestruct's free (ourFree); both bases declare one.
	static void operator delete(void* apMemory);
};
inline constexpr fnvd::Vtable FOIslandActivationListener_vftable_hkpEntityListener{};
inline constexpr fnvd::Vtable FOIslandActivationListener_vftable_hkpIslandActivationListener{};
