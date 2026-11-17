#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostSimulationListener.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldViewerBase.h"

#include <stdint.h>

class hkDisplayAABB;

// The viewer that draws the bounds of the active and the inactive simulation islands of the worlds it watches.
class hkpSimulationIslandViewer : public hkpWorldViewerBase, protected hkpWorldPostSimulationListener {
public:
    hkpSimulationIslandViewer(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts);
    ~hkpSimulationIslandViewer() override;

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
        return "Simulation Islands";
    }
    // DeletingDestruct's free (hkReferencedObject's); declared here because the listener base declares its own and
    // the lookup would be ambiguous.
    static void operator delete(void* apMemory) {
        deleteObject(apMemory);
    }

    hkBool m_showActiveIslands;
    hkBool m_showInactiveIslands;
    hkArray<hkDisplayAABB> m_inactiveIslandDisplayGeometries;
    hkArray<hkDisplayAABB> m_activeIslandDisplayGeometries;

    // The viewer's process tag, given by the process factory when the viewer is registered.
    static inline int32_t m_tag;
};
