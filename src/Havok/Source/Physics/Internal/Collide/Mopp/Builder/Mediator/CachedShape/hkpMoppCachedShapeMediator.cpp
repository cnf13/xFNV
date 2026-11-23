#include "Havok/Source/Physics/Internal/Collide/Mopp/Builder/Mediator/CachedShape/hkpMoppCachedShapeMediator.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Types/Geometry/Sphere/hkSphere.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexShape.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/hkpSphereRepShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Builder/Splitter/hkpMoppSplitter.h"
#include "xfnv/Compiler.h"

#include <xmmintrin.h>

hkpMoppCachedShapeMediator::hkpMoppCachedShapeMediator(const hkpShapeContainer* shapeContainer)
  : m_shapeCollection(shapeContainer), m_numChildShapes(shapeContainer->getNumChildShapes()) {}

// The cache array frees itself.
hkpMoppCachedShapeMediator::~hkpMoppCachedShapeMediator() {
    m_shapeCollection = nullptr;
}

void hkpMoppCachedShapeMediator::setSplittingPlaneDirections(
    const hkpMoppSplittingPlaneDirection* directions,
    int32_t numDirections
) {
    int32_t v4 = m_shapeCollection->getNumChildShapes();
    uint32_t v5 = m_shapeCollection->getFirstKey();
    if (v4 > 0) {
        bool v8;
        int32_t v9 = v4;
        do {
            alignas(16) char v10[512];
            const hkpConvexShape* v6 = static_cast<const hkpConvexShape*>(m_shapeCollection->getChildShape(v5, v10));
            if (v6->isConvex()) {
                addConvexShape(v6, v5, directions, numDirections);
            }
            uint32_t v7 = m_shapeCollection->getNextKey(v5);
            v8 = v9-- == 1;
            v5 = v7;
        }
        while (!v8);
    }
}

int32_t hkpMoppCachedShapeMediator::getNumPrimitives() {
    return m_numChildShapes;
}

void hkpMoppCachedShapeMediator::getPrimitives(hkpMoppCompilerPrimitive* primitives) {
    int32_t m_numChildShapes = this->m_numChildShapes;
    uint32_t v4 = m_shapeCollection->getFirstKey();
    for (int32_t i = 0; i < m_numChildShapes; ++i) {
        primitives->m_primitiveID = v4;
        primitives->m_primitiveID2 = i;
        ++primitives;
        v4 = m_shapeCollection->getNextKey(v4);
    }
}

// Copies each primitive's cached extent along direction directionIndex into the primitive and returns the range of all
// of them. The range updates as hkMath::min2/max2: a NaN bound takes the new extent.
void hkpMoppCachedShapeMediator::projectPrimitives(
    const hkVector4& direction,
    int32_t directionIndex,
    hkpMoppCompilerPrimitive* primitives,
    int32_t numPrimitives,
    float* absMinOut,
    float* absMaxOut
) {
    hkpMoppExtent kExtent = m_arrayConvexShapeData.m_data[primitives[0].m_primitiveID2].m_extents[directionIndex];
    primitives[0].m_extent = kExtent;
    float fAbsMin = kExtent.m_min;
    float fAbsMax = kExtent.m_max;
    for (int32_t i = 1; i < numPrimitives; i++) {
        kExtent = m_arrayConvexShapeData.m_data[primitives[i].m_primitiveID2].m_extents[directionIndex];
        primitives[i].m_extent = kExtent;
        fAbsMin = fAbsMin < kExtent.m_min ? fAbsMin : kExtent.m_min;
        fAbsMax = fAbsMax > kExtent.m_max ? fAbsMax : kExtent.m_max;
    }
    *absMinOut = fAbsMin;
    *absMaxOut = fAbsMax;
}

// The range of the primitives' cached extents along direction directionIndex. Each new extent is compared against the
// bound so that a NaN bound takes the new extent.
void hkpMoppCachedShapeMediator::findExtents(
    const hkVector4& direction,
    int32_t directionIndex,
    const hkpMoppCompilerPrimitive* primitives,
    int32_t numPrimitives,
    float* absMinOut,
    float* absMaxOut
) {
    const hkpMoppExtent& rFirst = m_arrayConvexShapeData.m_data[primitives[0].m_primitiveID2].m_extents[directionIndex];
    float fAbsMin = rFirst.m_min;
    float fAbsMax = rFirst.m_max;
    for (int32_t i = 1; i < numPrimitives; i++) {
        const hkpMoppExtent& rExtent = m_arrayConvexShapeData.m_data[primitives[i].m_primitiveID2]
                                           .m_extents[directionIndex];
        float fMin = rExtent.m_min;
        float fMax = rExtent.m_max;
        fAbsMin = fMin > fAbsMin ? fAbsMin : fMin;
        fAbsMax = fMax < fAbsMax ? fAbsMax : fMax;
    }
    *absMinOut = fAbsMin;
    *absMaxOut = fAbsMax;
}

void hkpMoppCachedShapeMediator::splitPrimitive(
    const hkpMoppCompilerPrimitive& primitiveIn,
    const hkVector4& direction,
    float planeOffset,
    int32_t depth,
    hkpMoppCompilerPrimitive* primitiveOut
) {
    *primitiveOut = primitiveIn;
}

int32_t hkpMoppCachedShapeMediator::getPrimitiveProperties(const hkpMoppCompilerPrimitive& primitiveIn, uint32_t* cid) {
    *cid = 0;
    return 1;
}

// Appends a cache entry for the convex child convexShape (shape key key): its extent along each of the numDirections
// directions is the range of its collision spheres' projections, centre dot direction minus and plus radius (both 0
// without spheres). The range takes a new value only on an ordered compare: a NaN bound stays. The spheres live on the
// Havok stack allocator for the call.
void hkpMoppCachedShapeMediator::addConvexShape(
    const hkpConvexShape* convexShape,
    uint32_t key,
    const hkpMoppSplittingPlaneDirection* directions,
    int32_t numDirections
) {
    const int32_t iNumSpheres = convexShape->getNumCollisionSpheresImpl();
    hkSphere* pSpheres = hkAllocateStack<hkSphere>(iNumSpheres, nullptr);
    convexShape->getCollisionSpheresImpl(pSpheres);

    hkpConvexShapeData& rData = m_arrayConvexShapeData.expandOne();
    rData.m_key = key;

    for (int32_t i = 0; i < numDirections; i++) {
        float fAbsMin;
        float fAbsMax;
        if (iNumSpheres <= 0) {
            fAbsMin = 0.0f;
            fAbsMax = 0.0f;
        }
        else {
            const hkVector4& kDirection = directions[i].m_direction;
            float fRadius = XFNV_M128_F32(pSpheres[0].m_pos.m_quad, 3);
            const float fDot = pSpheres[0].m_pos.dot3(kDirection);
            fAbsMin = fDot - fRadius;
            fAbsMax = fDot + fRadius;
            for (int32_t j = 1; j < iNumSpheres; j++) {
                fRadius = XFNV_M128_F32(pSpheres[j].m_pos.m_quad, 3);
                const float fDotJ = pSpheres[j].m_pos.dot3(kDirection);
                const float fMin = fDotJ - fRadius;
                const float fMax = fDotJ + fRadius;
                if (fAbsMin > fMin) {
                    fAbsMin = fMin;
                }
                if (fAbsMax < fMax) {
                    fAbsMax = fMax;
                }
            }
        }
        rData.m_extents[i].m_min = fAbsMin;
        rData.m_extents[i].m_max = fAbsMax;
    }

    hkDeallocateStack(pSpheres);
}
