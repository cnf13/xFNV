#include "Fallout Shared/Havok/TESHavokUtilities.h"

#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkRagdollController.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkRigidBodyT.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkShapeCollection.h"
#include "BSHavok/bhkUtilFunctions.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSMain/BSXFlags.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Combat/HitData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/hkpMoppBvTreeShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/ConvexVertices/hkpConvexVerticesShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/World/Util/hkpWorldOperationUtil.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"

double GetObjectImpactDamage(float a1, float a2) {
    double v5;
    float v7;
    float v6 = 1000.0f;
    if (!(fPhysicsDamage1Mass.Float() > static_cast<double>(a1))) {
        if (!(fPhysicsDamage2Mass.Float() > static_cast<double>(a1))) {
            if (!(fPhysicsDamage3Mass.Float() > static_cast<double>(a1))) {
                v7 = fPhysicsDamage3Damage.Float();
                v6 = fPhysicsDamage3SpeedMin.Float();
            }
            else {
                v7 = fPhysicsDamage2Damage.Float();
                v6 = fPhysicsDamage2SpeedMin.Float();
            }
        }
        else {
            v7 = fPhysicsDamage1Damage.Float();
            v6 = fPhysicsDamage1SpeedMin.Float();
        }
    }
    else {
        v7 = 0.0;
    }
    if (!(v6 < static_cast<double>(a2))) {
        v5 = 0.0;
    }
    else {
        float* v2 = &fPhysicsDamageSpeedBase.Float();
        v5 = a2 * fPhysicsDamageSpeedMult.Float() + *v2;
    }
    float v8 = v5;
    return (v7 * v8);
}

TESObjectREFR* TESHavokUtilities::FindCollidableRef(const hkpCollidable* apCol) {
    NiNode* a1;
    TESObjectREFR* v3 = nullptr;
    if (apCol) {
        a1 = static_cast<NiNode*>(bhkUtilFunctions::GetNiAVObject(const_cast<hkpCollidable*>(apCol)));
        if (a1) {
            return TESObjectREFR::FindReferenceFor3D(a1);
        }
    }
    return v3;
}

// Converts aVelocity to Havok units and hands it to the vector overload below.
void TESHavokUtilities::AddVelocity(NiAVObject* apObject, const NiPoint3& aVelocity, bool abReset) {
    hkVector4 kVelocity;
    kVelocity.getQuad();
    NI2HK(kVelocity, aVelocity);
    AddVelocity(apObject, kVelocity, abReset);
}

// Adds the Havok vector aVelocity to the dynamic rigid bodies under apObject, replacing their velocity when abReset.
void TESHavokUtilities::AddVelocity(NiAVObject* apObject, const hkVector4& aVelocity, bool abReset) {
    if (!apObject) {
        return;
    }
    bhkCollisionObject* pCollisionObject = bhkCollisionObject::GetbhkCollisionObject(apObject);
    if (pCollisionObject) {
        bhkRigidBody* pBody = static_cast<bhkRigidBody*>(pCollisionObject->GetWorldObject());
        if (pBody) {
            int32_t iMotionType = pBody->GetMotionType();
            if (bhkRigidBody::IsMotionTypeDynamic(static_cast<hkpMotion::MotionType>(iMotionType))) {
                alignas(16) hkVector4 kVelocity;
                kVelocity.getQuad();
                // Reads the components in the order z, y, x.
                float fZ = aVelocity(2);
                float fY = aVelocity(1);
                float fX = aVelocity(0);
                kVelocity.set(fX, fY, fZ, 0.0f);
                if (!abReset) {
                    kVelocity.add4(pBody->GetLinearVelocity());
                }
                pBody->SetLinearVelocity(kVelocity);
            }
        }
    }
    NiNode* pNode = apObject->IsNode();
    if (pNode) {
        for (uint32_t i = 0; i < pNode->GetArrayCount(); ++i) {
            AddVelocity(pNode->GetAt(i), aVelocity, abReset);
        }
    }
}

// Converts aVelocity to Havok units and hands it to the vector overload below.
void TESHavokUtilities::AddAngularVelocity(NiAVObject* apObject, const NiPoint3& aVelocity, bool abReset) {
    hkVector4 kVelocity;
    kVelocity.getQuad();
    NI2HK(kVelocity, aVelocity);
    AddAngularVelocity(apObject, kVelocity, abReset);
}

// Adds aVelocity's x, y and z to the angular velocity of every rigid body under apObject, replacing it when abReset,
// then does the same for the node's children.
void TESHavokUtilities::AddAngularVelocity(NiAVObject* apObject, const hkVector4& aVelocity, bool abReset) {
    if (!apObject) {
        return;
    }
    bhkCollisionObject* pCollisionObject = bhkCollisionObject::GetbhkCollisionObject(apObject);
    if (pCollisionObject) {
        bhkRigidBody* pBody = static_cast<bhkRigidBody*>(pCollisionObject->GetWorldObject());
        if (pBody) {
            hkVector4 kVelocity;
            kVelocity.getQuad();
            kVelocity.set(aVelocity(0), aVelocity(1), aVelocity(2), 0.0f);
            if (!abReset) {
                kVelocity.add4(pBody->GetAngularVelocity());
            }
            pBody->SetAngularVelocity(kVelocity);
        }
    }
    NiNode* pNode = apObject->IsNode();
    if (pNode) {
        for (uint32_t i = 0; i < pNode->GetArrayCount(); ++i) {
            AddAngularVelocity(pNode->GetAt(i), aVelocity, abReset);
        }
    }
}

// Merges the simulation islands of the actor's bodies (its ragdoll's, else its 3D's) that share the first body's world
// into the first body's island.
void TESHavokUtilities::ConsolidateSimIslands(Actor* pActor) {
    if (pActor) {
        using RigidBodyArray = hkArray<hkpRigidBody*, hkContainerHeapAllocator>;
        RigidBodyArray rbA;
        if (pActor->pRagdollController) {
            pActor->pRagdollController->GetRBForBones(rbA);
        }
        else {
            bhkUtilFunctions::GatherRigidBodies(pActor->Get3D(), rbA);
        }
        if (rbA.m_size) {
            hkpWorld* pw = rbA.operator[]<hkpRigidBody*>(0)->getWorld();
            if (pw) {
                pw->markForWrite();
                for (int32_t i = 1; i < rbA.m_size; ++i) {
                    hkpSimulationIsland* pislRoot = rbA.operator[]<hkpRigidBody*>(0)->getSimulationIsland();
                    hkpSimulationIsland* pislCur = rbA.operator[]<hkpRigidBody*>(i)->getSimulationIsland();
                    if (pw == rbA.operator[]<hkpRigidBody*>(i)->getWorld() && pislRoot != pislCur) {
                        // Reads the i-th body before the first.
                        hkpRigidBody* pBody = rbA.operator[]<hkpRigidBody*>(i);
                        hkpWorldOperationUtil::mergeIslands(pw, rbA.operator[]<hkpRigidBody*>(0), pBody);
                    }
                }
                pw->unmarkForWrite();
            }
        }
    }
}

float TESHavokUtilities::ScaleGameplayImpulseForce(float afBaseForce, bhkRigidBody* apPushedBody, bool abUseMinMass) {
    float* v4;
    float* v5;
    float* v6;
    float* v7;
    float* v8;
    float* v9;
    float mass;
    CFilter a2a;
    if (!apPushedBody) {
        return afBaseForce;
    }
    float v14 = (float)1.0;
    apPushedBody->bhkWorldObject::GetCollisionFilter(&a2a);
    switch ((int32_t)(&a2a)->GetLayer()) {
        case 8:
        case 29: {
            v5 = &(&fGameplayImpulseMultBiped)->Float();
            v14 = v14 * (*v5);
            break;
        }
        case 10: {
            v6 = &(&fGameplayImpulseMultProp)->Float();
            v14 = v14 * (*v6);
            break;
        }
        case 14: {
            v4 = &(&fGameplayImpulseMultTrap)->Float();
            v14 = v14 * (*v4);
            break;
        }
        case 20: {
            v7 = &(&fGameplayImpulseMultDebrisLarge)->Float();
            v14 = v14 * (*v7);
            break;
        }
        default: {
            v8 = &(&fGameplayImpulseMultClutter)->Float();
            v14 = v14 * (*v8);
            break;
        }
    }
    if (abUseMinMass) {
        mass = apPushedBody->GetMass();
        if ((&fGameplayImpulseMinMass)->Float() > ((double)mass)) {
            v9 = &(&fGameplayImpulseMinMass)->Float();
            v14 = (mass / (*v9)) * v14;
        }
    }
    float* v10 = &(&fGameplayImpulseScale)->Float();
    v14 = v14 * (*v10);
    return (v14 * afBaseForce);
}

void TESHavokUtilities::RemoveHavokFromSceneGraph(NiAVObject* apObject) {
    if (!apObject) {
        return;
    }
    if (apObject->GetCollisionObject()) {
        apObject->SetCollisionObject(nullptr);
    }
    if (apObject->IsNode()) {
        NiNode* pNode = static_cast<NiNode*>(apObject);
        for (uint32_t i = 0; i < pNode->GetArrayCount(); ++i) {
            RemoveHavokFromSceneGraph(pNode->GetAt(i));
        }
    }
}

// Each case returns its setting's float; materials 0 to 9, 24 and 25 have a setting and every other one returns 0.
float TESHavokUtilities::GetMaterialBuoyancy(HavokMaterialType aeMaterial) {
    switch (aeMaterial) {
        case BHK_MATERIAL_STONE: return fBuoyancyStone.Float();
        case BHK_MATERIAL_CLOTH: return fBuoyancyCloth.Float();
        case BHK_MATERIAL_DIRT: return fBuoyancyDirt.Float();
        case BHK_MATERIAL_GLASS: return fBuoyancyGlass.Float();
        case BHK_MATERIAL_GRASS: return fBuoyancyGrass.Float();
        case BHK_MATERIAL_METAL: return fBuoyancyMetal.Float();
        case BHK_MATERIAL_ORGANIC: return fBuoyancyOrganic.Float();
        case BHK_MATERIAL_SKIN: return fBuoyancySkin.Float();
        case BHK_MATERIAL_WATER: return fBuoyancyWater.Float();
        case BHK_MATERIAL_WOOD: return fBuoyancyWood.Float();
        case BHK_MATERIAL_BOTTLE: return fBuoyancyBottle.Float();
        case BHK_MATERIAL_SODACAN: return fBuoyancySodaCan.Float();
        default: return 0.0f;
    }
}

float TESHavokUtilities::GetTotalBuoyancy(bhkRigidBody* apBody) {
    HavokMaterialType eMaterial;
    float fTotalMass;
    CFilter kFilter;
    float v7 = 0.0;
    bhkShape* pShape = apBody->GetShape();
    bhkShapeCollection* pShapeCollection = pShape->FindShapeCollection();
    if (pShapeCollection) {
        int32_t iCount = 0;
        for (uint32_t aiOldKey = pShapeCollection->GetFirstKey(); aiOldKey != -1;
             aiOldKey = pShapeCollection->GetNextKey(aiOldKey)) {
            eMaterial = pShapeCollection->QSubMaterial(aiOldKey);
            v7 = TESHavokUtilities::GetMaterialBuoyancy(eMaterial) + v7;
            ++iCount;
        }
        fTotalMass = v7 / static_cast<double>(iCount);
    }
    else {
        eMaterial = static_cast<HavokMaterialType>(pShape->GetMaterial());
        fTotalMass = TESHavokUtilities::GetMaterialBuoyancy(eMaterial);
    }
    float fBuoyancyMult = 1.0;
    apBody->GetCollisionFilter(&kFilter);
    if (kFilter.QBipLayer()) {
        if (!(fTotalMass > 0.0)) {
        LABEL_12:;
            fBuoyancyMult = fBuoyancyMultBody.Float();
        }
        else {
            switch (kFilter.GetBipedPart()) {
                case P_HEAD:
                case P_BODY:
                case P_SPINE1:
                case P_SPINE2:
                case P_LUPPERARM:
                case P_LTHIGH:
                case P_RUPPERARM:
                case P_RTHIGH: {
                    goto LABEL_12;
                }
                case P_LFOREARM:
                case P_LHAND:
                case P_LCALF:
                case P_LFOOT:
                case P_RFOREARM:
                case P_RHAND:
                case P_RCALF:
                case P_RFOOT:
                case P_WING: {
                    fBuoyancyMult = fBuoyancyMultExtremity.Float();
                    break;
                }
                case P_TAIL:
                case P_SHIELD:
                case P_QUIVER:
                case P_WEAPON:
                case P_PONYTAIL: {
                    fBuoyancyMult = 0.0;
                    break;
                }
                default: {
                    break;
                }
            }
        }
    }
    return fTotalMass * fBuoyancyMult;
}

// Pushes every rigid body under apObject away from aLocation: the body's velocity gains afMagnitude, times the
// multiplier of its biped part, along the direction from aLocation to its centre of mass. A hit on a head, arm or leg
// limb raises the multiplier of that limb's parts to 1.5. Then does the same for the node's children.
void TESHavokUtilities::AddExplosionImpulse(
    NiAVObject* apObject,
    hkVector4& aLocation,
    float afMagnitude,
    HitData* apHitData
) {
    if (!apObject) {
        return;
    }
    bhkCollisionObject* pCollisionObject = bhkCollisionObject::GetbhkCollisionObject(apObject);
    if (pCollisionObject) {
        bhkWorldObject* pWorldObject = pCollisionObject->GetWorldObject();
        if (pWorldObject) {
            alignas(16) CFilter kFilter;
            BIPED_PART ePart = pWorldObject->GetCollisionFilter(&kFilter)->GetBipedPart();
            float fMult = pfExplosionMult[ePart];
            if (apHitData && apHitData->eDamageLimb != BGSBodyPart::LIMB_NONE) {
                int32_t eLimb = static_cast<int32_t>(apHitData->eDamageLimb);
                if ((eLimb == BGSBodyPart::LIMB_HEAD_1 || eLimb == BGSBodyPart::LIMB_HEAD_2) && ePart == P_HEAD) {
                    fMult = 1.5f;
                }
                if ((eLimb == BGSBodyPart::LIMB_LEFT_ARM_1 || eLimb == BGSBodyPart::LIMB_LEFT_ARM_2) &&
                    (ePart == P_LUPPERARM || ePart == P_LFOREARM || ePart == P_LHAND)) {
                    fMult = 1.5f;
                }
                if ((eLimb == BGSBodyPart::LIMB_RIGHT_ARM_1 || eLimb == BGSBodyPart::LIMB_RIGHT_ARM_2) &&
                    (ePart == P_RUPPERARM || ePart == P_RFOREARM || ePart == P_RHAND)) {
                    fMult = 1.5f;
                }
                if ((eLimb == BGSBodyPart::LIMB_LEFT_LEG_1 || eLimb == BGSBodyPart::LIMB_LEFT_LEG_2 ||
                     eLimb == BGSBodyPart::LIMB_LEFT_LEG_3) &&
                    (ePart == P_LTHIGH || ePart == P_LCALF || ePart == P_LFOOT)) {
                    fMult = 1.5f;
                }
                if ((eLimb == BGSBodyPart::LIMB_RIGHT_LEG_1 || eLimb == BGSBodyPart::LIMB_RIGHT_LEG_2 ||
                     eLimb == BGSBodyPart::LIMB_RIGHT_LEG_3) &&
                    (ePart == P_RTHIGH || ePart == P_RCALF || ePart == P_RFOOT)) {
                    fMult = 1.5f;
                }
            }
            float fImpulse = afMagnitude * fMult;
            alignas(16) hkVector4 kImpulse;
            kImpulse.getQuad();
            // Slot 60 through the vtable.
            static_cast<bhkRigidBody*>(pWorldObject)->GetCenterOfMassInWorld(kImpulse);
            kImpulse.sub4(aLocation);
            kImpulse.normalize3();
            hkpRigidBody* pBody = static_cast<hkpRigidBody*>(pWorldObject->GethkObject());
            alignas(16) hkSimdReal kScale;
            kScale.~hkSimdReal();
            ::new (&kScale) hkSimdReal(fImpulse);
            kImpulse.mul4(kScale);
            kImpulse.add4(pBody->getLinearVelocity());
            pBody->setLinearVelocity(kImpulse);
        }
    }
    NiNode* pNode = apObject->IsNode();
    if (pNode) {
        for (uint32_t i = 0; i < pNode->GetArrayCount(); ++i) {
            AddExplosionImpulse(pNode->GetAt(i), aLocation, afMagnitude, apHitData);
        }
    }
}

bool TESHavokUtilities::Should3DRagDoll(TESObjectREFR* apRef, NiAVObject* ap3D) {
    return (bool)((uint8_t)((apRef && (apRef->IsActor() && (apRef->IsDead(false) || apRef->IsKnockedOut()))) ||
                            (BSXFlags::FindFlag(ap3D, static_cast<BSXFlags::FLAGS>(72)) == 72)));
}

// Adds up, over apObject and its children, the convex vertices shapes' collision spheres and the triangle
// collections' and MOPP collections' child shapes.
void TESHavokUtilities::InspectHavokObjects(
    NiAVObject* apObject,
    int32_t& aiConvexVerticesCount,
    int32_t& aiShapeCollectionTriCount
) {
    if (apObject) {
        bhkCollisionObject* pcol = bhkCollisionObject::GetbhkCollisionObject(apObject);
        if (pcol) {
            bhkRigidBody* prb = pcol->GetRigidBody();
            if (prb) {
                hkpShape* phkShape;
                bhkShape* pshape = prb->GetShape();
                if (pshape) {
                    phkShape = pshape->Gethk();
                }
                else {
                    phkShape = nullptr;
                }
                if (phkShape) {
                    if (phkShape->getType() == HK_SHAPE_TRIANGLE_COLLECTION) {
                        const hkpShapeContainer* pContainer = static_cast<hkpShapeCollection*>(phkShape);
                        aiShapeCollectionTriCount += pContainer->getNumChildShapes();
                    }
                    else {
                        if (phkShape->getType() == HK_SHAPE_CONVEX_VERTICES) {
                            aiConvexVerticesCount +=
                                static_cast<hkpConvexVerticesShape*>(phkShape)->getNumCollisionSpheresImpl();
                        }
                        else {
                            if (phkShape->getType() == HK_SHAPE_MOPP) {
                                const hkpShapeCollection* pCollection =
                                    static_cast<hkpMoppBvTreeShape*>(phkShape)->getShapeCollection();
                                if (pCollection) {
                                    const hkpShapeContainer* pContainer = pCollection;
                                    aiShapeCollectionTriCount += pContainer->getNumChildShapes();
                                }
                            }
                        }
                    }
                }
            }
        }
        if (apObject->IsNode()) {
            uint32_t ui = 0;
            for (uint32_t i = static_cast<NiNode*>(apObject)->NiNode::GetArrayCount(); ui < i;
                 i = static_cast<NiNode*>(apObject)->NiNode::GetArrayCount()) {
                NiAVObject* v4 = static_cast<NiNode*>(apObject)->NiNode::GetAt(ui);
                TESHavokUtilities::InspectHavokObjects(v4, aiConvexVerticesCount, aiShapeCollectionTriCount);
                ++ui;
            }
        }
    }
}

// A random point in the middle tenth of the body's local box (0.45 to 0.55 of the way from min to max on each axis),
// moved into apObject's local space when the body is a bhkRigidBodyT or belongs to another object; the origin when
// apObject has no rigid body.
NiPoint3* TESHavokUtilities::GetLocalPointWithinCollision(NiPoint3* apResult, NiAVObject* apObject) {
    // kPoint is 16-aligned; NiTransform::operator* takes it.
    alignas(16) NiPoint3 kPoint = NiPoint3_ZERO;
    bhkCollisionObject* pcol = FindFirstCollisionObject(apObject);
    if (pcol) {
        bhkRigidBody* pb = pcol->GetRigidBody();
        if (pb) {
            hkAabb abb;
            pb->GetAabbLocal(abb);
            NiPoint3 min;
            HK2NI(min, abb.m_min);
            NiPoint3 max;
            HK2NI(max, abb.m_max);
            NiPoint3 kMinHigh = 0.45f * min;
            alignas(16) NiPoint3 high = 0.55f * max + kMinHigh;
            NiPoint3 kMinLow = 0.55f * min;
            alignas(16) NiPoint3 kMaxLow = 0.45f * max;
            NiPoint3 low = kMaxLow + kMinLow;
            float z = RandomFloat(low.z, high.z);
            float y = RandomFloat(low.y, high.y);
            float x = RandomFloat(low.x, high.x);
            kPoint = NiPoint3(x, y, z);
            if (NiObject::IsKindOf_0(&bhkRigidBodyT::ms_RTTI, pb) || pcol->m_pkSceneObject != apObject) {
                hkTransform hktrans;
                pb->bhkRigidBody::GetTransform(hktrans);
                NiTransform nitrans;
                HK2NI(nitrans, hktrans);
                nitrans.m_fScale = 1.0f;
                NiTransform invtrans;
                apObject->GetWorldTransform().Invert(invtrans);
                kPoint = invtrans * nitrans * kPoint;
            }
        }
    }
    *apResult = kPoint;
    return apResult;
}

TESObjectREFR* TESHavokUtilities::FindCollidableRef_009C69E0(const hkpCollidable* apCol) {
    TESObjectREFR* pref = nullptr;
    NiAVObject* pav = bhkUtilFunctions::GetNiAVObject(apCol);
    if (pav) {
        return TESObjectREFR::FindReferenceFor3D(pav);
    }
    return pref;
}

hkpShapeCollection* bhkShapeCollection::operator*() const {
    if (this) {
        return static_cast<hkpShapeCollection*>(const_cast<bhkShapeCollection*>(this)->Gethk());
    }
    else {
        return nullptr;
    }
}

// The container base is taken without a null test, as the game does, so it goes through a reference.
uint32_t bhkShapeCollection::GetFirstKey() {
    const hkpShapeContainer& kContainer = *operator*();
    return kContainer.getFirstKey();
}

uint32_t bhkShapeCollection::GetNextKey(uint32_t aiOldKey) {
    const hkpShapeContainer& kContainer = *operator*();
    return kContainer.getNextKey(aiOldKey);
}

void NiAVObject::SetCollisionObject(NiCollisionObject* pkCollisionObject) {
    m_spCollisionObject = static_cast<bhkNiCollisionObject*>(pkCollisionObject);
    if (m_spCollisionObject.operator->()) {
        NiCollisionObject* pkCollision = m_spCollisionObject.operator->();
        if (pkCollision->GetSceneGraphObject() != this) {
            pkCollision = m_spCollisionObject.operator->();
            pkCollision->SetSceneGraphObject(this);
        }
    }
}

// The SDK's inline body; the child container's getChild is a trivial accessor.
const hkpShapeCollection* hkpMoppBvTreeShape::getShapeCollection() const {
    return static_cast<const hkpShapeCollection*>(m_child.m_childShape);
}
