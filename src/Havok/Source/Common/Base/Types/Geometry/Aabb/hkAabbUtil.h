#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"

// Havok's bounding box helpers.
class hkAabbUtil {
public:
    // What sweeping a box along a motion needs: the motion state, the in-between transforms, the start transform
    // and the inverse of the end one. The SSE build's 16-aligned transforms make it 0x1A0 bytes.
    struct OffsetAabbInput {
        hkPadSpu<const hkMotionState*> m_motionState;
        hkTransform m_transforms[4];
        hkPadSpu<int32_t> m_numTransforms;
        hkTransform m_startTransform;
        hkTransform m_endTransformInv;
    };

    // The box around the numVertices points vectorArray points at; with no points, the inverted box from HK_REAL_MAX to
    // -HK_REAL_MAX. Returns &aabbOut.
    static hkAabb* calcAabb(const hkVector4* const* vectorArray, int32_t numVertices, hkAabb& aabbOut);
    // The box around the numVertices points vertices; with no points, the inverted box from HK_REAL_MAX to
    // -HK_REAL_MAX. Returns &aabbOut.
    static hkAabb* calcAabb(const hkVector4* vertices, int32_t numVertices, hkAabb& aabbOut);
    // The box around numVertices points striding bytes apart from vertexArray, w cleared; the first point is read
    // whatever numVertices is. Returns &aabbOut.
    static hkAabb* calcAabb(const float* vertexArray, int32_t numVertices, int32_t striding, hkAabb& aabbOut);
    // The box center +- halfExtents under localToWorld, grown by extraRadius.
    static void calcAabb(
        const hkTransform& localToWorld,
        const hkVector4& halfExtents,
        const hkVector4& center,
        float extraRadius,
        hkAabb& aabbOut
    );
    // The box the transform's translation +- its rotation applied to halfExtents, every axis grown by extraRadius.
    static void
    calcAabb(const hkTransform& localToWorld, const hkVector4& halfExtents, float extraRadius, hkAabb& aabbOut);
    // The squared distance from b to the box a, 0 when a contains b.
    static float distanceSquared(const hkAabb& a, const hkVector4& b);

    // aabbIn grown by the motion over the step: by the rotation's reach, clipped to the swept sphere around the end
    // centre of mass (never smaller than aabbIn), then stretched back along the linear path.
    static void sweepAabb(const hkMotionState* motionState, float tolerance, const hkAabb& aabbIn, hkAabb& aabbOut);
    // Fills input's transforms for the step from motionState: the motion state itself, the sampled transforms of the
    // step (each sample's translation w is the 1/cos factor of its angular step), their count, the transform at the
    // step's start and the inverse of the motion state's transform.
    static void initOffsetAabbInput(const hkMotionState* motionState, OffsetAabbInput& input);
    // aabbIn's centre carried back to the start transform and through each in-between transform, the box around those
    // points grown by aabbIn's half size (as far as its rotation reaches), and never smaller than aabbIn.
    static void sweepOffsetAabb(const OffsetAabbInput& input, const hkAabb& aabbIn, hkAabb& aabbOut);
};

inline void
hkAabbUtil::sweepAabb(const hkMotionState* motionState, float tolerance, const hkAabb& aabbIn, hkAabb& aabbOut) {
    const float radius = motionState->m_deltaAngle(3) * motionState->m_objectRadius;
    hkVector4 rotationalGain;
    rotationalGain.setAll(radius);
    aabbOut.m_min.setSub4(aabbIn.m_min, rotationalGain);
    aabbOut.m_max.setAdd4(aabbIn.m_max, rotationalGain);
    {
        hkVector4 radius4;
        radius4.setAll(motionState->m_objectRadius + tolerance);
        hkVector4 maxR;
        maxR.setAdd4(motionState->m_sweptTransform.m_centerOfMass1, radius4);
        hkVector4 minR;
        minR.setSub4(motionState->m_sweptTransform.m_centerOfMass1, radius4);
        aabbOut.m_min.setMax4(aabbOut.m_min, minR);
        aabbOut.m_min.setMin4(aabbOut.m_min, aabbIn.m_min);
        aabbOut.m_max.setMin4(aabbOut.m_max, maxR);
        aabbOut.m_max.setMax4(aabbOut.m_max, aabbIn.m_max);
    }
    hkVector4 invPath;
    invPath.setSub4(motionState->m_sweptTransform.m_centerOfMass0, motionState->m_sweptTransform.m_centerOfMass1);
    hkVector4 zero;
    zero.setZero4();
    hkVector4 minPath;
    minPath.setMin4(zero, invPath);
    hkVector4 maxPath;
    maxPath.setMax4(zero, invPath);
    aabbOut.m_min.add4(minPath);
    aabbOut.m_max.add4(maxPath);
}

inline void hkAabbUtil::sweepOffsetAabb(const OffsetAabbInput& input, const hkAabb& aabbIn, hkAabb& aabbOut) {
    hkSimdReal half(0.5f);
    hkVector4 aabbHalfSize;
    aabbHalfSize.setSub4(aabbIn.m_max, aabbIn.m_min);
    aabbHalfSize.mul4(half);
    hkVector4 aabbCenter;
    aabbCenter.setInterpolate4(aabbIn.m_max, aabbIn.m_min, half);
    hkVector4 arm;
    arm._setTransformedPos(input.m_endTransformInv, aabbCenter);

    hkVector4 sweptMin = aabbCenter;
    hkVector4 sweptMax = aabbCenter;
    {
        hkVector4 p;
        p._setTransformedPos(input.m_startTransform, arm);
        sweptMin.setMin4(sweptMin, p);
        sweptMax.setMax4(sweptMax, p);
    }
    const hkVector4 centerOfMassLocal = input.m_motionState.m_storage->m_sweptTransform.m_centerOfMassLocal;
    for (int32_t i = 0; i < input.m_numTransforms.m_storage; i++) {
        hkVector4 extendedArm;
        extendedArm.setSub4(arm, centerOfMassLocal);
        extendedArm.mul4(input.m_transforms[i].m_translation.getSimdAt(3));
        extendedArm.add4(centerOfMassLocal);
        hkVector4 p;
        p._setTransformedPos(input.m_transforms[i], extendedArm);
        sweptMin.setMin4(sweptMin, p);
        sweptMax.setMax4(sweptMax, p);
    }
    {
        hkSimdReal r = aabbHalfSize.length3();
        hkVector4 radius = r.broadcast();
        aabbHalfSize.addMul4(input.m_motionState.m_storage->m_deltaAngle.getSimdAt(3), radius);
        aabbHalfSize.setMin4(radius, aabbHalfSize);
    }
    aabbOut.m_min.setSub4(sweptMin, aabbHalfSize);
    aabbOut.m_max.setAdd4(sweptMax, aabbHalfSize);
    aabbOut.m_min.setMin4(aabbIn.m_min, aabbOut.m_min);
    aabbOut.m_max.setMax4(aabbIn.m_max, aabbOut.m_max);
}
