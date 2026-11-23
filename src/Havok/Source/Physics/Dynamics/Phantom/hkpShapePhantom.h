#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"

class hkpCdBodyPairCollector;
class hkpCdPointCollector;
class hkpCollisionInput;
class hkpLinearCastInput;

// A phantom with a shape and a motion state of its own: it can be moved, swept along a path to find what the shape
// hits, and asked which bodies it touches or penetrates.
class hkpShapePhantom : public hkpPhantom {
public:
    hkpShapePhantom(hkpShape* shape, hkTransform* transform);
    hkpShapePhantom(hkFinishLoadedObjectFlag flag) : hkpPhantom(flag) {}

    hkWorldOperation::Result setShape(const hkpShape* shape) override;
    hkMotionState* getMotionState() override;
    void calcAabb(hkAabb& aAabb) override;
    void deallocateInternalArrays() override;
    virtual void setPositionAndLinearCast(
        const hkVector4&,
        const hkpLinearCastInput&,
        hkpCdPointCollector&,
        hkpCdPointCollector*
    ) = 0;
    virtual void setTransformAndLinearCast(
        const hkTransform&,
        const hkpLinearCastInput&,
        hkpCdPointCollector&,
        hkpCdPointCollector*
    ) = 0;
    virtual void getClosestPoints(hkpCdPointCollector&, const hkpCollisionInput*) = 0;
    virtual void getPenetrations(hkpCdBodyPairCollector&, const hkpCollisionInput*) = 0;

    // Moves the phantom to aPosition; extraTolerance widens its AABB.
    void setPosition(const hkVector4& position, float extraTolerance);
    // Moves the phantom to transform.
    void setTransform(const hkTransform& transform);
    void firePhantomShapeSet();

    // m_motionState is 16-byte aligned, so the bytes after the base class are the compiler's alignment padding.
    alignas(16) hkMotionState m_motionState;
};

// The class's reflection record.
extern hkClass hkpShapePhantomClass;
