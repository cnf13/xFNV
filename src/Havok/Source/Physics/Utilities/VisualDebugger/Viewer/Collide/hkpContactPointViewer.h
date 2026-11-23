#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostSimulationListener.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldViewerBase.h"

#include <stdint.h>

class hkpSimulationIsland;

// The base of the viewers that draw the contact points of the simulation islands the derived class picks (the active
// or the inactive ones), after each simulation step.
class hkpContactPointViewer : public hkpWorldViewerBase, public hkpWorldPostSimulationListener {
public:
    hkpContactPointViewer(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts, int32_t color);
    ~hkpContactPointViewer() override;

    void init() override;
    void postSimulationCallback(hkpWorld* world) override;
    void worldAddedCallback(hkpWorld* w) override;
    void worldRemovedCallback(hkpWorld* w) override;
    // The islands whose contact points are drawn.
    virtual const hkArray<hkpSimulationIsland*>& getIslands(hkpWorld* world) const = 0;

    void drawAllContactPointsInIsland(const hkpSimulationIsland* island);

    // Several bases declare one; the deleting destructor frees through hkReferencedObject's.
    static void operator delete(void* apMemory) {
        hkReferencedObject::deleteObject(apMemory);
    }

    float m_limit;
    // The color the contact points are drawn in (const in the SDK).
    int32_t m_color;
};
