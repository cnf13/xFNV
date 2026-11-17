#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"

struct hkpMoppCompilerPrimitive;
struct hkpMoppSplittingPlaneDirection;

// The interface the MOPP compiler builds its tree through: it lists the primitives, projects them onto the splitting
// plane directions, finds their extents and splits them.
class hkpMoppMediator : public hkReferencedObject {
public:
    virtual void
    setSplittingPlaneDirections(const hkpMoppSplittingPlaneDirection* apDirections, int32_t aiNumDirections) = 0;
    virtual int32_t getNumPrimitives() = 0;
    virtual void getPrimitives(hkpMoppCompilerPrimitive* apPrimitiveArray) = 0;
    virtual void projectPrimitives(
        const hkVector4& apDirection,
        int32_t aiDirectionIndex,
        hkpMoppCompilerPrimitive* apPrimitives,
        int32_t aiNumPrimitives,
        float* apAbsMin,
        float* apAbsMax
    ) = 0;
    virtual void findExtents(
        const hkVector4& apDirection,
        int32_t aiDirectionIndex,
        const hkpMoppCompilerPrimitive* apPrimitives,
        int32_t aiNumPrimitives,
        float* apAbsMin,
        float* apAbsMax
    ) = 0;
    virtual void splitPrimitive(
        const hkpMoppCompilerPrimitive& apPrimitiveIn,
        const hkVector4& apDirection,
        float afPlaneOffset,
        int32_t aiDepth,
        hkpMoppCompilerPrimitive* apPrimitiveOut
    ) = 0;
    virtual int32_t getPrimitiveProperties(const hkpMoppCompilerPrimitive& apPrimitive, uint32_t* apProperties) = 0;
};
