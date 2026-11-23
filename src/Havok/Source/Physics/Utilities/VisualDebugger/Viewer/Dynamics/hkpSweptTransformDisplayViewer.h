#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostSimulationListener.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldViewerBase.h"

#include <cstddef>
#include <stdint.h>

// The viewer that draws the swept transform (the motion's start and end poses over the step) of the entities in the
// worlds it watches.
class hkpSweptTransformDisplayViewer : public hkpWorldViewerBase,
                                       protected hkpEntityListener,
                                       protected hkpWorldPostSimulationListener {
public:
    // What the viewer keeps per world: the display objects it made for that world's entities.
    struct WorldToEntityData {
        hkpWorld* world;
        // The geometry ids (the SDK's hkUlong): each entity's collidable address.
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

    hkpSweptTransformDisplayViewer(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts);
    ~hkpSweptTransformDisplayViewer() override;

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

    void addWorld(hkpWorld* world);
    int32_t findWorld(hkpWorld* world);
    void removeWorld(int32_t i);
    void removeAllGeometries(int32_t worldIndex);

    // DeletingDestruct's free (hkReferencedObject's); declared here because the listener bases declare their own
    // and the lookup would be ambiguous.
    static void operator delete(void* apMemory) {
        deleteObject(apMemory);
    }
    // Makes the viewer for the contexts.
    static hkProcess* create(const hkArray<hkProcessContext*>& contexts);
    // Registers the viewer under its name with the process factory.
    static void registerViewer();
    static const char* getName() {
        return "Swept Transforms";
    }

    hkArray<WorldToEntityData*> m_worldEntities;

    // The viewer's process tag, given by the process factory when the viewer is registered.
    static inline int32_t m_tag;
};

extern int32_t HK_DEFAULT_SWEPT_TRANSFORM_COLOR_T1;
extern int32_t HK_DEFAULT_SWEPT_TRANSFORM_COLOR_T0;
