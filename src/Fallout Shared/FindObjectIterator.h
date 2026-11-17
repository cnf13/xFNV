#pragma once

#include "Fallout Shared/InventoryIterator.h"
#include "Fallout Shared/LoadedReferenceMap.h"
#include "Fallout Shared/TESForm.h"
#include "Reconstruction.hpp"

class TESBoundObject;
class TESObjectREFR;

// Walks the loaded references of one form type (or the ones of one form), and the inventories of the containers
// among them, looking for a form. The PDB's source module is BGSLoadedReferenceCollection.cpp.
class FindObjectIterator {
public:
	// The hybrid's implicit default constructor (no zero-parameter Construct).
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
	// The PDB types it as the map's opaque iterator (void*); it points at pCurrentMap's items.
	NiTMapItem<uint32_t, TESObjectREFR*>* MapIterator;
	InventoryIterator InvIterator;
	uint8_t cMapIndex;
};
