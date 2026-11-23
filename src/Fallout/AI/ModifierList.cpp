#include "Fallout/AI/ModifierList.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueInfo.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"

Modifier::Modifier() : cKey(0), fValue(0.0f) {}

Modifier::Modifier(char acKey, float afValue) : cKey(static_cast<uint8_t>(acKey)), fValue(afValue) {}

Modifier::Modifier(Modifier* apSource) : cKey(apSource->cKey), fValue(apSource->GetValue()) {}

char Modifier::GetKey() {
    return cKey;
}

float Modifier::GetValue() {
    return fValue;
}

void Modifier::SetValue(float afValue) {
    fValue = afValue;
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
        char cKey = apModifier->GetKey();
        ActorValueInfo* pInfo = ActorValue::GetActorValueInfo(static_cast<ActorValue::Index>(cKey));
        if (pInfo && (pInfo->uFlags & ActorValue::FAST_MODIFIER)) {
            AddFastModifier(cKey, apModifier);
        }
        else {
            AddHead(apModifier);
        }
    }
}

void ModifierList::AddFastModifier(char acKey, Modifier* apNew) {
    if (apNew || pFastModifiers) {
        if (pFastModifiers) {
            pFastModifiers->DeleteModifier(acKey);
        }
        if (apNew && (apNew->fValue != 0.0 || bZeroValuesAllowed)) {
            if (!pFastModifiers) {
                FastModifierCollection* pCollection = new FastModifierCollection;
                pFastModifiers = pCollection;
            }
            pFastModifiers->pModifiers[FastModifierCollection::uiAVMapping[acKey]] = apNew;
        }
    }
}

void ModifierList::DeleteModifier(Modifier* apModifier) {
    if (apModifier) {
        ActorValueInfo* pInfo = ActorValue::GetActorValueInfo(static_cast<ActorValue::Index>(apModifier->GetKey()));
        if (pInfo && (pInfo->uFlags & ActorValue::FAST_MODIFIER) && pFastModifiers) {
            pFastModifiers->DeleteModifier(apModifier->GetKey());
        }
        else {
            Remove(apModifier);
            delete apModifier;
        }
    }
}

void ModifierList::DeleteAllModifiers() {
    FastModifierCollection* pFastModifiers;
    Modifier* pModifier;
    while (true) {
        pModifier = m_item;
        if (!pModifier) {
            break;
        }
        delete pModifier;
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
    if (!usModifierSize) {
        return 0;
    }
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
    if (pModifier) {
        return pModifier->GetValue();
    }
    return 0.0f;
}

Modifier* ModifierList::GetModifierItem(char acKey) {
    if (pFastModifiers) {
        // actor values with a fast modifier map to an index in the
        // collection's modifier array
        const int32_t iMapping = FastModifierCollection::uiAVMapping[static_cast<int8_t>(acKey)];
        // indexes below 77 point into the modifier array
        if (iMapping < 77) {
            return pFastModifiers->pModifiers[iMapping];
        }
    }

    // walk the list nodes until the first empty item
    Modifier* pResult = nullptr;
    for (BSSimpleList<Modifier*>* pNode = this; pNode; pNode = pNode->GetNext()) {
        Modifier* pItem = pNode->GetItem();
        if (!pItem || pResult) {
            break;
        }
        if (pItem->GetKey() == acKey) {
            pResult = pItem;
        }
    }
    return pResult;
}

void ModifierList::operator=(const ModifierList& arList) {
    DeleteAllModifiers();
    if (&arList) {
        if (arList.pFastModifiers) {
            FastModifierCollection* pCollection = new FastModifierCollection(arList.pFastModifiers);
            pFastModifiers = pCollection;
        }
        for (const ModifierList* plist = &arList; plist && plist->m_item;
             plist = static_cast<ModifierList*>(plist->m_pkNext)) {
            Modifier* pCopy;
            Modifier* pitem = plist->m_item;
            pCopy = new Modifier(pitem);
            AddModifier(pCopy);
        }
    }
}

void ModifierList::SetZeroValuesAllowed(bool abZeroValuesAllowed) {
    bZeroValuesAllowed = abZeroValuesAllowed;
}

void ModifierList::SetModifier(char acKey, float afValue) {
    Modifier* pNew;
    Modifier* pModifier = GetModifierItem(acKey);
    if (pModifier) {
        if (afValue != 0.0 || bZeroValuesAllowed) {
            pModifier->SetValue(afValue);
        }
        else {
            DeleteModifier(pModifier);
        }
    }
    else if (afValue != 0.0 || bZeroValuesAllowed) {
        pNew = new Modifier(acKey, afValue);
        AddModifier(pNew);
    }
}

void ModifierList::AdjustModifier(char acKey, float afValue, ClampStyle aeClampStyle) {
    float fValue;
    Modifier* pNew;
    Modifier* pModifier = GetModifierItem(acKey);
    if (pModifier) {
        fValue = pModifier->GetValue();
        float fNewValue = ModifierList::ModifyAndClamp(fValue, afValue, aeClampStyle);
        pModifier->SetValue(fNewValue);
        if (pModifier->GetValue() == 0.0 && !bZeroValuesAllowed) {
            DeleteModifier(pModifier);
        }
    }
    else if (afValue != 0.0 || bZeroValuesAllowed) {
        pNew = new Modifier(acKey, afValue);
        AddModifier(pNew);
    }
}

void ModifierList::Revert() {
    DeleteAllModifiers();
}

bool ModifierList::WriteModifier(Modifier* apModifier) {
    if (!apModifier) {
        return false;
    }
    int8_t cKey = apModifier->GetKey();
    float fValue = apModifier->GetValue();
    pSaveLoadGameOLD->SaveGameDataOLD(&cKey, sizeof(cKey));
    pSaveLoadGameOLD->SaveGameDataOLD(&fValue, sizeof(fValue));
    return true;
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
            apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&pModifier->cKey), sizeof(pModifier->cKey), 0);
            apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&pModifier->fValue), sizeof(pModifier->fValue), 0);
            ++uiCount;
        }
        pNode = pNode->GetNext();
    }
    if (pFastModifiers) {
        for (uint32_t i = 0; i < static_cast<uint32_t>(FastModifierCollection::uiModifierCount); ++i) {
            if (pFastModifiers->pModifiers[i]) {
                apSaveGameBuffer->SaveDataEndian(
                    reinterpret_cast<char*>(&pFastModifiers->pModifiers[i]->cKey),
                    sizeof(pFastModifiers->pModifiers[i]->cKey),
                    0
                );
                apSaveGameBuffer->SaveDataEndian(
                    reinterpret_cast<char*>(&pFastModifiers->pModifiers[i]->fValue),
                    sizeof(pFastModifiers->pModifiers[i]->fValue),
                    0
                );
                ++uiCount;
            }
        }
    }
    apSaveGameBuffer->SaveVariableSizedValue(uiCount, uiPosition);
}

void ModifierList::LoadGameTES() {
    char cKey;
    float fValue;
    uint16_t usCount;
    pSaveLoadGameOLD->LoadGameDataOLD(&usCount, sizeof(usCount));
    for (uint32_t i = 0; i < usCount; ++i) {
        Modifier* pModifier;
        pSaveLoadGameOLD->LoadGameDataOLD(&cKey, sizeof(cKey));
        pSaveLoadGameOLD->LoadGameDataOLD(&fValue, sizeof(fValue));
        pModifier = new Modifier(cKey, fValue);
        AddModifier(pModifier);
    }
}

void ModifierList::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    uint32_t uiCount = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t i = 0; i < uiCount; ++i) {
        Modifier* pModifier = new Modifier;
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&pModifier->cKey), sizeof(pModifier->cKey));
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&pModifier->fValue), sizeof(pModifier->fValue));
        AddModifier(pModifier);
    }
}

void ModifierList::SaveGame() {
    uint16_t usCount = 0;
    uint16_t* pCount = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
    pSaveLoadGameOLD->SaveGameDataOLD(&usCount, sizeof(usCount));
    for (BSSimpleList<Modifier*>* pNode = this; pNode && pNode->m_item; pNode = pNode->m_pkNext) {
        if (WriteModifier(pNode->m_item)) {
            ++usCount;
        }
    }
    int32_t iFastCount = pFastModifiers != nullptr ? FastModifierCollection::uiModifierCount : 0;
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
    }
    else if (aeClampStyle == GE_ZERO && fTotal < 0.0) {
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

// Copies apOther's modifiers into an ourMalloc array; the destructor deletes each copy and frees the array.
FastModifierCollection::FastModifierCollection(FastModifierCollection* apOther) : pModifiers(nullptr) {
    if (apOther->pModifiers) {
        if (uiModifierCount) {
            pModifiers = static_cast<Modifier**>(ourMalloc(sizeof(Modifier*) * static_cast<uint32_t>(uiModifierCount)));
        }
        for (int32_t i = 0; i < uiModifierCount; ++i) {
            pModifiers[i] = apOther->pModifiers[i] ? new Modifier(apOther->pModifiers[i]) : nullptr;
        }
    }
}

FastModifierCollection::~FastModifierCollection() {
    if (uiModifierCount && pModifiers) {
        for (int32_t i = 0; i < uiModifierCount; ++i) {
            if (pModifiers[i]) {
                delete pModifiers[i];
            }
        }
        ourFree(pModifiers);
        pModifiers = nullptr;
    }
}

void FastModifierCollection::DeleteModifier(uint32_t auiIndex) {
    if (auiIndex <= 76 &&
        pModifiers &&
        FastModifierCollection::uiAVMapping[auiIndex] < 77 &&
        pModifiers[FastModifierCollection::uiAVMapping[auiIndex]]) {
        delete pModifiers[FastModifierCollection::uiAVMapping[auiIndex]];
        pModifiers[FastModifierCollection::uiAVMapping[auiIndex]] = nullptr;
    }
}

void FastModifierCollection::CheckInitializeMapping() {
    if (!bInitialized) {
        int32_t iCount = 0;
        for (int32_t i = 0; i < ActorValue::ACTOR_VALUE_COUNT; ++i) {
            ActorValueInfo* pInfo = ActorValue::GetActorValueInfo(static_cast<ActorValue::Index>(i));
            if (pInfo && (pInfo->uFlags & ActorValue::FAST_MODIFIER)) {
                FastModifierCollection::uiAVMapping[i] = iCount++;
            }
            else {
                FastModifierCollection::uiAVMapping[i] = ActorValue::ACTOR_VALUE_COUNT;
            }
        }
        uiModifierCount = iCount;
        bInitialized = 1;
    }
}
