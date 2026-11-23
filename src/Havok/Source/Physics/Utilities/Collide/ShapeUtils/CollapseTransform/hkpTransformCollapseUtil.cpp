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
#include "xfnv/Compiler.h"

#include <cmath>

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
                apChildren->_pushBack(hkContainerHeapAllocator().get(apChildren), pChild);
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
    apChildren->_pushBack(hkContainerHeapAllocator().get(apChildren), pChild);
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
    char buf[512];
    {
        hkErrStream ostr(buf, 512);
        int32_t m_numCollapsedShapes = this->m_numCollapsedShapes;
        ostr << "Number of collapsed shapes :" << m_numCollapsedShapes;
        hkError::s_instance->message(
            hkError::MESSAGE_REPORT,
            -1,
            buf,
            ".\\Collide\\ShapeUtils\\CollapseTransform\\hkpTransformCollapseUtil.cpp",
            57
        );
    }
    {
        hkErrStream ostr(buf, 512);
        int32_t m_numIdentityTransformsRemoved = this->m_numIdentityTransformsRemoved;
        ostr << "Number of identity transforms removed : " << m_numIdentityTransformsRemoved;
        hkError::s_instance->message(
            hkError::MESSAGE_REPORT,
            -1,
            buf,
            ".\\Collide\\ShapeUtils\\CollapseTransform\\hkpTransformCollapseUtil.cpp",
            58
        );
    }
    {
        hkErrStream ostr(buf, 512);
        int32_t m_numSpecializedTransformShapes = this->m_numSpecializedTransformShapes;
        ostr << "Number of specialized transform shapes : " << m_numSpecializedTransformShapes;
        hkError::s_instance->message(
            hkError::MESSAGE_REPORT,
            -1,
            buf,
            ".\\Collide\\ShapeUtils\\CollapseTransform\\hkpTransformCollapseUtil.cpp",
            59
        );
    }
    {
        hkErrStream ostr(buf, 512);
        int32_t m_numPropagatedTransformsToLists = this->m_numPropagatedTransformsToLists;
        ostr << "Number of propagated transforms to lists : " << m_numPropagatedTransformsToLists;
        hkError::s_instance->message(
            hkError::MESSAGE_REPORT,
            -1,
            buf,
            ".\\Collide\\ShapeUtils\\CollapseTransform\\hkpTransformCollapseUtil.cpp",
            60
        );
    }
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
            parentFromChild.setIdentity();
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
        hkArray<const hkpShape*> newChildren;
        for (int32_t i = 0; i < pContainer->getNumChildShapes(); ++i) {
            const hkpShape* pGrandchild = pList->m_childInfo.m_data[i].m_shape;
            hkpTransformShape* pTransformed = new hkpTransformShape(pGrandchild, parentFromChild);
            const hkpShape* pCollapsed = collapseTransformShape(pTransformed, options, sharedShapeData, resultsOut);
            newChildren.pushBack(pCollapsed);
            pTransformed->removeReference();
        }
        hkpListShape* pNewList = new hkpListShape(
            newChildren.m_data,
            newChildren.m_size,
            hkpShapeContainer::REFERENCE_POLICY_INCREMENT
        );
        for (const hkpShape* pChild : newChildren) {
            pChild->removeReference();
        }
        sharedShapeData.setReplacement(transformShape, pNewList);
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
                const hkpShape* pNewShape = new hkpCylinderShape(vertexA, vertexB, cylinderRadius, radius);
                sharedShapeData.setReplacement(transformShape, pNewShape);
                resultsOut.m_numCollapsedShapes++;
                return pNewShape;
            }
            case HK_SHAPE_BOX: {
                const hkRotation& rotation = parentFromChild.m_rotation;
                if (!rotation.isOrthonormal(0.001f).m_bool ||
                    !rotation.isOk().m_bool ||
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
                        const float absolute = std::fabsf(XFNV_M128_F32(rColumn.m_quad, row));
                        const bool isZero = absolute < 9.999999747378752e-05;
                        const float offOne = absolute - 1.0f;
                        const float absoluteOffOne = std::fabsf(offOne);
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
                    const float absY = std::fabsf(XFNV_M128_F32(rColumn.m_quad, 1));
                    const float absX = std::fabsf(XFNV_M128_F32(rColumn.m_quad, 0));
                    const float sumX = absX + 0.0f;
                    const float sumXY = absY + sumX;
                    const float absZ = std::fabsf(XFNV_M128_F32(rColumn.m_quad, 2));
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
                    XFNV_M128_F32(pBox->m_halfExtents.m_quad, axisOfRow[0]),
                    XFNV_M128_F32(pBox->m_halfExtents.m_quad, axisOfRow[1]),
                    XFNV_M128_F32(pBox->m_halfExtents.m_quad, axisOfRow[2]),
                    0.0f
                );
                hkpBoxShape* pNewBox = new hkpBoxShape(halfExtents, hkConvexShapeDefaultRadius);
                const hkQuadReal squared = _mm_mul_ps(
                    parentFromChild.m_translation.m_quad,
                    parentFromChild.m_translation.m_quad
                );
                hkQuadReal sum = _mm_add_ss(_mm_shuffle_ps(squared, squared, 0x55), squared);
                sum = _mm_add_ss(_mm_shuffle_ps(squared, squared, 0xAA), sum);
                const float length = _mm_cvtss_f32(_mm_sqrt_ss(sum));
                if (length < 9.999999747378752e-05) {
                    sharedShapeData.setReplacement(transformShape, pNewBox);
                    resultsOut.m_numCollapsedShapes++;
                    return pNewBox;
                }
                hkpConvexTranslateShape* pTranslate = new hkpConvexTranslateShape(
                    pNewBox,
                    parentFromChild.m_translation,
                    hkpShapeContainer::REFERENCE_POLICY_INCREMENT
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
                const hkpShape* pNewShape = new hkpCapsuleShape(vertexA, vertexB, radius);
                sharedShapeData.setReplacement(transformShape, pNewShape);
                resultsOut.m_numCollapsedShapes++;
                return pNewShape;
            }
            case HK_SHAPE_CONVEX_VERTICES: {
                const hkpConvexVerticesShape* pVertices = static_cast<const hkpConvexVerticesShape*>(child);
                hkArray<hkVector4> vertices;
                pVertices->getOriginalVertices(vertices);
                // Transforms the vertices last to first, the first one included even when there are none (a do-while).
                int32_t index = vertices.m_size - 1;
                hkVector4* pVertex = &vertices.m_data[index];
                do {
                    pVertex->setTransformedPos(parentFromChild, *pVertex);
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
                hkArray<hkVector4> planes;
                planes.setSize(pPlanes->m_size);
                // Each plane moves with the transform: its offset by the dot product of its normal with the
                // translation (SSE scalar sums, the offset an x87 difference stored as a float), its normal turned.
                hkVector4 translationCopy;
                translationCopy.m_quad = parentFromChild.m_translation.m_quad;
                for (int32_t i = 0; i < pPlanes->m_size; ++i) {
                    hkVector4 plane;
                    plane.m_quad = pPlanes->m_data[i].m_quad;
                    const float planeW = XFNV_M128_F32(plane.m_quad, 3);
                    const hkQuadReal products = _mm_mul_ps(plane.m_quad, translationCopy.m_quad);
                    hkQuadReal dot = _mm_add_ss(_mm_shuffle_ps(products, products, 0x55), products);
                    dot = _mm_add_ss(_mm_shuffle_ps(products, products, 0xAA), dot);
                    const float distance = _mm_cvtss_f32(dot);
                    const float offset = planeW - distance;
                    plane.setRotatedDir(parentFromChild.m_rotation, plane);
                    // An x87 copy.
                    XFNV_M128_F32(plane.m_quad, 3) = offset;
                    planes.m_data[i].m_quad = plane.m_quad;
                }
                const hkpShape* pNewShape = new hkpConvexVerticesShape(stridedVertices, planes, pVertices->m_radius);
                sharedShapeData.setReplacement(transformShape, pNewShape);
                resultsOut.m_numCollapsedShapes++;
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
                hkpConvexTranslateShape* pTranslate = new hkpConvexTranslateShape(
                    const_cast<hkpConvexShape*>(pConvexChild),
                    parentFromChild.m_translation,
                    hkpShapeContainer::REFERENCE_POLICY_INCREMENT
                );
                sharedShapeData.setReplacement(transformShape, pTranslate);
                resultsOut.m_numSpecializedTransformShapes++;
                return pTranslate;
            }
            if (shapeType != HK_SHAPE_TRANSFORM) {
                break;
            }
            hkpConvexTransformShape* pConvexTransform = new hkpConvexTransformShape(
                const_cast<hkpConvexShape*>(pConvexChild),
                parentFromChild,
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
    if (const hkpShape* pReplacement = sharedShapeData.findReplacement(shape)) {
        pReplacement->addReference();
        return pReplacement;
    }

    const hkpShape* pNewShape;
    switch (shape->m_type.m_storage) {
        case HK_SHAPE_LIST: {
            const hkpListShape* pList = static_cast<const hkpListShape*>(shape);
            const hkpShapeContainer* pContainer = pList;
            const int32_t numChildren = pContainer->getNumChildShapes();
            hkArray<const hkpShape*> newChildren;
            bool allUnchanged = true;
            for (int32_t i = 0; i < numChildren; ++i) {
                const hkpShape* pChild = pList->m_childInfo.m_data[i].m_shape;
                const hkpShape* pNewChild = collapseShapesRecursively(pChild, options, sharedShapeData, resultsOut);
                newChildren.pushBack(pNewChild);
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
                hkpListShape* pNewList = new hkpListShape(
                    newChildren.m_data,
                    numChildren,
                    hkpShapeContainer::REFERENCE_POLICY_INCREMENT
                );
                for (int32_t i = 0; i < numChildren; ++i) {
                    newChildren.m_data[i]->removeReference();
                }
                sharedShapeData.setReplacement(shape, pNewList);
                pResult = pNewList;
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
                pNewShape = new hkpConvexTranslateShape(
                    const_cast<hkpConvexShape*>(static_cast<const hkpConvexShape*>(pNewChild)),
                    pTranslate->m_translation,
                    hkpShapeContainer::REFERENCE_POLICY_INCREMENT
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
                pNewShape = new hkpConvexTransformShape(
                    const_cast<hkpConvexShape*>(static_cast<const hkpConvexShape*>(pNewChild)),
                    pTransform->m_transform,
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
                pNewShape = new hkpTransformShape(pNewChild, pTransform->m_transform);
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
    _SharedShapeData sharedShapes(shape);
    *shapeOut = const_cast<hkpShape*>(collapseTransformShape(shape, options, sharedShapes, resultsOut));
    return HK_SUCCESS;
}

hkResult
hkpTransformCollapseUtil::collapseTransforms(hkpRigidBody* rigidBody, const Options& options, Results& resultsOut) {
    hkArray<hkpRigidBody*> rigidBodies;
    rigidBodies.pushBack(rigidBody);
    const hkResult result = collapseTransforms(rigidBodies, options, resultsOut);
    return result;
}

hkResult hkpTransformCollapseUtil::collapseTransforms(
    hkArray<hkpRigidBody*, hkContainerHeapAllocator>& rigidBodies,
    const Options& options,
    Results& resultsOut
) {
    resultsOut.reset();
    _SharedShapeData sharedShapeData(rigidBodies);
    for (hkpRigidBody* pBody : rigidBodies) {
        const hkpShape* pShape = pBody->m_collidable.m_shape;
        const hkpShape* pNewShape = collapseShapesRecursively(pShape, options, sharedShapeData, resultsOut);
        pShape->removeReference();
        pBody->m_collidable.m_shape = pNewShape;
    }
    return HK_SUCCESS;
}

// Returns the shape itself when the transform is the identity; a convex translate shape under a pure translation, a
// convex transform shape or a transform shape whose combined transform is the identity gives its child; else a convex
// translate shape (over the child, deliberately with the shape's own translation), a convex transform
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
            hkTransform childFromGrandchild;
            childFromGrandchild.setIdentity();
            childFromGrandchild.m_translation.m_quad = pTranslate->m_translation.m_quad;
            hkTransform parentFromGrandchild;
            parentFromGrandchild.setMul(transform, childFromGrandchild);
            if (parentFromGrandchild.isApproximatelyEqual(hkTransformIdentity, 0.001f)) {
                const hkpShape* pChild = pTranslate->m_childShape.m_childShape;
                pChild->addReference();
                return pChild;
            }
            return new hkpConvexTranslateShape(
                const_cast<hkpConvexShape*>(static_cast<const hkpConvexShape*>(pTranslate->m_childShape.m_childShape)),
                pTranslate->m_translation,
                hkpShapeContainer::REFERENCE_POLICY_INCREMENT
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
            return new hkpConvexTransformShape(
                const_cast<hkpConvexShape*>(static_cast<const hkpConvexShape*>(pTransform->m_childShape.m_childShape)),
                parentFromGrandchild,
                hkpShapeContainer::REFERENCE_POLICY_INCREMENT
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
            return new hkpTransformShape(pTransform->m_childShape.m_childShape, parentFromGrandchild);
        }
        default: break;
    }
    return new hkpTransformShape(shape, transform);
}

// Each rigid body's shape is recorded, then the shapes referenced once are dropped.
_SharedShapeData::_SharedShapeData(const hkArray<hkpRigidBody*, hkContainerHeapAllocator>& rigidBodies) {
    for (hkpRigidBody* pBody : rigidBodies) {
        recursivelyParseShapeReferences(pBody->m_collidable.m_shape);
    }
    removeSingleReferencedShapeData();
}

_SharedShapeData::_SharedShapeData(const hkpShape* shape) {
    recursivelyParseShapeReferences(shape);
    removeSingleReferencedShapeData();
}

// Every record's two shapes lose their reference; the records' storage goes with m_sharedShapes.
_SharedShapeData::~_SharedShapeData() {
    for (const SharedShape& kShape : m_sharedShapes) {
        if (kShape.m_originalShape) {
            kShape.m_originalShape->removeReference();
        }
        if (kShape.m_replacementShape) {
            kShape.m_replacementShape->removeReference();
        }
    }
}

void _SharedShapeData::recursivelyParseShapeReferences(const hkpShape* shape) {
    hkArrayBase<const hkpShape*> childShapes;
    int32_t v3 = 0;
    if (m_sharedShapes.m_size > 0) {
        _SharedShapeData::SharedShape* m_data = m_sharedShapes.begin();
        _SharedShapeData::SharedShape* v5 = m_sharedShapes.begin();
        do {
            if (v5->m_originalShape == shape) {
                ++m_data[v3].m_numReferences;
                return;
            }
            ++v3;
            ++v5;
        }
        while (v3 < m_sharedShapes.m_size);
    }
    _SharedShapeData::SharedShape& v7 = m_sharedShapes.expandOne();
    v7.m_originalShape = shape;
    v7.m_replacementShape = nullptr;
    v7.m_numReferences = 1;
    shape->addReference();
    childShapes.m_data = nullptr;
    childShapes.m_size = 0;
    childShapes.m_capacityAndFlags = hkArrayBase<const hkpShape*>::DONT_DEALLOCATE_FLAG;
    collectChildShapes(shape, &childShapes);
    for (const hkpShape* pChild : childShapes) {
        recursivelyParseShapeReferences(pChild);
    }
    hkMemoryAllocator* m_heap = hkMemoryRouter::getInstance().m_heap;
    childShapes.m_size = 0;
    if (childShapes.m_capacityAndFlags >= 0) {
        m_heap->bufFree(childShapes.begin(), sizeof(hkpShape*) * childShapes.m_capacityAndFlags);
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
         i->m_originalShape != shape && i->m_replacementShape != shape;
         ++i) {
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
            m_sharedShapes.removeAt(i);
        }
    }
}
