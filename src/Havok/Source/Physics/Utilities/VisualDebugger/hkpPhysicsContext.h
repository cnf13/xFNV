#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Visualize/hkProcessContext.h"
#include "Havok/Source/Common/Visualize/hkProcessFactory.h"
#include "Havok/Source/Common/Visualize/hkVisualDebugger.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpActionListener.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintListener.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomListener.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldDeletionListener.h"
#include "Reconstruction.hpp"

class hkpAction;
class hkpConstraintInstance;
class hkpEntity;
class hkpPhantom;
class hkpWorld;
class hkpWorldCinfo;

// The interface the viewers implement to hear the physics context gaining or losing a world. It holds no state.
class hkpPhysicsContextWorldListener {
public:
	virtual ~hkpPhysicsContextWorldListener();

	virtual void worldAddedCallback(hkpWorld* newWorld) = 0;
	virtual void worldRemovedCallback(hkpWorld* newWorld) = 0;

	// DeletingDestruct's free (ourFree).
	static void operator delete(void* apMemory);
};

// The visual debugger's view of physics worlds. Its code stays with the exe: src only declares the calls the game
// makes into it. The listener bases sit at 0x58 (world deletion), 0x5C (entity), 0x60 (phantom), 0x64 (constraint)
// and 0x68 (action), after hkProcessContext at 8. The exe enters each override of a secondary base's virtual with
// that subobject as `this`, so the forwarders pass it.
class hkpPhysicsContext : public hkReferencedObject,
						  public hkProcessContext,
						  public hkpWorldDeletionListener,
						  public hkpEntityListener,
						  public hkpPhantomListener,
						  public hkpConstraintListener,
						  public hkpActionListener {
public:
	hkpPhysicsContext();
	~hkpPhysicsContext() override;

	// The context's type name, HK_PHYSICS_CONTEXT_TYPE_STRING (hkProcessContext slot 1; the SDK's inline; the exe's
	// out-of-line copy 0xCD2890 is registered in hkpPhysicsContext.cpp).
	const char* getType() override { return "Physics"; }
	// Not reconstructed yet (hkProcessContext slot 2; thiscall, ret 4, 0xCD24D0).
	void setOwner(hkVisualDebugger* vdb) override;
	// Not reconstructed yet (hkpWorldDeletionListener slot 1; thiscall, ret 4, 0xCD2460).
	void worldDeletedCallback(hkpWorld* world) override;
	// Not reconstructed yet (hkpEntityListener slot 1; thiscall, ret 4, 0xCD1F30).
	void entityAddedCallback(hkpEntity* entity) override;

	// With an owner, drops the entity from the debugger (hkpEntityListener slot 2). The exe enters this override with
	// the hkpEntityListener subobject at +0x5C as `this`, so m_owner sits 0x50 bytes before it.
	void entityRemovedCallback(hkpEntity* entity) override {
		if (m_owner) {
			m_owner->removeTrackedObject(entity);
		}
	}

	// Not reconstructed yet (hkpPhantomListener slot 1; thiscall, ret 4, 0xCD1F70).
	void phantomAddedCallback(hkpPhantom* phantom) override;

	// With an owner, drops the phantom from the debugger (hkpPhantomListener slot 2). The exe enters this override
	// with the hkpPhantomListener subobject at +0x60 as `this`, so m_owner sits 0x54 bytes before it.
	void phantomRemovedCallback(hkpPhantom* phantom) override {
		if (m_owner) {
			m_owner->removeTrackedObject(phantom);
		}
	}

	// Not reconstructed yet (hkpConstraintListener slot 1; thiscall, ret 4, 0xCD1FB0).
	void constraintAddedCallback(hkpConstraintInstance* constraint) override;
	// Not reconstructed yet (hkpConstraintListener slot 2; thiscall, ret 4, 0xCD1FF0).
	void constraintRemovedCallback(hkpConstraintInstance* constraint) override;
	// Not reconstructed yet (hkpActionListener slot 1; thiscall, ret 4, 0xCD2020).
	void actionAddedCallback(hkpAction* action) override;

	// With an owner, drops the action from the debugger (hkpActionListener slot 2). The exe enters this override with
	// the hkpActionListener subobject at +0x68 as `this`, so m_owner sits 0x5C bytes before it.
	void actionRemovedCallback(hkpAction* action) override {
		if (m_owner) {
			m_owner->removeTrackedObject(action);
		}
	}

	// Adds apWorld for inspection.
	void addForInspection(hkpWorld* w);
	// Havok SDK.
	void addWorld(hkpWorld* newWorld);
	// Havok SDK: the index of world in m_worlds (data +0x6C, size +0x70), or -1.
	int32_t findWorld(hkpWorld* world);
	// Havok SDK signature (hkpUtilities.lib, tied by bytes,reloc).
	void removeFromInspection(hkpWorld* w);
	// Havok SDK signature (hkpUtilities.lib, tied by bytes,reloc).
	void removeWorld(hkpWorld* oldWorld);
	void addWorldAddedListener(hkpPhysicsContextWorldListener* cb);
	// The SDK's removeWorldAddedListener: finds cb in m_addListeners (data +0x84, size +0x88)
	// and removes it, moving the last one into its place. Its caller is
	// hkpWorldViewerBase's destructor path.
	void removeWorldAddedListener(hkpPhysicsContextWorldListener* cb);

	// DeletingDestruct's free (hkReferencedObject's); declared here because the other bases declare their own and
	// the lookup would be ambiguous.
	static void operator delete(void* apMemory) { deleteObject(apMemory); }
	// Registers the physics viewers with the visual debugger.
	static void registerAllPhysicsProcesses();

	// The worlds under inspection, their construction infos and the listeners told when a world is added; the
	// constructor clears all three (0xCD25D0) and the exe allocates the context at 0x90 bytes (0xC6A640).
	hkArray<hkpWorld*> m_worlds;
	hkArray<hkpWorldCinfo> m_worldCinfos;
	hkArray<hkpPhysicsContextWorldListener*> m_addListeners;
};

extern hkClass hkpActionClass;
inline constexpr fnvd::Vtable hkpPhysicsContextWorldListener_vftable{};
inline constexpr fnvd::Vtable hkpPhysicsContext_vftable_hkpActionListener{};
inline constexpr fnvd::Vtable hkpPhysicsContext_vftable_hkpConstraintListener{};
inline constexpr fnvd::Vtable hkpPhysicsContext_vftable_hkpPhantomListener{};
inline constexpr fnvd::Vtable hkpPhysicsContext_vftable_hkpEntityListener{};
inline constexpr fnvd::Vtable hkpPhysicsContext_vftable_hkpWorldDeletionListener{};
inline constexpr fnvd::Vtable hkpPhysicsContext_vftable_hkProcessContext{};
inline constexpr fnvd::Vtable hkpPhysicsContext_vftable{};
