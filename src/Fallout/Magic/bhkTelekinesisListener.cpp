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
#include "Reconstruction.hpp"
#include "SDK/MSVC/rtti.h"

// The exe calls it thiscall and its class is not known yet: the object comes first, then the unused edx. Returns
// nothing: eax is what hkSmallArray's destructor (0x807D60) left.
void DestroyPointerSmallArray(void* apThis, [[maybe_unused]] void* apEdx) {
	static_cast<hkSmallArray<void*>*>(apThis)->~hkSmallArray<void*>();
}

bhkTelekinesisListener::bhkTelekinesisListener(TESObjectREFR* a1, bhkRigidBody* a2, uint32_t a3)
	: pRef(a1), pBody(a2), DefaultFilter(a3), bCanFinish(false) {
	pBody->AddContactListener(this);
	pBody->AddEntityListener(static_cast<hkpEntityListener*>(this));
	pBody->AddEntityActivationListener(static_cast<hkpEntityActivationListener*>(this));
}

bhkTelekinesisListener::~bhkTelekinesisListener() {}

// Called through the hkpEntityListener vtable at +0x4: removes the listener from the entity, then deletes it through
// the primary vtable's deleting destructor (slot 3, flag 1).
void bhkTelekinesisListener::entityDeletedCallback(hkpEntity* apEntity) {
	Remove(this, apEntity);
	delete this;
}

// Called through the hkpEntityActivationListener vtable at +0x8.
void bhkTelekinesisListener::entityDeactivatedCallback(hkpEntity* apEntity) {
	if (bCanFinish)
		Finish(apEntity);
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
	if (apListener) {
		apEntity->removeEntityListener(static_cast<hkpEntityListener*>(apListener));
	} else {
		apEntity->removeEntityListener(nullptr);
	}
	if (apListener) {
		apEntity->removeEntityActivationListener(static_cast<hkpEntityActivationListener*>(apListener));
	} else {
		apEntity->removeEntityActivationListener(nullptr);
	}
}

// The exe reads each listener through 0x629AB0, its out-of-line copy of NiTSet::GetAt (inline in NiTSet.inl), a
// transparent function: the inline operator[] stands for it.
void bhkTelekinesisListener::AllowToFinish(bhkRigidBody* apBody) {
	hkpRigidBody* a2 = apBody->operator*();
	int32_t i_2 = 0;
	for (hkpEntity::ExtendedListeners* i = a2->getExtendedListeners(); i_2 < i->m_activationListeners.m_size;
		i = a2->getExtendedListeners()) {
		hkpEntity::ExtendedListeners* ExtendedListeners = a2->getExtendedListeners();
		hkpEntityActivationListener* pListener = ExtendedListeners->m_activationListeners[i_2];
		bhkTelekinesisListener* ptl = dynamic_cast<bhkTelekinesisListener*>(pListener);
		if (ptl) {
			ptl->Finish(a2);
			i_2 = 0;
		}
		++i_2;
	}
}

// 0x807DF0 is the exe's this-adjusting thunk (hkpEntityListener at +0x4) to DeletingDestruct (0x8078E0),
// registered there; src has no registration form for an adjustor thunk.
void* hkpEntity::ExtendedListeners::operator new(size_t aiNumBytes) noexcept {
	return hkMemoryRouter::getInstance().m_heap->blockAlloc(static_cast<int32_t>(aiNumBytes));
}

void hkpEntity::ExtendedListeners::operator delete(void* apMemory) {
	if (apMemory) {
		hkMemoryRouter::getInstance().heap().blockFree(apMemory, static_cast<int32_t>(sizeof(ExtendedListeners)));
	}
}
