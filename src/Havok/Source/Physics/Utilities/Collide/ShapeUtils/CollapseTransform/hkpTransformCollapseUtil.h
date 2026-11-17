#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class _SharedShapeData;
class hkTransform;
class hkpRigidBody;
class hkpShape;

// Havok's transform-collapsing utility: static functions only (the PDB's class, no data member). Only what src
// declares of it is here.
class hkpTransformCollapseUtil {
public:
	// What a collapse does with a shape more than one parent references (hkputilities.pdb, underlying int; the
	// collapse 0xD739F0 compares Options::m_sharedShapeBehaviour with 1 and 2).
	enum SharedShapeBehaviour : int32_t {
		ALWAYS_COLLAPSE = 0,
		NEVER_COLLAPSE = 1,
		COLLAPSE_IF_LESS_THAN_THRESHOLD = 2,
	};

	// The collapse's options (hkputilities.pdb, sizeof 12; 0xD739F0 reads +0x0, +0x4 and the byte at +0x8).
	struct Options {
		Options();

		SharedShapeBehaviour m_sharedShapeBehaviour;
		int32_t m_sharedShapeThreshold;
		hkBool m_propageTransformInList;
	};

	// The utility's counters of what a collapse did (the PDB's members).
	struct Results {
		Results();

		// Zeroes the four counters.
		void reset();
		// The PDB's report(): reports each counter through hkError as a message of
		// hkpTransformCollapseUtil.cpp, lines 57 to 60 (thiscall, ret 0, 0xD73430).
		void report() const;

		int32_t m_numCollapsedShapes;
		int32_t m_numIdentityTransformsRemoved;
		int32_t m_numSpecializedTransformShapes;
		int32_t m_numPropagatedTransformsToLists;
	};

	// The PDB's collapseTransformShape (cdecl, 0xD739F0): collapses the transform shape transformShape into its
	// child (box, capsule, cylinder, convex vertices, list, ...), when Options' shared-shape rule allows it for the
	// child's references. Callers: collapseShapesRecursively 0xD74480 and collapseTransforms 0xD74420.
	static const hkpShape* collapseTransformShape(const hkpShape* transformShape, const Options& options,
		_SharedShapeData& sharedShapeData, Results& resultsOut);

	// The PDB's collapseShapesRecursively (cdecl, 0xD74480): returns the shape's replacement recorded in
	// sharedShapeData, else collapses its children and the shape. Its caller is collapseTransforms 0xD74850.
	static const hkpShape* collapseShapesRecursively(const hkpShape* shape, const Options& options,
		_SharedShapeData& sharedShapeData, Results& resultsOut);
	// Havok SDK signature (hkpUtilities.lib, tied by bytes).
	static hkResult collapseTransforms(const hkpShape* shape, const hkpTransformCollapseUtil::Options& options,
		hkpTransformCollapseUtil::Results& resultsOut, hkpShape** shapeOut);
	// The PDB's collapseTransforms(hkpRigidBody*, const Options&, Results&) (cdecl, 0xD748D0): the array overload
	// on a one-body array.
	static hkResult collapseTransforms(hkpRigidBody* rigidBody, const Options& options, Results& resultsOut);
	// The PDB's collapseTransforms(hkArray<hkpRigidBody*>&, const Options&, Results&) (cdecl, 0xD74850): resets
	// resultsOut, records the bodies' shared shapes and replaces each body's shape by its collapsed one.
	static hkResult collapseTransforms(hkArray<hkpRigidBody*, hkContainerHeapAllocator>& rigidBodies,
		const Options& options, Results& resultsOut);
	// The PDB's transformTransformShape (cdecl, 0xD73590): shape placed by transform, folding transform into a
	// convex translate, convex transform or transform shape where it can; the result holds a reference.
	static const hkpShape* transformTransformShape(const hkpShape* shape, const hkTransform& transform);
};

// The collapse's record of shapes and their references and replacements: one SharedShape per shape reached
// (hkputilities.pdb, sizeof 12; the SDK defines it in hkpTransformCollapseUtil.cpp). collapseShapesRecursively
// 0xD74480 searches m_sharedShapes (+0x0 data, +0x4 size) by m_originalShape, in 12-byte steps.
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

	// The SDK's private recursivelyParseShapeReferences(const hkpShape*) (hkpUtilities.lib),
	// 0xD738F0 (thiscall, ret 4): counts one more reference of shape when it is recorded (by m_originalShape), else
	// records it ({shape, null, 1}, addReference) and recurses into its children (0xD737D0). Public here: its
	// callers in src are not members.
	void recursivelyParseShapeReferences(const hkpShape* shape);
	// The SDK's numberOfReferences(const hkpShape*) const (hkpUtilities.lib), 0xD74980 (thiscall, ret 4): the
	// m_numReferences of the first record whose original or replacement shape is shape, 0 when none is.
	int32_t numberOfReferences(const hkpShape* shape) const;
	// The SDK's setReplacement(const hkpShape*, const hkpShape*) (hkpUtilities.lib), 0xD749C0
	// (thiscall, ret 8): in the first record whose original or replacement shape is originalShape, replacementShape
	// (addReference) replaces m_replacementShape (removeReference); HK_FAILURE when no record is.
	hkResult setReplacement(const hkpShape* shape, const hkpShape* replacement);
	// Walks the records backwards and drops (removeReference on both shapes) those referenced
	// fewer than 2 times, compacting the rest down.
	// Havok SDK signature (hkpUtilities.lib, tied by bytes,reloc).
	void removeSingleReferencedShapeData();

	hkArray<SharedShape, hkContainerHeapAllocator> m_sharedShapes;
};
