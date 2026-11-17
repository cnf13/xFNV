#include "Fallout Shared/TESContainer.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/BGSBipedModelList.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESHealthForm.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectARMO.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESWeightForm.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdlib.h"

void CONTAINER_OBJECT_FILE::Endian() {
    ENDIAN(iObjectID, false);
    ENDIAN(iCount, false);
}

ContainerObject::ContainerObject() : i_count(1), pObj(nullptr), pItemExtra(nullptr) {}

// The destructor frees the extra data's memory without destroying it, as the game does.
ContainerObject::~ContainerObject() {
    ourFree(pItemExtra);
}

// While loading, pObj holds the object's form ID; this turns it into the bound object, with apOwner's file's
// compile index.
void ContainerObject::InitItem(TESForm* apOwner) {
    // The loader stored the raw form ID in the pointer field; it is read back here as an integer.
    uint32_t iID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pObj));
    TESForm::AddCompileIndex(iID, apOwner->GetFile(-1));
    pObj = dynamic_cast<TESBoundObject*>(TESForm::GetFormByNumericID(iID));
    if (!pObj) {
        if (apOwner) {
            if (apOwner->GetFormEditorIDLength()) {
                Warning(
                    "MASTERFILE: Unable to find container object (%08X) on owner object \"%s\".", iID,
                    apOwner->GetFormEditorID()
                );
            }
            else {
                Warning(
                    "MASTERFILE: Unable to find container object (%08X) on owner object (%08X).", iID,
                    apOwner->GetFormID()
                );
            }
        }
        else {
            Warning("MASTERFILE: Unable to find container object (%08X) on unknown owner.", iID);
        }
    }
    if (pItemExtra) {
        pItemExtra->InitItem(apOwner);
    }
}

void ContainerObject::Copy(ContainerObject* apCopy) {
    ContainerItemExtra* v2;
    void* v4;
    if (apCopy) {
        pObj = apCopy->pObj;
        i_count = (int32_t)((uint32_t)apCopy->i_count);
        if ((apCopy->pItemExtra) && (!apCopy->pItemExtra->IsDefault())) {
            v4 = ourMalloc(sizeof(ContainerItemExtra));
            if (v4) {
                v2 = ::new (v4) ContainerItemExtra;
            }
            else {
                v2 = nullptr;
            }
            pItemExtra = v2;
            pItemExtra->Copy(apCopy->pItemExtra);
        }
    }
}

bool ContainerObject::Compare(ContainerObject* apCompare) {
    if (!apCompare) {
        return true;
    }
    if (((uint32_t)i_count) != ((uint32_t)apCompare->i_count)) {
        return true;
    }
    if (pObj != apCompare->pObj) {
        return true;
    }
    if (pItemExtra && ((!pItemExtra->IsDefault()) && (!apCompare->pItemExtra))) {
        return true;
    }
    if ((!pItemExtra) && (apCompare->pItemExtra && (!apCompare->pItemExtra->IsDefault()))) {
        return true;
    }
    return (bool)((uint8_t)(pItemExtra && (apCompare->pItemExtra && pItemExtra->Compare(apCompare->pItemExtra))));
}

void ContainerObject::Save() {
    CONTAINER_OBJECT_FILE cfile;
    cfile.iCount = i_count;
    cfile.iObjectID = pObj->iFormID;
    if (TESForm::QEndianSwapOnSave()) {
        cfile.Endian();
    }
    TESForm::__AddChunkData(CNTO_ID, &cfile, 8u);
    if (TESForm::QEndianSwapOnSave()) {
        cfile.Endian();
    }
    if (pItemExtra) {
        pItemExtra->Save();
    }
}

TESContainer::TESContainer() {}

TESContainer::~TESContainer() {
    TESContainer::ClearDataComponent();
}

void TESContainer::ClearDataComponent() {
    FreeObjectList();
}

void TESContainer::CopyComponent(BaseFormComponent* apCopyFC) {
    TESContainer* pCopy = dynamic_cast<TESContainer*>(apCopyFC);
    if (pCopy) {
        FreeObjectList();
        for (BSSimpleList<ContainerObject*>* pList = &pCopy->objectList; pList && !pList->IsEmpty();
             pList = pList->m_pkNext) {
            AddDuplicateObject(pList->m_item);
        }
    }
}

bool TESContainer::CompareComponent(BaseFormComponent* apCompareFC) {
    TESContainer* pCompare = dynamic_cast<TESContainer*>(apCompareFC);
    if (!pCompare) {
        return true;
    }
    uint32_t iCount = 0;
    for (ContainerObject* pobj : *&objectList) {
        if (pobj) {
            BSSimpleList<ContainerObject*>* pCompList = &pCompare->objectList;
            bool bFound = false;
            while (pCompList) {
                ContainerObject* pcomp = pCompList->m_item;
                if (pcomp && !pcomp->Compare(pobj)) {
                    bFound = true;
                    break;
                }
                pCompList = pCompList->m_pkNext;
            }
            if (!bFound) {
                return true;
            }
            ++iCount;
        }
    }
    return (&pCompare->objectList)->ItemsInList() != iCount;
}

void TESContainer::Save() {
    BSSimpleList<ContainerObject*>* pkList = &objectList;
    if (!pkList->m_item) {
        pkList = nullptr;
    }
    while (pkList) {
        ContainerObject** v1 = &pkList->m_item;
        ContainerObject* obj = *v1;
        if (!(*v1)->pObj->GetDelete()) {
            obj->Save();
        }
        pkList = pkList->m_pkNext;
    }
}

float TESContainer::GetContainerWeight(bool abIsHardcore) {
    float weight = 0.0f;
    BSSimpleList<ContainerObject*>* pkList = &objectList;
    if (!pkList->m_item) {
        pkList = nullptr;
    }
    while (pkList) {
        ContainerObject* obj = pkList->m_item;
        pkList = pkList->m_pkNext;
        if (!obj->pObj->GetDelete()) {
            double v4 = static_cast<double>(std::abs(obj->i_count));
            weight = TESWeightForm::GetFormWeight(obj->pObj, abIsHardcore) * v4 + weight;
        }
    }
    return weight;
}

void TESContainer::InitItem(TESForm* apOwner) {
    BSSimpleList<ContainerObject*>* plist = &objectList;
    BSSimpleList<ContainerObject*>* plastlist = nullptr;
    while (plist && !plist->IsEmpty()) {
        ContainerObject* pobj = plist->m_item;
        pobj->InitItem(apOwner);
        if (pobj->pObj) {
            plastlist = plist;
            plist = plist->m_pkNext;
        }
        else {
            if (plastlist) {
                plastlist->Remove(pobj);
                plist = plastlist->m_pkNext;
            }
            else {
                plist->RemoveHead();
            }
            if (pobj) {
                delete pobj;
            }
            pobj = nullptr;
        }
    }
}

bool TESContainer::HasObject(TESObject* pObj) {
    BSSimpleList<ContainerObject*>* pNode = GetObjectList();
    if (!pNode->GetItem()) {
        pNode = nullptr;
    }
    while (pNode) {
        if (pNode->GetItem()->pObj == pObj) {
            return true;
        }
        pNode = pNode->GetNext();
    }
    return false;
}

int32_t TESContainer::GetObjectCount(TESObject* apObject) {
    int32_t iCount = 0;
    for (BSSimpleList<ContainerObject*>* pNode = GetObjectList(); pNode && pNode->GetItem(); pNode = pNode->GetNext()) {
        if (pNode->GetItem()->pObj == apObject) {
            iCount += pNode->GetItem()->i_count;
        }
    }
    return iCount;
}

void TESContainer::RemoveContainerObject(ContainerObject* apCObj) {
    if (apCObj) {
        (&objectList)->Remove(apCObj);
        if (apCObj) {
            delete apCObj;
        }
    }
}

void TESContainer::ApplyHealthPercentToContainer(float afHealthPerc) {
    for (BSSimpleList<ContainerObject*>* plist = &objectList; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
        ContainerObject** v2 = &plist->m_item;
        ContainerObject* pcont = *v2;
        if (!(*v2)->pItemExtra) {
            ContainerItemExtra* v3;
            ContainerItemExtra* apMemory = static_cast<ContainerItemExtra*>(ourMalloc(sizeof(ContainerItemExtra)));
            if (apMemory) {
                v3 = ::new (apMemory) ContainerItemExtra;
            }
            else {
                v3 = nullptr;
            }
            pcont->pItemExtra = v3;
        }
        pcont->pItemExtra->AdjustHealthPercent(afHealthPerc);
        if (pcont->pItemExtra->IsDefault()) {
            ourFree(pcont->pItemExtra);
            pcont->pItemExtra = nullptr;
        }
    }
}

void TESContainer::AddLevItemContainerToChanges(int32_t aiLevItemIndex, InventoryChanges* apChanges) {
    bool bhadChanges = false;
    if (apChanges) {
        for (BSSimpleList<ContainerObject*>* plist = &objectList; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
            ContainerObject* pcont = plist->m_item;
            if (pcont) {
                ExtraDataList* pextra;
                bhadChanges = true;
                ExtraDataList* apMemory = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
                if (apMemory) {
                    pextra = ::new (apMemory) ExtraDataList;
                }
                else {
                    pextra = nullptr;
                }
                pextra->SetLevelItemExtra(aiLevItemIndex);
                pextra->SetCount(pcont->i_count);
                if (pcont->pItemExtra) {
                    TESHealthForm* FormAsHealthForm = TESHealthForm::GetFormAsHealthForm(pcont->pObj);
                    pcont->pItemExtra->AddToExtraDataList(pextra, FormAsHealthForm);
                }
                apChanges->AddItem(pcont->pObj, pextra, pcont->i_count);
            }
        }
        if ((bhadChanges) && (apChanges->pRef) && (appMain->bOnIdle) &&
            (!pSaveLoadGame->GetSaveGameLoading() || pSaveLoadGame->GetPositioningPlayerCharacter())) {
            apChanges->pRef->ForceChange(CHANGE_REFR_LEVELED_INVENTORY);
        }
    }
}

// The file entry's form ID goes into pObj until InitItem resolves it.
ContainerObject* TESContainer::AddObject(CONTAINER_OBJECT_FILE* apCOFile) {
    if (!apCOFile) {
        return nullptr;
    }
    [[maybe_unused]] BSSimpleList<ContainerObject*>* pList = GetObjectList();
    void* pMemory = ourMalloc(sizeof(ContainerObject));
    ContainerObject* pObject = pMemory ? ::new (pMemory) ContainerObject : nullptr;
    pObject->pObj = reinterpret_cast<TESBoundObject*>(static_cast<uintptr_t>(apCOFile->iObjectID));
    pObject->i_count = apCOFile->iCount;
    GetObjectList()->AddHead(pObject);
    return pObject;
}

ContainerObject* TESContainer::AddObject(TESObject* apObj, int32_t aiCount, ContainerItemExtra* apExtra) {
    if (!apObj || !aiCount) {
        return nullptr;
    }
    ContainerObject* a2 = nullptr;
    for (BSSimpleList<ContainerObject*>* ecx0 = GetObjectList(); ecx0 && !ecx0->IsEmpty(); ecx0 = ecx0->m_pkNext) {
        a2 = ecx0->m_item;
        if (a2->pObj == apObj) {
            if (a2->pItemExtra) {
                if (apExtra && !a2->pItemExtra->Compare(apExtra)) {
                    break;
                }
            }
            else {
                if (!apExtra) {
                    break;
                }
            }
        }
        a2 = nullptr;
    }
    if (a2) {
        a2->i_count += aiCount;
    }
    else {
        ContainerObject* pContainerObj;
        ContainerObject* apMemory = static_cast<ContainerObject*>(ourMalloc(sizeof(ContainerObject)));
        if (apMemory) {
            pContainerObj = ::new (apMemory) ContainerObject;
        }
        else {
            pContainerObj = nullptr;
        }
        a2 = pContainerObj;
        pContainerObj->i_count = aiCount;
        a2->pObj = static_cast<TESBoundObject*>(apObj);
        if (apExtra) {
            ContainerItemExtra* v6;
            ContainerItemExtra* v9 = static_cast<ContainerItemExtra*>(ourMalloc(sizeof(ContainerItemExtra)));
            if (v9) {
                v6 = ::new (v9) ContainerItemExtra;
            }
            else {
                v6 = nullptr;
            }
            a2->pItemExtra = v6;
            a2->pItemExtra->Copy(apExtra);
        }
        BSSimpleList<ContainerObject*>* ActorPackageThatIsRunning = GetObjectList();
        ActorPackageThatIsRunning->AddHead(a2);
    }
    return a2;
}

void TESContainer::AddContainerToReference(TESObjectREFR* apRef, bool abSilent) {
    if (apRef) {
        Actor* pactor = dynamic_cast<Actor*>(apRef);
        ContainerObject* plast = nullptr;
        for (ContainerObject* pcont : *&objectList) {
            if (pcont) {
                Script* pscript = TESScriptableForm::GetFormScript(pcont->pObj);
                int32_t inum = pcont->i_count;
                plast = pcont;
                if (pscript) {
                    for (int32_t i = 0; i < inum; ++i) {
                        ExtraDataList* pextra_2;
                        ExtraDataList* pextra = nullptr;
                        ExtraDataList* apMemory = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
                        if (apMemory) {
                            pextra_2 = ::new (apMemory) ExtraDataList;
                        }
                        else {
                            pextra_2 = nullptr;
                        }
                        pextra = pextra_2;
                        pextra_2->SetCount(1);
                        if ((pextra_2) && (!pextra->GetScript())) {
                            TESObjectREFR* pref;
                            pextra->SetScript(pscript);
                            ScriptLocals* pLocals = pscript->CloneLocals();
                            pextra->SetScriptLocals(pLocals);
                            TESObjectREFR* pRefMemory = static_cast<TESObjectREFR*>(ourMalloc(sizeof(TESObjectREFR)));
                            if (pRefMemory) {
                                pref = ::new (pRefMemory) TESObjectREFR;
                            }
                            else {
                                pref = nullptr;
                            }
                            pref->SetTemporary();
                            ScriptLocals* pExtraLocals = pextra->GetScriptLocals();
                            pscript->Run(pref, pExtraLocals, nullptr, false);
                            if (pref) {
                                delete pref;
                            }
                            if (pcont->pItemExtra) {
                                TESHealthForm* FormAsHealthForm = TESHealthForm::GetFormAsHealthForm(pcont->pObj);
                                pcont->pItemExtra->AddToExtraDataList(pextra, FormAsHealthForm);
                            }
                        }

                        if (pactor) {
                            pactor->AddObjecttoContainer(pcont->pObj, pextra, 1);
                        }
                        else {
                            apRef->AddBoundObjectItem(pcont->pObj, pextra, 1);
                        }
                    }
                }
                else {
                    ExtraDataList* pextra_3 = nullptr;
                    if (pcont->pItemExtra) {
                        ExtraDataList* pextra_4;
                        ExtraDataList* pExtraMemory = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
                        if (pExtraMemory) {
                            pextra_4 = ::new (pExtraMemory) ExtraDataList;
                        }
                        else {
                            pextra_4 = nullptr;
                        }
                        pextra_3 = pextra_4;
                        pextra_4->SetCount(pcont->i_count);
                        TESHealthForm* pHealthForm = TESHealthForm::GetFormAsHealthForm(pcont->pObj);
                        pcont->pItemExtra->AddToExtraDataList(pextra_4, pHealthForm);
                    }
                    if (pactor) {
                        pactor->AddObjecttoContainer(pcont->pObj, pextra_3, pcont->i_count);
                    }
                    else {
                        apRef->AddBoundObjectItem(pcont->pObj, pextra_3, pcont->i_count);
                    }
                }
            }
        }
        if ((pactor == pPlayer) && (plast) && (!abSilent)) {
            BSStringT<char> pstring;
            if (plast->i_count <= 1) {
                const char* pAdded = sAddItemtoInventory.String();
                pstring.SPrintF("%s %s", TESFullName::GetFullName(plast->pObj), pAdded);
            }
            else {
                const char* pAdded = sAddItemtoInventory.String();
                const char* pPluralText = sPlural.String();
                const char* pFullName = TESFullName::GetFullName(plast->pObj);
                pstring.SPrintF("%i %s%s %s", plast->i_count, pFullName, pPluralText, pAdded);
            }
            // abSilent is false here, so the sound is always requested.
            switch (plast->pObj->GetFormType()) {
                case ARMO_ID:
                case BOOK_ID:
                case LIGH_ID:
                case MISC_ID:
                case WEAP_ID:
                case AMMO_ID:
                case KEYM_ID:
                case ALCH_ID:
                case COBJ_ID:
                case IMOD_ID:
                case CHIP_ID:
                case CCRD_ID:
                case CMNY_ID: {
                    const char* pSoundName = pPlayer->GetPickUpSoundName(plast->pObj, true, false);
                    Interface::ShowMessage(
                        pstring.pString, 0, "Interface\\Icons\\Message Icons\\glow_message_giftbox.dds", pSoundName,
                        2.0f, false
                    );
                    break;
                }
                default: break;
            }
        }
    }
}

void TESContainer::MultiplyContainerCounts(int32_t aiMult) {
    for (BSSimpleList<ContainerObject*>* pNode = GetObjectList(); pNode; pNode = pNode->GetNext()) {
        ContainerObject* pCObj = pNode->GetItem();
        if (pCObj) {
            pCObj->i_count *= aiMult;
        }
    }
}

void TESContainer::AddContainerContents(TESContainer* apAddCont) {
    if (apAddCont) {
        for (BSSimpleList<ContainerObject*>* pNode = apAddCont->GetObjectList(); pNode; pNode = pNode->GetNext()) {
            ContainerObject* pCObj = pNode->GetItem();
            if (pCObj) {
                AddObject(pCObj->pObj, pCObj->i_count, pCObj->pItemExtra);
            }
        }
    }
}

void TESContainer::FreeObjectList() {
    for (BSSimpleList<ContainerObject*>* pkList = &objectList;; pkList->RemoveHead()) {
        ContainerObject* pCObj = pkList->m_item;
        if (!pCObj) {
            break;
        }
        delete pCObj;
    }
}

void TESContainer::AddDuplicateObject(ContainerObject* pCObj) {
    if (pCObj) {
        ContainerObject* v3;
        ContainerObject* apMemory = static_cast<ContainerObject*>(ourMalloc(sizeof(ContainerObject)));
        if (apMemory) {
            v3 = ::new (apMemory) ContainerObject;
        }
        else {
            v3 = nullptr;
        }
        ContainerObject* a2 = v3;
        v3->Copy(pCObj);
        BSSimpleList<ContainerObject*>* ActorPackageThatIsRunning = GetObjectList();
        ActorPackageThatIsRunning->AddHead(a2);
    }
}

TESObjectWEAP* TESContainer::GetBestWeapon(TESActorBase* apActorBase) {
    BSSimpleList<ContainerObject*>* pList = &objectList;
    float fHighDesire = -3.4028235e+38f;
    TESObjectWEAP* pBest = nullptr;
    while (pList) {
        ContainerObject* pcont = pList->m_item;
        if (pcont) {
            TESObjectWEAP* pObj = dynamic_cast<TESObjectWEAP*>(pcont->pObj);
            if (pObj) {
                float fDesire = apActorBase->GetDesirability(pObj);
                if (fHighDesire < static_cast<double>(fDesire)) {
                    fHighDesire = fDesire;
                    pBest = pObj;
                }
            }
        }
        pList = pList->m_pkNext;
    }
    return pBest;
}

TESObjectARMO* TESContainer::GetBestArmor(TESActorBase* apActorBase, BIPED_OBJECT aeBodySlot) {
    BSSimpleList<ContainerObject*>* pList = &objectList;
    float fHighDesire = -3.4028235e+38f;
    TESObjectARMO* pBest = nullptr;
    while (pList) {
        ContainerObject* pcont = pList->m_item;
        if (pcont) {
            TESObjectARMO* pObj = dynamic_cast<TESObjectARMO*>(pcont->pObj);
            if ((pObj) && (aeBodySlot != BO_NONE && pObj->FillsBipedSlot(aeBodySlot, false, nullptr))) {
                float fDesire = apActorBase->GetDesirability(pObj);
                if (fHighDesire < static_cast<double>(fDesire)) {
                    fHighDesire = fDesire;
                    pBest = pObj;
                }
            }
        }
        pList = pList->m_pkNext;
    }
    return pBest;
}

BSSimpleList<ContainerObject*>* TESContainer::GetFilteredList() {
    BSSimpleList<ContainerObject*>* pnewlist;
    BSSimpleList<ContainerObject*>* v5 =
        static_cast<BSSimpleList<ContainerObject*>*>(ourMalloc(sizeof(BSSimpleList<ContainerObject*>)));
    if (v5) {
        pnewlist = ::new (v5) BSSimpleList<ContainerObject*>;
    }
    else {
        pnewlist = nullptr;
    }
    for (ContainerObject*& rpItem : objectList.ItemsUntilNull()) {
        pnewlist->AddHead(rpItem);
    }
    for (BSSimpleList<ContainerObject*>* plist_2 = pnewlist; plist_2 && plist_2->m_item; plist_2 = plist_2->m_pkNext) {
        BSSimpleList<ContainerObject*>* pnext = plist_2->m_pkNext;
        while (pnext && pnext->m_item) {
            ContainerObject* m_item = plist_2->m_item;
            if (m_item->pObj == pnext->m_item->pObj) {
                plist_2->Remove(pnext->m_item);
                pnext = plist_2->m_pkNext;
            }
            else {
                pnext = pnext->m_pkNext;
            }
        }
    }
    return pnewlist;
}

bool TESContainer::ContainerCanHoldType(uint8_t aType) {
    uint8_t result;
    switch (aType) {
        case 0x18u:
        case 0x19u:
        case 0x1Au:
        case 0x1Du:
        case 0x1Eu:
        case 0x1Fu:
        case 0x28u:
        case 0x29u:
        case 0x2Eu:
        case 0x2Fu:
        case 0x31u:
        case 0x32u:
        case 0x34u:
        case 0x67u:
        case 0x6Cu:
        case 0x73u:
        case 0x74u: {
            result = 1;
            break;
        }
        default: {
            result = 0;
            break;
        }
    }
    return (bool)result;
}

bool TESContainer::IsGold(TESObject* pObj) {
    return (bool)((uint8_t)(pObj && (pObj->TESForm::GetFormID() == 15)));
}

Script* TESScriptableForm::GetFormScript(TESForm* apForm) {
    auto* pScriptable = dynamic_cast<TESScriptableForm*>(apForm);
    return pScriptable ? pScriptable->GetFormScript() : nullptr;
}
