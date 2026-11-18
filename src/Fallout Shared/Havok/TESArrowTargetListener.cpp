#include "Fallout Shared/Havok/TESArrowTargetListener.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkEntity.h"
#include "BSHavok/bhkEntityListener.h"
#include "BSHavok/bhkRigidBody.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/Havok/bhkArrowTargetListener.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Combat/MissileProjectile.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Fallout/AI/ProcessArray.h"
#include "Fallout/AI/ProcessLists.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"

BSSimpleList<ArrowTargetGroup*> TESArrowTargetListener::TargetList;

TESArrowTargetListener::TESArrowTargetListener() {}

TESArrowTargetListener::~TESArrowTargetListener() {}

void TESArrowTargetListener::entityRemovedCallback(hkpEntity* apEntity) {
    bhkEntity* pe = bhkEntity::Getbhk(apEntity);
    bhkRigidBody* pb = static_cast<bhkRigidBody*>(NiObject::DynamicCastStatic(&bhkRigidBody::ms_RTTI, pe));
    if ((pb) && (pb->GetFlag(bhkWorldObject::STICKING_TARGET))) {
        NiAVObject* pav;
        bhkCollisionObject* pcol = bhkCollisionObject::Getbhk(pb->operator*());
        if (pcol) {
            pav = pcol->m_pkSceneObject;
        }
        else {
            pav = nullptr;
        }
        TESObjectREFR::FindReferenceFor3D(pav);
        ProcessArray* pMobArray = &ReferenceProcessLists.MobProcessArray;
        uint32_t i_2 = 0;
        for (uint32_t i = pMobArray->GetProcessLevelTail(PROCESS_TYPE_HIGH); i_2 < i;
             i = pMobArray->GetProcessLevelTail(PROCESS_TYPE_HIGH)) {
            Projectile* pprojectile = static_cast<Projectile*>(pMobArray->GetItem(i_2));
            if ((pprojectile) &&
                (pprojectile->IsProjectile() && pprojectile->IsFlagSet(Projectile::POST_PROCESS) &&
                 pprojectile->GetFormType() == PMIS_ID && (pprojectile->iFormFlags & 32) == 0 &&
                 pprojectile->GetImpactData()->pTargetWorldObject == apEntity &&
                 static_cast<MissileProjectile*>(pprojectile)->GetImpactResult() == MissileProjectile::IR_STICK))
            // A PMIS_ID projectile is a MissileProjectile.
            {
                pprojectile->Kill();
            }

            ++i_2;
        }
    }
}

// Kills every missile stuck in the deleted entity's body.
void TESArrowTargetListener::entityDeletedCallback(hkpEntity* apEntity) {
    bhkEntity* pe = bhkEntity::Getbhk(apEntity);
    bhkRigidBody* pb = static_cast<bhkRigidBody*>(NiObject::DynamicCastStatic(&bhkRigidBody::ms_RTTI, pe));
    if ((pb) && (pb->GetFlag(bhkWorldObject::STICKING_TARGET))) {
        NiAVObject* pav;
        bhkCollisionObject* pcol = bhkCollisionObject::Getbhk(pb->operator*());
        if (pcol) {
            pav = pcol->m_pkSceneObject;
        }
        else {
            pav = nullptr;
        }
        TESObjectREFR::FindReferenceFor3D(pav);
        ProcessArray* pMobArray = &ReferenceProcessLists.MobProcessArray;
        uint32_t i_2 = 0;
        for (uint32_t i = pMobArray->GetProcessLevelTail(PROCESS_TYPE_HIGH); i_2 < i;
             i = pMobArray->GetProcessLevelTail(PROCESS_TYPE_HIGH)) {
            Projectile* pprojectile = static_cast<Projectile*>(pMobArray->GetItem(i_2));
            if ((pprojectile) &&
                (pprojectile->IsProjectile() && pprojectile->IsFlagSet(Projectile::POST_PROCESS) &&
                 pprojectile->GetFormType() == PMIS_ID &&
                 pprojectile->GetImpactData()->pTargetWorldObject == apEntity &&
                 static_cast<MissileProjectile*>(pprojectile)->GetImpactResult() == MissileProjectile::IR_STICK))
            // A PMIS_ID projectile is a MissileProjectile.
            {
                pprojectile->Kill();
            }

            ++i_2;
        }
    }
}

void TESArrowTargetListener::AddArrowTarget(hkVector4& aArrowCenterOfMassWorld, NiAVObject* apTargetObject) {
    bhkCollisionObject* pCollisionObject =
        apTargetObject ? bhkCollisionObject::GetbhkCollisionObject(apTargetObject) : nullptr;
    bhkRigidBody* pBody = pCollisionObject ? pCollisionObject->GetRigidBody() : nullptr;
    if (!pBody) {
        return;
    }
    ArrowTargetGroup* pGroup = nullptr;
    if (!TargetList.IsEmpty()) {
        for (BSSimpleList<ArrowTargetGroup*>* pNode = &TargetList; pNode && !pNode->IsEmpty() && !pGroup;
             pNode = pNode->GetNext()) {
            ArrowTargetGroup* pItem = pNode->GetItem();
            if (pItem && pItem->pBody == pBody) {
                pGroup = pItem;
            }
        }
    }
    if (!pGroup) {
        pGroup = static_cast<ArrowTargetGroup*>(ourMalloc(sizeof(ArrowTargetGroup)));
        pGroup->pBody = pBody;
        pGroup->pObject = apTargetObject;
        pGroup->fTotalMass = pBody->GetDynamicMass();
        pGroup->iArrowCount = 0;
        TargetList.AddHead(pGroup);
    }
    float fTotalMass = pGroup->fTotalMass;
    alignas(16) hkTransform kBodyTransform;
    kBodyTransform.~hkTransform();
    ::new (&kBodyTransform) hkTransform;
    alignas(16) hkTransform kBodyInverse;
    kBodyInverse.~hkTransform();
    ::new (&kBodyInverse) hkTransform;
    alignas(16) hkTransform kArrowTransform;
    XFNV_RECONSTRUCT(hkTransform)(&kArrowTransform, hkQuaternion::getIdentity(), aArrowCenterOfMassWorld);
    alignas(16) hkTransform kArrowLocal;
    kArrowLocal.~hkTransform();
    ::new (&kArrowLocal) hkTransform;
    pBody->GetTransform(kBodyTransform);
    kBodyInverse.setInverse(kBodyTransform);
    kArrowLocal.setMul(kBodyInverse, kArrowTransform);
    alignas(16) hkVector4 kArrowMoment;
    XFNV_RECONSTRUCT(hkVector4)(&kArrowMoment, kArrowLocal.getTranslation());
    alignas(16) hkSimdReal kFakeMass;
    XFNV_RECONSTRUCT(hkSimdReal)(&kFakeMass, fArrowFakeMass.Float());
    kArrowMoment.mul4(kFakeMass);
    alignas(16) hkVector4 kCenterOfMass;
    kCenterOfMass.~hkVector4();
    ::new (&kCenterOfMass) hkVector4;
    pBody->GetCenterOfMassLocal(kCenterOfMass);
    alignas(16) hkSimdReal kTotalMass;
    kTotalMass.~hkSimdReal();
    ::new (&kTotalMass) hkSimdReal(fTotalMass);
    kCenterOfMass.mul4(kTotalMass);
    alignas(16) hkVector4 kNewCenter;
    kNewCenter.~hkVector4();
    ::new (&kNewCenter) hkVector4;
    kNewCenter.setAdd4(kCenterOfMass, kArrowMoment);
    float fInvMass = 1.0f / (fTotalMass + fArrowFakeMass.Float());
    alignas(16) hkSimdReal kInvMass;
    kInvMass.~hkSimdReal();
    ::new (&kInvMass) hkSimdReal(fInvMass);
    kNewCenter.mul4(kInvMass);
    pBody->operator*()->setCenterOfMassLocal(kNewCenter);
    pBody->operator*()->activate();
    pGroup->fTotalMass = fTotalMass + fArrowFakeMass.Float();
    pGroup->iArrowCount = pGroup->iArrowCount + 1;
}

void TESArrowTargetListener::RemoveArrowTarget(hkVector4& aArrowCenterOfMassWorld, NiAVObject* apTargetObject) {
    ArrowTargetGroup* pGroup = nullptr;
    if (!TargetList.IsEmpty()) {
        for (BSSimpleList<ArrowTargetGroup*>* pNode = &TargetList; pNode && !pNode->IsEmpty() && !pGroup;
             pNode = pNode->GetNext()) {
            ArrowTargetGroup* pItem = pNode->GetItem();
            if (pItem && pItem->pObject == apTargetObject) {
                pGroup = pItem;
            }
        }
    }
    if (!pGroup) {
        return;
    }
    bhkCollisionObject* pCollisionObject =
        apTargetObject ? bhkCollisionObject::GetbhkCollisionObject(apTargetObject) : nullptr;
    bhkRigidBody* pBody = pCollisionObject ? pCollisionObject->GetRigidBody() : nullptr;
    if (!pBody) {
        return;
    }
    float fTotalMass = pGroup->fTotalMass;
    alignas(16) hkTransform kBodyTransform;
    kBodyTransform.~hkTransform();
    ::new (&kBodyTransform) hkTransform;
    alignas(16) hkTransform kBodyInverse;
    kBodyInverse.~hkTransform();
    ::new (&kBodyInverse) hkTransform;
    alignas(16) hkTransform kArrowTransform;
    XFNV_RECONSTRUCT(hkTransform)(&kArrowTransform, hkQuaternion::getIdentity(), aArrowCenterOfMassWorld);
    alignas(16) hkTransform kArrowLocal;
    kArrowLocal.~hkTransform();
    ::new (&kArrowLocal) hkTransform;
    pBody->GetTransform(kBodyTransform);
    kBodyInverse.setInverse(kBodyTransform);
    kArrowLocal.setMul(kBodyInverse, kArrowTransform);
    alignas(16) hkVector4 kArrowMoment;
    XFNV_RECONSTRUCT(hkVector4)(&kArrowMoment, kArrowLocal.getTranslation());
    alignas(16) hkSimdReal kFakeMass;
    XFNV_RECONSTRUCT(hkSimdReal)(&kFakeMass, fArrowFakeMass.Float());
    kArrowMoment.mul4(kFakeMass);
    alignas(16) hkVector4 kCenterOfMass;
    kCenterOfMass.~hkVector4();
    ::new (&kCenterOfMass) hkVector4;
    pBody->GetCenterOfMassLocal(kCenterOfMass);
    alignas(16) hkSimdReal kTotalMass;
    kTotalMass.~hkSimdReal();
    ::new (&kTotalMass) hkSimdReal(fTotalMass);
    kCenterOfMass.mul4(kTotalMass);
    alignas(16) hkVector4 kNewCenter;
    kNewCenter.~hkVector4();
    ::new (&kNewCenter) hkVector4;
    kNewCenter.setSub4(kCenterOfMass, kArrowMoment);
    float fInvMass = 1.0f / (fTotalMass - fArrowFakeMass.Float());
    alignas(16) hkSimdReal kInvMass;
    kInvMass.~hkSimdReal();
    ::new (&kInvMass) hkSimdReal(fInvMass);
    kNewCenter.mul4(kInvMass);
    pBody->operator*()->setCenterOfMassLocal(kNewCenter);
    alignas(16) hkVector4 kPush;
    kPush.~hkVector4();
    ::new (&kPush) hkVector4;
    kPush.setSub4(kCenterOfMass, kArrowMoment);
    kPush.normalize3();
    alignas(16) hkSimdReal kPushSpeed;
    kPushSpeed.~hkSimdReal();
    ::new (&kPushSpeed) hkSimdReal(1.5f);
    kPush.mul4(kPushSpeed);
    kPush.add4(pBody->GetLinearVelocity());
    pBody->SetLinearVelocity(kPush);
    pGroup->fTotalMass = fTotalMass - fArrowFakeMass.Float();
    pGroup->iArrowCount = pGroup->iArrowCount - 1;
    if (pGroup->iArrowCount <= 0) {
        ourFree(pGroup);
        TargetList.Remove(pGroup);
    }
}

bhkArrowTargetListener::bhkArrowTargetListener() : bhkEntityListener(0) {}

bhkArrowTargetListener::~bhkArrowTargetListener() {}

void bhkArrowTargetListener::operator delete(void* apMemory) {
    ourFree(apMemory);
}
