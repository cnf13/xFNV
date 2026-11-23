#include "Havok/Source/Common/Internal/GeometryProcessing/hkGeometryProcessing.h"

#include "BSCore/MemoryManager.h"
#include "Havok/Source/Common/Base/Algorithm/PseudoRandom/hkPseudoRandomGenerator.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4Util.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/QueryUtils/hkAabbTreeQueryUtils.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTree.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTreeData.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/AbstractMesh/hkgpAbstractMesh.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/Mesh/hkgpMesh.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/ExtendedMeshShape/hkpExtendedMeshShape.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpBuildJacobianFromAtoms.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpOverwritePivotConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpSetLocalRotationsConstraintAtom.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bridge/hkpBridgeAtoms.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Utilities/Dynamics/Inertia/hkpInertiaTensorComputer.h"
#include "SDK/MSVC/stdlib.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

using hkGeometryProcessing_ConstFunction_hkGeometryProcessing_IFunction_hkVector4_float = hkGeometryProcessing::
    ConstFunction<hkGeometryProcessing::IFunction<hkVector4, float>>;

void cleanupLoadedObjecthkpSetLocalTransformsConstraintAtom(void* p) {
    ;
}

void cleanupLoadedObjecthkpAngConstraintAtom(void* p) {
    ;
}

void cleanupLoadedObjecthkpTwistLimitConstraintAtom(void* p) {
    ;
}

void cleanupLoadedObjecthkpRagdollMotorConstraintAtom(void* p) {
    ;
}

void cleanupLoadedObjecthkpMassChangerModifierConstraintAtom(void* p) {
    ;
}

void cleanupLoadedObjecthkpCenterOfMassChangerModifierConstraintAtom(void* p) {
    ;
}

void finishLoadedObjecthkpStiffSpringConstraintAtom(void* p, int32_t finishing) {
    ;
}

void finishLoadedObjecthkpSetLocalTranslationsConstraintAtom(void* p, int32_t finishing) {
    ;
}

void finishLoadedObjecthkpLinSoftConstraintAtom(void* p, int32_t finishing) {
    ;
}

void finishLoadedObjecthkpAngMotorConstraintAtom(void* p, int32_t finishing) {
    ;
}

void finishLoadedObjecthkpLinMotorConstraintAtom(void* p, int32_t finishing) {
    ;
}

void finishLoadedObjecthkpPulleyConstraintAtom(void* p, int32_t finishing) {
    ;
}

void finishLoadedObjecthkpMassChangerModifierConstraintAtom(void* p, int32_t finishing) {
    ;
}

void finishLoadedObjecthkpViscousSurfaceModifierConstraintAtom(void* p, int32_t finishing) {
    ;
}

void finishLoadedObjecthkpIgnoreModifierConstraintAtom(void* p, int32_t finishing) {
    ;
}

void cleanupLoadedObjecthkpSetLocalRotationsConstraintAtom(void* p) {
    ;
}

void cleanupLoadedObjecthkpConeLimitConstraintAtom(void* p) {
    ;
}

void cleanupLoadedObjecthkpLinMotorConstraintAtom(void* p) {
    ;
}

void cleanupLoadedObjecthkpOverwritePivotConstraintAtom(void* p) {
    ;
}

void finishLoadedObjecthkpLinLimitConstraintAtom(void* p, int32_t finishing) {
    ;
}

void finishLoadedObjecthkpAngFrictionConstraintAtom(void* p, int32_t finishing) {
    ;
}

void finishLoadedObjecthkpRagdollMotorConstraintAtom(void* p, int32_t finishing) {
    ;
}

void cleanupLoadedObjecthkpPulleyConstraintAtom(void* p) {
    ;
}

void cleanupLoadedObjecthkpSoftContactModifierConstraintAtom(void* p) {
    ;
}

void cleanupLoadedObjecthkpViscousSurfaceModifierConstraintAtom(void* p) {
    ;
}

void finishLoadedObjecthkpSetLocalRotationsConstraintAtom(void* p, int32_t finishing) {
    ;
}

void finishLoadedObjecthkpConeLimitConstraintAtom(void* p, int32_t finishing) {
    ;
}

void cleanupLoadedObjecthkpLinSoftConstraintAtom(void* p) {
    ;
}

void cleanupLoadedObjecthkpLinLimitConstraintAtom(void* p) {
    ;
}

void cleanupLoadedObjecthkpMovingSurfaceModifierConstraintAtom(void* p) {
    ;
}

void finishLoadedObjecthkpConstraintAtom(void* p, int32_t finishing) {
    ;
}

void finishLoadedObjecthkpAngLimitConstraintAtom(void* p, int32_t finishing) {
    ;
}

void finishLoadedObjecthkpModifierConstraintAtom(void* p, int32_t finishing) {
    ;
}

void finishLoadedObjecthkpMovingSurfaceModifierConstraintAtom(void* p, int32_t finishing) {
    ;
}

void cleanupLoadedObjecthkpBallSocketConstraintAtom(void* p) {
    ;
}

void cleanupLoadedObjecthkpAngMotorConstraintAtom(void* p) {
    ;
}

void cleanupLoadedObjecthkpSimpleContactConstraintAtom(void* p) {
    ;
}

// The finish function the reflection registers for hkpSimpleContactConstraintAtom: finishes the
// contact atom it is given, when there is one.
void finishLoadedObjecthkpSimpleContactConstraintAtom(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag aFlag;
        aFlag.m_finishing = finishing;
        ::new (p) hkpSimpleContactConstraintAtom(aFlag);
    }
}

void cleanupLoadedObjecthkpStiffSpringConstraintAtom(void* p) {
    ;
}

void cleanupLoadedObjecthkpSetLocalTranslationsConstraintAtom(void* p) {
    ;
}

void cleanupLoadedObjecthkpLinConstraintAtom(void* p) {
    ;
}

void cleanupLoadedObjecthkp2dAngConstraintAtom(void* p) {
    ;
}

void cleanupLoadedObjecthkpAngLimitConstraintAtom(void* p) {
    ;
}

void cleanupLoadedObjecthkpAngFrictionConstraintAtom(void* p) {
    ;
}

void cleanupLoadedObjecthkpLinFrictionConstraintAtom(void* p) {
    ;
}

void cleanupLoadedObjecthkpModifierConstraintAtom(void* p) {
    ;
}

void cleanupLoadedObjecthkpIgnoreModifierConstraintAtom(void* p) {
    ;
}

void finishLoadedObjecthkpBridgeConstraintAtom(void* p, int32_t finishing) {
    hkpBridgeConstraintAtom* atom = static_cast<hkpBridgeConstraintAtom*>(p);
    if (atom && finishing) {
        atom->init(atom->m_constraintData);
    }
}

void finishLoadedObjecthkpBridgeAtoms(void* p, int32_t finishing) {
    hkpBridgeConstraintAtom* atom = static_cast<hkpBridgeConstraintAtom*>(p);
    if (atom && finishing) {
        atom->init(atom->m_constraintData);
    }
}

void finishLoadedObjecthkpBallSocketConstraintAtom(void* p, int32_t finishing) {
    ;
}

void finishLoadedObjecthkpOverwritePivotConstraintAtom(void* p, int32_t finishing) {
    ;
}

void finishLoadedObjecthkpLinConstraintAtom(void* p, int32_t finishing) {
    ;
}

void finishLoadedObjecthkp2dAngConstraintAtom(void* p, int32_t finishing) {
    ;
}

void finishLoadedObjecthkpAngConstraintAtom(void* p, int32_t finishing) {
    ;
}

void finishLoadedObjecthkpTwistLimitConstraintAtom(void* p, int32_t finishing) {
    ;
}

void finishLoadedObjecthkpLinFrictionConstraintAtom(void* p, int32_t finishing) {
    ;
}

void finishLoadedObjecthkpSoftContactModifierConstraintAtom(void* p, int32_t finishing) {
    ;
}

void finishLoadedObjecthkpCenterOfMassChangerModifierConstraintAtom(void* p, int32_t finishing) {
    ;
}

char intersectRayPlane(__m128* x0, __m128* x1, __m128* plane, float* toi) {
    __m128 v4 = *plane;
    __m128 v5 = _mm_mul_ps(_mm_sub_ps(*x1, *x0), *plane);
    float v6 = XFNV_M128_F32(_mm_shuffle_ps(v5, v5, 170), 0) +
        (XFNV_M128_F32(_mm_shuffle_ps(v5, v5, 85), 0) + XFNV_M128_F32(v5, 0));
    if (!((v6 * v6) > HK_REAL_EPSILON)) {
        return 0;
    }
    float v7 = XFNV_M128_F32(_mm_shuffle_ps(v4, v4, 255), 0);
    __m128 v8 = _mm_mul_ps(v4, *x0);
    *toi = (0.0f -
            ((XFNV_M128_F32(_mm_shuffle_ps(v8, v8, 170), 0) +
              (XFNV_M128_F32(_mm_shuffle_ps(v8, v8, 85), 0) + XFNV_M128_F32(v8, 0))) +
             v7)) /
        v6;
    return 1;
}

double squaredDistanceToSegment(__m128* p, __m128* p0, __m128* p1, float* toi) {
    __m128 v4 = _mm_sub_ps(*p0, *p);
    __m128 v5 = _mm_sub_ps(*p1, *p0);
    __m128 v6 = _mm_mul_ps(v5, v4);
    __m128 v7 = _mm_mul_ps(v5, v5);
    float v12 = (0.0f -
                 (XFNV_M128_F32(_mm_shuffle_ps(v6, v6, 170), 0) +
                  (XFNV_M128_F32(_mm_shuffle_ps(v6, v6, 85), 0) + XFNV_M128_F32(v6, 0)))) /
        (XFNV_M128_F32(_mm_shuffle_ps(v7, v7, 170), 0) +
         (XFNV_M128_F32(_mm_shuffle_ps(v7, v7, 85), 0) + XFNV_M128_F32(v7, 0)));
    *toi = v12;
    if (v12 < 0.0f) {
        *toi = 0.0f;
    }
    else if (v12 > 1.0f) {
        *toi = 1.0f;
    }
    __m128 v9 = _mm_add_ps(
        _mm_mul_ps(
            _mm_shuffle_ps(
                _mm_set_ss(std::bit_cast<float>(*reinterpret_cast<uint32_t*>(toi))),
                _mm_set_ss(std::bit_cast<float>(*reinterpret_cast<uint32_t*>(toi))),
                0
            ),
            v5
        ),
        v4
    );
    __m128 v10 = _mm_mul_ps(v9, v9);
    return (
        XFNV_M128_F32(_mm_shuffle_ps(v10, v10, 170), 0) +
        (XFNV_M128_F32(_mm_shuffle_ps(v10, v10, 85), 0) + XFNV_M128_F32(v10, 0))
    );
}

__m128* normalToOctahedron(float* normal, __m128* uv) {
    __m128 v2 = _mm_undefined_ps();
    float v20;
    uint32_t v21;
    float v24;
    uint32_t v27;
    double v3 = hkMath_atan2fApproximation(normal[1], normal[0]);
    float v19 = (v3 + v3) / 3.1415927410125732;
    double v4 = normal[2];
    if (!(v4 < 0.0)) {
        double v11;
        float v25 = 1.0 - v4;
        float v26 = std::sqrt(static_cast<double>(v25));
        double v8 = v19;
        if (!(v19 >= 0.0f)) {
            if (!(v8 > -1.0f)) {
                double v16 = (v8 + 2.0) * -v26;
                double v17 = v26;
                v24 = v16;
                v11 = -(v17 + v24);
            }
            else {
                double v14 = v8 * v26;
                double v15 = v26;
                v24 = v14;
                v11 = v15 + v24;
            }
        }
        else {
            if (!(v8 < 1.0)) {
                double v12 = (2.0 - v8) * v26;
                double v13 = v26;
                v24 = v12;
                v11 = v24 - v13;
            }
            else {
                double v9 = v8 * v26;
                double v10 = v26;
                v24 = v9;
                v11 = v10 - v24;
            }
        }
        v20 = v11;
    }
    else {
        float v22 = v4 + 1.0;
        float v23 = std::sqrt(static_cast<double>(v22));
        double v5 = v19;
        if (!(v19 >= 0.0f)) {
            double v7;
            if (!(v5 >= -1.0f)) {
                v20 = (v5 + 2.0) * v23 - 1.0;
                v7 = v23 - 2.0 - v20;
            }
            else {
                v20 = v5 * v23 + 1.0;
                v7 = v23 - 2.0 + v20;
            }
            v24 = v7;
        }
        else {
            double v6;
            if (!(v5 <= 1.0)) {
                v20 = (2.0 - v5) * v23 - 1.0;
                v6 = 2.0 - v23 + v20;
            }
            else {
                v20 = 1.0 - v5 * v23;
                v6 = 2.0 - v23 - v20;
            }
            v24 = v6;
        }
    }
    XFNV_M128_F32(v2, 0) = 0.0f;
    *reinterpret_cast<float*>(&v21) = (v20 + 1.0) * 0.5;
    *reinterpret_cast<float*>(&v27) = 0.5 * (v24 + 1.0);
    *uv = _mm_unpacklo_ps(
        _mm_unpacklo_ps(_mm_set_ss(std::bit_cast<float>(v21)), v2),
        _mm_unpacklo_ps(
            _mm_set_ss(std::bit_cast<float>(v27)),
            _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)))
        )
    );
    return uv;
}

__m128* computeMidpoint(__m128* result, __m128* a, __m128* b) {
    __m128 v4 = _mm_shuffle_ps(_mm_set_ss(fHalf), _mm_set_ss(fHalf), 0);
    *result = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, v4), *a), _mm_mul_ps(*b, v4));
    return result;
}

__m128* computeTriangleCentroid(__m128* result, __m128* a, __m128* b, __m128* c) {
    *result = _mm_mul_ps(
        _mm_add_ps(_mm_add_ps(*a, *b), *c),
        _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(0x3EAAAAABu)), _mm_set_ss(std::bit_cast<float>(0x3EAAAAABu)), 0)
    );
    return result;
}

__m128* computeQuadCentroid(__m128* result, __m128* a, __m128* b, __m128* c, __m128* d) {
    *result = _mm_mul_ps(
        _mm_add_ps(_mm_add_ps(*c, *d), _mm_add_ps(*a, *b)),
        _mm_shuffle_ps(_mm_set_ss(fQuadCentroidWeight), _mm_set_ss(fQuadCentroidWeight), 0)
    );
    return result;
}

double squaredDistanceToEdge(__m128* p, __m128* p0, __m128* p1, __m128* normalOut, float* baryOut) {
    double v5 = 0.0;
    __m128 v6 = _mm_sub_ps(*p0, *p);
    __m128 v7 = _mm_sub_ps(*p1, *p0);
    __m128 v8 = _mm_mul_ps(v7, v6);
    __m128 v9 = _mm_mul_ps(v7, v7);
    float v13 = (0.0f -
                 (XFNV_M128_F32(_mm_shuffle_ps(v8, v8, 170), 0) +
                  (XFNV_M128_F32(_mm_shuffle_ps(v8, v8, 85), 0) + XFNV_M128_F32(v8, 0)))) /
        (XFNV_M128_F32(_mm_shuffle_ps(v9, v9, 170), 0) +
         (XFNV_M128_F32(_mm_shuffle_ps(v9, v9, 85), 0) + XFNV_M128_F32(v9, 0)));
    if (!(v13 < 0.0f)) {
        v5 = v13;
        if (v13 > 1.0f) {
            v5 = 1.0;
        }
    }
    float v14 = v5;
    __m128 v10 = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(v14), _mm_set_ss(v14), 0), v7), v6);
    *normalOut = _mm_mul_ps(
        _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(0xBF800000u)), _mm_set_ss(std::bit_cast<float>(0xBF800000u)), 0),
        v10
    );
    if (baryOut) {
        *baryOut = v14;
    }
    __m128 v11 = _mm_mul_ps(v10, v10);
    return (
        XFNV_M128_F32(_mm_shuffle_ps(v11, v11, 170), 0) +
        (XFNV_M128_F32(_mm_shuffle_ps(v11, v11, 85), 0) + XFNV_M128_F32(v11, 0))
    );
}

// The point p's closest point on the triangle p0, p1, p2: the triangle's plane normal into normalOut and, when baryOut
// is given, the barycentric weights into baryOut; returns the squared distance. A projection outside the triangle falls
// back to the closest of its three edges (squaredDistanceToEdge each).
double squaredDistanceToTriangle(hkVector4* p, __m128* p0, __m128* p1, __m128* p2, __m128* normalOut, __m128* baryOut) {
    __m128 v15 = _mm_undefined_ps();
    int32_t n2;
    float afEdgeDist[3];
    float afEdgeParam[3];
    __m128 akEdgePoint[3];
    __m128 v6 = *p1;
    __m128 v55 = *p2;
    __m128 v7 = *p0;
    __m128 v8 = _mm_sub_ps(v55, *p0);
    __m128 v9 = _mm_sub_ps(v6, *p0);
    __m128 v10 = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(v8, v8, 210), _mm_shuffle_ps(v9, v9, 201)),
        _mm_mul_ps(_mm_shuffle_ps(v8, v8, 201), _mm_shuffle_ps(v9, v9, 210))
    );
    __m128 v11 = _mm_mul_ps(v10, v10);
    float v12 = XFNV_M128_F32(_mm_shuffle_ps(v11, v11, 85), 0) + XFNV_M128_F32(v11, 0);
    __m128 v13 = _mm_shuffle_ps(v11, v11, 170);
    XFNV_M128_F32(v13, 0) = XFNV_M128_F32(v13, 0) + v12;
    float v14 = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(XFNV_M128_F32(v13, 0))));
    XFNV_M128_F32(v15, 0) = zero;
    __m128 v16 = _mm_cmpeq_ss(v13, v15);
    XFNV_M128_F32(v15, 0) = 3.0f - ((XFNV_M128_F32(v13, 0) * v14) * v14);
    __m128 v17 = _mm_setzero_ps();
    XFNV_M128_F32(v17, 0) = 0.5f * v14;
    __m128 m_quad = p->m_quad;
    XFNV_M128_F32(v17, 0) = XFNV_M128_F32(v17, 0) * XFNV_M128_F32(v15, 0);
    __m128 v19 = _mm_andnot_ps(v16, v17);
    __m128 v56 = _mm_mul_ps(_mm_shuffle_ps(v19, v19, 0), v10);
    __m128 v20 = _mm_mul_ps(v56, v7);
    XFNV_M128_F32(v56, 3) = 0.0f -
        (XFNV_M128_F32(_mm_shuffle_ps(v20, v20, 170), 0) +
         (XFNV_M128_F32(_mm_shuffle_ps(v20, v20, 85), 0) + XFNV_M128_F32(v20, 0)));
    __m128 v21 = _mm_mul_ps(v56, m_quad);
    double v22 =
        ((XFNV_M128_F32(_mm_shuffle_ps(v21, v21, 170), 0) +
          (XFNV_M128_F32(_mm_shuffle_ps(v21, v21, 85), 0) + XFNV_M128_F32(v21, 0))) +
         XFNV_M128_F32(_mm_shuffle_ps(v56, v56, 255), 0));
    // The negated distance is rounded to float before the SSE code reads it.
    const float fNegDist = static_cast<float>(-v22);
    __m128 v23 = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(fNegDist), _mm_set_ss(fNegDist), 0), v56), m_quad);
    __m128 v24 = _mm_sub_ps(v7, v23);
    __m128 v25 = _mm_sub_ps(v6, v23);
    __m128 v26 = _mm_mul_ps(
        _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(v25, v25, 210), _mm_shuffle_ps(v24, v24, 201)),
            _mm_mul_ps(_mm_shuffle_ps(v25, v25, 201), _mm_shuffle_ps(v24, v24, 210))
        ),
        v56
    );
    if ((XFNV_M128_F32(_mm_shuffle_ps(v26, v26, 170), 0) +
         (XFNV_M128_F32(_mm_shuffle_ps(v26, v26, 85), 0) + XFNV_M128_F32(v26, 0))) > 0.0f) {
        __m128 v27 = _mm_sub_ps(v55, v23);
        __m128 v28 = _mm_mul_ps(
            _mm_sub_ps(
                _mm_mul_ps(_mm_shuffle_ps(v27, v27, 210), _mm_shuffle_ps(v25, v25, 201)),
                _mm_mul_ps(_mm_shuffle_ps(v27, v27, 201), _mm_shuffle_ps(v25, v25, 210))
            ),
            v56
        );
        v25 = _mm_shuffle_ps(v28, v28, 85);
        if ((XFNV_M128_F32(_mm_shuffle_ps(v28, v28, 170), 0) + (XFNV_M128_F32(v25, 0) + XFNV_M128_F32(v28, 0))) >
            0.0f) {
            __m128 v29 = _mm_mul_ps(
                _mm_sub_ps(
                    _mm_mul_ps(_mm_shuffle_ps(v24, v24, 210), _mm_shuffle_ps(v27, v27, 201)),
                    _mm_mul_ps(_mm_shuffle_ps(v24, v24, 201), _mm_shuffle_ps(v27, v27, 210))
                ),
                v56
            );
            v24 = _mm_shuffle_ps(v29, v29, 85);
            v25 = _mm_shuffle_ps(v29, v29, 170);
            if ((XFNV_M128_F32(v25, 0) + (XFNV_M128_F32(v24, 0) + XFNV_M128_F32(v29, 0))) > 0.0f) {
                *normalOut = v56;
                if (baryOut) {
                    __m128 v46;
                    __m128 v30 = _mm_sub_ps(*p0, v23);
                    __m128 v31 = _mm_sub_ps(*p1, v23);
                    __m128 v32 = _mm_sub_ps(
                        _mm_mul_ps(_mm_shuffle_ps(v31, v31, 210), _mm_shuffle_ps(v30, v30, 201)),
                        _mm_mul_ps(_mm_shuffle_ps(v31, v31, 201), _mm_shuffle_ps(v30, v30, 210))
                    );
                    __m128 v33 = _mm_mul_ps(v32, v32);
                    __m128 v34 = _mm_shuffle_ps(v33, v33, 85);
                    XFNV_M128_F32(v34, 0) = _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(
                        (XFNV_M128_F32(_mm_shuffle_ps(v33, v33, 170), 0) +
                         (XFNV_M128_F32(v34, 0) + XFNV_M128_F32(v33, 0)))
                    )));
                    __m128 v35 = _mm_sub_ps(*p2, v23);
                    __m128 v36 = _mm_sub_ps(
                        _mm_mul_ps(_mm_shuffle_ps(v30, v30, 210), _mm_shuffle_ps(v35, v35, 201)),
                        _mm_mul_ps(_mm_shuffle_ps(v30, v30, 201), _mm_shuffle_ps(v35, v35, 210))
                    );
                    __m128 v37 = _mm_mul_ps(v36, v36);
                    XFNV_M128_F32(v30, 0) = _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(
                        (XFNV_M128_F32(_mm_shuffle_ps(v37, v37, 170), 0) +
                         (XFNV_M128_F32(_mm_shuffle_ps(v37, v37, 85), 0) + XFNV_M128_F32(v37, 0)))
                    )));
                    __m128 v38 = _mm_sub_ps(
                        _mm_mul_ps(_mm_shuffle_ps(v35, v35, 210), _mm_shuffle_ps(v31, v31, 201)),
                        _mm_mul_ps(_mm_shuffle_ps(v35, v35, 201), _mm_shuffle_ps(v31, v31, 210))
                    );
                    __m128 v39 = _mm_mul_ps(v38, v38);
                    __m128 v40 = _mm_shuffle_ps(v39, v39, 170);
                    XFNV_M128_F32(v40, 0) = _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss((
                        XFNV_M128_F32(v40, 0) + (XFNV_M128_F32(_mm_shuffle_ps(v39, v39, 85), 0) + XFNV_M128_F32(v39, 0))
                    ))));
                    // The weights of p0, p1 and p2: the areas opposite them (v40, v30 and v34), then 0.
                    __m128 v42 = _mm_unpacklo_ps(
                        _mm_unpacklo_ps(_mm_set_ss(XFNV_M128_F32(v40, 0)), _mm_set_ss(XFNV_M128_F32(v34, 0))),
                        _mm_unpacklo_ps(_mm_set_ss(XFNV_M128_F32(v30, 0)), _mm_set_ss(zero))
                    );
                    __m128 v43 = _mm_shuffle_ps(v42, v42, 85);
                    __m128 v44 = _mm_shuffle_ps(v42, v42, 170);
                    *baryOut = v42;
                    XFNV_M128_F32(v44, 0) = XFNV_M128_F32(v44, 0) + (XFNV_M128_F32(v43, 0) + XFNV_M128_F32(v42, 0));
                    if (!(XFNV_M128_F32(v44, 0) > HK_REAL_EPSILON)) {
                        XFNV_M128_F32(v44, 0) = 0.33333334f;
                        XFNV_M128_F32(v43, 0) = 0.33333334f;
                        v46 = _mm_unpacklo_ps(
                            _mm_unpacklo_ps(v44, _mm_set_ss(0.33333334f)),
                            _mm_unpacklo_ps(v43, _mm_set_ss(zero))
                        );
                    }
                    else {
                        __m128 kInvSum = _mm_set_ss(1.0f);
                        XFNV_M128_F32(kInvSum, 0) = 1.0f / XFNV_M128_F32(v44, 0);
                        v46 = _mm_mul_ps(_mm_shuffle_ps(kInvSum, kInvSum, 0), v42);
                    }
                    *baryOut = v46;
                }
                if (v22 < 0.0f) {
                    *normalOut = _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(-1.0f), _mm_set_ss(-1.0f), 0), *normalOut);
                }
                return static_cast<float>(v22 * v22);
            }
        }
    }
    afEdgeDist[0] = squaredDistanceToEdge(&p->m_quad, p0, p1, &akEdgePoint[0], &afEdgeParam[0]);
    afEdgeDist[1] = squaredDistanceToEdge(&p->m_quad, p1, p2, &akEdgePoint[1], &afEdgeParam[1]);
    afEdgeDist[2] = squaredDistanceToEdge(&p->m_quad, p2, p0, &akEdgePoint[2], &afEdgeParam[2]);
    if (!(afEdgeDist[1] > static_cast<double>(afEdgeDist[0]))) {
        n2 = 1;
        if (!(afEdgeDist[2] > static_cast<double>(afEdgeDist[1]))) {
            n2 = 2;
        }
    }
    else if (!(afEdgeDist[2] > static_cast<double>(afEdgeDist[0]))) {
        n2 = 2;
    }
    else {
        n2 = 0;
    }
    *normalOut = akEdgePoint[n2];
    if (baryOut) {
        // Each 1 - t is rounded to float before the SSE code reads it.
        __m128 v49;
        if (n2) {
            v49 = _mm_set_ss(zero);
            if (n2 == 1) {
                v25 = _mm_set_ss(afEdgeParam[1]);
                XFNV_M128_F32(v23, 0) = 0.0f;
                const float fRest = static_cast<float>(1.0 - afEdgeParam[1]);
                v24 = _mm_set_ss(fRest);
            }
            else {
                v23 = _mm_set_ss(afEdgeParam[2]);
                XFNV_M128_F32(v24, 0) = 0.0f;
                const float fRest = static_cast<float>(1.0 - afEdgeParam[2]);
                v25 = _mm_set_ss(fRest);
            }
        }
        else {
            v49 = _mm_set_ss(zero);
            v24 = _mm_set_ss(afEdgeParam[0]);
            XFNV_M128_F32(v25, 0) = 0.0f;
            const float fRest = static_cast<float>(1.0 - afEdgeParam[0]);
            v23 = _mm_set_ss(fRest);
        }
        *baryOut = _mm_unpacklo_ps(_mm_unpacklo_ps(v23, v25), _mm_unpacklo_ps(v24, v49));
    }
    return afEdgeDist[n2];
}

double nearestDistanceBetweenLines(__m128* lineA0, __m128* lineA1, __m128* lineB0, __m128* lineB1, float* u, float* v) {
    __m128 v6 = _mm_sub_ps(*lineB1, *lineB0);
    __m128 v7 = _mm_sub_ps(*lineB0, *lineA0);
    __m128 v8 = _mm_sub_ps(*lineA1, *lineA0);
    __m128 v9 = _mm_mul_ps(v6, v8);
    float v10 = XFNV_M128_F32(_mm_shuffle_ps(v9, v9, 170), 0) +
        (XFNV_M128_F32(_mm_shuffle_ps(v9, v9, 85), 0) + XFNV_M128_F32(v9, 0));
    __m128 v11 = _mm_mul_ps(v7, v8);
    double v12 = v10;
    double v13 = v10;
    float v14 = XFNV_M128_F32(_mm_shuffle_ps(v11, v11, 170), 0) +
        (XFNV_M128_F32(_mm_shuffle_ps(v11, v11, 85), 0) + XFNV_M128_F32(v11, 0));
    __m128 v15 = _mm_mul_ps(v6, v7);
    float v34 = XFNV_M128_F32(_mm_shuffle_ps(v15, v15, 170), 0) +
        (XFNV_M128_F32(_mm_shuffle_ps(v15, v15, 85), 0) + XFNV_M128_F32(v15, 0));
    __m128 v16 = _mm_mul_ps(v8, v8);
    float v31 = XFNV_M128_F32(_mm_shuffle_ps(v16, v16, 170), 0) +
        (XFNV_M128_F32(_mm_shuffle_ps(v16, v16, 85), 0) + XFNV_M128_F32(v16, 0));
    __m128 v17 = _mm_mul_ps(v6, v6);
    float v32 = v31 *
            (XFNV_M128_F32(_mm_shuffle_ps(v17, v17, 170), 0) +
             (XFNV_M128_F32(_mm_shuffle_ps(v17, v17, 85), 0) + XFNV_M128_F32(v17, 0))) -
        v13 * v13;
    double v18 = v32;
    float v33 = std::fabs(static_cast<double>(v32));
    if (!(v33 > HK_REAL_EPSILON)) {
        *v = 0.0f;
        *u = 0.0f;
        return HK_REAL_MAX;
    }

    double v20;
    double v21;
    float v29;
    float v35 = v31 * v34 - v12 * v14;
    float v36 = -v35 / v18;
    double v19 = 1.0;
    if (!(v36 < 0.0f)) {
        if (!(v36 > 1.0f)) {
            v19 = v36;
        }
        v29 = v19;
        v20 = 0.0;
        v21 = 1.0;
    }
    else {
        v20 = 0.0;
        v21 = 1.0;
        v29 = 0.0f;
    }
    *v = v29;
    __m128 v22 = _mm_add_ps(
        _mm_mul_ps(
            _mm_shuffle_ps(
                _mm_set_ss(std::bit_cast<float>(*reinterpret_cast<uint32_t*>(v))),
                _mm_set_ss(std::bit_cast<float>(*reinterpret_cast<uint32_t*>(v))),
                0
            ),
            v6
        ),
        *lineB0
    );
    __m128 v23 = _mm_mul_ps(_mm_sub_ps(*lineA0, v22), v8);
    float v24 = (XFNV_M128_F32(_mm_shuffle_ps(v23, v23, 170), 0) +
                 (XFNV_M128_F32(_mm_shuffle_ps(v23, v23, 85), 0) + XFNV_M128_F32(v23, 0))) /
        (XFNV_M128_F32(_mm_shuffle_ps(v16, v16, 170), 0) +
         (XFNV_M128_F32(_mm_shuffle_ps(v16, v16, 85), 0) + XFNV_M128_F32(v16, 0)));
    double v25 = (0.0f - v24);
    if (v25 < v20) {
        v21 = v20;
    }
    else {
        v20 = (0.0f - v24);
        if (!(v25 > v21)) {
            v21 = v20;
        }
    }
    float v30 = v21;
    *u = v30;
    __m128 v26 = _mm_sub_ps(
        _mm_add_ps(
            _mm_mul_ps(
                _mm_shuffle_ps(
                    _mm_set_ss(std::bit_cast<float>(*reinterpret_cast<uint32_t*>(u))),
                    _mm_set_ss(std::bit_cast<float>(*reinterpret_cast<uint32_t*>(u))),
                    0
                ),
                v8
            ),
            *lineA0
        ),
        v22
    );
    __m128 v27 = _mm_mul_ps(v26, v26);
    return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(
        (XFNV_M128_F32(_mm_shuffle_ps(v27, v27, 170), 0) +
         (XFNV_M128_F32(_mm_shuffle_ps(v27, v27, 85), 0) + XFNV_M128_F32(v27, 0)))
    )));
}

bool isPointInsideTriangle(__m128* p, __m128* a, __m128* b, __m128* c, float epsilon) {
    __m128 v5 = _mm_sub_ps(*b, *a);
    __m128 v6 = _mm_sub_ps(*c, *a);
    __m128 v7 = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(v6, v6, 210), _mm_shuffle_ps(v5, v5, 201)),
        _mm_mul_ps(_mm_shuffle_ps(v6, v6, 201), _mm_shuffle_ps(v5, v5, 210))
    );
    __m128 v8 = _mm_sub_ps(*a, *p);
    __m128 v9 = _mm_sub_ps(*b, *p);
    __m128 v10 = _mm_mul_ps(
        _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(v9, v9, 210), _mm_shuffle_ps(v8, v8, 201)),
            _mm_mul_ps(_mm_shuffle_ps(v9, v9, 201), _mm_shuffle_ps(v8, v8, 210))
        ),
        v7
    );
    double v11 = epsilon;
    bool result = false;
    if (!(epsilon > static_cast<double>(
                        (XFNV_M128_F32(_mm_shuffle_ps(v10, v10, 170), 0) +
                         (XFNV_M128_F32(_mm_shuffle_ps(v10, v10, 85), 0) + XFNV_M128_F32(v10, 0)))
                    ))) {
        __m128 v13 = _mm_sub_ps(*c, *p);
        __m128 v14 = _mm_mul_ps(
            _mm_sub_ps(
                _mm_mul_ps(_mm_shuffle_ps(v13, v13, 210), _mm_shuffle_ps(v9, v9, 201)),
                _mm_mul_ps(_mm_shuffle_ps(v13, v13, 201), _mm_shuffle_ps(v9, v9, 210))
            ),
            v7
        );
        if (!((XFNV_M128_F32(_mm_shuffle_ps(v14, v14, 170), 0) +
               (XFNV_M128_F32(_mm_shuffle_ps(v14, v14, 85), 0) + XFNV_M128_F32(v14, 0))) < v11)) {
            __m128 v15 = _mm_mul_ps(
                _mm_sub_ps(
                    _mm_mul_ps(_mm_shuffle_ps(v8, v8, 210), _mm_shuffle_ps(v13, v13, 201)),
                    _mm_mul_ps(_mm_shuffle_ps(v8, v8, 201), _mm_shuffle_ps(v13, v13, 210))
                ),
                v7
            );
            if (!((XFNV_M128_F32(_mm_shuffle_ps(v15, v15, 170), 0) +
                   (XFNV_M128_F32(_mm_shuffle_ps(v15, v15, 85), 0) + XFNV_M128_F32(v15, 0))) < v11)) {
                return true;
            }
        }
    }
    return result;
}

int32_t hkGeometryProcessing::IsoVertex::solveTriangle(
    const float* const values,
    hkGeometryProcessing::IsoVertex* const verticesOut,
    float orientation
) {
    double v15;
    hkGeometryProcessing::IsoVertex* v17;
    float v18;
    int32_t v19;
    double v20;
    double v3 = 0.0;
    float v26 = 0.0f;
    float v27 = 0.0f;
    float v28 = 0.0f;
    int32_t v4 = 0;
    double v5 = 1.0;
    if (values[0] != values[2] && values[2] * values[0] <= 0.0) {
        double v6;
        double v7;
        v4 = 4;
        float v21 = values[2] / (values[2] - values[0]);
        if (!(v21 < 0.0f)) {
            if (v21 > 1.0f) {
                v21 = 1.0f;
            }
            v6 = 0.0;
            v7 = 1.0;
        }
        else {
            v6 = 0.0;
            v7 = 1.0;
            v21 = 0.0f;
        }
        v28 = v21;
        v5 = v7;
        v3 = v6;
    }
    if (values[1] != values[0] && values[0] * values[1] <= 0.0) {
        double v10;
        double v11;
        v4 |= 1u;
        float v22 = values[0] / (values[0] - values[1]);
        if (!(v22 < v3)) {
            if (v22 > v5) {
                v22 = v5;
            }
            double v12 = v5;
            v10 = v3;
            v11 = v12;
        }
        else {
            double v9 = v5;
            v10 = v3;
            v11 = v9;
            v22 = v10;
        }
        v26 = v22;
        v5 = v11;
        v3 = v10;
    }
    if (values[2] == values[1] || !(values[2] * values[1] <= 0.0)) {
        v15 = 0.0;
    }
    else {
        v4 |= 2u;
        float v23 = values[1] / (values[1] - values[2]);
        v15 = 0.0;
        if (v23 < v3) {
            v27 = static_cast<float>(v3);
        }
        else {
            v3 = v23;
            if (!(v5 < v23)) {
                v27 = static_cast<float>(v3);
            }
            else {
                v27 = static_cast<float>(v5);
            }
        }
    }
    switch (v4) {
        case 3: {
            v17 = verticesOut;
            v18 = v26;
            v19 = 0;
            verticesOut[1].m_edge = 1;
            verticesOut[0].m_edge = 0;
            verticesOut[0].m_solution = v26;
            verticesOut[1].m_solution = v27;
            v20 = values[1];
            break;
        }
        case 5: {
            v17 = verticesOut;
            v18 = v28;
            v19 = 2;
            verticesOut[1].m_edge = 0;
            verticesOut[0].m_edge = 2;
            verticesOut[0].m_solution = v28;
            verticesOut[1].m_solution = v26;
            v20 = values[0];
            break;
        }
        case 6: {
            v17 = verticesOut;
            v18 = v27;
            v19 = 1;
            verticesOut[1].m_edge = 2;
            verticesOut[0].m_edge = 1;
            verticesOut[0].m_solution = v27;
            verticesOut[1].m_solution = v28;
            v20 = values[2];
            break;
        }
        default: {
            return 0;
        }
    }
    if (v20 * orientation < v15) {
        v17[0].m_edge = v17[1].m_edge;
        v17[0].m_solution = v17[1].m_solution;
        v17[1].m_edge = v19;
        v17[1].m_solution = v18;
    }
    return v4;
}

// The planes' normals are the rows of a matrix (its columns hold the three x, y and z, w zero) and the point solves
// matrix * point = -(p0.w, p1.w, p2.w). Each right-hand side is 0 minus the plane's w. The matrix is inverted with
// hkMatrix3::invert(1.1920929e-07f), whose nonzero hkResult returns false, and the point is col0 * x + col1 * y + col2
// * z on all four lanes.
bool hkGeometryProcessing::intersectPlanes(
    const hkVector4& p0,
    const hkVector4& p1,
    const hkVector4& p2,
    hkVector4& intersection
) {
    const __m128 kP0 = _mm_load_ps(&reinterpret_cast<const float*>(&p0.m_quad)[0]);
    const __m128 kP1 = _mm_load_ps(&reinterpret_cast<const float*>(&p1.m_quad)[0]);
    const __m128 kP2 = _mm_load_ps(&reinterpret_cast<const float*>(&p2.m_quad)[0]);
    const float fRhs0 = 0.0f - XFNV_M128_F32(p0.m_quad, 3);
    const float fRhs1 = 0.0f - XFNV_M128_F32(p1.m_quad, 3);
    const float fRhs2 = 0.0f - XFNV_M128_F32(p2.m_quad, 3);
    const __m128 kRhs = _mm_set_ps(0.0f, fRhs2, fRhs1, fRhs0);

    // Transpose, with the w lanes taken from zero.
    const __m128 kZero = _mm_setzero_ps();
    const __m128 kXY02 = _mm_unpacklo_ps(kP0, kP2);
    const __m128 kXY1 = _mm_unpacklo_ps(kP1, kZero);
    const __m128 kZW02 = _mm_unpackhi_ps(kP0, kP2);
    const __m128 kZW1 = _mm_unpackhi_ps(kP1, kZero);
    hkMatrix3 m;
    _mm_store_ps(&reinterpret_cast<float*>(&m.m_col0.m_quad)[0], _mm_unpacklo_ps(kXY02, kXY1));
    _mm_store_ps(&reinterpret_cast<float*>(&m.m_col1.m_quad)[0], _mm_unpackhi_ps(kXY02, kXY1));
    _mm_store_ps(&reinterpret_cast<float*>(&m.m_col2.m_quad)[0], _mm_unpacklo_ps(kZW02, kZW1));
    if (m.invert(HK_REAL_EPSILON)) {
        return false;
    }

    const __m128 kX = _mm_mul_ps(
        _mm_shuffle_ps(kRhs, kRhs, 0x00),
        _mm_load_ps(&reinterpret_cast<float*>(&m.m_col0.m_quad)[0])
    );
    const __m128 kY = _mm_mul_ps(
        _mm_shuffle_ps(kRhs, kRhs, 0x55),
        _mm_load_ps(&reinterpret_cast<float*>(&m.m_col1.m_quad)[0])
    );
    const __m128 kZ = _mm_mul_ps(
        _mm_shuffle_ps(kRhs, kRhs, 0xAA),
        _mm_load_ps(&reinterpret_cast<float*>(&m.m_col2.m_quad)[0])
    );
    _mm_store_ps(&reinterpret_cast<float*>(&intersection.m_quad)[0], _mm_add_ps(_mm_add_ps(kX, kY), kZ));
    return true;
}

// u and v are mapped to x = 2u - 1 and y = 2v - 1, and each octant gives the ring r, the hemisphere's sign and the
// position t (0 to 4) around the ring; every intermediate value is a float. The compares are: y >= 0 (NaN takes the y <
// 0 side), then x >= 0 (NaN: the x < 0 side) for y >= 0, x < 0 (NaN: the x >= 0 side) for y < 0, then x + y <= 1 or y -
// x <= 1 (NaN: the outer case), x + y >= -1 or y - x >= -1 (NaN: the outer case). The angle is t times
// 1.5707963705062866 unless r equals 0 (NaN multiplies). The sqrt, cos and sin results are each stored as a float.
void hkGeometryProcessing::octahedronToNormal(const hkVector4& uv, hkVector4& normal) {
    const float fX = XFNV_M128_F32(uv.m_quad, 0) * 2.0 - 1.0;
    const float fY = XFNV_M128_F32(uv.m_quad, 1) * 2.0 - 1.0;
    const float fSum = fY + fX;
    const float fDiff = fY - fX;
    float fR;
    float fSign;
    float fT;
    if (fY >= 0.0f) {
        if (fX >= 0.0f) {
            if (fSum <= 1.0f) {
                fR = fSum;
                fSign = 1.0f;
                fT = fY / fSum;
            }
            else {
                fR = 2.0 - fSum;
                fSign = -1.0f;
                fT = (1.0 - fX) / fR;
            }
        }
        else {
            if (fDiff <= 1.0f) {
                fR = fDiff;
                fSign = 1.0f;
                fT = 1.0 - fX / fDiff;
            }
            else {
                fR = 2.0 - fDiff;
                fSign = -1.0f;
                fT = 1.0 + (1.0 - fY) / fR;
            }
        }
    }
    else if (fX < 0.0f) {
        if (fSum >= -1.0f) {
            fR = -fSum;
            fSign = 1.0f;
            fT = 2.0 - fY / fR;
        }
        else {
            fR = fSum + 2.0;
            fSign = -1.0f;
            fT = (1.0 + fX) / fR + 2.0;
        }
    }
    else {
        if (fDiff >= -1.0f) {
            fR = -fDiff;
            fSign = 1.0f;
            fT = fX / fR + 3.0;
        }
        else {
            fR = fDiff + 2.0;
            fSign = -1.0f;
            fT = (1.0 + fY) / fR + 3.0;
        }
    }

    const float fAngle = (fR == 0.0f) ? 0.0f : static_cast<float>(fT * 1.5707963705062866);
    const float fR2 = fR * fR;
    const float fRadicand = 2.0 - fR2;
    // A negative radicand takes the CRT's error path.
    const float fRoot = std::sqrt(fRadicand);
    const float fScale = fRoot * fR;
    const float fCos = std::cos(fAngle);
    const float fNormalX = fCos * fScale;
    const float fSin = std::sin(fAngle);
    const float fNormalY = fSin * fScale;
    const float fNormalZ = (1.0 - fR2) * fSign;
    XFNV_M128_F32(normal.m_quad, 0) = fNormalX;
    XFNV_M128_F32(normal.m_quad, 1) = fNormalY;
    XFNV_M128_F32(normal.m_quad, 2) = fNormalZ;
    XFNV_M128_F32(normal.m_quad, 3) = 0.0f;
}

// Takes the four points of a block lane by lane, each product and sum rounded to a float. A lane keeps its best dot
// product and that point's index, both as floats: the indices start at 0.5, 1.5, 2.5 and 3.5 and step by 4.0, and a
// lane is replaced only when its best is below the new dot product (not for NaN). The best lane's index is truncated;
// the point's components are copied as bits.
void hkGeometryProcessing::getSupportingVertex(
    const hkArray<TransposedVectors>& transposedVertices,
    const hkVector4& direction,
    hkVector4& vertexOut
) {
    const TransposedVectors* pVertices = transposedVertices.m_data;
    const float fDirX = XFNV_M128_F32(direction.m_quad, 0);
    const float fDirY = XFNV_M128_F32(direction.m_quad, 1);
    const float fDirZ = XFNV_M128_F32(direction.m_quad, 2);

    // -3.40282e38 (0xFF7FFFEE) in every lane.
    float afBestDot[4] = {-0x1.ffffdcp+127f, -0x1.ffffdcp+127f, -0x1.ffffdcp+127f, -0x1.ffffdcp+127f};
    float afBestIndex[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float afIndex[4] = {0.5f, 1.5f, 2.5f, 3.5f};

    // Counts the points as four per block and loops ((count - 1) >> 2) + 1 times while the count is positive.
    const uint32_t uiNumPoints = transposedVertices.m_size * 4;
    if (static_cast<int32_t>(uiNumPoints) > 0) {
        const TransposedVectors* pBlock = pVertices;
        uint32_t uiBlocks = ((uiNumPoints - 1) >> 2) + 1;
        do {
            const float afX[4] = {
                XFNV_M128_F32(pBlock->m_x.m_quad, 0),
                XFNV_M128_F32(pBlock->m_x.m_quad, 1),
                XFNV_M128_F32(pBlock->m_x.m_quad, 2),
                XFNV_M128_F32(pBlock->m_x.m_quad, 3)
            };
            const float afY[4] = {
                XFNV_M128_F32(pBlock->m_y.m_quad, 0),
                XFNV_M128_F32(pBlock->m_y.m_quad, 1),
                XFNV_M128_F32(pBlock->m_y.m_quad, 2),
                XFNV_M128_F32(pBlock->m_y.m_quad, 3)
            };
            const float afZ[4] = {
                XFNV_M128_F32(pBlock->m_z.m_quad, 0),
                XFNV_M128_F32(pBlock->m_z.m_quad, 1),
                XFNV_M128_F32(pBlock->m_z.m_quad, 2),
                XFNV_M128_F32(pBlock->m_z.m_quad, 3)
            };
            for (int32_t i = 0; i < 4; ++i) {
                const float fDotY = afY[i] * fDirY;
                const float fDotX = fDirX * afX[i];
                const float fDotXY = fDotY + fDotX;
                const float fDotZ = afZ[i] * fDirZ;
                const float fDot = fDotXY + fDotZ;
                if (afBestDot[i] < fDot) {
                    afBestDot[i] = fDot;
                    afBestIndex[i] = afIndex[i];
                }
                afIndex[i] = afIndex[i] + 4.0f;
            }
            ++pBlock;
        }
        while (--uiBlocks);
    }

    // A later lane wins unless it is below the earlier one, so NaN picks the later lane.
    const int32_t iFirst = (afBestDot[1] < afBestDot[0]) ? 0 : 1;
    const int32_t iSecond = (afBestDot[3] < afBestDot[2]) ? 2 : 3;
    const int32_t iBest = (afBestDot[iSecond] < afBestDot[iFirst]) ? iFirst : iSecond;
    const int32_t iVertex = static_cast<int32_t>(afBestIndex[iBest]);

    // The point's block and lane; the shift is unsigned.
    const TransposedVectors& kBlock = pVertices[static_cast<uint32_t>(iVertex) >> 2];
    const uint32_t uiLane = static_cast<uint32_t>(iVertex) & 3;
    const uint32_t* puiX = reinterpret_cast<const uint32_t*>(&kBlock.m_x.m_quad);
    const uint32_t* puiY = reinterpret_cast<const uint32_t*>(&kBlock.m_y.m_quad);
    const uint32_t* puiZ = reinterpret_cast<const uint32_t*>(&kBlock.m_z.m_quad);
    const int32_t iResult = iVertex | 0x3F000000;
    uint32_t* puiOut = reinterpret_cast<uint32_t*>(&vertexOut.m_quad);
    puiOut[0] = puiX[uiLane];
    puiOut[1] = puiY[uiLane];
    puiOut[2] = puiZ[uiLane];
    puiOut[3] = static_cast<uint32_t>(iResult);
}

// With e1 = p1 - p0 and e2 = p2 - p0, the doubled area is the length of their cross product, and each squared length is
// summed z + (y + x). The numerator is sqrt(3) * (area + area), with sqrt(3) computed from the double 3.0 and stored as
// a float. The denominator is |p2 - p0|^2 + (|p1 - p2|^2 + |p0 - p1|^2). The result is the quotient when 1.1920929e-07f
// is below the denominator, and 0 otherwise, including when the denominator is NaN.
float hkGeometryProcessing::computeTriangleQuality(const hkVector4& p0, const hkVector4& p1, const hkVector4& p2) {
    auto sum3 = [](__m128 v) {
        return _mm_add_ss(_mm_shuffle_ps(v, v, 0xAA), _mm_add_ss(_mm_shuffle_ps(v, v, 0x55), v));
    };
    const __m128 kP0 = _mm_load_ps(&reinterpret_cast<const float*>(&p0.m_quad)[0]);
    const __m128 kP1 = _mm_load_ps(&reinterpret_cast<const float*>(&p1.m_quad)[0]);
    const __m128 kP2 = _mm_load_ps(&reinterpret_cast<const float*>(&p2.m_quad)[0]);
    const __m128 kE1 = _mm_sub_ps(kP1, kP0);
    const __m128 kE2 = _mm_sub_ps(kP2, kP0);
    const __m128 kCross = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(kE2, kE2, 0xD2), _mm_shuffle_ps(kE1, kE1, 0xC9)),
        _mm_mul_ps(_mm_shuffle_ps(kE2, kE2, 0xC9), _mm_shuffle_ps(kE1, kE1, 0xD2))
    );
    const float fArea = _mm_cvtss_f32(_mm_sqrt_ss(sum3(_mm_mul_ps(kCross, kCross))));
    const float fSqrt3 = std::sqrt(3.0f);
    const float fNumerator = fSqrt3 * (fArea + fArea);

    const float fE2 = _mm_cvtss_f32(sum3(_mm_mul_ps(kE2, kE2)));
    const __m128 kD12 = _mm_sub_ps(kP1, kP2);
    const float fD12 = _mm_cvtss_f32(sum3(_mm_mul_ps(kD12, kD12)));
    const __m128 kD01 = _mm_sub_ps(kP0, kP1);
    const float fD01 = _mm_cvtss_f32(sum3(_mm_mul_ps(kD01, kD01)));
    const float fPartial = fD12 + fD01;
    const float fDenominator = fE2 + fPartial;
    if (HK_REAL_EPSILON < fDenominator) {
        const float fResult = fNumerator / fDenominator;
        return fResult;
    }
    return 0.0f;
}

// Fills four local vectors (set to hkVector4Zero first) one vertex at a time and, at each fourth, writes them
// transposed as the next block; a last partial block gets its first vertex copied into the lanes past the end. output
// grows by (size + 3) >> 2 blocks.
void hkGeometryProcessing::buildTransposedArray(
    const hkArray<hkVector4>& vertices,
    hkArray<TransposedVectors>& output
) {
    hkVector4 aVertices[4];
    for (hkVector4& vertex : aVertices) {
        vertex = hkVector4Zero;
    }
    const int32_t iNumBlocks = (vertices.m_size + 3) >> 2;
    int32_t iUsed = 0;
    TransposedVectors* pBlock = output.expandBy(iNumBlocks);

    auto writeBlock = [&aVertices](TransposedVectors* apBlock) {
        const __m128 kV0 = _mm_load_ps(&reinterpret_cast<float*>(&aVertices[0].m_quad)[0]);
        const __m128 kV1 = _mm_load_ps(&reinterpret_cast<float*>(&aVertices[1].m_quad)[0]);
        const __m128 kV2 = _mm_load_ps(&reinterpret_cast<float*>(&aVertices[2].m_quad)[0]);
        const __m128 kV3 = _mm_load_ps(&reinterpret_cast<float*>(&aVertices[3].m_quad)[0]);
        const __m128 kXY02 = _mm_unpacklo_ps(kV0, kV2);
        const __m128 kXY13 = _mm_unpacklo_ps(kV1, kV3);
        const __m128 kZW02 = _mm_unpackhi_ps(kV0, kV2);
        const __m128 kZW13 = _mm_unpackhi_ps(kV1, kV3);
        _mm_store_ps(&reinterpret_cast<float*>(&apBlock->m_x.m_quad)[0], _mm_unpacklo_ps(kXY02, kXY13));
        _mm_store_ps(&reinterpret_cast<float*>(&apBlock->m_y.m_quad)[0], _mm_unpackhi_ps(kXY02, kXY13));
        _mm_store_ps(&reinterpret_cast<float*>(&apBlock->m_z.m_quad)[0], _mm_unpacklo_ps(kZW02, kZW13));
    };

    const int32_t iNumVertices = vertices.m_size;
    if (iNumVertices <= 0) {
        return;
    }
    for (int32_t i = 0; i < iNumVertices; ++i) {
        aVertices[iUsed++] = vertices.m_data[i];
        if (iUsed == 4) {
            iUsed = 0;
            writeBlock(pBlock);
            ++pBlock;
        }
    }
    if (iUsed) {
        for (int32_t i = iUsed; i < 4; ++i) {
            aVertices[i] = aVertices[0];
        }
        writeBlock(pBlock);
    }
}

// Separating-axis test. The triangles are projected on each axis as Span<hkSimdReal> and the test stops at the first
// axis whose spans do not overlap. The axes, in order: the two triangles' normals, the nine cross products of an edge
// of each (A's edges outer, B's inner, edge i running from vertex i to vertex i + 1) and the nine differences A[i] -
// B[j] (i outer). The spans' bounds are compared as floats, so a NaN projection leaves the bounds as they are.
int32_t hkGeometryProcessing::checkTrianglesIntersection(const hkVector4* triangleA, const hkVector4* triangleB) {
    auto overlap = [&](const hkVector4& axis) {
        Span<hkSimdReal> spanA;
        Span<hkSimdReal> spanB;
        spanA.setFromVertices(axis, triangleA, 3);
        spanB.setFromVertices(axis, triangleB, 3);
        return spanA.intersect(spanB);
    };

    hkVector4 edge0;
    hkVector4 edge1;
    hkVector4 axis;
    // The triangles' normals (the SDK's crossOffset of each).
    edge0.setSub4(triangleA[1], triangleA[0]);
    edge1.setSub4(triangleA[2], triangleA[0]);
    axis.setCross(edge0, edge1);
    if (!overlap(axis)) {
        return 0;
    }
    edge0.setSub4(triangleB[1], triangleB[0]);
    edge1.setSub4(triangleB[2], triangleB[0]);
    axis.setCross(edge0, edge1);
    if (!overlap(axis)) {
        return 0;
    }
    // The cross products of the edges (the SDK's crossEdges).
    for (int32_t i = 0; i < 3; ++i) {
        edge0.setSub4(triangleA[(i + 1) % 3], triangleA[i]);
        for (int32_t j = 0; j < 3; ++j) {
            edge1.setSub4(triangleB[(j + 1) % 3], triangleB[j]);
            axis.setCross(edge0, edge1);
            if (!overlap(axis)) {
                return 0;
            }
        }
    }
    // The differences of the vertices.
    for (int32_t i = 0; i < 3; ++i) {
        for (int32_t j = 0; j < 3; ++j) {
            axis.setSub4(triangleA[i], triangleB[j]);
            if (!overlap(axis)) {
                return 0;
            }
        }
    }
    return 1;
}

// Passes seed on as generateUniformDistribution's maxIterations, with seed 0 and no density function.
bool hkGeometryProcessing::generateRandomDistribution(
    const hkGeometry& geometry,
    int32_t numPoints,
    hkArray<hkVector4>& points,
    int32_t seed
) {
    return generateUniformDistribution(geometry, numPoints, points, seed, 0, nullptr);
}

// A point's box has no margin and w 0; its leaf's user data is the point.
bool hkGeometryProcessing::generateUniformDistribution(
    const hkGeometry& geometry,
    int32_t numPoints,
    hkArray<hkVector4>& points,
    int32_t maxIterations,
    int32_t seed,
    const IFunction_4_1* densityFunction
) {
    static ConstFunction<IFunction_4_1> unityFunction = 1.0f;
    hkPseudoRandomGenerator rndGen(seed);
    if (!densityFunction) {
        densityFunction = &unityFunction;
    }
    hkgpMesh mesh;
    mesh.appendFromGeometry(geometry, hkTransformIdentity, -1);
    mesh.assignVertexNormals();
    mesh.rebuildTrianglesTree();
    const int32_t firstNewPoint = points.m_size;
    bool result = false;
    if (hkGeometryProcessingInternals::generateRandomDistribution(mesh, numPoints, points, rndGen)) {
        result = true;
        if (maxIterations > 0) {
            hkVector4* samples = points.m_data + firstNewPoint;
            hkVector4 margin;
            margin.setAll(0.0f);

            hkArray<uint32_t> leaves;
            leaves.m_data = nullptr;
            leaves.m_size = 0;
            leaves.m_capacityAndFlags = static_cast<int32_t>(hkArrayBase<uint32_t>::DONT_DEALLOCATE_FLAG);
            hkAabbTreeAabbTree32 tree;
            leaves.setSize(numPoints);

            // The samples' boxes and user data, built by pushBack, each sample weighted by its density.
            hkArray<hkAabb> volumes;
            volumes.m_data = nullptr;
            volumes.m_size = 0;
            volumes.m_capacityAndFlags = static_cast<int32_t>(hkArrayBase<hkAabb>::DONT_DEALLOCATE_FLAG);
            hkArray<void*> userData;
            userData.m_data = nullptr;
            userData.m_size = 0;
            userData.m_capacityAndFlags = static_cast<int32_t>(hkArrayBase<void*>::DONT_DEALLOCATE_FLAG);
            for (int32_t i = 0; i < numPoints; ++i) {
                hkVector4& sample = samples[i];
                hkAabb aabb;
                aabb.m_min.setSub4(sample, margin);
                aabb.m_min(3) = 0.0f;
                aabb.m_max.setAdd4(sample, margin);
                aabb.m_max(3) = 0.0f;
                volumes.pushBack(aabb);
                userData.pushBack(&sample);
                const float d = densityFunction->evaluate(sample);
                sample.m_quad = _mm_mul_ps(_mm_set1_ps(d), sample.m_quad);
                sample(3) = d;
            }
            tree.build(volumes.m_data, userData.m_data, volumes.m_size, leaves);

            // Twice as many probes per pass as samples, from 256 to 8192.
            int32_t numProbes = 2 * numPoints;
            hkArray<hkVector4> probes;
            probes.m_data = nullptr;
            probes.m_size = 0;
            probes.m_capacityAndFlags = static_cast<int32_t>(hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG);
            if (numProbes < 256) {
                numProbes = 256;
            }
            else if (numProbes > 8192) {
                numProbes = 8192;
            }
            probes.reserveExactly(numProbes);

            // Each pass adds every weighted probe to the sample nearest it, then makes each sample the weighted mean
            // of what it got (dividing by the summed weight in w), moves its leaf and weights it again.
            for (int32_t iteration = 0; iteration < maxIterations; ++iteration) {
                probes.m_size = 0;
                if (!hkGeometryProcessingInternals::generateRandomDistribution(mesh, numProbes, probes, rndGen)) {
                    result = false;
                    break;
                }
                for (hkVector4& probe : probes) {
                    hkGeometryProcessingInternals::NearestFeaturePolicy policy;
                    policy.m_point = probe;
                    policy.m_distanceSquared = HK_REAL_MAX;
                    policy.m_leaf = nullptr;
                    queryClosestPoint(&tree, probe, &policy);
                    if (policy.m_leaf) {
                        hkVector4* pNearest = static_cast<hkVector4*>(policy.m_leaf->m_leafData.m_userData);
                        const float d = densityFunction->evaluate(probe);
                        probe.m_quad = _mm_mul_ps(_mm_set1_ps(d), probe.m_quad);
                        probe(3) = d;
                        pNearest->m_quad = _mm_add_ps(probe.m_quad, pNearest->m_quad);
                    }
                }
                for (int32_t k = 0; k < numPoints; ++k) {
                    hkVector4& sample = samples[k];
                    const float inverseWeight = 1.0f / sample(3);
                    sample.m_quad = _mm_mul_ps(_mm_set1_ps(inverseWeight), sample.m_quad);
                    hkAabb aabb;
                    aabb.m_min.setSub4(sample, margin);
                    aabb.m_min(3) = 0.0f;
                    aabb.m_max.setAdd4(sample, margin);
                    aabb.m_max(3) = 0.0f;
                    tree.update(leaves.m_data[k], aabb);
                    const float d = densityFunction->evaluate(sample);
                    sample.m_quad = _mm_mul_ps(_mm_set1_ps(d), sample.m_quad);
                    sample(3) = d;
                }
            }

            probes.clearAndDeallocate();
            userData.clearAndDeallocate();
            volumes.clearAndDeallocate();
            leaves.clearAndDeallocate();
        }
    }
    return result;
}

// The policy holds nothing to release.
hkGeometryProcessingInternals::NearestFeaturePolicy::~NearestFeaturePolicy() {}

float hkGeometryProcessingInternals::NearestFeaturePolicy::addLeaf(
    const hkAabbTree<hkAabbTreeData::hkAabbTreeAabb, uint32_t>::Node* leaf
) {
    __m128 v2 = _mm_sub_ps(
        m_point.m_quad,
        _mm_mul_ps(_mm_add_ps(leaf->m_volume.m_max.m_quad, leaf->m_volume.m_min.m_quad), hkQuadRealHalf)
    );
    __m128 v3 = _mm_mul_ps(v2, v2);
    float v6 = XFNV_M128_F32(_mm_shuffle_ps(v3, v3, 170), 0) +
        (XFNV_M128_F32(_mm_shuffle_ps(v3, v3, 85), 0) + XFNV_M128_F32(v3, 0));
    if (m_distanceSquared > static_cast<double>(v6)) {
        m_distanceSquared = v6;
        m_leaf = leaf;
    }
    return 1.0f;
}

// Each try starts from a zero vector, which getRandomVector01 fills, scaled into the mesh's bounding box;
// squaredDistanceToPoint's distance is dropped, only its inside flag is used.
bool hkGeometryProcessingInternals::generateRandomDistribution(
    const hkgpMesh& mesh,
    int32_t numPoints,
    hkArray<hkVector4>& points,
    hkPseudoRandomGenerator& rndGenerator
) {
    hkAabb boundingBox;
    mesh.getBoundingBox(boundingBox);
    const hkVector4 origin = boundingBox.m_min;
    hkVector4 extent;
    extent.setSub4(boundingBox.m_max, boundingBox.m_min);
    const int32_t maxMisses = 8 * numPoints;
    int32_t misses = 0;
    while (numPoints) {
        hkVector4 location = hkVector4Zero;
        bool inside = true;
        rndGenerator.getRandomVector01(location);
        location.m_quad = _mm_add_ps(_mm_mul_ps(location.m_quad, extent.m_quad), origin.m_quad);
        hkVector4 normal;
        mesh.squaredDistanceToPoint(location, normal, &inside);
        if (inside) {
            location(3) = 1.0f;
            points.pushBack(location);
            --numPoints;
            misses = 0;
        }
        else if (++misses > maxMisses) {
            break;
        }
    }
    return numPoints == 0;
}

const hkClass& hkpOverwritePivotConstraintAtom::staticClass() {
    return hkpOverwritePivotConstraintAtomClass;
}

const hkClass& hkpSetLocalRotationsConstraintAtom::staticClass() {
    return hkpSetLocalRotationsConstraintAtomClass;
}
