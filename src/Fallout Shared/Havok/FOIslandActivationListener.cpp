#include "Fallout Shared/Havok/FOIslandActivationListener.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkEntity.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/BSXFlags.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESObjectACTI.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/AILinearTaskThreadManager.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/World/Listener/hkpIslandActivationListener.h"
#include "Havok/Source/Physics/Dynamics/World/hkpSimulationIsland.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"

FOIslandActivationListener::FOIslandActivationListener() {}

FOIslandActivationListener::~FOIslandActivationListener() {}

void FOIslandActivationListener::islandActivatedCallback(hkpSimulationIsland* apIsland) {
    if (apIsland) {
        const hkArray<hkpEntity*, hkContainerHeapAllocator>* pentityArray =
            (const hkArray<hkpEntity*, hkContainerHeapAllocator>*)&apIsland->m_entities;
        int32_t iarraySize = pentityArray->m_size;
        for (int32_t ientityIdx = 0; ientityIdx < iarraySize; ++ientityIdx) {
            TESObjectREFR* pobjectRef;
            NiAVObject* pavObject;
            bhkNiCollisionObject* pniColObject =
                bhkNiCollisionObject::Getbhk(pentityArray->operator[]<hkpEntity*>(ientityIdx));
            if (pniColObject) {
                pavObject = pniColObject->m_pkSceneObject;
            }
            else {
                pavObject = nullptr;
            }
            if (pavObject) {
                pobjectRef = TESObjectREFR::FindReferenceFor3D(pavObject);
            }
            else {
                pobjectRef = nullptr;
            }
            if ((pobjectRef) && (!pobjectRef->IsActor())) {
                NiNode* pdynamicnode;
                pobjectRef->AddChange(4);
                TESObjectCELL* pcell = pobjectRef->GetParentCell();
                if (pcell) {
                    pdynamicnode = static_cast<NiNode*>(pcell->GetDynamicNode());
                }
                else {
                    pdynamicnode = nullptr;
                }
                if (pdynamicnode) {
                    if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                        TESObjectCELL::QueueAttachChild(pobjectRef, pdynamicnode);
                    }
                    else {
                        // abFirstAvail is passed as true.
                        pdynamicnode->AttachChild(pobjectRef->Get3D(), true);
                    }
                }
                if (pobjectRef->GetMotionBlur()) {
                    BSShaderManager::AddMotionBluredObject();
                }
            }
        }
    }
}

// The island's entities went to sleep: a projectile is told so; a non-actor reference without a model swap, and not
// a complex but unarticulated model, goes back into its cell's 3D (else a non-actor's lighting is updated); ash
// piles are fixed in place; a large-debris rigid body of a non-actor is made keyframed and its scene object is moved
// to the body's transform.
void FOIslandActivationListener::islandDeactivatedCallback(hkpSimulationIsland* apIsland) {
    if (!apIsland) {
        return;
    }
    const hkArray<hkpEntity*>* pentityArray = &apIsland->getEntities();
    int32_t iarraySize = pentityArray->m_size;
    for (int32_t ientityIdx = 0; ientityIdx < iarraySize; ++ientityIdx) {
        hkpEntity* pentity = pentityArray->operator[]<hkpEntity*>(ientityIdx);
        bhkNiCollisionObject* pniColObject = bhkNiCollisionObject::Getbhk(pentity);
        NiAVObject* pavObject = pniColObject ? pniColObject->m_pkSceneObject : nullptr;
        TESObjectREFR* pobjectRef = pavObject ? TESObjectREFR::FindReferenceFor3D(pavObject) : nullptr;
        TESModel* pmodelSwap = pobjectRef ? pobjectRef->m_Extra.GetModelSwap() : nullptr;
        bool bComplex = pobjectRef && BSXFlags::FindFlag(pobjectRef->Get3D(), BSXFlags::COMPLEX);
        bool bArticulated = pobjectRef && BSXFlags::FindFlag(pobjectRef->Get3D(), BSXFlags::ARTICULATED);
        if (pobjectRef) {
            if (pobjectRef->IsProjectile()) {
                // The source's downcast: IsProjectile said the reference is a projectile.
                static_cast<Projectile*>(pobjectRef)->ReportHavokDeactivation();
            }
            if (pobjectRef->IsActor() || pmodelSwap || (bComplex && !bArticulated)) {
                if (!pobjectRef->IsActor()) {
                    NiNode* p3D = pobjectRef->Get3D();
                    BSShaderManager::GetShadowSceneNode(0)->UpdateObjectLighting(p3D, false);
                }
            }
            else {
                TESObjectCELL* pcell = pobjectRef->GetParentCell();
                if (pcell) {
                    pcell->AttachReference3D(pobjectRef, false);
                }
                if (pobjectRef->GetMotionBlur()) {
                    BSShaderManager::ResetMotionBluredObject();
                }
            }
            if (pobjectRef->GetObjectReference() == pAshPile1 || pobjectRef->GetObjectReference() == pAshPile2) {
                bhkWorld::SetMotion(pavObject, hkpMotion::MOTION_FIXED, true, false, true);
            }
        }
        alignas(16) CFilter kFilter;
        kFilter.iFilter = pentity->getCollidable()->getCollisionFilterInfo();
        if (kFilter.GetLayer() == L_DEBRIS_LARGE && pobjectRef && !pobjectRef->IsActor()) {
            bhkEntity* pbody = bhkEntity::Getbhk(pentity);
            if (pbody && NiObject::IsKindOf_0(&bhkRigidBody::ms_RTTI, pbody)) {
                bhkRigidBody* pRigidBody = static_cast<bhkRigidBody*>(pbody);
                pRigidBody->SetMotionType(hkpMotion::MOTION_KEYFRAMED);
                bhkNiCollisionObject* pcol = bhkNiCollisionObject::Getbhk(pentity);
                if (pcol && !pcol->QSetLocal()) {
                    pcol->SetSetLocal(true);
                    hkTransform hktransf;
                    NiTransform nitransf;
                    pRigidBody->GetTransform(hktransf);
                    HK2NI(nitransf, hktransf);
                    pcol->SetTransform(nitransf);
                    pcol->SetSetLocal(false);
                }
            }
        }
    }
}

// An active entity's non-actor reference gets its 3D attached under its cell's dynamic node (queued when 3D tasks
// are queued) and is added to the motion-blurred objects when it wants motion blur. An inactive large-debris body of
// a non-actor reference is made keyframed.
void FOIslandActivationListener::entityAddedCallback(hkpEntity* apEntity) {
    if (!apEntity) {
        return;
    }
    if (apEntity->isActive()) {
        bhkNiCollisionObject* pColObj = bhkNiCollisionObject::Getbhk(apEntity);
        NiAVObject* pSceneObj = pColObj ? pColObj->GetSceneGraphObject() : nullptr;
        TESObjectREFR* pRef = pSceneObj ? TESObjectREFR::FindReferenceFor3D(pSceneObj) : nullptr;
        if (!pRef || pRef->IsActor()) {
            return;
        }
        TESObjectCELL* pCell = pRef->GetParentCell();
        NiNode* pDynamicNode = pCell ? static_cast<NiNode*>(pCell->GetDynamicNode()) : nullptr;
        if (pDynamicNode) {
            if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                TESObjectCELL::QueueAttachChild(pRef, pDynamicNode);
            }
            else {
                pDynamicNode->AttachChild(pRef->Get3D(), true);
            }
        }
        if (pRef->GetMotionBlur()) {
            BSShaderManager::AddMotionBluredObject();
        }
        return;
    }
    CFilter kFilter;
    if (FNVD_RECONSTRUCT(CFilter)(&kFilter, apEntity->getCollidable()->getCollisionFilterInfo())->GetLayer() !=
        L_DEBRIS_LARGE) {
        return;
    }
    bhkNiCollisionObject* pColObj = bhkNiCollisionObject::Getbhk(apEntity);
    NiAVObject* pSceneObj = pColObj ? pColObj->GetSceneGraphObject() : nullptr;
    TESObjectREFR* pRef = pSceneObj ? TESObjectREFR::FindReferenceFor3D(pSceneObj) : nullptr;
    if (!pRef || pRef->IsActor()) {
        return;
    }
    bhkEntity* pEntity = bhkEntity::Getbhk(apEntity);
    if (pEntity && NiObject::IsKindOf_0(&bhkRigidBody::ms_RTTI, pEntity)) {
        static_cast<bhkRigidBody*>(pEntity)->SetMotionType(hkpMotion::MOTION_KEYFRAMED);
    }
}

void FOIslandActivationListener::AddToWorld(hkpWorld* apWorld) {
    if (!apWorld) {
        return;
    }
    apWorld->addIslandActivationListener(this);
    apWorld->addEntityListener(this);
}

void FOIslandActivationListener::RemoveFromWorld(hkpWorld* apWorld) {
    if (!apWorld) {
        return;
    }
    apWorld->removeIslandActivationListener(this);
    apWorld->removeEntityListener(this);
}

void FOIslandActivationListener::operator delete(void* apMemory) {
    ourFree(apMemory);
}

hkpIslandActivationListener::hkpIslandActivationListener() {}

hkpIslandActivationListener::~hkpIslandActivationListener() {}

void hkpIslandActivationListener::operator delete(void* apMemory) {
    ourFree(apMemory);
}
