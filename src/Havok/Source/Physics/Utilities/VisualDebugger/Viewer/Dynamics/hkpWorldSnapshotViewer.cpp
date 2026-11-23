#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldSnapshotViewer.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/System/Io/Writer/Array/hkArrayStreamWriter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Util/hkStructureLayout.h"
#include "Havok/Source/Common/Visualize/Process/hkStatisticsProcess.h"
#include "Havok/Source/Common/Visualize/Serialize/hkDisplaySerializeOStream.h"
#include "Havok/Source/Common/Visualize/hkProcess.h"
#include "Havok/Source/Common/Visualize/hkProcessContext.h"
#include "Havok/Source/Common/Visualize/hkProcessHandler.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Utilities/Serialize/hkpHavokSnapshot.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldViewerBase.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/hkpPhysicsContext.h"

hkpWorldSnapshotViewer::hkpWorldSnapshotViewer(
    const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts,
    bool convertToPC
)
  : hkpWorldViewerBase(contexts), m_convert(convertToPC) {}

hkpWorldSnapshotViewer::~hkpWorldSnapshotViewer() {}

void hkpWorldSnapshotViewer::init() {
    hkpPhysicsContext* m_context = this->m_context;
    if (m_context) {
        int32_t v3 = 0;
        if (m_context->m_worlds.m_size > 0) {
            do {
                worldAddedCallback(m_context->m_worlds.m_data[v3]);
                m_context = this->m_context;
                ++v3;
            }
            while (v3 < m_context->m_worlds.m_size);
        }
    }
}

// Sends each world's snapshot to the client: the snapshot is saved into a borrowed array, then written behind the
// HK_SNAPSHOT command, the target layout rules and its size; then the process deletes itself through the handler.
void hkpWorldSnapshotViewer::step(float frameTimeInMs) {
    if (m_outStream) {
        hkArray<char> storage;
        storage.reserve(100 * 1024);
        {
            hkArrayStreamWriter writer(&storage, hkArrayStreamWriter::ARRAY_BORROW);
            const hkStructureLayout::LayoutRules* rules = m_convert ? &hkStructureLayout::MsvcWin32LayoutRules
                                                                    : &hkStructureLayout::HostLayoutRules;
            for (hkpWorld* world : m_context->m_worlds) {
                storage.setSize(0);
                hkpHavokSnapshot::save(world, &writer, true, rules, 0);
                const int32_t snapshotSize = storage.m_size;
                if (snapshotSize > 0) {
                    // The command byte, the four layout bytes and the 32-bit size, then the snapshot.
                    m_outStream->write32u(static_cast<uint32_t>(1 + 4 + 4 + snapshotSize));
                    m_outStream->write8u(HK_SNAPSHOT);
                    m_outStream->write8u(rules->m_bytesInPointer);
                    m_outStream->write8u(rules->m_littleEndian);
                    m_outStream->write8u(rules->m_reusePaddingOptimization);
                    m_outStream->write8u(rules->m_emptyBaseClassOptimization);
                    m_outStream->write32(snapshotSize);
                    m_outStream->writeRaw(storage.m_data, snapshotSize);
                }
            }
        }
    }
    if (m_processHandler) {
        m_processHandler->deleteProcess(hkpWorldSnapshotViewer::getProcessTag());
    }
}

hkProcess* hkpWorldSnapshotViewer::createNative(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts) {
    hkpWorldSnapshotViewer* pViewer = new hkpWorldSnapshotViewer(contexts, false);
    return &static_cast<hkProcess&>(static_cast<hkpWorldViewerBase&>(*pViewer));
}

hkProcess* hkpWorldSnapshotViewer::createPC(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts) {
    hkpWorldSnapshotViewer* pViewer = new hkpWorldSnapshotViewer(contexts, true);
    return &static_cast<hkProcess&>(static_cast<hkpWorldViewerBase&>(*pViewer));
}

void hkpWorldSnapshotViewer::registerViewer() {
    m_tagNative = hkProcessFactory::s_instance->registerProcess(
        "* Grab World Snapshot Native",
        &hkpWorldSnapshotViewer::createNative
    );
    m_tagPC = hkProcessFactory::s_instance->registerProcess(
        "* Grab World Snapshot PC",
        &hkpWorldSnapshotViewer::createPC
    );
}
