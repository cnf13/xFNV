#pragma once

#include "Fallout Shared/InventoryIterator.h"
#include "Fallout Shared/LoadedReferenceMap.h"
#include "Fallout Shared/TESForm.h"

class TESBoundObject;
class TESObjectREFR;

// Walks the loaded references of one form type (or the ones of one form), and the inventories of the containers among
// them, looking for a form.
class FindObjectIterator {
public:
	FindObjectIterator() = default;
	FindObjectIterator(ENUM_FORM_ID aeFormType, TESObjectREFR* apActivatingRef);
	FindObjectIterator(TESForm* apForm, TESObjectREFR* apRef);
	~FindObjectIterator();

	void Reset();

	ENUM_FORM_ID eFormType;
	// The form searched for; null when searching by type only.
	TESBoundObject* pBoundObject;
	// The reference the search is made for (the activating actor).
	TESObjectREFR* pActivatingRef;
	// The collection's map of the loaded references of eFormType.
	LoadedReferenceMap* pCurrentMap;
	TESObjectREFR* pCurrentRef;
	InventoryChanges* pCurrentInventory;
	// Points at pCurrentMap's items.
	NiTMapItem<uint32_t, TESObjectREFR*>* MapIterator;
	InventoryIterator InvIterator;
	uint8_t cMapIndex;
};
