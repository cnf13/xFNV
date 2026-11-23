#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostSimulationListener.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldViewerBase.h"

#include <stdint.h>

class hkDisplayAABB;

// The viewer that draws the bounds the midphase works with. The SDK keeps them in a member it names
// m_broadPhaseDisplayGeometries, as hkpBroadphaseViewer does.
class hkpMidphaseViewer : public hkpWorldViewerBase, protected hkpWorldPostSimulationListener {
public:
    hkpMidphaseViewer(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts);
    ~hkpMidphaseViewer() override;

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
        return "Midphase";
    }
    // Several bases declare one; the deleting destructor frees through hkReferencedObject's.
    static void operator delete(void* apMemory) {
        hkReferencedObject::deleteObject(apMemory);
    }

    hkArray<hkDisplayAABB> m_broadPhaseDisplayGeometries;

    // The viewer's process tag, given by the process factory when the viewer is registered.
    static inline int32_t m_tag;
};
