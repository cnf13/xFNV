#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldSnapshotViewer.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/System/Io/Writer/Array/hkArrayStreamWriter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Util/hkStructureLayout.h"
#include "Havok/Source/Common/Visualize/hkProcess.h"
#include "Havok/Source/Common/Visualize/hkProcessContext.h"
#include "Havok/Source/Common/Visualize/hkProcessHandler.h"
#include "Havok/Source/Common/Visualize/Process/hkStatisticsProcess.h"
#include "Havok/Source/Common/Visualize/Serialize/hkDisplaySerializeOStream.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Utilities/Serialize/hkpHavokSnapshot.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/hkpPhysicsContext.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldViewerBase.h"

hkpWorldSnapshotViewer::hkpWorldSnapshotViewer(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts,
	bool convertToPC)
	: hkpWorldViewerBase(contexts), m_convert(convertToPC) {}

hkpWorldSnapshotViewer::~hkpWorldSnapshotViewer() {}

void hkpWorldSnapshotViewer::init() {
	hkpPhysicsContext* m_context = this->m_context;
	if (m_context) {
		int32_t v3 = 0;
		if (m_context->m_worlds.m_size > 0) {
			hkpPhysicsContextWorldListener* v4 =
				&static_cast<hkpPhysicsContextWorldListener&>(static_cast<hkpWorldViewerBase&>(*this));
			do {
				worldAddedCallback(m_context->m_worlds.m_data[v3]);
				m_context = this->m_context;
				++v3;
			} while (v3 < m_context->m_worlds.m_size);
		}
	}
}

// Sends each world's snapshot to the client: the snapshot is saved into a borrowed array, then written behind the
// HK_SNAPSHOT command, the target layout rules and its size; then the process deletes itself through the handler.
void hkpWorldSnapshotViewer::step(float frameTimeInMs) {
	if (m_outStream) {
		// EXE-INLINED none: hkArray<char>::hkArray
		FNVD_INLINED_LOCAL(hkArray<char>, storage, std::nullopt, std::nullopt);
		storage.m_data = nullptr;
		storage.m_size = 0;
		storage.m_capacityAndFlags = hkArrayBase<char>::DONT_DEALLOCATE_FLAG;
		storage.reserve(100 * 1024);
		{
			hkArrayStreamWriter writer(&storage, hkArrayStreamWriter::ARRAY_BORROW);
			const hkStructureLayout::LayoutRules* rules =
				m_convert ? &hkStructureLayout::MsvcWin32LayoutRules : &hkStructureLayout::HostLayoutRules;
			for (int32_t i = 0; i < m_context->m_worlds.m_size; ++i) {
				storage.setSize(0);
				hkBool result;
				hkpHavokSnapshot::save(&result, m_context->m_worlds.m_data[i], &writer, true, rules, 0);
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
		// EXE-INLINED none: hkArray<char>::~hkArray
		hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
		const int32_t capacityAndFlags = storage.m_capacityAndFlags;
		storage.m_size = 0;
		if (capacityAndFlags >= 0)
			heap.bufFree(storage.m_data, capacityAndFlags & hkArrayBase<char>::CAPACITY_MASK);
	}
	if (m_processHandler) {
		m_processHandler->deleteProcess(hkpWorldSnapshotViewer::getProcessTag());
	}
}

hkProcess* hkpWorldSnapshotViewer::createNative(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts) {
	hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
	hkpWorldSnapshotViewer* v2 =
		static_cast<hkpWorldSnapshotViewer*>(Instance->m_heap->blockAlloc(sizeof(hkpWorldSnapshotViewer)));
	v2->m_memSizeAndFlags = sizeof(hkpWorldSnapshotViewer);
	hkpWorldSnapshotViewer* v3 = ::new (v2) hkpWorldSnapshotViewer(contexts, 0);
	if (v3) {
		return &static_cast<hkProcess&>(static_cast<hkpWorldViewerBase&>(*v3));
	} else {
		return 0;
	}
}

hkProcess* hkpWorldSnapshotViewer::createPC(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts) {
	hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
	hkpWorldSnapshotViewer* v2 =
		static_cast<hkpWorldSnapshotViewer*>(Instance->m_heap->blockAlloc(sizeof(hkpWorldSnapshotViewer)));
	v2->m_memSizeAndFlags = sizeof(hkpWorldSnapshotViewer);
	hkpWorldSnapshotViewer* v3 = ::new (v2) hkpWorldSnapshotViewer(contexts, 1);
	if (v3) {
		return &static_cast<hkProcess&>(static_cast<hkpWorldViewerBase&>(*v3));
	} else {
		return 0;
	}
}

void hkpWorldSnapshotViewer::registerViewer() {
	m_tagNative = hkProcessFactory::s_instance->registerProcess("* Grab World Snapshot Native",
		&hkpWorldSnapshotViewer::createNative);
	m_tagPC =
		hkProcessFactory::s_instance->registerProcess("* Grab World Snapshot PC", &hkpWorldSnapshotViewer::createPC);
}
