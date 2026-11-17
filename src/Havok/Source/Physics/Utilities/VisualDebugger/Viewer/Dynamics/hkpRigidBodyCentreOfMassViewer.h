#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostSimulationListener.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldViewerBase.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkpRigidBody;

// The viewer that draws the centre of mass of the rigid bodies in the worlds it watches.
class hkpRigidBodyCentreOfMassViewer : public hkpWorldViewerBase,
									   protected hkpEntityListener,
									   protected hkpWorldPostSimulationListener {
public:
	hkpRigidBodyCentreOfMassViewer(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts);
	~hkpRigidBodyCentreOfMassViewer() override;

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
	static const char* getName() { return "Center of Mass"; }
	// Several bases declare one; the deleting destructor frees through hkReferencedObject's.
	static void operator delete(void* apMemory) { hkReferencedObject::deleteObject(apMemory); }

	hkArray<hkpRigidBody*> m_entitiesCreated;

	// The viewer's process tag, given by the process factory when the viewer is registered.
	static inline int32_t m_tag;
	// The length of the axis lines drawn from each centre of mass (PDB: static float m_scale; 1.0 in the exe's data).
	static inline float m_scale = 1.0f;
};
inline constexpr fnvd::Vtable hkpRigidBodyCentreOfMassViewer_vftable{};
inline constexpr fnvd::Vtable hkpRigidBodyCentreOfMassViewer_vftable_hkProcess{};
inline constexpr fnvd::Vtable hkpRigidBodyCentreOfMassViewer_vftable_hkpPhysicsContextWorldListener{};
inline constexpr fnvd::Vtable hkpRigidBodyCentreOfMassViewer_vftable_hkpEntityListener{};
inline constexpr fnvd::Vtable hkpRigidBodyCentreOfMassViewer_vftable_hkpWorldPostSimulationListener{};
