#include "Fallout Shared/Havok/TESArrowTargetListener.h"

#include "BSCore/NewOverloads.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkEntity.h"
#include "BSHavok/bhkEntityListener.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkWorld.h"
#include "Fallout Shared/CombatFormulas.h"
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
    if (pb && pb->GetFlag(bhkWorldObject::STICKING_TARGET)) {
        bhkCollisionObject* pcol = bhkCollisionObject::Getbhk(**pb);
        NiAVObject* pav = pcol ? pcol->m_pkSceneObject : nullptr;
        TESObjectREFR::FindReferenceFor3D(pav);
        ProcessArray* pMobArray = &ReferenceProcessLists.MobProcessArray;
        for (uint32_t i = 0; i < pMobArray->GetProcessLevelTail(PROCESS_TYPE_HIGH); ++i) {
            Projectile* pProjectile = static_cast<Projectile*>(pMobArray->GetItem(i));
            if (pProjectile &&
                pProjectile->IsProjectile() &&
                pProjectile->IsFlagSet(Projectile::POST_PROCESS) &&
                pProjectile->GetFormType() == PMIS_ID &&
                (pProjectile->iFormFlags & TESForm::kFormFlag_Deleted) == 0 &&
                pProjectile->GetImpactData()->pTargetWorldObject == apEntity &&
                static_cast<MissileProjectile*>(pProjectile)->GetImpactResult() == MissileProjectile::IR_STICK)
            // A PMIS_ID projectile is a MissileProjectile.
            {
                pProjectile->Kill();
            }
        }
    }
}

// Kills every missile stuck in the deleted entity's body.
void TESArrowTargetListener::entityDeletedCallback(hkpEntity* apEntity) {
    bhkEntity* pe = bhkEntity::Getbhk(apEntity);
    bhkRigidBody* pb = static_cast<bhkRigidBody*>(NiObject::DynamicCastStatic(&bhkRigidBody::ms_RTTI, pe));
    if (pb && pb->GetFlag(bhkWorldObject::STICKING_TARGET)) {
        bhkCollisionObject* pcol = bhkCollisionObject::Getbhk(**pb);
        NiAVObject* pav = pcol ? pcol->m_pkSceneObject : nullptr;
        TESObjectREFR::FindReferenceFor3D(pav);
        ProcessArray* pMobArray = &ReferenceProcessLists.MobProcessArray;
        for (uint32_t i = 0; i < pMobArray->GetProcessLevelTail(PROCESS_TYPE_HIGH); ++i) {
            Projectile* pProjectile = static_cast<Projectile*>(pMobArray->GetItem(i));
            if (pProjectile &&
                pProjectile->IsProjectile() &&
                pProjectile->IsFlagSet(Projectile::POST_PROCESS) &&
                pProjectile->GetFormType() == PMIS_ID &&
                pProjectile->GetImpactData()->pTargetWorldObject == apEntity &&
                static_cast<MissileProjectile*>(pProjectile)->GetImpactResult() == MissileProjectile::IR_STICK)
            // A PMIS_ID projectile is a MissileProjectile.
            {
                pProjectile->Kill();
            }
        }
    }
}

void TESArrowTargetListener::AddArrowTarget(hkVector4& aArrowCenterOfMassWorld, NiAVObject* apTargetObject) {
    bhkCollisionObject* pCollisionObject = apTargetObject ? bhkCollisionObject::GetbhkCollisionObject(apTargetObject)
                                                          : nullptr;
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
    alignas(16) hkTransform kBodyInverse;
    alignas(16) hkTransform kArrowTransform(hkQuaternion::getIdentity(), aArrowCenterOfMassWorld);
    alignas(16) hkTransform kArrowLocal;
    pBody->GetTransform(kBodyTransform);
    kBodyInverse.setInverse(kBodyTransform);
    kArrowLocal.setMul(kBodyInverse, kArrowTransform);
    alignas(16) hkVector4 kArrowMoment(kArrowLocal.getTranslation());
    alignas(16) hkSimdReal kFakeMass(fArrowFakeMass.Float());
    kArrowMoment.mul4(kFakeMass);
    alignas(16) hkVector4 kCenterOfMass;
    pBody->GetCenterOfMassLocal(kCenterOfMass);
    alignas(16) hkSimdReal kTotalMass(fTotalMass);
    kCenterOfMass.mul4(kTotalMass);
    alignas(16) hkVector4 kNewCenter;
    kNewCenter.setAdd4(kCenterOfMass, kArrowMoment);
    float fInvMass = 1.0f / (fTotalMass + fArrowFakeMass.Float());
    alignas(16) hkSimdReal kInvMass(fInvMass);
    kNewCenter.mul4(kInvMass);
    (**pBody)->setCenterOfMassLocal(kNewCenter);
    (**pBody)->activate();
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
    bhkCollisionObject* pCollisionObject = apTargetObject ? bhkCollisionObject::GetbhkCollisionObject(apTargetObject)
                                                          : nullptr;
    bhkRigidBody* pBody = pCollisionObject ? pCollisionObject->GetRigidBody() : nullptr;
    if (!pBody) {
        return;
    }
    float fTotalMass = pGroup->fTotalMass;
    alignas(16) hkTransform kBodyTransform;
    alignas(16) hkTransform kBodyInverse;
    alignas(16) hkTransform kArrowTransform(hkQuaternion::getIdentity(), aArrowCenterOfMassWorld);
    alignas(16) hkTransform kArrowLocal;
    pBody->GetTransform(kBodyTransform);
    kBodyInverse.setInverse(kBodyTransform);
    kArrowLocal.setMul(kBodyInverse, kArrowTransform);
    alignas(16) hkVector4 kArrowMoment(kArrowLocal.getTranslation());
    alignas(16) hkSimdReal kFakeMass(fArrowFakeMass.Float());
    kArrowMoment.mul4(kFakeMass);
    alignas(16) hkVector4 kCenterOfMass;
    pBody->GetCenterOfMassLocal(kCenterOfMass);
    alignas(16) hkSimdReal kTotalMass(fTotalMass);
    kCenterOfMass.mul4(kTotalMass);
    alignas(16) hkVector4 kNewCenter;
    kNewCenter.setSub4(kCenterOfMass, kArrowMoment);
    float fInvMass = 1.0f / (fTotalMass - fArrowFakeMass.Float());
    alignas(16) hkSimdReal kInvMass(fInvMass);
    kNewCenter.mul4(kInvMass);
    (**pBody)->setCenterOfMassLocal(kNewCenter);
    alignas(16) hkVector4 kPush;
    kPush.setSub4(kCenterOfMass, kArrowMoment);
    kPush.normalize3();
    alignas(16) hkSimdReal kPushSpeed(1.5f);
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
