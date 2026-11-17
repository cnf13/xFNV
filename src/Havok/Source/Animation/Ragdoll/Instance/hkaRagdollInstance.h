#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkQsTransform;
class hkQuaternion;
class hkaSkeleton;
class hkpRigidBody;
class hkpWorld;

// The finish function hkaRagdollInstanceTypeInfo stores.
void finishLoadedObjecthkaRagdollInstance(void* p, int32_t finishing);

// A ragdoll: its rigid bodies and constraints, the skeleton they follow and the bone-to-body map.
class hkaRagdollInstance : public hkReferencedObject {
public:
	// An instance with no bodies, constraints or skeleton, safe to destroy: the getVtable function declares the
	// local, then constructs it again in place.
	hkaRagdollInstance() : m_skeleton(nullptr) {}
	hkaRagdollInstance(const hkArrayBase<hkpRigidBody*>& rigidBodies,
		const hkArrayBase<hkpConstraintInstance*>& constraints, const hkaSkeleton* skeleton,
		const hkArrayBase<int32_t>& boneToRigidBody);
	hkaRagdollInstance(const hkArrayBase<hkpRigidBody*>& rigidBodies,
		const hkArrayBase<hkpConstraintInstance*>& constraints, const hkaSkeleton* skeleton);
	hkaRagdollInstance(hkFinishLoadedObjectFlag flag);
	~hkaRagdollInstance() override;

	// A copy of the ragdoll: each rigid body cloned, each constraint cloned between the new bodies with mode
	// clonedRigidBodies (the PDB's parameter name), the skeleton and the bone-to-body map shared.
	virtual hkaRagdollInstance* clone(hkpConstraintInstance::CloningMode mode) const;

	// The bone's world transform at afTime into apResult. Const as in the SDK
	// (hkaRagdollInstance.h); getApproxPoseWorldSpaceAt (const) calls it.
	void getApproxWorldFromBoneTransformAt(int32_t i, float time, hkQsTransform& worldFromBoneOut) const;
	// Havok SDK: the rigid body mapped to bone i, null when none is.
	hkpRigidBody* getRigidBodyOfBone(int32_t i) const;
	// The pose in world space from the rigid bodies.
	void getPoseWorldSpace(hkQsTransform* poseOut) const;
	// The pose in world space approximated at time.
	void getApproxPoseWorldSpaceAt(float time, hkQsTransform* poseWorldSpaceOut) const;
	// The approximate pose at the world's current time, or the exact pose when the ragdoll is in no world.
	void getApproxCurrentPoseWorldSpace(hkQsTransform* poseWorldSpaceOut) const;
	// Sets each mapped rigid body's position and rotation from the model-space pose and the
	// world-from-model transform (world = worldFromModel * poseIn[bone]), zeroing its linear and angular velocities
	// when it has any (thiscall, ret 8, 0xCE1AD0; IDA names it setPoseWorldSpace, which the PDB types with one
	// parameter).
	void setPoseModelSpace(const hkQsTransform* poseIn, const hkQsTransform& worldFromModel);
	// Keys each rigid body to its bone in pose B (worldFromModelB * poseModelSpaceB) and gives it the linear and
	// angular velocities that move it there from pose A in timestep.
	void setPoseAndVelocitiesModelSpace(const hkQsTransform* poseModelSpaceA, const hkQsTransform& worldFromModelA,
		const hkQsTransform* poseModelSpaceB, const hkQsTransform& worldFromModelB, float timestep);
	void getPoseModelSpace(hkQsTransform* poseOut, const hkQsTransform& a2) const;
	// Const as in the SDK (hkaRagdollInstance.h); getPoseWorldSpace(hkQsTransform*) (const)
	// calls it.
	void getPoseWorldSpace(hkQsTransform* poseOut, const hkQsTransform& worldFromModel,
		const hkQsTransform* localSpacePoseForUnmappedBonesOptional) const;
	// Not reconstructed yet. Takes a reference on skeleton and releases the old one (+0x2C), copies rigidBodies (+0x8)
	// and constraints (+0x14) and takes a reference on each element (thiscall, ret 0xC, 0xCE22C0); both constructors
	// (0xCE2470, 0xCE2510) call it. IDA misnames 0xCE2470 commonInit.
	void commonInit(const hkArrayBase<hkpRigidBody*>& rigidBodies,
		const hkArrayBase<hkpConstraintInstance*>& constraints, const hkaSkeleton* skeleton);
	void setPoseModelSpaceZeroVelocities(hkQuaternion* position);
	// Havok SDK signature (hkaRagdoll.lib, tied by bytes).
	hkResult addToWorld(hkpWorld* world, hkBool updateFilter) const;
	// Havok SDK signature (hkaRagdoll.lib, tied by bytes).
	hkResult removeFromWorld() const;
	// The bone's world transform into worldFromBone. Const as in the SDK.
	void getWorldFromBoneTransform(int32_t i, hkQsTransform& worldFromBoneOut) const;

	static void operator delete(void* apMemory) { hkReferencedObject::deleteObject(apMemory); }
	// Not named in the PDB or IDA; name chosen here. The SDK's constructor from rigid bodies,
	// constraints and skeleton, with the identity bone-to-body map (thiscall, ret 0xC, 0xCE2470: sets the vtable and
	// the reference count, empties the arrays, calls commonInit, then fills m_boneToRigidBodyMap with 0..bones-1).
	static const hkClass& staticClass();

	hkArray<hkpRigidBody*> m_rigidBodies;
	hkArray<hkpConstraintInstance*> m_constraints;
	hkArray<int32_t> m_boneToRigidBodyMap;
	const hkaSkeleton* m_skeleton;
};

extern hkClass hkaRagdollInstanceClass;
extern hkTypeInfo hkaRagdollInstanceTypeInfo;
inline constexpr fnvd::Vtable hkaRagdollInstance_vftable{};
