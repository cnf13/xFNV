#include "BSHavok/bhkCollisionObject.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkRigidBody.h"
#include "Fallout Shared/TES.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"

#include <bit>
#include <xmmintrin.h>

NiRTTI bhkCollisionObject::ms_RTTI("bhkCollisionObject", &bhkNiCollisionObject::ms_RTTI);

// The clone's allocation is charged to the Havok heap context, which is restored after the copy.
NiObject* bhkCollisionObject::CreateClone(NiCloningProcess& arCloneData) {
    AutoMemContext kMemContext(MEM_CONTEXT_HAVOK);
    bhkCollisionObject* pkClone =
        static_cast<bhkCollisionObject*>(NiMemObject::operator new(sizeof(bhkCollisionObject)));
    if (pkClone) {
        ::new (pkClone) bhkCollisionObject;
    }
    CopyMembers(pkClone, arCloneData);
    return pkClone;
}

void bhkCollisionObject::LoadBinary(NiStream& arStream) {
    bhkNiCollisionObject::LoadBinary(arStream);
}

void bhkCollisionObject::LinkObject(NiStream& arStream) {
    bhkNiCollisionObject::LinkObject(arStream);
}

bool bhkCollisionObject::RegisterStreamables(NiStream& arStream) {
    return (bool)((uint8_t)bhkNiCollisionObject::RegisterStreamables(arStream));
}

void bhkCollisionObject::SaveBinary(NiStream& arStream) {
    bhkNiCollisionObject::SaveBinary(arStream);
}

bool bhkCollisionObject::IsEqual(NiObject* apObject) {
    return bhkNiCollisionObject::IsEqual(apObject);
}

void bhkCollisionObject::AddViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    TLSData* v3 = GetThreadTLSData();
    MEM_CONTEXT eMemContext = v3->eMemContext;
    v3->eMemContext = MEM_CONTEXT_HAVOK;
    bhkNiCollisionObject::AddViewerStrings(apStrings);
    char* ViewerString = NiGetViewerString(bhkCollisionObject::ms_RTTI.GetName());
    uint32_t m_usSize = apStrings->m_usSize;
    uint32_t m_usMaxSize = apStrings->m_usMaxSize;
    char* v14 = ViewerString;
    if (m_usSize >= m_usMaxSize) {
        apStrings->SetSize(m_usSize + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(m_usSize, v14);
    uint16_t usFlags = this->usFlags;
    bool v9 = (usFlags & 32) != 0 && (usFlags & 64) == 0;
    char* ViewerString_0 = NiGetViewerString("bUseVel", v9);
    uint32_t v11 = apStrings->m_usSize;
    uint32_t v12 = apStrings->m_usMaxSize;
    v14 = ViewerString_0;
    if (v11 >= v12) {
        apStrings->SetSize(v11 + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(v11, v14);
    v3->eMemContext = eMemContext;
}

void bhkCollisionObject::UpdateWorldData(NiUpdateData& UpdateData) {
    NiAVObject* m_pkSceneObject;
    bhkRigidBody* spWorldObj = spWorldObject;
    if (spWorldObj && ((spWorldObj->iFlags & 64) != 0)) {
        m_pkSceneObject = this->m_pkSceneObject;
        if (m_pkSceneObject) {
            m_pkSceneObject->UpdateWorldLoc();
        }
    }
    else {
        bhkNiCollisionObject::UpdateWorldData(UpdateData);
    }
}

// The linear velocity of the Havok body in game units; zero without a body.
NiPoint3 bhkCollisionObject::GetVelocity() {
    bhkRigidBody* pRigidBody = spWorldObject;
    if (!pRigidBody) {
        return NiPoint3_ZERO;
    }
    hkpRigidBody* phkObject = static_cast<hkpRigidBody*>(pRigidBody->phkObject);
    const hkVector4& kVelocity = phkObject ? phkObject->m_motion.m_linearVelocity : hkVector4Zero;
    NiPoint3 kResult;
    HK2NI(kResult, kVelocity);
    return kResult;
}

// KEEP: the transform handed to SetTransform has no scale written (the exe's slot at esp+0x48 is never stored), and
// bhkNiCollisionObject::SetTransform (0xC6BBB0) copies all 13 dwords of it (rep movsd) and multiplies that scale in
// NiTransform::operator*: an uninitialised read.
// A fixed body in a world only updates the scene object's world data; any other body's rotation and position are
// set as the collision object's transform.
void bhkCollisionObject::hktoNiTransform() {
    bhkRigidBody* pRigidBody = spWorldObject;
    if (pRigidBody) {
        hkpRigidBody* phkBody = static_cast<hkpRigidBody*>(pRigidBody->phkObject);
        if (phkBody) {
            if (phkBody->m_motion.m_type.m_storage == hkpMotion::MOTION_FIXED && phkBody->m_world) {
                NiAVObject* pObj = m_pkSceneObject;
                if (pObj) {
                    pObj->UpdateWorldLoc();
                }
            }
            else {
                NiQuaternion kRot;
                NiTransform kTransform;
                pRigidBody->GetRotation(kRot);
                kRot.ToRotation(kTransform.m_Rotate);
                pRigidBody->GetPosition(kTransform.m_Translate);
                SetTransform(kTransform);
            }
        }
    }
}

// Pushes the scene object's world transform to the rigid body. A collision object flagged 0x40 sets the body to the
// node's position and rotation under the keyframe lock. Otherwise a node flagged as moved, or an unflagged node more
// than one Havok unit away from its body, is set alike, unless the object uses velocities or the body's motion is
// dynamic: then the transform is applied as a hard keyframe. An unflagged closer node only has the body's velocities
// cleared. A fixed body in a world is left alone.
void bhkCollisionObject::NitohkTransform() {
    NiAVObject* pObj = m_pkSceneObject;
    bhkRigidBody* pRigidBody = spWorldObject;
    if (!pObj) {
        return;
    }
    pObj->UpdateWorldLoc();
    if (!pRigidBody) {
        return;
    }
    hkpRigidBody* phkBody = static_cast<hkpRigidBody*>(pRigidBody->phkObject);
    hkpMotion::MotionType eMotionType;
    if (phkBody) {
        eMotionType = static_cast<hkpMotion::MotionType>(phkBody->m_motion.m_type.m_storage);
    }
    else {
        eMotionType = hkpMotion::MOTION_FIXED;
    }
    if (eMotionType == hkpMotion::MOTION_FIXED && pRigidBody->bhkWorldObject::GethkWorld()) {
        return;
    }
    if (!pRigidBody->phkObject) {
        return;
    }
    const NiPoint3& kTranslate = pObj->m_kWorld.m_Translate;
    NiQuaternion kRot;
    kRot.FromRotation(pObj->m_kWorld.m_Rotate);
    hkVector4 kPos;
    if ((usFlags & 64) != 0) {
        NI2HK(kPos, kTranslate);
    }
    else {
        const uint32_t uiFlags = pObj->m_uFlags;
        if ((uiFlags & NiAVObject::WORLD_BOUND_CHANGE) != 0 || (uiFlags & NiAVObject::PLAYER_BONE) != 0 ||
            (uiFlags & NiAVObject::SELECTIVE_UPDATE_CONTROLLER) != 0 ||
            (uiFlags & NiAVObject::SELECTIVE_UPDATE_RIGID) != 0) {
            if (GetUseVelocity() || bhkRigidBody::IsMotionTypeDynamic(eMotionType)) {
                pRigidBody->ApplyHardKeyframe(kTranslate, kRot, &bhkNiCollisionObject::kKeyframeLock);
                return;
            }
            NI2HK(kPos, kTranslate);
        }
        else {
            hkVector4 kTargetPos;
            NI2HK(kTargetPos, kTranslate);
            hkVector4 kBodyPos;
            pRigidBody->GetPosition(kBodyPos);
            hkVector4 kDiff;
            kDiff.setSub4(kTargetPos, kBodyPos);
            const float fDistSq = kDiff.lengthSquared3();
            if (!(fDistSq > 1.0f)) {
                if (!pRigidBody->GetLinearVelocity().equals4(hkVector4Zero)) {
                    pRigidBody->SetLinearVelocity(hkVector4Zero);
                }
                if (!pRigidBody->GetAngularVelocity().equals4(hkVector4Zero)) {
                    pRigidBody->SetAngularVelocity(hkVector4Zero);
                }
                return;
            }
            if (GetUseVelocity() || bhkRigidBody::IsMotionTypeDynamic(eMotionType)) {
                pRigidBody->ApplyHardKeyframe(kTranslate, kRot, &bhkNiCollisionObject::kKeyframeLock);
                return;
            }
            NI2HK(kPos, kTranslate);
        }
    }
    hkQuaternion kHkRot;
    kHkRot.m_vec.m_quad[0] = kRot.m_fX;
    kHkRot.m_vec.m_quad[1] = kRot.m_fY;
    kHkRot.m_vec.m_quad[2] = kRot.m_fZ;
    kHkRot.m_vec.m_quad[3] = kRot.m_fW;
    kHkRot.normalize();
    bhkNiCollisionObject::kKeyframeLock.Lock(nullptr);
    pRigidBody->SetPosRot(kPos, kHkRot);
    bhkNiCollisionObject::kKeyframeLock.Unlock();
}

void bhkCollisionObject::ClearVelocities() {
    bhkRigidBody* spWorldObject = this->spWorldObject;
    if (spWorldObject) {
        hkpRigidBody* phkObject = static_cast<hkpRigidBody*>(spWorldObject->phkObject);
        hkVector4* p_m_linearVelocity = &phkObject->m_motion.m_linearVelocity;
        if (!phkObject) {
            p_m_linearVelocity = &hkVector4Zero;
        }
        __m128 v4 = _mm_shuffle_ps(
            _mm_set_ss(std::bit_cast<float>(0x3A83126Fu)), _mm_set_ss(std::bit_cast<float>(0x3A83126Fu)), 0
        );
        if (((_mm_movemask_ps(_mm_cmple_ps(
                  _mm_and_ps(_mm_sub_ps(p_m_linearVelocity->m_quad, hkVector4Zero.m_quad), hkVector4_setAbs4_absMask),
                  v4
              )) &
              7) != 7) &&
            (phkObject)) {
            phkObject->setLinearVelocity(hkVector4Zero);
        }

        hkpRigidBody* v5 = static_cast<hkpRigidBody*>(spWorldObject->phkObject);
        hkVector4* p_m_angularVelocity = &v5->m_motion.m_angularVelocity;
        if (!v5) {
            p_m_angularVelocity = &hkVector4Zero;
        }
        if (((_mm_movemask_ps(_mm_cmple_ps(
                  _mm_and_ps(_mm_sub_ps(p_m_angularVelocity->m_quad, hkVector4Zero.m_quad), hkVector4_setAbs4_absMask),
                  v4
              )) &
              7) != 7) &&
            (v5)) {
            v5->setAngularVelocity(hkVector4Zero);
        }
    }
}

// Changes the Havok body's motion type, keeping the dynamic flag (8) in step, and wakes the body when asked. The
// keyframe lock is held across the change; the release decrements the lock count and clears the owner at zero.
void bhkCollisionObject::SetMotionType(hkpMotion::MotionType aeType, bhkWorldObject* apObject, bool abAllowActivate) {
    bhkRigidBody* pRigidBody = nullptr;
    if (apObject) {
        pRigidBody = static_cast<bhkRigidBody*>(apObject->DynamicCastMember(&bhkRigidBody::ms_RTTI));
    }
    if (!pRigidBody) {
        pRigidBody = spWorldObject;
        if (!pRigidBody) {
            return;
        }
    }
    hkpRigidBody* phkObject = static_cast<hkpRigidBody*>(pRigidBody->phkObject);
    int32_t iCurrentType;
    if (phkObject) {
        iCurrentType = static_cast<int32_t>(phkObject->m_motion.m_type.m_storage);
    }
    else {
        iCurrentType = hkpMotion::MOTION_FIXED;
    }
    // A dynamic request against an already dynamic body changes nothing.
    if (iCurrentType == aeType) {
        return;
    }
    if (aeType == hkpMotion::MOTION_DYNAMIC &&
        bhkRigidBody::IsMotionTypeDynamic(static_cast<hkpMotion::MotionType>(iCurrentType))) {
        return;
    }
    bhkNiCollisionObject::kKeyframeLock.Lock(nullptr);
    bool bChanged = pRigidBody->SetMotionType(aeType);
    if (--bhkNiCollisionObject::kKeyframeLock.uiLockCount == 0) {
        bhkNiCollisionObject::kKeyframeLock.uiOwningThread = 0;
    }
    if (!bChanged) {
        return;
    }
    phkObject = static_cast<hkpRigidBody*>(pRigidBody->phkObject);
    if (phkObject) {
        iCurrentType = static_cast<int32_t>(phkObject->m_motion.m_type.m_storage);
    }
    else {
        iCurrentType = hkpMotion::MOTION_FIXED;
    }
    if (bhkRigidBody::IsMotionTypeDynamic(static_cast<hkpMotion::MotionType>(iCurrentType))) {
        usFlags |= 8;
    }
    else {
        usFlags &= static_cast<uint16_t>(~8u);
    }
    if (!abAllowActivate) {
        return;
    }
    int32_t iMotionType = pRigidBody->GetMotionType();
    if (!bhkRigidBody::IsMotionTypeDynamic(static_cast<hkpMotion::MotionType>(iMotionType))) {
        return;
    }
    hkpEntity* phkEntity = static_cast<hkpRigidBody*>(pRigidBody->phkObject);
    if (!phkEntity) {
        return;
    }
    pRigidBody->MarkWrite();
    phkEntity->activate();
    pRigidBody->UnMarkWrite();
}

bool bhkCollisionObject::QKeyframe() {
    bhkRigidBody* spWorldObject;
    hkpRigidBody* phkObject;
    int32_t m_type;
    spWorldObject = this->spWorldObject;
    if (!spWorldObject) {
        return false;
    }
    phkObject = static_cast<hkpRigidBody*>(spWorldObject->phkObject);
    uint8_t result = 1;
    if (!phkObject) {
        return (bool)result;
    }
    m_type = (int32_t)phkObject->m_motion.m_type.m_storage;
    if ((m_type != 4) && (m_type != 5)) {
        return false;
    }
    return (bool)result;
}

// Copies the base's members, the flags and a clone of the world object into apDest. The clone lookup is
// NiObject::CreateSharedClone written out.
void bhkCollisionObject::CopyMembers(bhkCollisionObject* apDest, NiCloningProcess& arCloneData) {
    NiObject::CopyMembers(apDest, arCloneData);
    apDest->usFlags = usFlags;
    bhkRigidBody* pWorldObject = spWorldObject;
    if (pWorldObject) {
        NiPointer<bhkWorldObject> spSource(pWorldObject);
        NiPointer<bhkWorldObject> spClone;
        // pkClone starts as apDest, so a lookup that succeeds without writing its result leaves apDest there.
        NiObject* pkClone = apDest;
        if (!arCloneData.m_pkCloneMap->GetAt(pWorldObject, pkClone)) {
            pkClone = pWorldObject->CreateClone(arCloneData);
        }
        spClone.SetNew(static_cast<bhkWorldObject*>(pkClone));
        apDest->SetWorldObject(spClone);
    }
}

// The class loader's factory: a bare bhkCollisionObject block, or null when the allocation fails.
NiObject* bhkCollisionObject::CreateObject() {
    bhkCollisionObject* pkObject =
        static_cast<bhkCollisionObject*>(NiMemObject::operator new(sizeof(bhkCollisionObject)));
    if (!pkObject) {
        return nullptr;
    }
    return ::new (pkObject) bhkCollisionObject;
}
