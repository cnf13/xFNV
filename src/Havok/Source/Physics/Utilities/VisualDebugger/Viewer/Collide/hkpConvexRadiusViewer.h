#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostSimulationListener.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldViewerBase.h"
#include "Reconstruction.hpp"

// The viewer that draws the convex radius of the shapes of the entities in the worlds it watches.
// The SDK derives the listeners protected; they are public here so the seam can register the entity listener's
// override through its subobject (FNVD_REPLACES_VIRTUAL_VIA). The layout is the same.
class hkpConvexRadiusViewer : public hkpWorldViewerBase,
							  public hkpEntityListener,
							  protected hkpWorldPostSimulationListener {
public:
	// What the viewer keeps per world: the display objects it made for that world's entities.
	struct WorldToEntityData {
		hkpWorld* world;
		// The geometry ids (the SDK's hkUlong): each entity's collidable address plus 3.
		hkArray<hkUlong> entitiesCreated;
	};

	hkpConvexRadiusViewer(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts);
	~hkpConvexRadiusViewer() override;

	int32_t getProcessTag() override { return m_tag; }
	void init() override;
	void entityAddedCallback(hkpEntity* entity) override;
	void entityRemovedCallback(hkpEntity* entity) override;
	void postSimulationCallback(hkpWorld* world) override;
	void worldAddedCallback(hkpWorld* world) override;
	void worldRemovedCallback(hkpWorld* world) override;
	// The SDK's inactiveEntityMovedCallback is virtual through hkpWorldPostSimulationListener.
	void inactiveEntityMovedCallback(hkpEntity* entity) override;

	void setMovableObjectColor(uint32_t c) { m_movableObjectColor = c; }
	void setFixedObjectColor(uint32_t c) { m_fixedObjectColor = c; }
	void addWorld(hkpWorld* world);
	int32_t findWorld(hkpWorld* world);
	void removeWorld(int32_t i);
	void removeAllGeometries(int32_t worldIndex);

	// Makes the viewer for the contexts.
	static hkProcess* create(const hkArray<hkProcessContext*>& contexts);
	// Registers the viewer under its name with the process factory.
	static void registerViewer();
	static const char* getName() { return "Convex Radius"; }
	// Several bases declare one; the deleting destructor frees through hkReferencedObject's.
	static void operator delete(void* apMemory) { hkReferencedObject::deleteObject(apMemory); }

	hkArray<WorldToEntityData*> m_worldEntities;
	uint32_t m_fixedObjectColor;
	uint32_t m_movableObjectColor;

	// The viewer's process tag, given by the process factory when the viewer is registered.
	static inline int32_t m_tag;
};
inline constexpr fnvd::Vtable hkpConvexRadiusViewer_vftable{};
inline constexpr fnvd::Vtable hkpConvexRadiusViewer_vftable_hkProcess{};
inline constexpr fnvd::Vtable hkpConvexRadiusViewer_vftable_hkpPhysicsContextWorldListener{};
inline constexpr fnvd::Vtable hkpConvexRadiusViewer_vftable_hkpEntityListener{};
inline constexpr fnvd::Vtable hkpConvexRadiusViewer_vftable_hkpWorldPostSimulationListener{};
