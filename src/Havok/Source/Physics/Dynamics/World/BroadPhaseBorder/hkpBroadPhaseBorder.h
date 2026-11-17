#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomOverlapListener.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldDeletionListener.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostSimulationListener.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldCinfo.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkpEntity;
class hkpPhantom;
class hkpWorld;

// The PDB's hkpBroadPhaseBorder_EntityCmpLess, a file static of hkpBroadPhaseBorder.cpp: orders entities by uid (cdecl,
// 0xD0B5D0). PDB: returns bool; the exe sets the whole of eax to 0 or 1 (sbb, neg).
int32_t hkpBroadPhaseBorder_EntityCmpLess(const hkpEntity* a, const hkpEntity* b);

// Not named in the PDB or IDA; name chosen here. hkpBroadPhaseBorder_EntityCmpLess at the sort's bool type (the
// PDB's), the native side of hkpBroadPhaseBorder_EntityCmpLess_original.
inline bool hkpBroadPhaseBorder_EntityCmpLessBool(const hkpEntity* a, const hkpEntity* b) {
	return hkpBroadPhaseBorder_EntityCmpLess(a, b) != 0;
}

// Six phantoms around the world's broad phase that catch the entities leaving it and then assert, fix, remove or
// ignore them as the world's border behaviour says. The SDK inherits hkpWorldPostSimulationListener privately; it is
// protected here so that FNVD_REPLACES_VIRTUAL_VIA can register postSimulationCallback through it.
class hkpBroadPhaseBorder : public hkReferencedObject,
							protected hkpWorldDeletionListener,
							protected hkpPhantomOverlapListener,
							protected hkpWorldPostSimulationListener {
public:
	hkpBroadPhaseBorder(hkpWorld* world, hkpWorldCinfo::BroadPhaseBorderBehaviour type,
		hkBool postponeAndSortCallbacks);
	~hkpBroadPhaseBorder() override;

	virtual void maxPositionExceededCallback(hkpEntity* entity);
	virtual void deactivate();
	void collidableAddedCallback(const hkpCollidableAddedEvent& event) override;
	void collidableRemovedCallback(const hkpCollidableRemovedEvent& event) override;
	void postSimulationCallback(hkpWorld* world) override;
	void worldDeletedCallback(hkpWorld* world) override;

	// DeletingDestruct's free (hkReferencedObject::deleteObject's block free); the listener bases declare their own.
	static void operator delete(void* apMemory);

	hkpWorld* m_world;
	hkpPhantom* m_phantoms[6];
	hkpWorldCinfo::BroadPhaseBorderBehaviour m_type;
	hkBool m_postponeAndSortCallbacks;
	hkArray<hkpEntity*> m_entitiesExitingBroadPhase;
};

inline constexpr fnvd::Vtable hkpBroadPhaseBorder_vftable{};
inline constexpr fnvd::Vtable hkpBroadPhaseBorder_vftable_hkpWorldDeletionListener{};
inline constexpr fnvd::Vtable hkpBroadPhaseBorder_vftable_hkpPhantomOverlapListener{};
inline constexpr fnvd::Vtable hkpBroadPhaseBorder_vftable_hkpWorldPostSimulationListener{};
