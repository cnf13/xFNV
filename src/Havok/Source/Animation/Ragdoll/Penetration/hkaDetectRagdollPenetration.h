#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

class hkaPose;
class hkaSkeleton;
class hkpAabbPhantom;

// The ray cast the detection asks its owner for (bhkRagdollPenetrationUtil is one in the engine): casts from fromWS to
// toWS against the world, ignoring the ragdoll's phantom; true when something was hit.
class hkaRagdollRaycastInterface {
public:
    virtual ~hkaRagdollRaycastInterface() {}

    virtual hkBool* castRay(
        hkBool* arResult,
        const hkpAabbPhantom* apPhantomIn,
        const hkVector4& arFromWS,
        const hkVector4& arToWS,
        hkVector4& arHitPointWS,
        hkVector4& arHitNormalWS
    ) = 0;

    // MemoryManager::Deallocate (defined in BSHavok/bhkRagdollPenetrationUtil.cpp).
    static void operator delete(void* apMemory);
};

// Finds the ragdoll's bones that have gone through the world, by ray casts along the skeleton.
class hkaDetectRagdollPenetration : public hkReferencedObject {
public:
    // What the detection is run on.
    struct Setup {
        const hkaSkeleton* m_ragdollSkeleton;
        hkaRagdollRaycastInterface* m_raycastInterface;
        hkpAabbPhantom* m_ragdollPhantom;
    };

    // A ray hit on a bone: the bone's range of the skeleton, the point and the normal.
    struct BonePenetration {
        // Sets the bone range to 0 to 1 and the point and the normal to zero.
        BonePenetration();
        ~BonePenetration() = default;

        int16_t m_boneBeginIndex;
        int16_t m_boneEndIndex;
        hkVector4 m_penetrationPoint;
        hkVector4 m_penetrationNormal;
    };

    // The detection's result: the bones that penetrate.
    struct Output {
        ~Output();

        hkArray<BonePenetration> m_penetrations;
    };

    // Per bone: not penetrating, penetrating, or below a penetrating bone.
    enum BonePenetrationStatus : int32_t {
        HK_NOP = 0,
        HK_YESP = 1,
        HK_YESP_DESCENDANT = 2,
    };

    hkaDetectRagdollPenetration(hkaDetectRagdollPenetration::Setup* a1);
    ~hkaDetectRagdollPenetration() override;

    // Marks every bone as penetrating and the ragdoll as not out yet.
    void resetBoneStates();
    // Finds the penetrating bones for the pose and writes them to arOutput.
    hkBool* detectPenetration(hkBool* arResult, const hkaPose& arRagdollPoseIn, Output& arOutput);
    const hkArray<BonePenetrationStatus>& getBoneStatusArray() {
        return m_boneStates;
    }

    static void operator delete(void* apMemory) {
        hkReferencedObject::deleteObject(apMemory);
    }

    Setup m_setup;
    int32_t m_numBones;
    hkArray<BonePenetrationStatus> m_boneStates;
    hkBool m_ragdollIsOut;
};
