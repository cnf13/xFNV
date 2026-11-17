#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Reconstruction.hpp"

struct hkStepInfo;
class hkpAction;
class hkpCollisionEvent;
struct hkpConstraintBrokenEvent;
class hkpConstraintInstance;
struct hkpConstraintRepairedEvent;
class hkpContactImpulseLimitBreachedListenerInfo;
struct hkpContactPointAddedEvent;
class hkpContactPointEvent;
struct hkpContactPointRemovedEvent;
struct hkpContactProcessEvent;
class hkpEntity;
class hkpPhantom;
class hkpSimulationIsland;
class hkpWorld;

// Removes the null entries from one of the world's listener arrays, keeping the order. One
// exe copy serves every listener type (the PDB's file-static worldCallbackUtil_cleanupNullPointers, over
// hkArray<void*, hkContainerHeapAllocator>&); it takes the array in esi (usercall, ret 0, 0xD00370).
void worldCallbackUtil_cleanupNullPointers(hkArray<void*, hkContainerHeapAllocator>& cleanupArray);

// The notifications the world runs after it changed: each tells every listener the world holds. The listeners
// themselves stay with the exe.
class hkpWorldCallbackUtil {
public:
	// Tells the world's contact listeners that a contact point was added; clears the event's
	// m_callbackFiredFrom first.
	static void fireContactPointAdded(hkpWorld* world, hkpContactPointAddedEvent& event);
	// Tells the world's contact listeners about a contact point.
	static void fireContactPointCallback(hkpWorld* world, hkpContactPointEvent& event);
	// Tells the world's entity listeners that apEntity was added.
	static void fireEntityAdded(hkpWorld* world, hkpEntity* entity);
	// Tells the world's entity listeners that apEntity was removed.
	static void fireEntityRemoved(hkpWorld* world, hkpEntity* entity);
	// Tells the world's entity listeners that apEntity's shape was replaced.
	static void fireEntityShapeSet(hkpWorld* world, hkpEntity* entity);
	// Tells the world's phantom listeners that apPhantom was added.
	static void firePhantomAdded(hkpWorld* world, hkpPhantom* Phantom);
	// Tells the world's phantom listeners that apPhantom was removed.
	static void firePhantomRemoved(hkpWorld* world, hkpPhantom* Phantom);
	// Tells the world's constraint listeners that apConstraint was added.
	static void fireConstraintAdded(hkpWorld* world, hkpConstraintInstance* constraint);
	// Tells the world's constraint listeners that apConstraint was removed.
	static void fireConstraintRemoved(hkpWorld* world, hkpConstraintInstance* constraint);
	// Not reconstructed yet. Tells the world's contact listeners that a collision's constraint was added (cdecl,
	// ret 0, 0xD00D40; the timer around it is "TtNewCollCb").
	static void fireContactConstraintAddedCallback(hkpWorld* world, hkpCollisionEvent& event);
	// Not reconstructed yet. Tells the world's contact listeners that a collision's constraint was removed (cdecl,
	// ret 0, 0xD00DE0; the timer around it is "TtFinCollCb").
	static void fireContactConstraintRemovedCallback(hkpWorld* world, hkpCollisionEvent& event);
	// Tells the world's post-simulation listeners that the step is over.
	static void firePostSimulationCallback(hkpWorld* world);
	// Not reconstructed yet. Tells the world's post-collide listeners that the collide step over stepInfo is over
	// (cdecl, ret 0, 0xD014F0). Not named in IDA; the PDB's firePostCollideCallback, chosen by the "TtPostCollideCB"
	// timer around both calls and the PDB's order of the fire functions [INFERENCE].
	static void firePostCollideCallback(hkpWorld* world, const hkStepInfo& info);
	// Tells the world's island post-collide listeners that island collided over stepInfo;
	// the parameter names are the SDK's.
	static void fireIslandPostCollideCallback(hkpWorld* world, hkpSimulationIsland* island, const hkStepInfo& info);
	// Tells the world's island post-integrate listeners that island integrated over stepInfo.
	static void fireIslandPostIntegrateCallback(hkpWorld* world, hkpSimulationIsland* island, const hkStepInfo& info);
	// Tells the world's post-integrate listeners that the integrate step over stepInfo is over.
	static void firePostIntegrateCallback(hkpWorld* world, const hkStepInfo& info);
	// Tells the world's constraint listeners that a constraint broke.
	static void fireConstraintBroken(hkpWorld* world, const hkpConstraintBrokenEvent& event);
	// Tells the world's constraint listeners that a constraint was repaired.
	static void fireConstraintRepaired(hkpWorld* world, const hkpConstraintRepairedEvent& event);
	// Tells the world's contact listeners that a contact point was removed.
	static void fireContactPointRemoved(hkpWorld* world, hkpContactPointRemovedEvent& event);
	// Tells the world's impulse limit listeners that numBreachedContacts contacts exceeded
	// their maximum impulse.
	static void fireContactImpulseLimitBreached(hkpWorld* world,
		const hkpContactImpulseLimitBreachedListenerInfo* breachedContacts, int32_t numBreachedContacts);
	// Tells the world's contact listeners that a collision's contact points were processed.
	static void fireContactProcess(hkpWorld* world, hkpContactProcessEvent& event);
	// Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc,string).
	static void fireActionAdded(hkpWorld* world, hkpAction* action);
	// Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
	static void firePhantomShapeSet(hkpWorld* world, hkpPhantom* Phantom);
	// Inferred type: a1: hkpWorld* from 0xcb51c0 (parameter 0 of hkpWorldOperationUtil::internalActivateIsland), its
	// members used at 0xd00feb; a2: hkpSimulationIsland* from 0xcb51c0 (parameter 1 of
	// hkpWorldOperationUtil::internalActivateIsland), its members used at 0xd01102
	static void fireIslandActivated(hkpWorld* a1, hkpSimulationIsland* a2);
	// Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc,string).
	static void fireInactiveEntityMoved(hkpWorld* world, hkpEntity* entity);
	// Tells the world's entity listeners that entity's motion type changed.
	static void fireEntitySetMotionType(hkpWorld* world, hkpEntity* entity);
	static void fireIslandDeactivated(hkpWorld* world, hkpSimulationIsland* island);
	static void fireActionRemoved(hkpWorld* world, hkpAction* action);
	// Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc,string).
	static void fireConstraintViolated(hkpWorld* world, hkpConstraintInstance* constraint);
	// Havok SDK signature (hkpDynamics.lib, tied by bytes).
	static void fireWorldDeleted(hkpWorld* world);
	// Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
	static void fireWorldRemoveAll(hkpWorld* world);
};
