#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostSimulationListener.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldViewerBase.h"

#include <stdint.h>

class hkDisplayBox;
class hkpRigidBody;

// The viewer that draws the inertia tensor of the rigid bodies in the worlds it watches as boxes.
class hkpRigidBodyInertiaViewer : public hkpWorldViewerBase,
								  protected hkpEntityListener,
								  protected hkpWorldPostSimulationListener {
public:
	hkpRigidBodyInertiaViewer(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts);
	~hkpRigidBodyInertiaViewer() override;

	int32_t getProcessTag() override { return m_tag; }
	void init() override;
	void entityAddedCallback(hkpEntity* entity) override;
	void entityRemovedCallback(hkpEntity* entity) override;
	void postSimulationCallback(hkpWorld* world) override;
	void worldAddedCallback(hkpWorld* world) override;
	void worldRemovedCallback(hkpWorld* world) override;

	void addWorld(hkpWorld* world);
	void removeWorld(hkpWorld* world);

	// Makes the viewer for the contexts.
	static hkProcess* create(const hkArray<hkProcessContext*>& contexts);
	// Registers the viewer under its name with the process factory.
	static void registerViewer();
	static const char* getName() { return "Inertia Tensors"; }
	// Several bases declare one; the deleting destructor frees through hkReferencedObject's.
	static void operator delete(void* apMemory) { hkReferencedObject::deleteObject(apMemory); }

	hkArray<hkpRigidBody*> m_entitiesCreated;
	hkArray<hkDisplayBox> m_displayBoxes;

	// The viewer's process tag, given by the process factory when the viewer is registered.
	static inline int32_t m_tag;
};
