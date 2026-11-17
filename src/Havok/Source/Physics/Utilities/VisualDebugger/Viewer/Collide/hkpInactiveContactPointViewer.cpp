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
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkpInactiveContactPointViewer* v2 = static_cast<hkpInactiveContactPointViewer*>(
        Instance->m_heap->blockAlloc(sizeof(hkpInactiveContactPointViewer))
    );
    v2->m_memSizeAndFlags = sizeof(hkpInactiveContactPointViewer);
    hkpInactiveContactPointViewer* v3 = ::new (v2) hkpInactiveContactPointViewer(contexts);
    if (v3) {
        return &static_cast<hkProcess&>(static_cast<hkpWorldViewerBase&>(static_cast<hkpContactPointViewer&>(*v3)));
    }

    return nullptr;
}

void hkpInactiveContactPointViewer::registerViewer() {
    hkpInactiveContactPointViewer::s_tag = hkProcessFactory::s_instance->registerProcess(
        "Contact Points (Inactive)", &hkpInactiveContactPointViewer::create
    );
}
