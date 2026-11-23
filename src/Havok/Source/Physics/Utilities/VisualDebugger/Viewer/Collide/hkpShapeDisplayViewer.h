#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Visualize/hkProcessContext.h"
#include "Havok/Source/Common/Visualize/hkProcessFactory.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostSimulationListener.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldViewerBase.h"

#include <cstddef>
#include <stdint.h>

class hkDebugDisplayHandler;
class hkpEntity;
class hkpShape;

const hkpShape* getUserShape(char noStreams, uint8_t* overriddenWithNone, hkpEntity* entity);

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

    // The context a viewer's creator passes to set the viewer's options, found by its type name
    // "ShapeDisplayViewerOptions".
    class ShapeDisplayViewerOptions : public hkReferencedObject, public hkProcessContext {
    public:
        // The SDK's hkReferencedObject allocator; hkProcessContext declares an operator delete too, so the lookup
        // needs the class's own.
        static void operator delete(void* apMemory) {
            hkReferencedObject::deleteObject(apMemory);
        }

        hkBool m_enableShapeTransformUpdate;
    };

    hkpShapeDisplayViewer(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts);
    ~hkpShapeDisplayViewer() override;

    int32_t getProcessTag() override {
        return m_tag;
    }
    void init() override;
    // Declared with the display handler overload first: the compiler lays overloads out in reverse order of
    // declaration, so the vtable holds the hkpWorld one before the display handler one.
    virtual void synchronizeTransforms(hkDebugDisplayHandler* displayHandler, hkpWorld* world);
    virtual void synchronizeTransforms(hkpWorld* world);
    void worldAddedCallback(hkpWorld* world) override;
    void worldRemovedCallback(hkpWorld* world) override;
    void entityAddedCallback(hkpEntity* entity) override;
    void entityRemovedCallback(hkpEntity* entity) override;
    void postSimulationCallback(hkpWorld* world) override;
    // The SDK's inactiveEntityMovedCallback is virtual through hkpWorldPostSimulationListener.
    void inactiveEntityMovedCallback(hkpEntity* entity) override;

    void setTimeForDisplay(float time) {
        m_timeForDisplay = time;
    }
    void setDisplayBodyCreateEnabled(bool on);
    bool getDisplayBodyCreateEnabled() const {
        return m_enableDisplayCreation.m_bool != 0;
    }
    void setInstancingEnabled(bool on);
    bool getInstancingEnabled() const {
        return m_enableInstancing.m_bool != 0;
    }
    void setAutoColorMode(bool on);
    bool getAutoColorMode() const {
        return m_enableAutoColor.m_bool != 0;
    }
    void setAutoColorModeOverride(bool on) {
        m_enableAutoColorOverride.m_bool = on;
    }
    bool getAutoColorModeOverride() const {
        return m_enableAutoColorOverride.m_bool != 0;
    }
    void setDisplayBodyCachingEnabled(bool on);
    bool getDisplayBodyCachingEnabled() const {
        return m_enableDisplayCaching.m_bool != 0;
    }
    // Empties m_cachedShapes.
    void clearCache();
    void setAutoGeometryCreation(bool on);
    bool getAutoGeometryCreation() const {
        return m_autoGeometryCreation.m_bool != 0;
    }
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
        return "Shapes";
    }
    // The id the entity's display object goes by: its debug display id property when it has one, else its
    // collidable's address.
    static hkUlong getId(hkpEntity* entity);
    // Several bases declare one; the deleting destructor frees through hkReferencedObject's.
    static void operator delete(void* apMemory) {
        hkReferencedObject::deleteObject(apMemory);
    }

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
