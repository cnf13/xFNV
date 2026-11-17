#include "Havok/Source/Physics/Utilities/Collide/ShapeUtils/CollapseTransform/hkpTransformCollapseUtil.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpLinkedCollidable.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/List/hkpListShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Box/hkpBoxShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Capsule/hkpCapsuleShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/ConvexTransform/hkpConvexTransformShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/ConvexTranslate/hkpConvexTranslateShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/ConvexVertices/hkpConvexVerticesShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Cylinder/hkpCylinderShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexTransformShapeBase.h"
#include "Havok/Source/Physics/Collide/Shape/Misc/Transform/hkpTransformShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

// The child collector of _SharedShapeData::recursivelyParseShapeReferences: appends apShape's child shapes to
// apChildren by its type: for 9 each child of its shape container, for 0xB and 0xC one child shape, for 0xF another.
static void collectChildShapes(const hkpShape* apShape, hkArrayBase<const hkpShape*>* apChildren);

// A list's children are its child infos' shapes, counted through its shape container's getNumChildShapes before each
// one; the convex translate and convex transform shapes' child and the transform shape's child are one more.
static void collectChildShapes(const hkpShape* apShape, hkArrayBase<const hkpShape*>* apChildren) {
    const hkpShape* pChild;
    switch (apShape->m_type.m_storage) {
        case HK_SHAPE_LIST: {
            const hkpListShape* pList = static_cast<const hkpListShape*>(apShape);
            const hkpShapeContainer* pContainer = pList;
            for (int32_t i = 0; i < pContainer->getNumChildShapes(); ++i) {
                pChild = pList->m_childInfo.m_data[i].m_shape;
                // EXE-INLINED none: hkArray<const hkpShape*>::pushBack
                hkMemoryRouter& rRouter = hkMemoryRouter::getInstance();
                if (apChildren->m_size ==
                    (apChildren->m_capacityAndFlags & hkArrayBase<const hkpShape*>::CAPACITY_MASK)) {
                    hkArrayUtil::_reserveMore(*rRouter.m_heap, apChildren, sizeof(const hkpShape*));
                }
                apChildren->m_data[apChildren->m_size] = pChild;
                apChildren->m_size++;
            }
            return;
        }
        case HK_SHAPE_CONVEX_TRANSLATE:
        case HK_SHAPE_CONVEX_TRANSFORM:
            pChild = static_cast<const hkpConvexTransformShapeBase*>(apShape)->m_childShape.m_childShape;
            break;
        case HK_SHAPE_TRANSFORM:
            pChild = static_cast<const hkpTransformShape*>(apShape)->m_childShape.m_childShape;
            break;
        default: return;
    }
    // EXE-INLINED none: hkArray<const hkpShape*>::pushBack
    hkMemoryRouter& rRouter = hkMemoryRouter::getInstance();
    if (apChildren->m_size == (apChildren->m_capacityAndFlags & hkArrayBase<const hkpShape*>::CAPACITY_MASK)) {
        hkArrayUtil::_reserveMore(*rRouter.m_heap, apChildren, sizeof(const hkpShape*));
    }
    apChildren->m_data[apChildren->m_size] = pChild;
    apChildren->m_size++;
}

hkpTransformCollapseUtil::Options::Options() : m_sharedShapeBehaviour(ALWAYS_COLLAPSE), m_sharedShapeThreshold(0) {
    m_propageTransformInList.m_bool = 1;
}

// The four counters zeroed, then reset.
hkpTransformCollapseUtil::Results::Results()
  : m_numCollapsedShapes(0), m_numIdentityTransformsRemoved(0), m_numSpecializedTransformShapes(0),
    m_numPropagatedTransformsToLists(0) {
    reset();
}

void hkpTransformCollapseUtil::Results::reset() {
    m_numCollapsedShapes = 0;
    m_numIdentityTransformsRemoved = 0;
    m_numSpecializedTransformShapes = 0;
    m_numPropagatedTransformsToLists = 0;
}

void hkpTransformCollapseUtil::Results::report() const {
    FNVD_INLINED_LOCAL(hkErrStream, apThis, std::nullopt, std::nullopt);
    char buf[512];
    ::new (&apThis) hkErrStream(buf, 512);
    int32_t m_numCollapsedShapes = this->m_numCollapsedShapes;
    hkOstream* v2 = &apThis.operator<<("Number of collapsed shapes :");
    v2->operator<<(m_numCollapsedShapes);
    hkError::s_instance->message(
        hkError::MESSAGE_REPORT, -1, buf, ".\\Collide\\ShapeUtils\\CollapseTransform\\hkpTransformCollapseUtil.cpp", 57
    );
    FNVD_DESTRUCT(hkOstream)(&apThis);
    ::new (&apThis) hkErrStream(buf, 512);
    int32_t m_numIdentityTransformsRemoved = this->m_numIdentityTransformsRemoved;
    hkOstream* v3 = &apThis.operator<<("Number of identity transforms removed : ");
    v3->operator<<(m_numIdentityTransformsRemoved);
    hkError::s_instance->message(
        hkError::MESSAGE_REPORT, -1, buf, ".\\Collide\\ShapeUtils\\CollapseTransform\\hkpTransformCollapseUtil.cpp", 58
    );
    FNVD_DESTRUCT(hkOstream)(&apThis);
    ::new (&apThis) hkErrStream(buf, 512);
    int32_t m_numSpecializedTransformShapes = this->m_numSpecializedTransformShapes;
    hkOstream* v4 = &apThis.operator<<("Number of specialized transform shapes : ");
    v4->operator<<(m_numSpecializedTransformShapes);
    hkError::s_instance->message(
        hkError::MESSAGE_REPORT, -1, buf, ".\\Collide\\ShapeUtils\\CollapseTransform\\hkpTransformCollapseUtil.cpp", 59
    );
    FNVD_DESTRUCT(hkOstream)(&apThis);
    ::new (&apThis) hkErrStream(buf, 512);
    int32_t m_numPropagatedTransformsToLists = this->m_numPropagatedTransformsToLists;
    hkOstream* v5 = &apThis.operator<<("Number of propagated transforms to lists : ");
    v5->operator<<(m_numPropagatedTransformsToLists);
    hkError::s_instance->message(
        hkError::MESSAGE_REPORT, -1, buf, ".\\Collide\\ShapeUtils\\CollapseTransform\\hkpTransformCollapseUtil.cpp", 60
    );
    FNVD_DESTRUCT(hkOstream)(&apThis);
}

// The transform (a convex translate, convex transform or transform shape's) over a sphere drops its rotation; an
// identity transform gives the child. A list child gets the transform pushed into each of its children (only when
// m_propageTransformInList is false) and collapse is allowed by the shared-shape rule. Otherwise, where allowed, a
// cylinder, capsule or convex vertices child is rebuilt transformed, and a box child under an axis permutation becomes
// a permuted box (under a translation, wrapped in a convex translate shape). What is left is specialized: a convex
// translate shape for a pure translation (unless it is one already), a convex transform shape for a transform shape,
// else the shape itself.
const hkpShape* hkpTransformCollapseUtil::collapseTransformShape(
    const hkpShape* transformShape,
    const Options& options,
    _SharedShapeData& sharedShapeData,
    Results& resultsOut
) {
    const int32_t shapeType = transformShape->m_type.m_storage;
    hkTransform parentFromChild;
    const hkpShape* child;
    switch (shapeType) {
        case HK_SHAPE_CONVEX_TRANSLATE: {
            const hkpConvexTranslateShape* pTranslate = static_cast<const hkpConvexTranslateShape*>(transformShape);
            // EXE-INLINED 0x577070: hkTransform::setIdentity
            parentFromChild.m_rotation.m_col0.m_quad = _mm_setr_ps(1.0f, 0.0f, 0.0f, 0.0f);
            parentFromChild.m_rotation.m_col1.m_quad = _mm_setr_ps(0.0f, 1.0f, 0.0f, 0.0f);
            parentFromChild.m_rotation.m_col2.m_quad = _mm_setr_ps(0.0f, 0.0f, 1.0f, 0.0f);
            parentFromChild.m_translation.m_quad = pTranslate->m_translation.m_quad;
            child = pTranslate->m_childShape.m_childShape;
            break;
        }
        case HK_SHAPE_CONVEX_TRANSFORM: {
            const hkpConvexTransformShape* pTransform = static_cast<const hkpConvexTransformShape*>(transformShape);
            parentFromChild.m_rotation.m_col0.m_quad = pTransform->m_transform.m_rotation.m_col0.m_quad;
            parentFromChild.m_rotation.m_col1.m_quad = pTransform->m_transform.m_rotation.m_col1.m_quad;
            parentFromChild.m_rotation.m_col2.m_quad = pTransform->m_transform.m_rotation.m_col2.m_quad;
            parentFromChild.m_translation.m_quad = pTransform->m_transform.m_translation.m_quad;
            child = pTransform->m_childShape.m_childShape;
            break;
        }
        case HK_SHAPE_TRANSFORM: {
            const hkpTransformShape* pTransform = static_cast<const hkpTransformShape*>(transformShape);
            parentFromChild.m_rotation.m_col0.m_quad = pTransform->m_transform.m_rotation.m_col0.m_quad;
            parentFromChild.m_rotation.m_col1.m_quad = pTransform->m_transform.m_rotation.m_col1.m_quad;
            parentFromChild.m_rotation.m_col2.m_quad = pTransform->m_transform.m_rotation.m_col2.m_quad;
            parentFromChild.m_translation.m_quad = pTransform->m_transform.m_translation.m_quad;
            child = pTransform->m_childShape.m_childShape;
            break;
        }
        default: transformShape->addReference(); return transformShape;
    }

    const int32_t childType = child->m_type.m_storage;
    if (childType == HK_SHAPE_SPHERE) {
        parentFromChild.m_rotation.m_col0.m_quad = hkTransformIdentity.m_rotation.m_col0.m_quad;
        parentFromChild.m_rotation.m_col1.m_quad = hkTransformIdentity.m_rotation.m_col1.m_quad;
        parentFromChild.m_rotation.m_col2.m_quad = hkTransformIdentity.m_rotation.m_col2.m_quad;
    }
    if (parentFromChild.isApproximatelyEqual(hkTransformIdentity, 0.001f)) {
        resultsOut.m_numIdentityTransformsRemoved++;
        child->addReference();
        sharedShapeData.setReplacement(transformShape, child);
        return child;
    }

    bool collapseAllowed = true;
    if (options.m_sharedShapeBehaviour == ALWAYS_COLLAPSE) {
        collapseAllowed = true;
    }
    else if (options.m_sharedShapeBehaviour == NEVER_COLLAPSE) {
        collapseAllowed = false;
    }
    else if (options.m_sharedShapeBehaviour == COLLAPSE_IF_LESS_THAN_THRESHOLD) {
        collapseAllowed = sharedShapeData.numberOfReferences(child) < options.m_sharedShapeThreshold;
    }

    if (childType == HK_SHAPE_LIST) {
        if (options.m_propageTransformInList.m_bool || !collapseAllowed) {
            transformShape->addReference();
            return transformShape;
        }
        resultsOut.m_numPropagatedTransformsToLists++;
        const hkpListShape* pList = static_cast<const hkpListShape*>(child);
        const hkpShapeContainer* pContainer = pList;
        // EXE-INLINED none: hkArray<const hkpShape*>::hkArray
        FNVD_INLINED_LOCAL(hkArray<const hkpShape*>, newChildren, std::nullopt, std::nullopt);
        newChildren.m_data = nullptr;
        newChildren.m_size = 0;
        newChildren.m_capacityAndFlags = hkArrayBase<const hkpShape*>::DONT_DEALLOCATE_FLAG;
        for (int32_t i = 0; i < pContainer->getNumChildShapes(); ++i) {
            const hkpShape* pGrandchild = pList->m_childInfo.m_data[i].m_shape;
            void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpTransformShape));
            static_cast<hkpTransformShape*>(pMemory)->m_memSizeAndFlags = sizeof(hkpTransformShape);
            hkpTransformShape* pTransformed = ::new (pMemory) hkpTransformShape(pGrandchild, parentFromChild);
            const hkpShape* pCollapsed = collapseTransformShape(pTransformed, options, sharedShapeData, resultsOut);
            // EXE-INLINED none: hkArray<const hkpShape*>::pushBack
            hkMemoryRouter& rRouter = hkMemoryRouter::getInstance();
            if (newChildren.m_size == (newChildren.m_capacityAndFlags & hkArrayBase<const hkpShape*>::CAPACITY_MASK)) {
                hkArrayUtil::_reserveMore(*rRouter.m_heap, &newChildren, sizeof(const hkpShape*));
            }
            newChildren.m_data[newChildren.m_size] = pCollapsed;
            newChildren.m_size++;
            pTransformed->removeReference();
        }
        void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpListShape));
        static_cast<hkpListShape*>(pMemory)->m_memSizeAndFlags = sizeof(hkpListShape);
        hkpListShape* pNewList = ::new (pMemory)
            hkpListShape(newChildren.m_data, newChildren.m_size, hkpShapeContainer::REFERENCE_POLICY_INCREMENT);
        for (int32_t i = 0; i < newChildren.m_size; ++i) {
            newChildren.m_data[i]->removeReference();
        }
        sharedShapeData.setReplacement(transformShape, pNewList);
        // EXE-INLINED none: hkArray<const hkpShape*>::~hkArray
        hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
        newChildren.m_size = 0;
        if (newChildren.m_capacityAndFlags >= 0) {
            pHeap->bufFree(
                newChildren.m_data,
                static_cast<int32_t>(
                    (newChildren.m_capacityAndFlags & hkArrayBase<const hkpShape*>::CAPACITY_MASK) *
                    sizeof(const hkpShape*)
                )
            );
        }
        return pNewList;
    }

    if (collapseAllowed) {
        switch (childType) {
            case HK_SHAPE_CYLINDER: {
                const hkpCylinderShape* pCylinder = static_cast<const hkpCylinderShape*>(child);
                const float cylinderRadius = pCylinder->getCylinderRadius();
                // An x87 copy.
                const float radius = pCylinder->m_radius;
                hkVector4 vertexA;
                vertexA.setTransformedPos(parentFromChild, pCylinder->m_vertexA);
                hkVector4 vertexB;
                vertexB.setTransformedPos(parentFromChild, pCylinder->m_vertexB);
                void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpCylinderShape));
                static_cast<hkpCylinderShape*>(pMemory)->m_memSizeAndFlags = sizeof(hkpCylinderShape);
                const hkpShape* pNewShape = ::new (pMemory) hkpCylinderShape(vertexA, vertexB, cylinderRadius, radius);
                sharedShapeData.setReplacement(transformShape, pNewShape);
                resultsOut.m_numCollapsedShapes++;
                return pNewShape;
            }
            case HK_SHAPE_BOX: {
                const hkRotation& rotation = parentFromChild.m_rotation;
                hkBool orthonormal;
                if (!rotation.isOrthonormal(&orthonormal, 0.001f)->m_bool || !rotation.isOk().m_bool ||
                    rotation.isApproximatelyEqual(hkTransformIdentity.m_rotation, 0.001f).m_bool) {
                    break;
                }
                // The row each column's unit element sits in: the box's axes permuted. Each element must be 0 or +-1
                // and each column's absolute sum 1, within 1e-4. Every row is written once the rotation passed
                // isOrthonormal.
                int32_t axisOfRow[3];
                bool isPermutation = true;
                for (int32_t column = 0; column < 3 && isPermutation; ++column) {
                    const hkVector4& rColumn = (&rotation.m_col0)[column];
                    bool found = false;
                    for (int32_t row = 0; row < 3; ++row) {
                        const float absolute = __builtin_fabsf(rColumn.m_quad[row]);
                        const bool isZero = absolute < 9.999999747378752e-05;
                        const float offOne = absolute - 1.0f;
                        const float absoluteOffOne = __builtin_fabsf(offOne);
                        const bool isOne = absoluteOffOne < 9.999999747378752e-05;
                        if (!isZero && !isOne) {
                            isPermutation = false;
                            break;
                        }
                        if (isOne) {
                            axisOfRow[row] = column;
                            found = true;
                        }
                    }
                    if (!isPermutation || !found) {
                        isPermutation = false;
                        break;
                    }
                    const float absY = __builtin_fabsf(rColumn.m_quad[1]);
                    const float absX = __builtin_fabsf(rColumn.m_quad[0]);
                    const float sumX = absX + 0.0f;
                    const float sumXY = absY + sumX;
                    const float absZ = __builtin_fabsf(rColumn.m_quad[2]);
                    const float sumXYZ = sumXY + absZ;
                    if (!(sumXYZ - 1.0 < 9.999999747378752e-05)) {
                        isPermutation = false;
                    }
                }
                if (!isPermutation) {
                    break;
                }
                const hkpBoxShape* pBox = static_cast<const hkpBoxShape*>(child);
                hkVector4 halfExtents;
                halfExtents.m_quad = _mm_setr_ps(
                    pBox->m_halfExtents.m_quad[axisOfRow[0]], pBox->m_halfExtents.m_quad[axisOfRow[1]],
                    pBox->m_halfExtents.m_quad[axisOfRow[2]], 0.0f
                );
                void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpBoxShape));
                static_cast<hkpBoxShape*>(pMemory)->m_memSizeAndFlags = sizeof(hkpBoxShape);
                hkpBoxShape* pNewBox = ::new (pMemory) hkpBoxShape(halfExtents, hkConvexShapeDefaultRadius);
                const hkQuadReal squared =
                    _mm_mul_ps(parentFromChild.m_translation.m_quad, parentFromChild.m_translation.m_quad);
                hkQuadReal sum = _mm_add_ss(_mm_shuffle_ps(squared, squared, 0x55), squared);
                sum = _mm_add_ss(_mm_shuffle_ps(squared, squared, 0xAA), sum);
                const float length = _mm_cvtss_f32(_mm_sqrt_ss(sum));
                if (length < 9.999999747378752e-05) {
                    sharedShapeData.setReplacement(transformShape, pNewBox);
                    resultsOut.m_numCollapsedShapes++;
                    return pNewBox;
                }
                void* pTranslateMemory =
                    hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpConvexTranslateShape));
                static_cast<hkpConvexTranslateShape*>(pTranslateMemory)->m_memSizeAndFlags =
                    sizeof(hkpConvexTranslateShape);
                hkpConvexTranslateShape* pTranslate = ::new (pTranslateMemory) hkpConvexTranslateShape(
                    pNewBox, parentFromChild.m_translation, hkpShapeContainer::REFERENCE_POLICY_INCREMENT
                );
                pNewBox->removeReference();
                sharedShapeData.setReplacement(transformShape, pTranslate);
                resultsOut.m_numSpecializedTransformShapes++;
                return pTranslate;
            }
            case HK_SHAPE_CAPSULE: {
                const hkpCapsuleShape* pCapsule = static_cast<const hkpCapsuleShape*>(child);
                // An x87 copy.
                const float radius = pCapsule->m_radius;
                hkVector4 vertexA;
                vertexA.setTransformedPos(parentFromChild, pCapsule->m_vertexA);
                hkVector4 vertexB;
                vertexB.setTransformedPos(parentFromChild, pCapsule->m_vertexB);
                void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpCapsuleShape));
                static_cast<hkpCapsuleShape*>(pMemory)->m_memSizeAndFlags = sizeof(hkpCapsuleShape);
                const hkpShape* pNewShape = ::new (pMemory) hkpCapsuleShape(vertexA, vertexB, radius);
                sharedShapeData.setReplacement(transformShape, pNewShape);
                resultsOut.m_numCollapsedShapes++;
                return pNewShape;
            }
            case HK_SHAPE_CONVEX_VERTICES: {
                const hkpConvexVerticesShape* pVertices = static_cast<const hkpConvexVerticesShape*>(child);
                // EXE-INLINED none: hkArray<hkVector4>::hkArray
                FNVD_INLINED_LOCAL(hkArray<hkVector4>, vertices, std::nullopt, std::nullopt);
                vertices.m_data = nullptr;
                vertices.m_size = 0;
                vertices.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
                pVertices->getOriginalVertices(vertices);
                // Transforms the vertices last to first, the first one included even when there are none (a do-while),
                // as the game does. EXE-INLINED 0xC7F640: hkVector4::setTransformedPos
                const hkQuadReal col0 = parentFromChild.m_rotation.m_col0.m_quad;
                const hkQuadReal col1 = parentFromChild.m_rotation.m_col1.m_quad;
                const hkQuadReal col2 = parentFromChild.m_rotation.m_col2.m_quad;
                const hkQuadReal translation = parentFromChild.m_translation.m_quad;
                int32_t index = vertices.m_size - 1;
                hkVector4* pVertex = &vertices.m_data[index];
                do {
                    const hkQuadReal v = pVertex->m_quad;
                    const hkQuadReal xy = _mm_add_ps(
                        _mm_mul_ps(_mm_shuffle_ps(v, v, 0x55), col1), _mm_mul_ps(_mm_shuffle_ps(v, v, 0x00), col0)
                    );
                    const hkQuadReal zt = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v, v, 0xAA), col2), translation);
                    pVertex->m_quad = _mm_add_ps(xy, zt);
                    --index;
                    --pVertex;
                }
                while (index >= 0);
                alignas(16) hkStridedVertices stridedVertices;
                // The SDK's hkStridedVertices over a vector array: its floats, read as untyped data with a 16-byte
                // step.
                stridedVertices.m_vertices = reinterpret_cast<const float*>(vertices.m_data);
                stridedVertices.m_numVertices = vertices.m_size;
                stridedVertices.m_striding = sizeof(hkVector4);
                const hkArray<hkVector4>* pPlanes = getPlaneEquations(const_cast<hkpConvexVerticesShape*>(pVertices));
                // EXE-INLINED none: hkArray<hkVector4>::hkArray
                FNVD_INLINED_LOCAL(hkArray<hkVector4>, planes, std::nullopt, std::nullopt);
                planes.m_data = nullptr;
                planes.m_size = 0;
                planes.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
                {
                    // EXE-INLINED none: hkArray<hkVector4>::setSize
                    const int32_t numPlanes = pPlanes->m_size;
                    hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
                    const int32_t capacity = planes.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK;
                    if (capacity < numPlanes) {
                        const int32_t doubled = 2 * capacity;
                        hkArrayUtil::_reserve(
                            *pHeap, &planes, numPlanes < doubled ? doubled : numPlanes, sizeof(hkVector4)
                        );
                    }
                    planes.m_size = numPlanes;
                }
                // Each plane moves with the transform: its offset by the dot product of its normal with the
                // translation (SSE scalar sums, the offset an x87 difference stored as a float), its normal turned.
                hkVector4 translationCopy;
                translationCopy.m_quad = parentFromChild.m_translation.m_quad;
                for (int32_t i = 0; i < pPlanes->m_size; ++i) {
                    hkVector4 plane;
                    plane.m_quad = pPlanes->m_data[i].m_quad;
                    const float planeW = plane.m_quad[3];
                    const hkQuadReal products = _mm_mul_ps(plane.m_quad, translationCopy.m_quad);
                    hkQuadReal dot = _mm_add_ss(_mm_shuffle_ps(products, products, 0x55), products);
                    dot = _mm_add_ss(_mm_shuffle_ps(products, products, 0xAA), dot);
                    const float distance = _mm_cvtss_f32(dot);
                    const float offset = planeW - distance;
                    plane.setRotatedDir(parentFromChild.m_rotation, plane);
                    // An x87 copy.
                    plane.m_quad[3] = offset;
                    planes.m_data[i].m_quad = plane.m_quad;
                }
                void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpConvexVerticesShape));
                static_cast<hkpConvexVerticesShape*>(pMemory)->m_memSizeAndFlags = sizeof(hkpConvexVerticesShape);
                // An x87 copy.
                const float radius = pVertices->m_radius;
                const hkpShape* pNewShape = ::new (pMemory) hkpConvexVerticesShape(stridedVertices, planes, radius);
                sharedShapeData.setReplacement(transformShape, pNewShape);
                resultsOut.m_numCollapsedShapes++;
                {
                    // EXE-INLINED none: hkArray<hkVector4>::~hkArray (its clearAndDeallocate form: the exe resets the
                    // pointer and the capacity, 0xD73FD0)
                    hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
                    planes.m_size = 0;
                    if (planes.m_capacityAndFlags >= 0) {
                        pHeap->bufFree(
                            planes.m_data,
                            static_cast<int32_t>(
                                (planes.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK) * sizeof(hkVector4)
                            )
                        );
                    }
                    planes.m_data = nullptr;
                    planes.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
                }
                {
                    // EXE-INLINED none: hkArray<hkVector4>::~hkArray
                    hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
                    vertices.m_size = 0;
                    if (vertices.m_capacityAndFlags >= 0) {
                        pHeap->bufFree(
                            vertices.m_data,
                            static_cast<int32_t>(
                                (vertices.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK) *
                                sizeof(hkVector4)
                            )
                        );
                    }
                }
                return pNewShape;
            }
            default: break;
        }
    }

    switch (childType) {
        case HK_SHAPE_SPHERE:
        case HK_SHAPE_CYLINDER:
        case HK_SHAPE_BOX:
        case HK_SHAPE_CAPSULE:
        case HK_SHAPE_CONVEX_VERTICES:
        case HK_SHAPE_CONVEX: {
            const hkpConvexShape* pConvexChild = static_cast<const hkpConvexShape*>(child);
            if (parentFromChild.m_rotation.isApproximatelyEqual(hkTransformIdentity.m_rotation, 0.001f).m_bool &&
                shapeType != HK_SHAPE_CONVEX_TRANSLATE) {
                void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpConvexTranslateShape));
                static_cast<hkpConvexTranslateShape*>(pMemory)->m_memSizeAndFlags = sizeof(hkpConvexTranslateShape);
                hkpConvexTranslateShape* pTranslate = ::new (pMemory) hkpConvexTranslateShape(
                    const_cast<hkpConvexShape*>(pConvexChild), parentFromChild.m_translation,
                    hkpShapeContainer::REFERENCE_POLICY_INCREMENT
                );
                sharedShapeData.setReplacement(transformShape, pTranslate);
                resultsOut.m_numSpecializedTransformShapes++;
                return pTranslate;
            }
            if (shapeType != HK_SHAPE_TRANSFORM) {
                break;
            }
            void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpConvexTransformShape));
            static_cast<hkpConvexTransformShape*>(pMemory)->m_memSizeAndFlags = sizeof(hkpConvexTransformShape);
            hkpConvexTransformShape* pConvexTransform = ::new (pMemory) hkpConvexTransformShape(
                const_cast<hkpConvexShape*>(pConvexChild), parentFromChild,
                hkpShapeContainer::REFERENCE_POLICY_INCREMENT
            );
            sharedShapeData.setReplacement(transformShape, pConvexTransform);
            resultsOut.m_numSpecializedTransformShapes++;
            return pConvexTransform;
        }
        default: break;
    }
    transformShape->addReference();
    return transformShape;
}

// The SDK's collapseShapesRecursively, from the asm: the recorded replacement when there is one (the lookup
// compares m_originalShape only), else by type: a list whose children all come back unchanged is kept, else a new
// list over the new children replaces it; a convex translate, convex transform or transform shape over a changed
// child is rebuilt over the new child; the result is then collapsed with collapseTransformShape.
const hkpShape* hkpTransformCollapseUtil::collapseShapesRecursively(
    const hkpShape* shape,
    const Options& options,
    _SharedShapeData& sharedShapeData,
    Results& resultsOut
) {
    {
        // EXE-INLINED none: _SharedShapeData::findReplacement
        const int32_t numShared = sharedShapeData.m_sharedShapes.m_size;
        const _SharedShapeData::SharedShape* pShared = sharedShapeData.m_sharedShapes.m_data;
        for (int32_t i = 0; i < numShared; ++i) {
            if (pShared[i].m_originalShape == shape) {
                const hkpShape* pReplacement = pShared[i].m_replacementShape;
                if (pReplacement) {
                    pReplacement->addReference();
                    return pReplacement;
                }
                break;
            }
        }
    }

    const hkpShape* pNewShape;
    switch (shape->m_type.m_storage) {
        case HK_SHAPE_LIST: {
            const hkpListShape* pList = static_cast<const hkpListShape*>(shape);
            const hkpShapeContainer* pContainer = pList;
            const int32_t numChildren = pContainer->getNumChildShapes();
            // EXE-INLINED none: hkArray<const hkpShape*>::hkArray
            FNVD_INLINED_LOCAL(hkArray<const hkpShape*>, newChildren, std::nullopt, std::nullopt);
            newChildren.m_data = nullptr;
            newChildren.m_size = 0;
            newChildren.m_capacityAndFlags = hkArrayBase<const hkpShape*>::DONT_DEALLOCATE_FLAG;
            bool allUnchanged = true;
            for (int32_t i = 0; i < numChildren; ++i) {
                const hkpShape* pChild = pList->m_childInfo.m_data[i].m_shape;
                const hkpShape* pNewChild = collapseShapesRecursively(pChild, options, sharedShapeData, resultsOut);
                // EXE-INLINED none: hkArray<const hkpShape*>::pushBack
                hkMemoryRouter& rRouter = hkMemoryRouter::getInstance();
                if (newChildren.m_size ==
                    (newChildren.m_capacityAndFlags & hkArrayBase<const hkpShape*>::CAPACITY_MASK)) {
                    hkArrayUtil::_reserveMore(*rRouter.m_heap, &newChildren, sizeof(const hkpShape*));
                }
                newChildren.m_data[newChildren.m_size] = pNewChild;
                newChildren.m_size++;
                if (pNewChild != pChild) {
                    allUnchanged = false;
                }
            }
            const hkpShape* pResult;
            if (allUnchanged) {
                for (int32_t i = 0; i < numChildren; ++i) {
                    newChildren.m_data[i]->removeReference();
                }
                shape->addReference();
                pResult = shape;
            }
            else {
                void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpListShape));
                static_cast<hkpListShape*>(pMemory)->m_memSizeAndFlags = sizeof(hkpListShape);
                hkpListShape* pNewList = ::new (pMemory)
                    hkpListShape(newChildren.m_data, numChildren, hkpShapeContainer::REFERENCE_POLICY_INCREMENT);
                for (int32_t i = 0; i < numChildren; ++i) {
                    newChildren.m_data[i]->removeReference();
                }
                sharedShapeData.setReplacement(shape, pNewList);
                pResult = pNewList;
            }
            // EXE-INLINED none: hkArray<const hkpShape*>::~hkArray
            hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
            newChildren.m_size = 0;
            if (newChildren.m_capacityAndFlags >= 0) {
                pHeap->bufFree(
                    newChildren.m_data,
                    static_cast<int32_t>(
                        (newChildren.m_capacityAndFlags & hkArrayBase<const hkpShape*>::CAPACITY_MASK) *
                        sizeof(const hkpShape*)
                    )
                );
            }
            return pResult;
        }
        case HK_SHAPE_CONVEX_TRANSLATE: {
            const hkpConvexTranslateShape* pTranslate = static_cast<const hkpConvexTranslateShape*>(shape);
            const hkpShape* pChild = pTranslate->m_childShape.m_childShape;
            const hkpShape* pNewChild = collapseShapesRecursively(pChild, options, sharedShapeData, resultsOut);
            if (pChild == pNewChild) {
                pNewChild->removeReference();
                pNewShape = shape;
                shape->addReference();
            }
            else {
                void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpConvexTranslateShape));
                static_cast<hkpConvexTranslateShape*>(pMemory)->m_memSizeAndFlags = sizeof(hkpConvexTranslateShape);
                pNewShape = ::new (pMemory) hkpConvexTranslateShape(
                    const_cast<hkpConvexShape*>(static_cast<const hkpConvexShape*>(pNewChild)),
                    pTranslate->m_translation, hkpShapeContainer::REFERENCE_POLICY_INCREMENT
                );
                pNewChild->removeReference();
                sharedShapeData.setReplacement(shape, pNewShape);
            }
            break;
        }
        case HK_SHAPE_CONVEX_TRANSFORM: {
            const hkpConvexTransformShape* pTransform = static_cast<const hkpConvexTransformShape*>(shape);
            const hkpShape* pChild = pTransform->m_childShape.m_childShape;
            const hkpShape* pNewChild = collapseShapesRecursively(pChild, options, sharedShapeData, resultsOut);
            if (pChild == pNewChild) {
                pNewChild->removeReference();
                pNewShape = shape;
                shape->addReference();
            }
            else {
                void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpConvexTransformShape));
                static_cast<hkpConvexTransformShape*>(pMemory)->m_memSizeAndFlags = sizeof(hkpConvexTransformShape);
                pNewShape = ::new (pMemory) hkpConvexTransformShape(
                    const_cast<hkpConvexShape*>(static_cast<const hkpConvexShape*>(pNewChild)), pTransform->m_transform,
                    hkpShapeContainer::REFERENCE_POLICY_INCREMENT
                );
                pNewChild->removeReference();
                sharedShapeData.setReplacement(shape, pNewShape);
            }
            break;
        }
        case HK_SHAPE_TRANSFORM: {
            const hkpTransformShape* pTransform = static_cast<const hkpTransformShape*>(shape);
            const hkpShape* pChild = pTransform->m_childShape.m_childShape;
            const hkpShape* pNewChild = collapseShapesRecursively(pChild, options, sharedShapeData, resultsOut);
            if (pChild == pNewChild) {
                pNewChild->removeReference();
                pNewShape = shape;
                shape->addReference();
            }
            else {
                void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpTransformShape));
                static_cast<hkpTransformShape*>(pMemory)->m_memSizeAndFlags = sizeof(hkpTransformShape);
                pNewShape = ::new (pMemory) hkpTransformShape(pNewChild, pTransform->m_transform);
                pNewChild->removeReference();
                sharedShapeData.setReplacement(shape, pNewShape);
            }
            break;
        }
        default: shape->addReference(); return shape;
    }
    const hkpShape* pResult = collapseTransformShape(pNewShape, options, sharedShapeData, resultsOut);
    pNewShape->removeReference();
    return pResult;
}

// collapseTransforms of one shape: the shape's shared-shape record is built and pruned on the stack, the shape
// collapsed through it into *shapeOut, then the record destroyed.
hkResult hkpTransformCollapseUtil::collapseTransforms(
    const hkpShape* shape,
    const hkpTransformCollapseUtil::Options& options,
    hkpTransformCollapseUtil::Results& resultsOut,
    hkpShape** shapeOut
) {
    FNVD_INLINED_LOCAL(_SharedShapeData, sharedShapes, std::nullopt, std::nullopt);
    // EXE-INLINED none: _SharedShapeData::_SharedShapeData (an empty record)
    sharedShapes.m_sharedShapes.m_data = nullptr;
    sharedShapes.m_sharedShapes.m_size = 0;
    sharedShapes.m_sharedShapes.m_capacityAndFlags = hkArrayBase<_SharedShapeData::SharedShape>::DONT_DEALLOCATE_FLAG;
    sharedShapes.recursivelyParseShapeReferences(shape);
    sharedShapes.removeSingleReferencedShapeData();
    *shapeOut = const_cast<hkpShape*>(collapseTransformShape(shape, options, sharedShapes, resultsOut));
    sharedShapes.~_SharedShapeData();
    return HK_SUCCESS;
}

hkResult
hkpTransformCollapseUtil::collapseTransforms(hkpRigidBody* rigidBody, const Options& options, Results& resultsOut) {
    // EXE-INLINED none: hkArray<hkpRigidBody*>::hkArray
    FNVD_INLINED_LOCAL(hkArray<hkpRigidBody*>, rigidBodies, std::nullopt, std::nullopt);
    rigidBodies.m_data = nullptr;
    rigidBodies.m_size = 0;
    rigidBodies.m_capacityAndFlags = hkArrayBase<hkpRigidBody*>::DONT_DEALLOCATE_FLAG;
    {
        // EXE-INLINED none: hkArray<hkpRigidBody*>::pushBack
        hkMemoryRouter& rRouter = hkMemoryRouter::getInstance();
        if (rigidBodies.m_size == (rigidBodies.m_capacityAndFlags & hkArrayBase<hkpRigidBody*>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(*rRouter.m_heap, &rigidBodies, sizeof(hkpRigidBody*));
        }
        rigidBodies.m_data[rigidBodies.m_size] = rigidBody;
        rigidBodies.m_size++;
    }
    const hkResult result = collapseTransforms(rigidBodies, options, resultsOut);
    // EXE-INLINED none: hkArray<hkpRigidBody*>::~hkArray
    hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
    rigidBodies.m_size = 0;
    if (rigidBodies.m_capacityAndFlags >= 0) {
        pHeap->bufFree(
            rigidBodies.m_data,
            static_cast<int32_t>(
                (rigidBodies.m_capacityAndFlags & hkArrayBase<hkpRigidBody*>::CAPACITY_MASK) * sizeof(hkpRigidBody*)
            )
        );
    }
    return result;
}

hkResult hkpTransformCollapseUtil::collapseTransforms(
    hkArray<hkpRigidBody*, hkContainerHeapAllocator>& rigidBodies,
    const Options& options,
    Results& resultsOut
) {
    resultsOut.reset();
    _SharedShapeData sharedShapeData(rigidBodies);
    for (int32_t i = 0; i < rigidBodies.m_size; ++i) {
        hkpRigidBody* pBody = rigidBodies.m_data[i];
        const hkpShape* pShape = pBody->m_collidable.m_shape;
        const hkpShape* pNewShape = collapseShapesRecursively(pShape, options, sharedShapeData, resultsOut);
        pShape->removeReference();
        pBody->m_collidable.m_shape = pNewShape;
    }
    return HK_SUCCESS;
}

// Returns the shape itself when the transform is the identity; a convex translate shape under a pure translation, a
// convex transform shape or a transform shape whose combined transform is the identity gives its child; else a convex
// translate shape (over the child, deliberately with the shape's own translation, as the game does), a convex transform
// shape or a transform shape over the child with the combined transform; any other shape, or a convex translate shape
// under a rotation, is wrapped in a new transform shape.
const hkpShape* hkpTransformCollapseUtil::transformTransformShape(const hkpShape* shape, const hkTransform& transform) {
    if (transform.isApproximatelyEqual(hkTransformIdentity, 0.001f)) {
        shape->addReference();
        return shape;
    }
    switch (shape->m_type.m_storage) {
        case HK_SHAPE_CONVEX_TRANSLATE: {
            if (!transform.m_rotation.isApproximatelyEqual(hkTransformIdentity.m_rotation, 0.001f)) {
                break;
            }
            const hkpConvexTranslateShape* pTranslate = static_cast<const hkpConvexTranslateShape*>(shape);
            // EXE-INLINED 0x577070: hkTransform::setIdentity
            hkTransform childFromGrandchild;
            childFromGrandchild.m_rotation.m_col0.m_quad = _mm_setr_ps(1.0f, 0.0f, 0.0f, 0.0f);
            childFromGrandchild.m_rotation.m_col1.m_quad = _mm_setr_ps(0.0f, 1.0f, 0.0f, 0.0f);
            childFromGrandchild.m_rotation.m_col2.m_quad = _mm_setr_ps(0.0f, 0.0f, 1.0f, 0.0f);
            childFromGrandchild.m_translation.m_quad = pTranslate->m_translation.m_quad;
            hkTransform parentFromGrandchild;
            parentFromGrandchild.setMul(transform, childFromGrandchild);
            if (parentFromGrandchild.isApproximatelyEqual(hkTransformIdentity, 0.001f)) {
                const hkpShape* pChild = pTranslate->m_childShape.m_childShape;
                pChild->addReference();
                return pChild;
            }
            void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpConvexTranslateShape));
            static_cast<hkpConvexTranslateShape*>(pMemory)->m_memSizeAndFlags = sizeof(hkpConvexTranslateShape);
            return ::new (pMemory) hkpConvexTranslateShape(
                const_cast<hkpConvexShape*>(static_cast<const hkpConvexShape*>(pTranslate->m_childShape.m_childShape)),
                pTranslate->m_translation, hkpShapeContainer::REFERENCE_POLICY_INCREMENT
            );
        }
        case HK_SHAPE_CONVEX_TRANSFORM: {
            const hkpConvexTransformShape* pTransform = static_cast<const hkpConvexTransformShape*>(shape);
            hkTransform parentFromGrandchild;
            parentFromGrandchild.setMul(transform, pTransform->m_transform);
            if (parentFromGrandchild.isApproximatelyEqual(hkTransformIdentity, 0.001f)) {
                const hkpShape* pChild = pTransform->m_childShape.m_childShape;
                pChild->addReference();
                return pChild;
            }
            void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpConvexTransformShape));
            static_cast<hkpConvexTransformShape*>(pMemory)->m_memSizeAndFlags = sizeof(hkpConvexTransformShape);
            return ::new (pMemory) hkpConvexTransformShape(
                const_cast<hkpConvexShape*>(static_cast<const hkpConvexShape*>(pTransform->m_childShape.m_childShape)),
                parentFromGrandchild, hkpShapeContainer::REFERENCE_POLICY_INCREMENT
            );
        }
        case HK_SHAPE_TRANSFORM: {
            const hkpTransformShape* pTransform = static_cast<const hkpTransformShape*>(shape);
            hkTransform parentFromGrandchild;
            parentFromGrandchild.setMul(transform, pTransform->m_transform);
            if (parentFromGrandchild.isApproximatelyEqual(hkTransformIdentity, 0.001f)) {
                const hkpShape* pChild = pTransform->m_childShape.m_childShape;
                pChild->addReference();
                return pChild;
            }
            void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpTransformShape));
            static_cast<hkpTransformShape*>(pMemory)->m_memSizeAndFlags = sizeof(hkpTransformShape);
            return ::new (pMemory) hkpTransformShape(pTransform->m_childShape.m_childShape, parentFromGrandchild);
        }
        default: break;
    }
    void* pMemory = hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpTransformShape));
    static_cast<hkpTransformShape*>(pMemory)->m_memSizeAndFlags = sizeof(hkpTransformShape);
    return ::new (pMemory) hkpTransformShape(shape, transform);
}

// Each rigid body's shape is recorded, then the shapes referenced once are dropped.
_SharedShapeData::_SharedShapeData(const hkArray<hkpRigidBody*, hkContainerHeapAllocator>& a1) {
    for (int32_t i = 0; i < a1.m_size; ++i) {
        recursivelyParseShapeReferences(a1.m_data[i]->m_collidable.m_shape);
    }
    removeSingleReferencedShapeData();
}

// Every record's two shapes lose their reference; the records' storage goes with m_sharedShapes.
_SharedShapeData::~_SharedShapeData() {
    for (int32_t i = 0; i < m_sharedShapes.m_size; i++) {
        if (m_sharedShapes.m_data[i].m_originalShape) {
            m_sharedShapes.m_data[i].m_originalShape->removeReference();
        }
        if (m_sharedShapes.m_data[i].m_replacementShape) {
            m_sharedShapes.m_data[i].m_replacementShape->removeReference();
        }
    }
}

void _SharedShapeData::recursivelyParseShapeReferences(const hkpShape* shape) {
    _SharedShapeData::SharedShape* m_data;
    _SharedShapeData::SharedShape* v5;
    hkArrayBase<const hkpShape*> r_esi;
    int32_t v3 = 0;
    if (m_sharedShapes.m_size <= 0) {
        goto LABEL_5;
    }
    m_data = m_sharedShapes.begin();
    v5 = m_sharedShapes.begin();
    while (v5->m_originalShape != shape) {
        ++v3;
        ++v5;
        if (v3 >= m_sharedShapes.m_size) {
            goto LABEL_5;
        }
    }
    if (v3 < 0) {
    LABEL_5:;
        hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
        if (m_sharedShapes.m_size ==
            (m_sharedShapes.m_capacityAndFlags & hkArrayBase<_SharedShapeData::SharedShape>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(*Instance->m_heap, this, sizeof(_SharedShapeData::SharedShape));
        }
        _SharedShapeData::SharedShape* v7 = &m_sharedShapes.m_data[m_sharedShapes.m_size];
        if (v7) {
            v7->m_originalShape = shape;
            v7->m_replacementShape = nullptr;
            v7->m_numReferences = 1;
        }
        ++m_sharedShapes.m_size;
        shape->addReference();
        r_esi.m_data = nullptr;
        r_esi.m_size = 0;
        r_esi.m_capacityAndFlags = hkArrayBase<const hkpShape*>::DONT_DEALLOCATE_FLAG;
        collectChildShapes(shape, &r_esi);
        for (int32_t i = 0; i < r_esi.m_size; ++i) {
            recursivelyParseShapeReferences(r_esi.m_data[i]);
        }
        hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
        r_esi.m_size = 0;
        if (r_esi.m_capacityAndFlags >= 0) {
            m_heap->bufFree(r_esi.begin(), sizeof(hkpShape*) * r_esi.m_capacityAndFlags);
        }
    }
    else {
        ++m_data[v3].m_numReferences;
    }
}

int32_t _SharedShapeData::numberOfReferences(const hkpShape* shape) const {
    const int32_t iSize = m_sharedShapes.m_size;
    for (int32_t i = 0; i < iSize; ++i) {
        const SharedShape& kShape = m_sharedShapes.m_data[i];
        if (kShape.m_originalShape == shape || kShape.m_replacementShape == shape) {
            return kShape.m_numReferences;
        }
    }
    return 0;
}

hkResult _SharedShapeData::setReplacement(const hkpShape* shape, const hkpShape* replacement) {
    int32_t m_size = m_sharedShapes.m_size;
    int32_t v5 = 0;
    if (m_size <= 0) {
        return HK_FAILURE;
    }
    for (_SharedShapeData::SharedShape* i = m_sharedShapes.begin();
         i->m_originalShape != shape && i->m_replacementShape != shape; ++i) {
        if (++v5 >= m_size) {
            return HK_FAILURE;
        }
    }
    replacement->addReference();
    const hkpShape* m_replacementShape = m_sharedShapes.m_data[v5].m_replacementShape;
    if (m_replacementShape) {
        m_replacementShape->removeReference();
    }
    m_sharedShapes.m_data[v5].m_replacementShape = replacement;
    return HK_SUCCESS;
}

// The SDK's removeSingleReferencedShapeData: from the last record down, each record of a shape referenced fewer than
// twice gives up its shapes' references and is removed, the last record moved into its slot.
void _SharedShapeData::removeSingleReferencedShapeData() {
    for (int32_t i = m_sharedShapes.m_size - 1; i >= 0; --i) {
        if (m_sharedShapes.m_data[i].m_numReferences < 2) {
            const hkpShape* pOriginal = m_sharedShapes.m_data[i].m_originalShape;
            if (pOriginal) {
                pOriginal->removeReference();
            }
            const hkpShape* pReplacement = m_sharedShapes.m_data[i].m_replacementShape;
            if (pReplacement) {
                pReplacement->removeReference();
            }
            // EXE-INLINED none: hkArray<_SharedShapeData::SharedShape>::removeAt
            const int32_t iLast = (--m_sharedShapes.m_size);
            if (iLast != i) {
                m_sharedShapes.m_data[i] = m_sharedShapes.m_data[iLast];
            }
        }
    }
}
