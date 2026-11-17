#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Visualize/hkProcessContext.h"
#include "Havok/Source/Common/Visualize/hkProcessFactory.h"
#include "Havok/Source/Common/Visualize/hkVisualDebugger.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpActionListener.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintListener.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomListener.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldDeletionListener.h"

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

// The visual debugger's view of physics worlds. Only the calls the game makes into it are declared.
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

	// The context's type name, HK_PHYSICS_CONTEXT_TYPE_STRING.
	const char* getType() override { return "Physics"; }
	void setOwner(hkVisualDebugger* vdb) override;
	void worldDeletedCallback(hkpWorld* world) override;
	void entityAddedCallback(hkpEntity* entity) override;

	// With an owner, drops the entity from the debugger.
	void entityRemovedCallback(hkpEntity* entity) override {
		if (m_owner) {
			m_owner->removeTrackedObject(entity);
		}
	}

	void phantomAddedCallback(hkpPhantom* phantom) override;

	// With an owner, drops the phantom from the debugger.
	void phantomRemovedCallback(hkpPhantom* phantom) override {
		if (m_owner) {
			m_owner->removeTrackedObject(phantom);
		}
	}

	void constraintAddedCallback(hkpConstraintInstance* constraint) override;
	void constraintRemovedCallback(hkpConstraintInstance* constraint) override;
	void actionAddedCallback(hkpAction* action) override;

	// With an owner, drops the action from the debugger.
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
	// constructor clears all three.
	hkArray<hkpWorld*> m_worlds;
	hkArray<hkpWorldCinfo> m_worldCinfos;
	hkArray<hkpPhysicsContextWorldListener*> m_addListeners;
};

extern hkClass hkpActionClass;
