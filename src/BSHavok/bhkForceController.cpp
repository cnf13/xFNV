#include "BSHavok/bhkForceController.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkBlendCollisionObject.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkRigidBody.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"

#include <xmmintrin.h>

NiRTTI bhkForceController::ms_RTTI("bhkForceController", &NiTimeController::ms_RTTI);

bhkForceController::bhkForceController() : Force(hkVector4Zero), fDeltaTime(0.0f) {}

// The object sits in a 16-aligned block (CreateClone, TestForce): the byte in front of it holds its offset into the
// block, which goes back to the heap.
bhkForceController::~bhkForceController() {}

const NiRTTI* bhkForceController::GetRTTI() const {
    return &bhkForceController::ms_RTTI;
}

// Clones the controller: a 16-byte-aligned block from the engine heap holding a constructed controller with the
// base's members copied, then this controller's force and its duration.
NiObject* bhkForceController::CreateClone(NiCloningProcess& kCloning) {
    // Asks for the object plus the 16 bytes the alignment can waste, aligns the block itself and keeps the offset in
    // the byte before the object, which the block's deallocator reads back. The block is untyped heap memory, so the
    // casts below are raw memory ones.
    uint8_t* pucMemory = static_cast<uint8_t*>(MemoryManager_Singleton.Allocate(sizeof(bhkForceController) + 0x10));
    uint8_t ucOffset = static_cast<uint8_t>(16 - (reinterpret_cast<uintptr_t>(pucMemory) & 0xF));
    bhkForceController* pClone = reinterpret_cast<bhkForceController*>(pucMemory + ucOffset);
    pucMemory[ucOffset - 1] = ucOffset;

    ::new (pClone) bhkForceController;
    NiTimeController::CopyMembers(pClone, kCloning);
    pClone->Force = this->Force;
    pClone->fDeltaTime = this->fDeltaTime;
    return pClone;
}

// Advances the force: the step time is the update's time since the last one (0.016 when there was none or it is
// negative), then the force is applied, and the active bit is cleared once the scaled time reaches the end key.
void bhkForceController::Update(NiUpdateData& UpdateData) {
    char m_usFlags = this->m_usFlags;
    fDeltaTime = 0.016f;
    if (((m_usFlags & 8) != 0) && (m_pkTarget)) {
        double m_fLastTime = this->m_fLastTime;
        if (m_fLastTime != -3.4028234663852886e+38) {
            float arUpdateDataa = UpdateData.fTime - this->m_fLastTime;
            fDeltaTime = arUpdateDataa;
            if (arUpdateDataa < 0.0) {
                fDeltaTime = 0.016f;
            }
        }
        float fScaledTime = ComputeScaledTime(UpdateData.fTime);
        ApplyForce();
        if (m_fHiKeyTime == fScaledTime) {
            this->m_usFlags &= ~8u;
        }
    }
}

// The target must be a scene graph object.
bool bhkForceController::TargetIsRequiredType() const {
    return NiObject::IsKindOf(&NiAVObject::ms_RTTI, m_pkTarget);
}

// Pushes the target's rigid body with the force, a hundredfold, over the last step's time, while the heir gain of the
// target's blend collision object is below 1.
void bhkForceController::ApplyForce() {
    NiAVObject* pAVObject = static_cast<NiAVObject*>(m_pkTarget);
    if (!pAVObject) {
        return;
    }
    // EXE-INLINED none: bhkBlendCollisionObject::GetbhkBlendCollisionObject
    bhkNiCollisionObject* pCollision = pAVObject->m_spCollisionObject.m_pObject;
    if (!pCollision) {
        return;
    }
    bhkBlendCollisionObject* pBlend =
        static_cast<bhkBlendCollisionObject*>(pCollision->DynamicCastMember(&bhkBlendCollisionObject::ms_RTTI));
    if (!pBlend) {
        return;
    }
    // Continues only when the gain is below 1; an unordered (NaN) gain stops.
    if (!(pBlend->fHeirGain < 1.0f)) {
        return;
    }
    // EXE-INLINED 0x6FA820
    bhkRigidBody* pBody = pBlend->spWorldObject;
    if (!pBody) {
        return;
    }
    // EXE-INLINED 0x4AE750
    hkpRigidBody* phk = static_cast<hkpRigidBody*>(pBody->phkObject);
    if (!phk) {
        return;
    }
    alignas(16) hkVector4 scaleForce = Force;
    scaleForce.mul4(100.0f);
    phk->applyForce(fDeltaTime, scaleForce);
}

// Removes the force controller from apObject and, through the vtable, from every node below it.
void bhkForceController::RemoveForce(NiAVObject* apObject) {
    if (!apObject) {
        return;
    }

    NiTimeController* pController = apObject->GetController(&bhkForceController::ms_RTTI);
    if (pController) {
        apObject->RemoveController(pController);
    }

    NiNode* pNode = apObject->IsNode();
    if (!pNode) {
        return;
    }
    for (uint32_t i = 0; i < pNode->m_kChildren.m_usSize; i++) {
        RemoveForce(pNode->m_kChildren.m_pBase[i].m_pObject);
    }
}

// Starts a force controller on apSpot, or on apObject when apSpot is null, creating it when the object has none. With a
// rigid body the force is scaled by the body's mass and then by 1 + 0.75 * its linear damping.
void bhkForceController::TestForce(NiAVObject* apObject, hkVector4& arForce, NiAVObject* apSpot) {
    NiAVObject* pTarget = apSpot;
    if (!pTarget) {
        pTarget = apObject;
    }
    NiTimeController* pController = pTarget->GetController(&bhkForceController::ms_RTTI);
    if (!pController) {
        // The block is aligned to 16 as in CreateClone, the offset kept in the byte before the object; the block is
        // untyped heap memory, so the casts are raw memory ones.
        uint8_t* pucMemory = static_cast<uint8_t*>(MemoryManager_Singleton.Allocate(sizeof(bhkForceController) + 0x10));
        uint8_t ucOffset = static_cast<uint8_t>(16 - (reinterpret_cast<uintptr_t>(pucMemory) & 0xF));
        bhkForceController* pNew = reinterpret_cast<bhkForceController*>(pucMemory + ucOffset);
        pucMemory[ucOffset - 1] = ucOffset;
        ::new (pNew) bhkForceController;
        pController = pNew;
        pController->SetTarget(pTarget);
    }
    bhkNiCollisionObject* pCollisionObject = pTarget->m_spCollisionObject.m_pObject;
    bhkCollisionObject* pBhkObject = nullptr;
    if (pCollisionObject) {
        pBhkObject =
            static_cast<bhkCollisionObject*>(pCollisionObject->DynamicCastMember(&bhkCollisionObject::ms_RTTI));
    }
    alignas(16) hkVector4 kForce = arForce;
    hkVector4 kResult = kForce;
    if (pBhkObject) {
        bhkRigidBody* pRigidBody = pBhkObject->spWorldObject;
        if (pRigidBody) {
            float fMass = pRigidBody->GetDynamicMass();
            float fMassScale = 1.0f * fMass;
            hkpRigidBody* phkBody = static_cast<hkpRigidBody*>(pRigidBody->phkObject);
            float fDamping = phkBody->m_motion.m_motionState.m_linearDamping * 0.75;
            hkVector4 kScaled;
            kScaled.setMul4(fMassScale, kForce);
            kResult.setAddMul4(kScaled, kScaled, fDamping);
        }
    }
    pController->m_fLoKeyTime = 0.0f;
    pController->m_fHiKeyTime = 0.2f;
    // The mask is 0xFFF5, not 0xFFF0, so the cycle bit 0x2 is kept.
    pController->m_usFlags = static_cast<uint16_t>((pController->m_usFlags & 0xFFF5) | 5);
    pController->m_fPhase = 0.0f;
    pController->m_fFrequency = 1.0f;
    static_cast<bhkForceController*>(pController)->Force.m_quad = kResult.m_quad;
    pController->Start(-3.4028235e+38f);
}

// As TestForce on apObject, under the Havok heap context, lasting until afEndTime. The damping share is taken from the
// caller's force (reread), the mass share from the copy, written as intrinsics.
void bhkForceController::TestForce2(NiAVObject* apObject, hkVector4& arForce, float afEndTime) {
    MEM_CONTEXT iOldMemContext = GetThreadTLSData()->eMemContext;
    GetThreadTLSData()->eMemContext = MEM_CONTEXT_HAVOK;
    NiTimeController* pController = apObject->GetController(&bhkForceController::ms_RTTI);
    if (!pController) {
        // The block is aligned to 16 as in CreateClone, the offset kept in the byte before the object; the block is
        // untyped heap memory, so the casts are raw memory ones.
        uint8_t* pucMemory = static_cast<uint8_t*>(MemoryManager_Singleton.Allocate(sizeof(bhkForceController) + 0x10));
        uint8_t ucOffset = static_cast<uint8_t>(16 - (reinterpret_cast<uintptr_t>(pucMemory) & 0xF));
        bhkForceController* pNew = reinterpret_cast<bhkForceController*>(pucMemory + ucOffset);
        pucMemory[ucOffset - 1] = ucOffset;
        ::new (pNew) bhkForceController;
        pController = pNew;
        pController->SetTarget(apObject);
    }
    bhkNiCollisionObject* pCollisionObject = apObject->m_spCollisionObject.m_pObject;
    bhkCollisionObject* pBhkObject = nullptr;
    if (pCollisionObject) {
        pBhkObject =
            static_cast<bhkCollisionObject*>(pCollisionObject->DynamicCastMember(&bhkCollisionObject::ms_RTTI));
    }
    alignas(16) hkVector4 kForce = arForce;
    hkVector4 kResult = kForce;
    if (pBhkObject) {
        bhkRigidBody* pRigidBody = pBhkObject->spWorldObject;
        if (pRigidBody) {
            float fMass = pRigidBody->GetDynamicMass();
            float fMassScale = 1.0f * fMass;
            hkpRigidBody* phkBody = static_cast<hkpRigidBody*>(pRigidBody->phkObject);
            hkVector4 kScaled;
            kScaled.setMul4(fMassScale, kForce);
            float fDamping = phkBody->m_motion.m_motionState.m_linearDamping * 0.75;
            kResult.setAddMul4(kScaled, arForce, fDamping);
        }
    }
    pController->m_fLoKeyTime = 0.0f;
    pController->m_fHiKeyTime = afEndTime;
    // The mask is 0xFFF5, not 0xFFF0, so the cycle bit 0x2 is kept.
    pController->m_usFlags = static_cast<uint16_t>((pController->m_usFlags & 0xFFF5) | 5);
    pController->m_fPhase = 0.0f;
    pController->m_fFrequency = 1.0f;
    static_cast<bhkForceController*>(pController)->Force.m_quad = kResult.m_quad;
    pController->Start(-3.4028235e+38f);
    GetThreadTLSData()->eMemContext = iOldMemContext;
}
