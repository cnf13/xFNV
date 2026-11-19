#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Collide/hkpConvexRadiusBuilder.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkStridedVertices.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/ConvexHull/hkGeometryUtility.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayBox.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayConvex.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayCylinder.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometry.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Box/hkpBoxShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/ConvexTransform/hkpConvexTransformShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/ConvexTranslate/hkpConvexTranslateShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/ConvexVertices/hkpConvexVerticesShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Cylinder/hkpCylinderShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Triangle/hkpTriangleShape.h"
#include "Havok/Source/Physics/Collide/Shape/Deprecated/ConvexPieceMesh/hkpConvexPieceShape.h"
#include "Havok/Source/Physics/Collide/Shape/Misc/Bv/hkpBvShape.h"
#include "Havok/Source/Physics/Collide/Shape/Misc/Transform/hkpTransformShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Collide/Util/hkpTriangleUtil.h"

#include <bit>
#include <xmmintrin.h>

// The singleton init node's create function for hkpUserConvexRadiusBuilder.
// Havok SDK signature (hkpUtilities.lib, tied by data).
static hkReferencedObject* hkpUserConvexRadiusBuildercreate();

// m_value is untyped storage: populate and depopulate move any singleton's instance pointer through it.
static hkSingletonInitNode hkSingletonRegisterhkpUserConvexRadiusBuilder(
    "hkpUserConvexRadiusBuilder",
    &hkpUserConvexRadiusBuildercreate,
    reinterpret_cast<void**>(&hkpUserConvexRadiusBuilder::s_instance)
);

// The singleton's create function: allocates a new hkpUserConvexRadiusBuilder with an empty registry.
static hkReferencedObject* hkpUserConvexRadiusBuildercreate() {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkpUserConvexRadiusBuilder* result =
        static_cast<hkpUserConvexRadiusBuilder*>(Instance->m_heap->blockAlloc(sizeof(hkpUserConvexRadiusBuilder)));
    result->m_memSizeAndFlags = sizeof(hkpUserConvexRadiusBuilder);
    return ::new (result) hkpUserConvexRadiusBuilder;
}

double _getPlaneToSurfaceDistance(const hkpShape* r_eax) {
    // getPlaneEquations, a trivial accessor: the shape's plane equations.
    hkArray<hkVector4, hkContainerHeapAllocator>* v2 =
        &static_cast<const hkpConvexVerticesShape*>(r_eax)->m_planeEquations;
    hkArray<hkVector4, hkContainerHeapAllocator> apVertices;
    static_cast<const hkpConvexVerticesShape*>(r_eax)->getOriginalVertices(apVertices);
    float v9 = -1000.0f;
    if (apVertices.m_size > 0) {
        hkVector4* m_data = apVertices.begin();
        int32_t m_size = apVertices.m_size;
        do {
            if (v2->m_size > 0) {
                __m128* p_m_quad = &v2->begin()->m_quad;
                int32_t v5 = v2->m_size;
                do {
                    __m128 v6 = _mm_mul_ps(*p_m_quad, m_data->m_quad);
                    float v11 =
                        (*p_m_quad)[3] + (_mm_shuffle_ps(v6, v6, 170)[0] + (_mm_shuffle_ps(v6, v6, 85)[0] + v6[0]));
                    if (v9 < static_cast<double>(v11)) {
                        v9 = v11;
                    }
                    ++p_m_quad;
                    --v5;
                }
                while (v5);
            }
            ++m_data;
            --m_size;
        }
        while (m_size);
    }
    return v9;
}

// Appends the six planes of the shape's AABB (expanded by radius): +x, +y, +z with w = -max, then -x, -y, -z with w =
// min.
void addAabbPlanes(const hkpShape* shape, float radius, hkArray<hkVector4, hkContainerHeapAllocator>& planes) {
    hkAabb aabb;
    shape->getAabbImpl(hkTransformIdentity, radius, aabb);
    hkMemoryAllocator* heap = hkMemoryRouter::getInstance().m_heap;
    int32_t numReq = planes.m_size + 6;
    int32_t capacity = planes.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK;
    if (capacity < numReq) {
        int32_t doubled = 2 * capacity;
        if (numReq < doubled) {
            numReq = doubled;
        }
        hkArrayUtil::_reserve(*heap, &planes, numReq, sizeof(hkVector4));
    }
    planes.m_size += 6;
    int32_t size = planes.m_size;
    const __m128 zero = _mm_set_ss(0.0f);
    const __m128 one = _mm_set_ss(1.0f);
    const __m128 minusOne = _mm_set_ss(-1.0f);
    const __m128 negZero = _mm_set_ss(-0.0f);
    __m128 w = _mm_sub_ss(negZero, _mm_load_ss(&aabb.m_max(0)));
    planes.m_data[size - 6].m_quad = _mm_unpacklo_ps(_mm_unpacklo_ps(one, zero), _mm_unpacklo_ps(zero, w));
    w = _mm_sub_ss(negZero, _mm_load_ss(&aabb.m_max(1)));
    planes.m_data[size - 5].m_quad = _mm_unpacklo_ps(_mm_unpacklo_ps(zero, zero), _mm_unpacklo_ps(one, w));
    w = _mm_sub_ss(negZero, _mm_load_ss(&aabb.m_max(2)));
    planes.m_data[size - 4].m_quad = _mm_unpacklo_ps(_mm_unpacklo_ps(zero, one), _mm_unpacklo_ps(zero, w));
    w = _mm_load_ss(&aabb.m_min(0));
    planes.m_data[size - 3].m_quad = _mm_unpacklo_ps(_mm_unpacklo_ps(minusOne, zero), _mm_unpacklo_ps(zero, w));
    w = _mm_load_ss(&aabb.m_min(1));
    planes.m_data[size - 2].m_quad = _mm_unpacklo_ps(_mm_unpacklo_ps(zero, zero), _mm_unpacklo_ps(minusOne, w));
    w = _mm_load_ss(&aabb.m_min(2));
    planes.m_data[size - 1].m_quad = _mm_unpacklo_ps(_mm_unpacklo_ps(zero, minusOne), _mm_unpacklo_ps(zero, w));
}

hkpConvexRadiusBuilder::hkpConvexRadiusBuilderEnvironment::hkpConvexRadiusBuilderEnvironment()
  : m_minVisibleRadius(0.001f) {}

hkpConvexRadiusBuilder::hkpConvexRadiusBuilder(const hkpConvexRadiusBuilderEnvironment& env)
  : m_environment(env), m_currentGeometry(nullptr) {}

void hkpConvexRadiusBuilder::buildDisplayGeometries(
    const hkpShape* shape,
    hkArray<hkDisplayGeometry*>& displayGeometries
) {
    alignas(16) hkTransform transform;
    transform.setIdentity();
    resetCurrentRawGeometry();
    displayGeometries.clear();
    buildShapeDisplay(shape, transform, displayGeometries);
}

// Its shape buffer, plane arrays and vectors passed by address sit at 16-byte aligned slots of its realigned frame.
void hkpConvexRadiusBuilder::buildShapeDisplay(
    const hkpShape* shape,
    const hkTransform& transform,
    hkArray<hkDisplayGeometry*, hkContainerHeapAllocator>& displayGeometries
) {
    // A bounding plane's distance: minus the dot product (z + (y + x)) of its normal with point moved out along the
    // normal by radius, subtracted from a zero vector.
    auto planeDistance = [](const hkVector4& normal, const hkVector4& point, float radius) {
        hkVector4 moved;
        moved.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(radius), normal.m_quad), point.m_quad);
        const hkSimdReal dot = moved.dot3(normal);
        return _mm_cvtss_f32(_mm_sub_ss(_mm_setzero_ps(), dot.getQuad()));
    };

    alignas(16) char shapeBuffer[512];
    alignas(16) hkTransform childTransform;
    switch (shape->m_type.m_storage) {
        // No convex radius to show.
        case HK_SHAPE_SPHERE:
        case HK_SHAPE_CAPSULE:
        case HK_SHAPE_SAMPLED_HEIGHT_FIELD:
        case HK_SHAPE_MULTI_SPHERE:
        case HK_SHAPE_MULTI_RAY:
        case HK_SHAPE_PLANE: break;

        case HK_SHAPE_CYLINDER: {
            const hkpCylinderShape* cylinder = static_cast<const hkpCylinderShape*>(shape);
            // hkpCylinderShape::getCylinderRadius, a trivial accessor.
            const float cylRadius = cylinder->m_cylRadius;
            const float convexRadius = cylinder->m_radius;
            if (m_environment.m_minVisibleRadius < convexRadius) {
                hkVector4 top;
                top.setTransformedPos(transform, cylinder->m_vertexB);
                hkVector4 bottom;
                bottom.setTransformedPos(transform, cylinder->m_vertexA);
                // The axis, scaled to the convex radius, lengthens the cylinder at both ends.
                hkVector4 axis;
                axis.setSub4(top, bottom);
                axis.normalize3();
                axis.m_quad = _mm_mul_ps(axis.m_quad, _mm_set1_ps(convexRadius));
                top.m_quad = _mm_add_ps(axis.m_quad, top.m_quad);
                bottom.m_quad = _mm_sub_ps(bottom.m_quad, axis.m_quad);
                // EXE-INLINED none: hkReferencedObject::operator new
                void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkDisplayCylinder));
                static_cast<hkReferencedObject*>(pMemory)->m_memSizeAndFlags = sizeof(hkDisplayCylinder);
                hkDisplayCylinder* display =
                    ::new (pMemory) hkDisplayCylinder(top, bottom, convexRadius + cylRadius, 9, 1);
                display->m_transform.Assign(transform);
                displayGeometries.pushBack(display);
            }
            break;
        }

        case HK_SHAPE_TRIANGLE: {
            const hkpTriangleShape* triangle = static_cast<const hkpTriangleShape*>(shape);
            const float radius = triangle->m_radius;
            if (!(m_environment.m_minVisibleRadius < radius)) {
                break;
            }
            const hkVector4& v0 = triangle->m_vertexA;
            const hkVector4& v1 = triangle->m_vertexB;
            const hkVector4& v2 = triangle->m_vertexC;
            if (hkpTriangleUtil::isDegenerate(v0, v1, v2, 0.001f).m_bool) {
                break;
            }
            // The edge directions and the face normal.
            hkVector4 edge01;
            edge01.setSub4(v1, v0);
            edge01.normalize3();
            hkVector4 edge12;
            edge12.setSub4(v2, v1);
            edge12.normalize3();
            hkVector4 edge02;
            edge02.setSub4(v2, v0);
            edge02.normalize3();
            hkVector4 normal;
            normal.setCross(edge01, edge02);

            alignas(16) hkArray<hkVector4> planes;
            int32_t numAllocated = 8;
            hkVector4* pPlanes = hkMemoryRouter::getInstance().m_heap->_bufAlloc<hkVector4>(numAllocated);
            planes.m_data = pPlanes;
            planes.m_size = numAllocated;
            planes.m_capacityAndFlags =
                numAllocated ? numAllocated : static_cast<int32_t>(hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG);
            planes.setSize(8);

            // The two faces, the three edges' outward planes, and the three corners' planes between those, each moved
            // out by the radius.
            planes.m_data[0] = normal;
            planes.m_data[0](3) = planeDistance(planes.m_data[0], v0, radius);
            planes.m_data[1].setNeg4(normal);
            planes.m_data[1](3) = planeDistance(planes.m_data[1], v0, radius);
            planes.m_data[2].setCross(edge01, normal);
            planes.m_data[2](3) = planeDistance(planes.m_data[2], v0, radius);
            planes.m_data[3].setCross(edge12, normal);
            planes.m_data[3](3) = planeDistance(planes.m_data[3], v1, radius);
            // normal x edge02, with edge02's lanes as the first operand of each product, as the game does (a NaN
            // operand's payload depends on the order).
            planes.m_data[4].m_quad = _mm_sub_ps(
                _mm_mul_ps(
                    _mm_shuffle_ps(edge02.m_quad, edge02.m_quad, _MM_SHUFFLE(3, 1, 0, 2)),
                    _mm_shuffle_ps(normal.m_quad, normal.m_quad, _MM_SHUFFLE(3, 0, 2, 1))
                ),
                _mm_mul_ps(
                    _mm_shuffle_ps(edge02.m_quad, edge02.m_quad, _MM_SHUFFLE(3, 0, 2, 1)),
                    _mm_shuffle_ps(normal.m_quad, normal.m_quad, _MM_SHUFFLE(3, 1, 0, 2))
                )
            );
            planes.m_data[4](3) = planeDistance(planes.m_data[4], v2, radius);
            planes.m_data[5].setAdd4(planes.m_data[3], planes.m_data[2]);
            planes.m_data[5].normalize3();
            planes.m_data[5](3) = planeDistance(planes.m_data[5], v1, radius);
            planes.m_data[6].setAdd4(planes.m_data[4], planes.m_data[3]);
            planes.m_data[6].normalize3();
            planes.m_data[6](3) = planeDistance(planes.m_data[6], v2, radius);
            planes.m_data[7].setAdd4(planes.m_data[4], planes.m_data[2]);
            planes.m_data[7].normalize3();
            planes.m_data[7](3) = planeDistance(planes.m_data[7], v0, radius);

            hkDisplayConvex* display = createConvexDisplayFromPoints(planes, transform);
            if (display) {
                displayGeometries.pushBack(display);
            }
            break;
        }

        case HK_SHAPE_BOX: {
            const hkpBoxShape* box = static_cast<const hkpBoxShape*>(shape);
            const float radius = box->m_radius;
            if (m_environment.m_minVisibleRadius < radius) {
                hkVector4 halfExtents;
                halfExtents.m_quad = _mm_add_ps(_mm_set1_ps(radius), box->m_halfExtents.m_quad);
                // EXE-INLINED none: hkReferencedObject::operator new
                void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkDisplayBox));
                static_cast<hkReferencedObject*>(pMemory)->m_memSizeAndFlags = sizeof(hkDisplayBox);
                hkDisplayBox* display = ::new (pMemory) hkDisplayBox(halfExtents);
                display->m_transform.Assign(transform);
                displayGeometries.pushBack(display);
            }
            break;
        }

        case HK_SHAPE_CONVEX_VERTICES: {
            const hkpConvexVerticesShape* convexVertices = static_cast<const hkpConvexVerticesShape*>(shape);
            const float radius = convexVertices->m_radius;
            if (!(m_environment.m_minVisibleRadius < radius)) {
                break;
            }
            // getPlaneEquations, a trivial accessor: the shape's plane equations.
            const hkArray<hkVector4>* planeEquations = &convexVertices->m_planeEquations;
            const float maxDistance = static_cast<float>(_getPlaneToSurfaceDistance(shape));

            alignas(16) hkArray<hkVector4> planes;
            planes = *planeEquations;
            const int32_t numPlanes = planes.getSize();

            // The planes move out by the radius unless the vertices already lie that far out. The radius is subtracted
            // from the double 0.005f and the result is compared as a float.
            const float threshold = 0.004999999888241291 - radius;
            if (threshold < maxDistance) {
                for (int32_t i = 0; i < numPlanes; i++) {
                    planes.m_data[i](3) = planes.m_data[i](3) - radius;
                }
            }
            addAabbPlanes(shape, radius, planes);

            hkDisplayConvex* display = createConvexDisplayFromPoints(planes, transform);
            if (display) {
                displayGeometries.pushBack(display);
            }
            break;
        }

        case HK_SHAPE_COLLECTION:
        case HK_SHAPE_LIST:
        case HK_SHAPE_CONVEX_LIST:
        case HK_SHAPE_TRIANGLE_COLLECTION: {
            const hkpShapeContainer* container = shape->getContainer();
            for (uint32_t key = container->getFirstKey(); key != HK_INVALID_SHAPE_KEY;
                 key = container->getNextKey(key)) {
                buildShapeDisplay(container->getChildShape(key, shapeBuffer), transform, displayGeometries);
            }
            break;
        }

        case HK_SHAPE_BV_TREE:
        case HK_SHAPE_MOPP: {
            const hkpShapeContainer* container = shape->getContainer();
            for (uint32_t key = container->getFirstKey(); key != HK_INVALID_SHAPE_KEY;
                 key = container->getNextKey(key)) {
                buildShapeDisplay(container->getChildShape(key, shapeBuffer), transform, displayGeometries);
            }
            break;
        }

        case HK_SHAPE_CONVEX_TRANSLATE: {
            const hkpConvexTranslateShape* translateShape = static_cast<const hkpConvexTranslateShape*>(shape);
            // The shape's translation with an identity rotation.
            alignas(16) hkTransform translation;
            translation.m_rotation.setIdentity();
            translation.m_translation = translateShape->m_translation;
            childTransform.setMul(transform, translation);
            buildShapeDisplay(translateShape->getChildShape(), childTransform, displayGeometries);
            break;
        }

        case HK_SHAPE_CONVEX_TRANSFORM: {
            const hkpConvexTransformShape* transformShape = static_cast<const hkpConvexTransformShape*>(shape);
            childTransform.setMul(transform, transformShape->m_transform);
            buildShapeDisplay(transformShape->getChildShape(), childTransform, displayGeometries);
            break;
        }

        case HK_SHAPE_TRANSFORM: {
            const hkpTransformShape* transformShape = static_cast<const hkpTransformShape*>(shape);
            childTransform.setMul(transform, transformShape->m_transform);
            buildShapeDisplay(transformShape->getChildShape(), childTransform, displayGeometries);
            break;
        }

        case HK_SHAPE_CONVEX_PIECE: {
            const hkpConvexPieceShape* pieceShape = static_cast<const hkpConvexPieceShape*>(shape);
            if (m_environment.m_minVisibleRadius < pieceShape->m_radius) {
                for (int32_t i = 0; i < pieceShape->m_numDisplayShapeKeys; i++) {
                    const hkpShape* child =
                        pieceShape->m_displayMesh->getChildShape(pieceShape->m_displayShapeKeys[i], shapeBuffer);
                    buildShapeDisplay(child, transform, displayGeometries);
                }
            }
            break;
        }

        case HK_SHAPE_BV: {
            const hkpBvShape* bvShape = static_cast<const hkpBvShape*>(shape);
            buildShapeDisplay(bvShape->m_boundingVolumeShape, transform, displayGeometries);
            break;
        }

        // Any other type: the user builders registered for it.
        default: {
            for (int32_t i = 0; i < hkpUserConvexRadiusBuilder::s_instance->m_userConvexRadiusBuilders.m_size; i++) {
                const hkpUserConvexRadiusBuilder::UserShapeBuilder& builder =
                    hkpUserConvexRadiusBuilder::s_instance->m_userConvexRadiusBuilders.m_data[i];
                if (static_cast<uint32_t>(builder.type) == shape->m_type.m_storage) {
                    builder.f(shape, transform, displayGeometries, this);
                }
            }
            break;
        }
    }
}

void hkpConvexRadiusBuilder::resetCurrentRawGeometry() {
    m_currentGeometry = nullptr;
}

hkDisplayGeometry* hkpConvexRadiusBuilder::getCurrentRawGeometry(
    hkArray<hkDisplayGeometry*, hkContainerHeapAllocator>& displayGeometries
) {
    hkGeometry* v3 = nullptr;
    if (!m_currentGeometry) {
        hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
        hkGeometry* v5 = static_cast<hkGeometry*>(Instance->m_heap->blockAlloc(sizeof(hkGeometry)));
        if (v5) {
            v5->m_vertices.m_data = nullptr;
            v5->m_vertices.m_size = 0;
            v5->m_vertices.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
            v5->m_triangles.m_data = nullptr;
            v5->m_triangles.m_size = 0;
            v5->m_triangles.m_capacityAndFlags = hkArrayBase<hkGeometry::Triangle>::DONT_DEALLOCATE_FLAG;
            v3 = v5;
        }
        hkMemoryRouter* v6 = &hkMemoryRouter::getInstance();
        hkDisplayConvex* v7 = static_cast<hkDisplayConvex*>(v6->m_heap->blockAlloc(sizeof(hkDisplayConvex)));
        v7->m_memSizeAndFlags = sizeof(hkDisplayConvex);
        m_currentGeometry = ::new (v7) hkDisplayConvex(v3);
        hkMemoryRouter* v8 = &hkMemoryRouter::getInstance();
        if (displayGeometries.m_size ==
            (displayGeometries.m_capacityAndFlags & hkArrayBase<hkDisplayGeometry*>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(*v8->m_heap, &displayGeometries, sizeof(hkDisplayGeometry*));
        }
        displayGeometries.m_data[displayGeometries.m_size++] = m_currentGeometry;
    }
    return m_currentGeometry;
}

hkDisplayConvex*
hkpConvexRadiusBuilder::createConvexDisplayFromPoints(const hkArray<hkVector4>& points, const hkTransform& transform) {
    hkArray<hkVector4> hullVertices;
    createVerticesFromPlaneEquations(points, hullVertices);
    const int32_t numVertices = hullVertices.m_size;
    if (numVertices < 1) {
        return nullptr;
    }

    hkArray<hkVector4> transformedVertices;
    transformedVertices.setSize(numVertices);
    for (int32_t i = 0; i < numVertices; i++) {
        transformedVertices.m_data[i].setTransformedPos(transform, hullVertices.m_data[i]);
    }

    hkGeometry* geometry =
        static_cast<hkGeometry*>(hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkGeometry)));
    if (geometry) {
        // EXE-INLINED none: hkGeometry::hkGeometry
        geometry->m_vertices.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
        geometry->m_vertices.m_data = nullptr;
        geometry->m_vertices.m_size = 0;
        geometry->m_triangles.m_capacityAndFlags = hkArrayBase<hkGeometry::Triangle>::DONT_DEALLOCATE_FLAG;
        geometry->m_triangles.m_data = nullptr;
        geometry->m_triangles.m_size = 0;
    }
    // EXE-INLINED none: hkStridedVertices::set
    hkStridedVertices stridedVertices;
    stridedVertices.m_numVertices = transformedVertices.m_size;
    stridedVertices.m_striding = sizeof(hkVector4);
    // The vertex array's storage, read as the floats it holds.
    stridedVertices.m_vertices = reinterpret_cast<const float*>(transformedVertices.m_data);
    // The allocation is passed on unchecked, as the game does.
    hkGeometryUtility::createConvexGeometry(stridedVertices, *geometry);

    // EXE-INLINED none: hkReferencedObject::operator new
    void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkDisplayConvex));
    static_cast<hkReferencedObject*>(pMemory)->m_memSizeAndFlags = sizeof(hkDisplayConvex);
    hkDisplayConvex* display = ::new (pMemory) hkDisplayConvex(geometry);

    return display;
}

// The builders array goes with its member.
hkpUserConvexRadiusBuilder::~hkpUserConvexRadiusBuilder() {}

// Adds f for type unless a builder for type is registered already.
void hkpUserConvexRadiusBuilder::registerUserConvexRadiusDisplayBuilder(
    void (*f)(
        const hkpShape*,
        const hkTransform&,
        hkArray<hkDisplayGeometry*, hkContainerHeapAllocator>&,
        hkpConvexRadiusBuilder*
    ),
    hkpShapeType type
) {
    const int32_t size = m_userConvexRadiusBuilders.m_size;
    for (int32_t i = 0; i < size; ++i) {
        if (m_userConvexRadiusBuilders.m_data[i].type == type) {
            return;
        }
    }
    m_userConvexRadiusBuilders.pushBack(UserShapeBuilder{f, type});
}
