#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"

// Stores integer attribute samples and their timestamps.
class hkxSparselyAnimatedInt : public hkReferencedObject {
public:
    // The SDK's finish constructor: a loaded object keeps the fields the load wrote.
    hkxSparselyAnimatedInt(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag), m_ints(aFlag), m_times(aFlag) {}
    ~hkxSparselyAnimatedInt() override;

    // Havok SDK signature (hkSceneData.lib, tied by target).
    static const hkClass& staticClass();

    hkArray<int32_t, hkContainerHeapAllocator> m_ints;
    hkArray<float, hkContainerHeapAllocator> m_times;
};
