#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"

#include <stdint.h>

class _SharedShapeData;
class hkpRigidBody;
class hkpShape;
class hkTransform;

// Havok's transform-collapsing utility: static functions only, no data member. Only part of the class is declared here.
class hkpTransformCollapseUtil {
public:
    // What a collapse does with a shape more than one parent references.
    enum SharedShapeBehaviour : int32_t {
        ALWAYS_COLLAPSE = 0,
        NEVER_COLLAPSE = 1,
        COLLAPSE_IF_LESS_THAN_THRESHOLD = 2,
    };

    // The collapse's options.
    struct Options {
        Options();

        SharedShapeBehaviour m_sharedShapeBehaviour;
        int32_t m_sharedShapeThreshold;
        hkBool m_propageTransformInList;
    };

    // The utility's counters of what a collapse did.
    struct Results {
        Results();

        // Zeroes the four counters.
        void reset();
        // Reports each counter through hkError as a message of hkpTransformCollapseUtil.cpp, lines 57 to 60.
        void report() const;

        int32_t m_numCollapsedShapes;
        int32_t m_numIdentityTransformsRemoved;
        int32_t m_numSpecializedTransformShapes;
        int32_t m_numPropagatedTransformsToLists;
    };

    // Collapses the transform shape transformShape into its child (box, capsule, cylinder, convex vertices, list, ...),
    // when Options' shared-shape rule allows it for the child's references. Called by collapseShapesRecursively and
    // collapseTransforms.
    static const hkpShape* collapseTransformShape(
        const hkpShape* transformShape,
        const Options& options,
        _SharedShapeData& sharedShapeData,
        Results& resultsOut
    );

    // Returns the shape's replacement recorded in sharedShapeData, else collapses its children and the shape. Its
    // caller is collapseTransforms.
    static const hkpShape* collapseShapesRecursively(
        const hkpShape* shape,
        const Options& options,
        _SharedShapeData& sharedShapeData,
        Results& resultsOut
    );
    // Havok SDK signature (hkpUtilities.lib, tied by bytes).
    static hkResult collapseTransforms(
        const hkpShape* shape,
        const hkpTransformCollapseUtil::Options& options,
        hkpTransformCollapseUtil::Results& resultsOut,
        hkpShape** shapeOut
    );
    // Calls the array overload on a one-body array.
    static hkResult collapseTransforms(hkpRigidBody* rigidBody, const Options& options, Results& resultsOut);
    // Resets resultsOut, records the bodies' shared shapes and replaces each body's shape by its collapsed one.
    static hkResult collapseTransforms(
        hkArray<hkpRigidBody*, hkContainerHeapAllocator>& rigidBodies,
        const Options& options,
        Results& resultsOut
    );
    // Returns shape placed by transform, folding transform into a convex translate, convex transform or transform shape
    // where it can; the result holds a reference.
    static const hkpShape* transformTransformShape(const hkpShape* shape, const hkTransform& transform);
};

// The collapse's record of shapes and their references and replacements: one SharedShape per shape reached.
// collapseShapesRecursively searches m_sharedShapes by m_originalShape.
class _SharedShapeData {
public:
    struct SharedShape {
        const hkpShape* m_originalShape;
        const hkpShape* m_replacementShape;
        int32_t m_numReferences;
    };

    // Adds each rigid body's collidable shape (recursivelyParseShapeReferences) to the empty
    // record, then prunes it (removeSingleReferencedShapeData).
    _SharedShapeData(const hkArray<hkpRigidBody*, hkContainerHeapAllocator>& a1);
    ~_SharedShapeData();

    // Counts one more reference of shape when it is recorded (by m_originalShape), else records it ({shape, null, 1},
    // addReference) and recurses into its children. Public here because its callers are not members.
    void recursivelyParseShapeReferences(const hkpShape* shape);
    // Returns the m_numReferences of the first record whose original or replacement shape is shape, 0 when none is.
    int32_t numberOfReferences(const hkpShape* shape) const;
    // In the first record whose original or replacement shape is originalShape, replacementShape (addReference)
    // replaces m_replacementShape (removeReference); returns HK_FAILURE when no record matches.
    hkResult setReplacement(const hkpShape* shape, const hkpShape* replacement);
    // Walks the records backwards and drops (removeReference on both shapes) those referenced
    // fewer than 2 times, compacting the rest down.
    // Havok SDK signature (hkpUtilities.lib, tied by bytes,reloc).
    void removeSingleReferencedShapeData();

    hkArray<SharedShape, hkContainerHeapAllocator> m_sharedShapes;
};
