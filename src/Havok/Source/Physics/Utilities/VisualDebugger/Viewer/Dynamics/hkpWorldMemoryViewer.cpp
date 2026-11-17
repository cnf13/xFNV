#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldMemoryViewer.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/Stream/hkStreamStatisticsCollector.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/Registry/hkVtableClassRegistry.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Visualize/hkProcess.h"
#include "Havok/Source/Common/Visualize/hkProcessContext.h"
#include "Havok/Source/Common/Visualize/hkProcessFactory.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldViewerBase.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/hkpPhysicsContext.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"

// The collector gets 500000 bytes of buffer, as the exe passes.
hkpWorldMemoryViewer::hkpWorldMemoryViewer(hkVtableClassRegistry* vtlbReg,
	const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts)
	: hkpWorldViewerBase(contexts), m_collector(vtlbReg, 500000) {}

hkpWorldMemoryViewer::~hkpWorldMemoryViewer() {}

void hkpWorldMemoryViewer::init() {
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

void hkpWorldMemoryViewer::step(float frameTimeInMs) {
	hkStreamStatisticsCollector* p_m_collector = &m_collector;
	m_collector.reset();
	p_m_collector->beginSnapshot(0);
	hkpPhysicsContext* m_context = this->m_context;
	for (int32_t i = 0; i < m_context->m_worlds.m_size; ++i) {
		p_m_collector->addReferencedObject("World", m_context->m_worlds.m_data[i], 0);
		m_context = this->m_context;
	}
	p_m_collector->endSnapshot();
	if (p_m_collector->m_stream.m_size > 0) {
		if (p_m_collector->m_stream.begin()) {
			int32_t m_size = m_collector.m_stream.m_size;
			if (p_m_collector->m_stream.m_size <= 0) {
				m_displayHandler->sendMemStatsDump(0, m_collector.m_stream.m_size);
			} else {
				m_displayHandler->sendMemStatsDump(p_m_collector->m_stream.begin(), m_collector.m_stream.m_size);
			}
		}
	}
}

hkProcess* hkpWorldMemoryViewer::create(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts) {
	hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
	hkpWorldMemoryViewer* v2 =
		static_cast<hkpWorldMemoryViewer*>(Instance->m_heap->blockAlloc(sizeof(hkpWorldMemoryViewer)));
	v2->m_memSizeAndFlags = sizeof(hkpWorldMemoryViewer);
	hkpWorldMemoryViewer* v3 = ::new (v2) hkpWorldMemoryViewer(nullptr, contexts);
	if (v3) {
		return &static_cast<hkProcess&>(static_cast<hkpWorldViewerBase&>(*v3));
	} else {
		return 0;
	}
}

void hkpWorldMemoryViewer::registerViewer() {
	m_tag = hkProcessFactory::s_instance->registerProcess("World Memory", &hkpWorldMemoryViewer::create);
}
