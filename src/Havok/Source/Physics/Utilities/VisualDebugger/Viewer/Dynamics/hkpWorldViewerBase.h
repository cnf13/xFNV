#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Visualize/hkProcess.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/hkpPhysicsContext.h"

#include <stdint.h>

// The base of the viewers that watch one physics context: the visual debugger process and the listener of the context's
// worlds. The process's getConsumableCommands, consumeCommand and step use hkProcess's own bodies, so they are not
// overridden here.
class hkpWorldViewerBase : public hkReferencedObject, public hkProcess, protected hkpPhysicsContextWorldListener {
public:
    hkpWorldViewerBase(const hkArray<hkProcessContext*, hkContainerHeapAllocator>& contexts);
    ~hkpWorldViewerBase() override;

    void worldRemovedCallback(hkpWorld* world) override {}
    void worldAddedCallback(hkpWorld* world) override {}

    // DeletingDestruct's free (hkReferencedObject's); declared here because hkProcess and the listener base
    // declare their own and the lookup would be ambiguous.
    static void operator delete(void* apMemory) {
        deleteObject(apMemory);
    }

    hkpPhysicsContext* m_context;
};

extern int32_t s_movableObjectColor;
extern int32_t s_fixedObjectColor;
extern int32_t s_convexRadiusMovableObjectColor;
extern int32_t s_convexRadiusFixedObjectColor;
