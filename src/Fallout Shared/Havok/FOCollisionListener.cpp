#include "Fallout Shared/Havok/FOCollisionListener.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkEntity.h"
#include "BSHavok/bhkRagdollController.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkUtilFunctions.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/Setting.h"
#include "BSShader/BSShaderProperty.h"
#include "Fallout Shared/ActorCause.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/HitCaster.h"
#include "Fallout Shared/Havok/TESHavokUtilities.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/AILinearTaskThreadManager.h"
#include "Fallout/AI/Combat/GrenadeProjectile.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Audio/ImpactMixer.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPoint.h"
#include "Havok/Source/Common/Base/Types/Physics/ContactPoint/hkContactPointMaterial.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Query/Collector/PointCollector/hkpRootCdPoint.h"
#include "Havok/Source/Physics/Dynamics/Collide/ContactListener/hkpContactListener.h"
#include "Havok/Source/Physics/Dynamics/Collide/Deprecated/hkpCollisionListener.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

#include <cstdint>
#include <xmmintrin.h>

static SettingT<INISettingCollection> fMinSoundVel("fMinSoundVel:Audio", 10.0f);

// The shape's material capped at BHK_MATERIAL_TUMBLEWEED through the int BSMIN; without a game shape, the
// land's material for ground, else the shape collection's.
HavokMaterialType GetMaterialFromCollidable(const hkpCdBody* apCollidable, NiPoint3* arLandPos) {
    bhkShape* pShape = bhkShape::Getbhk(apCollidable->m_shape);
    if (pShape) {
        HavokMaterialType Material = pShape->GetMaterial();
        return static_cast<HavokMaterialType>(
            BSMIN(static_cast<int32_t>(Material), static_cast<int32_t>(BHK_MATERIAL_TUMBLEWEED))
        );
    }

    const hkpCollidable* Root = apCollidable->getRootCollidable();
    CFilter kFilter;
    kFilter.iFilter = Root->getCollisionFilterInfo();
    if (kFilter.GetLayer() == L_GROUND) {
        return TES::pInstance->GetLandMaterialType(*arLandPos);
    }

    return bhkUtilFunctions::GetMaterialValue(apCollidable);
}

bhkContactListener::bhkContactListener() {}

bhkContactListener::~bhkContactListener() {}

void bhkContactListener::operator delete(void* apMemory) {
    ourFree(apMemory);
}

FOCollisionListener::ObjectDamageImpactData::ObjectDamageImpactData()
  : spBody(nullptr), fDamage(0.0f), spCause(nullptr) {}

FOCollisionListener::ObjectDamageImpactData::~ObjectDamageImpactData() {}

void FOCollisionListener::ObjectDamageImpactData::operator delete(void* apMemory) {
    ourFree(apMemory);
}

FOCollisionListener::FOCollisionListener() {
    pSingleton = this;
    DamageImpacts.RemoveAll();
}

FOCollisionListener::~FOCollisionListener() {
    pSingleton = nullptr;
    ClearDamageImpacts();
}

// A contact point the world accepted: plays the collision sound when the bodies meet fast enough (unless a biped's
// tail or a body of its own group is involved), then records the impact's object damage and reports it to
// projectiles.
void FOCollisionListener::contactPointAddedCallback(hkpContactPointAddedEvent& arEvent) {
    if (arEvent.m_status != HK_CONTACT_POINT_ACCEPT) {
        return;
    }
    ImpactMixer::COLLISION_SOUND soundInfo;
    soundInfo.~COLLISION_SOUND();
    ::new (&soundInfo) ImpactMixer::COLLISION_SOUND;
    soundInfo.fVelocity = NiAbs(HK2NI(arEvent.m_projectedVelocity));
    const hkpCollidable* pca = arEvent.m_bodyA->getRootCollidable();
    const hkpCollidable* pcb = arEvent.m_bodyB->getRootCollidable();
    if (soundInfo.fVelocity >= static_cast<float>(fMinSoundVel) && soundInfo.fVelocity >= 1.0) {
        if (pca && pcb) {
            CFilter ifa;
            ifa.iFilter = pca->getCollisionFilterInfo();
            CFilter ifb;
            ifb.iFilter = pcb->getCollisionFilterInfo();
            uint32_t uiGroupA = ifa.GetGroup();
            if (uiGroupA == ifb.GetGroup()) {
                if (ifa.QBipLayer()) {
                    return;
                }
            }
            else if (ifa.QBipLayer()) {
                if (ifa.GetBipedPart() == P_TAIL) {
                    return;
                }
            }
            else if (ifb.QBipLayer() && ifb.GetBipedPart() == P_TAIL) {
                return;
            }
        }
        HK2NI(soundInfo.Location, arEvent.m_contactPoint->getPosition());
        float fFriction;
        if (arEvent.m_contactPointProperties) {
            fFriction = arEvent.m_contactPointProperties->getFriction();
        }
        else {
            fFriction = 0.3f;
        }
        soundInfo.fFriction = fFriction;
        soundInfo.cMaterialType1 = GetMaterialFromCollidable(arEvent.m_bodyA, &soundInfo.Location);
        soundInfo.cMaterialType2 = GetMaterialFromCollidable(arEvent.m_bodyB, &soundInfo.Location);
        soundInfo.pBody1 = nullptr;
        soundInfo.pBody2 = nullptr;
        if (pca && pca->getType() == hkpWorldObject::BROAD_PHASE_ENTITY) {
            hkpRigidBody* prb = hkpGetRigidBody(pca);
            if (prb) {
                soundInfo.pBody1 = static_cast<bhkRigidBody*>(bhkEntity::Getbhk(prb));
            }
        }
        if (pcb && pcb->getType() == hkpWorldObject::BROAD_PHASE_ENTITY) {
            hkpRigidBody* prb = hkpGetRigidBody(pcb);
            if (prb) {
                soundInfo.pBody2 = static_cast<bhkRigidBody*>(bhkEntity::Getbhk(prb));
            }
        }
        soundInfo.iID = (soundInfo.cMaterialType1 + 1) * (soundInfo.cMaterialType2 + 1);
        ImpactMixer::PlayCollisionSound(soundInfo);
    }
    hkpRigidBody* pbodyA = hkpGetRigidBody(pca);
    bhkCollisionObject* pcolA = pbodyA ? bhkCollisionObject::Getbhk(pbodyA) : nullptr;
    NiAVObject* pavA = pcolA ? pcolA->m_pkSceneObject : nullptr;
    TESObjectREFR* prefA = pavA ? TESObjectREFR::FindReferenceFor3D(pavA) : nullptr;
    hkpRigidBody* pbodyB = hkpGetRigidBody(pcb);
    bhkCollisionObject* pcolB = pbodyB ? bhkCollisionObject::Getbhk(pbodyB) : nullptr;
    NiAVObject* pavB = pcolB ? pcolB->m_pkSceneObject : nullptr;
    TESObjectREFR* prefB = pavB ? TESObjectREFR::FindReferenceFor3D(pavB) : nullptr;
    StoreObjectDamage(arEvent.m_projectedVelocity, pbodyA, pbodyB, prefA, prefB);
    ReportProjectileImpact(prefA, arEvent);
    ReportProjectileImpact(prefB, arEvent);
}

void FOCollisionListener::DealObjectDamage() {
    BSSimpleList<FOCollisionListener::ObjectDamageImpactData*>* plist = &DamageImpacts;
    while (plist && !plist->IsEmpty()) {
        FOCollisionListener::ObjectDamageImpactData* pdata = plist->m_item;
        plist->RemoveHead();
        if ((pdata->spBody.m_pObject) && (pdata->fDamage > 0.0)) {
            TESObjectREFR* pref;
            NiAVObject* pav;
            bhkCollisionObject* pcol;
            hkpRigidBody* pbody = pdata->spBody.m_pObject->operator*();
            if (pbody) {
                pcol = bhkCollisionObject::Getbhk(pbody);
            }
            else {
                pcol = nullptr;
            }
            if (pcol) {
                pav = pcol->m_pkSceneObject;
            }
            else {
                pav = nullptr;
            }
            if (pav) {
                pref = TESObjectREFR::FindReferenceFor3D(pav);
            }
            else {
                pref = nullptr;
            }
            if (pref) {
                pref->SetActorCause(pdata->spCause.m_pObject);
                if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                    float afDamage = pdata->fDamage;
                    TaskQueueInterface* Instance = TaskQueueInterface::GetInstance();
                    Instance->QueueUpdateDestructibleObject(pref, afDamage, false);
                }
                else {
                    pref->DamageObject(pdata->fDamage, false);
                }
                if ((pref->GetDestroyed()) && (pdata->spCause.m_pObject)) {
                    PlayerCharacter* Actor_ = static_cast<PlayerCharacter*>(pdata->spCause.m_pObject->pActor);
                    if (Actor_ == pPlayer) {
                        pPlayer->AddPlayerAction(PLAYER_ACTION_DESTROY_OBJECT, 8.0f, nullptr);
                    }
                }
            }
        }

        if (pdata) {
            delete pdata;
        }
    }
}

void FOCollisionListener::ReportProjectileImpact(TESObjectREFR* apCollidingRef, hkpContactPointAddedEvent& arEvent) {
    if ((apCollidingRef) &&
        (apCollidingRef->IsProjectile() &&
         (((int32_t)static_cast<Projectile*>(apCollidingRef)->GetProjectileType()) == 3))) {
        static_cast<GrenadeProjectile*>(apCollidingRef)->ReportCollision(arEvent);
    }
}

void FOCollisionListener::ClearDamageImpacts() {
    int32_t result;
    FOCollisionListener::ObjectDamageImpactData* v3;
    while (true) {
        result = (int32_t)(&DamageImpacts)->IsEmpty();
        if (result) {
            break;
        }
        v3 = (&DamageImpacts)->m_item;
        if (v3) {
            delete v3;
        }
        (&DamageImpacts)->RemoveHead();
    }
}

void FOCollisionListener::StoreObjectDamage(
    float afProjectedVelocity,
    hkpRigidBody* apBodyA,
    hkpRigidBody* apBodyB,
    TESObjectREFR* apRefA,
    TESObjectREFR* apRefB
) {
    float fvelocity = NiAbs(afProjectedVelocity);
    if (!(fPhysicsDamageSpeedMin.Float() > static_cast<double>(fvelocity))) {
        if ((apRefA) &&
            (apRefA->GetDestructible() && !apRefA->GetDestroyed() && !apRefA->IsProjectile() &&
             (!apRefB || !apRefB->IsProjectile()))) {
            float fmass = apBodyB->getMass();
            if (fmass == 0.0) {
                fmass = 10000.0f;
            }
            float fdamage = GetObjectImpactDamage(fmass, fvelocity);
            if (fdamage > 0.0) {
                FOCollisionListener::ObjectDamageImpactData* pdata;
                FOCollisionListener::ObjectDamageImpactData* apMemory =
                    static_cast<FOCollisionListener::ObjectDamageImpactData*>(
                        ourMalloc(sizeof(FOCollisionListener::ObjectDamageImpactData))
                    );
                if (apMemory) {
                    pdata = ::new (apMemory) FOCollisionListener::ObjectDamageImpactData;
                }
                else {
                    pdata = nullptr;
                }
                FOCollisionListener::ObjectDamageImpactData* pdata_2 = pdata;
                pdata->spBody.operator=(static_cast<bhkRigidBody*>(bhkEntity::Getbhk(apBodyA)));
                pdata->fDamage = fdamage;
                if (apRefB) {
                    ActorCause* apObject = apRefB->GetActorCause();
                    pdata_2->spCause.operator=(apObject);
                }
                else {
                    pdata_2->spCause.operator=(static_cast<ActorCause*>(nullptr));
                }
                DamageImpacts.AddHead(pdata_2);
            }
        }

        if ((apRefB) &&
            (apRefB->GetDestructible() && !apRefB->GetDestroyed() && !apRefB->IsProjectile() &&
             (!apRefA || !apRefA->IsProjectile()))) {
            float fmass_2 = apBodyA->getMass();
            if (fmass_2 == 0.0) {
                fmass_2 = 10000.0f;
            }
            float fdamage_2 = GetObjectImpactDamage(fmass_2, fvelocity);
            if (fdamage_2 > 0.0) {
                FOCollisionListener::ObjectDamageImpactData* pdata_3;
                FOCollisionListener::ObjectDamageImpactData* v11 =
                    static_cast<FOCollisionListener::ObjectDamageImpactData*>(
                        ourMalloc(sizeof(FOCollisionListener::ObjectDamageImpactData))
                    );
                if (v11) {
                    pdata_3 = ::new (v11) FOCollisionListener::ObjectDamageImpactData;
                }
                else {
                    pdata_3 = nullptr;
                }
                FOCollisionListener::ObjectDamageImpactData* pdata_4 = pdata_3;
                pdata_3->spBody.operator=(static_cast<bhkRigidBody*>(bhkEntity::Getbhk(apBodyB)));
                pdata_3->fDamage = fdamage_2;
                if (apRefA) {
                    ActorCause* v6 = apRefA->GetActorCause();
                    pdata_4->spCause.operator=(v6);
                }
                else {
                    pdata_4->spCause.operator=(static_cast<ActorCause*>(nullptr));
                }
                DamageImpacts.AddHead(pdata_4);
            }
        }
    }
}

FOCollisionListener* FOCollisionListener::Instance() {
    if (FOCollisionListener::pSingleton) {
        return FOCollisionListener::pSingleton;
    }
    void* v3 = ourMalloc(sizeof(FOCollisionListener));
    if (v3) {
        return ::new (v3) FOCollisionListener;
    }

    return nullptr;
}

BIPED_PART CFilter::GetBipedPart() {
    return static_cast<BIPED_PART>(GetPart());
}

float hkContactPointMaterial::getFriction() const {
    return m_friction;
}

const hkClass& hkContactPointMaterial::staticClass() {
    return hkContactPointMaterialClass;
}

hkpContactListener::hkpContactListener() {}

hkpContactListener::~hkpContactListener() {}

void hkpContactListener::operator delete(void* apMemory) {
    ourFree(apMemory);
}
