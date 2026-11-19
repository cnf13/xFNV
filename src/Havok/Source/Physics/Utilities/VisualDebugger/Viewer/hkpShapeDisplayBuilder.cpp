#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/hkpShapeDisplayBuilder.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Geometry/Sphere/hkSphere.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkStridedVertices.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/ConvexHull/hkGeometryUtility.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayBox.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayCapsule.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayConvex.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayCylinder.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometry.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayPlane.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplaySphere.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplay.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/List/hkpListShape.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Box/hkpBoxShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Capsule/hkpCapsuleShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/ConvexTransform/hkpConvexTransformShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/ConvexTranslate/hkpConvexTranslateShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/ConvexVertices/hkpConvexVerticesShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Cylinder/hkpCylinderShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Sphere/hkpSphereShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Triangle/hkpTriangleShape.h"
#include "Havok/Source/Physics/Collide/Shape/Deprecated/ConvexPieceMesh/hkpConvexPieceShape.h"
#include "Havok/Source/Physics/Collide/Shape/Deprecated/MultiSphere/hkpMultiSphereShape.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/Plane/hkpPlaneShape.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/SampledHeightField/hkpSampledHeightFieldShape.h"
#include "Havok/Source/Physics/Collide/Shape/Misc/Bv/hkpBvShape.h"
#include "Havok/Source/Physics/Collide/Shape/Misc/MultiRay/hkpMultiRayShape.h"
#include "Havok/Source/Physics/Collide/Shape/Misc/Transform/hkpTransformShape.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayBundleCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeBuffer.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Utilities/Dynamics/Inertia/hkpInertiaTensorComputer.h"
#include "SDK/MSVC/string.h"

#include <bit>
#include <xmmintrin.h>

// Havok SDK signature (hkpUtilities.lib, tied by data).
static hkReferencedObject* hkpUserShapeDisplayBuildercreate();

// m_value is untyped storage: populate and depopulate move any singleton's instance pointer through it.
static hkSingletonInitNode hkSingletonRegisterhkpUserShapeDisplayBuilder(
    "hkpUserShapeDisplayBuilder",
    &hkpUserShapeDisplayBuildercreate,
    reinterpret_cast<void**>(&hkpUserShapeDisplayBuilder::s_instance)
);

// The singleton's create function (HK_SINGLETON_IMPLEMENTATION: new hkpUserShapeDisplayBuilder()), with the constructor
// written out in the body.
static hkReferencedObject* hkpUserShapeDisplayBuildercreate() {
    // EXE-INLINED 0x56D280
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkpUserShapeDisplayBuilder* pBuilder =
        static_cast<hkpUserShapeDisplayBuilder*>(Instance->m_heap->blockAlloc(sizeof(hkpUserShapeDisplayBuilder)));
    pBuilder->m_memSizeAndFlags = sizeof(hkpUserShapeDisplayBuilder);
    return ::new (pBuilder) hkpUserShapeDisplayBuilder;
}

hkpShapeDisplayBuilder::hkpShapeDisplayBuilderEnvironment::hkpShapeDisplayBuilderEnvironment()
  : m_spherePhiRes(8), m_sphereThetaRes(8) {}

hkpShapeDisplayBuilder::hkpShapeDisplayBuilder(const hkpShapeDisplayBuilderEnvironment& env)
  : m_environment(env), m_currentGeometry(nullptr) {}

// The builder destroys nothing of its own.
hkpShapeDisplayBuilder::~hkpShapeDisplayBuilder() {}

void hkpShapeDisplayBuilder::buildDisplayGeometries(
    const hkpShape* shape,
    hkArray<hkDisplayGeometry*>& displayGeometries
) {
    // EXE-INLINED none: hkTransform::setIdentity
    alignas(16) hkTransform transform;
    const __m128 kZero = _mm_setzero_ps();
    _mm_store_ps(&reinterpret_cast<float*>(&transform.m_rotation.m_col0.m_quad)[0], kZero);
    transform.m_rotation.m_col0.m_quad[0] = 1.0f;
    _mm_store_ps(&reinterpret_cast<float*>(&transform.m_rotation.m_col1.m_quad)[0], kZero);
    transform.m_rotation.m_col1.m_quad[1] = 1.0f;
    _mm_store_ps(&reinterpret_cast<float*>(&transform.m_rotation.m_col2.m_quad)[0], kZero);
    transform.m_rotation.m_col2.m_quad[2] = 1.0f;
    _mm_store_ps(&reinterpret_cast<float*>(&transform.m_translation.m_quad)[0], kZero);
    resetCurrentRawGeometry();
    // EXE-INLINED none: hkArrayBase<hkDisplayGeometry*>::clear
    displayGeometries.m_size = 0;
    buildShapeDisplay(shape, transform, displayGeometries);
}

// Switch over the shape type. A sphere, cylinder, box, capsule or plane gives one display geometry, placed at transform
// and appended after the switch; every other case appends its own. As the game does, the cylinder's end points are
// transformed and its geometry placed at transform as well, and a multi-ray shape's middle vertex (the start moved
// along x) stays untransformed, its triangles indexed as if the geometry had started empty.
void hkpShapeDisplayBuilder::buildShapeDisplay(
    const hkpShape* shape,
    const hkTransform& transform,
    hkArray<hkDisplayGeometry*>& displayGeometries
) {
    hkDisplayGeometry* displayGeometry;
    switch (shape->m_type.m_storage) {
        case HK_SHAPE_SPHERE: {
            const hkpSphereShape* sphereShape = static_cast<const hkpSphereShape*>(shape);
            // EXE-INLINED none: hkSphere::hkSphere
            hkSphere sphere;
            sphere.m_pos.setZero4();
            sphere.m_pos(3) = sphereShape->m_radius;
            // EXE-INLINED 0x56D280
            hkDisplaySphere* displaySphere =
                static_cast<hkDisplaySphere*>(hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkDisplaySphere)));
            displaySphere->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(hkDisplaySphere));
            displayGeometry = ::new (displaySphere)
                hkDisplaySphere(sphere, m_environment.m_sphereThetaRes, m_environment.m_spherePhiRes);
            break;
        }
        case HK_SHAPE_CYLINDER: {
            const hkpCylinderShape* cylinder = static_cast<const hkpCylinderShape*>(shape);
            hkVector4 top;
            top.setTransformedPos(transform, cylinder->m_vertexB);
            hkVector4 bottom;
            bottom.setTransformedPos(transform, cylinder->m_vertexA);
            // EXE-INLINED 0x56D280
            hkDisplayCylinder* displayCylinder = static_cast<hkDisplayCylinder*>(
                hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkDisplayCylinder))
            );
            displayCylinder->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(hkDisplayCylinder));
            displayGeometry =
                ::new (displayCylinder) hkDisplayCylinder(top, bottom, cylinder->getCylinderRadius(), 9, 1);
            break;
        }
        case HK_SHAPE_TRIANGLE: {
            const hkpTriangleShape* triangle = static_cast<const hkpTriangleShape*>(shape);
            hkGeometry* geom = getCurrentRawGeometry(displayGeometries)->m_geometry;
            const int32_t numVertices = geom->m_vertices.m_size;
            hkVector4* vertices = geom->m_vertices.expandBy(3);
            vertices[0].setTransformedPos(transform, triangle->m_vertexA);
            vertices[1].setTransformedPos(transform, triangle->m_vertexB);
            vertices[2].setTransformedPos(transform, triangle->m_vertexC);
            hkGeometry::Triangle* face = geom->m_triangles.expandBy(1);
            face->m_a = numVertices;
            face->m_b = numVertices + 1;
            face->m_c = numVertices + 2;
            face->m_material = -1;
            // EXE-INLINED 0x630B40: hkVector4::length3 (z + (y + x), then the square root)
            const hkQuadReal squares = _mm_mul_ps(triangle->m_extrusion.m_quad, triangle->m_extrusion.m_quad);
            const float extrusionLength = _mm_cvtss_f32(_mm_sqrt_ss(_mm_add_ss(
                _mm_shuffle_ps(squares, squares, 0xAA), _mm_add_ss(_mm_shuffle_ps(squares, squares, 0x55), squares)
            )));
            if (extrusionLength > 0.0f) {
                // The extruded copy of the triangle and the six side triangles joining it to the original.
                hkVector4 extrudedA;
                extrudedA.setAdd4(triangle->m_extrusion, triangle->m_vertexA);
                hkVector4 extrudedB;
                extrudedB.setAdd4(triangle->m_vertexB, triangle->m_extrusion);
                hkVector4 extrudedC;
                extrudedC.setAdd4(triangle->m_vertexC, triangle->m_extrusion);
                hkVector4* extruded = geom->m_vertices.expandBy(3);
                extruded[0].setTransformedPos(transform, extrudedA);
                extruded[1].setTransformedPos(transform, extrudedB);
                extruded[2].setTransformedPos(transform, extrudedC);
                hkGeometry::Triangle* sides = geom->m_triangles.expandBy(7);
                const int32_t n = numVertices;
                sides[0].m_a = n + 3;
                sides[0].m_b = n + 1;
                sides[0].m_c = n;
                sides[0].m_material = -1;
                sides[1].m_a = n + 3;
                sides[1].m_b = n + 4;
                sides[1].m_c = n + 1;
                sides[1].m_material = -1;
                sides[2].m_a = n + 4;
                sides[2].m_b = n + 5;
                sides[2].m_c = n + 1;
                sides[2].m_material = -1;
                sides[3].m_a = n + 5;
                sides[3].m_b = n + 2;
                sides[3].m_c = n + 1;
                sides[3].m_material = -1;
                sides[4].m_a = n + 5;
                sides[4].m_b = n;
                sides[4].m_c = n + 2;
                sides[4].m_material = -1;
                sides[5].m_a = n + 5;
                sides[5].m_b = n + 3;
                sides[5].m_c = n;
                sides[5].m_material = -1;
                sides[6].m_a = n + 5;
                sides[6].m_b = n + 4;
                sides[6].m_c = n + 3;
                sides[6].m_material = -1;
            }
            return;
        }
        case HK_SHAPE_BOX: {
            // EXE-INLINED 0x56D280
            hkDisplayBox* displayBox =
                static_cast<hkDisplayBox*>(hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkDisplayBox)));
            displayBox->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(hkDisplayBox));
            displayGeometry = ::new (displayBox) hkDisplayBox(static_cast<const hkpBoxShape*>(shape)->m_halfExtents);
            break;
        }
        case HK_SHAPE_CAPSULE: {
            const hkpCapsuleShape* capsule = static_cast<const hkpCapsuleShape*>(shape);
            // EXE-INLINED 0x56D280
            hkDisplayCapsule* displayCapsule = static_cast<hkDisplayCapsule*>(
                hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkDisplayCapsule))
            );
            displayCapsule->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(hkDisplayCapsule));
            displayGeometry = ::new (displayCapsule)
                hkDisplayCapsule(capsule->m_vertexB, capsule->m_vertexA, capsule->m_radius, 6, 1);
            break;
        }
        case HK_SHAPE_CONVEX_VERTICES: {
            const hkpConvexVerticesShape* convexVertices = static_cast<const hkpConvexVerticesShape*>(shape);
            const int32_t numSpheres = convexVertices->getNumCollisionSpheresImpl();
            alignas(16) hkLocalArray<hkSphere> sphereBuffer;
            {
                // EXE-INLINED none: hkLocalArray<hkSphere>::hkLocalArray(numSpheres) (hkAllocateStack)
                sphereBuffer.m_data = nullptr;
                sphereBuffer.m_size = 0;
                sphereBuffer.m_capacityAndFlags = hkArrayBase<hkSphere>::DONT_DEALLOCATE_FLAG;
                hkStackMemory& stack = hkMemoryRouter::getInstance().m_stack;
                char* pBlock = stack.m_next.m_storage;
                stack.m_next.m_storage = pBlock + ((numSpheres * static_cast<int32_t>(sizeof(hkSphere)) + 127) & ~127);
                // The thread's stack allocator hands out untyped bytes; the array's storage is that block.
                sphereBuffer.m_data = reinterpret_cast<hkSphere*>(pBlock);
                sphereBuffer.m_capacityAndFlags = numSpheres | hkArrayBase<hkSphere>::DONT_DEALLOCATE_FLAG;
                sphereBuffer.m_localMemory = sphereBuffer.m_data;
            }
            // EXE-INLINED none: hkArray<hkSphere>::setSize (hkSphere's construct is empty)
            sphereBuffer.reserve(numSpheres);
            sphereBuffer.setSizeUnchecked(numSpheres);
            const hkSphere* spheres = convexVertices->getCollisionSpheresImpl(sphereBuffer.m_data);

            alignas(16) hkArray<hkVector4> transformedVertices;
            transformedVertices.m_data = nullptr;
            transformedVertices.m_size = 0;
            transformedVertices.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
            transformedVertices.setSize(numSpheres);
            for (int32_t i = 0; i < numSpheres; i++) {
                transformedVertices.m_data[i].setTransformedPos(transform, spheres[i].m_pos);
            }

            // EXE-INLINED 0xCCE5E0: hkGeometry::hkGeometry
            hkGeometry* geom =
                static_cast<hkGeometry*>(hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkGeometry)));
            if (geom) {
                geom->m_vertices.m_data = nullptr;
                geom->m_vertices.m_size = 0;
                geom->m_vertices.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
                geom->m_triangles.m_data = nullptr;
                geom->m_triangles.m_size = 0;
                geom->m_triangles.m_capacityAndFlags = hkArrayBase<hkGeometry::Triangle>::DONT_DEALLOCATE_FLAG;
            }
            hkStridedVertices stridedVertices;
            stridedVertices.m_vertices = &transformedVertices.m_data[0](0);
            stridedVertices.m_numVertices = transformedVertices.m_size;
            stridedVertices.m_striding = sizeof(hkVector4);
            hkGeometryUtility::createConvexGeometry(stridedVertices, *geom);

            // EXE-INLINED 0x56D280
            hkDisplayConvex* displayConvex =
                static_cast<hkDisplayConvex*>(hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkDisplayConvex)));
            displayConvex->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(hkDisplayConvex));
            displayGeometry = ::new (displayConvex) hkDisplayConvex(geom);
            {
                // EXE-INLINED none: hkArray<hkDisplayGeometry*>::pushBack
                hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
                if (displayGeometries.m_size ==
                    (displayGeometries.m_capacityAndFlags & hkArrayBase<hkDisplayGeometry*>::CAPACITY_MASK)) {
                    hkArrayUtil::_reserveMore(heap, &displayGeometries, sizeof(hkDisplayGeometry*));
                }
                displayGeometries.m_data[displayGeometries.m_size++] = displayGeometry;
            }

            {
                // EXE-INLINED none: hkArray<hkVector4>::~hkArray
                hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
                const int32_t capacityAndFlags = transformedVertices.m_capacityAndFlags;
                transformedVertices.m_size = 0;
                if (capacityAndFlags >= 0) {
                    heap.bufFree(
                        transformedVertices.m_data,
                        static_cast<int32_t>(
                            (capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK) * sizeof(hkVector4)
                        )
                    );
                }
                transformedVertices.m_data = nullptr;
                transformedVertices.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
            }
            return;
        }
        case HK_SHAPE_COLLECTION:
        case HK_SHAPE_EXTENDED_MESH:
        case HK_SHAPE_COMPRESSED_MESH:
        case HK_SHAPE_CONVEX_LIST:
        case HK_SHAPE_TRIANGLE_COLLECTION: {
            const hkpShapeContainer* container = shape->getContainer();
            alignas(16) char buffer[HK_SHAPE_BUFFER_SIZE];
            for (uint32_t key = container->getFirstKey(); key != HK_INVALID_SHAPE_KEY;
                 key = container->getNextKey(key)) {
                buildShapeDisplay(container->getChildShape(key, buffer), transform, displayGeometries);
            }
            return;
        }
        case HK_SHAPE_BV_TREE:
        case HK_SHAPE_MOPP: {
            const hkpShapeContainer* container = shape->getContainer();
            alignas(16) char buffer[HK_SHAPE_BUFFER_SIZE];
            for (uint32_t key = container->getFirstKey(); key != HK_INVALID_SHAPE_KEY;
                 key = container->getNextKey(key)) {
                buildShapeDisplay(container->getChildShape(key, buffer), transform, displayGeometries);
            }
            return;
        }
        case HK_SHAPE_LIST: {
            const hkpListShape* list = static_cast<const hkpListShape*>(shape);
            for (uint32_t key = list->getFirstKey(); key != HK_INVALID_SHAPE_KEY; key = list->getNextKey(key)) {
                // EXE-INLINED 0x4D8610: hkpListShape::getChildShapeInl
                buildShapeDisplay(list->m_childInfo.m_data[key].m_shape, transform, displayGeometries);
            }
            return;
        }
        case HK_SHAPE_CONVEX_TRANSLATE: {
            const hkpConvexTranslateShape* translateShape = static_cast<const hkpConvexTranslateShape*>(shape);
            // EXE-INLINED 0x577070: hkTransform::setIdentity
            hkTransform translation;
            translation.m_rotation.m_col0.setZero4();
            translation.m_rotation.m_col0(0) = 1.0f;
            translation.m_rotation.m_col1.setZero4();
            translation.m_rotation.m_col1(1) = 1.0f;
            translation.m_rotation.m_col2.setZero4();
            translation.m_rotation.m_col2(2) = 1.0f;
            translation.m_translation.setZero4();
            translation.m_translation = translateShape->m_translation;
            hkTransform childTransform;
            childTransform.setMul(transform, translation);
            buildShapeDisplay(translateShape->getChildShape(), childTransform, displayGeometries);
            return;
        }
        case HK_SHAPE_CONVEX_TRANSFORM: {
            const hkpConvexTransformShape* transformShape = static_cast<const hkpConvexTransformShape*>(shape);
            hkTransform childTransform;
            childTransform.setMul(transform, transformShape->m_transform);
            buildShapeDisplay(transformShape->getChildShape(), childTransform, displayGeometries);
            return;
        }
        case HK_SHAPE_SAMPLED_HEIGHT_FIELD: {
            const hkpSampledHeightFieldShape* heightField = static_cast<const hkpSampledHeightFieldShape*>(shape);
            hkGeometry* geom = getCurrentRawGeometry(displayGeometries)->m_geometry;
            const hkVector4 scale = heightField->m_intToFloatScale;
            for (int32_t i = 0; i < heightField->m_xRes - 1; i++) {
                // Converts each grid index to a float, then adds 0.0 or 1.0 and stores the sum as a float.
                const float fi = static_cast<float>(i);
                const float x0 = fi + 0.0f;
                const float x1 = fi + 1.0f;
                for (int32_t j = 0; j < heightField->m_zRes - 1; j++) {
                    const float fj = static_cast<float>(j);
                    const float z0 = fj + 0.0f;
                    const float z1 = fj + 1.0f;
                    hkVector4 p00;
                    p00.set(x0, heightField->getHeightAt(i, j), z0, 0.0f);
                    p00.mul4(scale);
                    hkVector4 p01;
                    p01.set(x0, heightField->getHeightAt(i, j + 1), z1, 0.0f);
                    p01.mul4(scale);
                    hkVector4 p10;
                    p10.set(x1, heightField->getHeightAt(i + 1, j), z0, 0.0f);
                    p10.mul4(scale);
                    hkVector4 p11;
                    p11.set(x1, heightField->getHeightAt(i + 1, j + 1), z1, 0.0f);
                    p11.mul4(scale);

                    const int32_t numVertices = geom->m_vertices.m_size;
                    hkVector4* vertices = geom->m_vertices.expandBy(4);
                    vertices[0].setTransformedPos(transform, p00);
                    vertices[1].setTransformedPos(transform, p01);
                    vertices[2].setTransformedPos(transform, p10);
                    vertices[3].setTransformedPos(transform, p11);
                    if (heightField->getTriangleFlip()) {
                        hkGeometry::Triangle* faces = geom->m_triangles.expandBy(2);
                        faces[0].m_a = numVertices;
                        faces[0].m_b = numVertices + 1;
                        faces[0].m_c = numVertices + 3;
                        faces[0].m_material = -1;
                        faces[1].m_a = numVertices;
                        faces[1].m_b = numVertices + 3;
                        faces[1].m_c = numVertices + 2;
                        faces[1].m_material = -1;
                    }
                    else {
                        hkGeometry::Triangle* faces = geom->m_triangles.expandBy(2);
                        faces[0].m_a = numVertices;
                        faces[0].m_b = numVertices + 1;
                        faces[0].m_c = numVertices + 2;
                        faces[0].m_material = -1;
                        faces[1].m_a = numVertices + 3;
                        faces[1].m_b = numVertices + 2;
                        faces[1].m_c = numVertices + 1;
                        faces[1].m_material = -1;
                    }
                }
            }
            return;
        }
        case HK_SHAPE_TRANSFORM: {
            const hkpTransformShape* transformShape = static_cast<const hkpTransformShape*>(shape);
            hkTransform childTransform;
            childTransform.setMul(transform, transformShape->m_transform);
            buildShapeDisplay(transformShape->getChildShape(), childTransform, displayGeometries);
            return;
        }
        case HK_SHAPE_CONVEX_PIECE: {
            const hkpConvexPieceShape* convexPiece = static_cast<const hkpConvexPieceShape*>(shape);
            hkGeometry* geom = nullptr;
            if (void* pMemory = hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkGeometry))) {
                geom = ::new (pMemory) hkGeometry;
            }
            alignas(16) char buffer[HK_SHAPE_BUFFER_SIZE];
            for (int32_t i = 0; i < convexPiece->m_numDisplayShapeKeys; i++) {
                const hkpTriangleShape* triangle = static_cast<const hkpTriangleShape*>(
                    convexPiece->m_displayMesh->getChildShape(convexPiece->m_displayShapeKeys[i], buffer)
                );
                hkGeometry::Triangle* face = geom->m_triangles.expandBy(1);
                const int32_t vertexIndex = geom->m_vertices.m_size;
                face->m_a = vertexIndex;
                face->m_b = vertexIndex + 1;
                face->m_c = vertexIndex + 2;
                face->m_material = -1;
                for (int32_t j = 0; j < 3; j++) {
                    hkVector4* vertex = geom->m_vertices.expandBy(1);
                    vertex->setTransformedPos(transform, triangle->getVertex(j));
                }
            }
            // EXE-INLINED 0x56D280
            hkDisplayConvex* displayConvex =
                static_cast<hkDisplayConvex*>(hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkDisplayConvex)));
            displayConvex->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(hkDisplayConvex));
            displayGeometry = ::new (displayConvex) hkDisplayConvex(geom);
            // EXE-INLINED none: hkArray<hkDisplayGeometry*>::pushBack
            hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
            if (displayGeometries.m_size ==
                (displayGeometries.m_capacityAndFlags & hkArrayBase<hkDisplayGeometry*>::CAPACITY_MASK)) {
                hkArrayUtil::_reserveMore(heap, &displayGeometries, sizeof(hkDisplayGeometry*));
            }
            displayGeometries.m_data[displayGeometries.m_size++] = displayGeometry;
            return;
        }
        case HK_SHAPE_MULTI_SPHERE: {
            const hkpMultiSphereShape* multiSphere = static_cast<const hkpMultiSphereShape*>(shape);
            const hkVector4* spheres = multiSphere->m_spheres;
            for (int32_t i = 0; i < multiSphere->m_numSpheres; i++) {
                // EXE-INLINED none: hkSphere::hkSphere
                hkSphere sphere;
                sphere.m_pos = hkVector4::getZero();
                sphere.m_pos(3) = spheres[i](3);
                // EXE-INLINED 0x56D280
                hkDisplaySphere* displaySphere = static_cast<hkDisplaySphere*>(
                    hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkDisplaySphere))
                );
                displaySphere->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(hkDisplaySphere));
                displaySphere = ::new (displaySphere)
                    hkDisplaySphere(sphere, m_environment.m_sphereThetaRes, m_environment.m_spherePhiRes);
                // EXE-INLINED 0x577070: hkTransform::setIdentity
                hkTransform& sphereTransform = displaySphere->m_transform;
                sphereTransform.m_rotation.m_col0.setZero4();
                sphereTransform.m_rotation.m_col1.setZero4();
                sphereTransform.m_rotation.m_col2.setZero4();
                sphereTransform.m_rotation.m_col0(0) = 1.0f;
                sphereTransform.m_rotation.m_col1(1) = 1.0f;
                sphereTransform.m_rotation.m_col2(2) = 1.0f;
                sphereTransform.m_translation.setZero4();
                sphereTransform.m_translation = spheres[i];
                // EXE-INLINED none: hkArray<hkDisplayGeometry*>::pushBack
                hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
                if (displayGeometries.m_size ==
                    (displayGeometries.m_capacityAndFlags & hkArrayBase<hkDisplayGeometry*>::CAPACITY_MASK)) {
                    hkArrayUtil::_reserveMore(heap, &displayGeometries, sizeof(hkDisplayGeometry*));
                }
                displayGeometries.m_data[displayGeometries.m_size++] = displaySphere;
            }
            return;
        }
        case HK_SHAPE_MULTI_RAY: {
            const hkpMultiRayShape* multiRay = static_cast<const hkpMultiRayShape*>(shape);
            hkGeometry* geom = getCurrentRawGeometry(displayGeometries)->m_geometry;
            hkVector4 offset;
            offset.set(0.01f, 0.0f, 0.0f, 0.0f);
            int32_t lastVertex = 2;
            for (int32_t i = 0; i < multiRay->m_rays.m_size; i++) {
                const hkpMultiRayShape::Ray ray = multiRay->m_rays.m_data[i];
                hkVector4* start = geom->m_vertices.expandBy(1);
                *start = ray.m_start;
                start->setTransformedPos(transform, *start);
                hkVector4* offsetStart = geom->m_vertices.expandBy(1);
                offsetStart->setAdd4(ray.m_start, offset);
                hkVector4* end = geom->m_vertices.expandBy(1);
                *end = ray.m_end;
                end->setTransformedPos(transform, *end);
                hkGeometry::Triangle* face = geom->m_triangles.expandBy(1);
                face->m_a = lastVertex - 2;
                face->m_b = lastVertex - 1;
                face->m_c = lastVertex;
                face->m_material = -1;
                lastVertex += 3;
            }
            return;
        }
        case HK_SHAPE_BV: {
            buildShapeDisplay(
                static_cast<const hkpBvShape*>(shape)->m_boundingVolumeShape, transform, displayGeometries
            );
            return;
        }
        case HK_SHAPE_PLANE: {
            const hkpPlaneShape* planeShape = static_cast<const hkpPlaneShape*>(shape);
            hkAabb localAabb;
            planeShape->getAabb(hkTransformIdentity, 0.001f, localAabb);
            hkVector4 ext;
            ext.setSub4(localAabb.m_max, localAabb.m_min);
            hkVector4 perpToNormal;
            perpToNormal.setZero4();
            perpToNormal(ext.getMajorAxis()) = 1.0f;
            hkVector4 extents;
            // EXE-INLINED 0x630AE0: hkVector4::setMul4
            extents.m_quad = _mm_mul_ps(_mm_set1_ps(0.5f), ext.m_quad);
            hkVector4 center;
            center.setAdd4(extents, localAabb.m_min);
            // EXE-INLINED 0x56D280
            hkDisplayPlane* displayPlane =
                static_cast<hkDisplayPlane*>(hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(hkDisplayPlane)));
            displayPlane->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(hkDisplayPlane));
            displayGeometry = ::new (displayPlane) hkDisplayPlane(planeShape->m_plane, perpToNormal, center, extents);
            break;
        }
        default: {
            // The builders registered for user shape types: each one registered for this shape's type is called.
            // EXE-INLINED none: hkSingleton<hkpUserShapeDisplayBuilder>::getInstance
            const hkpUserShapeDisplayBuilder* builders = hkpUserShapeDisplayBuilder::s_instance;
            for (int32_t i = 0; i < builders->m_userShapeBuilders.m_size; i++) {
                const hkpUserShapeDisplayBuilder::UserShapeBuilder& builder = builders->m_userShapeBuilders.m_data[i];
                if (static_cast<uint32_t>(builder.type) == shape->m_type.m_storage) {
                    builder.f(shape, transform, displayGeometries, this);
                    builders = hkpUserShapeDisplayBuilder::s_instance;
                }
            }
            return;
        }
    }

    displayGeometry->m_transform.Assign(transform);
    // EXE-INLINED none: hkArray<hkDisplayGeometry*>::pushBack
    hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
    if (displayGeometries.m_size ==
        (displayGeometries.m_capacityAndFlags & hkArrayBase<hkDisplayGeometry*>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(heap, &displayGeometries, sizeof(hkDisplayGeometry*));
    }
    displayGeometries.m_data[displayGeometries.m_size++] = displayGeometry;
}

void hkpShapeDisplayBuilder::resetCurrentRawGeometry() {
    m_currentGeometry = nullptr;
}

hkDisplayGeometry* hkpShapeDisplayBuilder::getCurrentRawGeometry(
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

// Constructs the builder on raw stack storage and deliberately runs no destructor for it, as the game does. The
// geometries are deleted from the front, the last one moved into the freed slot each time.
void hkpShapeDisplayBuilder::addObjectToDebugDisplay(const hkpShape* shape, hkTransform& t, hkUlong id) {
    hkpShapeDisplayBuilderEnvironment env;
    env.~hkpShapeDisplayBuilderEnvironment();
    ::new (&env) hkpShapeDisplayBuilderEnvironment;
    alignas(hkpShapeDisplayBuilder) uint8_t aBuilderMemory[sizeof(hkpShapeDisplayBuilder)];
    hkpShapeDisplayBuilder* pBuilder = ::new (aBuilderMemory) hkpShapeDisplayBuilder(env);
    hkArray<hkDisplayGeometry*> displayGeometries;
    pBuilder->buildDisplayGeometries(shape, displayGeometries);
    // The shape's address is the shape id hint (a pointer to integer conversion, as the SDK's (hkUlong) cast).
    hkDebugDisplay::s_instance->addGeometry(displayGeometries, t, id, 0, reinterpret_cast<hkUlong>(shape));

    while (displayGeometries.m_size != 0) {
        hkDisplayGeometry* geometry = displayGeometries.m_data[0];
        if (geometry) {
            delete geometry;
        }
        // EXE-INLINED none: hkArrayBase<hkDisplayGeometry*>::removeAt
        if (--displayGeometries.m_size == 0) {
            break;
        }
        displayGeometries.m_data[0] = displayGeometries.m_data[displayGeometries.m_size];
    }
}

// The builders array goes with its member.
hkpUserShapeDisplayBuilder::~hkpUserShapeDisplayBuilder() {}

// Adds f for type unless a builder for type is registered already.
void hkpUserShapeDisplayBuilder::registerUserShapeDisplayBuilder(
    void (*f)(
        const hkpShape*,
        const hkTransform&,
        hkArray<hkDisplayGeometry*, hkContainerHeapAllocator>&,
        hkpShapeDisplayBuilder*
    ),
    hkpShapeType type
) {
    const int32_t size = m_userShapeBuilders.m_size;
    for (int32_t i = 0; i < size; ++i) {
        if (m_userShapeBuilders.m_data[i].type == type) {
            return;
        }
    }
    // EXE-INLINED none: hkArray<hkpUserShapeDisplayBuilder::UserShapeBuilder>::pushBack
    hkArray<UserShapeBuilder>& builders = m_userShapeBuilders;
    hkMemoryAllocator& heap = *hkMemoryRouter::getInstance().m_heap;
    if (builders.m_size == (builders.m_capacityAndFlags & hkArrayBase<UserShapeBuilder>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(heap, &builders, sizeof(UserShapeBuilder));
    }
    UserShapeBuilder* pBuilder = &builders.m_data[builders.m_size];
    if (pBuilder) {
        pBuilder->f = f;
        pBuilder->type = type;
    }
    ++builders.m_size;
}
