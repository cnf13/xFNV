#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkaRagdollInstance;
class hkaSkeleton;
class hkpConstraintInstance;
class hkpGroupFilter;
class hkpRigidBody;

// Havok SDK signature (hkaRagdoll.lib, tied by reloc,string).
hkResult alignConstraintSpacesToChildren(const hkArrayBase<hkpConstraintInstance*>& a1);

// Static ragdoll conversion and constraint utilities; no instance state.
class hkaRagdollUtils {
public:
	// Deletes skeleton (its virtual deleting destructor); nothing for null.
	static void destroySkeleton(hkaSkeleton* skeleton);
	// Builds two ragdoll instances sharing original's bodies, constraints and skeleton: trunk from the bones outside
	// splitBone's subtree, branch from the bones in it.
	static void splitRagdollInstanceShallow(const hkaRagdollInstance* original, int32_t splitBone,
		hkaRagdollInstance*& trunk, hkaRagdollInstance*& branch);
	// Deletes ragdollInstance (its virtual deleting destructor) but not the bodies it shares; nothing for null.
	static void destroyRagdollInstanceShallow(hkaRagdollInstance* ragdoll);
	// Not named in the PDB or IDA; name chosen here. Whether the ragdoll utilities handle apConstraint's type,
	// warning about it when abWarn is set. The exe takes apConstraint in eax (see the body).
	static bool isConstraintSupported(const hkpConstraintInstance* apConstraint, bool abWarn);
	static hkResult reorderForRagdoll(hkArray<hkpRigidBody*, hkContainerHeapAllocator>& rigidBodies,
		hkArray<hkpConstraintInstance*, hkContainerHeapAllocator>& constraints);
	static hkResult reorderAndAlignForRagdoll(hkArray<hkpRigidBody*, hkContainerHeapAllocator>& rigidBodiesInOut,
		hkArray<hkpConstraintInstance*, hkContainerHeapAllocator>& constraintsInOut, hkBool warnOnAlignment);
	static hkaRagdollInstance* createRagdollInstanceFromSkeleton(const hkaSkeleton* skeleton,
		const hkArrayBase<hkpRigidBody*>& candidateRBs,
		const hkArrayBase<hkpConstraintInstance*>& candidateConstraints);
	static hkaSkeleton* constructSkeletonForRagdoll(const hkArrayBase<hkpRigidBody*>& rigidBodies,
		const hkArrayBase<hkpConstraintInstance*>& constraints);
	// Not named in the PDB or IDA; name chosen here. The pivot of apConstraint in its first body's space, null for
	// an unsupported type (usercall: apConstraint in eax).
	static hkVector4* _accessConstraintPivotA(hkpConstraintInstance* apConstraint);
	// Not named in the PDB or IDA; name chosen here. The pivot of apConstraint in its second body's space (a stiff
	// spring's first pivot), null for an unsupported type (usercall: apConstraint in eax).
	static hkVector4* _accessConstraintPivotB(hkpConstraintInstance* apConstraint);
	// Havok SDK signature (hkaRagdoll.lib, tied by bytes).
	static void setCollisionLayer(hkaRagdollInstance* ragdoll, int32_t layer, hkpGroupFilter* collisionFilter);
	// Havok SDK signature (hkaRagdoll.lib, tied by bytes).
	static hkaRagdollInstance* copyRagdollInstanceShallow(const hkaRagdollInstance* original);
};
