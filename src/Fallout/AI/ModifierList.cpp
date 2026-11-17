#include "Fallout/AI/ModifierList.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueInfo.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"

// the modifier array is the first member of FastModifierCollection
static Modifier* const* FastModifierCollection_GetModifiers(FastModifierCollection* apCollection) {
	return *reinterpret_cast<Modifier* const* const*>(apCollection);
}

Modifier::Modifier() : cKey(0), fValue(0.0f) {}

Modifier::Modifier(char acKey, float afValue) : cKey(static_cast<uint8_t>(acKey)), fValue(afValue) {}

Modifier::Modifier(Modifier* apSource) : cKey(apSource->cKey), fValue(apSource->GetValue()) {}

char Modifier::GetKey() {
	return cKey;
}

float Modifier::GetValue() {
	return fValue;
}

void Modifier::SetValue(float a1) {
	fValue = a1;
}

ModifierList::ModifierList()
	// an empty list is a head node with no item
	: BSSimpleList<Modifier*>(nullptr) {
	pFastModifiers = nullptr;
	bZeroValuesAllowed = false;
}

ModifierList::~ModifierList() {
	DeleteAllModifiers();
}

// The key is passed sign-extended to GetActorValueInfo and zero-extended to AddFastModifier.
void ModifierList::AddModifier(Modifier* apModifier) {
	if (apModifier) {
		char cKey = static_cast<char>(apModifier->GetKey());
		ActorValueInfo* pInfo = ActorValue::GetActorValueInfo(static_cast<ActorValue::Index>(cKey));
		if (pInfo && (pInfo->uFlags & 1)) {
			AddFastModifier(cKey, apModifier);
		} else {
			AddHead(apModifier);
		}
	}
}

void ModifierList::AddFastModifier(char acKey, Modifier* apNew) {
	if (apNew || pFastModifiers) {
		if (pFastModifiers) {
			pFastModifiers->DeleteModifier(acKey);
		}
		if (apNew) {
			if (apNew->fValue != 0.0 || bZeroValuesAllowed) {
				if (!pFastModifiers) {
					FastModifierCollection* v3;
					FastModifierCollection* apMemory =
						static_cast<FastModifierCollection*>(ourMalloc(sizeof(FastModifierCollection)));
					if (apMemory) {
						v3 = ::new (apMemory) FastModifierCollection;
					} else {
						v3 = 0;
					}
					pFastModifiers = v3;
				}
				pFastModifiers->pModifiers[FastModifierCollection::uiAVMapping[acKey]] = apNew;
			}
		}
	}
}

void ModifierList::DeleteModifier(Modifier* apModifier) {
	if (apModifier) {
		ActorValueInfo* pInfo =
			ActorValue::GetActorValueInfo(static_cast<ActorValue::Index>(static_cast<char>(apModifier->GetKey())));
		if (pInfo && (pInfo->uFlags & 1) && pFastModifiers) {
			pFastModifiers->DeleteModifier(static_cast<char>(apModifier->GetKey()));
		} else {
			Remove(apModifier);
			ourFree(apModifier);
		}
	}
}

void ModifierList::DeleteAllModifiers() {
	FastModifierCollection* pFastModifiers;
	Modifier* v3;
	while (1) {
		v3 = m_item;
		if (!v3) {
			break;
		}
		ourFree(v3);
		BSSimpleList<Modifier*>::RemoveHead();
	}
	if (this->pFastModifiers) {
		pFastModifiers = this->pFastModifiers;
		if (pFastModifiers) {
			delete pFastModifiers;
		}
		this->pFastModifiers = nullptr;
	}
}

// Five bytes per modifier plus a two-byte header, counted over the list and the fast collection. The per-modifier size
// is kept in a local and tested for 0 before use; the value is 5, so the return-0 block is dead code.
uint16_t ModifierList::GetSaveSize() {
	uint16_t usSize = 0;
	uint16_t usModifierSize = 5;
	if (!usModifierSize)
		return 0;
	int32_t iItems = ItemsInList();
	usSize = usSize + usModifierSize * iItems + 2;
	int32_t iFastCount = pFastModifiers ? FastModifierCollection::uiModifierCount : 0;
	usSize = usSize + usModifierSize * iFastCount;
	return usSize;
}

float ModifierList::GetModifier(char acKey) {
	bool abExists = false;
	return GetModifier(acKey, abExists);
}

float ModifierList::GetModifier(char acKey, bool& abExists) {
	Modifier* pModifier = GetModifierItem(acKey);
	abExists = pModifier != nullptr;
	if (pModifier)
		return pModifier->GetValue();
	return 0.0f;
}

Modifier* ModifierList::GetModifierItem(char acKey) {
	if (pFastModifiers) {
		// actor values with a fast modifier map to an index in the
		// collection's modifier array
		const int32_t iMapping = FastModifierCollection::uiAVMapping[static_cast<int8_t>(acKey)];
		// indexes below 77 point into the modifier array
		if (iMapping < 77)
			return FastModifierCollection_GetModifiers(pFastModifiers)[iMapping];
	}

	// walk the list nodes until the first empty item
	Modifier* pResult = nullptr;
	for (BSSimpleList<Modifier*>* pNode = this; pNode; pNode = pNode->GetNext()) {
		Modifier* pItem = pNode->GetItem();
		if (!pItem || pResult)
			break;
		if (static_cast<int8_t>(pItem->GetKey()) == static_cast<int8_t>(acKey))
			pResult = pItem;
	}
	return pResult;
}

void ModifierList::operator=(const ModifierList& arList) {
	DeleteAllModifiers();
	if (&arList) {
		if (arList.pFastModifiers) {
			FastModifierCollection* v3;
			FastModifierCollection* v6 =
				static_cast<FastModifierCollection*>(ourMalloc(sizeof(FastModifierCollection)));
			if (v6) {
				v3 = ::new (v6) FastModifierCollection(arList.pFastModifiers);
			} else {
				v3 = 0;
			}
			pFastModifiers = v3;
		}
		for (const ModifierList* plist = &arList; plist && plist->m_item;
			plist = static_cast<ModifierList*>(plist->m_pkNext)) {
			Modifier* v2;
			Modifier* pitem = plist->m_item;
			Modifier* apMemory = static_cast<Modifier*>(ourMalloc(sizeof(Modifier)));
			if (apMemory) {
				v2 = ::new (apMemory) Modifier(pitem);
			} else {
				v2 = 0;
			}
			AddModifier(v2);
		}
	}
}

void ModifierList::SetZeroValuesAllowed(bool abZeroValuesAllowed) {
	bZeroValuesAllowed = abZeroValuesAllowed;
}

void ModifierList::SetModifier(char acKey, float afValue) {
	Modifier* v3;
	Modifier* pModifier = GetModifierItem(acKey);
	if (pModifier) {
		if ((afValue != 0.0) || bZeroValuesAllowed) {
			pModifier->SetValue(afValue);
		} else {
			DeleteModifier(pModifier);
		}
	} else {
		if ((afValue != 0.0) || bZeroValuesAllowed) {
			pModifier = static_cast<Modifier*>(ourMalloc(sizeof(Modifier)));
			if (pModifier) {
				v3 = ::new (pModifier) Modifier(acKey, afValue);
			} else {
				v3 = nullptr;
			}
			AddModifier(v3);
		}
	}
}

void ModifierList::AdjustModifier(char acKey, float afValue, ClampStyle aeClampStyle) {
	float fValue;
	Modifier* v5;
	void* v7;
	float v8;
	Modifier* pModifier = GetModifierItem(acKey);
	if (pModifier) {
		fValue = pModifier->GetValue();
		v8 = (float)ModifierList::ModifyAndClamp(fValue, afValue, aeClampStyle);
		pModifier->SetValue(v8);
		if ((pModifier->GetValue() == 0.0) && (!bZeroValuesAllowed)) {
			DeleteModifier(pModifier);
		}
	} else {
		if ((afValue != 0.0) || bZeroValuesAllowed) {
			v7 = ourMalloc(sizeof(Modifier));
			if (v7) {
				v5 = ::new (v7) Modifier((uint8_t)acKey, afValue);
			} else {
				v5 = nullptr;
			}
			AddModifier(v5);
		}
	}
}

void ModifierList::Revert() {
	DeleteAllModifiers();
}

bool ModifierList::WriteModifier(Modifier* apModifier) {
	if (!apModifier) {
		return (bool)0;
	}
	int8_t v3 = (int8_t)apModifier->GetKey();
	float flt004 = apModifier->GetValue();
	pSaveLoadGameOLD->SaveGameDataOLD(&v3, 1);
	pSaveLoadGameOLD->SaveGameDataOLD(&flt004, 4);
	return (bool)1;
}

void ModifierList::RevertBGS(BGSLoadFormBuffer* apBuffer) {
	DeleteAllModifiers();
}

void ModifierList::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
	uint32_t uiCount = 0;
	uint32_t uiPosition = apSaveGameBuffer->StartVariableSizedValue();
	for (BSSimpleList<Modifier*>* pNode = this; pNode;) {
		Modifier* pModifier;
		pModifier = pNode->GetItem();
		if (pModifier) {
			apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&pModifier->cKey), 1, 0);
			apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&pModifier->fValue), 4, 0);
			++uiCount;
		}
		pNode = pNode->GetNext();
	}
	if (pFastModifiers) {
		for (uint32_t i = 0; i < static_cast<uint32_t>(FastModifierCollection::uiModifierCount); ++i) {
			if (pFastModifiers->pModifiers[i]) {
				apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&pFastModifiers->pModifiers[i]->cKey), 1, 0);
				apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&pFastModifiers->pModifiers[i]->fValue), 4, 0);
				++uiCount;
			}
		}
	}
	apSaveGameBuffer->SaveVariableSizedValue(uiCount, uiPosition);
}

void ModifierList::LoadGameTES() {
	char v4;
	float v5;
	uint16_t usCount;
	pSaveLoadGameOLD->LoadGameDataOLD(&usCount, 2u);
	for (uint32_t i = 0; i < usCount; ++i) {
		Modifier* v1;
		pSaveLoadGameOLD->LoadGameDataOLD(&v4, 1u);
		pSaveLoadGameOLD->LoadGameDataOLD(&v5, 4u);
		Modifier* apMemory = static_cast<Modifier*>(ourMalloc(sizeof(Modifier)));
		if (apMemory) {
			v1 = ::new (apMemory) Modifier(v4, v5);
		} else {
			v1 = 0;
		}
		AddModifier(v1);
	}
}

void ModifierList::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	uint32_t uiCount = apLoadGameBuffer->LoadVariableSizedValue();
	for (uint32_t i = 0; i < uiCount; ++i) {
		void* pMemory = ourMalloc(sizeof(Modifier));
		Modifier* pModifier = pMemory ? ::new (pMemory) Modifier : nullptr;
		apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&pModifier->cKey), 1);
		apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&pModifier->fValue), 4);
		AddModifier(pModifier);
	}
}

void ModifierList::SaveGame_009378D0() {
	uint16_t usCount = 0;
	uint16_t* pCount = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
	pSaveLoadGameOLD->SaveGameDataOLD(&usCount, 2u);
	for (BSSimpleList<Modifier*>* pNode = this; pNode && pNode->m_item; pNode = pNode->m_pkNext) {
		if (WriteModifier(pNode->m_item)) {
			++usCount;
		}
	}
	int32_t iFastCount = pFastModifiers != 0 ? FastModifierCollection::uiModifierCount : 0;
	if (iFastCount) {
		for (int32_t i = 0; i < iFastCount; ++i) {
			WriteModifier(pFastModifiers->pModifiers[i]);
			++usCount;
		}
	}
	*pCount = usCount;
}

float ModifierList::ModifyAndClamp(float afValue, float afModifier, ClampStyle aeClampStyle) {
	float fTotal = afValue + afModifier;
	if (aeClampStyle == LE_ZERO) {
		if (fTotal > 0.0) {
			fTotal = 0.0f;
		}
	} else if (aeClampStyle == GE_ZERO && fTotal < 0.0) {
		fTotal = 0.0f;
	}
	return fTotal;
}

// An empty slot for each fast actor value; the array comes from ourMalloc, which the destructor frees with ourFree.
FastModifierCollection::FastModifierCollection() : pModifiers(nullptr) {
	CheckInitializeMapping();
	if (uiModifierCount) {
		pModifiers = static_cast<Modifier**>(ourMalloc(sizeof(Modifier*) * static_cast<uint32_t>(uiModifierCount)));
		for (int32_t i = 0; i < uiModifierCount; ++i) {
			pModifiers[i] = nullptr;
		}
	}
}

// Copies a2's modifiers; the array and each copy come from ourMalloc, which the destructor frees with ourFree.
FastModifierCollection::FastModifierCollection(FastModifierCollection* a2) : pModifiers(nullptr) {
	if (a2->pModifiers) {
		if (uiModifierCount) {
			pModifiers = static_cast<Modifier**>(ourMalloc(sizeof(Modifier*) * static_cast<uint32_t>(uiModifierCount)));
		}
		for (int32_t i = 0; i < uiModifierCount; ++i) {
			pModifiers[i] =
				a2->pModifiers[i] ? ::new (ourMalloc(sizeof(Modifier))) Modifier(a2->pModifiers[i]) : nullptr;
		}
	}
}

FastModifierCollection::~FastModifierCollection() {
	if (uiModifierCount && pModifiers) {
		for (int32_t i = 0; i < uiModifierCount; ++i) {
			if (pModifiers[i]) {
				ourFree(pModifiers[i]);
			}
		}
		ourFree(pModifiers);
		pModifiers = nullptr;
	}
}

void FastModifierCollection::DeleteModifier(uint32_t auiIndex) {
	if (auiIndex <= 76) {
		if (pModifiers) {
			if (FastModifierCollection::uiAVMapping[auiIndex] < 77) {
				if (pModifiers[FastModifierCollection::uiAVMapping[auiIndex]]) {
					ourFree(pModifiers[FastModifierCollection::uiAVMapping[auiIndex]]);
					pModifiers[FastModifierCollection::uiAVMapping[auiIndex]] = 0;
				}
			}
		}
	}
}

void FastModifierCollection::CheckInitializeMapping() {
	if (!bInitialized) {
		int32_t iCount = 0;
		for (int32_t i = 0; i < ActorValue::ACTOR_VALUE_COUNT; ++i) {
			ActorValueInfo* pInfo = ActorValue::GetActorValueInfo(static_cast<ActorValue::Index>(i));
			if (pInfo && (pInfo->uFlags & 1)) {
				FastModifierCollection::uiAVMapping[i] = iCount++;
			} else {
				FastModifierCollection::uiAVMapping[i] = ActorValue::ACTOR_VALUE_COUNT;
			}
		}
		uiModifierCount = iCount;
		bInitialized = 1;
	}
}
