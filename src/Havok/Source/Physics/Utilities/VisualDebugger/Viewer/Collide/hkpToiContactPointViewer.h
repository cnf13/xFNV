#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpContactListener.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldViewerBase.h"

#include <stdint.h>

class hkDebugDisplayHandler;

// hkpToiContactPointViewer.cpp's file-local arrow helper; its one caller is
// hkpToiContactPointViewer::contactPointCallback.
void hkToiContactPointViewer_displayArrow(hkDebugDisplayHandler* handler, const hkVector4& from, int32_t color,
	int32_t id, int32_t tag, const hkVector4& dir);

// The viewer that draws the contact points the time-of-impact solver reports.
class hkpToiContactPointViewer : public hkpWorldViewerBase, public hkpContactListener {
public:
	hkpToiContactPointViewer(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts);
	~hkpToiContactPointViewer() override;

	int32_t getProcessTag() override;
	void init() override;
	void contactPointCallback(const hkpContactPointEvent& event) override;
	void worldAddedCallback(hkpWorld* w) override;
	void worldRemovedCallback(hkpWorld* w) override;

	// Makes the viewer for the contexts.
	static hkProcess* create(const hkArray<hkProcessContext*>& contexts);
	// Registers the viewer under its name with the process factory.
	static void registerViewer();
	static const char* getName() { return "Contact Points (TOI)"; }
	// Several bases declare one; the deleting destructor frees through hkReferencedObject's.
	static void operator delete(void* apMemory) { hkReferencedObject::deleteObject(apMemory); }

	// The viewer's process tag, given by the process factory when the viewer is registered.
	static inline int32_t s_tag;
};
