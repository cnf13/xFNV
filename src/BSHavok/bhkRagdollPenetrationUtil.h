#pragma once

#include "BSCore/BSPrecisionTimer.h"
#include "BSMain/Setting.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "BSCore/BSSimpleArray.h"
#include "Havok/Source/Animation/Ragdoll/Penetration/hkaDetectRagdollPenetration.h"
#include "Reconstruction.hpp"

#include <cstddef>
#include <stdint.h>

class bhkAabbPhantom;
class bhkRigidBody;
class hkaRagdollInstance;
class hkaSkeleton;
class hkpConstraintInstance;
class hkpRigidBody;

// Detects ragdoll bones stuck inside geometry and pushes them out: the ray casts its detection utils ask for.
// Only what the reconstructed code uses is declared: an Actor allocates one in Actor::ConstructorInitialize.
class bhkRagdollPenetrationUtil : public hkaRagdollRaycastInterface {
public:
	// One ragdoll the util watches: its scene graph root, ragdoll and skeleton, the bounding phantom, the detection
	// util and the bones it found penetrating.
	struct ConstrainedSystem {
		~ConstrainedSystem();

		NiAVObject* pRoot3d;
		hkaRagdollInstance* pRagdollInstance;
		hkaSkeleton* pSkel;
		bhkAabbPhantom* pBoundingPhantom;
		hkaDetectRagdollPenetration* pDetectionUtil;
		// The bones the last detection found penetrating (IDA types its three words kPenetratedBones and two more).
		hkaDetectRagdollPenetration::Output PenetratedBones;
		bool bPenetrating;
		uint8_t padding21[3];
	};

	bhkRagdollPenetrationUtil();
	~bhkRagdollPenetrationUtil() override;

	// Casts a ray through the world from arFromWS to arToWS, skipping apPhantomIn (slot 1;
	// thiscall, ret 0x18, 0xCA1C50).
	hkBool* castRay(hkBool* arResult, const hkpAabbPhantom* apPhantomIn, const hkVector4& arFromWS,
		const hkVector4& arToWS, hkVector4& arHitPointWS, hkVector4& arHitNormalWS) override;

	// bAttemptToInitializeNextUpdate (thiscall, ret 0, 0x978400). PDB: bool; the exe returns the field's raw byte
	// (`mov al, [eax+0x18]`), so the return is the field's uint8_t.
	uint8_t QInitialOnNextUpdate();
	// Finds the ragdoll systems under apRoot for collision group aiGroup.
	bool Initialize(NiAVObject* apRoot, int32_t aiGroup);
	// Detects and pushes out penetrating bones.
	void Update();
	// Marks every watched system as penetrating, resets its bone states and restarts the detection timer.
	void ResetSystems();
	// Drops the ragdoll systems it watches.
	void Clear();
	// Appends to arSystems every object under apObject whose Havok body masters only contact constraints and is the
	// slave of a constraint of another type, then recurses into the children.
	void FindSystems(NiAVObject* apObject, BSSimpleArray<NiAVObject*>& arRoots);
	// Appends to arLeaves the bhkRigidBody of each body of arRigidBodies that masters a non-contact constraint and is
	// slave of no non-contact one (thiscall, ret 8, 0xCA1080; the PDB's FindLeaves, matched by its parameters and
	// locals).
	void FindLeaves(hkArray<hkpRigidBody*, hkContainerHeapAllocator>& arRigidBodies,
		BSSimpleArray<bhkRigidBody*, 1024>& arLeaves);
	// Appends apRB's Havok body to arRigidBodies and the Havok constraints on its action/constraint list to
	// arConstraints, unless one of them is breakable (thiscall, ret 0xC, 0xCA11E0; the PDB's AddToArrays, matched by
	// its parameters and locals).
	void AddToArrays(bhkRigidBody* apRB, hkArray<hkpRigidBody*, hkContainerHeapAllocator>& arRigidBodies,
		hkArray<hkpConstraintInstance*, hkContainerHeapAllocator>& arConstraints);
	// Adds the Havok bodies and constraints of every blended rigid body under apObject that is in a world, then
	// recurses into the children (thiscall, ret 0xC, 0xCA1F00; the PDB's FindBones, matched by its parameters and
	// locals).
	void FindBones(NiAVObject* apObject, hkArray<hkpRigidBody*, hkContainerHeapAllocator>& arRigidBodies,
		hkArray<hkpConstraintInstance*, hkContainerHeapAllocator>& arConstraints);
	// A new skeleton with a bone per rigid body and a nub bone per leaf.
	hkaSkeleton* CreateSkeletonForRagdoll(hkArray<hkpRigidBody*, hkContainerHeapAllocator>& arRigidBodies,
		hkArray<hkpConstraintInstance*, hkContainerHeapAllocator>& arConstraints,
		BSSimpleArray<bhkRigidBody*, 1024>& arLeaves);

	// 16-byte aligned memory.
	static void* operator new(size_t blocksize);
	// Frees the 16-aligned block operator new made: the alignment padding byte sits in front of the object.
	static void operator delete(void* apMemory);

	BSSimpleArray<ConstrainedSystem*> ActiveSystems;
	uint32_t iGroup;
	uint8_t bAttemptToInitializeNextUpdate;
	uint8_t padding19[3];
	uint32_t uiInitAttempts;
	BSPrecisionTimer DetectionTimer;

	// The ini setting for the interval between two penetration detections.
	static SettingT<INISettingCollection> fDetectionUpdateTimeSec;
};
inline constexpr fnvd::Vtable bhkRagdollPenetrationUtil_vftable{};
