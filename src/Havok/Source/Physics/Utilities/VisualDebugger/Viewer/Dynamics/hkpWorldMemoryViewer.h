#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/Stream/hkStreamStatisticsCollector.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldViewerBase.h"

#include <stdint.h>

class hkVtableClassRegistry;

// The viewer that collects the memory the worlds it watches use, with a stream statistics collector, and sends it to
// the client each step. The SDK's constructor takes the vtable class registry first.
class hkpWorldMemoryViewer : public hkpWorldViewerBase {
public:
	hkpWorldMemoryViewer(hkVtableClassRegistry* vtlbReg,
		const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts);
	~hkpWorldMemoryViewer() override;

	int32_t getProcessTag() override { return m_tag; }
	void init() override;
	void step(float frameTimeInMs) override;

	// Makes the viewer for the contexts.
	static hkProcess* create(const hkArray<hkProcessContext*>& contexts);
	// Registers the viewer under its name with the process factory.
	static void registerViewer();
	static const char* getName() { return "World Memory"; }

	hkStreamStatisticsCollector m_collector;

	// The viewer's process tag, given by the process factory when the viewer is registered.
	static inline int32_t m_tag;
};
