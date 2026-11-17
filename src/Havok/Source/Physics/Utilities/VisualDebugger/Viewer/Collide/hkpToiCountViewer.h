#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpContactListener.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostSimulationListener.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldViewerBase.h"

#include <stdint.h>
#include <xmmintrin.h>

class hkDebugDisplayHandler;
class hkpRigidBody;

// The viewer that counts, per rigid body, the time-of-impact events of the contacts it hears about and displays the
// count at the body.
class hkpToiCountViewer : public hkpWorldViewerBase,
						  public hkpContactListener,
						  public hkpWorldPostSimulationListener,
						  protected hkpEntityListener {
public:
	using MapType = hkPointerMap<hkpRigidBody*, uint32_t>;

	// Where a body's count is drawn, from the body's centre (values from the Havok SDK).
	enum DisplayPosition : int32_t {
		POSITIVE_X = 0x0,
		POSITIVE_Y = 0x1,
		POSITIVE_Z = 0x2,
		NEGATIVE_X = 0x3,
		NEGATIVE_Y = 0x4,
		NEGATIVE_Z = 0x5,
	};

	hkpToiCountViewer(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts);
	~hkpToiCountViewer() override;

	void worldRemovedCallback(hkpWorld* w) override;
	void worldAddedCallback(hkpWorld* w) override;
	int32_t getProcessTag() override;
	void init() override;
	void contactPointCallback(const hkpContactPointEvent& event) override;
	void postSimulationCallback(hkpWorld* world) override;
	void entityRemovedCallback(hkpEntity* entity) override;
	void entityDeletedCallback(hkpEntity* entity) override;

	void reset();
	// Havok SDK signature (hkpUtilities.lib, tied by bytes,reloc).
	void displayCountForBody(hkpRigidBody* body, hkpToiCountViewer::DisplayPosition displayPosition);

	// Makes the viewer for the contexts.
	static hkProcess* create(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts);
	// Registers the viewer under its name with the process factory.
	static void registerViewer();
	// Several bases declare one; the deleting destructor frees through hkReferencedObject's.
	static void operator delete(void* apMemory) { hkReferencedObject::deleteObject(apMemory); }
	// Havok SDK signature (hkpUtilities.lib, tied by reloc).
	static void displayTextAtBody(hkDebugDisplayHandler* handler, int32_t tag, hkpRigidBody* body,
		hkpToiCountViewer::DisplayPosition pos, const char* text, int32_t color, float extraDistance, hkBool drawLine);
	// Returns the axis of gravity's largest component, +3 when it is positive.
	static hkpToiCountViewer::DisplayPosition getDisplayPositionFromGravityVector(const hkVector4& gravity);

	MapType m_toiCounts;

	static inline int32_t s_tag;
};
