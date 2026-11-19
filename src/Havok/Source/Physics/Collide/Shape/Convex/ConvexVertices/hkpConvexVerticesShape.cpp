#include "Havok/Source/Physics/Collide/Shape/Convex/ConvexVertices/hkpConvexVerticesShape.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Thread/Thread/hkThreadLocalData.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabbUtil.h"
#include "Havok/Source/Common/Base/Types/Geometry/Sphere/hkSphere.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkStridedVertices.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeGroupClass.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/CompressedMesh/hkpCompressedMeshShape.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/ExtendedMeshShape/hkpExtendedMeshShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Capsule/hkpCapsuleShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/ConvexVertices/hkpConvexVerticesConnectivity.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpCdVertex.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexShape.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastCollectorOutput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Utilities/Dynamics/Inertia/hkpInertiaTensorComputer.h"
#include "SDK/Windows/winbase.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

static void cleanupLoadedObjecthkpConvexVerticesShape(void* p);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpConvexVerticesShapeFourVectorsTypeInfo = {
    "hkpConvexVerticesShapeFourVectors",
    nullptr,
    cleanupLoadedObjecthkpConvexVerticesShapeFourVectors,
    nullptr,
    &typeid(hkpConvexVerticesShape::FourVectors),
    sizeof(hkpConvexVerticesShape::FourVectors)
};

// The class records' member tables.
using hkpConvexVerticesShapeFourVectorsClassMemberArray = const hkClassMember[3];
static hkpConvexVerticesShapeFourVectorsClassMemberArray hkpConvexVerticesShapeFourVectorsClass_Members{
    {"x", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, 0, 0x0, nullptr},
    {"y", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, 0, 0x10, nullptr},
    {"z", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, 0, 0x20, nullptr}
};
using hkpConvexVerticesShapeClassMemberArray = const hkClassMember[7];
static hkpConvexVerticesShapeClassMemberArray hkpConvexVerticesShapeClass_Members{
    {"aabbHalfExtents",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_VECTOR4},
     {hkClassMember::TYPE_VOID},
     0,
     0,
     0x20,
     nullptr},
    {"aabbCenter", nullptr, nullptr, {hkClassMember::TYPE_VECTOR4}, {hkClassMember::TYPE_VOID}, 0, 0, 0x30, nullptr},
    {"rotatedVertices",
     &hkpConvexVerticesShapeFourVectorsClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_STRUCT},
     0,
     0,
     0x40,
     nullptr},
    {"numVertices", nullptr, nullptr, {hkClassMember::TYPE_INT32}, {hkClassMember::TYPE_VOID}, 0, 0, 0x4C, nullptr},
    {"convexPiece",
     &hkpCompressedMeshShapeConvexPieceClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x50,
     nullptr},
    {"planeEquations",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_VECTOR4},
     0,
     0,
     0x54,
     nullptr},
    {"connectivity",
     &hkpConvexVerticesConnectivityClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     0,
     0x60,
     nullptr}
};
hkClass hkpConvexVerticesShapeFourVectorsClass(
    "hkpConvexVerticesShapeFourVectors",
    nullptr,
    0x30,
    nullptr,
    0,
    nullptr,
    0,
    hkpConvexVerticesShapeFourVectorsClass_Members,
    3,
    nullptr,
    nullptr,
    0,
    0
);

static void cleanupLoadedObjecthkpConvexVerticesShape(void* p) {
    static_cast<hkpConvexVerticesShape*>(p)->~hkpConvexVerticesShape();
}

hkClass hkpConvexVerticesShapeClass(
    "hkpConvexVerticesShape",
    &hkpConvexShapeClass,
    0x70,
    nullptr,
    0,
    nullptr,
    0,
    hkpConvexVerticesShapeClass_Members,
    7,
    nullptr,
    nullptr,
    0,
    3
);
hkTypeInfo hkpConvexVerticesShapeTypeInfo{"hkpConvexVerticesShape",
                                          finishLoadedObjecthkpConvexVerticesShape,
                                          cleanupLoadedObjecthkpConvexVerticesShape,
                                          getVtablehkpConvexVerticesShape(),
                                          &typeid(hkpConvexVerticesShape),
                                          sizeof(hkpConvexVerticesShape)};

const hkClass& hkpConvexVerticesShape::FourVectors::staticClass() {
    return hkpConvexVerticesShapeFourVectorsClass;
}

hkpConvexVerticesShape::hkpConvexVerticesShape(
    const hkStridedVertices& vertsIn,
    const hkArray<hkVector4>& planeEquations,
    float radius
)
  : hkpConvexShape(HK_SHAPE_CONVEX_VERTICES, radius), m_convexPiece(nullptr), m_connectivity(nullptr) {
    m_planeEquations.copyFromArray(hkContainerHeapAllocator().get(&m_planeEquations), planeEquations, hkTypeIsPod());
    copyVertexData(vertsIn.m_vertices, vertsIn.m_striding, vertsIn.m_numVertices);
}

hkpConvexVerticesShape::hkpConvexVerticesShape(float afRadius)
  : hkpConvexShape(HK_SHAPE_CONVEX_VERTICES, afRadius), m_convexPiece(nullptr), m_connectivity(nullptr) {}

// The caller's buffers kept as the arrays' storage (flagged DONT_DEALLOCATE), and the box's centre and half extents.
hkpConvexVerticesShape::hkpConvexVerticesShape(
    FourVectors* rotatedVertices,
    int32_t numVertices,
    hkVector4* planes,
    int32_t numPlanes,
    const hkAabb& aabb,
    float radius
)
  : hkpConvexShape(HK_SHAPE_CONVEX_VERTICES, radius), m_convexPiece(nullptr), m_connectivity(nullptr) {
    const int32_t numBlocks = ((numVertices + 3) & ~3) / 4;
    m_rotatedVertices.m_data = rotatedVertices;
    m_rotatedVertices.m_size = numBlocks;
    m_rotatedVertices.m_capacityAndFlags = numBlocks | hkArrayBase<FourVectors>::DONT_DEALLOCATE_FLAG;
    m_numVertices = numVertices;
    m_planeEquations.m_data = planes;
    m_planeEquations.m_size = numPlanes;
    m_planeEquations.m_capacityAndFlags = numPlanes | hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
    m_aabbHalfExtents.setSub4(aabb.m_max, aabb.m_min);
    m_aabbHalfExtents.m_quad = _mm_mul_ps(_mm_set1_ps(0.5f), m_aabbHalfExtents.m_quad);
    m_aabbCenter.setAdd4(aabb.m_min, aabb.m_max);
    m_aabbCenter.m_quad = _mm_mul_ps(_mm_set1_ps(0.5f), m_aabbCenter.m_quad);
}

hkpConvexVerticesShape::~hkpConvexVerticesShape() {
    if (m_connectivity) {
        m_connectivity->removeReference();
    }
}

int32_t hkpConvexVerticesShape::calcSizeForSpu(const CalcSizeForSpuInput& a1, int32_t a2) const {
    return 112;
}

// The cached box under localToWorld, grown by the convex radius plus the tolerance (summed to a float).
void hkpConvexVerticesShape::getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const {
    float extraRadius = m_radius + tolerance;
    hkAabbUtil::calcAabb(localToWorld, m_aabbHalfExtents, m_aabbCenter, extraRadius, out);
}

// Ray cast against the planes. A shape without planes builds them once from its compressed mesh convex piece: one plane
// per face, through the face's first three vertices. Those vertices' w lanes are never written: they flow only into the
// plane's w, which the plane's distance then overwrites. The ray is then clipped by each plane from the last one down:
// the entry fraction is the latest entry, the exit fraction the earliest exit, and the normal is the plane of the
// latest entry.
hkBool hkpConvexVerticesShape::castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) const {
    HK_TIMER_BEGIN("rcConvexVert", nullptr);
    if (m_planeEquations.m_size == 0) {
        const hkpCompressedMeshShape::ConvexPiece* pPiece = m_convexPiece;
        if (pPiece) {
            const uint16_t* pFaceOffset = pPiece->m_faceOffsets.m_data;
            int32_t iNumFaces = pPiece->m_faceOffsets.m_size - 1;
            if (iNumFaces > 0) {
                do {
                    const uint8_t* pFace = &pPiece->m_faceVertices.m_data[*pFaceOffset];
                    const FourVectors* pVertices = m_rotatedVertices.m_data;
                    __m128 v0;
                    __m128 v1;
                    __m128 v2;
                    uint32_t uiId = pFace[0];
                    const FourVectors* pBlock = &pVertices[uiId >> 2];
                    v0[0] = pBlock->m_x.m_quad[uiId & 3];
                    v0[1] = pBlock->m_y.m_quad[uiId & 3];
                    v0[2] = pBlock->m_z.m_quad[uiId & 3];
                    uiId = pFace[1];
                    pBlock = &pVertices[uiId >> 2];
                    v1[0] = pBlock->m_x.m_quad[uiId & 3];
                    v1[1] = pBlock->m_y.m_quad[uiId & 3];
                    v1[2] = pBlock->m_z.m_quad[uiId & 3];
                    uiId = pFace[2];
                    pBlock = &pVertices[uiId >> 2];
                    v2[0] = pBlock->m_x.m_quad[uiId & 3];
                    v2[1] = pBlock->m_y.m_quad[uiId & 3];
                    v2[2] = pBlock->m_z.m_quad[uiId & 3];
                    hkVector4 e1;
                    e1.m_quad = _mm_sub_ps(v1, v0);
                    hkVector4 e2;
                    e2.m_quad = _mm_sub_ps(v2, v0);
                    hkVector4 planeEquation;
                    planeEquation.setCross(e1, e2);
                    planeEquation.normalize3();
                    const __m128 d = _mm_mul_ps(planeEquation.m_quad, v0);
                    const __m128 dot =
                        _mm_add_ss(_mm_shuffle_ps(d, d, 0xAA), _mm_add_ss(_mm_shuffle_ps(d, d, 0x55), d));
                    planeEquation.m_quad[3] = _mm_cvtss_f32(_mm_sub_ss(_mm_setzero_ps(), dot));
                    m_planeEquations.pushBack(planeEquation);
                    ++pFaceOffset;
                }
                while (--iNumFaces);
            }
        }
    }
    float fEnter = -1.0f;
    float fExit = results.m_hitFraction;
    __m128 bestNormal = _mm_setzero_ps();
    for (int32_t i = m_planeEquations.m_size - 1; i >= 0; --i) {
        const __m128 plane = m_planeEquations.m_data[i].m_quad;
        const __m128 w = _mm_shuffle_ps(plane, plane, 0xFF);
        const __m128 pf = _mm_mul_ps(plane, input.m_from.m_quad);
        const float fDistFrom = _mm_cvtss_f32(
            _mm_add_ps(_mm_add_ss(_mm_shuffle_ps(pf, pf, 0x55), pf), _mm_add_ss(_mm_shuffle_ps(pf, pf, 0xAA), w))
        );
        const __m128 pt = _mm_mul_ps(plane, input.m_to.m_quad);
        const float fDistTo = _mm_cvtss_f32(
            _mm_add_ps(_mm_add_ss(_mm_shuffle_ps(pt, pt, 0x55), pt), _mm_add_ss(_mm_shuffle_ps(pt, pt, 0xAA), w))
        );
        if (fDistFrom >= 0.0f) {
            // The ray starts outside this plane: no hit when it ends outside too.
            if (fDistTo >= 0.0f) {
                HK_TIMER_END();
                return hkBool(false);
            }
            const float fFraction = fDistFrom / (fDistFrom - fDistTo);
            if (fFraction >= fEnter) {
                fEnter = fFraction;
                bestNormal = plane;
            }
        }
        else {
            // Inside at the start: only a ray that leaves through this plane moves the exit, and only then are the
            // fractions tested.
            if (!(fDistTo >= 0.0f)) {
                continue;
            }
            const float fFraction = fDistFrom / (fDistFrom - fDistTo);
            if (!(fFraction > fExit)) {
                fExit = fFraction;
            }
        }
        if (fExit < fEnter) {
            HK_TIMER_END();
            return hkBool(false);
        }
    }
    if (fEnter < 0.0f) {
        HK_TIMER_END();
        return hkBool(false);
    }
    results.m_hitFraction = fEnter;
    results.m_normal.m_quad = bestNormal;
    results.m_shapeKeys[results.m_shapeKeyIndex] = HK_INVALID_SHAPE_KEY;
    HK_TIMER_END();
    return hkBool(true);
}

int32_t hkpConvexVerticesShape::getNumCollisionSpheresImpl() const {
    return (int32_t)((uint32_t)m_numVertices);
}

// One sphere (x, y, z, m_radius) per vertex, four to a FourVectors block, then the last one to three lanes of the next
// block.
const hkSphere* hkpConvexVerticesShape::getCollisionSpheresImpl(hkSphere* sphereBuffer) const {
    const FourVectors* pBlock = m_rotatedVertices.m_data;
    hkSphere* pSphere = sphereBuffer;
    int32_t i = m_numVertices - 1;
    for (; i >= 3; i -= 4) {
        for (int32_t j = 0; j < 4; ++j) {
            pSphere->m_pos.m_quad =
                _mm_setr_ps(pBlock->m_x.m_quad[j], pBlock->m_y.m_quad[j], pBlock->m_z.m_quad[j], m_radius);
            ++pSphere;
        }
        ++pBlock;
    }
    for (int32_t j = 0; j <= i; ++j) {
        pSphere->m_pos.m_quad =
            _mm_setr_ps(pBlock->m_x.m_quad[j], pBlock->m_y.m_quad[j], pBlock->m_z.m_quad[j], m_radius);
        ++pSphere;
    }
    return sphereBuffer;
}

void hkpConvexVerticesShape::getFirstVertex(hkVector4& v) const {
    hkpConvexVerticesShape::FourVectors* m_data = m_rotatedVertices.m_data;
    v.m_quad[0] = m_data->m_x.m_quad[0];
    v.m_quad[1] = m_data->m_y.m_quad[0u];
    v.m_quad[2] = m_data->m_z.m_quad[0u];
    v.m_quad[3] = 0.0f;
}

// Computes the dots of four vertices at a time with the direction, keeping per lane the best dot and its vertex index;
// then takes the best of the four lanes, and returns that vertex with its index in w (setInt24W). initialDot is a
// guarded static -HK_REAL_MAX vector.
void hkpConvexVerticesShape::getSupportingVertexImpl(
    const hkVector4& direction,
    hkpCdVertex& supportingVertexOut
) const {
    const FourVectors* pVertices = m_rotatedVertices.m_data;
    if ((initialDot_guard & 1) == 0) {
        initialDot_guard |= 1;
        initialDot[0] = -3.40282e+38f;
        initialDot[1] = -3.40282e+38f;
        initialDot[2] = -3.40282e+38f;
        initialDot[3] = -3.40282e+38f;
    }
    __m128 bestDot = initialDot;
    __m128 curIndices = rdata_10D9FE0;
    __m128 bestIndices = _mm_setzero_ps();
    const __m128 dir = direction.m_quad;
    const __m128 dirX = _mm_shuffle_ps(dir, dir, 0x00);
    const __m128 dirY = _mm_shuffle_ps(dir, dir, 0x55);
    const __m128 dirZ = _mm_shuffle_ps(dir, dir, 0xAA);
    const FourVectors* pBlock = pVertices;
    for (int32_t i = 0; i < 4 * m_rotatedVertices.m_size; i += 4) {
        __m128 dot = _mm_add_ps(
            _mm_add_ps(_mm_mul_ps(pBlock->m_y.m_quad, dirY), _mm_mul_ps(pBlock->m_x.m_quad, dirX)),
            _mm_mul_ps(pBlock->m_z.m_quad, dirZ)
        );
        __m128 comp = _mm_cmplt_ps(bestDot, dot);
        bestDot = _mm_or_ps(_mm_and_ps(comp, dot), _mm_andnot_ps(comp, bestDot));
        bestIndices = _mm_or_ps(_mm_andnot_ps(comp, bestIndices), _mm_and_ps(comp, curIndices));
        curIndices = _mm_add_ps(rdata_10D0FA0, curIndices);
        ++pBlock;
    }
    // Unordered compares keep lane 1, take lane 3, and take the second pair's lane.
    int32_t iBestLow = 1;
    if (bestDot[1] < bestDot[0]) {
        iBestLow = 0;
    }
    int32_t iBestHigh = 2;
    if (!(bestDot[3] < bestDot[2])) {
        iBestHigh = 3;
    }
    int32_t iBest = iBestLow;
    if (!(bestDot[iBestHigh] < bestDot[iBestLow])) {
        iBest = iBestHigh;
    }
    const uint32_t uiId = static_cast<uint32_t>(static_cast<int32_t>(bestIndices[iBest]));
    const FourVectors& rBlock = pVertices[uiId >> 2];
    const uint32_t uiLane = uiId & 3;
    supportingVertexOut.m_quad[0] = rBlock.m_x.m_quad[uiLane];
    supportingVertexOut.m_quad[1] = rBlock.m_y.m_quad[uiLane];
    supportingVertexOut.m_quad[3] = std::bit_cast<float>(uiId | 0x3F000000u);
    supportingVertexOut.m_quad[2] = rBlock.m_z.m_quad[uiLane];
}

// Each id picks lane (id & 3) of group (id >> 2) of m_rotatedVertices; w holds the id as an int with 0x3F000000 or'd in
// (setInt24W).
void hkpConvexVerticesShape::convertVertexIdsToVerticesImpl(
    const uint16_t* ids,
    int32_t numIds,
    hkpCdVertex* verticesOut
) const {
    for (int32_t i = numIds - 1; i >= 0; --i) {
        uint32_t uiId = *ids++;
        const FourVectors& rFour = m_rotatedVertices.m_data[static_cast<int32_t>(uiId) >> 2];
        uint32_t uiLane = uiId & 3;
        verticesOut->m_quad[0] = (&reinterpret_cast<const float*>(&rFour.m_x.m_quad)[0])[uiLane];
        verticesOut->m_quad[1] = (&reinterpret_cast<const float*>(&rFour.m_y.m_quad)[0])[uiLane];
        verticesOut->m_quad[2] = (&reinterpret_cast<const float*>(&rFour.m_z.m_quad)[0])[uiLane];
        verticesOut->m_quad[3] = std::bit_cast<float>(uiId | 0x3F000000u);
        ++verticesOut;
    }
}

void hkpConvexVerticesShape::getCentreImpl(hkVector4& centreOut) const {
    centreOut = m_aabbCenter;
}

// The vertices, taken back out of their blocks into a stack array, moved by t (the last first) and stored again; then
// each plane's normal rotated by a copy of t's rotation and its distance moved by minus t's translation through the
// transposed rotation. Both loops are do-whiles: each runs its body once even for an empty array, as the game does.
void hkpConvexVerticesShape::transformVerticesAndPlaneEquations(const hkTransform& t) {
    const int32_t numVertices = getNumCollisionSpheresImpl();
    hkLocalArray<hkVector4> vertices(numVertices);
    getOriginalVertices(vertices);

    int32_t i = vertices.m_size - 1;
    hkVector4* pVertex = &vertices.m_data[i];
    do {
        pVertex->setTransformedPos(t, *pVertex);
        --i;
        --pVertex;
    }
    while (i >= 0);
    copyVertexData(&vertices.m_data[0](0), static_cast<int32_t>(sizeof(hkVector4)), vertices.m_size);

    hkVector4* pPlane = m_planeEquations.m_data;
    int32_t remaining = m_planeEquations.m_size - 1;
    hkVector4 negTranslation;
    negTranslation.setNeg4(t.m_translation);
    hkRotation rotation;
    rotation.m_col0.m_quad = t.m_rotation.m_col0.m_quad;
    rotation.m_col1.m_quad = t.m_rotation.m_col1.m_quad;
    rotation.m_col2.m_quad = t.m_rotation.m_col2.m_quad;
    // The transposed rotation's rows (its w lanes are not used), times minus the translation.
    const __m128 lo = _mm_unpacklo_ps(rotation.m_col0.m_quad, rotation.m_col1.m_quad);
    const __m128 hi = _mm_unpackhi_ps(rotation.m_col0.m_quad, rotation.m_col1.m_quad);
    const __m128 row0 = _mm_movelh_ps(lo, rotation.m_col2.m_quad);
    const __m128 row1 = _mm_shuffle_ps(_mm_movehl_ps(row0, lo), rotation.m_col2.m_quad, 0xD4);
    const __m128 row2 = _mm_shuffle_ps(hi, rotation.m_col2.m_quad, 0xE4);
    const __m128 n = negTranslation.m_quad;
    const __m128 shift = _mm_add_ps(
        _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(n, n, 0x55), row1), _mm_mul_ps(_mm_shuffle_ps(n, n, 0x00), row0)),
        _mm_mul_ps(row2, _mm_shuffle_ps(n, n, 0xAA))
    );
    do {
        const __m128 plane = pPlane->m_quad;
        const __m128 product = _mm_mul_ps(plane, shift);
        const __m128 distance = _mm_add_ps(
            _mm_add_ss(_mm_shuffle_ps(product, product, 0xAA), _mm_shuffle_ps(plane, plane, 0xFF)),
            _mm_add_ss(_mm_shuffle_ps(product, product, 0x55), product)
        );
        pPlane->setRotatedDir(rotation, *pPlane);
        // The rotated normal's x, y and z, the distance in w.
        const __m128 rotated = pPlane->m_quad;
        pPlane->m_quad = _mm_shuffle_ps(rotated, _mm_unpackhi_ps(rotated, _mm_shuffle_ps(distance, distance, 0)), 0xC4);
        ++pPlane;
    }
    while (--remaining >= 0);
}

void hkpConvexVerticesShape::setConnectivity(const hkpConvexVerticesConnectivity* connect, bool sort) {
    if (connect) {
        connect->addReference();
    }
    const hkpConvexVerticesConnectivity* m_connectivity = this->m_connectivity;
    if (m_connectivity) {
        m_connectivity->removeReference();
    }
    this->m_connectivity = connect;
    if ((sort) && (connect)) {
        sortPlanes();
    }
}

// The planes are copied in, then sorted when the shape has its connectivity.
void hkpConvexVerticesShape::setPlaneEquations(const hkArray<hkVector4, hkContainerHeapAllocator>& planes) {
    m_planeEquations = planes;
    if (m_connectivity) {
        sortPlanes();
    }
}

// Stores the vertices transposed, four to a FourVectors, with the last vertex repeated up to the next multiple of four,
// then the AABB's centre and half extents.
void hkpConvexVerticesShape::copyVertexData(const float* vertexIn, int32_t byteStriding, int32_t numVertices) {
    m_numVertices = numVertices;
    // HK_NEXT_MULTIPLE_OF(4, numVertices)
    const int32_t paddedSize = (numVertices + 3) & ~3;
    // EXE-INLINED none: hkArray<FourVectors>::setSize
    // FourVectors' constructor and destructor are empty, so only the _reserve and the size store are left.
    m_rotatedVertices._reserve(hkMemoryRouter::getInstance().heap(), paddedSize / 4);
    m_rotatedVertices.m_size = paddedSize / 4;

    const float* v = vertexIn;
    int32_t i = 0;
    for (; i < numVertices; i++) {
        FourVectors& fourVectors = m_rotatedVertices.m_data[i >> 2];
        fourVectors.m_x(i & 3) = v[0];
        fourVectors.m_y(i & 3) = v[1];
        fourVectors.m_z(i & 3) = v[2];
        // EXE-INLINED none: hkAddByteOffsetConst (the vertices are raw bytes byteStriding apart, as the exe steps them)
        v = reinterpret_cast<const float*>(reinterpret_cast<const char*>(v) + byteStriding);
    }
    // The padding repeats the last vertex.
    v = reinterpret_cast<const float*>(reinterpret_cast<const char*>(v) - byteStriding);
    for (; i < paddedSize; i++) {
        FourVectors& fourVectors = m_rotatedVertices.m_data[i >> 2];
        fourVectors.m_x(i & 3) = v[0];
        fourVectors.m_y(i & 3) = v[1];
        fourVectors.m_z(i & 3) = v[2];
    }

    // The box is not default-constructed here: calcAabb fills it.
    hkAabb aabb;
    hkAabbUtil::calcAabb(vertexIn, numVertices, byteStriding, aabb);
    m_aabbHalfExtents.setSub4(aabb.m_max, aabb.m_min);
    m_aabbHalfExtents.m_quad = _mm_mul_ps(m_aabbHalfExtents.m_quad, hkQuadRealHalf);
    m_aabbCenter.setAdd4(aabb.m_min, aabb.m_max);
    m_aabbCenter.m_quad = _mm_mul_ps(m_aabbCenter.m_quad, hkQuadRealHalf);
}

// The vertices back out of their transposed blocks: each one's x, y and z from its lane of its FourVectors, w 0.
void hkpConvexVerticesShape::getOriginalVertices(hkArray<hkVector4>& vertices) const {
    vertices.setSize(m_numVertices);
    for (int32_t i = 0; i < m_numVertices; i++) {
        const FourVectors& block = m_rotatedVertices.m_data[i / 4];
        const int32_t lane = i & 3;
        vertices.m_data[i].set(block.m_x(lane), block.m_y(lane), block.m_z(lane), 0.0f);
    }
}

// The SDK's sortPlanes: the planes, copied out, go back in the order of the connectivity's faces. For each face of
// at least three vertices (only those advance the index into the face vertices): the plane whose normal has the
// largest dot with the face's unit normal (the x87 compare takes a later plane only when its dot is greater), removed
// from the copy (the last plane moved into its place); a face whose normal has length 0 takes the first plane again,
// without removing it. The walk stops when the copy is empty; the planes no face took go after the others.
void hkpConvexVerticesShape::sortPlanes() {
    hkArray<hkVector4> planes(m_planeEquations);
    const int32_t numVertices = m_numVertices;
    m_planeEquations.clear();
    // The local is 16-aligned; getOriginalVertices gets its address.
    alignas(16) hkLocalArray<hkVector4> vertices(numVertices);
    getOriginalVertices(vertices);

    int32_t startIndex = 0;
    int32_t face = 0;
    if (face < m_connectivity->m_numVerticesPerFace.m_size) {
        do {
            if (planes.m_size == 0) {
                break;
            }
            const int32_t numFaceVertices = m_connectivity->m_numVerticesPerFace.m_data[face];
            if (numFaceVertices >= 3) {
                const uint16_t* pIndices = &m_connectivity->m_vertexIndices.m_data[startIndex];
                const hkVector4& v0 = vertices.m_data[pIndices[0]];
                const hkVector4& v1 = vertices.m_data[pIndices[1]];
                const hkVector4& v2 = vertices.m_data[pIndices[2]];
                startIndex += numFaceVertices;
                hkVector4 e1;
                e1.setSub4(v1, v0);
                hkVector4 e2;
                e2.setSub4(v2, v0);
                hkVector4 normal;
                normal.setCross(e1, e2);
                const __m128 square = _mm_mul_ps(normal.m_quad, normal.m_quad);
                const float lengthSquared = _mm_cvtss_f32(_mm_add_ss(
                    _mm_shuffle_ps(square, square, 0xAA), _mm_add_ss(_mm_shuffle_ps(square, square, 0x55), square)
                ));
                if (lengthSquared == 0.0f) {
                    // EXE-INLINED none: hkArray<hkVector4>::pushBackUnchecked
                    m_planeEquations.m_data[m_planeEquations.m_size].m_quad = planes.m_data[0].m_quad;
                    ++m_planeEquations.m_size;
                }
                else {
                    const float fLength = std::sqrt(lengthSquared);
                    const float fInvLength = 1.0 / fLength;
                    const __m128 unitNormal = _mm_mul_ps(_mm_set1_ps(fInvLength), normal.m_quad);
                    const __m128 first = _mm_mul_ps(unitNormal, planes.m_data[0].m_quad);
                    float fBestDot = _mm_cvtss_f32(_mm_add_ss(
                        _mm_shuffle_ps(first, first, 0xAA), _mm_add_ss(_mm_shuffle_ps(first, first, 0x55), first)
                    ));
                    int32_t best = 0;
                    for (int32_t j = 1; j < planes.m_size; ++j) {
                        const __m128 product = _mm_mul_ps(unitNormal, planes.m_data[j].m_quad);
                        const float fDot = _mm_cvtss_f32(_mm_add_ss(
                            _mm_shuffle_ps(product, product, 0xAA),
                            _mm_add_ss(_mm_shuffle_ps(product, product, 0x55), product)
                        ));
                        if (fBestDot < fDot) {
                            fBestDot = fDot;
                            best = j;
                        }
                    }
                    // EXE-INLINED none: hkArray<hkVector4>::pushBackUnchecked
                    m_planeEquations.m_data[m_planeEquations.m_size].m_quad = planes.m_data[best].m_quad;
                    ++m_planeEquations.m_size;
                    planes.removeAt(best);
                }
            }
            ++face;
        }
        while (face < m_connectivity->m_numVerticesPerFace.m_size);
    }
    if (planes.m_size > 0) {
        const int32_t index = m_planeEquations.m_size;
        hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
        m_planeEquations._insertAt(heap, index, planes.m_data, planes.m_size);
    }
}

int32_t getVector4At(int32_t a1, int32_t r_eax) {
    return a1 + 16 * r_eax;
}
