#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpActionListener.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostSimulationListener.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldViewerBase.h"

class hkpVehicleInstance;

char* getHkVehicleInstanceName();

// The viewer that draws the vehicles of the worlds it watches (the vehicle actions it hears being added and removed).
class hkpVehicleViewer : public hkpWorldViewerBase, public hkpActionListener, public hkpWorldPostSimulationListener {
public:
	hkpVehicleViewer(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts);
	~hkpVehicleViewer() override;

	int32_t getProcessTag() override { return m_tag; }
	void init() override;
	void worldAddedCallback(hkpWorld* w) override;
	void worldRemovedCallback(hkpWorld* w) override;
	void postSimulationCallback(hkpWorld* world) override;
	void actionAddedCallback(hkpAction* action) override;
	void actionRemovedCallback(hkpAction* action) override;

	// DeletingDestruct's free (hkReferencedObject's); declared here because the listener bases declare their own
	// and the lookup would be ambiguous.
	static void operator delete(void* apMemory) { deleteObject(apMemory); }
	// Havok SDK signature (hkpUtilities.lib, tied by reloc).
	static hkProcess* create(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts);
	// Registers the viewer under its name with the process factory.
	static void registerViewer();

	hkArray<hkpVehicleInstance*> m_vehicles;

	// The viewer's process tag, given by the process factory when the viewer is registered.
	static inline int32_t m_tag;
};
