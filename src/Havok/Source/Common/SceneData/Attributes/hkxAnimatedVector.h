#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttribute.h"

// Stores sampled vector values for an animated scene attribute and how to interpret them. m_hint needs no freeing.
class hkxAnimatedVector : public hkReferencedObject {
public:
    hkxAnimatedVector() = default;
    hkxAnimatedVector(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag), m_vectors(aFlag) {}
    ~hkxAnimatedVector() override;

    // The reflection class hkxAnimatedVectorClass.
    static const hkClass& staticClass();

    hkArray<hkVector4, hkContainerHeapAllocator> m_vectors;
    hkEnum<hkxAttribute::Hint, uint8_t> m_hint;
};

extern hkClass hkxAnimatedVectorClass;

extern hkTypeInfo hkxAnimatedVectorTypeInfo;
