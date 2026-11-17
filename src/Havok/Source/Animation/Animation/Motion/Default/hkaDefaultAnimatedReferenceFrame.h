#pragma once

#include "Havok/Source/Animation/Animation/Motion/hkaAnimatedReferenceFrame.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"

class hkQsTransform;

// The reference frame stored as samples over the duration, in a frame given by its up and forward axes.
class hkaDefaultAnimatedReferenceFrame : public hkaAnimatedReferenceFrame {
public:
    // The options the reference frame is extracted from an animation with.
    struct MotionExtractionOptions {
        MotionExtractionOptions();

        const hkQsTransform* m_referenceFrameTransforms;
        int32_t m_numReferenceFrameTransforms;
        float m_referenceFrameDuration;
        int32_t m_numSamples;
        hkBool m_allowUpDown;
        hkBool m_allowFrontBack;
        hkBool m_allowRightLeft;
        hkBool m_allowTurning;
        hkVector4 m_up;
        hkVector4 m_forward;
    };

    // The SDK's finish constructor: a loaded frame keeps the fields the load wrote.
    hkaDefaultAnimatedReferenceFrame(hkFinishLoadedObjectFlag aFlag)
      : hkaAnimatedReferenceFrame(aFlag), m_referenceFrameSamples(aFlag) {}

    // The SDK's constructor: the frame sampled from options.
    explicit hkaDefaultAnimatedReferenceFrame(const hkaDefaultAnimatedReferenceFrame::MotionExtractionOptions& options);
    ~hkaDefaultAnimatedReferenceFrame() override;

    // The motion at time, from the reference frame samples.
    void getReferenceFrame(float time, hkQsTransform& motionOut) const override;
    void getDeltaReferenceFrame(float time, float nextTime, int32_t loops, hkQsTransform& output) const override;
    float getDuration() const override;

    static void operator delete(void* apMemory) {
        hkReferencedObject::deleteObject(apMemory);
    }
    // Havok SDK signature (hkaAnimation.lib, tied by target).
    static const hkClass& staticClass();
    // The motion's translation projected on the axes options allows, and
    // its turn about the up axis unwrapped against previous's.
    static void project(
        const hkaDefaultAnimatedReferenceFrame::MotionExtractionOptions& options,
        const hkQsTransform& motion,
        const hkVector4& previous,
        hkVector4& projectedOut
    );

    hkVector4 m_up;
    hkVector4 m_forward;
    float m_duration;
    hkArray<hkVector4> m_referenceFrameSamples;
};

extern hkClass hkaDefaultAnimatedReferenceFrameClass;

extern hkTypeInfo hkaDefaultAnimatedReferenceFrameTypeInfo;
