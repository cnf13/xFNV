#pragma once

#include "BSCore/BSPrecisionTimer.h"
#include "BSCore/BSSimpleArray.h"
#include "BSMain/Setting.h"
#include "Havok/Source/Animation/Ragdoll/Penetration/hkaDetectRagdollPenetration.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"

#include <cstddef>
#include <stdint.h>

class bhkAabbPhantom;
class bhkRigidBody;
class hkaRagdollInstance;
class hkaSkeleton;
class hkpConstraintInstance;
class hkpRigidBody;

// Detects ragdoll bones stuck inside geometry and pushes them out: the ray casts its detection utils ask for. Only part
// of the class is declared: an Actor allocates one in Actor::ConstructorInitialize.
class bhkRagdollPenetrationUtil : public hkaRagdollRaycastInterface {
public:
    // One ragdoll the util watches: its scene graph root, ragdoll and skeleton, the bounding phantom, the detection
    // util and the bones it found penetrating.
    struct ConstrainedSystem {
        ConstrainedSystem()
          : pRoot3d(nullptr), pRagdollInstance(nullptr), pSkel(nullptr), pBoundingPhantom(nullptr),
            pDetectionUtil(nullptr), bPenetrating(true) {}
        ~ConstrainedSystem();

        NiAVObject* pRoot3d;
        hkaRagdollInstance* pRagdollInstance;
        hkaSkeleton* pSkel;
        bhkAabbPhantom* pBoundingPhantom;
        hkaDetectRagdollPenetration* pDetectionUtil;
        // The bones the last detection found penetrating.
        hkaDetectRagdollPenetration::Output PenetratedBones;
        bool bPenetrating;
    };

    bhkRagdollPenetrationUtil();
    ~bhkRagdollPenetrationUtil() override;

    // Casts a ray through the world from arFromWS to arToWS, skipping apPhantomIn.
    hkBool castRay(
        const hkpAabbPhantom* apPhantomIn,
        const hkVector4& arFromWS,
        const hkVector4& arToWS,
        hkVector4& arHitPointWS,
        hkVector4& arHitNormalWS
    ) override;

    // Returns bAttemptToInitializeNextUpdate as the field's raw byte, so the return type is uint8_t rather than bool.
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
    // slave of no non-contact one.
    void FindLeaves(
        hkArray<hkpRigidBody*, hkContainerHeapAllocator>& arRigidBodies,
        BSSimpleArray<bhkRigidBody*, 1024>& arLeaves
    );
    // Appends apRB's Havok body to arRigidBodies and the Havok constraints on its action/constraint list to
    // arConstraints, unless one of them is breakable.
    void AddToArrays(
        bhkRigidBody* apRB,
        hkArray<hkpRigidBody*, hkContainerHeapAllocator>& arRigidBodies,
        hkArray<hkpConstraintInstance*, hkContainerHeapAllocator>& arConstraints
    );
    // Adds the Havok bodies and constraints of every blended rigid body under apObject that is in a world, then
    // recurses into the children.
    void FindBones(
        NiAVObject* apObject,
        hkArray<hkpRigidBody*, hkContainerHeapAllocator>& arRigidBodies,
        hkArray<hkpConstraintInstance*, hkContainerHeapAllocator>& arConstraints
    );
    // A new skeleton with a bone per rigid body and a nub bone per leaf.
    hkaSkeleton* CreateSkeletonForRagdoll(
        hkArray<hkpRigidBody*, hkContainerHeapAllocator>& arRigidBodies,
        hkArray<hkpConstraintInstance*, hkContainerHeapAllocator>& arConstraints,
        BSSimpleArray<bhkRigidBody*, 1024>& arLeaves
    );

    // The game's heap (MemoryManager).
    static void* operator new(size_t blocksize);
    static void operator delete(void* apMemory);

    BSSimpleArray<ConstrainedSystem*> ActiveSystems;
    uint32_t iGroup;
    uint8_t bAttemptToInitializeNextUpdate;
    uint32_t uiInitAttempts;
    BSPrecisionTimer DetectionTimer;

    // The ini setting for the interval between two penetration detections.
    static SettingT<INISettingCollection> fDetectionUpdateTimeSec;
};
