#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Collide/hkpContactPointViewer.h"

// The contact point viewer of the inactive simulation islands.
class hkpInactiveContactPointViewer : public hkpContactPointViewer {
public:
    hkpInactiveContactPointViewer(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts);
    ~hkpInactiveContactPointViewer() override;

    int32_t getProcessTag() override {
        return s_tag;
    }
    const hkArray<hkpSimulationIsland*>& getIslands(hkpWorld* world) const override;

    // Makes the viewer for the contexts.
    static hkProcess* create(const hkArray<hkProcessContext*>& contexts);
    // Registers the viewer under its name with the process factory.
    static void registerViewer();
    static const char* getName() {
        return "Contact Points (Inactive)";
    }

    // The viewer's process tag, given by the process factory when the viewer is registered.
    static inline int32_t s_tag;
};
