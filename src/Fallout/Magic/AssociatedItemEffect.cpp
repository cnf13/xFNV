#include "Fallout/Magic/AssociatedItemEffect.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicCaster.h"

AssociatedItemEffect::AssociatedItemEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect)
	: ActiveEffect(apCaster, apSpell, apEffect), pAssociatedItem(apEffect->pEffectSetting->GetAssociatedItem()) {}

AssociatedItemEffect::~AssociatedItemEffect() {}

// Copies the effect: a new instance built from the same caster, spell and effect item, then the field copy.
ActiveEffect* AssociatedItemEffect::Copy() {
	ActiveEffect* v2;
	void* v4 = ourMalloc(sizeof(AssociatedItemEffect));
	if (v4) {
		v2 = ::new (v4) AssociatedItemEffect(pCaster, pSpell, pEffect);
	} else {
		v2 = nullptr;
	}
	ActiveEffect* pThis = this;
	pThis->CopyData(v2);
	return v2;
}

uint16_t AssociatedItemEffect::GetSaveSize(Actor* apActor) {
	return (uint16_t)(((int16_t)ActiveEffect::GetSaveSize(apActor)) + 4);
}

// The associated item is saved as its form ID.
void AssociatedItemEffect::SaveGame(Actor* apActor) {
	ActiveEffect::SaveGame(apActor);
	uint32_t uiItemID = 0;
	if (pAssociatedItem)
		uiItemID = pAssociatedItem->GetFormID();
	pSaveLoadGameOLD->SaveNumericID(&uiItemID, 4);
}

// The item field holds the loaded form ID until the load game resolves it.
void AssociatedItemEffect::LoadGame(Actor* apActor) {
	ActiveEffect::LoadGame(apActor);
	uint32_t uiItemID;
	pSaveLoadGameOLD->LoadNumericID(&uiItemID, 4);
	pAssociatedItem = reinterpret_cast<TESForm*>(static_cast<uintptr_t>(uiItemID));
}

// The base's load-game init, then the item field, which LoadGame left holding the saved form ID, is resolved to its
// form.
void AssociatedItemEffect::InitLoadGame(Actor* apActor) {
	ActiveEffect::InitLoadGame(apActor);
	TESForm* pItem = pAssociatedItem;
	if (pItem) {
		pAssociatedItem = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pItem)));
	}
}

void AssociatedItemEffect::CopyData(ActiveEffect* apDestination) {
	ActiveEffect::CopyData(apDestination);
	AssociatedItemEffect* v3 = dynamic_cast<AssociatedItemEffect*>(apDestination);
	if (v3) {
		v3->pAssociatedItem = pAssociatedItem;
	}
}
