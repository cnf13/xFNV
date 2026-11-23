#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostSimulationListener.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldViewerBase.h"

#include <cstddef>

// The viewer that draws the convex radius of the shapes of the entities in the worlds it watches.
class hkpConvexRadiusViewer : public hkpWorldViewerBase,
                              public hkpEntityListener,
                              protected hkpWorldPostSimulationListener {
public:
    // What the viewer keeps per world: the display objects it made for that world's entities.
    struct WorldToEntityData {
        hkpWorld* world;
        // The geometry ids (the SDK's hkUlong): each entity's collidable address plus 3.
        hkArray<hkUlong> entitiesCreated;

        // The class allocator: blocks of the Havok heap.
        static void* operator new(size_t aSize) noexcept {
            return hkMemoryRouter::getInstance().m_heap->blockAlloc(static_cast<int32_t>(aSize));
        }
        static void operator delete(void* apMemory, size_t aSize) {
            hkMemoryRouter::getInstance().m_heap->blockFree(apMemory, static_cast<int32_t>(aSize));
        }
        static void* operator new(size_t, void* apMemory) {
            return apMemory;
        }
        static void operator delete(void*, void*) noexcept {}
    };

    hkpConvexRadiusViewer(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts);
    ~hkpConvexRadiusViewer() override;

    int32_t getProcessTag() override {
        return m_tag;
    }
    void init() override;
    void entityAddedCallback(hkpEntity* entity) override;
    void entityRemovedCallback(hkpEntity* entity) override;
    void postSimulationCallback(hkpWorld* world) override;
    void worldAddedCallback(hkpWorld* world) override;
    void worldRemovedCallback(hkpWorld* world) override;
    // The SDK's inactiveEntityMovedCallback is virtual through hkpWorldPostSimulationListener.
    void inactiveEntityMovedCallback(hkpEntity* entity) override;

    void setMovableObjectColor(uint32_t c) {
        m_movableObjectColor = c;
    }
    void setFixedObjectColor(uint32_t c) {
        m_fixedObjectColor = c;
    }
    void addWorld(hkpWorld* world);
    int32_t findWorld(hkpWorld* world);
    void removeWorld(int32_t i);
    void removeAllGeometries(int32_t worldIndex);

    // Makes the viewer for the contexts.
    static hkProcess* create(const hkArray<hkProcessContext*>& contexts);
    // Registers the viewer under its name with the process factory.
    static void registerViewer();
    static const char* getName() {
        return "Convex Radius";
    }
    // Several bases declare one; the deleting destructor frees through hkReferencedObject's.
    static void operator delete(void* apMemory) {
        hkReferencedObject::deleteObject(apMemory);
    }

    hkArray<WorldToEntityData*> m_worldEntities;
    uint32_t m_fixedObjectColor;
    uint32_t m_movableObjectColor;

    // The viewer's process tag, given by the process factory when the viewer is registered.
    static inline int32_t m_tag;
};
