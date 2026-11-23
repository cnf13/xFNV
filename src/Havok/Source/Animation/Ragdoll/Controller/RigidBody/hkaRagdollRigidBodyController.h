#pragma once

#include "Havok/Source/Animation/Ragdoll/Controller/RigidBody/hkaKeyFrameHierarchyUtility.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"

#include <cstddef>
#include <stdint.h>

class hkaRagdollInstance;

// Drives a ragdoll's rigid bodies to a pose through key-frame control data (see hkaKeyFrameHierarchyUtility).
class hkaRagdollRigidBodyController {
public:
    hkaRagdollRigidBodyController(hkaRagdollInstance* ragdoll);
    ~hkaRagdollRigidBodyController();

    // The class allocator: blocks of the Havok heap.
    static void* operator new(size_t aSize) noexcept {
        return hkMemoryRouter::getInstance().m_heap->blockAlloc(static_cast<int32_t>(aSize));
    }
    static void operator delete(void* apMemory, size_t aSize) {
        hkMemoryRouter::getInstance().m_heap->blockFree(apMemory, static_cast<int32_t>(aSize));
    }

    // Re-seeds the key-frame work data from the rigid bodies' current state.
    void reinitialize();
    const hkaRagdollInstance* getRagdollInstance() const;
    void setBoneWeights(float* boneWeights);
    void driveToPose(
        float deltaTime,
        const hkQsTransform* poseLocal,
        const hkQsTransform& worldFromModel,
        hkaKeyFrameHierarchyUtility::Output* stressOut
    );

    hkArray<hkaKeyFrameHierarchyUtility::ControlData> m_controlDataPalette;
    hkArray<int32_t> m_bodyIndexToPaletteIndex;
    hkaKeyFrameHierarchyUtility::BodyData m_bodyData;
    hkArray<hkaKeyFrameHierarchyUtility::WorkElem> m_internalData;
    hkaRagdollInstance* m_ragdollInstance;
    hkArray<int16_t> m_rbParentIndices;
};
