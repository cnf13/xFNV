#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Collide/hkpInactiveContactPointViewer.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Visualize/hkProcessContext.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Collide/hkpContactPointViewer.h"

hkpInactiveContactPointViewer::hkpInactiveContactPointViewer(
    const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts
)
  : hkpContactPointViewer(contexts, -0xFF8000) {}

// The base does the work.
hkpInactiveContactPointViewer::~hkpInactiveContactPointViewer() {}

const hkArray<hkpSimulationIsland*, hkContainerHeapAllocator>&
hkpInactiveContactPointViewer::getIslands(hkpWorld* world) const {
    return world->m_inactiveSimulationIslands;
}

hkProcess* hkpInactiveContactPointViewer::create(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts) {
    hkpInactiveContactPointViewer* pViewer = new hkpInactiveContactPointViewer(contexts);
    return &static_cast<hkProcess&>(static_cast<hkpWorldViewerBase&>(static_cast<hkpContactPointViewer&>(*pViewer)));
}

void hkpInactiveContactPointViewer::registerViewer() {
    hkpInactiveContactPointViewer::s_tag = hkProcessFactory::s_instance->registerProcess(
        "Contact Points (Inactive)",
        &hkpInactiveContactPointViewer::create
    );
}
