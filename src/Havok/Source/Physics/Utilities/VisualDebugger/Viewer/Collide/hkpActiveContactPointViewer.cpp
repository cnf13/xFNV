#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Collide/hkpActiveContactPointViewer.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Visualize/hkProcessContext.h"
#include "Havok/Source/Common/Visualize/hkProcessFactory.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Collide/hkpContactPointViewer.h"

hkpActiveContactPointViewer::hkpActiveContactPointViewer(
    const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts
)
  : hkpContactPointViewer(contexts, -0xFFFF01) {}

// The base does the work.
hkpActiveContactPointViewer::~hkpActiveContactPointViewer() {}

const hkArray<hkpSimulationIsland*, hkContainerHeapAllocator>&
hkpActiveContactPointViewer::getIslands(hkpWorld* world) const {
    return world->m_activeSimulationIslands;
}

hkProcess* hkpActiveContactPointViewer::create(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts) {
    hkpActiveContactPointViewer* pViewer = new hkpActiveContactPointViewer(contexts);
    return &static_cast<hkProcess&>(static_cast<hkpWorldViewerBase&>(static_cast<hkpContactPointViewer&>(*pViewer)));
}

void hkpActiveContactPointViewer::registerViewer() {
    s_tag = hkProcessFactory::s_instance->registerProcess(
        "Contact Points (Active)",
        &hkpActiveContactPointViewer::create
    );
}
