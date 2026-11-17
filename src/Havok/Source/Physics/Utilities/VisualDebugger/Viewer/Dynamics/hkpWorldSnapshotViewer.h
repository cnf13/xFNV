#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldViewerBase.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// The viewer that sends a snapshot of the worlds it watches to the client on request (the PC or the native format).
class hkpWorldSnapshotViewer : public hkpWorldViewerBase {
public:
	// Values from the SDK.
	enum Command : int32_t {
		HK_SNAPSHOT = 0xD0,
	};

	hkpWorldSnapshotViewer(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts, bool convertToPC);
	~hkpWorldSnapshotViewer() override;

	int32_t getProcessTag() override { return m_convert ? m_tagPC : m_tagNative; }
	void init() override;
	void step(float frameTimeInMs) override;

	// Makes the viewer for the contexts, converting for the native format.
	static hkProcess* createNative(const hkArray<hkProcessContext*>& contexts);
	// Makes the viewer for the contexts, converting for the PC format.
	static hkProcess* createPC(const hkArray<hkProcessContext*>& contexts);
	// Registers the viewers under their names with the process factory.
	static void registerViewer();
	static const char* getNameNative() { return "* Grab World Snapshot Native"; }
	static const char* getNamePC() { return "* Grab World Snapshot PC"; }

	bool m_convert;

	// The viewers' process tags (PC and native), given by the process factory when the viewers are registered.
	static inline int32_t m_tagPC;
	static inline int32_t m_tagNative;
};
inline constexpr fnvd::Vtable hkpWorldSnapshotViewer_vftable{};
inline constexpr fnvd::Vtable hkpWorldSnapshotViewer_vftable_hkProcess{};
inline constexpr fnvd::Vtable hkpWorldSnapshotViewer_vftable_hkpPhysicsContextWorldListener{};
