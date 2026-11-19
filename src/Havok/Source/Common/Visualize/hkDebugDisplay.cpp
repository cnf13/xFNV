#include "Havok/Source/Common/Visualize/hkDebugDisplay.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3x4.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Thread/CriticalSection/hkCriticalSection.h"
#include "Havok/Source/Common/Base/Types/Color/hkColor.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometry.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"

#include <bit>
#include <xmmintrin.h>

// m_value is untyped storage: populate and depopulate move any singleton's instance pointer through it.
static hkSingletonInitNode hkSingletonRegisterhkDebugDisplay(
    "hkDebugDisplay",
    &hkDebugDisplaycreate,
    reinterpret_cast<void**>(&hkDebugDisplay::s_instance)
);

// Starts with an empty handler list and a lock with spin count 1000 in a heap block (none when the allocation fails).
hkDebugDisplay::hkDebugDisplay() {
    void* pLockMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkCriticalSection));
    m_arrayLock = pLockMemory ? new (pLockMemory) hkCriticalSection(1000) : nullptr;
}

// The lock is freed as a heap block of its size, as the game does; m_debugDisplayHandlers frees its buffer in its own
// destructor.
hkDebugDisplay::~hkDebugDisplay() {
    if (m_arrayLock) {
        m_arrayLock->~hkCriticalSection();
        hkMemoryRouter::getInstance().m_heap->blockFree(m_arrayLock, sizeof(hkCriticalSection));
    }
}

void hkDebugDisplay::addGeometry(
    const hkArray<hkDisplayGeometry*, hkContainerHeapAllocator>& geometries,
    const hkTransform& transform,
    hkUlong id,
    int32_t tag,
    hkUlong shapeIdHint
) {
    EnterCriticalSection(&m_arrayLock->m_section);
    for (int32_t i = 0; i < m_debugDisplayHandlers.m_size; ++i) {
        hkDebugDisplayHandler* v8 = m_debugDisplayHandlers.m_data[i];
        v8->addGeometry(geometries, transform, id, tag, shapeIdHint);
    }
    LeaveCriticalSection(&m_arrayLock->m_section);
}

// Finds the handler's index, then removes it: the last handler moves into its slot.
void hkDebugDisplay::removeDebugDisplayHandler(hkDebugDisplayHandler* debugDisplay) {
    EnterCriticalSection(&m_arrayLock->m_section);
    const int32_t iIndex = m_debugDisplayHandlers.indexOf(debugDisplay);
    if (iIndex >= 0) {
        m_debugDisplayHandlers.removeAt(iIndex);
    }
    LeaveCriticalSection(&m_arrayLock->m_section);
}

void hkDebugDisplay::displayLine(const hkVector4& start, const hkVector4& end, int32_t color, int32_t id, int32_t tag) {
    EnterCriticalSection(&m_arrayLock->m_section);
    for (int32_t i = 0; i < m_debugDisplayHandlers.m_size; ++i) {
        hkDebugDisplayHandler* v8 = m_debugDisplayHandlers.m_data[i];
        v8->displayLine(start, end, color, id, tag);
    }
    LeaveCriticalSection(&m_arrayLock->m_section);
}

// The vector code is written as lane operations with intrinsics; the three-lane sums are packed, as only lane 0 is
// read. The normalize3 Newton step uses the constants 0.0, 3.0 and 0.5; 0.85 and the head's 0.15 offsets (1 - 0.85) are
// float constants.
void hkDebugDisplay::displayArrow(const hkVector4& from, const hkVector4& dir, int32_t color, int32_t id, int32_t tag) {
    const __m128 kDir = _mm_load_ps(&reinterpret_cast<const float*>(&dir.m_quad)[0]);
    const __m128 kDirSquares = _mm_mul_ps(kDir, kDir);
    const __m128 kDirLengthSquared = _mm_add_ps(
        _mm_shuffle_ps(kDirSquares, kDirSquares, 0xAA),
        _mm_add_ps(_mm_shuffle_ps(kDirSquares, kDirSquares, 0x55), kDirSquares)
    );
    const float fDirLengthSquared = _mm_cvtss_f32(kDirLengthSquared);
    if (fDirLengthSquared < 1.1920929e-07f) {
        return;
    }

    hkVector4 to;
    _mm_store_ps(
        &reinterpret_cast<float*>(&to.m_quad)[0],
        _mm_add_ps(_mm_load_ps(&reinterpret_cast<const float*>(&from.m_quad)[0]), kDir)
    );

    // EXE-INLINED none: hkVector4Util::calculatePerpendicularVector
    const float* pDir = &reinterpret_cast<const float*>(&dir.m_quad)[0];
    float fMin = __builtin_fabsf(pDir[0]);
    const float fAbsY = __builtin_fabsf(pDir[1]);
    const float fAbsZ = __builtin_fabsf(pDir[2]);
    int32_t iMin = 0;
    int32_t iFirst = 1;
    int32_t iSecond = 2;
    if (fMin > fAbsY) {
        fMin = fAbsY;
        iFirst = 0;
        iMin = 1;
    }
    if (fMin > fAbsZ) {
        iSecond = iMin;
    }
    hkVector4 ort;
    _mm_store_ps(&reinterpret_cast<float*>(&ort.m_quad)[0], _mm_setzero_ps());
    (&reinterpret_cast<float*>(&ort.m_quad)[0])[iFirst] = pDir[iSecond];
    (&reinterpret_cast<float*>(&ort.m_quad)[0])[iSecond] = -pDir[iFirst];

    // EXE-INLINED none: hkVector4::normalize3 (rsqrtss and one Newton step, zero for a zero vector)
    const __m128 kOrt = _mm_load_ps(&reinterpret_cast<float*>(&ort.m_quad)[0]);
    const __m128 kOrtSquares = _mm_mul_ps(kOrt, kOrt);
    const __m128 kOrtLengthSquared = _mm_add_ps(
        _mm_shuffle_ps(kOrtSquares, kOrtSquares, 0xAA),
        _mm_add_ps(_mm_shuffle_ps(kOrtSquares, kOrtSquares, 0x55), kOrtSquares)
    );
    const __m128 kEstimate = _mm_rsqrt_ss(kOrtLengthSquared);
    const __m128 kZeroMask = _mm_cmpeq_ss(kOrtLengthSquared, _mm_set_ss(0.0f));
    const __m128 kStep = _mm_sub_ps(_mm_set_ss(3.0f), _mm_mul_ps(_mm_mul_ps(kOrtLengthSquared, kEstimate), kEstimate));
    const __m128 kScale = _mm_andnot_ps(kZeroMask, _mm_mul_ps(_mm_mul_ps(_mm_set_ss(0.5f), kEstimate), kStep));
    const __m128 kOrtUnit = _mm_mul_ps(_mm_shuffle_ps(kScale, kScale, 0), kOrt);

    // EXE-INLINED none: hkVector4::setCross
    const __m128 kOrt2 = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(kDir, kDir, 0xC9), _mm_shuffle_ps(kOrtUnit, kOrtUnit, 0xD2)),
        _mm_mul_ps(_mm_shuffle_ps(kDir, kDir, 0xD2), _mm_shuffle_ps(kOrtUnit, kOrtUnit, 0xC9))
    );
    // EXE-INLINED 0x630B40
    const __m128 kDirLength = _mm_sqrt_ss(kDirLengthSquared);
    const __m128 kOrtScaled = _mm_mul_ps(_mm_shuffle_ps(kDirLength, kDirLength, 0), kOrtUnit);

    // EXE-INLINED none: hkVector4::setInterpolate4
    const float fC = 0.85f;
    const __m128 kC = _mm_set1_ps(fC);
    const __m128 kP = _mm_add_ps(
        _mm_mul_ps(
            _mm_sub_ps(_mm_load_ps(&reinterpret_cast<float*>(&hkQuadReal1111_0)[0]), kC),
            _mm_load_ps(&reinterpret_cast<const float*>(&from.m_quad)[0])
        ),
        _mm_mul_ps(kC, _mm_load_ps(&reinterpret_cast<float*>(&to.m_quad)[0]))
    );

    // EXE-INLINED none: hkVector4::setAddMul4
    const __m128 kSide = _mm_set1_ps(0.1499999761581421f);
    const __m128 kNegSide = _mm_set1_ps(-0.1499999761581421f);
    hkVector4 p0;
    _mm_store_ps(&reinterpret_cast<float*>(&p0.m_quad)[0], _mm_add_ps(_mm_mul_ps(kSide, kOrtScaled), kP));
    hkVector4 p1;
    _mm_store_ps(&reinterpret_cast<float*>(&p1.m_quad)[0], _mm_add_ps(_mm_mul_ps(kNegSide, kOrtScaled), kP));
    hkVector4 p2;
    _mm_store_ps(&reinterpret_cast<float*>(&p2.m_quad)[0], _mm_add_ps(_mm_mul_ps(kSide, kOrt2), kP));
    hkVector4 p3;
    _mm_store_ps(&reinterpret_cast<float*>(&p3.m_quad)[0], _mm_add_ps(_mm_mul_ps(kNegSide, kOrt2), kP));

    displayLine(from, to, color, id, tag);
    displayLine(to, p0, color, id, tag);
    displayLine(to, p1, color, id, tag);
    displayLine(to, p2, color, id, tag);
    displayLine(to, p3, color, id, tag);
}

void hkDebugDisplay::displayLineModelSpace(
    const hkQsTransform& worldFromModel,
    const hkVector4& start,
    const hkVector4& end,
    int32_t color,
    int32_t id,
    int32_t tag
) {
    hkVector4 v8;
    hkVector4 v9;
    v9.setTransformedPos(worldFromModel, start);
    v8.setTransformedPos(worldFromModel, end);
    displayLine(v9, v8, color, id, tag);
}

void hkDebugDisplay::displayLineModelSpace(
    const hkTransform& worldFromModel,
    const hkVector4& start,
    const hkVector4& end,
    int32_t color,
    int32_t id,
    int32_t tag
) {
    hkQsTransform t;
    t.setFromTransform(worldFromModel);
    displayLineModelSpace(t, start, end, color, id, tag);
}

void hkDebugDisplay::displayRayModelSpace(
    const hkQsTransform& worldFromModel,
    const hkVector4& start,
    const hkVector4& direction,
    int32_t color,
    int32_t id,
    int32_t tag
) {
    hkQuaternion v8;
    hkVector4 v13;
    hkVector4 a1;
    a1.setTransformedPos(worldFromModel, start);
    v8.m_vec.m_quad = worldFromModel.m_rotation.m_vec.m_quad;
    __m128 v9 = _mm_mul_ps(direction.m_quad, v8.m_vec.m_quad);
    __m128 v10 = _mm_shuffle_ps(v9, v9, 170);
    v10[0] = v10[0] + (_mm_shuffle_ps(v9, v9, 85)[0] + v9[0]);
    __m128 v11 = _mm_shuffle_ps(v8.m_vec.m_quad, v8.m_vec.m_quad, 255);
    __m128 v12 = _mm_add_ps(
        _mm_mul_ps(
            _mm_sub_ps(
                _mm_mul_ps(
                    _mm_shuffle_ps(direction.m_quad, direction.m_quad, 210),
                    _mm_shuffle_ps(v8.m_vec.m_quad, v8.m_vec.m_quad, 201)
                ),
                _mm_mul_ps(
                    _mm_shuffle_ps(direction.m_quad, direction.m_quad, 201),
                    _mm_shuffle_ps(v8.m_vec.m_quad, v8.m_vec.m_quad, 210)
                )
            ),
            v11
        ),
        _mm_add_ps(
            _mm_mul_ps(_mm_add_ps(_mm_mul_ps(v11, v11), hkQuadRealMinusHalf), direction.m_quad),
            _mm_mul_ps(_mm_shuffle_ps(v10, v10, 0), v8.m_vec.m_quad)
        )
    );
    v13.m_quad = _mm_add_ps(v12, v12);
    displayRay(a1, v13, color, id, tag);
}

void hkDebugDisplay::displayRayModelSpace(
    const hkTransform& worldFromModel,
    const hkVector4& start,
    const hkVector4& direction,
    int32_t color,
    int32_t id,
    int32_t tag
) {
    hkQsTransform t;
    t.setFromTransform(worldFromModel);
    displayRayModelSpace(t, start, direction, color, id, tag);
}

void hkDebugDisplay::addDebugDisplayHandler(hkDebugDisplayHandler* debugDisplay) {
    EnterCriticalSection(&m_arrayLock->m_section);
    m_debugDisplayHandlers.pushBack(debugDisplay);
    LeaveCriticalSection(&m_arrayLock->m_section);
}

// Each bone: a line to its parent's position (or, for a root and showRootTranslation, to the model's origin), its
// length clamped to [0.1, 10] and scaled by boneAxisSize, then the bone's three axes of that length in red, green and
// blue. The bone's position is copied aside before the quaternion product and stored back before the second and third
// axis.
void hkDebugDisplay::displayModelSpacePose(
    int32_t numTransforms,
    const int16_t* parentIndices,
    const hkQsTransform* modelSpacePose,
    const hkQsTransform& worldFromModel,
    int32_t id,
    int32_t tag,
    int32_t color,
    float boneAxisSize,
    int32_t showRootTranslation
) {
    if (static_cast<int16_t>(numTransforms) > 0) {
        const int16_t* parentIndex = parentIndices;
        const hkQsTransform* bone = modelSpacePose;
        int32_t remaining = static_cast<uint16_t>(numTransforms);
        do {
            hkVector4 bonePos;
            bonePos.setTransformedPos(worldFromModel, bone->m_translation);
            float boneLength = 1.0f;
            if (*parentIndex == -1) {
                if (static_cast<uint8_t>(showRootTranslation)) {
                    hkVector4 root;
                    root.set(0.0f, 0.0f, 0.0f, 0.0f);
                    root.setTransformedPos(worldFromModel, root);
                    displayLine(bonePos, root, color, id, tag);
                }
            }
            else {
                hkVector4 parentPos;
                parentPos.setTransformedPos(worldFromModel, modelSpacePose[*parentIndex].m_translation);
                displayLine(bonePos, parentPos, color, id, tag);
                const __m128 diff = _mm_sub_ps(bonePos.m_quad, parentPos.m_quad);
                const __m128 squares = _mm_mul_ps(diff, diff);
                const __m128 sumXY = _mm_add_ss(_mm_shuffle_ps(squares, squares, 0x55), squares);
                const __m128 lengthSquared = _mm_add_ss(_mm_shuffle_ps(squares, squares, 0xAA), sumXY);
                const float distance = _mm_cvtss_f32(_mm_sqrt_ss(lengthSquared));
                boneLength = distance;
                if (distance > 10.0f) {
                    boneLength = 10.0f;
                }
                if (boneLength < 0.10000000149011612) {
                    boneLength = 0.1f;
                }
            }
            const hkVector4 start = bonePos;
            hkQuaternion rotation;
            rotation.setMul(worldFromModel.m_rotation, bone->m_rotation);
            const float axisLength = boneLength * boneAxisSize;
            hkVector4 axis;
            hkVector4 end;
            axis.set(axisLength, 0.0f, 0.0f, 0.0f);
            end.setRotatedDir(rotation, axis);
            end.add4(bonePos);
            displayLine(bonePos, end, 0x7FFF0000, id, tag);
            bonePos = start;
            axis.set(0.0f, axisLength, 0.0f, 0.0f);
            end.setRotatedDir(rotation, axis);
            end.add4(start);
            displayLine(bonePos, end, 0x7F00FF00, id, tag);
            bonePos = start;
            axis.set(0.0f, 0.0f, axisLength, 0.0f);
            end.setRotatedDir(rotation, axis);
            end.add4(start);
            displayLine(bonePos, end, 0x7F0000FF, id, tag);
            ++parentIndex;
            ++bone;
        }
        while (--remaining);
    }
}

// The model space pose goes in a buffer from the thread's stack allocator (each bone's size rounded up to 0x80 bytes in
// all), built bone by bone from its parent's (the identity for a root), then handed to displayModelSpacePose.
// showRootTranslation is an hkBool passed as an int32_t, so the caller's whole value is passed on.
void hkDebugDisplay::displayLocalSpacePose(
    int32_t numTransforms,
    const int16_t* parentIndices,
    const hkQsTransform* localSpacePose,
    const hkQsTransform& worldFromModel,
    int32_t id,
    int32_t tag,
    int32_t color,
    float boneAxisSize,
    int32_t showRootTranslation
) {
    hkMemoryRouter& kRouter = hkMemoryRouter::getInstance();
    // EXE-INLINED 0x458B50
    char* pBuffer = kRouter.m_stack.m_next.m_storage;
    kRouter.m_stack.m_next.m_storage = pBuffer + ((numTransforms * 0x30 + 0x7F) & ~0x7F);
    // The stack allocator's bytes are untyped memory, used here as the pose.
    hkQsTransform* pModelSpacePose = reinterpret_cast<hkQsTransform*>(pBuffer);
    // EXE-INLINED none: hkaSkeletonUtils::transformLocalPoseToModelPose
    for (int32_t i = 0; i < numTransforms; ++i) {
        const int32_t iParent = parentIndices[i];
        const hkQsTransform& rParent = iParent == -1 ? hkQsTransformIdentity : pModelSpacePose[iParent];
        const hkQsTransform& rLocal = localSpacePose[i];
        hkQsTransform& rModel = pModelSpacePose[i];
        // EXE-INLINED none: hkQsTransform::setMul (translation and scale; the rotation is the out-of-line call)
        hkVector4 kRotated;
        kRotated.setRotatedDir(rParent.m_rotation, rLocal.m_translation);
        _mm_store_ps(
            &reinterpret_cast<float*>(&rModel.m_translation.m_quad)[0],
            _mm_add_ps(
                _mm_load_ps(&reinterpret_cast<float*>(&kRotated.m_quad)[0]),
                _mm_load_ps(&reinterpret_cast<const float*>(&rParent.m_translation.m_quad)[0])
            )
        );
        rModel.m_rotation.setMul(rParent.m_rotation, rLocal.m_rotation);
        _mm_store_ps(
            &reinterpret_cast<float*>(&rModel.m_scale.m_quad)[0],
            _mm_mul_ps(
                _mm_load_ps(&reinterpret_cast<const float*>(&rLocal.m_scale.m_quad)[0]),
                _mm_load_ps(&reinterpret_cast<const float*>(&rParent.m_scale.m_quad)[0])
            )
        );
    }
    displayModelSpacePose(
        numTransforms, parentIndices, pModelSpacePose, worldFromModel, id, tag, color, boneAxisSize, showRootTranslation
    );
    hkMemoryRouter::getInstance().m_stack.m_next.m_storage = pBuffer;
}

void hkDebugDisplay::setGeometryColor(int32_t a1, hkUlong a2, int32_t a3) {
    int32_t i;
    hkDebugDisplayHandler* v6;
    EnterCriticalSection(&m_arrayLock->m_section);
    for (i = 0; i < m_debugDisplayHandlers.m_size; ++i) {
        v6 = m_debugDisplayHandlers.m_data[i];
        v6->setGeometryColor(a1, a2, a3);
    }
    LeaveCriticalSection(&m_arrayLock->m_section);
}

void hkDebugDisplay::displayTriangle(
    const hkVector4& a,
    const hkVector4& b,
    const hkVector4& c,
    int32_t color,
    int32_t id,
    int32_t tag
) {
    EnterCriticalSection(&m_arrayLock->m_section);
    for (int32_t i = 0; i < m_debugDisplayHandlers.m_size; ++i) {
        hkDebugDisplayHandler* v9 = m_debugDisplayHandlers.m_data[i];
        v9->displayTriangle(a, b, c, color, id, tag);
    }
    LeaveCriticalSection(&m_arrayLock->m_section);
}

// Lights the triangle's normal against two directional lights (each light direction normalised and scaled by 0.9, its
// dot with the normal mapped from [-1, 1] to [0, 1]), the colour lit by each light's colour plus a 0.05 ambient term,
// clamped to [0, 1], and drawn through the singleton (HK_DISPLAY_TRIANGLE: s_instance, not this). The colour bytes are
// scaled by 1/255 and stored as floats.
void hkDebugDisplay::displayLitTriangle(const hkVector4& a, const hkVector4& b, const hkVector4& c, int32_t color) {
    const __m128 kZero = _mm_setzero_ps();
    const __m128 kThree = _mm_set_ss(3.0f);
    const __m128 kHalf = _mm_set_ss(0.5f);
    // EXE-INLINED none: hkVector4::setCross
    const __m128 kAC = _mm_sub_ps(c.m_quad, a.m_quad);
    const __m128 kAB = _mm_sub_ps(b.m_quad, a.m_quad);
    const __m128 kCross = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(kAC, kAC, 0xD2), _mm_shuffle_ps(kAB, kAB, 0xC9)),
        _mm_mul_ps(_mm_shuffle_ps(kAC, kAC, 0xC9), _mm_shuffle_ps(kAB, kAB, 0xD2))
    );
    // EXE-INLINED none: hkVector4::normalize3
    const __m128 kCrossSq = _mm_mul_ps(kCross, kCross);
    const __m128 kCrossLen2 = _mm_add_ss(
        _mm_shuffle_ps(kCrossSq, kCrossSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(kCrossSq, kCrossSq, 0x55), kCrossSq)
    );
    const __m128 kCrossR = _mm_rsqrt_ss(kCrossLen2);
    const __m128 kCrossInv = _mm_andnot_ps(
        _mm_cmpeq_ss(kCrossLen2, kZero),
        _mm_mul_ss(_mm_mul_ss(kHalf, kCrossR), _mm_sub_ss(kThree, _mm_mul_ss(_mm_mul_ss(kCrossLen2, kCrossR), kCrossR)))
    );
    const __m128 kNormal = _mm_mul_ps(_mm_shuffle_ps(kCrossInv, kCrossInv, 0), kCross);

    // The lights' directions, each normalised.
    const __m128 kLight0 = _mm_setr_ps(0.5f, 2.0f, 1.0f, 0.0f);
    const __m128 kLight0Sq = _mm_mul_ps(kLight0, kLight0);
    const __m128 kLight0Len2 = _mm_add_ss(
        _mm_shuffle_ps(kLight0Sq, kLight0Sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(kLight0Sq, kLight0Sq, 0x55), kLight0Sq)
    );
    const __m128 kLight0R = _mm_rsqrt_ss(kLight0Len2);
    const __m128 kLight0Inv = _mm_andnot_ps(
        _mm_cmpeq_ss(kLight0Len2, kZero),
        _mm_mul_ss(
            _mm_mul_ss(kHalf, kLight0R), _mm_sub_ss(kThree, _mm_mul_ss(_mm_mul_ss(kLight0Len2, kLight0R), kLight0R))
        )
    );
    const __m128 kLight1 = _mm_setr_ps(-3.0f, -2.0f, -1.0f, 0.0f);
    const __m128 kLight1Sq = _mm_mul_ps(kLight1, kLight1);
    const __m128 kLight1Len2 = _mm_add_ss(
        _mm_shuffle_ps(kLight1Sq, kLight1Sq, 0xAA), _mm_add_ss(_mm_shuffle_ps(kLight1Sq, kLight1Sq, 0x55), kLight1Sq)
    );
    const __m128 kLight1R = _mm_rsqrt_ss(kLight1Len2);
    const __m128 kLight1Inv = _mm_andnot_ps(
        _mm_cmpeq_ss(kLight1Len2, kZero),
        _mm_mul_ss(
            _mm_mul_ss(kHalf, kLight1R), _mm_sub_ss(kThree, _mm_mul_ss(_mm_mul_ss(kLight1Len2, kLight1R), kLight1R))
        )
    );

    // The colour's bytes as floats in [0, 1] (alpha later, for the call).
    const float fRed = static_cast<double>((static_cast<uint32_t>(color) >> 16) & 0xFF) * 0.003921568859368563;
    const float fGreen = static_cast<double>((static_cast<uint32_t>(color) >> 8) & 0xFF) * 0.003921568859368563;
    const float fBlue = static_cast<double>(static_cast<uint32_t>(color) & 0xFF) * 0.003921568859368563;
    const __m128 kColour = _mm_setr_ps(fRed, fGreen, fBlue, 0.0f);

    const __m128 kStrength = _mm_set1_ps(0.9f);
    const __m128 kDot0 =
        _mm_mul_ps(_mm_mul_ps(_mm_mul_ps(_mm_shuffle_ps(kLight0Inv, kLight0Inv, 0), kLight0), kStrength), kNormal);
    const __m128 kDot0Sum =
        _mm_add_ss(_mm_shuffle_ps(kDot0, kDot0, 0xAA), _mm_add_ss(_mm_shuffle_ps(kDot0, kDot0, 0x55), kDot0));
    const __m128 kShade0 = _mm_add_ss(_mm_mul_ss(kDot0Sum, kHalf), kHalf);
    const __m128 kDot1 =
        _mm_mul_ps(_mm_mul_ps(_mm_mul_ps(_mm_shuffle_ps(kLight1Inv, kLight1Inv, 0), kLight1), kStrength), kNormal);
    const __m128 kDot1Sum =
        _mm_add_ss(_mm_shuffle_ps(kDot1, kDot1, 0xAA), _mm_add_ss(_mm_shuffle_ps(kDot1, kDot1, 0x55), kDot1));
    const __m128 kShade1 = _mm_add_ss(_mm_mul_ss(kDot1Sum, kHalf), kHalf);

    // The lights' colours and the ambient term.
    const __m128 kLight1Colour = _mm_setr_ps(0.0f, 0.05f, 0.08f, 0.0f);
    const __m128 kLight0Colour = _mm_setr_ps(0.75f, 0.75f, 0.75f, 0.0f);
    const __m128 kLit1 = _mm_mul_ps(_mm_mul_ps(kLight1Colour, kColour), _mm_shuffle_ps(kShade1, kShade1, 0));
    const __m128 kLit0 = _mm_mul_ps(_mm_mul_ps(kLight0Colour, kColour), _mm_shuffle_ps(kShade0, kShade0, 0));
    const __m128 kAmbient = _mm_mul_ps(_mm_set1_ps(0.05f), kColour);
    const float fAlpha = static_cast<double>(static_cast<uint32_t>(color) >> 24) * 0.003921568859368563;
    hkVector4 finalColor;
    finalColor.m_quad =
        _mm_min_ps(_mm_max_ps(_mm_add_ps(_mm_add_ps(kLit1, kLit0), kAmbient), hkVector4Zero.m_quad), _mm_set1_ps(1.0f));
    const int32_t iColor = hkColor::rgbFromFloats(finalColor(0), finalColor(1), finalColor(2), fAlpha);
    hkDebugDisplay::s_instance->displayTriangle(a, b, c, iColor, 0, 0);
}

void hkDebugDisplay::displayStarModelSpace(
    const hkQsTransform& worldFromModel,
    const hkVector4& position,
    float scale,
    int32_t color,
    int32_t id,
    int32_t tag
) {
    hkVector4 a1;
    a1.setTransformedPos(worldFromModel, position);
    displayStar(&a1.m_quad, scale, color, id, tag);
}

void hkDebugDisplay::addGeometryInstance(
    hkUlong instanceId,
    const hkTransform& transform,
    hkUlong id,
    int32_t tag,
    hkUlong shapeIdHint
) {
    EnterCriticalSection(&m_arrayLock->m_section);
    for (int32_t i = 0; i < m_debugDisplayHandlers.m_size; ++i) {
        hkDebugDisplayHandler* v8 = m_debugDisplayHandlers.m_data[i];
        v8->addGeometryInstance(instanceId, transform, id, tag, shapeIdHint);
    }
    LeaveCriticalSection(&m_arrayLock->m_section);
}

void hkDebugDisplay::updateGeometry(const hkTransform& transform, hkUlong id, int32_t tag) {
    EnterCriticalSection(&m_arrayLock->m_section);
    for (int32_t i = 0; i < m_debugDisplayHandlers.m_size; ++i) {
        hkDebugDisplayHandler* v6 = m_debugDisplayHandlers.m_data[i];
        v6->updateGeometry(transform, id, tag);
    }
    LeaveCriticalSection(&m_arrayLock->m_section);
}

void hkDebugDisplay::removeGeometry(hkUlong id, int32_t tag, hkUlong shapeIdHint) {
    EnterCriticalSection(&m_arrayLock->m_section);
    for (int32_t i = 0; i < m_debugDisplayHandlers.m_size; ++i) {
        hkDebugDisplayHandler* v6 = m_debugDisplayHandlers.m_data[i];
        v6->removeGeometry(id, tag, shapeIdHint);
    }
    LeaveCriticalSection(&m_arrayLock->m_section);
}

void hkDebugDisplay::updateCamera(
    const hkVector4& from,
    const hkVector4& to,
    const hkVector4& up,
    float nearPlane,
    float farPlane,
    float fov,
    const char* name
) {
    EnterCriticalSection(&m_arrayLock->m_section);
    for (int32_t i = 0; i < m_debugDisplayHandlers.m_size; ++i) {
        hkDebugDisplayHandler* v10 = m_debugDisplayHandlers.m_data[i];
        v10->updateCamera(from, to, up, nearPlane, farPlane, fov, name);
    }
    LeaveCriticalSection(&m_arrayLock->m_section);
}

void hkDebugDisplay::updateBehavior(
    hkArray<int32_t, hkContainerHeapAllocator>& wordVarIdx,
    hkArray<int32_t, hkContainerHeapAllocator>& wordStack,
    hkArray<int32_t, hkContainerHeapAllocator>& quadVarIdx,
    hkArray<hkVector4, hkContainerHeapAllocator>& quadStack,
    hkArray<char*, hkContainerHeapAllocator>& activeNodes,
    hkArray<int32_t, hkContainerHeapAllocator>& activeStateIds,
    hkArray<int32_t, hkContainerHeapAllocator>& activeTransitions,
    hkArray<hkQsTransform, hkContainerHeapAllocator>& transforms
) {
    EnterCriticalSection(&m_arrayLock->m_section);
    for (int32_t i = 0; i < m_debugDisplayHandlers.m_size; ++i) {
        hkDebugDisplayHandler* v11 = m_debugDisplayHandlers.m_data[i];
        v11->updateBehavior(
            wordVarIdx, wordStack, quadVarIdx, quadStack, activeNodes, activeStateIds, activeTransitions, transforms
        );
    }
    LeaveCriticalSection(&m_arrayLock->m_section);
}

void hkDebugDisplay::displayPoint(const hkVector4& position, int32_t color, int32_t id, int32_t tag) {
    EnterCriticalSection(&m_arrayLock->m_section);
    for (int32_t i = 0; i < m_debugDisplayHandlers.m_size; ++i) {
        hkDebugDisplayHandler* v7 = m_debugDisplayHandlers.m_data[i];
        v7->displayPoint(position, color, id, tag);
    }
    LeaveCriticalSection(&m_arrayLock->m_section);
}

// Takes no array lock; calls each handler's displayLine from start to start + direction.
void hkDebugDisplay::displayRay(
    const hkVector4& start,
    const hkVector4& direction,
    int32_t color,
    int32_t id,
    int32_t tag
) {
    hkVector4 end;
    end.setAdd4(start, direction);
    for (int32_t i = 0; i < m_debugDisplayHandlers.m_size; ++i) {
        hkDebugDisplayHandler* handler = m_debugDisplayHandlers.m_data[i];
        handler->displayLine(start, end, color, id, tag);
    }
}

// The twelve edges as 24 end points from the box's min and max corners, each with w 0, drawn pairwise.
void hkDebugDisplay::displayAabb(const hkAabb& aabb, int32_t color, int32_t id, int32_t tag) {
    const hkVector4& mn = aabb.m_min;
    const hkVector4& mx = aabb.m_max;
    hkVector4 lines[24];
    lines[0].set(mn(0), mn(1), mn(2), 0.0f);
    lines[1].set(mn(0), mx(1), mn(2), 0.0f);
    lines[2].set(mn(0), mn(1), mn(2), 0.0f);
    lines[3].set(mn(0), mn(1), mx(2), 0.0f);
    lines[4].set(mn(0), mn(1), mn(2), 0.0f);
    lines[5].set(mx(0), mn(1), mn(2), 0.0f);
    lines[6].set(mx(0), mx(1), mx(2), 0.0f);
    lines[7].set(mx(0), mx(1), mn(2), 0.0f);
    lines[8].set(mx(0), mx(1), mx(2), 0.0f);
    lines[9].set(mn(0), mx(1), mx(2), 0.0f);
    lines[10].set(mx(0), mx(1), mx(2), 0.0f);
    lines[11].set(mx(0), mn(1), mx(2), 0.0f);
    lines[12].set(mn(0), mx(1), mn(2), 0.0f);
    lines[13].set(mx(0), mx(1), mn(2), 0.0f);
    lines[14].set(mn(0), mx(1), mn(2), 0.0f);
    lines[15].set(mn(0), mx(1), mx(2), 0.0f);
    lines[16].set(mx(0), mx(1), mn(2), 0.0f);
    lines[17].set(mx(0), mn(1), mn(2), 0.0f);
    lines[18].set(mn(0), mx(1), mx(2), 0.0f);
    lines[19].set(mn(0), mn(1), mx(2), 0.0f);
    lines[20].set(mn(0), mn(1), mx(2), 0.0f);
    lines[21].set(mx(0), mn(1), mx(2), 0.0f);
    lines[22].set(mx(0), mn(1), mx(2), 0.0f);
    lines[23].set(mx(0), mn(1), mn(2), 0.0f);
    for (int32_t i = 0; i < 24; i += 2) {
        displayLine(lines[i], lines[i + 1], color, id, tag);
    }
}

void hkDebugDisplay::displayStar(__m128* a1, float a2, int32_t a3, int32_t a4, int32_t a5) {
    int32_t v6 = 0;
    __m128 v10 = _mm_setzero_ps();
    do {
        hkVector4 end;
        hkVector4 start;
        v10[v6] = a2;
        __m128 v7 = _mm_sub_ps(*a1, v10);
        start.m_quad = _mm_add_ps(*a1, v10);
        end.m_quad = v7;
        displayLine(start, end, a3, a4, a5);
        ++v6;
    }
    while (v6 < 3);
}

void hkDebugDisplay::displayStarModelSpace(
    const hkTransform& worldFromModel,
    const hkVector4& position,
    float scale,
    int32_t color,
    int32_t id,
    int32_t tag
) {
    hkVector4 a1;
    a1.setTransformedPos(worldFromModel, position);
    displayStar(&a1.m_quad, scale, color, id, tag);
}

void hkDebugDisplay::displayPlane(__m128* a1, __m128* a2, uint32_t a3, int32_t a4, int32_t a5, int32_t a6) {
    int32_t v10;
    uint32_t v30;
    hkVector4 v36;
    hkVector4 dir;
    __m128 v7 = *a1;
    *reinterpret_cast<float*>(&v30) = -(*a1)[3];
    __m128 v8 = _mm_add_ps(
        _mm_mul_ps(
            _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v30)), _mm_set_ss(std::bit_cast<float>(v30)), 0), *a1
        ),
        *a2
    );
    __m128 v9 = _mm_and_ps(*a1, hkVector4_setAbs4_absMask);
    __m128 v33 = v9;
    v36.m_quad = v8;
    if (!(v9[1] > static_cast<double>(v9[0]))) {
        if (!(v33[2] > static_cast<double>(v9[0]))) {
            v10 = 0;
        }
        else {
            v10 = 2;
        }
    }
    else {
        if (!(v33[2] > static_cast<double>(v9[1]))) {
            v10 = 1;
        }
        else {
            v10 = 2;
        }
    }
    v33 = _mm_setzero_ps();
    __m128 v11 = _mm_shuffle_ps(v7, v7, 201);
    __m128 v12 = _mm_shuffle_ps(v7, v7, 210);
    int32_t v13 = 1;
    int32_t v31 = 4;
    v33[(v10 + 1) % 3] = 1.0f;
    __m128 v14 =
        _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v33, v33, 210), v11), _mm_mul_ps(_mm_shuffle_ps(v33, v33, 201), v12));
    __m128 v15 = _mm_mul_ps(v14, v14);
    __m128 v16 = _mm_shuffle_ps(v15, v15, 170);
    __m128 v17 = _mm_set_ss(std::bit_cast<float>(a3));
    v16[0] = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss((v16[0] + (_mm_shuffle_ps(v15, v15, 85)[0] + v15[0])))));
    __m128 v18 = _mm_mul_ps(_mm_mul_ps(_mm_shuffle_ps(v16, v16, 0), v14), _mm_shuffle_ps(v17, v17, 0));
    __m128 v19 =
        _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(v18, v18, 201), v12), _mm_mul_ps(_mm_shuffle_ps(v18, v18, 210), v11));
    __m128 v20 = _mm_mul_ps(v19, v19);
    __m128 v21 = _mm_shuffle_ps(v20, v20, 170);
    v21[0] = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss((v21[0] + (_mm_shuffle_ps(v20, v20, 85)[0] + v20[0])))));
    __m128 m_quad = _mm_mul_ps(_mm_mul_ps(_mm_shuffle_ps(v21, v21, 0), v19), _mm_shuffle_ps(v17, v17, 0));
    v33 = v18;
    for (dir.m_quad = m_quad;; m_quad = dir.m_quad) {
        hkVector4 end;
        hkVector4 start;
        __m128 v23 = (static_cast<uint8_t>(v13) - 1 & 2) != 0 ? _mm_sub_ps(v8, m_quad) : _mm_add_ps(m_quad, v8);
        int32_t v24 = v13 % 4 / 2;
        __m128 v25 = v24 ? _mm_sub_ps(v23, v18) : _mm_add_ps(v23, v18);
        start.m_quad = v25;
        __m128 v26 = v24 ? _mm_sub_ps(v8, m_quad) : _mm_add_ps(m_quad, v8);
        int32_t v27 = v13 + 1;
        __m128 v28 = (v13 + 1) % 4 / 2 ? _mm_sub_ps(v26, v18) : _mm_add_ps(v26, v18);
        end.m_quad = v28;
        displayLine(start, end, a4, a5, a6);
        bool v29 = v31-- == 1;
        v13 = v27;
        if (v29) {
            break;
        }
        v8 = v36.m_quad;
        v18 = v33;
    }
    dir.m_quad =
        _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(a3)), _mm_set_ss(std::bit_cast<float>(a3)), 0), *a1);
    displayArrow(v36, dir, a4, a5, a6);
}

void hkDebugDisplay::displayText(const char* text, int32_t color, int32_t id, int32_t tag) {
    EnterCriticalSection(&m_arrayLock->m_section);
    for (int32_t i = 0; i < m_debugDisplayHandlers.m_size; ++i) {
        hkDebugDisplayHandler* v7 = m_debugDisplayHandlers.m_data[i];
        v7->displayText(text, color, id, tag);
    }
    LeaveCriticalSection(&m_arrayLock->m_section);
}

void hkDebugDisplay::display3dText(const char* text, const hkVector4& pos, int32_t color, int32_t id, int32_t tag) {
    EnterCriticalSection(&m_arrayLock->m_section);
    for (int32_t i = 0; i < m_debugDisplayHandlers.m_size; ++i) {
        hkDebugDisplayHandler* v8 = m_debugDisplayHandlers.m_data[i];
        v8->display3dText(text, pos, color, id, tag);
    }
    LeaveCriticalSection(&m_arrayLock->m_section);
}

void hkDebugDisplay::displayGeometry(
    const hkArrayBase<hkDisplayGeometry*>& geometries,
    const hkTransform& transform,
    int32_t color,
    int32_t id,
    int32_t tag
) {
    EnterCriticalSection(&m_arrayLock->m_section);
    for (int32_t i = 0; i < m_debugDisplayHandlers.m_size; ++i) {
        hkDebugDisplayHandler* v8 = m_debugDisplayHandlers.m_data[i];
        v8->displayGeometry(geometries, transform, color, id, tag);
    }
    LeaveCriticalSection(&m_arrayLock->m_section);
}

void hkDebugDisplay::displayGeometry(
    const hkArrayBase<hkDisplayGeometry*>& geometries,
    int32_t color,
    int32_t id,
    int32_t tag
) {
    EnterCriticalSection(&m_arrayLock->m_section);
    for (int32_t i = 0; i < m_debugDisplayHandlers.m_size; ++i) {
        hkDebugDisplayHandler* v7 = m_debugDisplayHandlers.m_data[i];
        v7->displayGeometry(geometries, color, id, tag);
    }
    LeaveCriticalSection(&m_arrayLock->m_section);
}

void hkDebugDisplay::clear() {
    EnterCriticalSection(&this->m_arrayLock->m_section);
    hkCriticalSection* m_arrayLock = this->m_arrayLock;
    m_debugDisplayHandlers.m_size = 0;
    LeaveCriticalSection(&m_arrayLock->m_section);
}

void hkDebugDisplay::displayFrame(hkTransform* a1, uint32_t a2, int32_t a3, int32_t a4) {
    hkQsTransform v6;
    v6.setFromTransform(*a1);
    displayFrameQs(this, a3, &v6, a2, a3, a4);
}

void displayFrameQs(hkDebugDisplay* a1, int32_t edx0, const hkQsTransform* a3, uint32_t a4, int32_t a5, int32_t a6) {
    __m128 v6 = _mm_undefined_ps();
    __m128 v7 = _mm_undefined_ps();
    hkVector4 a3a;
    hkVector4 v13;
    hkVector4 v14;
    hkVector4 v15;
    hkVector4 v16;
    hkVector4 dir;
    hkVector4 v18;
    hkVector4 v19;
    a3a.m_quad = _mm_setzero_ps();
    v13.setTransformedPos(*a3, a3a);
    v6[0] = 0.0f;
    v7[0] = 0.0f;
    __m128 v9 = _mm_unpacklo_ps(
        _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(a4)), v7),
        _mm_unpacklo_ps(v6, _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero))))
    );
    a3a.m_quad = v9;
    v14.setTransformedPos(*a3, a3a);
    v9[0] = 0.0f;
    v7[0] = 0.0f;
    __m128 v10 = _mm_unpacklo_ps(
        _mm_set_ss(std::bit_cast<float>(a4)), _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))
    );
    __m128 v11 = _mm_unpacklo_ps(_mm_unpacklo_ps(v9, v7), v10);
    a3a.m_quad = v11;
    v15.setTransformedPos(*a3, a3a);
    v11[0] = 0.0f;
    v10[0] = 0.0f;
    a3a.m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(v11, _mm_set_ss(std::bit_cast<float>(a4))),
        _mm_unpacklo_ps(v10, _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero))))
    );
    v16.setTransformedPos(*a3, a3a);
    dir.m_quad = _mm_sub_ps(v14.m_quad, v13.m_quad);
    v18.m_quad = _mm_sub_ps(v15.m_quad, v13.m_quad);
    v19.m_quad = _mm_sub_ps(v16.m_quad, v13.m_quad);
    a1->displayArrow(v13, dir, -0x10000, a5, a6);
    a1->displayArrow(v13, v18, -0xFF8000, a5, a6);
    a1->displayArrow(v13, v19, -0xFFFF01, a5, a6);
}

// hkReferencedObject's operator new (a heap block, its size recorded in the object), then the constructor.
hkReferencedObject* hkDebugDisplaycreate() {
    hkDebugDisplay* pDisplay =
        static_cast<hkDebugDisplay*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkDebugDisplay)));
    pDisplay->m_memSizeAndFlags = sizeof(hkDebugDisplay);
    return ::new (pDisplay) hkDebugDisplay;
}
