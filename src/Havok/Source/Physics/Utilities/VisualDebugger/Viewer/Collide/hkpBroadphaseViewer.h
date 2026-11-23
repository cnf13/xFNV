#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostSimulationListener.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldViewerBase.h"

class hkDisplayAABB;

// The viewer that draws the bounds of the broadphase of the worlds it watches, refreshed after each simulation step.
class hkpBroadphaseViewer : public hkpWorldViewerBase, protected hkpWorldPostSimulationListener {
public:
    hkpBroadphaseViewer(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts);
    ~hkpBroadphaseViewer() override;

    int32_t getProcessTag() override {
        return m_tag;
    }
    void postSimulationCallback(hkpWorld* world) override;
    void worldAddedCallback(hkpWorld* world) override;
    void worldRemovedCallback(hkpWorld* world) override;

    // Makes the viewer for the contexts.
    static hkProcess* create(const hkArray<hkProcessContext*>& contexts);
    // Registers the viewer under its name with the process factory.
    static void registerViewer();
    static const char* getName() {
        return "Broadphase";
    }
    // Several bases declare one; the deleting destructor frees through hkReferencedObject's.
    static void operator delete(void* apMemory) {
        hkReferencedObject::deleteObject(apMemory);
    }

    hkArray<hkDisplayAABB> m_broadPhaseDisplayGeometries;

    // The viewer's process tag, given by the process factory when the viewer is registered.
    static inline int32_t m_tag;
};
