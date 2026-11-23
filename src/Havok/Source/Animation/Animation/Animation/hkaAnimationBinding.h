#pragma once

#include "Havok/Source/Animation/Animation/Animation/hkaAnimation.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"

#include <stdint.h>

// Maps animation tracks to a skeleton's bones and float slots.
class hkaAnimationBinding : public hkReferencedObject {
public:
    enum BlendHint : int32_t { NORMAL = 0, ADDITIVE = 1 };

    hkaAnimationBinding(hkFinishLoadedObjectFlag aFlag)
      : hkReferencedObject(aFlag), m_originalSkeletonName(aFlag), m_animation(aFlag),
        m_transformTrackToBoneIndices(aFlag), m_floatTrackToFloatSlotIndices(aFlag) {}

    ~hkaAnimationBinding() override;

    int16_t findTrackIndexFromBoneIndex(int16_t boneIndex) const;
    int16_t findTrackIndexFromSlotIndex(int16_t slotIndex) const;

    static int16_t findTrackIndexFromBoneIndex(int16_t boneIndex, int32_t numTracks, const int16_t* trackToBoneIndices);
    static const hkClass& staticClass();
    static void operator delete(void* apMemory) {
        hkReferencedObject::deleteObject(apMemory);
    }

    hkStringPtr m_originalSkeletonName;
    hkRefPtr<hkaAnimation> m_animation;
    hkArray<int16_t> m_transformTrackToBoneIndices;
    hkArray<int16_t> m_floatTrackToFloatSlotIndices;
    hkEnum<BlendHint, int8_t> m_blendHint;
};

extern hkClass hkaAnimationBindingClass;
extern hkTypeInfo hkaAnimationBindingTypeInfo;
