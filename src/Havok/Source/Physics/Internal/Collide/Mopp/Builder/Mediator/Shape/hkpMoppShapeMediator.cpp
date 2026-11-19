#include "Havok/Source/Physics/Internal/Collide/Mopp/Builder/Mediator/Shape/hkpMoppShapeMediator.h"

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeBuffer.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Builder/Splitter/hkpMoppSplitter.h"

#include <xmmintrin.h>

hkpMoppShapeMediator::hkpMoppShapeMediator(const hkpShapeContainer* a1)
  : m_shape(a1), m_numChildShapes(a1->getNumChildShapes()) {}

hkpMoppShapeMediator::~hkpMoppShapeMediator() {
    m_shape = nullptr;
}

void hkpMoppShapeMediator::setSplittingPlaneDirections(
    const hkpMoppSplittingPlaneDirection* directions,
    int32_t numDirections
) {
    ;
}

int32_t hkpMoppShapeMediator::getNumPrimitives() {
    return (int32_t)((uint32_t)m_numChildShapes);
}

void hkpMoppShapeMediator::getPrimitives(hkpMoppCompilerPrimitive* primitives) {
    int32_t m_numChildShapes = this->m_numChildShapes;
    uint32_t v4 = m_shape->getFirstKey();
    if (m_numChildShapes > 0) {
        do {
            primitives->m_primitiveID = v4;
            primitives->m_primitiveID2 = 0;
            ++primitives;
            v4 = m_shape->getNextKey(v4);
            --m_numChildShapes;
        }
        while (m_numChildShapes);
    }
}

// Each primitive's extent along apDirection: its child shape's maximum projection onto apDirection and the negated
// maximum projection onto -apDirection (both 0 for a missing child), stored in the primitive, and the range of all of
// them. The range takes a new extent only on an ordered compare: a NaN bound stays.
void hkpMoppShapeMediator::projectPrimitives(
    const hkVector4& direction,
    int32_t directionIndex,
    hkpMoppCompilerPrimitive* primitives,
    int32_t numPrimitives,
    float* absMinOut,
    float* absMaxOut
) {
    alignas(16) char shapeBuffer[HK_SHAPE_BUFFER_SIZE];
    float fMin;
    float fMax;
    const hkpShape* pChild = m_shape->getChildShape(primitives[0].m_primitiveID, shapeBuffer);
    if (pChild) {
        fMax = pChild->getMaximumProjection(direction);
        hkVector4 negDirection;
        negDirection.setNeg4(direction);
        fMin = -pChild->getMaximumProjection(negDirection);
    }
    else {
        fMin = 0.0f;
        fMax = 0.0f;
    }
    primitives[0].m_extent.m_min = fMin;
    primitives[0].m_extent.m_max = fMax;
    float fAbsMin = fMin;
    float fAbsMax = fMax;
    for (int32_t i = 1; i < numPrimitives; i++) {
        const hkpShape* pChildI = m_shape->getChildShape(primitives[i].m_primitiveID, shapeBuffer);
        if (pChildI) {
            fMax = pChildI->getMaximumProjection(direction);
            hkVector4 negDirection;
            negDirection.setNeg4(direction);
            fMin = -pChildI->getMaximumProjection(negDirection);
        }
        else {
            fMin = 0.0f;
            fMax = 0.0f;
        }
        primitives[i].m_extent.m_min = fMin;
        primitives[i].m_extent.m_max = fMax;
        if (fAbsMin > fMin) {
            fAbsMin = fMin;
        }
        if (fAbsMax < fMax) {
            fAbsMax = fMax;
        }
    }
    *absMinOut = fAbsMin;
    *absMaxOut = fAbsMax;
}

// The range of the primitives' extents along apDirection, computed as projectPrimitives does but not stored.
void hkpMoppShapeMediator::findExtents(
    const hkVector4& direction,
    int32_t directionIndex,
    const hkpMoppCompilerPrimitive* primitives,
    int32_t numPrimitives,
    float* absMinOut,
    float* absMaxOut
) {
    alignas(16) char shapeBuffer[HK_SHAPE_BUFFER_SIZE];
    float fMin;
    float fMax;
    const hkpShape* pChild = m_shape->getChildShape(primitives[0].m_primitiveID, shapeBuffer);
    if (pChild) {
        fMax = pChild->getMaximumProjection(direction);
        hkVector4 negDirection;
        negDirection.setNeg4(direction);
        fMin = -pChild->getMaximumProjection(negDirection);
    }
    else {
        fMin = 0.0f;
        fMax = 0.0f;
    }
    float fAbsMin = fMin;
    float fAbsMax = fMax;
    for (int32_t i = 1; i < numPrimitives; i++) {
        const hkpShape* pChildI = m_shape->getChildShape(primitives[i].m_primitiveID, shapeBuffer);
        if (pChildI) {
            fMax = pChildI->getMaximumProjection(direction);
            hkVector4 negDirection;
            negDirection.setNeg4(direction);
            fMin = -pChildI->getMaximumProjection(negDirection);
        }
        else {
            fMin = 0.0f;
            fMax = 0.0f;
        }
        if (fAbsMin > fMin) {
            fAbsMin = fMin;
        }
        if (fAbsMax < fMax) {
            fAbsMax = fMax;
        }
    }
    *absMinOut = fAbsMin;
    *absMaxOut = fAbsMax;
}

void hkpMoppShapeMediator::splitPrimitive(
    const hkpMoppCompilerPrimitive& primitiveIn,
    const hkVector4& direction,
    float planeOffset,
    int32_t depth,
    hkpMoppCompilerPrimitive* primitiveOut
) {
    *primitiveOut = primitiveIn;
}

int32_t hkpMoppShapeMediator::getPrimitiveProperties(const hkpMoppCompilerPrimitive& primitiveIn, uint32_t* cid) {
    *cid = primitiveIn.m_primitiveID2;
    return 1;
}
