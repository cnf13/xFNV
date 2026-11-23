#include "Havok/Source/Physics/Collide/Shape/Deprecated/ConvexPieceMesh/hkpConvexPieceShape.h"

#include "Havok/Source/Animation/Animation/Ik/LookAt/hkaLookAtIkSolver.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Geometry/Sphere/hkSphere.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Filter/hkpRayShapeCollectionFilter.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Triangle/hkpTriangleShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpCdVertex.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexShape.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpRayHitCollector.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeBuffer.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <xmmintrin.h>

int32_t getPrimitiveStyle(int32_t type) {
    int32_t result;
    switch (type) {
        case 1: {
            result = 1;
            break;
        }
        case 2: {
            result = 2;
            break;
        }
        case 3:
        case 4: {
            result = 3;
            break;
        }
        default: {
            result = 0;
            break;
        }
    }
    return result;
}

hkpConvexPieceShape::hkpConvexPieceShape(float radius) : hkpConvexShape(HK_SHAPE_CONVEX_PIECE, radius) {}

hkpConvexPieceShape::~hkpConvexPieceShape() {}

const hkpShapeContainer* hkpConvexPieceShape::getContainer() const {
    if (this) {
        return static_cast<const hkpShapeContainer*>(this);
    }

    return nullptr;
}

void hkpConvexPieceShape::getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const {
    uint32_t v9;
    out.m_min.m_quad = _mm_shuffle_ps(_mm_set_ss(HK_REAL_MAX), _mm_set_ss(HK_REAL_MAX), 0);
    int32_t v4 = 0;
    out.m_max.m_quad = _mm_shuffle_ps(_mm_set_ss(-HK_REAL_MAX), _mm_set_ss(-HK_REAL_MAX), 0);
    if (m_numVertices > 0) {
        int32_t v5 = 0;
        do {
            __m128 m_quad = m_vertices[v5].m_quad;
            __m128 v7 = _mm_add_ps(
                _mm_add_ps(
                    _mm_mul_ps(_mm_shuffle_ps(m_quad, m_quad, 0), localToWorld.m_rotation.m_col0.m_quad),
                    _mm_mul_ps(_mm_shuffle_ps(m_quad, m_quad, 85), localToWorld.m_rotation.m_col1.m_quad)
                ),
                _mm_add_ps(
                    _mm_mul_ps(_mm_shuffle_ps(m_quad, m_quad, 170), localToWorld.m_rotation.m_col2.m_quad),
                    localToWorld.m_translation.m_quad
                )
            );
            out.m_min.m_quad = _mm_min_ps(out.m_min.m_quad, v7);
            ++v4;
            out.m_max.m_quad = _mm_max_ps(out.m_max.m_quad, v7);
            ++v5;
        }
        while (v4 < m_numVertices);
    }
    *reinterpret_cast<float*>(&v9) = m_radius + tolerance;
    __m128 v8 = _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(v9)), _mm_set_ss(std::bit_cast<float>(v9)), 0);
    out.m_min.m_quad = _mm_sub_ps(out.m_min.m_quad, v8);
    out.m_max.m_quad = _mm_add_ps(out.m_max.m_quad, v8);
}

// Casts the ray against each display triangle the filter lets through, one shape key level down; the last hit's index
// becomes that level's key.
hkBool hkpConvexPieceShape::castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) const {
    HK_TIMER_BEGIN("rcConvxPiece", nullptr);
    alignas(16) char buffer[HK_SHAPE_BUFFER_SIZE];
    ++results.m_shapeKeyIndex;
    int32_t hitIndex = -1;
    for (int32_t i = 0; i < m_numDisplayShapeKeys; ++i) {
        if (input.m_rayShapeCollectionFilter &&
            (!input.m_rayShapeCollectionFilter
                  ->isCollisionEnabled(
                      input,
                      *m_displayMesh,
                      *static_cast<const hkpShapeContainer*>(m_displayMesh),
                      m_displayShapeKeys[i]
                  )
                  .m_bool))
        // Stands in for *m_displayMesh->getContainer(); the container adjustment is null-tested here.
        {
            continue;
        }

        // m_displayMesh->getChildShape: adjusts to the container base without a null test.
        const hkpShape* childShape = static_cast<const hkpShapeContainer&>(*m_displayMesh)
                                         .getChildShape(m_displayShapeKeys[i], buffer);
        if (childShape->castRayImpl(input, results).m_bool) {
            hitIndex = i;
        }
    }
    --results.m_shapeKeyIndex;
    if (hitIndex != -1) {
        results.m_shapeKeys[results.m_shapeKeyIndex] = hitIndex;
    }
    HK_TIMER_END();
    return hkBool(hitIndex != -1);
}

// Casts the ray against each display triangle of the piece in turn (the SDK's castRayWithCollector): the ray's
// collection filter, when it has one, decides per display shape key; each child gets a body below cdBody.
void hkpConvexPieceShape::castRayWithCollectorImpl(
    const hkpShapeRayCastInput& input,
    const hkpCdBody& cdBody,
    hkpRayHitCollector& collector
) const {
    HK_TIMER_BEGIN("rcConvxPiece", nullptr);
    for (int32_t i = 0; i < m_numDisplayShapeKeys; i++) {
        if (input.m_rayShapeCollectionFilter) {
            // Converts the display mesh to its container with a null test: a pointer conversion, which a reference
            // conversion would not make.
            const hkpShapeContainer* pContainer = m_displayMesh;
            if (!input.m_rayShapeCollectionFilter
                     ->isCollisionEnabled(input, *m_displayMesh, *pContainer, m_displayShapeKeys[i])
                     .m_bool) {
                continue;
            }
        }
        alignas(16) char shapeBuffer[HK_SHAPE_BUFFER_SIZE];
        const hkpShape* childShape = m_displayMesh->getChildShape(m_displayShapeKeys[i], shapeBuffer);
        hkpCdBody childBody;
        childBody.m_parent = &cdBody;
        childBody.m_motion.pState = cdBody.m_motion.pState;
        childBody.m_shape = childShape;
        childBody.m_shapeKey = i;
        childShape->castRayWithCollectorImpl(input, childBody, collector);
    }
    HK_TIMER_END();
}

int32_t hkpConvexPieceShape::getNumCollisionSpheresImpl() const {
    return m_numVertices;
}

const hkSphere* hkpConvexPieceShape::getCollisionSpheresImpl(hkSphere* sphereBuffer) const {
    int32_t v3 = 0;
    hkSphere* v4 = sphereBuffer;
    if (m_numVertices > 0) {
        int32_t v5 = 0;
        do {
            *v4 = static_cast<hkSphere>(m_vertices[v5]);
            XFNV_M128_F32(v4->m_pos.m_quad, 3) = m_radius;
            ++v3;
            ++v4;
            ++v5;
        }
        while (v3 < m_numVertices);
    }
    return sphereBuffer;
}

void hkpConvexPieceShape::getFirstVertex(hkVector4& v) const {
    v = *m_vertices;
}

// The supporting vertex over the display triangles: the child vertex of the largest dot product with aDirection, its
// id the triangle's base (3 per key) plus the child's id over 16.
void hkpConvexPieceShape::getSupportingVertexImpl(const hkVector4& dir, hkpCdVertex& supportingVertex) const {
    alignas(16) char buffer[HK_SHAPE_BUFFER_SIZE];
    float maxDot = -HK_REAL_MAX;
    int32_t vertexId = 0;
    int32_t vertexBase = 0;
    for (int32_t i = 0; i < m_numDisplayShapeKeys; ++i) {
        // m_displayMesh->getChildShape: adjusts to the container base without a null test.
        const hkpConvexShape* childShape = static_cast<const hkpConvexShape*>(
            static_cast<const hkpShapeContainer&>(*m_displayMesh).getChildShape(m_displayShapeKeys[i], buffer)
        );
        alignas(16) hkpCdVertex support;
        childShape->getSupportingVertexImpl(dir, support);
        // The dot3 is computed in SSE as (y + x) + z, single precision. The additions are written as addps and only
        // lane 0 is read.
        const __m128 product = _mm_mul_ps(dir.m_quad, support.m_quad);
        const float dot = _mm_cvtss_f32(_mm_add_ps(
            _mm_shuffle_ps(product, product, 0xAA),
            _mm_add_ps(_mm_shuffle_ps(product, product, 0x55), product)
        ));
        if (maxDot < dot) {
            maxDot = dot;
            // The child's id is divided signed.
            vertexId = vertexBase + support.getInt24W() / 16;
            supportingVertex.m_quad = support.m_quad;
        }
        vertexBase += 3;
    }
    supportingVertex.setInt24W(vertexId);
}

void hkpConvexPieceShape::convertVertexIdsToVerticesImpl(
    const uint16_t* ids,
    int32_t numIds,
    hkpCdVertex* verticesOut
) const {
    for (int32_t i = numIds - 1; i >= 0; --i) {
        alignas(16) char buffer[HK_SHAPE_BUFFER_SIZE];
        int32_t vertexId = *ids++;
        // m_displayMesh->getChildShape: adjusts to the container base without a null test.
        const hkpTriangleShape* triangleShape = static_cast<const hkpTriangleShape*>(
            static_cast<const hkpShapeContainer&>(*m_displayMesh)
                .getChildShape(m_displayShapeKeys[vertexId / 3], buffer)
        );
        verticesOut->m_quad = triangleShape->getVertex(vertexId % 3).m_quad;
        verticesOut->setInt24W(vertexId);
        ++verticesOut;
    }
}

uint32_t hkpConvexPieceShape::getFirstKey() const {
    return 0;
}

uint32_t hkpConvexPieceShape::getNextKey(uint32_t oldKey) const {
    // The next key is compared signed against the key count.
    uint32_t result = oldKey + 1;
    if (static_cast<int32_t>(oldKey + 1) >= m_numDisplayShapeKeys) {
        return HK_INVALID_SHAPE_KEY;
    }
    return result;
}

const hkpShape* hkpConvexPieceShape::getChildShape(uint32_t key, char (&buffer)[512]) const {
    return m_displayMesh->getChildShape(m_displayShapeKeys[key], buffer);
}
