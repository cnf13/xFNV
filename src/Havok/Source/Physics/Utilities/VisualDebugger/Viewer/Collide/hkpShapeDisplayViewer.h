#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Visualize/hkProcessContext.h"
#include "Havok/Source/Common/Visualize/hkProcessFactory.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostSimulationListener.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldViewerBase.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkDebugDisplayHandler;
class hkpEntity;
class hkpShape;

// Inferred type: r_eax: hkpEntity* from 0xd65b06 (parameter 0 of hkpShapeDisplayViewer::entityAddedCallback), its
// members used at 0xd6531c. The result is the shape the entity's display properties (0x1235, 0x1135, 0x1234) name:
// entityRemovedCallback (0xD65640) uses it as the shape.
const hkpShape* getUserShape(char a1, uint8_t* arg4, hkpEntity* r_eax);

// The viewer that sends the shapes of the entities in the worlds it watches to the display handler and keeps their
// transforms up to date. It is the one viewer with a public interface the engine's own debug display uses.
class hkpShapeDisplayViewer : public hkpWorldViewerBase,
							  protected hkpEntityListener,
							  public hkpWorldPostSimulationListener {
public:
	// What the viewer keeps per world: the display objects it made for that world's entities.
	struct WorldToEntityData {
		hkpWorld* world;
		hkArray<hkUlong> entitiesCreated;
	};

	// The context a viewer's creator passes to set the viewer's options (the SDK's and the PDB's name), found by
	// its type name "ShapeDisplayViewerOptions" (the Construct 0xD657D0 reads its one field).
	class ShapeDisplayViewerOptions : public hkReferencedObject, public hkProcessContext {
	public:
		// The SDK's hkReferencedObject allocator; hkProcessContext declares an operator delete too, so the lookup
		// needs the class's own.
		static void operator delete(void* apMemory) { hkReferencedObject::deleteObject(apMemory); }

		hkBool m_enableShapeTransformUpdate;
	};

	hkpShapeDisplayViewer(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts);
	~hkpShapeDisplayViewer() override;

	// The SDK's inline (its static m_tag is hkpShapeDisplayViewer_m_tag); the exe's out-of-line copy 0xD661D0 is registered in
	// hkpShapeDisplayViewer.cpp.
	int32_t getProcessTag() override { return m_tag; }
	void init() override;
	// Declared in the SDK's order (the display handler overload first). The exe's vtable holds the hkpWorld one at
	// slot 3 and the display handler one at slot 4: the compiler lays overloads out in reverse order of declaration.
	virtual void synchronizeTransforms(hkDebugDisplayHandler* displayHandler, hkpWorld* world);
	virtual void synchronizeTransforms(hkpWorld* world);
	void worldAddedCallback(hkpWorld* world) override;
	void worldRemovedCallback(hkpWorld* world) override;
	void entityAddedCallback(hkpEntity* entity) override;
	void entityRemovedCallback(hkpEntity* entity) override;
	void postSimulationCallback(hkpWorld* world) override;
	// The SDK's inactiveEntityMovedCallback is virtual through hkpWorldPostSimulationListener.
	void inactiveEntityMovedCallback(hkpEntity* entity) override;

	void setTimeForDisplay(float time) { m_timeForDisplay = time; }
	void setDisplayBodyCreateEnabled(bool on);
	bool getDisplayBodyCreateEnabled() const { return m_enableDisplayCreation.m_bool != 0; }
	void setInstancingEnabled(bool on);
	bool getInstancingEnabled() const { return m_enableInstancing.m_bool != 0; }
	void setAutoColorMode(bool on);
	bool getAutoColorMode() const { return m_enableAutoColor.m_bool != 0; }
	void setAutoColorModeOverride(bool on) { m_enableAutoColorOverride.m_bool = on; }
	bool getAutoColorModeOverride() const { return m_enableAutoColorOverride.m_bool != 0; }
	void setDisplayBodyCachingEnabled(bool on);
	bool getDisplayBodyCachingEnabled() const { return m_enableDisplayCaching.m_bool != 0; }
	// Empties m_cachedShapes.
	void clearCache();
	void setAutoGeometryCreation(bool on);
	bool getAutoGeometryCreation() const { return m_autoGeometryCreation.m_bool != 0; }
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
	static const char* getName() { return "Shapes"; }
	// The id the entity's display object goes by: its debug display id property when it has one, else its
	// collidable's address.
	static hkUlong getId(hkpEntity* entity);
	// Several bases declare one; the deleting destructor frees through hkReferencedObject's.
	static void operator delete(void* apMemory) { hkReferencedObject::deleteObject(apMemory); }

	hkArray<WorldToEntityData*> m_worldEntities;
	// The shape's first geometry id (the SDK's hkUlong).
	hkPointerMap<const hkpShape*, hkUlong> m_instancedShapeToGeomID;
	hkPointerMap<const hkpShape*, uint32_t> m_instancedShapeToUsageCount;
	hkPointerMap<const hkpShape*, uint32_t> m_cachedShapes;
	hkBool m_enableShapeTransformUpdate;
	hkBool m_enableInstancing;
	hkBool m_enableDisplayCaching;
	hkBool m_enableDisplayCreation;
	hkBool m_autoGeometryCreation;
	hkBool m_enableAutoColor;
	hkBool m_enableAutoColorOverride;
	// hkTime
	float m_timeForDisplay;
	uint32_t m_fixedObjectColor;
	uint32_t m_movableObjectColor;

	static inline int32_t m_tag;
};
// Not named in the PDB or IDA; name chosen here. The exe registers it as a hkProcessFactory::CreationFunction.
inline constexpr fnvd::Vtable hkpShapeDisplayViewer_vftable{};
inline constexpr fnvd::Vtable hkpShapeDisplayViewer_vftable_hkProcess{};
inline constexpr fnvd::Vtable hkpShapeDisplayViewer_vftable_hkpPhysicsContextWorldListener{};
inline constexpr fnvd::Vtable hkpShapeDisplayViewer_vftable_hkpEntityListener{};
inline constexpr fnvd::Vtable hkpShapeDisplayViewer_vftable_hkpWorldPostSimulationListener{};
