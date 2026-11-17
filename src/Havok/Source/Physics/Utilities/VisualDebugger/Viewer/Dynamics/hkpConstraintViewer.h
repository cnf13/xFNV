#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpWorldPostSimulationListener.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/hkpWorldViewerBase.h"

class hkDebugDisplayHandler;
class hkpConstraintInstance;
class hkpEntity;

// The viewer that draws the constraints of the worlds it watches, after each simulation step.
class hkpConstraintViewer : public hkpWorldViewerBase, protected hkpWorldPostSimulationListener {
public:
    hkpConstraintViewer(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts);
    ~hkpConstraintViewer() override;

    int32_t getProcessTag() override {
        return m_tag;
    }
    void postSimulationCallback(hkpWorld* world) override;
    void worldAddedCallback(hkpWorld* world) override;
    void worldRemovedCallback(hkpWorld* world) override;

    // Draws one constraint, seen from the master entity.
    // Havok SDK signature (hkpUtilities.lib, tied by bytes,reloc).
    void draw(hkpConstraintInstance* constraint, const hkpEntity* masterEntity, hkDebugDisplayHandler* displayHandler);

    // Makes the viewer for the contexts.
    static hkProcess* create(const hkArray<hkProcessContext*>& contexts);
    // Registers the viewer under its name with the process factory.
    static void registerViewer();
    static const char* getName() {
        return "Constraints";
    }
    // Several bases declare one; the deleting destructor frees through hkReferencedObject's.
    static void operator delete(void* apMemory) {
        hkReferencedObject::deleteObject(apMemory);
    }

    hkArray<hkpConstraintInstance*> m_constraints;

    // The viewer's process tag, given by the process factory when the viewer is registered.
    static inline int32_t m_tag;
    // The scale the constraint drawers draw at.
    static inline float m_scale = 1.0f;
};
