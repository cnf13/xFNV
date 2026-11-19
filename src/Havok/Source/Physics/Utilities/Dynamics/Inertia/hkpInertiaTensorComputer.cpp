#include "Havok/Source/Physics/Utilities/Dynamics/Inertia/hkpInertiaTensorComputer.h"

#include "Havok/Source/Animation/Animation/Ik/LookAt/hkaLookAtIkSolver.h"
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
#include "Havok/Source/Common/Base/Math/hkMath.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkStridedVertices.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Internal/ConvexHull/hkGeometryUtility.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Box/hkpBoxShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Capsule/hkpCapsuleShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/ConvexTransform/hkpConvexTransformShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/ConvexTranslate/hkpConvexTranslateShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/ConvexVertices/hkpConvexVerticesShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Cylinder/hkpCylinderShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Sphere/hkpSphereShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Triangle/hkpTriangleShape.h"
#include "Havok/Source/Physics/Collide/Shape/Deprecated/MultiSphere/hkpMultiSphereShape.h"
#include "Havok/Source/Physics/Collide/Shape/Misc/Bv/hkpBvShape.h"
#include "Havok/Source/Physics/Collide/Shape/Misc/Transform/hkpTransformShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBodyCinfo.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "SDK/MSVC/string.h"
#include "xfnv/Compiler.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

static void finishLoadedObjecthkpMassProperties(void* p, int32_t finishing);
static void cleanupLoadedObjecthkpMassProperties(void* p);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpMassPropertiesTypeInfo = {
    "hkpMassProperties",
    finishLoadedObjecthkpMassProperties,
    cleanupLoadedObjecthkpMassProperties,
    nullptr,
    &typeid(hkpMassProperties),
    sizeof(hkpMassProperties)
};

// The member table of hkpMassProperties's class record, as the Havok SDK's class file has it.
using hkpMassPropertiesClass_MembersArray = const hkClassMember[4];
static hkpMassPropertiesClass_MembersArray hkpMassPropertiesClass_Members{
    {"volume", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x0, nullptr},
    {"mass", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x4, nullptr},
    {"centerOfMass",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_VECTOR4},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x10,
     nullptr},
    {"inertiaTensor",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_MATRIX3},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x20,
     nullptr}
};
hkClass hkpMassPropertiesClass(
    "hkpMassProperties",
    nullptr,
    80,
    nullptr,
    0,
    nullptr,
    0,
    hkpMassPropertiesClass_Members,
    4,
    nullptr,
    nullptr,
    0,
    0
);

static void finishLoadedObjecthkpMassProperties(void* p, int32_t finishing) {
    ;
}

static void cleanupLoadedObjecthkpMassProperties(void* p) {
    ;
}

// The member-wise copy; the padding after m_mass is not copied.
hkpMassProperties& hkpMassProperties::operator=(const hkpMassProperties& other) {
    m_volume = other.m_volume;
    m_mass = other.m_mass;
    m_centerOfMass.m_quad = other.m_centerOfMass.m_quad;
    m_inertiaTensor.m_col0.m_quad = other.m_inertiaTensor.m_col0.m_quad;
    m_inertiaTensor.m_col1.m_quad = other.m_inertiaTensor.m_col1.m_quad;
    m_inertiaTensor.m_col2.m_quad = other.m_inertiaTensor.m_col2.m_quad;
    return *this;
}

void hkpMassProperties::scaleMassAndInertia(float afScale) {
    hkSimdReal v2;
    m_mass = afScale * m_mass;
    v2.m_real = _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(afScale)));
    m_inertiaTensor.mul(v2);
}

void hkpMassProperties::scaleToMass(float newMass) {
    uint32_t v4;
    hkSimdReal v5;
    double v2 = newMass / m_mass;
    hkMatrix3* p_m_inertiaTensor = &m_inertiaTensor;
    *reinterpret_cast<float*>(&v4) = v2;
    XFNV_M128_F32(p_m_inertiaTensor[-1].m_col1.m_quad, 1) = newMass;
    v5.m_real = _mm_set_ss(std::bit_cast<float>(v4));
    p_m_inertiaTensor->mul(v5);
}

void hkpMassProperties::scaleToDensity(float density) {
    float densitya = m_volume * density;
    scaleToMass(densitya);
}

const hkClass& hkpMassProperties::staticClass() {
    return hkpMassPropertiesClass;
}

// Each primitive gets its mass properties with mass 1.0f into newMassProperties (zeroed first); when its volume is not
// zero (NaN goes on) the mass is scaled to the volume and combined with massProperties. Containers and wrapper shapes
// recurse and return at once, without the volume test (their newMassProperties stays zero). The box's extents and the
// cylinder's axis, normalized with a reciprocal square root and one Newton step, use SSE intrinsics.
void computeRecursiveShapeVolumeMassProperties(
    const hkpShape* shape,
    const hkTransform& transform,
    float minTriangleThickness,
    hkpMassProperties& massPropertiesOut
) {
    // EXE-INLINED none: hkpMassProperties::hkpMassProperties
    alignas(16) hkpMassProperties newMassProperties;
    newMassProperties.m_volume = 0.0f;
    newMassProperties.m_mass = 0.0f;
    const __m128 kZero = _mm_setzero_ps();
    _mm_store_ps(&reinterpret_cast<float*>(&newMassProperties.m_centerOfMass.m_quad)[0], kZero);
    _mm_store_ps(&reinterpret_cast<float*>(&newMassProperties.m_inertiaTensor.m_col0.m_quad)[0], kZero);
    _mm_store_ps(&reinterpret_cast<float*>(&newMassProperties.m_inertiaTensor.m_col1.m_quad)[0], kZero);
    _mm_store_ps(&reinterpret_cast<float*>(&newMassProperties.m_inertiaTensor.m_col2.m_quad)[0], kZero);

    switch (shape->m_type) {
        case HK_SHAPE_SPHERE: {
            const hkpSphereShape* sphere = static_cast<const hkpSphereShape*>(shape);
            hkpInertiaTensorComputer::computeSphereVolumeMassProperties(sphere->m_radius, 1.0f, newMassProperties);
            break;
        }
        case HK_SHAPE_CYLINDER: {
            const hkpCylinderShape* cylinder = static_cast<const hkpCylinderShape*>(shape);
            const __m128 kVertexB = _mm_load_ps(&reinterpret_cast<const float*>(&cylinder->m_vertexB.m_quad)[0]);
            const __m128 kVertexA = _mm_load_ps(&reinterpret_cast<const float*>(&cylinder->m_vertexA.m_quad)[0]);
            const float fRadius = cylinder->m_radius;
            hkVector4 direction;
            direction.m_quad = _mm_sub_ps(kVertexB, kVertexA);
            direction.normalize3();
            const __m128 kDirection = direction.m_quad;
            const __m128 kRadius = _mm_set1_ps(fRadius);
            alignas(16) hkVector4 startAxis;
            _mm_store_ps(
                &reinterpret_cast<float*>(&startAxis.m_quad)[0], _mm_sub_ps(kVertexA, _mm_mul_ps(kRadius, kDirection))
            );
            alignas(16) hkVector4 endAxis;
            _mm_store_ps(
                &reinterpret_cast<float*>(&endAxis.m_quad)[0], _mm_add_ps(_mm_mul_ps(kRadius, kDirection), kVertexB)
            );
            const float fCylinderRadius = cylinder->getCylinderRadius() + fRadius;
            hkpInertiaTensorComputer::computeCylinderVolumeMassProperties(
                startAxis, endAxis, fCylinderRadius, 1.0f, newMassProperties
            );
            break;
        }
        case HK_SHAPE_TRIANGLE: {
            const hkpTriangleShape* triangle = static_cast<const hkpTriangleShape*>(shape);
            alignas(16) hkVector4 v0;
            _mm_store_ps(
                &reinterpret_cast<float*>(&v0.m_quad)[0],
                _mm_load_ps(&reinterpret_cast<const float*>(&triangle->m_vertexA.m_quad)[0])
            );
            alignas(16) hkVector4 v1;
            _mm_store_ps(
                &reinterpret_cast<float*>(&v1.m_quad)[0],
                _mm_load_ps(&reinterpret_cast<const float*>(&triangle->m_vertexB.m_quad)[0])
            );
            alignas(16) hkVector4 v2;
            _mm_store_ps(
                &reinterpret_cast<float*>(&v2.m_quad)[0],
                _mm_load_ps(&reinterpret_cast<const float*>(&triangle->m_vertexC.m_quad)[0])
            );
            const float fThickness = hkMath::max2<float>(triangle->m_radius, minTriangleThickness);
            hkpInertiaTensorComputer::computeTriangleSurfaceMassProperties(
                v0, v1, v2, 1.0f, fThickness, newMassProperties
            );
            break;
        }
        case HK_SHAPE_BOX: {
            const hkpBoxShape* box = static_cast<const hkpBoxShape*>(shape);
            const float fRadius = box->m_radius;
            alignas(16) hkVector4 halfExtents;
            _mm_store_ps(
                &reinterpret_cast<float*>(&halfExtents.m_quad)[0],
                _mm_add_ps(
                    _mm_set1_ps(fRadius), _mm_load_ps(&reinterpret_cast<const float*>(&box->m_halfExtents.m_quad)[0])
                )
            );
            hkpInertiaTensorComputer::computeBoxVolumeMassProperties(halfExtents, 1.0f, newMassProperties);
            break;
        }
        case HK_SHAPE_CAPSULE: {
            const hkpCapsuleShape* capsule = static_cast<const hkpCapsuleShape*>(shape);
            hkpInertiaTensorComputer::computeCapsuleVolumeMassProperties(
                capsule->m_vertexA, capsule->m_vertexB, capsule->m_radius, 1.0f, newMassProperties
            );
            break;
        }
        case HK_SHAPE_CONVEX_VERTICES: {
            const hkpConvexShape* convex = static_cast<const hkpConvexShape*>(shape);
            hkArray<hkVector4> vertices;
            static_cast<const hkpConvexVerticesShape*>(convex)->getOriginalVertices(vertices);
            hkStridedVertices stridedVertices;
            // The vertex array's storage, read as the floats it holds.
            stridedVertices.m_vertices = reinterpret_cast<const float*>(vertices.m_data);
            stridedVertices.m_numVertices = vertices.m_size;
            stridedVertices.m_striding = sizeof(hkVector4);
            hkpInertiaTensorComputer::computeConvexHullMassProperties(
                stridedVertices, convex->m_radius, newMassProperties
            );
            break;
        }
        case HK_SHAPE_COLLECTION:
        case HK_SHAPE_BV_TREE:
        case HK_SHAPE_LIST:
        case HK_SHAPE_MOPP:
        case HK_SHAPE_EXTENDED_MESH:
        case HK_SHAPE_CONVEX_LIST:
        case HK_SHAPE_TRIANGLE_COLLECTION: {
            const hkpShapeContainer* container = shape->getContainer();
            alignas(16) char buffer[512];
            for (uint32_t key = container->getFirstKey(); key != 0xFFFFFFFF; key = container->getNextKey(key)) {
                const hkpShape* child = container->getChildShape(key, buffer);
                if (child) {
                    computeRecursiveShapeVolumeMassProperties(
                        child, transform, minTriangleThickness, massPropertiesOut
                    );
                }
            }
            return;
        }
        case HK_SHAPE_CONVEX_TRANSLATE: {
            const hkpConvexTranslateShape* translateShape = static_cast<const hkpConvexTranslateShape*>(shape);
            // EXE-INLINED none: hkTransform::hkTransform(hkQuaternion identity, translation)
            alignas(16) hkTransform translation;
            _mm_store_ps(&reinterpret_cast<float*>(&translation.m_rotation.m_col0.m_quad)[0], kZero);
            XFNV_M128_F32(translation.m_rotation.m_col0.m_quad, 0) = 1.0f;
            _mm_store_ps(&reinterpret_cast<float*>(&translation.m_rotation.m_col1.m_quad)[0], kZero);
            XFNV_M128_F32(translation.m_rotation.m_col1.m_quad, 1) = 1.0f;
            _mm_store_ps(&reinterpret_cast<float*>(&translation.m_rotation.m_col2.m_quad)[0], kZero);
            XFNV_M128_F32(translation.m_rotation.m_col2.m_quad, 2) = 1.0f;
            _mm_store_ps(
                &reinterpret_cast<float*>(&translation.m_translation.m_quad)[0],
                _mm_load_ps(&reinterpret_cast<const float*>(&translateShape->m_translation.m_quad)[0])
            );
            alignas(16) hkTransform childTransform;
            childTransform.setMul(transform, translation);
            computeRecursiveShapeVolumeMassProperties(
                translateShape->m_childShape.m_childShape, childTransform, minTriangleThickness, massPropertiesOut
            );
            return;
        }
        case HK_SHAPE_CONVEX_TRANSFORM: {
            const hkpConvexTransformShape* transformShape = static_cast<const hkpConvexTransformShape*>(shape);
            alignas(16) hkTransform childTransform;
            // m_transform is declared as hkQsTransform but is read here as an hkTransform and passed to
            // hkTransform::setMul.
            childTransform.setMul(transform, *reinterpret_cast<const hkTransform*>(&transformShape->m_transform));
            computeRecursiveShapeVolumeMassProperties(
                transformShape->m_childShape.m_childShape, childTransform, minTriangleThickness, massPropertiesOut
            );
            return;
        }
        case HK_SHAPE_TRANSFORM: {
            const hkpTransformShape* transformShape = static_cast<const hkpTransformShape*>(shape);
            alignas(16) hkTransform childTransform;
            childTransform.setMul(transform, transformShape->m_transform);
            computeRecursiveShapeVolumeMassProperties(
                transformShape->m_childShape.m_childShape, childTransform, minTriangleThickness, massPropertiesOut
            );
            return;
        }
        case HK_SHAPE_MULTI_SPHERE: {
            const hkpMultiSphereShape* multiSphere = static_cast<const hkpMultiSphereShape*>(shape);
            alignas(16) hkTransform sphereTransform;
            // Stack storage each sphere is constructed in (hkpSphereShape::Construct, no destructor).
            alignas(16) uint8_t sphereMemory[sizeof(hkpSphereShape)];
            for (int32_t i = 0; i < multiSphere->m_numSpheres; i++) {
                const hkVector4& sphere = multiSphere->m_spheres[i];
                _mm_store_ps(
                    &reinterpret_cast<float*>(&sphereTransform.m_rotation.m_col0.m_quad)[0],
                    _mm_load_ps(&reinterpret_cast<const float*>(&transform.m_rotation.m_col0.m_quad)[0])
                );
                _mm_store_ps(
                    &reinterpret_cast<float*>(&sphereTransform.m_rotation.m_col1.m_quad)[0],
                    _mm_load_ps(&reinterpret_cast<const float*>(&transform.m_rotation.m_col1.m_quad)[0])
                );
                _mm_store_ps(
                    &reinterpret_cast<float*>(&sphereTransform.m_rotation.m_col2.m_quad)[0],
                    _mm_load_ps(&reinterpret_cast<const float*>(&transform.m_rotation.m_col2.m_quad)[0])
                );
                _mm_store_ps(
                    &reinterpret_cast<float*>(&sphereTransform.m_translation.m_quad)[0],
                    _mm_load_ps(&reinterpret_cast<const float*>(&transform.m_translation.m_quad)[0])
                );
                sphereTransform.m_translation.setTransformedPos(sphereTransform, sphere);
                hkpSphereShape* sphereShape = ::new (sphereMemory) hkpSphereShape(XFNV_M128_F32(sphere.m_quad, 3));
                computeRecursiveShapeVolumeMassProperties(
                    sphereShape, sphereTransform, minTriangleThickness, massPropertiesOut
                );
            }
            return;
        }
        case HK_SHAPE_BV: {
            const hkpBvShape* bvShape = static_cast<const hkpBvShape*>(shape);
            computeRecursiveShapeVolumeMassProperties(
                bvShape->m_childShape.m_childShape, transform, minTriangleThickness, massPropertiesOut
            );
            return;
        }
        default: return;
    }

    if (newMassProperties.m_volume != 0.0f) {
        newMassProperties.scaleMassAndInertia(newMassProperties.m_volume);

        hkInplaceArrayAligned16<hkpMassElement, 2> elements;

        // EXE-INLINED none: hkArray<hkpMassElement>::expandOne
        hkMemoryRouter& router = hkMemoryRouter::getInstance();
        if (elements.m_size == (elements.m_capacityAndFlags & hkArrayBase<hkpMassElement>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(*router.m_heap, &elements, sizeof(hkpMassElement));
        }
        hkpMassElement* pNew = elements.m_data + elements.m_size;
        if (pNew) {
            // EXE-INLINED none: hkpMassElement::hkpMassElement
            pNew->m_properties.m_volume = 0.0f;
            _mm_store_ps(&reinterpret_cast<float*>(&pNew->m_properties.m_centerOfMass.m_quad)[0], kZero);
            pNew->m_properties.m_mass = 0.0f;
            _mm_store_ps(&reinterpret_cast<float*>(&pNew->m_properties.m_inertiaTensor.m_col0.m_quad)[0], kZero);
            _mm_store_ps(&reinterpret_cast<float*>(&pNew->m_properties.m_inertiaTensor.m_col1.m_quad)[0], kZero);
            _mm_store_ps(&reinterpret_cast<float*>(&pNew->m_properties.m_inertiaTensor.m_col2.m_quad)[0], kZero);
            _mm_store_ps(&reinterpret_cast<float*>(&pNew->m_transform.m_rotation.m_col0.m_quad)[0], kZero);
            _mm_store_ps(&reinterpret_cast<float*>(&pNew->m_transform.m_rotation.m_col1.m_quad)[0], kZero);
            _mm_store_ps(&reinterpret_cast<float*>(&pNew->m_transform.m_rotation.m_col2.m_quad)[0], kZero);
            XFNV_M128_F32(pNew->m_transform.m_rotation.m_col0.m_quad, 0) = 1.0f;
            XFNV_M128_F32(pNew->m_transform.m_rotation.m_col1.m_quad, 1) = 1.0f;
            XFNV_M128_F32(pNew->m_transform.m_rotation.m_col2.m_quad, 2) = 1.0f;
            _mm_store_ps(&reinterpret_cast<float*>(&pNew->m_transform.m_translation.m_quad)[0], kZero);
        }
        hkpMassElement* pAccumulated = elements.m_data + elements.m_size;
        elements.m_size++;
        pAccumulated->m_properties = massPropertiesOut;
        pAccumulated->m_transform.setIdentity();

        // EXE-INLINED none: hkArray<hkpMassElement>::expandOne
        hkMemoryRouter& router2 = hkMemoryRouter::getInstance();
        if (elements.m_size == (elements.m_capacityAndFlags & hkArrayBase<hkpMassElement>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(*router2.m_heap, &elements, sizeof(hkpMassElement));
        }
        pNew = elements.m_data + elements.m_size;
        if (pNew) {
            // EXE-INLINED none: hkpMassElement::hkpMassElement
            pNew->m_properties.m_volume = 0.0f;
            _mm_store_ps(&reinterpret_cast<float*>(&pNew->m_properties.m_centerOfMass.m_quad)[0], kZero);
            pNew->m_properties.m_mass = 0.0f;
            _mm_store_ps(&reinterpret_cast<float*>(&pNew->m_properties.m_inertiaTensor.m_col0.m_quad)[0], kZero);
            _mm_store_ps(&reinterpret_cast<float*>(&pNew->m_properties.m_inertiaTensor.m_col1.m_quad)[0], kZero);
            _mm_store_ps(&reinterpret_cast<float*>(&pNew->m_properties.m_inertiaTensor.m_col2.m_quad)[0], kZero);
            _mm_store_ps(&reinterpret_cast<float*>(&pNew->m_transform.m_rotation.m_col0.m_quad)[0], kZero);
            _mm_store_ps(&reinterpret_cast<float*>(&pNew->m_transform.m_rotation.m_col1.m_quad)[0], kZero);
            _mm_store_ps(&reinterpret_cast<float*>(&pNew->m_transform.m_rotation.m_col2.m_quad)[0], kZero);
            XFNV_M128_F32(pNew->m_transform.m_rotation.m_col0.m_quad, 0) = 1.0f;
            XFNV_M128_F32(pNew->m_transform.m_rotation.m_col1.m_quad, 1) = 1.0f;
            XFNV_M128_F32(pNew->m_transform.m_rotation.m_col2.m_quad, 2) = 1.0f;
            _mm_store_ps(&reinterpret_cast<float*>(&pNew->m_transform.m_translation.m_quad)[0], kZero);
        }
        hkpMassElement* pShapeElement = elements.m_data + elements.m_size;
        elements.m_size++;
        pShapeElement->m_properties = newMassProperties;
        _mm_store_ps(
            &reinterpret_cast<float*>(&pShapeElement->m_transform.m_rotation.m_col0.m_quad)[0],
            _mm_load_ps(&reinterpret_cast<const float*>(&transform.m_rotation.m_col0.m_quad)[0])
        );
        _mm_store_ps(
            &reinterpret_cast<float*>(&pShapeElement->m_transform.m_rotation.m_col1.m_quad)[0],
            _mm_load_ps(&reinterpret_cast<const float*>(&transform.m_rotation.m_col1.m_quad)[0])
        );
        _mm_store_ps(
            &reinterpret_cast<float*>(&pShapeElement->m_transform.m_rotation.m_col2.m_quad)[0],
            _mm_load_ps(&reinterpret_cast<const float*>(&transform.m_rotation.m_col2.m_quad)[0])
        );
        _mm_store_ps(
            &reinterpret_cast<float*>(&pShapeElement->m_transform.m_translation.m_quad)[0],
            _mm_load_ps(&reinterpret_cast<const float*>(&transform.m_translation.m_quad)[0])
        );

        hkpInertiaTensorComputer::combineMassProperties(elements, massPropertiesOut);
    }
}

void clipInertia(float a1, float* a2) {
    double v4;
    float a2a = a2[20];
    float v5 = a2[25];
    if (!(v5 < static_cast<double>(a2a))) {
        a2a = v5;
    }
    float v6 = a2[30];
    if (!(v6 < static_cast<double>(a2a))) {
        a2a = v6;
    }
    float a2b = a2a / a1;
    float a1a = a2[20];
    double v3 = a2b;
    if (!(a2b < static_cast<double>(a1a))) {
        v4 = a2b;
    }
    else {
        v3 = a1a;
        v4 = a2b;
    }
    float a2e = v3;
    a2[20] = a2e;
    float a2c = a2[25];
    if (!(a2c > v4)) {
        a2c = v4;
    }
    a2[25] = a2c;
    float a2d = a2[30];
    if (!(a2d > v4)) {
        a2d = v4;
    }
    a2[30] = a2d;
}

// The constraints that touch body move from constraints into a local hkInplaceArray (16 in place); each one's other
// body is optimised first. A fixed or keyframed body stands for 0. Otherwise its diagonal inertia elements are raised
// to min(largest * inertiaFactorHint, sum of the children's), but no less than the largest child's, and it stands for
// min(largest + sum, largest * inertiaFactorHint), but no less than that new inertia.
float hkInertiaTensorComputer_optimizeInertiasOfConstraintTreeInt(
    hkArray<const hkpConstraintInstance*, hkContainerHeapAllocator>& constraints,
    hkpRigidBody* body,
    float inertiaFactorHint
) {
    using ChildArray = hkInplaceArray<const hkpConstraintInstance*, 16>;
    alignas(16) ChildArray childConstraints;
    for (int32_t i = constraints.m_size - 1; i >= 0; --i) {
        const hkpConstraintInstance* ci = constraints.m_data[i];
        hkpRigidBody* other;
        if (ci->m_entities[0] == body) {
            other = ci->getRigidBodyB();
        }
        else if (ci->m_entities[1] == body) {
            other = ci->getRigidBodyA();
        }
        else {
            continue;
        }
        if (!other) {
            continue;
        }
        constraints.removeAt(i);
        childConstraints.pushBack(ci);
    }
    float sumInertia = 0.0f;
    float maxInertia = 0.0f;
    for (int32_t i = childConstraints.m_size - 1; i >= 0; --i) {
        const hkpConstraintInstance* ci = childConstraints.m_data[i];
        hkpRigidBody* other = (ci->m_entities[0] == body) ? ci->getRigidBodyB() : ci->getRigidBodyA();
        const float childInertia =
            hkInertiaTensorComputer_optimizeInertiasOfConstraintTreeInt(constraints, other, inertiaFactorHint);
        sumInertia = childInertia + sumInertia;
        if (maxInertia < childInertia) {
            maxInertia = childInertia;
        }
    }
    hkBool isFixedOrKeyframed = body->isFixedOrKeyframed();
    if (isFixedOrKeyframed.m_bool) {
        return 0.0f;
    }
    hkMatrix3 inertia;
    body->getInertiaLocal(inertia);
    float maxDiagonal = (inertia(2, 2) < inertia(1, 1)) ? inertia(1, 1) : inertia(2, 2);
    if (maxDiagonal < inertia(0, 0)) {
        maxDiagonal = inertia(0, 0);
    }
    if (sumInertia == 0.0f) {
        return maxDiagonal;
    }
    const float maxScaled = maxDiagonal * inertiaFactorHint;
    float newInertia = (maxScaled > sumInertia) ? sumInertia : maxScaled;
    newInertia = (maxInertia < newInertia) ? newInertia : maxInertia;
    if (!(newInertia < inertia(0, 0))) {
        inertia(0, 0) = newInertia;
    }
    if (!(newInertia < inertia(1, 1))) {
        inertia(1, 1) = newInertia;
    }
    if (!(newInertia < inertia(2, 2))) {
        inertia(2, 2) = newInertia;
    }
    body->setInertiaLocal(inertia);
    const float total = maxDiagonal + sumInertia;
    float standsFor = (total > maxScaled) ? maxScaled : total;
    standsFor = (newInertia < standsFor) ? standsFor : newInertia;
    return standsFor;
}

// KEEP: the exe body never writes ecx, and its only caller, compFaceIntegrals (0xCEC3D0), reads `this` from ecx
// right after the call (`mov eax, [ecx+8]` at 0xCEC3E4), which clang does not guarantee.
void InternalInertiaTensorComputer::compProjectionIntegrals(const hkVector4* v) {
    bool v15;
    float P1;
    float Pa;
    float Pb;
    float Paab;
    float Pabb;
    float Paa;
    float Paaa;
    float Pbb;
    float Pbbb;
    float Pab;
    int32_t A = this->A;
    this->Pbbb = 0.0f;
    this->Pabb = 0.0f;
    this->Paab = 0.0f;
    this->Paaa = 0.0f;
    int32_t B = this->B;
    this->Pbb = 0.0f;
    this->Pab = 0.0f;
    this->Paa = 0.0f;
    int32_t v4 = 1;
    this->Pb = 0.0f;
    const float* v24 = &v[0](B);
    this->Pa = 0.0f;
    int32_t v41 = 1;
    this->P1 = 0.0f;
    const float* v5 = &v[0](A);
    double v6 = 3.0;
    int32_t n3 = 3;
    do {
        float v44 = *v24;
        int32_t v7 = v4 % 3;
        // Indexes the vertex's floats unmasked; a vector subscript masks the index.
        double v8 = v[v7](this->A);
        double v9 = *v5;
        float v23 = v8 - v9;
        double v10 = v[v7](this->B);
        float v36 = v10 - v44;
        float v50 = v9 * v9;
        double v11 = v50;
        float v20 = v50 * v9;
        float v17 = v44 * v44;
        float v21 = v17 * v44;
        float v18 = v8 * v8;
        float v22 = v10 * v10;
        float v38 = v22 * v10;
        float v25 = v8 + v9;
        float v27 = v25 * v8 + v50;
        float v51 = v27 * v8 + v20;
        float v29 = (v10 + v44) * v10 + v17;
        float v48 = v29 * v10 + v21;
        double v46 = (v8 + v8) * v9;
        float v31 = v46 + v18 * v6 + v11;
        double v12 = v6 * v11 + (v18 + v46);
        double v13 = v44;
        float v33 = v12;
        P1 = v25 * v36 + this->P1;
        this->P1 = P1;
        Pa = v27 * v36 + this->Pa;
        this->Pa = Pa;
        double v14 = v51;
        Paa = v51 * v36 + this->Paa;
        this->Paa = Paa;
        float v52 = v20 * v9;
        float v53 = v14 * v8 + v52;
        Paaa = v53 * v36 + this->Paaa;
        this->Paaa = Paaa;
        Pb = v29 * v23 + this->Pb;
        this->Pb = Pb;
        Pbb = v23 * v48 + this->Pbb;
        this->Pbb = Pbb;
        float v54 = v21 * v13;
        float v55 = v48 * v10 + v54;
        Pbbb = v55 * v23 + this->Pbbb;
        this->Pbbb = Pbbb;
        v24 += 4;
        v4 = v41 + 1;
        v5 += 4;
        v15 = n3-- == 1;
        ++v41;
        Pab = v36 * (v31 * v10 + v33 * v13) + this->Pab;
        this->Pab = Pab;
        float v19 = v18 * v8;
        float v32 = v19 * 4.0 + v31 * v9;
        float v34 = v33 * v8 + v20 * 4.0;
        Paab = (v32 * v10 + v34 * v13) * v36 + this->Paab;
        this->Paab = Paab;
        float v35 = 4.0 * v38 + v22 * 3.0 * v13 + (v10 + v10) * v17 + v21;
        double v16 = v13 * (v22 + v22) + v38;
        v6 = 3.0;
        float v39 = v10 * 3.0 * v17 + v16 + v21 * 4.0;
        Pabb = (v8 * v35 + v9 * v39) * v23 + this->Pabb;
        this->Pabb = Pabb;
    }
    while (!v15);
    this->P1 = P1 * 0.5;
    this->Pa = Pa / 6.0;
    this->Paa = Paa / 12.0;
    this->Paaa = Paaa / 20.0;
    this->Pb = Pb / -6.0;
    this->Pbb = Pbb / -12.0;
    this->Pbbb = Pbbb / -20.0;
    this->Pab = Pab / 24.0;
    this->Paab = Paab / 60.0;
    this->Pabb = Pabb / -60.0;
}

// Computes Mirtich's face integrals Fa..Fcca from the projection integrals P1..Pbbb.
void InternalInertiaTensorComputer::compFaceIntegrals(const hkVector4* v, const hkVector4& n) {
    compProjectionIntegrals(v);
    const float w = -n(0) * XFNV_M128_F32(v[0].m_quad, 0) - XFNV_M128_F32(v[0].m_quad, 1) * n(1) -
        XFNV_M128_F32(v[0].m_quad, 2) * n(2);
    const float k1 = 1.0f / n(C);
    const float k2 = k1 * k1;
    const float k3 = k2 * k1;
    Fa = Pa * k1;
    Fb = Pb * k1;
    const double dP1w = P1 * w;
    const float fNegK2 = -k2;
    Fc = (n(B) * Pb + n(A) * Pa + dP1w) * fNegK2;
    Faa = Paa * k1;
    Fbb = Pbb * k1;
    const float nA = n(A);
    const float nB = n(B);
    const float fSqA = nA * nA;
    const float fSqB = nB * nB;
    Fcc = (nA * 2.0 * nB * Pab + fSqA * Paa + fSqB * Pbb + ((nB * Pb + nA * Pa) * 2.0 + dP1w) * w) * k3;
    Faaa = k1 * Paaa;
    Fbbb = k1 * Pbbb;
    const float fCubeA = nA * nA * nA;
    const float fCubeB = nB * nB * nB;
    const float k4 = k3 * k1;
    Fccc = -k4 *
        (fSqA * 3.0 * nB * Paab + fCubeA * Paaa + fSqB * (nA * 3.0) * Pabb + fCubeB * Pbbb +
         (nA * 2.0 * nB * Pab + fSqA * Paa + fSqB * Pbb) * (w * 3.0) + (dP1w + (nB * Pb + nA * Pa) * 3.0) * (w * w));
    Faab = k1 * Paab;
    Fbbc = (nB * Pbbb + nA * Pabb + Pbb * w) * fNegK2;
    Fcca = (nA * 2.0 * nB * Paab + fSqA * Paaa + fSqB * Pabb + w * ((nA * Paa + Pab * nB) * 2.0 + Pa * w)) * k3;
}

// KEEP: the exe body (and compFaceIntegrals, compProjectionIntegrals it calls) never writes ecx, and its caller keeps
// `this` in ecx across the call, which clang does not guarantee: computeGeometryVolumeMassPropertiesChecked calls
// computeInertialTensorInternal at 0xCED9D1 with the ecx it had before the call at 0xCED989.
// The exe keeps the zero it compares each face's normal length with on the x87 stack, and reads A once for T1's
// first term but again for T2 and TP (compFaceIntegrals and the stores in between go through `this`).
void InternalInertiaTensorComputer::compVolumeIntegrals(const hkGeometry* geom, const hkVector4& shift) {
    int32_t numTriangles = geom->m_triangles.m_size;
    TP[2] = 0.0f;
    TP[1] = 0.0f;
    TP[0] = 0.0f;
    T2[2] = 0.0f;
    T2[1] = 0.0f;
    T2[0] = 0.0f;
    T1[2] = 0.0f;
    T1[1] = 0.0f;
    T1[0] = 0.0f;
    T0 = 0.0f;
    for (int32_t i = 0; i < numTriangles; i++) {
        const hkGeometry::Triangle& triangle = geom->m_triangles.m_data[i];
        int32_t a = triangle.m_a;
        int32_t b = triangle.m_b;
        int32_t c = triangle.m_c;
        alignas(16) hkVector4 vertices[3];
        __m128 vb = _mm_add_ps(geom->m_vertices.m_data[b].m_quad, shift.m_quad);
        __m128 vc = _mm_add_ps(geom->m_vertices.m_data[c].m_quad, shift.m_quad);
        vertices[0].m_quad = _mm_add_ps(geom->m_vertices.m_data[a].m_quad, shift.m_quad);
        vertices[1].m_quad = vb;
        __m128 e1 = _mm_sub_ps(vb, vertices[0].m_quad);
        vertices[2].m_quad = vc;
        __m128 e2 = _mm_sub_ps(vc, vertices[0].m_quad);
        __m128 n = _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(e2, e2, 0xD2), _mm_shuffle_ps(e1, e1, 0xC9)),
            _mm_mul_ps(_mm_shuffle_ps(e2, e2, 0xC9), _mm_shuffle_ps(e1, e1, 0xD2))
        );
        __m128 nSq = _mm_mul_ps(n, n);
        float len = _mm_cvtss_f32(
            _mm_sqrt_ss(_mm_add_ss(_mm_shuffle_ps(nSq, nSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(nSq, nSq, 0x55), nSq)))
        );
        if (!(len > 0.0)) {
            continue;
        }
        float invLen = 1.0 / len;
        alignas(16) hkVector4 normal;
        normal.m_quad = _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(invLen), _mm_set_ss(invLen), 0), n);
        float absX = std::fabs(normal(0));
        float absY = std::fabs(normal(1));
        float absZ = std::fabs(normal(2));
        // C is the axis of the largest normal component; a NaN compare picks 2.
        if (absY < absX && absZ < absX) {
            C = 0;
        }
        else if (absY > absZ) {
            C = 1;
        }
        else {
            C = 2;
        }
        A = (C + 1) % 3;
        B = (A + 1) % 3;
        compFaceIntegrals(vertices, normal);
        int32_t axisA = A;
        float fA;
        if (axisA == 0) {
            fA = Fa;
        }
        else if (B == 0) {
            fA = Fb;
        }
        else {
            fA = Fc;
        }
        T0 = normal(0) * fA + T0;
        T1[axisA] = normal(axisA) * Faa + T1[axisA];
        T1[B] = normal(B) * Fbb + T1[B];
        T1[C] = normal(C) * Fcc + T1[C];
        T2[A] = normal(A) * Faaa + T2[A];
        T2[B] = normal(B) * Fbbb + T2[B];
        T2[C] = normal(C) * Fccc + T2[C];
        TP[A] = normal(A) * Faab + TP[A];
        TP[B] = normal(B) * Fbbc + TP[B];
        TP[C] = normal(C) * Fcca + TP[C];
    }
    T1[0] = T1[0] * 0.5;
    T1[1] = T1[1] * 0.5;
    T1[2] = T1[2] * 0.5;
    T2[0] = T2[0] / 3.0;
    T2[1] = T2[1] / 3.0;
    T2[2] = T2[2] / 3.0;
    TP[0] = TP[0] * 0.5;
    TP[1] = TP[1] * 0.5;
    TP[2] = 0.5 * TP[2];
}

void InternalInertiaTensorComputer::computeInertialTensorInternal(
    float mass,
    float density,
    hkVector4& r,
    hkMatrix3& J
) {
    XFNV_M128_F32(r.m_quad, 0) = T1[0u] / T0;
    XFNV_M128_F32(r.m_quad, 1) = T1[1] / T0;
    XFNV_M128_F32(r.m_quad, 2) = T1[2] / T0;
    XFNV_M128_F32(J.m_col0.m_quad, 0) = (T2[2] + T2[1]) * density;
    XFNV_M128_F32(J.m_col1.m_quad, 1) = (T2[2] + T2[0u]) * density;
    XFNV_M128_F32(J.m_col2.m_quad, 2) = (T2[1] + T2[0u]) * density;
    float massCentera = -density;
    double v6 = massCentera;
    float massCenterb = TP[0u] * massCentera;
    XFNV_M128_F32(J.m_col0.m_quad, 1) = massCenterb;
    XFNV_M128_F32(J.m_col1.m_quad, 0) = massCenterb;
    float massCenterc = TP[1] * v6;
    XFNV_M128_F32(J.m_col1.m_quad, 2) = massCenterc;
    XFNV_M128_F32(J.m_col2.m_quad, 1) = massCenterc;
    float massCenterd = v6 * TP[2];
    XFNV_M128_F32(J.m_col2.m_quad, 0) = massCenterd;
    XFNV_M128_F32(J.m_col0.m_quad, 2) = massCenterd;
    XFNV_M128_F32(J.m_col0.m_quad, 0) = XFNV_M128_F32(J.m_col0.m_quad, 0) -
        (XFNV_M128_F32(r.m_quad, 2) * XFNV_M128_F32(r.m_quad, 2) +
         XFNV_M128_F32(r.m_quad, 1) * XFNV_M128_F32(r.m_quad, 1)) *
            mass;
    XFNV_M128_F32(J.m_col1.m_quad, 1) = XFNV_M128_F32(J.m_col1.m_quad, 1) -
        (XFNV_M128_F32(r.m_quad, 2) * XFNV_M128_F32(r.m_quad, 2) +
         XFNV_M128_F32(r.m_quad, 0) * XFNV_M128_F32(r.m_quad, 0)) *
            mass;
    XFNV_M128_F32(J.m_col2.m_quad, 2) = XFNV_M128_F32(J.m_col2.m_quad, 2) -
        (XFNV_M128_F32(r.m_quad, 1) * XFNV_M128_F32(r.m_quad, 1) +
         XFNV_M128_F32(r.m_quad, 0) * XFNV_M128_F32(r.m_quad, 0)) *
            mass;
    float massCentere =
        XFNV_M128_F32(r.m_quad, 0) * mass * XFNV_M128_F32(r.m_quad, 1) + XFNV_M128_F32(J.m_col0.m_quad, 1);
    XFNV_M128_F32(J.m_col0.m_quad, 1) = massCentere;
    XFNV_M128_F32(J.m_col1.m_quad, 0) = massCentere;
    float massCenterf =
        XFNV_M128_F32(r.m_quad, 1) * mass * XFNV_M128_F32(r.m_quad, 2) + XFNV_M128_F32(J.m_col1.m_quad, 2);
    XFNV_M128_F32(J.m_col1.m_quad, 2) = massCenterf;
    XFNV_M128_F32(J.m_col2.m_quad, 1) = massCenterf;
    float massCenterg =
        mass * XFNV_M128_F32(r.m_quad, 2) * XFNV_M128_F32(r.m_quad, 0) + XFNV_M128_F32(J.m_col2.m_quad, 0);
    XFNV_M128_F32(J.m_col2.m_quad, 0) = massCenterg;
    XFNV_M128_F32(J.m_col0.m_quad, 2) = massCenterg;
}

void InternalInertiaTensorComputer::compGeometryAabb(
    const hkVector4* vertices,
    int32_t numVerts,
    hkVector4& centerOut,
    hkVector4& aabbExtentsOut
) {
    centerOut.m_quad = _mm_setzero_ps();
    aabbExtentsOut.m_quad = _mm_setzero_ps();
    __m128 v4 = _mm_shuffle_ps(_mm_set_ss(rdata_104DA48), _mm_set_ss(rdata_104DA48), 0);
    __m128 v5 = _mm_xor_ps(hkQuadRealNegateMask, v4);
    bool v6 = numVerts == 0;
    if (numVerts > 0) {
        int32_t v8 = numVerts;
        do {
            __m128 m_quad = vertices->m_quad;
            __m128 v10 = vertices->m_quad;
            ++vertices;
            --v8;
            v4 = _mm_min_ps(v10, v4);
            v5 = _mm_max_ps(m_quad, v5);
        }
        while (v8);
        v6 = numVerts == 0;
    }
    if (!v6) {
        __m128 v11 = _mm_shuffle_ps(_mm_set_ss(f0_5_), _mm_set_ss(f0_5_), 0);
        centerOut.m_quad = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, v11), v4), _mm_mul_ps(v11, v5));
        aabbExtentsOut.m_quad = _mm_sub_ps(v5, v4);
    }
}

void hkpInertiaTensorComputer::computeShapeVolumeMassProperties(
    const hkpShape* shape,
    float mass,
    hkpMassProperties& result
) {
    hkpMassProperties edx0;
    hkTransform arA;
    edx0.m_mass = 0.0f;
    edx0.m_volume = 0.0f;
    std::memset(&arA, 0, sizeof(arA));
    XFNV_M128_F32(arA.m_rotation.m_col0.m_quad, 0) = 1.0f;
    XFNV_M128_F32(arA.m_rotation.m_col1.m_quad, 1) = 1.0f;
    XFNV_M128_F32(arA.m_rotation.m_col2.m_quad, 2) = 1.0f;
    std::memset(&edx0.m_centerOfMass, 0, sizeof(edx0.m_centerOfMass));
    std::memset(&edx0.m_inertiaTensor, 0, sizeof(edx0.m_inertiaTensor));
    computeRecursiveShapeVolumeMassProperties(shape, arA, 0.0f, edx0);
    double m_volume = edx0.m_volume;
    if (edx0.m_volume != 0.0f ||
        (computeRecursiveShapeVolumeMassProperties(shape, arA, 0.01f, edx0),
         (m_volume = edx0.m_volume, edx0.m_volume != 0.0f))) {
        float v5 = mass / m_volume;
        edx0.scaleMassAndInertia(v5);
        result.m_volume = edx0.m_volume;
        double m_mass = edx0.m_mass;
        result.m_centerOfMass = edx0.m_centerOfMass;
        result.m_mass = m_mass;
        result.m_inertiaTensor = edx0.m_inertiaTensor;
    }
}

void hkpInertiaTensorComputer::shiftInertiaFromCom(const hkVector4& shift, float mass, hkMatrix3& inertia) {
    XFNV_M128_F32((&inertia)->m_col0.m_quad, 0) =
        ((((XFNV_M128_F32((&shift)->m_quad, 2) * XFNV_M128_F32((&shift)->m_quad, 2)) +
           (XFNV_M128_F32((&shift)->m_quad, 1) * XFNV_M128_F32((&shift)->m_quad, 1))) *
          mass) +
         XFNV_M128_F32((&inertia)->m_col0.m_quad, 0));
    XFNV_M128_F32((&inertia)->m_col1.m_quad, 1) =
        ((((XFNV_M128_F32((&shift)->m_quad, 2) * XFNV_M128_F32((&shift)->m_quad, 2)) +
           (XFNV_M128_F32((&shift)->m_quad, 0) * XFNV_M128_F32((&shift)->m_quad, 0))) *
          mass) +
         XFNV_M128_F32((&inertia)->m_col1.m_quad, 1));
    XFNV_M128_F32((&inertia)->m_col2.m_quad, 2) =
        ((((XFNV_M128_F32((&shift)->m_quad, 1) * XFNV_M128_F32((&shift)->m_quad, 1)) +
           (XFNV_M128_F32((&shift)->m_quad, 0) * XFNV_M128_F32((&shift)->m_quad, 0))) *
          mass) +
         XFNV_M128_F32((&inertia)->m_col2.m_quad, 2));
    float v5 = XFNV_M128_F32((&inertia)->m_col0.m_quad, 1) -
        ((XFNV_M128_F32((&shift)->m_quad, 0) * mass) * XFNV_M128_F32((&shift)->m_quad, 1));
    XFNV_M128_F32((&inertia)->m_col0.m_quad, 1) = v5;
    XFNV_M128_F32((&inertia)->m_col1.m_quad, 0) = v5;
    float v6 = XFNV_M128_F32((&inertia)->m_col1.m_quad, 2) -
        ((XFNV_M128_F32((&shift)->m_quad, 1) * mass) * XFNV_M128_F32((&shift)->m_quad, 2));
    XFNV_M128_F32((&inertia)->m_col1.m_quad, 2) = v6;
    XFNV_M128_F32((&inertia)->m_col2.m_quad, 1) = v6;
    float v7 = XFNV_M128_F32((&inertia)->m_col2.m_quad, 0) -
        ((mass * XFNV_M128_F32((&shift)->m_quad, 2)) * XFNV_M128_F32((&shift)->m_quad, 0));
    XFNV_M128_F32((&inertia)->m_col2.m_quad, 0) = v7;
    XFNV_M128_F32((&inertia)->m_col0.m_quad, 2) = v7;
}

void hkpInertiaTensorComputer::shiftInertiaToCom(const hkVector4& shift, float mass, hkMatrix3& inertia) {
    const float* a1 = reinterpret_cast<const float*>(&shift.m_quad);
    const float a2 = mass;
    float* a3 = reinterpret_cast<float*>(&inertia.m_col0.m_quad);
    *a3 = ((*a3) - (((a1[2] * a1[2]) + (a1[1] * a1[1])) * a2));
    a3[5] = (a3[5] - (((a1[2] * a1[2]) + ((*a1) * (*a1))) * a2));
    a3[10] = (a3[10] - (((a1[1] * a1[1]) + ((*a1) * (*a1))) * a2));
    float v5 = (((*a1) * a2) * a1[1]) + a3[1];
    a3[1] = v5;
    a3[4] = v5;
    float v6 = ((a2 * a1[1]) * a1[2]) + a3[6];
    a3[6] = v6;
    a3[9] = v6;
    float v7 = ((a2 * a1[2]) * (*a1)) + a3[8];
    a3[8] = v7;
    a3[2] = v7;
}

hkResult
hkpInertiaTensorComputer::computeSphereVolumeMassProperties(float radius, float mass, hkpMassProperties& result) {
    if (mass <= 0.0f) {
        return HK_FAILURE;
    }
    double v4 = radius;
    if (radius <= 0.0f) {
        return HK_FAILURE;
    }
    result.m_inertiaTensor.m_col0.m_quad = _mm_setzero_ps();
    result.m_inertiaTensor.m_col1.m_quad = _mm_setzero_ps();
    result.m_inertiaTensor.m_col2.m_quad = _mm_setzero_ps();
    float v5 = v4 * mass * v4 * 0.4000000059604645;
    XFNV_M128_F32(result.m_inertiaTensor.m_col0.m_quad, 0u) = 1.0f;
    XFNV_M128_F32(result.m_inertiaTensor.m_col1.m_quad, 1) = 1.0f;
    XFNV_M128_F32(result.m_inertiaTensor.m_col2.m_quad, 2) = 1.0f;
    result.m_centerOfMass.m_quad = _mm_setzero_ps();
    XFNV_M128_F32(result.m_inertiaTensor.m_col0.m_quad, 0u) = v5;
    XFNV_M128_F32(result.m_inertiaTensor.m_col1.m_quad, 1) = v5;
    XFNV_M128_F32(result.m_inertiaTensor.m_col2.m_quad, 2) = v5;
    result.m_volume = v4 * (v4 * 4.188790321350098 * v4);
    result.m_mass = mass;
    return HK_SUCCESS;
}

hkResult hkpInertiaTensorComputer::computeConvexHullMassProperties(
    const hkStridedVertices& vertices,
    float radius,
    hkpMassProperties& result
) {
    uint32_t v10;
    hkgpConvexHull apMemory;
    alignas(16) hkgpConvexHull::BuildConfig arg4;
    arg4.m_allowLowerDimensions.m_bool = 1;
    apMemory.build(vertices, arg4);
    if (apMemory.getDimensions() == -1) {
        return HK_FAILURE;
    }

    float v6 = hkConvexShapeDefaultRadius / 5.0;
    float v7 = 0.0f;
    if (apMemory.getDimensions() == 3) {
        apMemory.buildMassProperties();
        if (radius > 0.0f) {
            float v8 = apMemory.getSurfaceArea() * radius;
            v7 = v8 + radius * (radius * 12.566370964050293 * radius) / 3.0;
        }
    }
    while (apMemory.getDimensions() != 3 || apMemory.getVolume() < 1.1920928955078125e-05) {
        apMemory.build(vertices, arg4);
        apMemory.expandByPlanarMargin(v6);
        if (apMemory.getDimensions() == 3) {
            apMemory.buildMassProperties();
        }
        v6 = v6 + v6;
    }
    result.m_centerOfMass.m_quad = (&apMemory.getCenterOfMass())->m_quad;
    result.m_inertiaTensor = apMemory.getWorldInertia();
    double v3 = apMemory.getVolume() + v7;
    double v4 = v7;
    float v9 = v3;
    result.m_volume = v9;
    result.m_mass = 1.0f;
    if (v9 > 0.0f && v4 > 0.0) {
        hkSimdReal v12;
        *reinterpret_cast<float*>(&v10) = v4 / v9 + 1.0;
        v12.m_real = _mm_set_ss(std::bit_cast<float>(v10));
        result.m_inertiaTensor.mul(v12);
    }
    return HK_SUCCESS;
}

// On the thin-surface and the degenerate-triangle paths only the nine components of the local hkMatrix3 are written
// before all three columns, w lanes included, are copied into result: the three w lanes are uninitialised reads that
// reach result, deliberately, as the game does.
hkResult hkpInertiaTensorComputer::computeTriangleSurfaceMassProperties(
    const hkVector4& v0,
    const hkVector4& v1,
    const hkVector4& v2,
    float mass,
    float surfaceThickness,
    hkpMassProperties& result
) {
    if (mass <= 0.0f) {
        return HK_FAILURE;
    }
    if (surfaceThickness < 0.0f) {
        return HK_FAILURE;
    }
    alignas(16) hkVector4 com;
    alignas(16) hkMatrix3 inertia;
    __m128 e0 = _mm_sub_ps(v0.m_quad, v1.m_quad);
    __m128 e1 = _mm_sub_ps(v2.m_quad, v1.m_quad);
    alignas(16) hkVector4 normal;
    normal.m_quad = _mm_sub_ps(
        _mm_mul_ps(_mm_shuffle_ps(e0, e0, 0xD2), _mm_shuffle_ps(e1, e1, 0xC9)),
        _mm_mul_ps(_mm_shuffle_ps(e0, e0, 0xC9), _mm_shuffle_ps(e1, e1, 0xD2))
    );
    // Twice the triangle's area.
    float doubleArea = normal.length3();
    if (surfaceThickness < 1e-05f) {
        // A thin surface: the inertia of the triangle about the origin, then moved to its centre.
        com.m_quad = _mm_mul_ps(
            _mm_add_ps(_mm_add_ps(v0.m_quad, v1.m_quad), v2.m_quad),
            _mm_shuffle_ps(_mm_set_ss(0.33333334f), _mm_set_ss(0.33333334f), 0)
        );
        double cx9 = com(0) * 9.0;
        double m12 = mass / 12.0;
        float xx = (v0(0) * v0(0) + com(0) * cx9 + v1(0) * v1(0) + v2(0) * v2(0)) * m12;
        double cy9 = com(1) * 9.0;
        float yy = (v0(1) * v0(1) + cy9 * com(1) + v1(1) * v1(1) + v2(1) * v2(1)) * m12;
        float zz = (v0(2) * v0(2) + 9.0 * com(2) * com(2) + v1(2) * v1(2) + v2(2) * v2(2)) * m12;
        inertia(0, 0) = zz + yy;
        inertia(1, 1) = zz + xx;
        inertia(2, 2) = xx + yy;
        float xy = (v0(0) * v0(1) + com(1) * cx9 + v1(0) * v1(1) + v2(1) * v2(0)) * m12;
        inertia(1, 0) = -xy;
        inertia(0, 1) = inertia(1, 0);
        float xz = (v0(0) * v0(2) + cx9 * com(2) + v1(0) * v1(2) + v2(2) * v2(0)) * m12;
        inertia(2, 0) = -xz;
        inertia(0, 2) = inertia(2, 0);
        float yz = (v0(2) * v0(1) + com(2) * cy9 + v1(2) * v1(1) + v2(2) * v2(1)) * m12;
        inertia(1, 2) = -yz;
        inertia(2, 1) = inertia(1, 2);
        hkpInertiaTensorComputer::shiftInertiaToCom(com, mass, inertia);
    }
    else if (doubleArea < 1e-05f) {
        // A degenerate triangle: a point mass at its centre.
        com.m_quad = _mm_mul_ps(
            _mm_add_ps(_mm_add_ps(v0.m_quad, v1.m_quad), v2.m_quad),
            _mm_shuffle_ps(_mm_set_ss(0.33333334f), _mm_set_ss(0.33333334f), 0)
        );
        inertia(0, 0) = (com(1) * com(1) + com(2) * com(2)) * mass;
        inertia(1, 1) = (com(2) * com(2) + com(0) * com(0)) * mass;
        inertia(2, 2) = (com(1) * com(1) + com(0) * com(0)) * mass;
        float negMass = -mass;
        float xy = com(1) * com(0) * negMass;
        inertia(0, 1) = xy;
        inertia(1, 0) = xy;
        float xz = com(0) * com(2) * negMass;
        inertia(0, 2) = xz;
        inertia(2, 0) = xz;
        float yz = com(2) * com(1) * negMass;
        inertia(2, 1) = yz;
        inertia(1, 2) = yz;
    }
    else {
        // A prism of the given thickness around the triangle, as a volume.
        normal.normalize3();

        using VertexArray = hkInplaceArray<hkVector4, 6, hkContainerHeapAllocator>;
        VertexArray vertices;
        vertices.setSize(6);
        float h = surfaceThickness * 0.5;
        float nh = -surfaceThickness * 0.5;
        // Each vertex once on either side of the triangle's plane.
        vertices.m_data[0] = v0;
        vertices.m_data[1] = v0;
        vertices.m_data[0].addMul4(h, normal);
        vertices.m_data[1].addMul4(nh, normal);
        vertices.m_data[2] = v1;
        vertices.m_data[3] = v1;
        vertices.m_data[2].m_quad = _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(h), _mm_set_ss(h), 0), normal.m_quad), vertices.m_data[2].m_quad
        );
        vertices.m_data[3].m_quad = _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(nh), _mm_set_ss(nh), 0), normal.m_quad), vertices.m_data[3].m_quad
        );
        vertices.m_data[4] = v2;
        vertices.m_data[5] = v2;
        vertices.m_data[4].m_quad = _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(h), _mm_set_ss(h), 0), normal.m_quad), vertices.m_data[4].m_quad
        );
        vertices.m_data[5].m_quad = _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(nh), _mm_set_ss(nh), 0), normal.m_quad), vertices.m_data[5].m_quad
        );

        alignas(16) hkpMassProperties massProperties;
        // EXE-INLINED none: hkpMassProperties::hkpMassProperties
        massProperties.m_volume = 0.0f;
        massProperties.m_mass = 0.0f;
        massProperties.m_centerOfMass.m_quad = _mm_setzero_ps();
        massProperties.m_inertiaTensor.m_col0.m_quad = _mm_setzero_ps();
        massProperties.m_inertiaTensor.m_col1.m_quad = _mm_setzero_ps();
        massProperties.m_inertiaTensor.m_col2.m_quad = _mm_setzero_ps();

        alignas(16) hkGeometry geom;
        // EXE-INLINED 0xCCE5E0
        geom.m_vertices.m_data = nullptr;
        geom.m_vertices.m_size = 0;
        geom.m_vertices.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
        geom.m_triangles.m_data = nullptr;
        geom.m_triangles.m_size = 0;
        geom.m_triangles.m_capacityAndFlags = hkArrayBase<hkGeometry::Triangle>::DONT_DEALLOCATE_FLAG;
        geom.m_vertices = vertices;
        // The prism's two faces and three sides, two triangles each.
        hkGeometry::Triangle* t = geom.m_triangles.expandBy(1);
        t->m_a = 0;
        t->m_b = 2;
        t->m_c = 4;
        t->m_material = -1;
        t = geom.m_triangles.expandBy(1);
        t->m_a = 1;
        t->m_b = 5;
        t->m_c = 3;
        t->m_material = -1;
        t = geom.m_triangles.expandBy(1);
        t->m_a = 0;
        t->m_b = 3;
        t->m_c = 2;
        t->m_material = -1;
        t = geom.m_triangles.expandBy(1);
        t->m_a = 0;
        t->m_b = 1;
        t->m_c = 3;
        t->m_material = -1;
        t = geom.m_triangles.expandBy(1);
        t->m_a = 1;
        t->m_b = 0;
        t->m_c = 4;
        t->m_material = -1;
        t = geom.m_triangles.expandBy(1);
        t->m_a = 1;
        t->m_b = 4;
        t->m_c = 5;
        t->m_material = -1;
        t = geom.m_triangles.expandBy(1);
        t->m_a = 2;
        t->m_b = 5;
        t->m_c = 4;
        t->m_material = -1;
        t = geom.m_triangles.expandBy(1);
        t->m_a = 2;
        t->m_b = 3;
        t->m_c = 5;
        t->m_material = -1;
        hkpInertiaTensorComputer::computeGeometryVolumeMassProperties(&geom, mass, massProperties);
        com = massProperties.m_centerOfMass;
        inertia.m_col0 = massProperties.m_inertiaTensor.m_col0;
        inertia.m_col1 = massProperties.m_inertiaTensor.m_col1;
        inertia.m_col2 = massProperties.m_inertiaTensor.m_col2;
    }
    result.m_mass = mass;
    result.m_inertiaTensor.m_col0 = inertia.m_col0;
    result.m_inertiaTensor.m_col1 = inertia.m_col1;
    result.m_inertiaTensor.m_col2 = inertia.m_col2;
    result.m_volume = doubleArea * 0.5 * surfaceThickness;
    result.m_centerOfMass = com;
    return HK_SUCCESS;
}

// The capsule as a cylinder between startAxis and endAxis plus two hemispheres, each an hkpMassElement in an inline
// array of three, combined by combineMassProperties. capsuleToLocal turns the z axis onto the capsule's axis (identity
// for a zero-length axis or one within 1e-5 of z). The volumes, masses and the inertia go through float locals; the
// half height is kept as a double.
hkResult hkpInertiaTensorComputer::computeCapsuleVolumeMassProperties(
    const hkVector4& startAxis,
    const hkVector4& endAxis,
    float radius,
    float mass,
    hkpMassProperties& result
) {
    if (mass <= 0.0f) {
        return HK_FAILURE;
    }
    if (radius <= 0.0f) {
        return HK_FAILURE;
    }

    alignas(16) hkVector4 axis;
    axis.setSub4(endAxis, startAxis);
    const float height = axis.length3();

    alignas(16) hkRotation capsuleToLocal;
    bool bIdentity = true;
    if (height > 0.0f) {
        axis.normalize3();

        alignas(16) hkVector4 canonicalZ;
        canonicalZ.set(0.0f, 0.0f, 1.0f, 0.0f);
        const float fDot = canonicalZ.dot3(axis);
        if (hkMath::fabs(fDot) < 0.9999899864196777) {
            bIdentity = false;
            alignas(16) hkVector4 rotAxis;
            rotAxis.setCross(canonicalZ, axis);
            rotAxis.normalize3();

            const float fCosAngle = canonicalZ.dot3(axis);
            const float fAngle = hkMath::acos(fCosAngle);
            alignas(16) hkQuaternion q;
            q.setAxisAngle(rotAxis, fAngle);
            capsuleToLocal.set(q);
        }
    }
    if (bIdentity) {
        capsuleToLocal.setIdentity();
    }

    alignas(16) hkVector4 centre;
    centre.m_quad = _mm_mul_ps(_mm_set1_ps(0.5f), _mm_add_ps(startAxis.m_quad, endAxis.m_quad));

    const float sphereVolume = radius * 4.188790321350098 * radius * radius;
    const float cylinderVolume = radius * 3.1415927410125732 * radius * height;
    const float totalVolume = cylinderVolume + sphereVolume;
    const float sphereMass = sphereVolume * mass / totalVolume;
    const float cylinderMass = cylinderVolume * mass / totalVolume;

    hkInplaceArray<hkpMassElement, 3> elements;
    for (int32_t i = 0; i < 3; ++i) {
        // EXE-INLINED none: hkpMassElement::hkpMassElement
        hkpMassElement& rStorage = elements.m_storage[i];
        rStorage.m_properties.m_volume = 0.0f;
        rStorage.m_properties.m_mass = 0.0f;
        rStorage.m_properties.m_centerOfMass.m_quad = _mm_setzero_ps();
        rStorage.m_properties.m_inertiaTensor.m_col0.m_quad = _mm_setzero_ps();
        rStorage.m_properties.m_inertiaTensor.m_col1.m_quad = _mm_setzero_ps();
        rStorage.m_properties.m_inertiaTensor.m_col2.m_quad = _mm_setzero_ps();
        rStorage.m_transform.m_rotation.m_col0.m_quad = _mm_setzero_ps();
        rStorage.m_transform.m_rotation.m_col1.m_quad = _mm_setzero_ps();
        rStorage.m_transform.m_rotation.m_col2.m_quad = _mm_setzero_ps();
        XFNV_M128_F32(rStorage.m_transform.m_rotation.m_col0.m_quad, 0) = 1.0f;
        XFNV_M128_F32(rStorage.m_transform.m_rotation.m_col1.m_quad, 1) = 1.0f;
        XFNV_M128_F32(rStorage.m_transform.m_rotation.m_col2.m_quad, 2) = 1.0f;
        rStorage.m_transform.m_translation.m_quad = _mm_setzero_ps();
    }

    // The cylinder: its inertia about its centre, scaled by its mass.
    const double halfHeight = height * 0.5;
    alignas(16) hkpMassElement element;
    element.m_transform.m_rotation.m_col0 = capsuleToLocal.m_col0;
    element.m_transform.m_rotation.m_col1 = capsuleToLocal.m_col1;
    element.m_transform.m_rotation.m_col2 = capsuleToLocal.m_col2;
    element.m_transform.m_translation = centre;
    element.m_properties.m_inertiaTensor.m_col0.m_quad = _mm_setzero_ps();
    element.m_properties.m_inertiaTensor.m_col1.m_quad = _mm_setzero_ps();
    element.m_properties.m_inertiaTensor.m_col2.m_quad = _mm_setzero_ps();
    const float fRadialInertia = height * halfHeight * 0.5 / 3.0 + radius * radius * 0.25;
    XFNV_M128_F32(element.m_properties.m_inertiaTensor.m_col0.m_quad, 0) = fRadialInertia;
    XFNV_M128_F32(element.m_properties.m_inertiaTensor.m_col1.m_quad, 1) = fRadialInertia;
    XFNV_M128_F32(element.m_properties.m_inertiaTensor.m_col2.m_quad, 2) = radius * radius * 0.5;
    hkSimdReal cylinderMassReal;
    cylinderMassReal.m_real = _mm_set_ss(cylinderMass);
    element.m_properties.m_inertiaTensor.mul(cylinderMassReal);
    element.m_properties.m_volume = cylinderVolume;
    element.m_properties.m_mass = cylinderMass;
    element.m_properties.m_centerOfMass.setZero4();

    elements.pushBack(element);

    // The upper hemisphere: half a sphere's inertia, moved from the sphere's centre to the hemisphere's centre of
    // mass 3/8 of the radius up the axis; the element sits at the top of the cylinder.
    const float halfSphereVolume = sphereVolume * 0.5;
    const float halfSphereMass = sphereMass * 0.5;
    {
        element.m_properties.m_volume = 0.0f;
        element.m_properties.m_mass = 0.0f;
        element.m_properties.m_centerOfMass.setZero4();
        element.m_properties.m_inertiaTensor.m_col0.m_quad = _mm_setzero_ps();
        element.m_properties.m_inertiaTensor.m_col1.m_quad = _mm_setzero_ps();
        element.m_properties.m_inertiaTensor.m_col2.m_quad = _mm_setzero_ps();
        element.m_transform.m_rotation.m_col0 = capsuleToLocal.m_col0;
        element.m_transform.m_rotation.m_col1 = capsuleToLocal.m_col1;
        element.m_transform.m_rotation.m_col2 = capsuleToLocal.m_col2;
        element.m_transform.m_translation = centre;
        const float fHalfHeight = halfHeight;
        alignas(16) hkVector4 offset;
        offset.set(0.0f, 0.0f, fHalfHeight, 0.0f);
        offset.setRotatedDir(capsuleToLocal, offset);
        element.m_transform.m_translation.m_quad = _mm_add_ps(element.m_transform.m_translation.m_quad, offset.m_quad);
        const float fComOffset = 0.375 * radius;
        element.m_properties.m_centerOfMass.set(0.0f, 0.0f, fComOffset, 0.0f);

        alignas(16) hkpMassProperties sphereProperties;
        sphereProperties.m_inertiaTensor.m_col0.m_quad = _mm_setzero_ps();
        sphereProperties.m_inertiaTensor.m_col1.m_quad = _mm_setzero_ps();
        sphereProperties.m_inertiaTensor.m_col2.m_quad = _mm_setzero_ps();
        hkpInertiaTensorComputer::computeSphereVolumeMassProperties(radius, sphereMass, sphereProperties);
        element.m_properties.m_inertiaTensor.m_col0 = sphereProperties.m_inertiaTensor.m_col0;
        element.m_properties.m_inertiaTensor.m_col1 = sphereProperties.m_inertiaTensor.m_col1;
        element.m_properties.m_inertiaTensor.m_col2 = sphereProperties.m_inertiaTensor.m_col2;
        hkSimdReal half;
        half.m_real = _mm_set_ss(0.5f);
        element.m_properties.m_inertiaTensor.mul(half);
        hkpInertiaTensorComputer::shiftInertiaToCom(
            element.m_properties.m_centerOfMass, halfSphereMass, element.m_properties.m_inertiaTensor
        );
        element.m_properties.m_volume = halfSphereVolume;
        element.m_properties.m_mass = halfSphereMass;
    }

    elements.pushBack(element);

    // The lower hemisphere: the same, mirrored down the axis.
    {
        element.m_properties.m_volume = 0.0f;
        element.m_properties.m_mass = 0.0f;
        element.m_properties.m_centerOfMass.setZero4();
        element.m_properties.m_inertiaTensor.m_col0.m_quad = _mm_setzero_ps();
        element.m_properties.m_inertiaTensor.m_col1.m_quad = _mm_setzero_ps();
        element.m_properties.m_inertiaTensor.m_col2.m_quad = _mm_setzero_ps();
        element.m_transform.m_rotation.m_col0 = capsuleToLocal.m_col0;
        element.m_transform.m_rotation.m_col1 = capsuleToLocal.m_col1;
        element.m_transform.m_rotation.m_col2 = capsuleToLocal.m_col2;
        element.m_transform.m_translation = centre;
        const float fNegHalfHeight = -halfHeight;
        alignas(16) hkVector4 offset;
        offset.set(0.0f, 0.0f, fNegHalfHeight, 0.0f);
        offset.setRotatedDir(capsuleToLocal, offset);
        element.m_transform.m_translation.m_quad = _mm_add_ps(element.m_transform.m_translation.m_quad, offset.m_quad);
        const float fComOffset = -0.375 * radius;
        element.m_properties.m_centerOfMass.set(0.0f, 0.0f, fComOffset, 0.0f);

        alignas(16) hkpMassProperties sphereProperties;
        sphereProperties.m_inertiaTensor.m_col0.m_quad = _mm_setzero_ps();
        sphereProperties.m_inertiaTensor.m_col1.m_quad = _mm_setzero_ps();
        sphereProperties.m_inertiaTensor.m_col2.m_quad = _mm_setzero_ps();
        hkpInertiaTensorComputer::computeSphereVolumeMassProperties(radius, sphereMass, sphereProperties);
        element.m_properties.m_inertiaTensor.m_col0 = sphereProperties.m_inertiaTensor.m_col0;
        element.m_properties.m_inertiaTensor.m_col1 = sphereProperties.m_inertiaTensor.m_col1;
        element.m_properties.m_inertiaTensor.m_col2 = sphereProperties.m_inertiaTensor.m_col2;
        hkSimdReal half;
        half.m_real = _mm_set_ss(0.5f);
        element.m_properties.m_inertiaTensor.mul(half);
        hkpInertiaTensorComputer::shiftInertiaToCom(
            element.m_properties.m_centerOfMass, halfSphereMass, element.m_properties.m_inertiaTensor
        );
        element.m_properties.m_volume = halfSphereVolume;
        element.m_properties.m_mass = halfSphereMass;
    }

    elements.pushBack(element);

    hkpInertiaTensorComputer::combineMassProperties(elements, result);
    return HK_SUCCESS;
}

// The solid cylinder between startAxis and endAxis as one hkpMassElement in an inline array of one, combined by
// combineMassProperties. cylinderToLocal turns the z axis (hkTransform::getIdentity's third column) onto the cylinder's
// axis (identity for an axis within 1e-5 of z). The volume and the radial inertia go through float locals.
hkResult hkpInertiaTensorComputer::computeCylinderVolumeMassProperties(
    const hkVector4& startAxis,
    const hkVector4& endAxis,
    float radius,
    float mass,
    hkpMassProperties& result
) {
    if (mass <= 0.0f) {
        return HK_FAILURE;
    }
    if (radius <= 0.0f) {
        return HK_FAILURE;
    }

    alignas(16) hkVector4 axis;
    axis.setSub4(endAxis, startAxis);
    const float height = axis.length3();
    // A NaN height fails too.
    if (!(height > 0.0f)) {
        return HK_FAILURE;
    }

    axis.normalize3();

    const hkVector4& canonicalZ = hkTransformIdentity.m_rotation.m_col2;
    const float fDot = canonicalZ.dot3(axis);
    alignas(16) hkRotation cylinderToLocal;
    if (hkMath::fabs(fDot) < 0.9999899864196777) {
        alignas(16) hkVector4 rotAxis;
        rotAxis.setCross(canonicalZ, axis);
        rotAxis.normalize3();

        const float fCosAngle = canonicalZ.dot3(axis);
        const float fAngle = hkMath::acos(fCosAngle);
        alignas(16) hkQuaternion q;
        q.setAxisAngle(rotAxis, fAngle);
        cylinderToLocal.set(q);
    }
    else {
        cylinderToLocal.setIdentity();
    }

    alignas(16) hkVector4 centre;
    centre.m_quad = _mm_mul_ps(_mm_set1_ps(0.5f), _mm_add_ps(startAxis.m_quad, endAxis.m_quad));
    const float volume = radius * 3.1415927410125732 * radius * height;

    hkInplaceArray<hkpMassElement, 1> elements;
    {
        // EXE-INLINED none: hkpMassElement::hkpMassElement
        hkpMassElement& rStorage = elements.m_storage[0];
        rStorage.m_properties.m_volume = 0.0f;
        rStorage.m_properties.m_mass = 0.0f;
        rStorage.m_properties.m_centerOfMass.m_quad = _mm_setzero_ps();
        rStorage.m_properties.m_inertiaTensor.m_col0.m_quad = _mm_setzero_ps();
        rStorage.m_properties.m_inertiaTensor.m_col1.m_quad = _mm_setzero_ps();
        rStorage.m_properties.m_inertiaTensor.m_col2.m_quad = _mm_setzero_ps();
        rStorage.m_transform.m_rotation.m_col0.m_quad = _mm_setzero_ps();
        rStorage.m_transform.m_rotation.m_col1.m_quad = _mm_setzero_ps();
        rStorage.m_transform.m_rotation.m_col2.m_quad = _mm_setzero_ps();
        XFNV_M128_F32(rStorage.m_transform.m_rotation.m_col0.m_quad, 0) = 1.0f;
        XFNV_M128_F32(rStorage.m_transform.m_rotation.m_col1.m_quad, 1) = 1.0f;
        XFNV_M128_F32(rStorage.m_transform.m_rotation.m_col2.m_quad, 2) = 1.0f;
        rStorage.m_transform.m_translation.m_quad = _mm_setzero_ps();
    }

    // EXE-INLINED none: hkArray<hkpMassElement>::pushBack (of a default-constructed element)
    hkMemoryRouter& router = hkMemoryRouter::getInstance();
    if (elements.m_size == (elements.m_capacityAndFlags & hkArrayBase<hkpMassElement>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*router.m_heap, &elements, sizeof(hkpMassElement));
    }
    hkpMassElement* pNew = elements.m_data + elements.m_size;
    if (pNew) {
        // EXE-INLINED none: hkpMassElement::hkpMassElement
        pNew->m_properties.m_volume = 0.0f;
        pNew->m_properties.m_mass = 0.0f;
        pNew->m_properties.m_centerOfMass.m_quad = _mm_setzero_ps();
        pNew->m_properties.m_inertiaTensor.m_col0.m_quad = _mm_setzero_ps();
        pNew->m_properties.m_inertiaTensor.m_col1.m_quad = _mm_setzero_ps();
        pNew->m_properties.m_inertiaTensor.m_col2.m_quad = _mm_setzero_ps();
        pNew->m_transform.m_rotation.m_col0.m_quad = _mm_setzero_ps();
        pNew->m_transform.m_rotation.m_col1.m_quad = _mm_setzero_ps();
        pNew->m_transform.m_rotation.m_col2.m_quad = _mm_setzero_ps();
        XFNV_M128_F32(pNew->m_transform.m_rotation.m_col0.m_quad, 0) = 1.0f;
        XFNV_M128_F32(pNew->m_transform.m_rotation.m_col1.m_quad, 1) = 1.0f;
        XFNV_M128_F32(pNew->m_transform.m_rotation.m_col2.m_quad, 2) = 1.0f;
        pNew->m_transform.m_translation.m_quad = _mm_setzero_ps();
    }
    hkpMassElement& rElement = elements.m_data[elements.m_size];
    elements.m_size++;

    // The cylinder's inertia about its centre, scaled by its mass.
    rElement.m_transform.m_rotation.m_col0 = cylinderToLocal.m_col0;
    rElement.m_transform.m_rotation.m_col1 = cylinderToLocal.m_col1;
    rElement.m_transform.m_rotation.m_col2 = cylinderToLocal.m_col2;
    rElement.m_transform.m_translation = centre;
    rElement.m_properties.m_centerOfMass.set(0.0f, 0.0f, 0.0f, 0.0f);
    rElement.m_properties.m_inertiaTensor.setIdentity();
    const float fRadialInertia = height * (height * 0.5) * 0.5 / 3.0 + radius * radius * 0.25;
    XFNV_M128_F32(rElement.m_properties.m_inertiaTensor.m_col0.m_quad, 0) = fRadialInertia;
    XFNV_M128_F32(rElement.m_properties.m_inertiaTensor.m_col1.m_quad, 1) = fRadialInertia;
    XFNV_M128_F32(rElement.m_properties.m_inertiaTensor.m_col2.m_quad, 2) = radius * radius * 0.5;
    hkSimdReal massReal;
    massReal.m_real = _mm_set_ss(mass);
    rElement.m_properties.m_inertiaTensor.mul(massReal);
    rElement.m_properties.m_volume = volume;
    rElement.m_properties.m_mass = mass;
    rElement.m_properties.m_centerOfMass.setZero4();

    hkpInertiaTensorComputer::combineMassProperties(elements, result);
    return HK_SUCCESS;
}

hkResult hkpInertiaTensorComputer::computeBoxVolumeMassProperties(
    const hkVector4& halfExtents,
    float mass,
    hkpMassProperties& result
) {
    if (mass <= 0.0f) {
        return HK_FAILURE;
    }
    float v5 = XFNV_M128_F32(halfExtents.m_quad, 0);
    float v7 = XFNV_M128_F32(halfExtents.m_quad, 1);
    float v6 = XFNV_M128_F32(halfExtents.m_quad, 2);
    result.m_inertiaTensor.m_col0.m_quad = _mm_setzero_ps();
    result.m_inertiaTensor.m_col1.m_quad = _mm_setzero_ps();
    result.m_inertiaTensor.m_col2.m_quad = _mm_setzero_ps();
    float v8 = 0.3333333432674408 * mass;
    XFNV_M128_F32(result.m_inertiaTensor.m_col0.m_quad, 0u) = 1.0f;
    XFNV_M128_F32(result.m_inertiaTensor.m_col1.m_quad, 1) = 1.0f;
    XFNV_M128_F32(result.m_inertiaTensor.m_col2.m_quad, 2) = 1.0f;
    result.m_centerOfMass.m_quad = _mm_setzero_ps();
    double v4 = v7 * v7;
    XFNV_M128_F32(result.m_inertiaTensor.m_col0.m_quad, 0u) = (v4 + v6 * v6) * v8;
    XFNV_M128_F32(result.m_inertiaTensor.m_col1.m_quad, 1) = (v6 * v6 + v5 * v5) * v8;
    XFNV_M128_F32(result.m_inertiaTensor.m_col2.m_quad, 2) = v8 * (v4 + v5 * v5);
    result.m_volume = v6 * (v7 * v5) * 8.0;
    result.m_mass = mass;
    return HK_SUCCESS;
}

// aabbInertia's volume and mass are stored as 0.0, the value of the first compare, since compGeometryAabb returns
// nothing.
void hkpInertiaTensorComputer::computeGeometryVolumeMassProperties(
    const hkGeometry* geom,
    float mass,
    hkpMassProperties& result
) {
    // Only a mass that is not greater than zero (not NaN) becomes 1.
    if (mass <= 0.0f) {
        mass = 1.0f;
    }
    alignas(16) hkVector4 aabbExtents;
    alignas(16) hkVector4 aabbCenter;
    InternalInertiaTensorComputer::compGeometryAabb(
        geom->m_vertices.m_data, geom->m_vertices.m_size, aabbCenter, aabbExtents
    );
    // EXE-INLINED none: hkpMassProperties::hkpMassProperties
    alignas(16) hkpMassProperties aabbInertia;
    aabbInertia.m_volume = 0.0f;
    aabbInertia.m_mass = 0.0f;
    aabbInertia.m_centerOfMass.m_quad = {0.0f, 0.0f, 0.0f, 0.0f};
    aabbInertia.m_inertiaTensor.m_col0.m_quad = {0.0f, 0.0f, 0.0f, 0.0f};
    aabbInertia.m_inertiaTensor.m_col1.m_quad = {0.0f, 0.0f, 0.0f, 0.0f};
    aabbInertia.m_inertiaTensor.m_col2.m_quad = {0.0f, 0.0f, 0.0f, 0.0f};
    computeBoxVolumeMassProperties(aabbExtents, mass, aabbInertia);
    aabbInertia.m_centerOfMass.m_quad = aabbCenter.m_quad;
    alignas(16) hkVector4 shift;
    shift.setNeg4(aabbCenter);
    InternalInertiaTensorComputer computer;
    computer.compVolumeIntegrals(geom, shift);
    // The mesh's own properties are used only when its volume is greater than zero or NaN.
    if (computer.T0 <= 0.0f) {
        result.m_volume = aabbInertia.m_volume;
        result.m_centerOfMass.m_quad = aabbCenter.m_quad;
        result.m_mass = aabbInertia.m_mass;
        result.m_inertiaTensor.m_col0.m_quad = aabbInertia.m_inertiaTensor.m_col0.m_quad;
        result.m_inertiaTensor.m_col1.m_quad = aabbInertia.m_inertiaTensor.m_col1.m_quad;
        result.m_inertiaTensor.m_col2.m_quad = aabbInertia.m_inertiaTensor.m_col2.m_quad;
        return;
    }
    const float fVolume = computer.T0;
    result.m_volume = fVolume;
    result.m_mass = mass;
    const float fDensity = mass / fVolume;
    computer.computeInertialTensorInternal(mass, fDensity, result.m_centerOfMass, result.m_inertiaTensor);
    result.m_centerOfMass.add4(aabbCenter);
    // Each principal moment is kept at a tenth of the AABB's at least; a raised one takes the AABB's centre on that
    // axis and drops the negative products of inertia on its row and column. The limit is 0.1f.
    hkVector4* pResultColumns = &result.m_inertiaTensor.m_col0;
    const hkVector4* pAabbColumns = &aabbInertia.m_inertiaTensor.m_col0;
    for (int32_t i = 0; i < 3; ++i) {
        float* pColumn = &reinterpret_cast<float*>(&pResultColumns[i].m_quad)[0];
        const double dLimit = (&reinterpret_cast<const float*>(&pAabbColumns[i].m_quad)[0])[i] * 0.10000000149011612;
        // Raised only when below the limit, not when NaN.
        if (pColumn[i] < dLimit) {
            pColumn[i] = static_cast<float>(dLimit);
            (&reinterpret_cast<float*>(&result.m_centerOfMass.m_quad)[0])[i] =
                (&reinterpret_cast<float*>(&aabbCenter.m_quad)[0])[i];
            for (int32_t j = 0; j < 3; ++j) {
                if (j == i) {
                    continue;
                }
                float fProduct = pColumn[j];
                // A NaN is kept.
                if (fProduct < 0.0f) {
                    fProduct = 0.0f;
                }
                (&reinterpret_cast<float*>(&pResultColumns[j].m_quad)[0])[i] = fProduct;
                pColumn[j] = fProduct;
            }
        }
    }
}

// computeGeometryVolumeMassProperties with failures instead of the mass and AABB fallbacks.
hkResult hkpInertiaTensorComputer::computeGeometryVolumeMassPropertiesChecked(
    const hkGeometry* geom,
    float mass,
    hkpMassProperties& result
) {
    // Fails unless the mass is greater than zero or NaN.
    if (mass <= 0.0f) {
        return HK_FAILURE;
    }
    alignas(16) hkVector4 aabbExtents;
    alignas(16) hkVector4 aabbCenter;
    InternalInertiaTensorComputer::compGeometryAabb(
        geom->m_vertices.m_data, geom->m_vertices.m_size, aabbCenter, aabbExtents
    );
    // EXE-INLINED none: hkpMassProperties::hkpMassProperties
    alignas(16) hkpMassProperties aabbInertia;
    aabbInertia.m_volume = 0.0f;
    aabbInertia.m_mass = 0.0f;
    aabbInertia.m_centerOfMass.m_quad = {0.0f, 0.0f, 0.0f, 0.0f};
    aabbInertia.m_inertiaTensor.m_col0.m_quad = {0.0f, 0.0f, 0.0f, 0.0f};
    aabbInertia.m_inertiaTensor.m_col1.m_quad = {0.0f, 0.0f, 0.0f, 0.0f};
    aabbInertia.m_inertiaTensor.m_col2.m_quad = {0.0f, 0.0f, 0.0f, 0.0f};
    computeBoxVolumeMassProperties(aabbExtents, mass, aabbInertia);
    aabbInertia.m_centerOfMass.m_quad = aabbCenter.m_quad;
    alignas(16) hkVector4 shift;
    shift.setNeg4(aabbCenter);
    InternalInertiaTensorComputer computer;
    computer.compVolumeIntegrals(geom, shift);
    // Fails unless the mesh's volume is greater than zero or NaN.
    if (computer.T0 <= 0.0f) {
        return HK_FAILURE;
    }
    const float fVolume = computer.T0;
    result.m_volume = fVolume;
    result.m_mass = mass;
    const float fDensity = mass / fVolume;
    computer.computeInertialTensorInternal(mass, fDensity, result.m_centerOfMass, result.m_inertiaTensor);
    result.m_centerOfMass.add4(aabbCenter);
    // The same clamp as computeGeometryVolumeMassProperties.
    hkVector4* pResultColumns = &result.m_inertiaTensor.m_col0;
    const hkVector4* pAabbColumns = &aabbInertia.m_inertiaTensor.m_col0;
    for (int32_t i = 0; i < 3; ++i) {
        float* pColumn = &reinterpret_cast<float*>(&pResultColumns[i].m_quad)[0];
        const double dLimit = (&reinterpret_cast<const float*>(&pAabbColumns[i].m_quad)[0])[i] * 0.10000000149011612;
        if (pColumn[i] < dLimit) {
            pColumn[i] = static_cast<float>(dLimit);
            (&reinterpret_cast<float*>(&result.m_centerOfMass.m_quad)[0])[i] =
                (&reinterpret_cast<float*>(&aabbCenter.m_quad)[0])[i];
            for (int32_t j = 0; j < 3; ++j) {
                if (j == i) {
                    continue;
                }
                float fProduct = pColumn[j];
                if (fProduct < 0.0f) {
                    fProduct = 0.0f;
                }
                (&reinterpret_cast<float*>(&pResultColumns[j].m_quad)[0])[i] = fProduct;
                pColumn[j] = fProduct;
            }
        }
    }
    return HK_SUCCESS;
}

hkResult
hkpInertiaTensorComputer::combineMassProperties(const hkArray<hkpMassElement>& elements, hkpMassProperties& result) {
    alignas(16) hkVector4 lCenterOfMass;
    alignas(16) hkVector4 lTransformedCom;
    float fTotalMass = 0.0f;
    float fTotalVolume = 0.0f;
    lCenterOfMass.setZero4();
    if (elements.m_size <= 0) {
        return HK_FAILURE;
    }
    for (const hkpMassElement& rElement : elements) {
        lTransformedCom.setTransformedPos(rElement.m_transform, rElement.m_properties.m_centerOfMass);
        const float fMass = rElement.m_properties.m_mass;
        fTotalMass = fTotalMass + fMass;
        fTotalVolume = rElement.m_properties.m_volume + fTotalVolume;
        lCenterOfMass.addMul4(fMass, lTransformedCom);
    }
    // Continues only when the total mass is greater than zero (not when NaN).
    if (!(fTotalMass > 0.0f)) {
        return HK_FAILURE;
    }
    const float fInvMass = 1.0f / fTotalMass;
    result.m_mass = fTotalMass;
    lCenterOfMass.setMul4(fInvMass, lCenterOfMass);
    result.m_centerOfMass.m_quad = lCenterOfMass.m_quad;
    result.m_volume = fTotalVolume;
    result.m_inertiaTensor.setZero();
    for (const hkpMassElement& rElement : elements) {
        alignas(16) hkMatrix3 lInertia;
        lInertia.m_col0.m_quad = rElement.m_properties.m_inertiaTensor.m_col0.m_quad;
        lInertia.m_col1.m_quad = rElement.m_properties.m_inertiaTensor.m_col1.m_quad;
        lInertia.m_col2.m_quad = rElement.m_properties.m_inertiaTensor.m_col2.m_quad;
        lInertia.changeBasis(rElement.m_transform.getRotation());
        lTransformedCom.setTransformedPos(rElement.m_transform, rElement.m_properties.m_centerOfMass);
        lTransformedCom.sub4(lCenterOfMass);
        shiftInertiaFromCom(lTransformedCom, rElement.m_properties.m_mass, lInertia);
        result.m_inertiaTensor.add(lInertia);
    }
    return HK_SUCCESS;
}

// The solid sphere's properties minus the inner sphere's, scaled to mass.
hkResult hkpInertiaTensorComputer::computeSphereSurfaceMassProperties(
    float radius,
    float mass,
    float surfaceThickness,
    hkpMassProperties& result
) {
    if (mass <= 0.0f) {
        return HK_FAILURE;
    }
    const double dRadius = radius;
    const double dThickness = surfaceThickness;
    if (dThickness >= dRadius || dThickness <= 0.0) {
        return HK_FAILURE;
    }
    float innerRadius = dRadius - dThickness;
    // EXE-INLINED none: hkpMassProperties::hkpMassProperties
    hkpMassProperties resultBig;
    hkpMassProperties resultSmall;
    resultBig.m_inertiaTensor.m_col0.m_quad = _mm_setzero_ps();
    resultBig.m_inertiaTensor.m_col1.m_quad = _mm_setzero_ps();
    resultBig.m_inertiaTensor.m_col2.m_quad = _mm_setzero_ps();
    resultSmall.m_centerOfMass.m_quad = _mm_setzero_ps();
    resultSmall.m_inertiaTensor.m_col0.m_quad = _mm_setzero_ps();
    resultSmall.m_inertiaTensor.m_col1.m_quad = _mm_setzero_ps();
    resultSmall.m_inertiaTensor.m_col2.m_quad = _mm_setzero_ps();
    float sphereMass = dRadius * 4.188790321350098 * dRadius * dRadius;
    float innerMass = 4.188790321350098 * innerRadius * innerRadius * innerRadius;
    resultBig.m_volume = 0.0f;
    resultBig.m_mass = 0.0f;
    resultSmall.m_volume = 0.0f;
    resultSmall.m_mass = 0.0f;
    hkpInertiaTensorComputer::computeSphereVolumeMassProperties(radius, sphereMass, resultBig);
    hkpInertiaTensorComputer::computeSphereVolumeMassProperties(innerRadius, innerMass, resultSmall);
    result.m_centerOfMass.m_quad = _mm_setzero_ps();
    result.m_inertiaTensor.m_col0.m_quad = resultBig.m_inertiaTensor.m_col0.m_quad;
    result.m_inertiaTensor.m_col1.m_quad = resultBig.m_inertiaTensor.m_col1.m_quad;
    result.m_inertiaTensor.m_col2.m_quad = resultBig.m_inertiaTensor.m_col2.m_quad;
    result.m_inertiaTensor.sub(resultSmall.m_inertiaTensor);
    result.m_mass = resultBig.m_mass - resultSmall.m_mass;
    result.m_volume = resultBig.m_volume - resultSmall.m_volume;
    float fScale = mass / (sphereMass - innerMass);
    result.scaleMassAndInertia(fScale);
    return HK_SUCCESS;
}

// The solid box's properties minus the inner box's (half extents less surfaceThickness, w 0), scaled to mass. The two
// locals start with centres and inertias zeroed.
hkResult hkpInertiaTensorComputer::computeBoxSurfaceMassProperties(
    const hkVector4& halfExtents,
    float mass,
    float surfaceThickness,
    hkpMassProperties& result
) {
    if (mass <= 0.0f) {
        return HK_FAILURE;
    }
    const double dThickness = surfaceThickness;
    if (!(dThickness < halfExtents(0)) || !(halfExtents(1) > dThickness) || !(halfExtents(2) > dThickness) ||
        dThickness <= 0.0) {
        return HK_FAILURE;
    }
    float smallX = halfExtents(0) - dThickness;
    float smallY = halfExtents(1) - dThickness;
    float smallZ = halfExtents(2) - dThickness;
    hkVector4 halfExtentsSmall;
    halfExtentsSmall.m_quad = _mm_unpacklo_ps(
        _mm_unpacklo_ps(_mm_set_ss(smallX), _mm_set_ss(smallZ)), _mm_unpacklo_ps(_mm_set_ss(smallY), _mm_set_ss(0.0f))
    );
    float bigMass = halfExtents(0) * 8.0 * halfExtents(1) * halfExtents(2);
    // EXE-INLINED none: hkpMassProperties::hkpMassProperties
    hkpMassProperties resultBig;
    hkpMassProperties resultSmall;
    resultBig.m_centerOfMass.m_quad = _mm_setzero_ps();
    resultBig.m_inertiaTensor.m_col0.m_quad = _mm_setzero_ps();
    resultBig.m_inertiaTensor.m_col1.m_quad = _mm_setzero_ps();
    resultBig.m_inertiaTensor.m_col2.m_quad = _mm_setzero_ps();
    resultSmall.m_centerOfMass.m_quad = _mm_setzero_ps();
    resultSmall.m_inertiaTensor.m_col0.m_quad = _mm_setzero_ps();
    resultSmall.m_inertiaTensor.m_col1.m_quad = _mm_setzero_ps();
    resultSmall.m_inertiaTensor.m_col2.m_quad = _mm_setzero_ps();
    float smallMass = 8.0 * halfExtentsSmall(0) * halfExtentsSmall(1) * halfExtentsSmall(2);
    resultBig.m_volume = 0.0f;
    resultBig.m_mass = 0.0f;
    resultSmall.m_volume = 0.0f;
    resultSmall.m_mass = 0.0f;
    hkpInertiaTensorComputer::computeBoxVolumeMassProperties(halfExtents, bigMass, resultBig);
    hkpInertiaTensorComputer::computeBoxVolumeMassProperties(halfExtentsSmall, smallMass, resultSmall);
    result.m_centerOfMass.m_quad = _mm_setzero_ps();
    result.m_inertiaTensor = resultBig.m_inertiaTensor;
    result.m_inertiaTensor.sub(resultSmall.m_inertiaTensor);
    result.m_mass = resultBig.m_mass - resultSmall.m_mass;
    result.m_volume = resultBig.m_volume - resultSmall.m_volume;
    float fScale = mass / (bigMass - smallMass);
    result.scaleMassAndInertia(fScale);
    return HK_SUCCESS;
}

// Replaces the matrix with a zeroed matrix holding the largest diagonal element on its diagonal; on a tie or NaN the
// compares keep the earlier element, as the game does.
void hkpInertiaTensorComputer::simplifyInertiaTensorToOrientedParticle(hkMatrix3& inertia) {
    float a = inertia(0, 0);
    float b = inertia(1, 1);
    float fMax = (b < a) ? a : b;
    float c = inertia(2, 2);
    fMax = (c < fMax) ? fMax : c;
    inertia.setDiagonal(fMax, fMax, fMax);
}

// Each pass takes the largest below-diagonal element of inertia (row q, column p), stops once its square is below
// 2^-46, and applies the Jacobi rotation rot that zeroes it: inertia = rot * inertia * rot^T, accumulated as
// principleAxisOut = principleAxisOut * rot^T. At most five passes; then principleAxisOut's columns are made
// orthonormal again (column 0 normalised, column 1 = column 2 x column 0, column 2 = column 0 x column 1, each
// normalised). Every scalar temporary is kept as a float, the sign of theta as a double.
void hkpInertiaTensorComputer::convertInertiaTensorToPrincipleAxis(hkMatrix3& inertia, hkRotation& principleAxisOut) {
    principleAxisOut.setIdentity();
    for (int32_t iteration = 5; iteration > 0; --iteration) {
        int32_t p = 0;
        int32_t q = 1;
        float maxSquare = inertia(1, 0) * inertia(1, 0);
        const float a20 = inertia(2, 0);
        const float square20 = a20 * a20;
        if (maxSquare < square20) {
            maxSquare = square20;
            q = 2;
        }
        const float a21 = inertia(2, 1);
        const float square21 = a21 * a21;
        if (maxSquare < square21) {
            maxSquare = square21;
            p = 1;
            q = 2;
        }
        if (maxSquare < 1.4210855e-14f) {
            break;
        }
        const float theta = (inertia(q, q) - inertia(p, p)) / (inertia(p, q) + inertia(p, q));
        const float sign = (theta > 0.0f) ? 1.0f : -1.0f;
        const double dSign = sign;
        const float thetaSquare1 = theta * theta + 1.0;
        const float root1 = std::sqrt(thetaSquare1);
        const float absTheta = std::fabs(theta);
        const float t = dSign / (root1 + absTheta);
        const float tSquare1 = t * t + 1.0;
        const float root2 = std::sqrt(tSquare1);
        const float c = 1.0f / root2;
        const float s = c * t;
        hkMatrix3 rot;
        rot.setIdentity();
        rot(p, p) = c;
        rot(q, q) = c;
        const float negS = -s;
        rot(p, q) = negS;
        rot(q, p) = s;
        hkMatrix3 tmp;
        tmp.setMul(rot, inertia);
        rot(p, q) = s;
        rot(q, p) = negS;
        inertia.setMul(tmp, rot);
        tmp.setMul(principleAxisOut, rot);
        static_cast<hkMatrix3&>(principleAxisOut) = tmp;
    }
    // EXE-INLINED none: hkRotation::renormalize
    principleAxisOut.m_col0.normalize3();
    {
        const __m128 col0 = principleAxisOut.m_col0.m_quad;
        const __m128 col2 = principleAxisOut.m_col2.m_quad;
        // EXE-INLINED 0x630DF0
        principleAxisOut.m_col1.m_quad = _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(col0, col0, 0xD2), _mm_shuffle_ps(col2, col2, 0xC9)),
            _mm_mul_ps(_mm_shuffle_ps(col0, col0, 0xC9), _mm_shuffle_ps(col2, col2, 0xD2))
        );
    }
    principleAxisOut.m_col1.normalize3();
    {
        const __m128 col0 = principleAxisOut.m_col0.m_quad;
        const __m128 col1 = principleAxisOut.m_col1.m_quad;
        // EXE-INLINED 0x630DF0
        principleAxisOut.m_col2.m_quad = _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(col0, col0, 0xC9), _mm_shuffle_ps(col1, col1, 0xD2)),
            _mm_mul_ps(_mm_shuffle_ps(col0, col0, 0xD2), _mm_shuffle_ps(col1, col1, 0xC9))
        );
    }
    principleAxisOut.m_col2.normalize3();
}

// The vertices go into a local hkArray<hkVector4> (w 0), the centre of mass is their mean and each vertex adds its
// point-mass inertia about it.
hkResult hkpInertiaTensorComputer::computeVertexCloudMassProperties(
    const float* vertexIn,
    int32_t striding,
    int32_t numVertices,
    float mass,
    hkpMassProperties& result
) {
    if (numVertices <= 0) {
        return HK_FAILURE;
    }
    hkArray<hkVector4> verts;
    verts.setSize(numVertices);
    const float* pVertex = vertexIn;
    for (int32_t i = 0; i < numVertices; ++i) {
        verts.m_data[i].set(pVertex[0], pVertex[1], pVertex[2], 0.0f);
        // The vertex buffer is untyped memory: the pointer steps by striding bytes (hkAddByteOffset).
        pVertex = reinterpret_cast<const float*>(reinterpret_cast<const char*>(pVertex) + striding);
    }
    result.m_mass = mass;
    result.m_inertiaTensor.setZero();
    result.m_centerOfMass.m_quad = _mm_setzero_ps();
    const float fNumVertices = static_cast<float>(numVertices);
    const float massPerVertex = mass / fNumVertices;
    for (int32_t i = 0; i < numVertices; ++i) {
        result.m_centerOfMass.m_quad = _mm_add_ps(verts.m_data[i].m_quad, result.m_centerOfMass.m_quad);
    }
    const float invNumVertices = 1.0f / fNumVertices;
    result.m_centerOfMass.m_quad = _mm_mul_ps(
        _mm_shuffle_ps(_mm_set_ss(invNumVertices), _mm_set_ss(invNumVertices), 0), result.m_centerOfMass.m_quad
    );
    hkMatrix3& inertia = result.m_inertiaTensor;
    for (int32_t i = 0; i < numVertices; ++i) {
        const float x = verts.m_data[i](0) - result.m_centerOfMass(0);
        const float y = verts.m_data[i](1) - result.m_centerOfMass(1);
        const float z = verts.m_data[i](2) - result.m_centerOfMass(2);
        inertia(0, 0) = (y * y + z * z) * massPerVertex + inertia(0, 0);
        inertia(1, 1) = (z * z + x * x) * massPerVertex + inertia(1, 1);
        inertia(2, 2) = (y * y + x * x) * massPerVertex + inertia(2, 2);
        inertia(0, 1) = inertia(0, 1) - y * x * massPerVertex;
        inertia(1, 0) = inertia(1, 0) - y * x * massPerVertex;
        inertia(0, 2) = inertia(0, 2) - x * z * massPerVertex;
        inertia(2, 0) = inertia(2, 0) - x * z * massPerVertex;
        inertia(2, 1) = inertia(2, 1) - z * y * massPerVertex;
        inertia(1, 2) = inertia(1, 2) - z * y * massPerVertex;
    }
    return HK_SUCCESS;
}

// Each triangle gets a share of the mass: by area (half its edge cross product's length, 5e-06 below 1e-05) when
// distributeUniformly is set, else an equal one. Its plate's properties are summed into result about the origin, then
// the centre of mass is divided out and the inertia moved to it.
hkResult hkpInertiaTensorComputer::computeGeometrySurfaceMassProperties(
    const hkGeometry* geom,
    float surfaceThickness,
    hkBool distributeUniformly,
    float mass,
    hkpMassProperties& result
) {
    if (mass <= 0.0f || surfaceThickness <= 0.0f) {
        return HK_FAILURE;
    }
    result.m_inertiaTensor.setZero();
    result.m_centerOfMass.m_quad = _mm_setzero_ps();
    const int32_t numTriangles = geom->m_triangles.m_size;
    hkArray<float> triArea;
    triArea.setSize(numTriangles);
    float totalArea = 0.0f;
    for (int32_t i = 0; i < geom->m_triangles.m_size; ++i) {
        const hkGeometry::Triangle& tri = geom->m_triangles.m_data[i];
        const __m128 vb = geom->m_vertices.m_data[tri.m_b].m_quad;
        const __m128 edgeCB = _mm_sub_ps(geom->m_vertices.m_data[tri.m_c].m_quad, vb);
        const __m128 edgeAB = _mm_sub_ps(geom->m_vertices.m_data[tri.m_a].m_quad, vb);
        // EXE-INLINED 0x630DF0
        const __m128 cross = _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(edgeAB, edgeAB, 0xD2), _mm_shuffle_ps(edgeCB, edgeCB, 0xC9)),
            _mm_mul_ps(_mm_shuffle_ps(edgeAB, edgeAB, 0xC9), _mm_shuffle_ps(edgeCB, edgeCB, 0xD2))
        );
        hkVector4 crossVector;
        crossVector.m_quad = cross;
        const float length = crossVector.length3();
        if (distributeUniformly.m_bool) {
            if (length < 1e-05f) {
                triArea.m_data[i] = 5e-06f;
            }
            else {
                triArea.m_data[i] = length * 0.5;
            }
        }
        else {
            triArea.m_data[i] = 1.0f;
        }
        totalArea = triArea.m_data[i] + totalArea;
    }
    for (int32_t i = 0; i < geom->m_triangles.m_size; ++i) {
        const hkGeometry::Triangle& tri = geom->m_triangles.m_data[i];
        hkVector4 v0;
        v0.m_quad = geom->m_vertices.m_data[tri.m_a].m_quad;
        hkVector4 v1;
        v1.m_quad = geom->m_vertices.m_data[tri.m_b].m_quad;
        hkVector4 v2;
        v2.m_quad = geom->m_vertices.m_data[tri.m_c].m_quad;
        const float triMass = triArea.m_data[i] * mass / totalArea;
        // EXE-INLINED none: hkpMassProperties::hkpMassProperties
        hkpMassProperties triProps;
        triProps.m_centerOfMass.m_quad = _mm_setzero_ps();
        triProps.m_inertiaTensor.m_col0.m_quad = _mm_setzero_ps();
        triProps.m_inertiaTensor.m_col1.m_quad = _mm_setzero_ps();
        triProps.m_inertiaTensor.m_col2.m_quad = _mm_setzero_ps();
        triProps.m_volume = 0.0f;
        triProps.m_mass = 0.0f;
        hkpInertiaTensorComputer::computeTriangleSurfaceMassProperties(v0, v1, v2, triMass, surfaceThickness, triProps);
        hkMatrix3 triInertia;
        triInertia = triProps.m_inertiaTensor;
        result.m_centerOfMass.m_quad = _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(triMass), _mm_set_ss(triMass), 0), triProps.m_centerOfMass.m_quad),
            result.m_centerOfMass.m_quad
        );
        hkVector4 shift;
        shift.m_quad = triProps.m_centerOfMass.m_quad;
        hkpInertiaTensorComputer::shiftInertiaFromCom(shift, triMass, triInertia);
        result.m_inertiaTensor.add(triInertia);
    }
    result.m_mass = mass;
    const float invMass = 1.0f / mass;
    result.m_centerOfMass.m_quad =
        _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(invMass), _mm_set_ss(invMass), 0), result.m_centerOfMass.m_quad);
    hkpInertiaTensorComputer::shiftInertiaToCom(result.m_centerOfMass, mass, result.m_inertiaTensor);
    return HK_SUCCESS;
}

// The eigenvectors of a copy of inertiaTensor become the rows of the rotation of a transform without translation;
// pShape's AABB in that frame picks them: majorAxis is the one of its longest extent, minorAxis the one of its longest
// extent once that one is zeroed (each the first of equal ones).
void hkpInertiaTensorComputer::calculatePrincipalAxis(
    const hkpShape* pShape,
    const hkMatrix3& inertiaTensor,
    hkVector4& majorAxis,
    hkVector4& minorAxis
) {
    hkMatrix3 tensor;
    tensor = inertiaTensor;
    uint8_t converged;
    hkVector4 eigenVector0;
    hkVector4 eigenVector1;
    hkVector4 eigenVector2;
    calcEigenVectors(
        &converged, &tensor.m_col0.m_quad, &eigenVector0.m_quad, &eigenVector1.m_quad, &eigenVector2.m_quad
    );
    hkTransform aabbTransform;
    aabbTransform.m_rotation.setRows(eigenVector0, eigenVector1, eigenVector2);
    aabbTransform.m_translation.m_quad = _mm_setzero_ps();
    hkAabb aabb;
    // EXE-INLINED 0x6286A0
    pShape->getAabbImpl(aabbTransform, 0.0f, aabb);
    hkVector4 diff;
    diff.m_quad = _mm_sub_ps(aabb.m_max.m_quad, aabb.m_min.m_quad);
    const int32_t major = diff.getMajorAxis3();
    diff(major) = 0.0f;
    const int32_t minor = diff.getMajorAxis3();
    majorAxis.m_quad = aabbTransform.m_rotation.getColumn(major).m_quad;
    minorAxis.m_quad = aabbTransform.m_rotation.getColumn(minor).m_quad;
}

void hkpInertiaTensorComputer::setAndScaleToMass(
    const hkpMassProperties& props,
    float mass,
    hkpRigidBodyCinfo& bodyInfo
) {
    uint32_t v3;
    hkSimdReal v4;
    *reinterpret_cast<float*>(&v3) = mass / props.m_mass;
    bodyInfo.m_mass = mass;
    bodyInfo.m_centerOfMass = props.m_centerOfMass;
    bodyInfo.m_inertiaTensor = props.m_inertiaTensor;
    v4.m_real = _mm_set_ss(std::bit_cast<float>(v3));
    bodyInfo.m_inertiaTensor.mul(v4);
}

// Havok SDK: copies the mass, centre of mass and inertia tensor into bodyInfo.
void hkpInertiaTensorComputer::setMassProperties(const hkpMassProperties& massProperties, hkpRigidBodyCinfo& bodyInfo) {
    float fMass = massProperties.m_mass;
    bodyInfo.m_mass = fMass;
    bodyInfo.m_centerOfMass = massProperties.m_centerOfMass;
    bodyInfo.m_inertiaTensor = massProperties.m_inertiaTensor;
}

// Havok SDK: setAndScaleToMass with the mass that props' volume has at density.
void hkpInertiaTensorComputer::setAndScaleToDensity(
    const hkpMassProperties& props,
    float density,
    hkpRigidBodyCinfo& bodyInfo
) {
    setAndScaleToMass(props, props.m_volume * density, bodyInfo);
}

hkResult hkpInertiaTensorComputer::computeVertexHullVolumeMassProperties(
    const float* vertexIn,
    int32_t striding,
    int32_t numVertices,
    float mass,
    hkpMassProperties& result
) {
    hkStridedVertices vertexIna;
    if (mass <= 0.0f || numVertices < 1) {
        return HK_FAILURE;
    }
    vertexIna.m_numVertices = numVertices;
    vertexIna.m_striding = striding;
    vertexIna.m_vertices = vertexIn;
    hkpInertiaTensorComputer::computeConvexHullMassProperties(vertexIna, 0.0f, result);
    result.scaleToMass(mass);
    return HK_SUCCESS;
}

// The constraints copied into a stack-allocated local array for the recursive helper, whose result is dropped.
void hkpInertiaTensorComputer::optimizeInertiasOfConstraintTree(
    hkpConstraintInstance* const* constraints,
    int32_t numConstraints,
    hkpRigidBody* rootBody,
    float inertiaFactorHint
) {
    hkLocalArray<const hkpConstraintInstance*> constrs(numConstraints);
    for (int32_t i = 0; i < numConstraints; ++i) {
        constrs.pushBackUnchecked(constraints[i]);
    }
    hkInertiaTensorComputer_optimizeInertiasOfConstraintTreeInt(constrs, rootBody, inertiaFactorHint);
}
