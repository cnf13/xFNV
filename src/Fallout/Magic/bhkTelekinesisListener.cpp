#include "Fallout/Magic/bhkTelekinesisListener.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkEntity.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSShader/BSFadeNode.h"
#include "Fallout Shared/Havok/FOCollisionListener.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESImageSpaceModifier.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Magic/ReanimateEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityActivationListener.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "SDK/MSVC/rtti.h"

bhkTelekinesisListener::bhkTelekinesisListener(TESObjectREFR* apRef, bhkRigidBody* apBody, uint32_t aiDefaultFilter)
  : pRef(apRef), pBody(apBody), DefaultFilter(aiDefaultFilter), bCanFinish(false) {
    pBody->AddContactListener(this);
    pBody->AddEntityListener(static_cast<hkpEntityListener*>(this));
    pBody->AddEntityActivationListener(static_cast<hkpEntityActivationListener*>(this));
}

bhkTelekinesisListener::~bhkTelekinesisListener() {}

// Removes the listener from the entity, then deletes it.
void bhkTelekinesisListener::entityDeletedCallback(hkpEntity* apEntity) {
    Remove(this, apEntity);
    delete this;
}

// Called through the hkpEntityActivationListener vtable at +0x8.
void bhkTelekinesisListener::entityDeactivatedCallback(hkpEntity* apEntity) {
    if (bCanFinish) {
        Finish(apEntity);
    }
}

void bhkTelekinesisListener::Finish(hkpEntity* apEntity) {
    if (pRef) {
        NiAVObject* pav = pRef->Get3D();
        BSFadeNode* pavlod = dynamic_cast<BSFadeNode*>(pav);
        if (pavlod) {
            pavlod->SetLODMultType(LOD_MULT_ITEMS);
        }
    }
    pBody->SetCollisionFilter(DefaultFilter);
    bhkTelekinesisListener::Remove(this, apEntity);
    if (this) {
        delete this;
    }
}

void bhkTelekinesisListener::Remove(bhkTelekinesisListener* apListener, hkpEntity* apEntity) {
    apEntity->removeContactListener(apListener);
    apEntity->removeEntityListener(static_cast<hkpEntityListener*>(apListener));
    apEntity->removeEntityActivationListener(static_cast<hkpEntityActivationListener*>(apListener));
}

void bhkTelekinesisListener::AllowToFinish(bhkRigidBody* apBody) {
    hkpRigidBody* pRigidBody = **apBody;
    for (int32_t i = 0; i < pRigidBody->getExtendedListeners()->m_activationListeners.m_size; ++i) {
        hkpEntity::ExtendedListeners* pExtendedListeners = pRigidBody->getExtendedListeners();
        hkpEntityActivationListener* pListener = pExtendedListeners->m_activationListeners[i];
        bhkTelekinesisListener* ptl = dynamic_cast<bhkTelekinesisListener*>(pListener);
        if (ptl) {
            ptl->Finish(pRigidBody);
            i = 0;
        }
    }
}

void* hkpEntity::ExtendedListeners::operator new(size_t aiNumBytes) noexcept {
    return hkMemoryRouter::getInstance().m_heap->blockAlloc(static_cast<int32_t>(aiNumBytes));
}

void hkpEntity::ExtendedListeners::operator delete(void* apMemory) {
    if (apMemory) {
        hkMemoryRouter::getInstance().heap().blockFree(apMemory, static_cast<int32_t>(sizeof(ExtendedListeners)));
    }
}
