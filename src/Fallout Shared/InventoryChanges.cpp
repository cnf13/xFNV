#include "Fallout Shared/InventoryChanges.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSBipedModelList.h"
#include "Fallout Shared/BGSEntryPoint.h"
#include "Fallout Shared/BGSListForm.h"
#include "Fallout Shared/BGSNote.h"
#include "Fallout Shared/BGSRepairItemList.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/InventoryIterator.h"
#include "Fallout Shared/Magic/AlchemyItem.h"
#include "Fallout Shared/Magic/EnchantmentItem.h"
#include "Fallout Shared/Magic/IngredientItem.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESAmmo.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESEnchantableForm.h"
#include "Fallout Shared/TESFaction.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESHealthForm.h"
#include "Fallout Shared/TESLevItem.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectARMO.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectIMOD.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESValueForm.h"
#include "Fallout Shared/TESWeightForm.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/AI/AILinearTaskThreadManager.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/Explosion.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/rtti.h"

#include <cfloat>

// The game settings this file defines, with their IDs and values, in initialization order.
BSSpinLock InventoryChanges::kRunScriptsLock;
SettingT<GameSettingCollection> fPackRatThreshold("fPackRatThreshold", 2.0f);
SettingT<GameSettingCollection> fPackRatModifier("fPackRatModifier", 0.5f);

using NiTMap_TESObject_bool = NiTMap<TESObject*, bool>;

// The base class of NiTMap<TESObject*, bool>, the map of objects already looked at.
using NiTMapBaseTESObjectBool = NiTMapBase<DFALL<NiTMapItem<TESObject*, bool>>, TESObject*, bool>;

ItemChange::ItemChange() : pExtraObjectList(nullptr), iNumber(0), pContainerObj(nullptr) {}

ItemChange::ItemChange(TESBoundObject* apObject, int32_t aiNumber)
  : pExtraObjectList(new BSSimpleList<ExtraDataList*>), iNumber(aiNumber), pContainerObj(apObject) {}

ItemChange::ItemChange(const ItemChange* apCopy)
  : pExtraObjectList(new BSSimpleList<ExtraDataList*>), pContainerObj(apCopy->pContainerObj) {
    for (BSSimpleList<ExtraDataList*>* pkFirst = apCopy->pExtraObjectList; pkFirst && pkFirst->m_item;
         pkFirst = pkFirst->m_pkNext) {
        ExtraDataList* pExtra = new ExtraDataList;
        pExtra->DuplicateExtraListForContainer(pkFirst->m_item);
        pExtraObjectList->AddTail(pExtra);
    }
    iNumber = apCopy->iNumber;
}

// The instance list is the change's own allocation: emptied and freed here.
ItemChange::~ItemChange() {
    if (pExtraObjectList) {
        pExtraObjectList->RemoveAll();
    }
    delete pExtraObjectList;
    pExtraObjectList = nullptr;
}

uint8_t ItemChange::GetModSlots() {
    ExtraDataList* v2;
    BSSimpleList<ExtraDataList*>* ecx0 = pExtraObjectList;
    if (pExtraObjectList && (ecx0->m_item && ((v2 = ecx0->m_item) != 0 && v2->HasWeaponMods()))) {
        return v2->GetWeaponModFlags();
    }
    else {
        return 0;
    }
}

void ItemChange::DeleteAllExtra() {
    for (BSSimpleList<ExtraDataList*>* pkFirst = pExtraObjectList; pkFirst && pkFirst->m_item;
         pkFirst = pkFirst->m_pkNext) {
        ExtraDataList* pExtra = pkFirst->m_item;
        pExtra->RemoveAll(1);
        pkFirst->Remove(pExtra);
        if (pExtra) {
            delete pExtra;
        }
    }
}

TESForm* ItemChange::GetItemOwnership() {
    ExtraDataList* v2;
    BSSimpleList<ExtraDataList*>* ecx0 = pExtraObjectList;
    if (pExtraObjectList && (ecx0->m_item && (((v2 = ecx0->m_item) != 0) && v2->GetOwner()))) {
        return v2->GetOwner();
    }
    else {
        return nullptr;
    }
}

TESBoundObject* ItemChange::GetContainerObject() {
    return pContainerObj;
}

BSSimpleList<ExtraDataList*>* ItemChange::GetExtraDataList() {
    return pExtraObjectList;
}

bool ItemChange::GetWorn(bool ableft) {
    for (BSSimpleList<ExtraDataList*>* pIter = pExtraObjectList; pIter && pIter->GetItem(); pIter = pIter->GetNext()) {
        ExtraDataList* pList = pIter->GetItem();
        if (pList && pList->GetWorn(ableft)) {
            return true;
        }
    }
    return false;
}

void ItemChange::SetWorn(bool abflag, bool ableft, bool abdelete) {
    if (abflag) {
        ExtraDataList* pworn;
        ExtraDataList* apMemory = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
        if (apMemory) {
            pworn = ::new (apMemory) ExtraDataList;
        }
        else {
            pworn = 0;
        }
        ExtraDataList* pworn_2 = pworn;
        pworn->SetWorn(1, ableft);
        if (!pExtraObjectList) {
            BSSimpleList<ExtraDataList*>* v4;
            BSSimpleList<ExtraDataList*>* v7 =
                static_cast<BSSimpleList<ExtraDataList*>*>(ourMalloc(sizeof(BSSimpleList<ExtraDataList*>)));
            if (v7) {
                v4 = ::new (v7) BSSimpleList<ExtraDataList*>;
            }
            else {
                v4 = 0;
            }
            pExtraObjectList = v4;
        }
        pExtraObjectList->AddHead(pworn_2);
    }
    else {
        if (GetWorn(ableft)) {
            for (BSSimpleList<ExtraDataList*>* pkFirst = pExtraObjectList; pkFirst && pkFirst->m_item;
                 pkFirst = pkFirst->m_pkNext) {
                ExtraDataList* pExtra = pkFirst->m_item;
                if (pExtra && pExtra->GetWorn(ableft)) {
                    pkFirst->Remove(pExtra);
                    if (abdelete) {
                        if (pExtra) {
                            delete pExtra;
                        }
                    }
                    return;
                }
            }
        }
    }
}

void ItemChange::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    for (BSSimpleList<ExtraDataList*>* pNode = pExtraObjectList; pNode;) {
        ExtraDataList* pList;
        pList = pNode->GetItem();
        if (pList) {
            pList->InitLoadGame(apLoadGameBuffer, pContainerObj);
        }
        pNode = pNode->GetNext();
    }
}

void ItemChange::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveFormID(pContainerObj, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iNumber), 4, 0);
    uint32_t uiCount = 0;
    uint32_t uiPosition = apSaveGameBuffer->StartVariableSizedValue();
    for (BSSimpleList<ExtraDataList*>* pNode = pExtraObjectList; pNode;) {
        ExtraDataList* pList;
        pList = pNode->GetItem();
        if (pList) {
            pList->SaveGame(apSaveGameBuffer);
            ++uiCount;
        }
        pNode = pNode->GetNext();
    }
    apSaveGameBuffer->SaveVariableSizedValue(uiCount, uiPosition);
}

bool ItemChange::HasLeveledExtra() {
    for (BSSimpleList<ExtraDataList*>* pNode = pExtraObjectList; pNode;) {
        ExtraDataList* pList;
        pList = pNode->GetItem();
        if (!pList) {
            break;
        }
        pList = pNode->GetItem();
        if (pList && pList->HasLeveledItem()) {
            return true;
        }
        pNode = pNode->GetNext();
    }
    return false;
}

bool ItemChange::HasStolenData() {
    for (BSSimpleList<ExtraDataList*>* pNode = pExtraObjectList; pNode;) {
        ExtraDataList* pList;
        pList = pNode->GetItem();
        if (!pList) {
            break;
        }
        pList = pNode->GetItem();
        if (pList && pList->HasOnlyOwnershipNonDefault()) {
            return true;
        }
        pNode = pNode->GetNext();
    }
    return false;
}

AlchemyItem* ItemChange::GetPoison() {
    AlchemyItem* pPoison = nullptr;
    BSSimpleList<ExtraDataList*>* pNode = pExtraObjectList;
    if (!pNode) {
        return pPoison;
    }
    ExtraDataList* pList;
    pList = pNode->GetItem();
    if (!pList) {
        return pPoison;
    }
    pList = pNode->GetItem();
    if (!pList) {
        return pPoison;
    }
    if (pList->GetPoison()) {
        return pList->GetPoison();
    }
    return pPoison;
}

void ItemChange::SetPoison(AlchemyItem* apPoison) {
    BSSimpleList<ExtraDataList*>* pNode = pExtraObjectList;
    if (!pNode) {
        return;
    }
    ExtraDataList* pList;
    pList = pNode->GetItem();
    if (!pList) {
        return;
    }
    pList = pNode->GetItem();
    if (pList && !pList->GetPoison()) {
        pList->SetPoison(apPoison);
    }
}

void ItemChange::RemovePoison() {
    BSSimpleList<ExtraDataList*>* pNode = pExtraObjectList;
    if (!pNode) {
        return;
    }
    ExtraDataList* pList;
    pList = pNode->GetItem();
    if (!pList) {
        return;
    }
    pList = pNode->GetItem();
    if (pList && pList->GetPoison()) {
        pList->RemovePoison();
    }
}

float ItemChange::GetItemCharge() {
    ExtraDataList* v4;
    BSSimpleList<ExtraDataList*>* ecx0;
    TESEnchantableForm* v6 = dynamic_cast<TESEnchantableForm*>(pContainerObj);
    if (v6) {
        ecx0 = pExtraObjectList;
        if (pExtraObjectList && (ecx0->m_item && (((v4 = ecx0->m_item) != 0) && (v4->GetCharge() != (-1.0))))) {
            return v4->GetCharge();
        }
        else {
            return (float)v6->iAmountofEnchantment;
        }
    }
    else {
        return (-0x1p+0f);
    }
}

void ItemChange::SetCount(int32_t ainumber) {
    iNumber = ainumber;
}

int32_t ItemChange::GetHotKey() {
    int32_t ivalue = -1;
    for (BSSimpleList<ExtraDataList*>* pextraList = pExtraObjectList; ivalue < 0 && (pextraList && pextraList->m_item);
         pextraList = pextraList->m_pkNext) {
        ExtraDataList** v1 = &pextraList->m_item;
        if ((*v1)->GetHotKey() >= 0) {
            ExtraDataList** v2 = &pextraList->m_item;
            ivalue = (*v2)->GetHotKey();
        }
    }
    return ivalue;
}

// Each mod's value is read through TESValueForm's getter and converted as a signed value.
float ItemChange::GetItemValue() {
    float afNearest = GetItemHealth(1);
    float afVal = static_cast<float>(TESValueForm::GetFormValue(pContainerObj));
    float fvalue = GamePlayFormulas::CalcItemHealthValue(afVal, afNearest);
    TESObjectWEAP* pWeap = dynamic_cast<TESObjectWEAP*>(pContainerObj);
    if (pWeap) {
        uint8_t ModSlots = GetModSlots();
        if (ModSlots) {
            if ((ModSlots & 1) != 0) {
                TESObjectIMOD* ModItem = pWeap->GetModItem(1u);
                fvalue = static_cast<double>(ModItem->TESValueForm::GetFormValue()) + fvalue;
            }
            if ((ModSlots & 2) != 0) {
                TESObjectIMOD* v2 = pWeap->GetModItem(2u);
                fvalue = static_cast<double>(v2->TESValueForm::GetFormValue()) + fvalue;
            }
            if ((ModSlots & 4) != 0) {
                TESObjectIMOD* v3 = pWeap->GetModItem(4u);
                fvalue = static_cast<double>(v3->TESValueForm::GetFormValue()) + fvalue;
            }
        }
    }
    return BSRoundToNearest(fvalue, 0.1f);
}

int32_t ItemChange::GetExtraTotalCount(bool abWornIgnore) {
    BSSimpleList<ExtraDataList*>* ecx0 = pExtraObjectList;
    int32_t v4 = 0;
    while (ecx0 && ecx0->m_item) {
        ExtraDataList* v3 = ecx0->m_item;
        if (!v3->IsExtraDefaultforContainer(0) && (abWornIgnore || !v3->GetWorn(0))) {
            v4 += v3->GetCount();
        }
        ecx0 = ecx0->m_pkNext;
    }
    return v4;
}

int32_t ItemChange::GetExtraTotalDefaultCount() {
    BSSimpleList<ExtraDataList*>* pkFirst = pExtraObjectList;
    int32_t icount = 0;
    while (pkFirst && pkFirst->m_item) {
        ExtraDataList* pExtra = pkFirst->m_item;
        if (pExtra->IsExtraDefaultforContainer(0)) {
            int16_t Count = pExtra->GetCount();
            icount += Count;
        }
        pkFirst = pkFirst->m_pkNext;
        if (pExtra->IsExtraListEmpty()) {
            pExtraObjectList->Remove(pExtra);
            if (pExtra) {
                delete pExtra;
            }
            icount = 0;
            pkFirst = pExtraObjectList;
        }
    }
    return icount;
}

Script* ItemChange::GetScript() {
    ExtraDataList* v2;
    BSSimpleList<ExtraDataList*>* ecx0;
    for (ecx0 = pExtraObjectList; ecx0 && ecx0->m_item; ecx0 = ecx0->m_pkNext) {
        v2 = ecx0->m_item;
        if (v2 && v2->GetScript()) {
            return v2->GetScript();
        }
    }
    return nullptr;
}

ScriptLocals* ItemChange::GetScriptLocals() {
    for (ExtraDataList* v2 : pExtraObjectList->ItemsUntilNull()) {
        if (v2 && v2->GetScriptLocals()) {
            return v2->GetScriptLocals();
        }
    }
    return 0;
}

const char* ItemChange::GetIconOne(TESObjectREFR* apRef) {
    const char* Icon = 0;
    TESForm* ExtraOwnership_ = GetItemOwnership();
    TESBipedModelForm* v7 = dynamic_cast<TESBipedModelForm*>(pContainerObj);
    if (ExtraOwnership_) {
        if (v7) {
            TESActorBase* v5 =
                static_cast<TESActorBase*>(FNVD_DYNAMIC_CAST_VOID(TESNPC)(ExtraOwnership_, RTTI_TESForm, RTTI_TESNPC));
            if (v5) {
                TESActorBase::SEX Sex = v5->GetSex();
                Icon = v7->GetIcon(Sex);
            }
        }
    }
    if (!Icon || !*Icon) {
        Icon = TESTexture::GetTextureName(pContainerObj, apRef);
    }
    if (Icon && *Icon) {
        return Icon;
    }
    else {
        return sMissingImage.String();
    }
}

const char* ItemChange::GetFullName() {
    TESFullName* pfull = dynamic_cast<TESFullName*>(pContainerObj);
    if (pfull && pfull->GetFullNameLength()) {
        return pfull->GetFullName();
    }
    else {
        return sMissingName.String();
    }
}

bool ItemChange::HasModEffectActive(uint8_t eEffect) {
    BSSimpleList<ExtraDataList*>* ecx0 = pExtraObjectList;
    if (!pExtraObjectList) {
        return 0;
    }
    if (!ecx0->m_item) {
        return 0;
    }
    ExtraDataList* v5 = ecx0->m_item;
    if (!v5) {
        return 0;
    }
    TESObjectWEAP* pContainerObj = static_cast<TESObjectWEAP*>(this->pContainerObj);
    if (v5->GetWeaponModSlotActive(1u) && pContainerObj->GetModEffect(1u) == eEffect) {
        return 1;
    }
    if (v5->GetWeaponModSlotActive(2u) && pContainerObj->GetModEffect(2u) == eEffect) {
        return 1;
    }
    return v5->GetWeaponModSlotActive(4u) && pContainerObj->GetModEffect(4u) == eEffect;
}

bool ItemChange::HasModEffectActive(uint8_t eEffect, float& fValue) {
    TESObjectWEAP* pContainerObj;
    BSSimpleList<ExtraDataList*>* ecx0 = pExtraObjectList;
    if (!pExtraObjectList) {
        return (bool)0;
    }
    if (!ecx0->m_item) {
        return (bool)0;
    }
    ExtraDataList* xDataList = ecx0->m_item;
    if (!xDataList) {
        return (bool)0;
    }
    pContainerObj = static_cast<TESObjectWEAP*>(this->pContainerObj);
    if (xDataList->GetWeaponModSlotActive(1u) && (((uint32_t)pContainerObj->GetModEffect(1)) == ((uint32_t)eEffect))) {
        fValue = (float)pContainerObj->GetModValue(1, 0);
        return (bool)1;
    }
    if (xDataList->GetWeaponModSlotActive(2u) && (((uint32_t)pContainerObj->GetModEffect(2)) == ((uint32_t)eEffect))) {
        fValue = (float)pContainerObj->GetModValue(2, 0);
        return (bool)1;
    }
    if ((!xDataList->GetWeaponModSlotActive(4u)) ||
        (((uint32_t)pContainerObj->GetModEffect(4)) != ((uint32_t)eEffect))) {
        return (bool)0;
    }
    fValue = (float)pContainerObj->GetModValue(4, 0);
    return (bool)1;
}

// The item's health (-1 when it has no health form): its extra list's health, else the form's (a weapon's with its
// max-health mod); as a percentage, min(health / max, 100) * 100, or 100 without an extra health. The integer max
// health is returned exactly, hence the double return.
double ItemChange::GetItemHealth(bool abPercentage) {
    TESHealthForm* pHealthForm = TESHealthForm::GetFormAsHealthForm(pContainerObj);
    if (!pHealthForm) {
        return -1.0f;
    }
    TESObjectWEAP* pWeapon = dynamic_cast<TESObjectWEAP*>(pContainerObj);
    int32_t iMaxHealth = pHealthForm->GetFormHealth();
    if (pWeapon) {
        iMaxHealth = pWeapon->GetFormHealth(HasModEffectActive(WEAPON_MOD_MAX_HEALTH));
    }
    BSSimpleList<ExtraDataList*>* pNode = pExtraObjectList;
    ExtraDataList* pList = nullptr;
    if (pNode) {
        pList = pNode->GetItem();
        if (pList) {
            pList = pNode->GetItem();
        }
    }
    // A health of exactly -1.0 goes to the default; NaN takes the extra's health.
    if (pList && !(pList->GetHealth() == -1.0)) {
        if (!abPercentage) {
            return pList->GetHealth();
        }
        float fRatio = static_cast<float>(static_cast<double>(pList->GetHealth()) / iMaxHealth);
        float fResult = static_cast<float>(BSMIN(100.0f, fRatio) * 100.0);
        return fResult;
    }
    if (abPercentage) {
        return 100.0f;
    }
    return iMaxHealth;
}

float ItemChange::GetModifiedAttackDamage(ActorValueOwner* apActor, float afDamageMultiplier, bool abIgnoreExplosion) {
    if (pContainerObj->GetFormType() == 40) {
        TESObjectWEAP* apWeapon = static_cast<TESObjectWEAP*>(pContainerObj);
        double ItemHealth = GetItemHealth(0);
        float afCondition = ItemHealth / static_cast<double>(TESHealthForm::GetFormHealth(apWeapon));
        return CombatFormulas::CalcWeaponDamage(
            apActor, apWeapon, afCondition, afDamageMultiplier, 0, abIgnoreExplosion, this, 1
        );
    }
    else {
        return 0.0;
    }
}

float ItemChange::GetModifiedDamageThreshold(ActorValueOwner* apActor) {
    float fDT = -1.0f;
    if (pContainerObj->GetFormType() == ARMO_ID) {
        TESObjectARMO* pArmor = static_cast<TESObjectARMO*>(pContainerObj);
        float fHealth = static_cast<float>(TESHealthForm::GetFormHealth(pArmor));
        double dCondition;
        if (fHealth == 0.0) {
            dCondition = 0.0;
        }
        else {
            dCondition = GetItemHealth(false) / fHealth;
        }
        float fCondition = static_cast<float>(dCondition);
        uint16_t usDT = static_cast<uint16_t>(static_cast<int32_t>(pArmor->GetDamageThreshold()));
        fDT = CombatFormulas::CalcArmorRating(usDT, fCondition);
    }
    return FloatCeil(fDT);
}

void ItemChange::LoadGame() {
    uint32_t iblockHeader;
    int32_t icount;
    uint16_t iblockSize;
    uint32_t iboundID;
    iblockSize = 0;
    uint8_t* istartBufferPosition = 0;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        pSaveLoadGameOLD->LoadGameDataOLD(&iblockHeader, 4u);
        if (iblockHeader != 0x424C4F4B) {
            LoadFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
            if (pheader) {
                TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
                uint32_t iFlags = pheader->iFlags;
                int32_t cVersion = pheader->cVersion;
                const char* v1 = pform->GetFormEditorID();
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Currently "
                    "loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp", 1607,
                    pheader->iFormID, v1, cVersion, iFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Current "
                    "version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp", 1607,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        istartBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->LoadGameDataOLD(&iblockSize, 2u);
    }
    pSaveLoadGameOLD->LoadNumericID(&iboundID, 4u);
    pSaveLoadGameOLD->LoadGameDataOLD(&iNumber, 4u);
    TESForm* FormByNumericID = TESForm::GetFormByNumericID(iboundID);
    pContainerObj = dynamic_cast<TESBoundObject*>(FormByNumericID);
    pExtraObjectList = 0;
    pSaveLoadGameOLD->LoadGameDataOLD(&icount, 4u);
    if (icount) {
        BSSimpleList<ExtraDataList*>* v14;
        BSSimpleList<ExtraDataList*>* v17 =
            static_cast<BSSimpleList<ExtraDataList*>*>(ourMalloc(sizeof(BSSimpleList<ExtraDataList*>)));
        if (v17) {
            v14 = ::new (v17) BSSimpleList<ExtraDataList*>;
        }
        else {
            v14 = 0;
        }
        pExtraObjectList = v14;
        for (int32_t i = 0; i < icount; ++i) {
            ExtraDataList* pextraDataList;
            ExtraDataList* apMemory = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
            if (apMemory) {
                pextraDataList = ::new (apMemory) ExtraDataList;
            }
            else {
                pextraDataList = 0;
            }
            pextraDataList->LoadGame(0, 0, 0);
            pExtraObjectList->AddTail(pextraDataList);
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* icurrentBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        LoadFormHeader* pheader_2 = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
        if (pheader_2) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader_2->iFormID);
            if (icurrentBufferPosition <= &istartBufferPosition[iblockSize]) {
                if (icurrentBufferPosition < &istartBufferPosition[iblockSize]) {
                    uint32_t iFlags = pheader_2->iFlags;
                    int32_t cVersion = pheader_2->cVersion;
                    const char* v4 = pform->GetFormEditorID();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading "
                        "form is %08X %s with version %i and flags %08X",
                        static_cast<int32_t>(&istartBufferPosition[iblockSize] - icurrentBufferPosition),
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp", 1643,
                        pheader_2->iFormID, v4, cVersion, iFlags
                    );
                }
            }
            else {
                uint32_t v11 = pheader_2->iFlags;
                int32_t v8 = pheader_2->cVersion;
                const char* v3 = pform->GetFormEditorID();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading form "
                    "is %08X %s with version %i and flags %08X",
                    static_cast<int32_t>(icurrentBufferPosition - &istartBufferPosition[iblockSize]),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp", 1643,
                    pheader_2->iFormID, v3, v8, v11
                );
            }
        }
        else {
            if (icurrentBufferPosition <= &istartBufferPosition[iblockSize]) {
                if (icurrentBufferPosition < &istartBufferPosition[iblockSize]) {
                    uint8_t CurrentVersion = pSaveLoadGameOLD->GetCurrentVersion();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is "
                        "%i",
                        static_cast<int32_t>(&istartBufferPosition[iblockSize] - icurrentBufferPosition),
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp", 1643,
                        CurrentVersion
                    );
                }
            }
            else {
                uint8_t v5 = pSaveLoadGameOLD->GetCurrentVersion();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                    static_cast<int32_t>(icurrentBufferPosition - &istartBufferPosition[iblockSize]),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp", 1643, v5
                );
            }
        }
    }
}

void ItemChange::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    uint32_t FormID_0 = apLoadGameBuffer->LoadFormID();
    TESForm* FormByNumericID = TESForm::GetFormByNumericID(FormID_0);
    pContainerObj = dynamic_cast<TESBoundObject*>(FormByNumericID);
    apLoadGameBuffer->LoadData(&iNumber, 4u);
    DeleteAllExtra();
    uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    if (isaveGameCount) {
        if (!pExtraObjectList) {
            BSSimpleList<ExtraDataList*>* v5;
            BSSimpleList<ExtraDataList*>* v8 =
                static_cast<BSSimpleList<ExtraDataList*>*>(ourMalloc(sizeof(BSSimpleList<ExtraDataList*>)));
            if (v8) {
                v5 = ::new (v8) BSSimpleList<ExtraDataList*>;
            }
            else {
                v5 = 0;
            }
            pExtraObjectList = v5;
        }
        for (uint32_t i = 0; i < isaveGameCount; ++i) {
            ExtraDataList* pextraDataList;
            ExtraDataList* apMemory = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
            if (apMemory) {
                pextraDataList = ::new (apMemory) ExtraDataList;
            }
            else {
                pextraDataList = 0;
            }
            pextraDataList->LoadGameBGS(apLoadGameBuffer);
            pExtraObjectList->AddTail(pextraDataList);
        }
    }
}

void ItemChange::SetItemOwnership(TESForm* apowner) {
    if (apowner) {
        if (pExtraObjectList && pExtraObjectList->m_item) {
            pExtraObjectList->m_item->SetOwnership(apowner);
        }
        else {
            ExtraDataList* pextra;
            ExtraDataList* apMemory = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
            if (apMemory) {
                pextra = ::new (apMemory) ExtraDataList;
            }
            else {
                pextra = 0;
            }
            ExtraDataList* pextra_2 = pextra;
            pextra->SetOwnership(apowner);
            if (!pExtraObjectList) {
                BSSimpleList<ExtraDataList*>* v2;
                BSSimpleList<ExtraDataList*>* v5 =
                    static_cast<BSSimpleList<ExtraDataList*>*>(ourMalloc(sizeof(BSSimpleList<ExtraDataList*>)));
                if (v5) {
                    v2 = ::new (v5) BSSimpleList<ExtraDataList*>;
                }
                else {
                    v2 = 0;
                }
                pExtraObjectList = v2;
            }
            pExtraObjectList->AddHead(pextra_2);
        }
    }
    else {
        if (pExtraObjectList) {
            ExtraDataList* pextra_3 = pExtraObjectList->m_item;
            if (pextra_3) {
                pExtraObjectList->Remove(pextra_3);
                if (pextra_3) {
                    delete pextra_3;
                }
            }
        }
    }
}

// SaveGame of the old save format. With save blocks, the block's size is written back as a 16-bit value over the slot
// reserved after the 'BLOK' marker.
void ItemChange::SaveGameTES() {
    uint32_t iblockHeader;
    int32_t icount;
    uint16_t iblockSize;
    uint32_t iboundID;
    iblockSize = 0;
    // The reserved size slot in the save buffer (untyped bytes there).
    void* pblockSize = 0;
    uint8_t* itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    if (bDebugSaveBuffer.Bool()) {
        itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        iblockHeader = 0x424C4F4B;
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockHeader, 4u);
        pblockSize = pSaveLoadGameOLD->m_pBuffer;
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockSize, 2u);
    }
    iboundID = pContainerObj->iFormID;
    pSaveLoadGameOLD->SaveNumericID(&iboundID, 4u);
    pSaveLoadGameOLD->SaveGameDataOLD(&iNumber, 4u);
    icount = 0;
    int32_t* pcount = static_cast<int32_t*>(pSaveLoadGameOLD->m_pBuffer);
    pSaveLoadGameOLD->SaveGameDataOLD(&icount, 4u);
    for (BSSimpleList<ExtraDataList*>* plist = pExtraObjectList; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
        plist->m_item->SaveGame(0, 0);
        ++icount;
    }
    *pcount = icount;
    if (bDebugSaveBuffer.Bool()) {
        uint8_t* BufferPointer = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            uint32_t iFlags = pheader->iFlags;
            const char* v1 = pform->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                static_cast<int32_t>(BufferPointer - itempStartPosition), pheader->iFormID, v1, iFlags, 1601,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s",
                static_cast<int32_t>(BufferPointer - itempStartPosition), 1601,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp"
            );
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* pcurrent = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        uint8_t* pblockStart = static_cast<uint8_t*>(pblockSize);
        if (pcurrent > pblockStart + 0xFFFF) {
            Warning(
                "SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp", 1601
            );
        }
        *static_cast<uint16_t*>(pblockSize) = static_cast<uint16_t>(pcurrent - pblockStart);
    }
}

void ItemChange::Copy(ItemChange* apCopy) {
    if (apCopy) {
        BSSimpleList<ExtraDataList*>* v2;
        pContainerObj = apCopy->pContainerObj;
        void* v4 = ourMalloc(sizeof(BSSimpleList<ExtraDataList*>));
        if (v4) {
            v2 = ::new (v4) BSSimpleList<ExtraDataList*>;
        }
        else {
            v2 = 0;
        }
        pExtraObjectList = v2;
        BSSimpleList<ExtraDataList*>* pkFirst = apCopy->pExtraObjectList;
        for (ExtraDataList*& rpItem : apCopy->pExtraObjectList->ItemsUntilNull()) {
            pExtraObjectList->AddHead(rpItem);
        }
    }
}

int32_t ItemChange::GetAmountNonDefaultExtra() {
    BSSimpleList<ExtraDataList*>* pkFirst = pExtraObjectList;
    int32_t icount = 0;
    while (pkFirst && pkFirst->m_item) {
        ExtraDataList* pExtra = pkFirst->m_item;
        int16_t Count = pExtra->GetCount();
        if (!pExtra->IsExtraDefaultforContainer(0) && Count > 0) {
            ++icount;
        }
        pkFirst = pkFirst->m_pkNext;
        if (pExtra->ItemsInList() == 1 && Count > 1) {
            pExtra->RemoveCount();
            pExtraObjectList->Remove(pExtra);
            if (pExtra) {
                delete pExtra;
            }
            icount = 0;
            pkFirst = pExtraObjectList;
        }
    }
    return icount;
}

// Removes the original-reference extra from each extra list, up to the first empty node.
void ItemChange::RemoveOriginalChanges() {
    for (BSSimpleList<ExtraDataList*>* pNode = pExtraObjectList; pNode && pNode->m_item; pNode = pNode->m_pkNext) {
        ExtraDataList* pList = pNode->m_item;
        if (pList && pList->GetOriginalReference()) {
            pList->RemoveOriginalReferenceExtra();
        }
    }
}

int32_t ItemChange::CountLeveledItems() {
    BSSimpleList<ExtraDataList*>* ecx0 = pExtraObjectList;
    int32_t v3 = 0;
    while (ecx0 && ecx0->m_item) {
        ExtraDataList* v2 = ecx0->m_item;
        if (v2) {
            if (v2->HasLeveledItem()) {
                v3 += v2->GetCount();
            }
        }
        ecx0 = ecx0->m_pkNext;
    }
    return v3;
}

ExtraLeveledItem* ItemChange::RemoveLeveledItemExtra() {
    BSSimpleList<ExtraDataList*>* pkFirst = pExtraObjectList;
    ExtraLeveledItem* pextralevel = 0;
    while (pkFirst && pkFirst->m_item) {
        ExtraDataList* pExtra = pkFirst->m_item;
        if (pExtra) {
            if (pExtra->GetLeveledItem()) {
                pextralevel = pExtra->GetLeveledItem();
                pExtra->RemoveExtra(pextralevel, 0);
                if (pExtra->IsExtraListEmpty()) {
                    pExtraObjectList->Remove(pExtra);
                }
                pkFirst = pExtraObjectList;
            }
        }
        pkFirst = pkFirst->m_pkNext;
    }
    return pextralevel;
}

void ItemChange::SetItemHealth(float afHealth, InventoryChanges* pchange, ExtraDataList* apextra, bool abdelete) {
    TESHealthForm* phealth = dynamic_cast<TESHealthForm*>(pContainerObj);
    int32_t iMaxHealth = TESHealthForm::GetFormHealth(pContainerObj);
    int32_t ihealth = static_cast<int32_t>(GetItemHealth(0));
    if (pchange->pRef) {
        TESObjectREFR* Ref = pchange->pRef;
        Ref->AddChange(32);
    }
    if (phealth) {
        if (!(afHealth < static_cast<double>(iMaxHealth)) && ihealth <= iMaxHealth) {
            for (ExtraDataList* pextra_6 : pExtraObjectList->ItemsUntilNull()) {
                if (pextra_6 && pextra_6 == apextra) {
                    pextra_6->RemoveHealth();
                    if (pextra_6->IsExtraListEmpty() && abdelete) {
                        pchange->GetObjectInList(pContainerObj, 1, 0)->pExtraObjectList->Remove(pextra_6);
                        if (pextra_6) {
                            delete pextra_6;
                        }
                    }
                    return;
                }
            }
        }
        else {
            BSSimpleList<ExtraDataList*>* pkFirst = pExtraObjectList;
            if (pkFirst) {
                ExtraDataList* pextra_4;
                int32_t item = 0;
                while (pkFirst && pkFirst->m_item) {
                    ExtraDataList* pextra_3 = pkFirst->m_item;
                    if (pextra_3 && pextra_3 == apextra) {
                        pextra_3->SetHealth(afHealth);
                        return;
                    }
                    pkFirst = pkFirst->m_pkNext;
                }
                ExtraDataList* v10 = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
                if (v10) {
                    pextra_4 = ::new (v10) ExtraDataList;
                }
                else {
                    pextra_4 = 0;
                }
                ExtraDataList* pextra_5 = pextra_4;
                pextra_4->SetHealth(afHealth);
                pExtraObjectList->AddHead(pextra_5);
            }
            else {
                BSSimpleList<ExtraDataList*>* pkFirst_2;
                ExtraDataList* pextra;
                ExtraDataList* apMemory = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
                if (apMemory) {
                    pextra = ::new (apMemory) ExtraDataList;
                }
                else {
                    pextra = 0;
                }
                ExtraDataList* pextra_2 = pextra;
                BSSimpleList<ExtraDataList*>* v11 =
                    static_cast<BSSimpleList<ExtraDataList*>*>(ourMalloc(sizeof(BSSimpleList<ExtraDataList*>)));
                if (v11) {
                    pkFirst_2 = ::new (v11) BSSimpleList<ExtraDataList*>;
                }
                else {
                    pkFirst_2 = 0;
                }
                pextra_2->SetCharge(afHealth);
                pkFirst_2->AddHead(pextra_2);
                pExtraObjectList = pkFirst_2;
            }
        }
    }
}

void ItemChange::SetIsModding() {
    BSSimpleList<ExtraDataList*>* pkFirst = pExtraObjectList;
    if (!pkFirst) {
        BSSimpleList<ExtraDataList*>* pkFirst_2;
        void* v5 = ourMalloc(sizeof(BSSimpleList<ExtraDataList*>));
        if (v5) {
            pkFirst_2 = ::new (v5) BSSimpleList<ExtraDataList*>;
        }
        else {
            pkFirst_2 = 0;
        }
        pkFirst = pkFirst_2;
        pExtraObjectList = pkFirst_2;
    }
    if (pkFirst->m_item) {
        if (pkFirst) {
            if (pkFirst->m_item) {
                ExtraDataList* pextra_3 = pkFirst->m_item;
                if (pextra_3) {
                    pextra_3->SetIsModding(1);
                }
            }
        }
    }
    else {
        ExtraDataList* pextra;
        ExtraDataList* apMemory = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
        if (apMemory) {
            pextra = ::new (apMemory) ExtraDataList;
        }
        else {
            pextra = 0;
        }
        ExtraDataList* pextra_2 = pextra;
        pextra->SetIsModding(1);
        pExtraObjectList->AddHead(pextra_2);
    }
}

// Clears the modding flag of the first extra list.
void ItemChange::RemoveIsModding() {
    BSSimpleList<ExtraDataList*>* pNode = pExtraObjectList;
    if (pNode && pNode->m_item) {
        ExtraDataList* pList = pNode->m_item;
        if (pList) {
            pList->RemoveIsModding();
        }
    }
}

float ItemChange::GetConditionScaledDR(ActorValueOwner* apActor) {
    float v10 = -1.0f;
    if (this->pContainerObj->GetFormType() == 24) {
        double v5;
        TESObjectARMO* pContainerObj = static_cast<TESObjectARMO*>(this->pContainerObj);
        float FormHealth = static_cast<float>(TESHealthForm::GetFormHealth(pContainerObj));
        if (FormHealth == 0.0) {
            v5 = 0.0;
        }
        else {
            v5 = GetItemHealth(0) / FormHealth;
        }
        float afcondition = v5;
        int32_t ArmorRating = static_cast<int32_t>(pContainerObj->GetArmorRating());
        v10 = CombatFormulas::CalcArmorRating(ArmorRating, afcondition);
    }
    return FloatCeil(v10);
}

int32_t ItemChange::GetTypeforMenu() {
    uint32_t FormType = pContainerObj->GetFormType();
    if (FormType == 24 || FormType == 26) {
        return 4;
    }
    if (FormType == 40) {
        return 3;
    }
    return 5;
}

uint16_t ItemChange::GetSaveSize() {
    int16_t v7 = 0;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        v7 = 6;
    }
    uint16_t isize = v7 + 12;
    for (BSSimpleList<ExtraDataList*>* plist = pExtraObjectList; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
        ExtraDataList** v1 = &plist->m_item;
        isize += (*v1)->GetSaveSize(0, 0);
    }
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s", isize,
                pheader->iFormID, pform->GetFormEditorID(), pheader->iFlags, 1570,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s", isize, 1570,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp"
            );
        }
    }
    return isize;
}

void ItemChange::InitLoadGame_0() {
    for (BSSimpleList<ExtraDataList*>* ecx0 = pExtraObjectList; ecx0 && !ecx0->IsEmpty(); ecx0 = ecx0->m_pkNext) {
        ExtraDataList* v2 = ecx0->m_item;
        if (v2) {
            v2->InitLoadGameTES(0, 0, 0, pContainerObj);
        }
    }
}

void ItemChange::operator delete(void* apMemory) {
    ourFree(apMemory);
}

InventoryChanges::IterData::~IterData() {
    if (pFilteredList) {
        pFilteredList->RemoveAll();
        delete pFilteredList;
        pFilteredList = nullptr;
    }
}

void InventoryChanges::IterData::operator delete(void* apMemory) {
    ourFree(apMemory);
}

InventoryChanges::InventoryChanges(TESObjectREFR* apRef)
  : pListofChanges(nullptr), pRef(apRef), fcontainerweight(-1.0f), bcountdirty(false) {
    AutoMemContext tempMemContext(MEM_CONTEXT_INVENTORY);
    pListofChanges = new BSSimpleList<ItemChange*>;
    if (!InventoryChanges::pTempRef) {
        InventoryChanges::pTempRef = new TESObjectREFR;
        InventoryChanges::pTempRef->SetTemporary();
    }
    fpreviousContainerWeight = fcontainerweight;
    ResetCachedWeights();
}

// The changes and their list are the inventory's own allocations: each change's extra data is deleted with it.
InventoryChanges::~InventoryChanges() {
    BSSimpleList<ItemChange*>* pkFirst = pListofChanges;
    while (pkFirst && pkFirst->m_item) {
        ItemChange* pitem = pkFirst->m_item;
        pkFirst = pkFirst->m_pkNext;
        pitem->DeleteAllExtra();
        delete pitem;
    }
    pListofChanges->RemoveAll();
    if (pRef) {
        pRef->RemoveChange(CHANGE_REFR_INVENTORY | CHANGE_REFR_LEVELED_INVENTORY);
    }
    delete pListofChanges;
}

bool InventoryChanges::ContainerHasObjects(
    TESBoundObject* apObj,
    PACKAGE_OBJECT_TYPE aeObjectType,
    int32_t ainumber,
    PACKAGE_OBJECT_TYPE& aeMatchId,
    uint32_t aId,
    TESObjectREFR* apRef
) {
    int32_t iamount = 0;
    if (aId) {
        return HasObjectId(aId);
    }
    Actor* pactor = 0;
    if (apRef) {
        if (apRef->IsActor()) {
            pactor = static_cast<Actor*>(apRef);
        }
    }
    for (BSSimpleList<ContainerObject*>* pList = &GetOriginalContainer()->objectList; pList && !pList->IsEmpty();
         pList = pList->m_pkNext) {
        TESBoundObject* pObj = pList->m_item->pObj;
        if (pObj &&
            ((apObj && pObj == apObj) ||
             (!apObj && (aeObjectType && TESPackage::FormMatchesPackageObjectType(pObj, aeObjectType))))) {
            ItemChange* pitem = GetObjectInList(pObj, 1, 0);
            if (!pitem) {
                if (pactor) {
                    if (pactor->pCurrentProcess) {
                        BaseProcess* CurrentAIProcess = pactor->pCurrentProcess;
                        CurrentAIProcess->SetItemBeingUsed(pObj);
                    }
                }
                aeMatchId = TESPackage::GetObjectTypeFromForm(pObj);
                return 1;
            }
            if (pList->m_item->i_count < 0) {
                return 1;
            }
            uint32_t FormHealth_0 = pitem->iNumber;
            iamount += pList->m_item->i_count + FormHealth_0;
            if (pactor) {
                if (pactor->pCurrentProcess) {
                    BaseProcess* v12 = pactor->pCurrentProcess;
                    v12->SetItemBeingUsed(pitem->pContainerObj);
                }
            }
            aeMatchId = TESPackage::GetObjectTypeFromForm(pitem->pContainerObj);
        }
    }
    if (iamount >= ainumber) {
        return 1;
    }
    if (!apObj) {
        for (BSSimpleList<ItemChange*>* pInvList = pListofChanges;
             pInvList && (!pInvList->IsEmpty() && pInvList->m_item); pInvList = pInvList->m_pkNext) {
            TESBoundObject* pObj_2 = pInvList->m_item->pContainerObj;
            if (pObj_2 &&
                (aeObjectType &&
                 (TESPackage::FormMatchesPackageObjectType(pObj_2, aeObjectType) &&
                  pInvList->m_item->iNumber >= ainumber))) {
                if (pactor) {
                    if (pactor->pCurrentProcess) {
                        BaseProcess* v9 = pactor->pCurrentProcess;
                        v9->SetItemBeingUsed(pInvList->m_item->pContainerObj);
                    }
                }
                aeMatchId = TESPackage::GetObjectTypeFromForm(pInvList->m_item->pContainerObj);
                return 1;
            }
        }
        return 0;
    }
    ItemChange* pitem_2 = GetObjectInList(apObj, 1, 0);
    if (!pitem_2 || pitem_2->iNumber < ainumber) {
        return 0;
    }
    if (pactor) {
        if (pactor->pCurrentProcess) {
            BaseProcess* v10 = pactor->pCurrentProcess;
            v10->SetItemBeingUsed(pitem_2->pContainerObj);
        }
    }
    aeMatchId = TESPackage::GetObjectTypeFromForm(pitem_2->pContainerObj);
    return 1;
}

void InventoryChanges::RemoveGold(TESObjectREFR* apRef, int32_t aiamount, TESObjectREFR* apCont) {
    BSSimpleList<ContainerObject*>* pList;
    TESBoundObject* pObj;
    BSSimpleList<ContainerObject*>* pList_2;
    TESContainer* porigContainer = GetOriginalContainer();
    if (porigContainer) {
        pList = &porigContainer->objectList;
    }
    else {
        pList = 0;
    }
    for (pList_2 = pList; pList_2 && !pList_2->IsEmpty(); pList_2 = pList_2->m_pkNext) {
        pObj = pList_2->m_item->pObj;
        if (pObj && TESContainer::IsGold(pObj)) {
        LABEL_9:;
            RemoveItem(apRef, pObj, 0, aiamount, 0, 0, apCont, 0, 0, 1, 0, 0);
            return;
        }
    }
    for (BSSimpleList<ItemChange*>* pListInv = pListofChanges; pListInv && !pListInv->IsEmpty();
         pListInv = pListInv->m_pkNext) {
        pObj = pListInv->m_item->pContainerObj;
        if (pObj && TESContainer::IsGold(pObj)) {
            goto LABEL_9;
        }
    }
}

int32_t InventoryChanges::GetGoldAmount() {
    BSSimpleList<ContainerObject*>* pList;
    int32_t iamount = 0;
    TESContainer* porigContainer = GetOriginalContainer();
    if (porigContainer) {
        pList = &porigContainer->objectList;
    }
    else {
        pList = 0;
    }
    for (BSSimpleList<ContainerObject*>* pList_2 = pList; pList_2 && !pList_2->IsEmpty(); pList_2 = pList_2->m_pkNext) {
        TESBoundObject* pObj = pList_2->m_item->pObj;
        if (pObj) {
            if (TESContainer::IsGold(pObj)) {
                ItemChange* pitem = GetObjectInList(pObj, 1, 0);
                if (pitem) {
                    uint32_t FormHealth_0 = pitem->iNumber;
                    iamount += pList_2->m_item->i_count + FormHealth_0;
                }
                else {
                    iamount += pList_2->m_item->i_count;
                    if (iamount < 0) {
                        iamount = -iamount;
                    }
                }
            }
        }
    }
    for (BSSimpleList<ItemChange*>* pListInv = pListofChanges; pListInv && !pListInv->IsEmpty();
         pListInv = pListInv->m_pkNext) {
        TESBoundObject* pObj_2 = pListInv->m_item->pContainerObj;
        if (pObj_2) {
            if (TESContainer::IsGold(pObj_2) &&
                (!porigContainer || (!porigContainer->HasObject(pObj_2) && pListInv->m_item->iNumber > 0))) {
                iamount += pListInv->m_item->iNumber;
            }
        }
    }
    return iamount;
}

int32_t InventoryChanges::GetCount(bool abcountplayable) {
    int32_t icount = 0;
    if (GetOriginalContainer()) {
        for (ContainerObject* pobject : (&GetOriginalContainer()->objectList)->ItemsUntilNull()) {
            TESLevItem* pObj = dynamic_cast<TESLevItem*>(pobject->pObj);
            TESBipedModelForm* pbipedForm = 0;
            TESBoundObject* v10 = pobject->pObj;
            TESContainer* OriginalContainer = GetOriginalContainer();
            int32_t ioriginalcount = OriginalContainer->GetObjectCount(v10);
            if (!abcountplayable) {
                int32_t FormType = pobject->pObj->GetFormType();
                if (FormType == 40) {
                    if (!static_cast<TESObjectWEAP*>(pobject->pObj)->TESObjectWEAP::GetIsPlayable()) {
                        pobject = 0;
                    }
                }
                else {
                    if (FormType == 41) {
                        if (!static_cast<TESAmmo*>(pobject->pObj)->TESAmmo::GetIsPlayable()) {
                            pobject = 0;
                        }
                    }
                    else {
                        pbipedForm = TESBipedModelForm::GetFormAsBipedModel(pobject->pObj);
                        if (pbipedForm) {
                            if (!pbipedForm->GetPlayable()) {
                                pobject = 0;
                            }
                        }
                    }
                }
            }
            if (pobject) {
                if (!pObj) {
                    ItemChange* pitem = GetObjectInList(pobject->pObj, 1, 0);
                    int32_t iorgCount = ioriginalcount;
                    if (ioriginalcount < 0) {
                        iorgCount = -iorgCount;
                    }
                    if (pitem &&
                        (iorgCount + pitem->iNumber > 0 && (ioriginalcount >= 0 || pitem->iNumber > ioriginalcount))) {
                        if (pitem->pExtraObjectList && pitem->pExtraObjectList->m_item) {
                            icount += pitem->GetAmountNonDefaultExtra();
                            if (pitem->HasStolenData()) {
                                ++icount;
                            }
                            if (pitem->GetExtraTotalCount(0) < pitem->iNumber + ioriginalcount) {
                                if (pitem->pExtraObjectList->m_item) {
                                    if (!pitem->GetWorn(0)) {
                                        ExtraDataList** v3 = &pitem->pExtraObjectList->m_item;
                                        if (!(*v3)->HasLeveledItem()) {
                                            ++icount;
                                        }
                                    }
                                }
                            }
                        }
                        else {
                            ++icount;
                        }
                    }
                    else {
                        if (!pitem) {
                            ++icount;
                        }
                    }
                }
            }
        }
    }
    BSSimpleList<ItemChange*>* pkFirst = pListofChanges;
    int32_t itempcount = icount;
    bool bflag = 1;
    while (pkFirst && pkFirst->m_item) {
        ItemChange* pitem_2 = pkFirst->m_item;
        bflag = 1;
        bool bcontinue = 1;
        if (pitem_2) {
            ExtraDataList** v4;
            if (pitem_2->pExtraObjectList &&
                (pitem_2->pExtraObjectList->m_item &&
                 (v4 = &pitem_2->pExtraObjectList->m_item, (*v4)->HasLeveledItem()))) {
                bcontinue = 1;
            }
            else {
                if (GetOriginalContainer()) {
                    TESBoundObject* pContainerObj = pitem_2->pContainerObj;
                    TESContainer* v5 = GetOriginalContainer();
                    if (v5->HasObject(pContainerObj)) {
                        bcontinue = 0;
                    }
                }
            }
            if (!abcountplayable) {
                int32_t v12 = pitem_2->pContainerObj->GetFormType();
                if (v12 == 40) {
                    if (!static_cast<TESObjectWEAP*>(pitem_2->pContainerObj)->TESObjectWEAP::GetIsPlayable()) {
                        bcontinue = 0;
                    }
                }
                else {
                    if (v12 == 41) {
                        if (!static_cast<TESAmmo*>(pitem_2->pContainerObj)->TESAmmo::GetIsPlayable()) {
                            bcontinue = 0;
                        }
                    }
                    else {
                        TESBipedModelForm* pbipedForm_2 =
                            TESBipedModelForm::GetFormAsBipedModel(pitem_2->pContainerObj);
                        if (pbipedForm_2) {
                            bcontinue = pbipedForm_2->GetPlayable();
                        }
                    }
                }
            }
        }
        if (pitem_2) {
            if (pitem_2->iNumber > 0 && bcontinue) {
                int32_t AmountNonDefaultExtra = pitem_2->GetAmountNonDefaultExtra();
                icount += AmountNonDefaultExtra;
                if (pitem_2->HasStolenData()) {
                    ++icount;
                }
                int32_t ExtraTotalCount = pitem_2->GetExtraTotalCount(0);
                if (ExtraTotalCount < pitem_2->iNumber) {
                    ++icount;
                }
                if ((pitem_2->iNumber & 0x80000000u) != 0) {
                    ExtraDataList** v8;
                    if (!pitem_2->pExtraObjectList ||
                        (!pitem_2->pExtraObjectList->m_item ||
                         (v8 = &pitem_2->pExtraObjectList->m_item, !(*v8)->HasLeveledItem()))) {
                        pListofChanges->Remove(pitem_2);
                        pitem_2->DeleteAllExtra();
                        if (pitem_2) {
                            delete pitem_2;
                        }
                        icount = itempcount;
                        pkFirst = pListofChanges;
                        if (pkFirst && pkFirst->m_item) {
                            pitem_2 = pkFirst->m_item;
                        }
                        else {
                            pitem_2 = 0;
                        }
                        bflag = 0;
                    }
                }
            }
        }
        if (bflag) {
            pkFirst = pkFirst->m_pkNext;
        }
    }
    return icount + 1;
}

bool InventoryChanges::WearingVoiceModulateItem() {
    bool bmodulate = 0;
    for (ItemChange* pitem : pListofChanges->ItemsUntilNull()) {
        if (pitem) {
            if (pitem->pContainerObj->GetFormType() == ARMO_ID && !bmodulate) {
                TESObjectARMO* parmor = static_cast<TESObjectARMO*>(pitem->pContainerObj);
                for (BSSimpleList<ExtraDataList*>* pkFirst = pitem->pExtraObjectList;
                     pkFirst && (pkFirst->m_item && !bmodulate); pkFirst = pkFirst->m_pkNext) {
                    ExtraDataList* pExtra = pkFirst->m_item;
                    if (pExtra) {
                        if (pExtra->GetWorn(0)) {
                            bmodulate = parmor->ModulatesVoice();
                        }
                    }
                }
            }
        }
    }
    return bmodulate;
}

void InventoryChanges::CreateExtraDataList(
    TESBoundObject* apObject,
    int32_t aiNumber,
    Actor* apActor,
    ExtraDataList* apExtra,
    ItemChange* pChange
) {
    ItemChange* pItem;
    bool bCreateNew;
    ExtraDataList* pExtra;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_INVENTORY, 1, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp", 2947
    );
    if (pRef) {
        pRef->AddChange(32);
    }
    pItem = GetObjectInList(apObject, 1, 0);
    TESContainer* OriginalContainer = GetOriginalContainer();
    int32_t iorgCount = OriginalContainer->GetObjectCount(apObject);
    int32_t iorgCount_2 = iorgCount;
    if (pItem) {
        iorgCount_2 = pItem->iNumber + iorgCount;
    }
    int32_t v34 = 0;
    if (pItem) {
        if (pItem->pExtraObjectList) {
            int32_t ExtraTotalCount = pItem->GetExtraTotalCount(0);
            v34 = pItem->GetExtraTotalDefaultCount() + ExtraTotalCount;
        }
    }
    int32_t idefaultcount = iorgCount_2 - v34;
    if (pItem) {
        if (pItem->pExtraObjectList) {
            if (pItem->pExtraObjectList->m_item) {
                if (pItem->pExtraObjectList->m_item->HasLeveledItem()) {
                    idefaultcount = iorgCount - v34;
                }
            }
            bool bresetflag = 0;
            if (apExtra) {
                if (!pItem->pExtraObjectList->IsInList(apExtra)) {
                    for (ExtraDataList* pextra : pItem->pExtraObjectList->ItemsUntilNull()) {
                        if (pextra && !pextra->CompareList(apExtra)) {
                            apExtra = pextra;
                            bresetflag = 1;
                            break;
                        }
                    }
                    if (!bresetflag) {
                        apExtra = 0;
                    }
                }
            }
        }
    }
    bCreateNew = 1;
    pExtra = 0;
    if ((apExtra || idefaultcount <= 0) && pItem) {
        if (pItem->GetWorn(0)) {
            return;
        }
        BSSimpleList<ExtraDataList*>* pkFirst = pItem->pExtraObjectList;
        while (pkFirst && bCreateNew) {
            pExtra = pkFirst->m_item;
            if (apExtra && apExtra == pExtra) {
                bool v19 = apObject->GetFormType() == WEAP_ID &&
                    static_cast<TESObjectWEAP*>(apObject)->TESObjectWEAP::IsThrownWeapon();
                if (pExtra->GetCount() <= 1 || (apObject->GetFormType() == AMMO_ID || v19)) {
                    pExtra->SetCount(pItem->iNumber);
                }
                else {
                    ExtraDataList* v18;
                    ExtraDataList* v27 = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
                    if (v27) {
                        v18 = ::new (v27) ExtraDataList;
                    }
                    else {
                        v18 = 0;
                    }
                    apExtra = v18;
                    v18->CopyList(pExtra);
                    apExtra->SetCount(1);
                    int16_t Count = pExtra->GetCount();
                    pExtra->SetCount(Count - 1);
                    if (pExtra->GetHotKey() >= 0) {
                        pExtra->RemoveHotKey();
                    }
                }
                bCreateNew = 0;
            }
            else {
                if (!pExtra || apExtra) {
                    pkFirst = pkFirst->m_pkNext;
                }
                else {
                    if (pExtra->GetCount() == 1) {
                        apExtra = pExtra;
                    }
                    else {
                        bool v17 = apObject->GetFormType() == WEAP_ID &&
                            static_cast<TESObjectWEAP*>(apObject)->TESObjectWEAP::IsThrownWeapon();
                        if (pExtra->GetCount() <= 1 || (apObject->GetFormType() == AMMO_ID || v17)) {
                            apExtra = pExtra;
                        }
                        else {
                            ExtraDataList* v16;
                            ExtraDataList* v26 = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
                            if (v26) {
                                v16 = ::new (v26) ExtraDataList;
                            }
                            else {
                                v16 = 0;
                            }
                            apExtra = v16;
                            v16->CopyList(pExtra);
                            apExtra->SetCount(1);
                            int16_t v11 = pExtra->GetCount();
                            pExtra->SetCount(v11 - 1);
                            if (pExtra->GetHotKey() >= 0) {
                                pExtra->RemoveHotKey();
                            }
                        }
                    }
                    bCreateNew = 0;
                }
            }
        }
    }
    else {
        if (!pItem) {
            ItemChange* pitem;
            ItemChange* v28 = static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
            if (v28) {
                pitem = ::new (v28) ItemChange(apObject, 0);
            }
            else {
                pitem = 0;
            }
            pItem = pitem;
            bCreateNew = 1;
            pListofChanges->AddTail(pItem);
            apExtra = 0;
        }
    }
    if (apExtra) {
        apExtra->SetCount(aiNumber);
        bCreateNew = 0;
        if (!pItem->pExtraObjectList) {
            BSSimpleList<ExtraDataList*>* v15;
            BSSimpleList<ExtraDataList*>* v25 =
                static_cast<BSSimpleList<ExtraDataList*>*>(ourMalloc(sizeof(BSSimpleList<ExtraDataList*>)));
            if (v25) {
                v15 = ::new (v25) BSSimpleList<ExtraDataList*>;
            }
            else {
                v15 = 0;
            }
            pItem->pExtraObjectList = v15;
        }
        if (!pItem->pExtraObjectList->IsInList(apExtra)) {
            pItem->pExtraObjectList->AddHead(apExtra);
        }
        if (pChange) {
            if (!pChange->pExtraObjectList->IsInList(apExtra)) {
                pChange->pExtraObjectList->AddHead(apExtra);
            }
        }
        pExtra = apExtra;
    }
    if (bCreateNew) {
        ExtraDataList* pExtra_2;
        ExtraDataList* v24 = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
        if (v24) {
            pExtra_2 = ::new (v24) ExtraDataList;
        }
        else {
            pExtra_2 = 0;
        }
        pExtra = pExtra_2;
        if (aiNumber > 1) {
            pExtra->SetCount(aiNumber);
        }
        if (!pItem->pExtraObjectList) {
            BSSimpleList<ExtraDataList*>* v13;
            BSSimpleList<ExtraDataList*>* v23 =
                static_cast<BSSimpleList<ExtraDataList*>*>(ourMalloc(sizeof(BSSimpleList<ExtraDataList*>)));
            if (v23) {
                v13 = ::new (v23) BSSimpleList<ExtraDataList*>;
            }
            else {
                v13 = 0;
            }
            pItem->pExtraObjectList = v13;
        }
        if (!pItem->pExtraObjectList->IsInList(pExtra)) {
            pItem->pExtraObjectList->AddHead(pExtra);
        }
        if (pChange) {
            if (!pChange->pExtraObjectList) {
                BSSimpleList<ExtraDataList*>* v12;
                BSSimpleList<ExtraDataList*>* v22 =
                    static_cast<BSSimpleList<ExtraDataList*>*>(ourMalloc(sizeof(BSSimpleList<ExtraDataList*>)));
                if (v22) {
                    v12 = ::new (v22) BSSimpleList<ExtraDataList*>;
                }
                else {
                    v12 = 0;
                }
                pChange->pExtraObjectList = v12;
            }
            if (!pChange->pExtraObjectList->IsInList(pExtra)) {
                pChange->pExtraObjectList->AddHead(pExtra);
            }
        }
    }
}

// Calls apCallback on every item of the inventory (the container's entries the changes do not hold, then the changes
// with their counts added to the container's), through a temporary ItemChange, until it returns true; with bit 0 of
// aiIterateFlags, quest items, unplayable biped forms and embedded weapons are skipped. Returns the number of calls.
uint32_t InventoryChanges::IterateInventory(
    InventoryChanges::IterateInventoryCallback apCallback,
    void* apData,
    uint32_t aiIterateFlags
) {
    uint32_t uiCalls = 0;
    BSSimpleList<ContainerObject*>* pContIter = &GetOriginalContainer()->objectList;
    while (pContIter) {
        ContainerObject* pContainerObject = pContIter->m_item;
        pContIter = pContIter->m_pkNext;
        if (pContainerObject) {
            if (!GetObjectInList(pContainerObject->pObj, 1, 0)) {
                TESBipedModelForm* pbipedform;
                if ((aiIterateFlags & 1) == 0 ||
                    (!pContainerObject->pObj->GetQuestObject() &&
                     (((pbipedform = TESBipedModelForm::GetFormAsBipedModel(pContainerObject->pObj)) == 0 ||
                       pbipedform->GetPlayable()) &&
                      (pContainerObject->pObj->GetFormType() != WEAP_ID ||
                       !static_cast<TESObjectWEAP*>(pContainerObject->pObj)->IsEmbeddedWeapon())))) {
                    ItemChange kItemChange;
                    kItemChange.iNumber = pContainerObject->i_count;
                    kItemChange.pContainerObj = pContainerObject->pObj;
                    ++uiCalls;
                    if (apCallback(&kItemChange, apData)) {
                        return uiCalls;
                    }
                }
            }
        }
    }
    BSSimpleList<ItemChange*>* pchangeList = pListofChanges;
    while (pchangeList) {
        ItemChange* pitemChange = pchangeList->m_item;
        pchangeList = pchangeList->m_pkNext;
        if (pitemChange) {
            TESBipedModelForm* pbipedform_2;
            TESObjectWEAP* pweapon_2 = static_cast<TESObjectWEAP*>(pitemChange->pContainerObj);
            if ((aiIterateFlags & 1) == 0 ||
                (!pweapon_2->GetQuestObject() &&
                 (((pbipedform_2 = TESBipedModelForm::GetFormAsBipedModel(pweapon_2)) == 0 ||
                   pbipedform_2->GetPlayable()) &&
                  (pweapon_2->GetFormType() != WEAP_ID || !pweapon_2->IsEmbeddedWeapon())))) {
                uint32_t FormHealth_0 = pitemChange->iNumber;
                TESObjectWEAP* v7 = pweapon_2;
                TESContainer* OriginalContainer = GetOriginalContainer();
                int32_t iitemCount = OriginalContainer->GetObjectCount(v7) + FormHealth_0;
                if (iitemCount) {
                    ItemChange kItemChange;
                    kItemChange.pExtraObjectList = pitemChange->pExtraObjectList;
                    kItemChange.iNumber = iitemCount;
                    kItemChange.pContainerObj = pweapon_2;
                    ++uiCalls;
                    // The temporary shares the change's extra data lists, so they are cleared before the destructor
                    // runs.
                    if (apCallback(&kItemChange, apData)) {
                        kItemChange.pExtraObjectList = 0;
                        return uiCalls;
                    }
                    kItemChange.pExtraObjectList = 0;
                }
            }
        }
    }
    return uiCalls;
}

uint32_t InventoryChanges::GetInventoryValue(bool abCountQuest, bool abCountGold) {
    uint32_t iretval = 0;
    for (BSSimpleList<ContainerObject*>* pList = &GetOriginalContainer()->objectList; pList && !pList->IsEmpty();
         pList = pList->m_pkNext) {
        TESBoundObject* pObj = pList->m_item->pObj;
        if ((abCountQuest || !pObj->GetQuestObject()) && (abCountGold || pObj->iFormID != 15)) {
            int32_t ival = TESValueForm::GetFormValue(pObj);
            if (ival == -1) {
                ival = 0;
            }
            if (ival) {
                ItemChange* pitem = GetObjectInList(pObj, 1, 0);
                if (pitem) {
                    uint32_t FormHealth_0 = pitem->iNumber;
                    if (pList->m_item->i_count + FormHealth_0) {
                        uint32_t v4 = pitem->iNumber;
                        iretval += ival * (pList->m_item->i_count + v4);
                    }
                }
                else {
                    iretval += pList->m_item->i_count * ival;
                }
            }
        }
    }
    for (BSSimpleList<ItemChange*>* pInvList = pListofChanges; pInvList && (!pInvList->IsEmpty() && pInvList->m_item);
         pInvList = pInvList->m_pkNext) {
        ItemChange* pitem_2 = pInvList->m_item;
        TESBoundObject* pObj_2 = pitem_2->pContainerObj;
        if (pObj_2) {
            TESContainer* OriginalContainer;
            if (!GetOriginalContainer() ||
                (OriginalContainer = GetOriginalContainer(), !OriginalContainer->HasObject(pObj_2))) {
                if ((abCountQuest || !pObj_2->GetQuestObject()) && (abCountGold || pObj_2->iFormID != 15)) {
                    int32_t ival_2 = TESValueForm::GetFormValue(pObj_2);
                    if (ival_2 == -1) {
                        ival_2 = 0;
                    }
                    if (ival_2) {
                        if (pitem_2->iNumber) {
                            iretval += ival_2 * pitem_2->iNumber;
                        }
                    }
                }
            }
        }
    }
    return iretval;
}

void InventoryChanges::GetItemGroup(TESBoundObject* apObj, BSSimpleList<ItemChange*>* apReturnList) {
    if (apObj && apReturnList) {
        IterData* pIter = GetIterator();
        ItemChange* a2 = 0;
        char v4 = 0;
        while (1) {
            a2 = Iterate(pIter);
            if (a2) {
                if (a2->pContainerObj == apObj) {
                    v4 = 1;
                    apReturnList->AddHead(a2);
                }
                else {
                    if (a2) {
                        delete a2;
                    }
                    if (v4) {
                    LABEL_12:;
                        if (pIter) {
                            delete pIter;
                        }
                        return;
                    }
                }
            }
            if (!a2) {
                goto LABEL_12;
            }
        }
    }
}

TESBoundObject* InventoryChanges::GetObjectbyType(ENUM_FORM_ID aeFormID, int32_t& ainumb) {
    BSSimpleList<ContainerObject*>* pList = &GetOriginalContainer()->objectList;
    TESBoundObject* pObj = 0;
    while (pList && (!pObj && pList->m_item)) {
        pObj = pList->m_item->pObj;
        if (pObj && pObj->GetFormType() == aeFormID) {
            uint32_t v3;
            ItemChange* pitem = GetObjectInList(pObj, 1, 0);
            if (!pitem || (v3 = pitem->iNumber, pList->m_item->i_count + v3)) {
                if (pitem) {
                    uint32_t FormHealth_0 = pitem->iNumber;
                    ainumb = pList->m_item->i_count + FormHealth_0;
                }
                else {
                    ainumb = pList->m_item->i_count;
                }
            }
        }
        else {
            pObj = 0;
        }
        pList = pList->m_pkNext;
    }
    for (BSSimpleList<ItemChange*>* pListInv = pListofChanges; pListInv && (!pObj && pListInv->m_item);
         pListInv = pListInv->m_pkNext) {
        pObj = pListInv->m_item->pContainerObj;
        if (pObj && pObj->GetFormType() == aeFormID) {
            TESContainer* OriginalContainer = GetOriginalContainer();
            if (!OriginalContainer->HasObject(pObj) && pListInv->m_item->iNumber > 0) {
                ainumb = pListInv->m_item->iNumber;
            }
        }
        else {
            pObj = 0;
        }
    }
    return pObj;
}

void InventoryChanges::InitLoadGameTES() {
    for (BSSimpleList<ItemChange*>* ecx0 = pListofChanges; ecx0 && !ecx0->IsEmpty(); ecx0 = ecx0->m_pkNext) {
        ItemChange* v1 = ecx0->m_item;
        if (v1) {
            v1->InitLoadGame_0();
        }
    }
}

void InventoryChanges::SaveGame() {
    uint32_t iblockHeader;
    uint16_t icount;
    uint16_t iblockSize;
    iblockSize = 0;
    void* pblockSize = nullptr;
    uint8_t* itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    if (bDebugSaveBuffer.Bool()) {
        itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        // 'BLOK', the block header.
        iblockHeader = 0x424C4F4B;
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockHeader, 4u);
        pblockSize = pSaveLoadGameOLD->m_pBuffer;
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockSize, 2u);
    }
    icount = 0;
    uint16_t* pcount = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
    pSaveLoadGameOLD->SaveGameDataOLD(&icount, 2u);
    for (BSSimpleList<ItemChange*>* plist = pListofChanges; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
        ItemChange* pchange = plist->m_item;
        if (pchange) {
            pchange->SaveGameTES();
            ++icount;
        }
    }
    *pcount = icount;
    if (bDebugSaveBuffer.Bool()) {
        uint8_t* BufferPointer = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            uint32_t iFlags = pheader->iFlags;
            const char* v1 = pform->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                static_cast<int32_t>(BufferPointer - itempStartPosition), pheader->iFormID, v1, iFlags, 10346,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s",
                static_cast<int32_t>(BufferPointer - itempStartPosition), 10346,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp"
            );
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* pcurrent = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        uint8_t* pblockStart = static_cast<uint8_t*>(pblockSize);
        if (pcurrent > pblockStart + 0xFFFF) {
            Warning(
                "SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp", 10346
            );
        }
        *static_cast<uint16_t*>(pblockSize) = static_cast<uint16_t>(pcurrent - pblockStart);
    }
}

// Loads the item changes of the old save format: a block header and size when the save uses blocks, then the count and
// the changes. A change whose object did not load is deleted. The block's end is checked against its size afterwards,
// with a warning on an underrun or overrun.
void InventoryChanges::LoadGame() {
    uint32_t iblockHeader;
    uint16_t icount;
    uint16_t iblockSize;
    iblockSize = 0;
    uint8_t* istartBufferPosition = 0;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        pSaveLoadGameOLD->LoadGameDataOLD(&iblockHeader, 4u);
        if (iblockHeader != 0x424C4F4B) {
            LoadFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
            if (pheader) {
                TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Currently "
                    "loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp", 10353,
                    pheader->iFormID, pform->GetFormEditorID(), pheader->cVersion, pheader->iFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Current "
                    "version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp", 10353,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        istartBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->LoadGameDataOLD(&iblockSize, 2u);
    }
    pSaveLoadGameOLD->LoadGameDataOLD(&icount, 2u);
    for (int32_t i = 0; i < icount; ++i) {
        ItemChange* pchange;
        ItemChange* apMemory = static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
        if (apMemory) {
            pchange = ::new (apMemory) ItemChange;
        }
        else {
            pchange = 0;
        }
        ItemChange* pchange_2 = pchange;
        pchange->LoadGame();
        if (pchange_2->pContainerObj) {
            pListofChanges->AddHead(pchange_2);
        }
        else {
            pchange_2->DeleteAllExtra();
            if (pchange_2) {
                delete pchange_2;
            }
        }
    }
    CleanUpBadData();
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* icurrentBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        LoadFormHeader* pheader_2 = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
        if (pheader_2) {
            TESForm* pform_2 = TESForm::GetFormByNumericID(pheader_2->iFormID);
            if (icurrentBufferPosition <= &istartBufferPosition[iblockSize]) {
                if (icurrentBufferPosition < &istartBufferPosition[iblockSize]) {
                    uint32_t iFlags = pheader_2->iFlags;
                    int32_t cVersion = pheader_2->cVersion;
                    const char* v2 = pform_2->GetFormEditorID();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading "
                        "form is %08X %s with version %i and flags %08X",
                        &istartBufferPosition[iblockSize] - icurrentBufferPosition,
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp", 10372,
                        pheader_2->iFormID, v2, cVersion, iFlags
                    );
                }
            }
            else {
                uint32_t v5 = pheader_2->iFlags;
                int32_t v3 = pheader_2->cVersion;
                const char* v1 = pform_2->GetFormEditorID();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading form "
                    "is %08X %s with version %i and flags %08X",
                    icurrentBufferPosition - &istartBufferPosition[iblockSize],
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp", 10372,
                    pheader_2->iFormID, v1, v3, v5
                );
            }
        }
        else {
            if (icurrentBufferPosition <= &istartBufferPosition[iblockSize]) {
                if (icurrentBufferPosition < &istartBufferPosition[iblockSize]) {
                    int32_t CurrentVersion = pSaveLoadGameOLD->GetCurrentVersion();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is "
                        "%i",
                        &istartBufferPosition[iblockSize] - icurrentBufferPosition,
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp", 10372,
                        CurrentVersion
                    );
                }
            }
            else {
                int32_t v7 = pSaveLoadGameOLD->GetCurrentVersion();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                    icurrentBufferPosition - &istartBufferPosition[iblockSize],
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp", 10372, v7
                );
            }
        }
    }
}

// Saves each change with the buffer's form cleared and its change flags set to 0x400, then puts both back.
void InventoryChanges::SaveGameBGS(BGSSaveFormBuffer* apSaveGameBuffer) {
    TESForm* pForm = apSaveGameBuffer->GetForm();
    BGSChangeFlags kChangeFlags = apSaveGameBuffer->GetChangeFlags();
    apSaveGameBuffer->SetForm(nullptr);
    BGSChangeFlags kTempFlags;
    kTempFlags.~BGSChangeFlags();
    ::new (&kTempFlags) BGSChangeFlags(0x400);
    apSaveGameBuffer->SetChangeFlags(kTempFlags);
    uint32_t uiCount = 0;
    uint32_t uiPosition = apSaveGameBuffer->StartVariableSizedValue();
    for (BSSimpleList<ItemChange*>* pNode = pListofChanges; pNode;) {
        ItemChange* pChange;
        pChange = pNode->GetItem();
        if (pChange) {
            pChange->SaveGame(apSaveGameBuffer);
            ++uiCount;
        }
        pNode = pNode->GetNext();
    }
    apSaveGameBuffer->SaveVariableSizedValue(uiCount, uiPosition);
    apSaveGameBuffer->SetForm(pForm);
    apSaveGameBuffer->SetChangeFlags(kChangeFlags);
}

bool InventoryChanges::IsEmpty() {
    // BSSimpleList::IsEmpty: no next node and no item
    return pListofChanges->m_pkNext == nullptr && pListofChanges->m_item == nullptr;
}

uint16_t InventoryChanges::GetSaveSize() {
    int16_t v7 = 0;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        v7 = 6;
    }
    uint16_t isize = v7 + 2;
    for (BSSimpleList<ItemChange*>* plist = pListofChanges; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
        ItemChange* pchange = plist->m_item;
        if (pchange) {
            isize += pchange->GetSaveSize();
        }
    }
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(pheader->iFormID);
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s", isize,
                pheader->iFormID, FormByNumericID->GetFormEditorID(), pheader->iFlags, 0x284E,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s", isize, 0x284E,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp"
            );
        }
    }
    return isize;
}

ItemChange* InventoryChanges::GetHotKeyItem(int32_t aiKey) {
    ItemChange* photkey;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_INVENTORY, 1, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp", 1898
    );
    InventoryChanges::IterData* i = GetIterator();
    while (1) {
        photkey = Iterate(i);
        if (photkey) {
            break;
        }
    LABEL_12:;
        if (!photkey) {
            if (i) {
                delete i;
            }
            return 0;
        }
    }
    for (BSSimpleList<ExtraDataList*>* pextraList = photkey->pExtraObjectList;; pextraList = pextraList->m_pkNext) {
        if (!pextraList || !pextraList->m_item) {
            delete photkey;
            goto LABEL_12;
        }
        if (pextraList->m_item->GetHotKey() == aiKey) {
            break;
        }
    }
    if (i) {
        delete i;
    }
    return photkey;
}

// A new array of the changes on the hotkeys aiFirst through aiLast, null per hotkey without one; a change is stored
// under every hotkey in range, and a change with none is freed.
ItemChange** InventoryChanges::GetHotKeyItem(int32_t aiFirst, int32_t aiLast) {
    if (aiFirst > aiLast) {
        return nullptr;
    }
    const uint32_t uiCount = static_cast<uint32_t>(aiLast - aiFirst + 1);
    // Allocates ItemChange*[uiCount] with a checked multiply, as the game does: the size becomes 0xFFFFFFFF when the
    // bytes do not fit in 32 bits.
    const uint32_t uiBytes =
        uiCount > UINT32_MAX / sizeof(ItemChange*) ? 0xFFFFFFFFu : static_cast<uint32_t>(sizeof(ItemChange*) * uiCount);
    ItemChange** ppItems = static_cast<ItemChange**>(ourMalloc(uiBytes));
    ourMemset(ppItems, 0, sizeof(ItemChange*) * uiCount);
    IterData* pIterator = GetIterator();
    ItemChange* pChange;
    do {
        pChange = Iterate(pIterator);
        if (pChange) {
            bool bFound = false;
            BSSimpleList<ExtraDataList*>* pNode = pChange->GetExtraDataList();
            while (true) {
                if (!pNode || !pNode->GetItem()) {
                    break;
                }
                const int8_t iHotKey = pNode->GetItem()->GetHotKey();
                if (iHotKey >= aiFirst && iHotKey <= aiLast) {
                    const int32_t iIndex = iHotKey - aiFirst;
                    if (ppItems[iIndex]) {
                        delete ppItems[iIndex];
                    }
                    ppItems[iIndex] = pChange;
                    bFound = true;
                }
                pNode = pNode->GetNext();
            }
            if (!bFound) {
                delete pChange;
            }
        }
    }
    while (pChange);
    if (pIterator) {
        delete pIterator;
    }
    return ppItems;
}

// A new iterator over the changes, for the caller to free.
InventoryChanges::IterData* InventoryChanges::GetIterator() {
    InventoryChanges::IterData* v3;
    AutoMemContext apMemory(
        MEM_CONTEXT_INVENTORY, 1, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp", 0x19B2
    );
    void* ecx0 = ourMalloc(sizeof(InventoryChanges::IterData));
    if (ecx0) {
        v3 = static_cast<InventoryChanges::IterData*>(ecx0);
    }
    else {
        v3 = 0;
    }
    v3->pFilteredList = 0;
    v3->pItemList = 0;
    if (GetOriginalContainer()) {
        TESContainer* OriginalContainer = GetOriginalContainer();
        v3->pFilteredList = OriginalContainer->GetFilteredList();
    }
    v3->dword8 = 0;
    v3->pCurrentItem = 0;
    v3->pContainer = GetOriginalContainer();
    v3->pItemList = v3->pFilteredList;
    v3->uiPhase = 0;
    v3->pExtraLists = 0;
    v3->pAddExtraLists = 0;
    v3->pCountExtraLists = 0;
    return v3;
}

// The next change of the iterator, or null at the end.
ItemChange* InventoryChanges::Iterate(InventoryChanges::IterData* apIterator) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_INVENTORY, 1, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp", 0x19C9
    );
    apIterator->pCurrentItem = 0;
    if (!apIterator->uiPhase) {
        while (apIterator->pItemList && (apIterator->pItemList->m_item && !apIterator->pCurrentItem)) {
            ContainerObject* pobject = apIterator->pItemList->m_item;
            TESLevItem* pObj = 0;
            char v57 = 1;
            if (pobject->pObj->GetFormType() == LVLI_ID) {
                pObj = static_cast<TESLevItem*>(pobject->pObj);
            }
            if (pobject) {
                if (!pObj) {
                    ItemChange* pitem = GetObjectInList(pobject->pObj, 1, 0);
                    int32_t iorgCount = apIterator->pContainer->GetObjectCount(pobject->pObj);
                    if (pobject && (pitem && iorgCount <= 0)) {
                        apIterator->pCurrentItem = 0;
                    }
                    else {
                        int32_t v2;
                        if ((iorgCount < 0 && pitem) ||
                            ((pitem && (v2 = pitem->iNumber, iorgCount + v2 <= 0)) ||
                             (pitem &&
                              (pitem->pExtraObjectList &&
                               (pitem->pExtraObjectList->m_item &&
                                pitem->pExtraObjectList->m_item->HasLeveledItem()))))) {
                            apIterator->pCurrentItem = 0;
                        }
                        else {
                            if (!pobject || (pitem && pitem->GetAmountNonDefaultExtra())) {
                                if (pitem) {
                                    int32_t inumb = 0;
                                    if (pitem->pExtraObjectList) {
                                        inumb = pitem->GetAmountNonDefaultExtra();
                                    }
                                    if (!apIterator->pExtraLists) {
                                        apIterator->pExtraLists = pitem->pExtraObjectList;
                                    }
                                    while (apIterator->pExtraLists &&
                                           (apIterator->pExtraLists->m_item && !apIterator->pCurrentItem)) {
                                        ExtraDataList* pcurextra = apIterator->pExtraLists->m_item;
                                        if (!pcurextra->IsExtraDefaultforContainer(0)) {
                                            ItemChange* preturnitem_2;
                                            int32_t icount = pcurextra->GetCount();
                                            int32_t FormHealth_0 = pitem->iNumber;
                                            if (icount > iorgCount + FormHealth_0) {
                                                int32_t v6 = pitem->iNumber;
                                                icount = iorgCount + v6;
                                            }
                                            ItemChange* v32 = static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
                                            if (v32) {
                                                preturnitem_2 = ::new (v32) ItemChange(pitem->pContainerObj, icount);
                                            }
                                            else {
                                                preturnitem_2 = 0;
                                            }
                                            apIterator->pCurrentItem = preturnitem_2;
                                            apIterator->pCurrentItem->pExtraObjectList->AddHead(pcurextra);
                                        }
                                        apIterator->pExtraLists = apIterator->pExtraLists->m_pkNext;
                                    }
                                    bool v22 = !apIterator->pExtraLists || !apIterator->pExtraLists->m_item;
                                    v57 = v22;
                                }
                                else {
                                    // Reached only with a null pobject, so pitem is null too: this calls it on null
                                    // deliberately, as the game does.
                                    pitem->GetAmountNonDefaultExtra();
                                    bool bflag = 0;
                                    TESObjectWEAP* pweap = 0;
                                    if (pitem->pContainerObj->GetFormType() == WEAP_ID) {
                                        pweap = static_cast<TESObjectWEAP*>(pitem->pContainerObj);
                                    }
                                    if (!pitem->GetExtraTotalCount(0) && pitem->GetWorn(0) &&
                                        (pitem->iNumber <= 0 ||
                                         (pitem->pContainerObj->GetFormType() == AMMO_ID ||
                                          (pweap &&
                                           (pweap->GetWeaponType() == WEAPON_ONE_HAND_GRENADE ||
                                            (pweap->GetWeaponType() == WEAPON_ONE_HAND_MINE ||
                                             pweap->GetWeaponType() == WEAPON_ONE_HAND_THROWN)))))) {
                                        bflag = 1;
                                    }
                                    if (!bflag) {
                                        int32_t ExtraTotalCount = pitem->GetExtraTotalCount(1);
                                        if (ExtraTotalCount < pitem->iNumber + iorgCount) {
                                            int32_t v8;
                                            if (!pitem->HasLeveledExtra() || (v8 = pitem->iNumber, v8 <= iorgCount)) {
                                                ItemChange* preturnitem_3;
                                                ItemChange* v31 =
                                                    static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
                                                if (v31) {
                                                    preturnitem_3 = ::new (v31) ItemChange;
                                                }
                                                else {
                                                    preturnitem_3 = 0;
                                                }
                                                apIterator->pCurrentItem = preturnitem_3;
                                                if (pitem) {
                                                    int32_t v9 = pitem->iNumber + iorgCount;
                                                    apIterator->pCurrentItem->iNumber =
                                                        v9 - pitem->GetExtraTotalCount(0);
                                                }
                                                else {
                                                    apIterator->pCurrentItem->iNumber = iorgCount;
                                                }
                                                apIterator->pCurrentItem->pContainerObj = pobject->pObj;
                                                if (pitem) {
                                                    if (pitem->GetScript()) {
                                                        if (pitem->pExtraObjectList) {
                                                            if (pitem->pExtraObjectList->ItemsInList()) {
                                                                if (!apIterator->pCurrentItem->pExtraObjectList) {
                                                                    BSSimpleList<ExtraDataList*>* v20;
                                                                    BSSimpleList<ExtraDataList*>* v30 =
                                                                        static_cast<BSSimpleList<ExtraDataList*>*>(
                                                                            ourMalloc(
                                                                                sizeof(BSSimpleList<ExtraDataList*>)
                                                                            )
                                                                        );
                                                                    if (v30) {
                                                                        v20 = ::new (v30) BSSimpleList<ExtraDataList*>;
                                                                    }
                                                                    else {
                                                                        v20 = 0;
                                                                    }
                                                                    apIterator->pCurrentItem->pExtraObjectList = v20;
                                                                }
                                                                for (ExtraDataList*& rpItem :
                                                                     pitem->pExtraObjectList->ItemsUntilNull()) {
                                                                    apIterator->pCurrentItem->pExtraObjectList->AddHead(
                                                                        rpItem
                                                                    );
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            else {
                                ItemChange* preturnitem;
                                ItemChange* v34 = static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
                                if (v34) {
                                    preturnitem = ::new (v34) ItemChange;
                                }
                                else {
                                    preturnitem = 0;
                                }
                                apIterator->pCurrentItem = preturnitem;
                                if (pitem) {
                                    if (iorgCount >= 0) {
                                        int32_t v3 = pitem->iNumber + iorgCount;
                                        apIterator->pCurrentItem->iNumber = v3 - pitem->GetExtraTotalCount(0);
                                    }
                                    else {
                                        apIterator->pCurrentItem->iNumber = pitem->iNumber + iorgCount;
                                    }
                                }
                                else {
                                    if (iorgCount >= 0) {
                                        apIterator->pCurrentItem->iNumber = iorgCount;
                                    }
                                    else {
                                        apIterator->pCurrentItem->iNumber = -iorgCount;
                                    }
                                }
                                apIterator->pCurrentItem->pContainerObj = pobject->pObj;
                                if (pitem) {
                                    TESObjectREFR* ItemOwnership;
                                    if (pitem->GetScript() ||
                                        (pitem->GetHotKey() >= 0 ||
                                         (ItemOwnership = static_cast<TESObjectREFR*>(pitem->GetItemOwnership()),
                                          ItemOwnership != pRef))) {
                                        if (pitem->pExtraObjectList) {
                                            if (pitem->pExtraObjectList->ItemsInList()) {
                                                if (!apIterator->pCurrentItem->pExtraObjectList) {
                                                    BSSimpleList<ExtraDataList*>* v24;
                                                    BSSimpleList<ExtraDataList*>* v33 =
                                                        static_cast<BSSimpleList<ExtraDataList*>*>(
                                                            ourMalloc(sizeof(BSSimpleList<ExtraDataList*>))
                                                        );
                                                    if (v33) {
                                                        v24 = ::new (v33) BSSimpleList<ExtraDataList*>;
                                                    }
                                                    else {
                                                        v24 = 0;
                                                    }
                                                    apIterator->pCurrentItem->pExtraObjectList = v24;
                                                }
                                                for (ExtraDataList*& rpItem :
                                                     pitem->pExtraObjectList->ItemsUntilNull()) {
                                                    apIterator->pCurrentItem->pExtraObjectList->AddHead(rpItem);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (v57) {
                apIterator->pItemList = apIterator->pItemList->m_pkNext;
            }
        }
        if (!apIterator->pCurrentItem) {
            apIterator->uiPhase = 1;
            apIterator->pChangeList = pListofChanges;
        }
    }
    if (apIterator->uiPhase == 1 && !apIterator->pCurrentItem) {
        while (apIterator->pChangeList && (apIterator->pChangeList->m_item && !apIterator->pCurrentItem)) {
            char v45 = 1;
            ItemChange* pitem_2 = apIterator->pChangeList->m_item;
            int32_t ioriginalcount = 0;
            if (GetOriginalContainer()) {
                if (pitem_2) {
                    TESBoundObject* pContainerObj = pitem_2->pContainerObj;
                    TESContainer* pcontainer = GetOriginalContainer();
                    ioriginalcount = pcontainer->GetObjectCount(pContainerObj);
                }
            }
            if (ioriginalcount < 0 && pitem_2->GetWorn(0)) {
                pitem_2->pExtraObjectList->m_item->SetCount(-static_cast<int16_t>(ioriginalcount));
            }
            if (pitem_2) {
                if (pitem_2->iNumber > 0 || ioriginalcount < 0) {
                    TESContainer* pcontainer_2;
                    TESBoundObject* v16;
                    if ((pitem_2->pExtraObjectList &&
                         (pitem_2->pExtraObjectList->m_item && pitem_2->pExtraObjectList->m_item->HasLeveledItem())) ||
                        (ioriginalcount < 0 ||
                         (!GetOriginalContainer() ||
                          (v16 = pitem_2->pContainerObj,
                           (pcontainer_2 = GetOriginalContainer(), !pcontainer_2->HasObject(v16)))))) {
                        int32_t itotalNonDefaultCount = 0;
                        if (!apIterator->pCountExtraLists) {
                            apIterator->pCountExtraLists = pitem_2->pExtraObjectList;
                        }
                        while (apIterator->pCountExtraLists && apIterator->pCountExtraLists->m_item) {
                            ExtraDataList* pkFirst = apIterator->pCountExtraLists->m_item;
                            if (!pkFirst->IsExtraDefaultforContainer(0)) {
                                int16_t Count = pkFirst->GetCount();
                                if (Count > 0) {
                                    itotalNonDefaultCount += Count;
                                }
                            }
                            apIterator->pCountExtraLists = apIterator->pCountExtraLists->m_pkNext;
                        }
                        apIterator->pCountExtraLists = 0;
                        if (pitem_2->pExtraObjectList && pitem_2->pExtraObjectList->m_item) {
                            pitem_2->GetAmountNonDefaultExtra();
                            if (!apIterator->pAddExtraLists) {
                                apIterator->pAddExtraLists = pitem_2->pExtraObjectList;
                            }
                            while (apIterator->pAddExtraLists &&
                                   (apIterator->pAddExtraLists->m_item && !apIterator->pCurrentItem)) {
                                ExtraDataList* pcurextra_2 = apIterator->pAddExtraLists->m_item;
                                if (!pcurextra_2->IsExtraDefaultforContainer(0)) {
                                    int16_t v38 = pcurextra_2->GetCount();
                                    if (v38 > 0) {
                                        ItemChange* preturnitem_4;
                                        ItemChange* v29 = static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
                                        if (v29) {
                                            preturnitem_4 = ::new (v29) ItemChange(pitem_2->pContainerObj, v38);
                                        }
                                        else {
                                            preturnitem_4 = 0;
                                        }
                                        apIterator->pCurrentItem = preturnitem_4;
                                        apIterator->pCurrentItem->pExtraObjectList->AddHead(pcurextra_2);
                                    }
                                }
                                apIterator->pAddExtraLists = apIterator->pAddExtraLists->m_pkNext;
                            }
                            bool v18 = !apIterator->pAddExtraLists || !apIterator->pAddExtraLists->m_item;
                            v45 = v18;
                        }
                        else {
                            uint32_t v12 = pitem_2->iNumber;
                            if (v12 == itotalNonDefaultCount) {
                                pitem_2->GetAmountNonDefaultExtra();
                            }
                        }
                        if (!apIterator->pCurrentItem) {
                            int32_t v13 = pitem_2->iNumber;
                            if (v13 > itotalNonDefaultCount) {
                                // The count is computed and never read, as the game does.
                                pitem_2->GetAmountNonDefaultExtra();
                            }
                            if (!apIterator->pCurrentItem && pitem_2->iNumber - itotalNonDefaultCount > 0) {
                                ItemChange* preturnitem_5;
                                ExtraDataList* pextra = 0;
                                ItemChange* v28 = static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
                                if (v28) {
                                    preturnitem_5 = ::new (v28) ItemChange(pitem_2->pContainerObj, 0);
                                }
                                else {
                                    preturnitem_5 = 0;
                                }
                                apIterator->pCurrentItem = preturnitem_5;
                                apIterator->pCurrentItem->iNumber = pitem_2->iNumber - itotalNonDefaultCount;
                                if (pitem_2->pExtraObjectList) {
                                    for (ExtraDataList*& rpItem : pitem_2->pExtraObjectList->ItemsUntilNull()) {
                                        pextra = rpItem;
                                        if (!pextra->GetWorn(0) && pextra->IsExtraDefaultforContainer(1)) {
                                            apIterator->pCurrentItem->pExtraObjectList->AddTail(pextra);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (v45) {
                apIterator->pChangeList = apIterator->pChangeList->m_pkNext;
            }
        }
    }
    ItemChange* preturnitem_6 = apIterator->pCurrentItem;
    return preturnitem_6;
}

bool InventoryChanges::HasObjectId(uint32_t aId) {
    int32_t iamount = 0;
    for (BSSimpleList<ContainerObject*>* pList = &GetOriginalContainer()->objectList; pList && !pList->IsEmpty();
         pList = pList->m_pkNext) {
        TESBoundObject* pObj = pList->m_item->pObj;
        if (pObj && pObj->iFormID == aId) {
            uint32_t v3;
            ItemChange* pitem = GetObjectInList(pObj, 1, 0);
            if (pList->m_item->i_count < 0) {
                return 1;
            }
            if (pitem && (v3 = pitem->iNumber, pList->m_item->i_count + v3)) {
                uint32_t FormHealth_0 = pitem->iNumber;
                iamount += pList->m_item->i_count + FormHealth_0;
            }
            else {
                iamount += pList->m_item->i_count;
            }
            if (iamount > 0) {
                return 1;
            }
        }
    }
    for (BSSimpleList<ItemChange*>* pListInv = pListofChanges; pListInv && !pListInv->IsEmpty();
         pListInv = pListInv->m_pkNext) {
        for (ExtraDataList* pextra : pListInv->m_item->pExtraObjectList->ItemsUntilNull()) {
            if (pextra) {
                if (pextra->GetReferencePointer()) {
                    TESObjectREFR* ReferencePointer = pextra->GetReferencePointer();
                    if (ReferencePointer->iFormID == aId) {
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

// The base container's count of the object (made positive) plus the change's count. A change with a zero count
// over a zero base count counts as one.
int32_t InventoryChanges::GetObjectCount(TESBoundObject* apObject) {
    int32_t iCount = 0;
    TESContainer* pContainer = GetOriginalContainer();
    iCount = pContainer ? pContainer->GetObjectCount(apObject) : 0;
    if (iCount < 0) {
        iCount = iCount * -1;
    }
    ItemChange* pItem = GetObjectInList(apObject, true, 0);
    if (pItem) {
        if (iCount == 0 && pItem->GetCount() == 0) {
            iCount = 1;
        }
        else {
            iCount = pItem->GetCount() + iCount;
        }
    }
    return iCount;
}

// Whether a quest object is held: a base container entry not cancelled by a change of the opposite count, else an added
// change with a positive count. The base container is deliberately not null-checked, as the game does.
bool InventoryChanges::HasQuestObject() {
    bool bHasQuestObject = false;
    for (BSSimpleList<ContainerObject*>* pNode = GetOriginalContainer()->GetObjectList();
         pNode && pNode->GetItem() && !bHasQuestObject; pNode = pNode->GetNext()) {
        ContainerObject* pObject = pNode->GetItem();
        if (pObject->pObj->GetQuestObject()) {
            bHasQuestObject = true;
            ItemChange* pItem = GetObjectInList(pObject->pObj, true, 0);
            if (pItem && pItem->iNumber + pObject->i_count == 0) {
                bHasQuestObject = false;
            }
        }
    }
    if (!bHasQuestObject) {
        for (BSSimpleList<ItemChange*>* pNode = pListofChanges; pNode && !pNode->IsEmpty() && !bHasQuestObject;
             pNode = pNode->GetNext()) {
            ItemChange* pItem = pNode->GetItem();
            if (pItem && pItem->pContainerObj && pItem->pContainerObj->GetQuestObject() && pItem->iNumber > 0) {
                bHasQuestObject = true;
            }
        }
    }
    return bHasQuestObject;
}

// Whether an object of the base container or of the changes has a script. A base container object counts only when its
// change does not cancel its count; each object is looked at once (a map of 37 buckets remembers them). A change counts
// when its count is nonzero and its object was not seen in the base container.
bool InventoryChanges::HasScript() {
    bool bHasScript = false;
    NiTMap_TESObject_bool kChecked(37);
    NiTMap<TESObject*, bool>* pChecked = &kChecked;
    for (BSSimpleList<ContainerObject*>* pNode = GetOriginalContainer()->GetObjectList();
         pNode && !pNode->IsEmpty() && !bHasScript; pNode = pNode->GetNext()) {
        ContainerObject* pObject = pNode->GetItem();
        bool bChecked = false;
        if (pChecked->GetAt(pObject->pObj, bChecked)) {
            continue;
        }
        pChecked->SetAt(pObject->pObj, true);
        if (TESScriptableForm::GetFormScript(pObject->pObj)) {
            bHasScript = true;
            ItemChange* pChange = GetObjectInList(pObject->pObj, true, 0);
            if (pChange && pChange->iNumber + pObject->i_count == 0) {
                bHasScript = false;
            }
        }
    }
    if (!bHasScript) {
        for (BSSimpleList<ItemChange*>* pNode = pListofChanges; pNode && !pNode->IsEmpty() && !bHasScript;
             pNode = pNode->GetNext()) {
            ItemChange* pChange = pNode->GetItem();
            bool bChecked = false;
            if (pChange->iNumber && !pChecked->GetAt(pChange->pContainerObj, bChecked) &&
                TESScriptableForm::GetFormScript(pChange->pContainerObj)) {
                bHasScript = true;
            }
        }
    }
    return bHasScript;
}

// A new change for the object (or, with aId, its instance referencing that form): the change's count plus the base
// container's, with the change's instances (all of them when the first is a container default, else only the first).
// Without a change, a new change for the object's base container entry. The base container is deliberately not
// null-checked, as the game does.
ItemChange* InventoryChanges::GetInventoryItem(TESBoundObject* apObject, uint32_t aId) {
    ItemChange* pItem = GetObjectInList(apObject, true, aId);
    ItemChange* pNewItem = nullptr;
    if (pItem) {
        int32_t iBaseCount = GetOriginalContainer()->GetObjectCount(apObject);
        void* pMemory = ourMalloc(sizeof(ItemChange));
        pNewItem = pMemory ? ::new (pMemory) ItemChange(apObject, pItem->GetCount() + iBaseCount) : nullptr;
        if (pItem->GetExtraDataList() && pItem->GetExtraDataList()->ItemsInList()) {
            if (!pNewItem->GetExtraDataList()) {
                void* pListMemory = ourMalloc(sizeof(BSSimpleList<ExtraDataList*>));
                pNewItem->pExtraObjectList = pListMemory ? new (pListMemory) BSSimpleList<ExtraDataList*>() : nullptr;
            }
            ExtraDataList* pExtra = pItem->GetExtraDataList()->GetItem();
            if (pExtra->IsExtraDefaultforContainer(false)) {
                for (BSSimpleList<ExtraDataList*>* pNode = pItem->GetExtraDataList(); pNode && pNode->GetItem();
                     pNode = pNode->GetNext()) {
                    pExtra = pNode->GetItem();
                    pNewItem->pExtraObjectList->AddTail(pExtra);
                }
            }
            else {
                pNewItem->pExtraObjectList->AddHead(pExtra);
            }
        }
        else if (pNewItem->GetExtraDataList()) {
            BSSimpleList<ExtraDataList*>* pList = pNewItem->pExtraObjectList;
            if (pList) {
                pList->RemoveAll();
                ourFree(pList);
            }
            pNewItem->pExtraObjectList = nullptr;
        }
    }
    else {
        for (BSSimpleList<ContainerObject*>* pNode = GetOriginalContainer()->GetObjectList();
             pNode && !pNode->IsEmpty(); pNode = pNode->GetNext()) {
            if (pNode->GetItem()->pObj == apObject) {
                void* pMemory = ourMalloc(sizeof(ItemChange));
                pNewItem = pMemory ? ::new (pMemory) ItemChange(apObject, pNode->GetItem()->i_count) : nullptr;
                break;
            }
        }
    }
    return pNewItem;
}

// The first worn instance of the object; with bshouldwear, an instance that may not be unequipped is worn first.
ExtraDataList* InventoryChanges::WearingObject(TESBoundObject* apObj, bool bshouldwear) {
    ItemChange* pItem = GetObjectInList(apObj, true, 0);
    if (!pItem) {
        return nullptr;
    }
    for (BSSimpleList<ExtraDataList*>* pNode = pItem->pExtraObjectList; pNode && pNode->GetItem();
         pNode = pNode->GetNext()) {
        ExtraDataList* pExtra = pNode->GetItem();
        if (pExtra && pExtra->GetWorn(false)) {
            return pExtra;
        }
        if (pExtra && pExtra->GetCanNotWear() && bshouldwear) {
            pExtra->SetWorn(true, false);
            return pExtra;
        }
    }
    return nullptr;
}

// ableveled is not read.
ItemChange* InventoryChanges::GetObjectInList(TESBoundObject* apObj, bool ableveled, uint32_t aId) {
    BSSimpleList<ItemChange*>* pNode = pListofChanges;
    bool bSearching = true;
    while (pNode && bSearching) {
        ItemChange* pItem = pNode->GetItem();
        if (pItem && pItem->pContainerObj == apObj) {
            bSearching = false;
        }
        else {
            pNode = pNode->GetNext();
        }
    }
    if (!pNode) {
        return nullptr;
    }

    ItemChange* pItem = pNode->GetItem();
    if (!pItem || !aId) {
        return pItem;
    }
    for (BSSimpleList<ExtraDataList*>* pExtras = pItem->pExtraObjectList; pExtras; pExtras = pExtras->GetNext()) {
        ExtraDataList* pExtra = pExtras->GetItem();
        if (pExtra && pExtra->GetReferencePointer() && pExtra->GetReferencePointer()->iFormID == aId) {
            return pItem;
        }
    }
    return nullptr;
}

// The first change with a worn instance filling the body part: a weapon for BO_WEAPON, else through the object's biped
// model (with its biped model list for the head, hair and hat slots). A leveled-item change with a negative count is
// unworn first. The result is a new change of the object holding that instance and its count; with bOnlyArmor, null
// when the object is not an armor. The AutoMemContext is constructed and destroyed explicitly in local storage, and the
// constructor's result is ignored.
ItemChange* InventoryChanges::GetWornItem(BIPED_OBJECT aeBodyPart, bool bOnlyArmor) {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_INVENTORY, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp",
        5978
    );
    ItemChange* pWornItem = nullptr;
    for (BSSimpleList<ItemChange*>* pNode = pListofChanges; pNode && pNode->GetItem(); pNode = pNode->GetNext()) {
        ItemChange* pItem = pNode->GetItem();
        if (!pItem) {
            continue;
        }
        BSSimpleList<ExtraDataList*>* pExtras = pItem->pExtraObjectList;
        ExtraDataList* pExtra = nullptr;
        bool bFound = false;
        if (pItem->HasLeveledExtra() && pItem->iNumber < 0) {
            pItem->SetWorn(false, false, true);
        }
        while (pExtras && pExtras->GetItem() && !bFound) {
            pExtra = pExtras->GetItem();
            if (pExtra->GetWorn(false)) {
                if (aeBodyPart == BO_WEAPON) {
                    if (pItem->GetContainerObject()->GetFormType() == WEAP_ID) {
                        bFound = true;
                    }
                }
                else {
                    TESBipedModelForm* pBipedModel =
                        TESBipedModelForm::GetFormAsBipedModel(pItem->GetContainerObject());
                    if (aeBodyPart >= BO_HEAD && (aeBodyPart <= BO_HAIR || aeBodyPart == BO_HAT)) {
                        BGSBipedModelList* pModelList =
                            BGSBipedModelList::GetFormAsBipedModelList(pItem->GetContainerObject());
                        if (pBipedModel && pBipedModel->FillsBipedSlot(aeBodyPart, false, pModelList)) {
                            bFound = true;
                        }
                    }
                    else if (pBipedModel && pBipedModel->FillsBipedSlot(aeBodyPart, false, nullptr)) {
                        bFound = true;
                    }
                }
            }
            pExtras = pExtras->GetNext();
        }
        if (!bFound) {
            continue;
        }

        if (bOnlyArmor && pItem->GetContainerObject()->GetFormType() != ARMO_ID) {
            pContext->~AutoMemContext();
            return pWornItem;
        }
        void* pMemory = ourMalloc(sizeof(ItemChange));
        pWornItem = pMemory ? ::new (pMemory) ItemChange : nullptr;
        pWornItem->pContainerObj = pItem->GetContainerObject();
        if (!pWornItem->pExtraObjectList) {
            void* pListMemory = ourMalloc(sizeof(BSSimpleList<ExtraDataList*>));
            pWornItem->pExtraObjectList = pListMemory ? ::new (pListMemory) BSSimpleList<ExtraDataList*> : nullptr;
        }
        pWornItem->pExtraObjectList->AddHead(pExtra);
        pWornItem->SetCount(pExtra->GetCount());
        pContext->~AutoMemContext();
        return pWornItem;
    }
    pContext->~AutoMemContext();
    return pWornItem;
}

// abSkipCanNotWear is not read. Each worn change is unequipped with the count of its first worn instance; when
// RemoveWorn reports a removal the walk restarts at the head of the list.
void InventoryChanges::RemoveAllObjectsWorn(bool abSkipCanNotWear, TESObjectREFR* apRef) {
    BSSimpleList<ItemChange*>* pNode = pListofChanges;
    while (pNode) {
        ItemChange* pItem = pNode->GetItem();
        if (!pItem) {
            pNode = pNode->GetNext();
            continue;
        }
        bool bRemoved = false;
        Actor* pActor = static_cast<Actor*>(apRef);
        if (!pItem->GetWorn(false)) {
            pNode = pNode->GetNext();
            continue;
        }
        int32_t iCount = 1;
        ExtraDataList* pWornExtra = nullptr;
        for (BSSimpleList<ExtraDataList*>* pExtras = pItem->pExtraObjectList; pExtras && pExtras->GetItem();
             pExtras = pExtras->GetNext()) {
            if (pExtras->GetItem()->GetWorn(false)) {
                iCount = pExtras->GetItem()->GetCount();
                pWornExtra = pExtras->GetItem();
                break;
            }
        }
        if (pActor) {
            static_cast<MagicTarget*>(pActor)->Dispel(pItem->GetContainerObject(), true);
        }
        RemoveWorn(&bRemoved, pItem->GetContainerObject(), iCount, pActor, pWornExtra, false, true, pItem, nullptr);
        if (bRemoved) {
            pNode = pListofChanges;
        }
        else {
            pNode = pNode->GetNext();
        }
    }
}

// Walks the weapon's ammo list: a held ammo at or past the list's script-added count is returned at once, a held one
// before it is remembered (the last one wins). Without one, the weapon's single ammo when held. bNeedsAmmo is set once
// the weapon has an ammo to look for.
TESAmmo* InventoryChanges::GetBestAmmoForWeapon(TESObjectWEAP* pWeapon, bool& bNeedsAmmo) {
    bNeedsAmmo = false;
    if (!pWeapon) {
        return nullptr;
    }

    BGSListForm* pAmmoList = static_cast<BGSAmmoForm*>(pWeapon)->GetAmmoFormList();
    TESBoundObject* pBestAmmo = nullptr;
    if (pAmmoList) {
        uint32_t uiIndex = 0;
        uint32_t uiScriptAddedCount = pAmmoList->iScriptAddedFormCount;
        for (BSSimpleList<TESForm*>* pNode = pAmmoList->GetFormList(); pNode && pNode->GetItem();
             pNode = pNode->GetNext(), ++uiIndex) {
            TESBoundObject* pAmmo = static_cast<TESBoundObject*>(pNode->GetItem());
            bNeedsAmmo = true;
            if (GetObjectCount(pAmmo) > 0) {
                if (uiIndex >= uiScriptAddedCount) {
                    return static_cast<TESAmmo*>(pAmmo);
                }
                pBestAmmo = pAmmo;
            }
        }
    }
    if (pBestAmmo) {
        return static_cast<TESAmmo*>(pBestAmmo);
    }

    TESBoundObject* pAmmo = static_cast<TESBoundObject*>(static_cast<BGSAmmoForm*>(pWeapon)->GetAmmoForm());
    if (pAmmo) {
        bNeedsAmmo = true;
        if (GetObjectCount(pAmmo) > 0) {
            return static_cast<TESAmmo*>(pAmmo);
        }
    }
    return nullptr;
}

// The next item of the inventory for arIterator: first the base container's entries the changes do not hold, then
// the changes with their counts added to the container's; with bit 0 of aiIterateFlags, quest items, unplayable
// biped forms and embedded weapons are skipped.
bool InventoryChanges::GetNextItem(InventoryIterator& arIterator, ItemChange*& arpItemChange, uint32_t aiIterateFlags) {
    if (arIterator.bFinished) {
        return 0;
    }
    if (!arIterator.pContainerObjectList && !arIterator.pInventoryChangeList) {
        arIterator.pContainerObjectList = &GetOriginalContainer()->objectList;
    }
    if (arIterator.pContainerObjectList) {
        while (arIterator.pContainerObjectList) {
            ContainerObject* pcontainerObject = arIterator.pContainerObjectList->m_item;
            arIterator.pContainerObjectList = arIterator.pContainerObjectList->m_pkNext;
            if (!arIterator.pContainerObjectList) {
                if (pListofChanges) {
                    arIterator.pInventoryChangeList = pListofChanges;
                }
                else {
                    arIterator.bFinished = 1;
                }
            }
            if (pcontainerObject) {
                TESBipedModelForm* pbipedform;
                if (!GetObjectInList(pcontainerObject->pObj, 1, 0) &&
                    ((aiIterateFlags & 1) == 0 ||
                     (!pcontainerObject->pObj->GetQuestObject() &&
                      (((pbipedform = TESBipedModelForm::GetFormAsBipedModel(pcontainerObject->pObj)) == 0 ||
                        pbipedform->GetPlayable()) &&
                       (pcontainerObject->pObj->GetFormType() != WEAP_ID ||
                        !static_cast<TESObjectWEAP*>(pcontainerObject->pObj)->TESObjectWEAP::IsEmbeddedWeapon()))))) {
                    arIterator.TempItemChange.pExtraObjectList = 0;
                    arIterator.TempItemChange.iNumber = pcontainerObject->i_count;
                    arIterator.TempItemChange.pContainerObj = pcontainerObject->pObj;
                    arpItemChange = &arIterator.TempItemChange;
                    return 1;
                }
            }
        }
    }
    if (arIterator.pInventoryChangeList) {
        while (arIterator.pInventoryChangeList) {
            ItemChange* pitemChange = arIterator.pInventoryChangeList->m_item;
            arIterator.pInventoryChangeList = arIterator.pInventoryChangeList->m_pkNext;
            if (!arIterator.pInventoryChangeList) {
                arIterator.bFinished = 1;
            }
            if (pitemChange) {
                TESBipedModelForm* pbipedform_2;
                TESObjectWEAP* pweapon = static_cast<TESObjectWEAP*>(pitemChange->pContainerObj);
                if ((aiIterateFlags & 1) == 0 ||
                    (!pweapon->GetQuestObject() &&
                     (((pbipedform_2 = TESBipedModelForm::GetFormAsBipedModel(pweapon)) == 0 ||
                       pbipedform_2->GetPlayable()) &&
                      (pweapon->GetFormType() != WEAP_ID || !pweapon->IsEmbeddedWeapon())))) {
                    uint32_t FormHealth_0 = pitemChange->iNumber;
                    TESContainer* OriginalContainer = GetOriginalContainer();
                    int32_t icount = OriginalContainer->GetObjectCount(pweapon) + FormHealth_0;
                    if (icount > 0) {
                        arpItemChange = pitemChange;
                        arIterator.TempItemChange.pExtraObjectList = pitemChange->pExtraObjectList;
                        arIterator.TempItemChange.iNumber = icount;
                        arIterator.TempItemChange.pContainerObj = pweapon;
                        arpItemChange = &arIterator.TempItemChange;
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

bool InventoryChanges::RemoveWorn(
    bool* abRemoved,
    TESBoundObject* apObject,
    int32_t aiNumber,
    Actor* apActor,
    ExtraDataList* apExtra,
    bool ableft,
    bool abforce,
    ItemChange* apItemChange,
    bool* apbExtraDeleted
) {
    uint32_t v21;
    ItemChange* pitem;
    pRestackedList = 0;
    ResetCachedWeights();
    if (apItemChange) {
        pitem = apItemChange;
    }
    else {
        pitem = GetObjectInList(apObject, 1, 0);
    }
    ItemChange* pitem_2 = pitem;
    if (!pitem) {
        return 1;
    }
    *abRemoved = 1;
    BSSimpleList<ExtraDataList*>* pkFirst = pitem_2->pExtraObjectList;
    bool bExtraGood = 1;
    bool bremovedPowerArmor = 0;
    bool bIsThrown = 0;
    while (pkFirst && pkFirst->m_item) {
        ExtraDataList* pExtra = pkFirst->m_item;
        if (pExtra->GetWorn(ableft) && (!apExtra || apExtra == pExtra)) {
            ItemChange* pchangeItem = 0;
            TESBipedModelForm* pmodel = TESBipedModelForm::GetFormAsBipedModel(apObject);
            if (pExtra->GetCanNotWear() && !apActor->IsDead(0)) {
                *abRemoved = 0;
                return bExtraGood;
            }
            if (apActor) {
                if (apActor->pCurrentProcess) {
                    char v44 = 1;
                    switch (apObject->GetFormType()) {
                        case ARMO_ID: {
                            TESObjectARMO* pArmor = static_cast<TESObjectARMO*>(apObject);
                            if (pArmor->TESBipedModelForm::IsPowerArmor() || pArmor->TESBipedModelForm::IsHeavy()) {
                                bremovedPowerArmor = 1;
                            }
                            ExtraDismemberedLimbs* DismembermentExtra = (&apActor->m_Extra)->GetDismembermentExtra();
                            if (DismembermentExtra) {
                                if (DismembermentExtra->DismemberedLimbs.iSize) {
                                    if (DismembermentExtra->GetDismemberedLimb(0)) {
                                        for (uint32_t aiIndex = 0;; ++aiIndex) {
                                            uint32_t FormFlags = DismembermentExtra->DismemberedLimbs.iSize;
                                            if (aiIndex >= FormFlags) {
                                                break;
                                            }
                                            DismemberedLimb* pWeap = DismembermentExtra->GetDismemberedLimb(aiIndex);
                                            if (!pWeap->cLimb && pWeap->bLimbExploded) {
                                                v44 = 0;
                                            }
                                        }
                                    }
                                }
                            }
                            if (v44) {
                                apActor->pCurrentProcess->Set3DUpdateFlag(RESET_MODEL);
                            }
                            break;
                        }
                        case CLOT_ID: {
                            ExtraDismemberedLimbs* v43 = (&apActor->m_Extra)->GetDismembermentExtra();
                            if (v43) {
                                if (v43->DismemberedLimbs.iSize) {
                                    if (v43->GetDismemberedLimb(0)) {
                                        for (int32_t a2 = 0;; ++a2) {
                                            uint32_t v11 = v43->DismemberedLimbs.iSize;
                                            if (a2 >= v11) {
                                                break;
                                            }
                                            DismemberedLimb* DismemberedLimb_ = v43->GetDismemberedLimb(a2);
                                            if (!DismemberedLimb_->cLimb && DismemberedLimb_->bLimbExploded) {
                                                v44 = 0;
                                            }
                                        }
                                    }
                                }
                            }
                            if (v44) {
                                apActor->pCurrentProcess->Set3DUpdateFlag(RESET_MODEL);
                            }
                            break;
                        }
                        case WEAP_ID: {
                            BaseProcess* CurrentAIProcess = apActor->pCurrentProcess;
                            pchangeItem = CurrentAIProcess->GetCurrentWeapon();
                            if (pchangeItem) {
                                BSSimpleList<ExtraDataList*>* v12 = pchangeItem->pExtraObjectList;
                                if (v12->m_item == pExtra || !apExtra) {
                                    if (apActor == pPlayer) {
                                        pPlayer->pQueuedWeaponAttach = 0;
                                    }
                                    TESObjectWEAP* v40 = static_cast<TESObjectWEAP*>(apObject);
                                    if (static_cast<TESObjectWEAP*>(apObject)->TESObjectWEAP::IsThrownWeapon()) {
                                        bIsThrown = 1;
                                    }
                                    if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                                        TESBoundObject* v23 = pchangeItem->pContainerObj;
                                        TaskQueueInterface* Instance = TaskQueueInterface::GetInstance();
                                        Instance->QueueRemoveWeapon(apActor, v23);
                                    }
                                    else {
                                        apActor->RemoveWeapon();
                                        BaseProcess* v29 = apActor->pCurrentProcess;
                                        v29->SetCurrentWeapon(0, 0, 0);
                                        BaseProcess* v28 = apActor->pCurrentProcess;
                                        v28->SetCurrentAmmo(0);
                                    }
                                }
                            }
                            break;
                        }
                        case AMMO_ID: {
                            BaseProcess* v27 = apActor->pCurrentProcess;
                            pchangeItem = v27->GetCurrentAmmo();
                            if (pchangeItem) {
                                BSSimpleList<ExtraDataList*>* v15 = pchangeItem->pExtraObjectList;
                                if (v15->m_item == pExtra) {
                                    BaseProcess* v26 = apActor->pCurrentProcess;
                                    v26->SetCurrentAmmo(0);
                                }
                            }
                            break;
                        }
                        default: {
                            break;
                        }
                    }
                }
            }
            pExtra->SetWorn(0, ableft);
            if (pExtra->ItemsInList() <= 1) {
                if (pExtra->GetHotKey() < 0) {
                    pExtra->SetCount(0);
                }
                else {
                    int16_t FormHealth_0 = pitem_2->iNumber;
                    int16_t ExtraTotalCount = pitem_2->GetExtraTotalCount(0);
                    pExtra->SetCount(FormHealth_0 - ExtraTotalCount);
                }
            }
            if (pExtra->IsExtraListEmpty()) {
                pitem_2->pExtraObjectList->Remove(pExtra);
                if (pExtra) {
                    delete pExtra;
                }
                if (pExtra == apExtra) {
                    if (apbExtraDeleted) {
                        *apbExtraDeleted = 1;
                    }
                }
                pExtra = 0;
                bExtraGood = 0;
                break;
            }
            BSSimpleList<ExtraDataList*>* pkFirst_2 = pitem_2->pExtraObjectList;
            while (pkFirst_2 && (pkFirst_2->m_item && bExtraGood)) {
                ExtraDataList* pCurrent = pkFirst_2->m_item;
                ExtraDataList* pcompare = pExtra;
                bool bcomp = 1;
                if (pExtra) {
                    bool v25 = pcompare->CompareListForContainer(pCurrent, 0, bIsThrown) ||
                        pCurrent->CompareListForContainer(pcompare, 0, bIsThrown);
                    bcomp = v25;
                }
                if (bcomp || pExtra == pCurrent) {
                    pkFirst_2 = pkFirst_2->m_pkNext;
                }
                else {
                    int16_t Count = pCurrent->GetCount();
                    pCurrent->SetCount(aiNumber + Count);
                    pRestackedList = pCurrent;
                    if (pExtra) {
                        if (pExtra->GetHotKey() >= 0) {
                            char HotKey = pExtra->GetHotKey();
                            pCurrent->SetHotKey(HotKey);
                        }
                    }
                    if (pCurrent->IsExtraListEmpty()) {
                        pitem_2->pExtraObjectList->Remove(pCurrent);
                        if (pCurrent) {
                            delete pCurrent;
                        }
                    }
                    else {
                        pitem_2->pExtraObjectList->Remove(pExtra);
                    }
                    if (pExtra) {
                        delete pExtra;
                    }
                    pExtra = 0;
                    bExtraGood = 0;
                }
            }
            pkFirst = 0;
        }
        else {
            pkFirst = pkFirst->m_pkNext;
        }
    }
    if (bremovedPowerArmor) {
        if (apActor) {
            if (apActor->pCurrentProcess) {
                BaseProcess* v24 = apActor->pCurrentProcess;
                v24->UpdateArmorWorn(apActor);
            }
        }
    }
    if (pRef) {
        pRef->AddChange(32);
    }
    TESContainer* OriginalContainer = GetOriginalContainer();
    int32_t iorgCount = OriginalContainer->GetObjectCount(apObject);
    if (pitem_2 &&
        (pitem_2->pExtraObjectList &&
         (pitem_2->pExtraObjectList->IsEmpty() && (!pitem_2->iNumber || (v21 = pitem_2->iNumber, iorgCount == v21))))) {
        pListofChanges->Remove(pitem_2);
        if (pitem_2) {
            delete pitem_2;
        }
        return 0;
    }
    else {
        if (pitem_2->iNumber < 0) {
            if (pitem_2->pExtraObjectList) {
                pitem_2->pExtraObjectList->RemoveAll();
            }
        }
        if (apActor) {
            NiNode* v22 = apActor->TESObjectREFR::Get3D();
            apActor->InitLightingPropertyPtr(v22);
        }
        return bExtraGood;
    }
}

// Constructs and destroys an AutoMemContext on the stack and ignores the constructor's result. A null result of
// GetOriginalContainer is used without a check, as the game does.
void InventoryChanges::AddWorn(
    TESBoundObject* apObject,
    int32_t aiNumber,
    Actor* apActor,
    ExtraDataList* apExtra,
    bool ableft,
    bool abForceEquip
) {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_INVENTORY, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp",
        2400
    );
    ResetCachedWeights();
    if (pRef) {
        pRef->AddChange(CHANGE_REFR_INVENTORY);
        // The biped model form is looked up and not used.
        if (apObject->GetFormType() == CLOT_ID || apObject->GetFormType() == ARMO_ID) {
            TESBipedModelForm::GetFormAsBipedModel(apObject);
        }
    }

    ItemChange* pChange = GetObjectInList(apObject, true, 0);
    int32_t iBaseCount = GetOriginalContainer()->GetObjectCount(apObject);
    int32_t iTotalCount = iBaseCount;
    if (pChange) {
        iTotalCount += pChange->iNumber;
    }

    int32_t iExtraCount = 0;
    if (pChange && pChange->pExtraObjectList) {
        iExtraCount = pChange->GetExtraTotalCount(false);
        iExtraCount += pChange->GetExtraTotalDefaultCount();
    }

    int32_t iFreeCount = iTotalCount - iExtraCount;
    if (pChange && pChange->pExtraObjectList) {
        if (pChange->pExtraObjectList->GetItem() && pChange->pExtraObjectList->GetItem()->HasLeveledItem()) {
            iFreeCount = iBaseCount - iExtraCount;
        }

        // An instance not in the list is replaced by the first listed instance whose extra data matches it.
        bool bFound = false;
        if (apExtra && !pChange->pExtraObjectList->IsInList(apExtra)) {
            for (BSSimpleList<ExtraDataList*>* pNode = pChange->pExtraObjectList; pNode && pNode->GetItem();
                 pNode = pNode->GetNext()) {
                ExtraDataList* pNodeExtra = pNode->GetItem();
                if (pNodeExtra && !pNodeExtra->CompareList(apExtra)) {
                    apExtra = pNodeExtra;
                    bFound = true;
                    break;
                }
            }
            if (!bFound) {
                apExtra = nullptr;
            }
        }
    }

    bool bAddNew = true;
    ExtraDataList* pWornExtra = nullptr;
    if ((!apExtra && iFreeCount > 0) || !pChange) {
        if (!pChange) {
            void* pMemory = ourMalloc(sizeof(ItemChange));
            pChange = pMemory ? ::new (pMemory) ItemChange(apObject, 0) : nullptr;
            bAddNew = true;
            pListofChanges->AddTail(pChange);
            apExtra = nullptr;
        }
    }
    else {
        if (pChange->GetWorn(false)) {
            pContext->~AutoMemContext();
            return;
        }

        // Splits one off a stacked instance, or takes the given or first instance.
        BSSimpleList<ExtraDataList*>* pNode = pChange->pExtraObjectList;
        while (pNode && bAddNew) {
            pWornExtra = pNode->GetItem();
            if (apExtra && apExtra == pWornExtra) {
                bool bThrown =
                    apObject->GetFormType() == WEAP_ID && static_cast<TESObjectWEAP*>(apObject)->IsThrownWeapon();
                if (pWornExtra->GetCount() > 1 && apObject->GetFormType() != AMMO_ID && !bThrown) {
                    void* pMemory = ourMalloc(sizeof(ExtraDataList));
                    apExtra = pMemory ? ::new (pMemory) ExtraDataList : nullptr;
                    apExtra->CopyList(pWornExtra);
                    apExtra->SetCount(1);
                    pWornExtra->SetCount(pWornExtra->GetCount() - 1);
                    if (pWornExtra->GetHotKey() >= 0) {
                        pWornExtra->RemoveHotKey();
                    }
                }
                else {
                    pWornExtra->SetWorn(true, ableft);
                    pWornExtra->SetCount(pChange->GetCount());
                }
                bAddNew = false;
            }
            else if (pWornExtra && !apExtra) {
                if (pWornExtra->GetCount() == 1) {
                    apExtra = pWornExtra;
                }
                else {
                    bool bThrown =
                        apObject->GetFormType() == WEAP_ID && static_cast<TESObjectWEAP*>(apObject)->IsThrownWeapon();
                    if (pWornExtra->GetCount() > 1 && apObject->GetFormType() != AMMO_ID && !bThrown) {
                        void* pMemory = ourMalloc(sizeof(ExtraDataList));
                        apExtra = pMemory ? ::new (pMemory) ExtraDataList : nullptr;
                        apExtra->CopyList(pWornExtra);
                        apExtra->SetCount(1);
                        pWornExtra->SetCount(pWornExtra->GetCount() - 1);
                        if (pWornExtra->GetHotKey() >= 0) {
                            pWornExtra->RemoveHotKey();
                        }
                    }
                    else {
                        apExtra = pWornExtra;
                    }
                }
                bAddNew = false;
            }
            else {
                pNode = pNode->GetNext();
            }
        }
    }

    if (apExtra && !apExtra->GetCanNotWear()) {
        apExtra->SetWorn(true, ableft);
        // Passes the count's low 16 bits, zero-extended.
        apExtra->SetCount(static_cast<uint16_t>(aiNumber));
        bAddNew = false;
        if (!pChange->pExtraObjectList) {
            void* pMemory = ourMalloc(sizeof(BSSimpleList<ExtraDataList*>));
            pChange->pExtraObjectList = pMemory ? ::new (pMemory) BSSimpleList<ExtraDataList*> : nullptr;
        }
        bool bInList;
        bInList = pChange->pExtraObjectList->IsInList(apExtra);
        if (!bInList) {
            pChange->pExtraObjectList->AddHead(apExtra);
        }
        pWornExtra = apExtra;
    }

    if (bAddNew) {
        void* pMemory = ourMalloc(sizeof(ExtraDataList));
        pWornExtra = pMemory ? ::new (pMemory) ExtraDataList : nullptr;
        pWornExtra->SetWorn(true, ableft);
        if (aiNumber > 1) {
            pWornExtra->SetCount(static_cast<uint16_t>(aiNumber));
        }
        if (!pChange->pExtraObjectList) {
            void* pListMemory = ourMalloc(sizeof(BSSimpleList<ExtraDataList*>));
            pChange->pExtraObjectList = pListMemory ? ::new (pListMemory) BSSimpleList<ExtraDataList*> : nullptr;
        }
        bool bInList;
        bInList = pChange->pExtraObjectList->IsInList(pWornExtra);
        if (!bInList) {
            pChange->pExtraObjectList->AddHead(pWornExtra);
        }
    }

    if (pWornExtra) {
        pWornExtra->SetCanNotWear(abForceEquip);
    }

    if (!apActor->pCurrentProcess) {
        pContext->~AutoMemContext();
        return;
    }

    ItemChange* pProcessChange = nullptr;
    // The biped model form is looked up and not used.
    TESBipedModelForm::GetFormAsBipedModel(apObject);
    switch (apObject->GetFormType()) {
        case ARMO_ID: {
            TESObjectARMO* pArmor = static_cast<TESObjectARMO*>(apObject);
            if ((pArmor->IsPowerArmor() || pArmor->IsHeavy() || pArmor->HasBackPack()) &&
                apActor->GetCurrentAIProcess()) {
                apActor->GetCurrentAIProcess()->UpdateArmorWorn(apActor);
            }
            apActor->pCurrentProcess->Set3DUpdateFlag(RESET_MODEL);
            break;
        }
        case CLOT_ID: apActor->pCurrentProcess->Set3DUpdateFlag(RESET_MODEL); break;
        case WEAP_ID: {
            pProcessChange = apActor->GetCurrentAIProcess()->GetCurrentWeapon();
            if (pProcessChange) {
                pProcessChange->pExtraObjectList->RemoveAll();
                pProcessChange->pExtraObjectList->AddHead(pWornExtra);
                pProcessChange->pContainerObj = apObject;
                apActor->ReloadTargets(false);
            }
            else {
                void* pMemory = ourMalloc(sizeof(ItemChange));
                pProcessChange = pMemory ? ::new (pMemory) ItemChange(apObject, aiNumber) : nullptr;
                pProcessChange->pExtraObjectList->AddHead(pWornExtra);
                BaseProcess* pProcess = apActor->GetCurrentAIProcess();
                NiNode* p3D = apActor->Get3D();
                if (!pProcess->SetCurrentWeapon(pProcessChange, p3D, false)) {
                    if (pProcessChange) {
                        delete pProcessChange;
                    }
                    pProcessChange = nullptr;
                }
                else {
                    Animation* pAnimation = apActor->GetAnimation();
                    float fAttackSpeed = static_cast<TESObjectWEAP*>(pProcessChange->GetContainerObject())->GetSpeed();
                    BGSEntryPoint::HandleEntryPoint(
                        BGSEntryPoint::ATTACK_SPEED_MODIFIER, apActor, pProcessChange->GetContainerObject(),
                        &fAttackSpeed
                    );
                    if (pAnimation) {
                        pAnimation->SetAttackSpeed(fAttackSpeed);
                    }
                }
            }

            if (apActor == pPlayer || !apActor->pCurrentProcess->Get3DUpdateFlag(RESET_MODEL)) {
                if (pProcessChange) {
                    TESObjectWEAP* pWeapon = static_cast<TESObjectWEAP*>(pProcessChange->pContainerObj);
                    if (apActor == pPlayer) {
                        pPlayer->QueueAttachWeapon(pWeapon);
                    }
                    else if (
                        AILinearTaskThreadManager::ShouldQueue3DTask() &&
                        apActor != TESObjectREFR::GetBackgroundLoadingRef()
                    ) {
                        TaskQueueInterface::GetInstance()->QueueAttachWeapon(apActor, pWeapon);
                    }
                    else {
                        apActor->AttachWeapon(pWeapon);
                    }
                }
            }
            break;
        }
        case AMMO_ID: {
            pProcessChange = apActor->GetCurrentAIProcess()->GetCurrentAmmo();
            if (pProcessChange) {
                pProcessChange->pExtraObjectList->RemoveAll();
                pProcessChange->pExtraObjectList->AddHead(pWornExtra);
                pProcessChange->pContainerObj = apObject;
            }
            else {
                void* pMemory = ourMalloc(sizeof(ItemChange));
                pProcessChange = pMemory ? ::new (pMemory) ItemChange(apObject, 0) : nullptr;
                pProcessChange->pExtraObjectList->AddHead(pWornExtra);
                if (!apActor->GetCurrentAIProcess()->SetCurrentAmmo(pProcessChange)) {
                    if (pProcessChange) {
                        delete pProcessChange;
                    }
                    pProcessChange = nullptr;
                }
            }

            if (!apActor->pCurrentProcess->Get3DUpdateFlag(RESET_MODEL) && pProcessChange) {
                apActor->AttachAmmo(static_cast<TESAmmo*>(pProcessChange->pContainerObj));
            }
            break;
        }
        default: break;
    }

    // Calls TESObjectREFR's own Get3D here, not through the vtable.
    apActor->InitLightingPropertyPtr(apActor->TESObjectREFR::Get3D());
    pContext->~AutoMemContext();
}

// The notes above AddWorn apply.
void InventoryChanges::AddItem(TESBoundObject* apObject, ExtraDataList* pextra, int32_t ainumber) {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_INVENTORY, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp",
        3368
    );
    bcountdirty = true;
    fpreviousContainerWeight = fcontainerweight;
    fcontainerweight = -1.0f;

    int32_t iAmmoCount = 0;
    if (!pextra) {
        void* pMemory = ourMalloc(sizeof(ExtraDataList));
        pextra = pMemory ? ::new (pMemory) ExtraDataList : nullptr;
    }
    if (pextra) {
        ExtraAmmo* pAmmo = pextra->GetAmmoExtra();
        if (pAmmo) {
            iAmmoCount = pAmmo->iCount;
        }
    }
    iAmmoCount *= ainumber;
    if (ainumber == 0) {
        ainumber = 1;
    }

    ItemChange* pChange = GetObjectInList(apObject, true, 0);
    bool bThrownEquipped = false;
    if (pextra && pextra->GetOwner() == pPlayer && pRef == pPlayer) {
        pextra->RemoveOwnership();
    }

    Actor* pActor = nullptr;
    if (pRef && pRef->IsActor()) {
        pActor = static_cast<Actor*>(pRef);
    }

    // A thrown weapon the actor has equipped as its weapon and ammo gets the count on both and stays worn.
    if (apObject->GetFormType() == WEAP_ID && pActor && pActor->GetCurrentAIProcess()) {
        TESObjectWEAP* pWeapon = static_cast<TESObjectWEAP*>(apObject);
        BaseProcess* pProcess = pActor->GetCurrentAIProcess();
        if (pWeapon->IsThrownWeapon() && pProcess->GetCurrentAmmo() &&
            pWeapon == pProcess->GetCurrentAmmo()->GetContainerObject()) {
            bThrownEquipped = true;
            if (pProcess->GetCurrentWeapon()) {
                int32_t iCount = pProcess->GetCurrentWeapon()->GetCount() + ainumber;
                pProcess->GetCurrentWeapon()->SetCount(iCount);
            }
            if (pProcess->GetCurrentAmmo()) {
                int32_t iCount = pProcess->GetCurrentAmmo()->GetCount() + ainumber;
                pProcess->GetCurrentAmmo()->SetCount(iCount);
            }
            if (!pChange || !pChange->GetWorn(false)) {
                if (!pextra) {
                    void* pMemory = ourMalloc(sizeof(ExtraDataList));
                    pextra = pMemory ? ::new (pMemory) ExtraDataList : nullptr;
                }
                pextra->SetWorn(true, false);
            }
        }
    }

    if (pextra && pextra->GetReferencePointer()) {
        TESObjectREFR* pReference = pextra->GetReferencePointer();
        if (pReference) {
            pReference->GetExtra()->SetReferencePointer(pRef);
        }
    }

    if (!pChange) {
        if (pextra && pextra->IsExtraListEmpty()) {
            delete pextra;
            pextra = nullptr;
        }
        void* pMemory = ourMalloc(sizeof(ItemChange));
        pChange = pMemory ? ::new (pMemory) ItemChange(apObject, ainumber) : nullptr;
        if (!pChange->pExtraObjectList) {
            void* pListMemory = ourMalloc(sizeof(BSSimpleList<ExtraDataList*>));
            pChange->pExtraObjectList = pListMemory ? new (pListMemory) BSSimpleList<ExtraDataList*>() : nullptr;
        }
        pChange->pExtraObjectList->AddHead(pextra);
        AddChangeItem(pChange, true);
    }
    else {
        // A leveled first instance of an uncounted change absorbs the count of an anonymous new instance.
        if (pChange->GetCount() <= 0 && pextra && !pextra->GetOwner() && !pextra->GetScript() &&
            pChange->pExtraObjectList && pChange->pExtraObjectList->ItemsInList() &&
            pChange->pExtraObjectList->GetItem()->HasLeveledItem()) {
            int32_t iCount = pChange->pExtraObjectList->GetItem()->GetCount() + ainumber;
            pChange->pExtraObjectList->GetItem()->SetCount(iCount);
            delete pextra;
            pextra = nullptr;
        }

        if (pextra && pChange->pExtraObjectList) {
            // Merges into the first instance with matching extra data, else adds the instance.
            BSSimpleList<ExtraDataList*>* pNode = pChange->pExtraObjectList;
            bool bAdd = true;
            while (pNode && pNode->GetItem() && bAdd) {
                ExtraDataList* pExtra = pNode->GetItem();
                bool bDifferent = true;
                if (pextra) {
                    bDifferent = pextra->CompareListForContainer(pExtra, false, bThrownEquipped) ||
                        pExtra->CompareListForContainer(pextra, false, bThrownEquipped);
                }
                if (bDifferent) {
                    pNode = pNode->GetNext();
                }
                else {
                    pExtra->SetCount(pExtra->GetCount() + ainumber);
                    if (pExtra->IsExtraListEmpty()) {
                        pChange->pExtraObjectList->Remove(pExtra);
                        delete pExtra;
                    }
                    bAdd = false;
                }
            }

            if (bAdd) {
                if (pextra && pextra->IsExtraListEmpty()) {
                    delete pextra;
                    pextra = nullptr;
                }
                else {
                    if (!pChange->pExtraObjectList) {
                        void* pMemory = ourMalloc(sizeof(BSSimpleList<ExtraDataList*>));
                        pChange->pExtraObjectList = pMemory ? new (pMemory) BSSimpleList<ExtraDataList*>() : nullptr;
                    }
                    pChange->pExtraObjectList->AddHead(pextra);
                }
            }
            else if (!Interface::ContainerIsBarter() && !Interface::ContainerIsRecipe()) {
                delete pextra;
                pextra = nullptr;
            }
        }
        else if (pextra && !pextra->IsExtraListEmpty()) {
            if (!pChange->pExtraObjectList) {
                void* pMemory = ourMalloc(sizeof(BSSimpleList<ExtraDataList*>));
                pChange->pExtraObjectList = pMemory ? new (pMemory) BSSimpleList<ExtraDataList*>() : nullptr;
            }
            pChange->pExtraObjectList->AddHead(pextra);
        }

        TESContainer* pContainer = GetOriginalContainer();
        int32_t iBaseCount = pContainer ? pContainer->GetObjectCount(apObject) : 0;
        if (pChange->GetCount() < 0 && iBaseCount <= 0) {
            pChange->SetCount(ainumber);
        }
        else {
            pChange->SetCount(pChange->GetCount() + ainumber);
        }

        if (pChange->pExtraObjectList && pChange->pExtraObjectList->IsEmpty() && pChange->GetCount() == 0) {
            pListofChanges->Remove(pChange);
            if (pChange) {
                delete pChange;
            }
        }
    }

    // The rounds loaded in the added weapons go in as their ammo.
    if (iAmmoCount > 0) {
        TESObjectREFR* pHolder = pRef->IsActor() ? pRef : nullptr;
        TESForm* pAmmo = static_cast<TESObjectWEAP*>(apObject)->GetCurrentAmmo(static_cast<Actor*>(pHolder));
        if (pAmmo) {
            AddItem(static_cast<TESBoundObject*>(pAmmo), nullptr, iAmmoCount);
        }
    }
    pContext->~AutoMemContext();
}

// A freed instance list goes through `delete` (~BSSimpleList runs RemoveAll, then ourFree). The notes above AddWorn and
// AddItem apply.
void InventoryChanges::AddChangeItem(ItemChange* apChange, bool abDelete) {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_INVENTORY, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp",
        3587
    );
    fpreviousContainerWeight = fcontainerweight;
    fcontainerweight = -1.0f;
    if (!apChange) {
        pContext->~AutoMemContext();
        return;
    }

    if (pRef) {
        pRef->AddChange(CHANGE_REFR_INVENTORY);
    }

    ItemChange* pExisting = GetObjectInList(apChange->GetContainerObject(), true, 0);
    TESBoundObject* pObject = apChange->GetContainerObject();
    bool bThrown = false;
    if (pObject->GetFormType() == WEAP_ID) {
        bThrown = static_cast<TESObjectWEAP*>(pObject)->IsThrownWeapon();
    }

    if (apChange->GetExtraDataList() && apChange->GetExtraDataList()->IsEmpty()) {
        BSSimpleList<ExtraDataList*>* pList = apChange->pExtraObjectList;
        if (pList) {
            delete pList;
        }
        apChange->pExtraObjectList = nullptr;
    }

    if (!pExisting) {
        pListofChanges->AddTail(apChange);
        pContext->~AutoMemContext();
        return;
    }

    int32_t iCount = pExisting->GetCount();
    iCount += apChange->GetCount();
    pExisting->SetCount(iCount);

    if (!apChange->pExtraObjectList || !apChange->pExtraObjectList->IsEmpty()) {
        // Each instance merges into a matching instance of the existing change, else is added to it. The search
        // position in the existing change carries over from one instance to the next.
        BSSimpleList<ExtraDataList*>* pDstNode = pExisting->pExtraObjectList;
        BSSimpleList<ExtraDataList*>* pSrcNode = apChange->pExtraObjectList;
        while (pSrcNode && pSrcNode->GetItem()) {
            ExtraDataList* pSrcExtra = pSrcNode->GetItem();
            bool bAdd = true;
            if (pSrcExtra->GetReferencePointer()) {
                pSrcExtra->SetReferencePointer(pRef);
            }

            while (pDstNode && pDstNode->GetItem() && bAdd) {
                ExtraDataList* pDstExtra = pDstNode->GetItem();
                bool bDifferent = true;
                if (pSrcExtra) {
                    bDifferent = pSrcExtra->CompareListForContainer(pDstExtra, false, bThrown) ||
                        pDstExtra->CompareListForContainer(pSrcExtra, false, bThrown);
                }
                if (bDifferent) {
                    pDstNode = pDstNode->GetNext();
                }
                else {
                    int32_t iMerged = pDstExtra->GetCount();
                    iMerged += pSrcExtra->GetCount();
                    pDstExtra->SetCount(iMerged);
                    if (pSrcExtra) {
                        apChange->pExtraObjectList->Remove(pSrcExtra);
                        delete pSrcExtra;
                    }
                    bAdd = false;
                }
            }

            if (bAdd) {
                if (!pExisting->pExtraObjectList) {
                    void* pMemory = ourMalloc(sizeof(BSSimpleList<ExtraDataList*>));
                    pExisting->pExtraObjectList = pMemory ? new (pMemory) BSSimpleList<ExtraDataList*>() : nullptr;
                }
                pExisting->pExtraObjectList->AddHead(pSrcExtra);
            }
            pSrcNode = pSrcNode->GetNext();
        }

        if (abDelete) {
            if (apChange) {
                delete apChange;
            }
            apChange = nullptr;
        }
    }

    // A merged change left with no instances and no count change is dropped.
    pExisting = GetObjectInList(pObject, true, 0);
    if (pExisting && (!pExisting->pExtraObjectList || pExisting->pExtraObjectList->IsEmpty()) &&
        !pExisting->GetCount()) {
        pListofChanges->Remove(pExisting);
        if (pExisting) {
            delete pExisting;
        }
        if (apChange) {
            delete apChange;
        }
    }
    pContext->~AutoMemContext();
}

ItemChange* InventoryChanges::GetBestAmmo(TESActorBase* apActorBase, bool abUseCanNotWear) {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_INVENTORY, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp",
        5045
    );
    float fBestDesirability = -FLT_MAX;
    TESBoundObject* pBestAmmo = nullptr;
    TESBoundObject* pBestAddedAmmo = nullptr;
    // The base container's ammo: kept unless the changes removed all of it (a negative entry always counts).
    // Ammo is held as TESBoundObject, its primary base, at the address the TESAmmo cast returns.
    for (BSSimpleList<ContainerObject*>* pNode = GetOriginalContainer()->GetObjectList(); pNode;
         pNode = pNode->GetNext()) {
        ContainerObject* pEntry = pNode->GetItem();
        TESBoundObject* pAmmo = nullptr;
        if (pEntry) {
            pAmmo = static_cast<TESBoundObject*>(
                FNVD_DYNAMIC_CAST_VOID(TESAmmo)(pEntry->pObj, RTTI_TESBoundObject, RTTI_TESAmmo)
            );
        }
        if (!pAmmo) {
            continue;
        }
        ItemChange* pItem = GetObjectInList(pAmmo, true, 0);
        if (!pItem || (pItem && pItem->iNumber + pNode->GetItem()->i_count > 0) || pNode->GetItem()->i_count < 0) {
            const float fDesirability = apActorBase->GetDesirability(pAmmo);
            if (fBestDesirability < fDesirability) {
                fBestDesirability = fDesirability;
                pBestAmmo = pAmmo;
            }
        }
    }
    // Ammo only the changes hold.
    for (BSSimpleList<ItemChange*>* pNode = pListofChanges; pNode; pNode = pNode->GetNext()) {
        ItemChange* pItem = pNode->GetItem();
        TESBoundObject* pAmmo = nullptr;
        if (pItem) {
            pAmmo = static_cast<TESBoundObject*>(
                FNVD_DYNAMIC_CAST_VOID(TESAmmo)(pItem->pContainerObj, RTTI_TESBoundObject, RTTI_TESAmmo)
            );
        }
        if (!pAmmo) {
            continue;
        }
        if (pItem->HasLeveledExtra() && pItem->iNumber < 0) {
            continue;
        }
        if (!pItem->GetCount()) {
            continue;
        }
        if (GetOriginalContainer()->HasObject(pAmmo)) {
            continue;
        }
        const float fDesirability = apActorBase->GetDesirability(pAmmo);
        if (fBestDesirability < fDesirability) {
            fBestDesirability = fDesirability;
            pBestAddedAmmo = pAmmo;
        }
    }
    // The added ammo's desirability is held as a double and asked for first.
    if (pBestAddedAmmo && pBestAddedAmmo != pBestAmmo) {
        const double dAddedDesirability = apActorBase->GetDesirability(pBestAddedAmmo);
        if (apActorBase->GetDesirability(pBestAmmo) < dAddedDesirability) {
            pBestAmmo = pBestAddedAmmo;
        }
    }
    ItemChange* pChange = GetObjectInList(pBestAmmo, true, 0);
    ItemChange* pBest = nullptr;
    if (pBestAmmo) {
        void* pMemory = ourMalloc(sizeof(ItemChange));
        pBest = pMemory ? ::new (pMemory) ItemChange : nullptr;
    }
    if (pChange) {
        pBest->pContainerObj = pChange->GetContainerObject();
        if (pChange->GetExtraDataList() && pChange->pExtraObjectList->GetItem()) {
            void* pMemory = ourMalloc(sizeof(BSSimpleList<ExtraDataList*>));
            pBest->pExtraObjectList = pMemory ? new (pMemory) BSSimpleList<ExtraDataList*>() : nullptr;
            pBest->pExtraObjectList->AddHead(pChange->pExtraObjectList->GetItem());
            pBest->SetCount(pChange->GetCount());
        }
    }
    else if (pBestAmmo) {
        pBest->pContainerObj = pBestAmmo;
        int32_t iCount = GetOriginalContainer()->GetObjectCount(pBestAmmo);
        if (iCount < 0) {
            iCount = iCount * -1;
        }
        pBest->SetCount(iCount);
    }
    pContext->~AutoMemContext();
    return pBest;
}

ItemChange* InventoryChanges::GetBestWeapon(
    TESActorBase* apActorBase,
    float& afvalue,
    COMBAT_WEAPON_TYPE aeType,
    bool abUseCanNotWear
) {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_INVENTORY, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp",
        5188
    );
    float fBestScore = 0.0f;
    ItemChange* pBest = nullptr;
    TESForm* pActorForm = apActorBase;
    Actor* pActor = nullptr;
    // An actor holding a live grenade keeps it: no weapon is chosen.
    if (pRef && pRef->IsActor()) {
        pActor = static_cast<Actor*>(pRef);
        pActorForm = pActor;
        if (pActor->GetCurrentAIProcess() && pActor->GetCurrentAIProcess()->GetPlantedExplosive()) {
            pContext->~AutoMemContext();
            return pBest;
        }
    }
    // A worn weapon that may not be unequipped wins outright; otherwise the probe change is freed.
    if (abUseCanNotWear) {
        ItemChange* pWorn = GetWornItem(BO_WEAPON, false);
        if (pWorn && pWorn->GetExtraDataList()->GetItem()->GetCanNotWear()) {
            pContext->~AutoMemContext();
            return pWorn;
        }
        if (pWorn) {
            delete pWorn;
        }
    }
    // Fetches the base container's list before the actor value reads.
    BSSimpleList<ContainerObject*>* pNode = GetOriginalContainer()->GetObjectList();
    ActorValueOwner& rActorValues = static_cast<ActorValueOwner&>(*apActorBase);
    // These actor values and the fatigue (and later the weapon skill and attack damage) are read without being used;
    // the calls are kept deliberately, as the game does.
    rActorValues.GetClampedActorFloatValue(ActorValue::LUCK);
    rActorValues.GetClampedActorValue(ActorValue::STRENGTH);
    if (pActor) {
        pActor->GetFatiguePercentage();
    }
    // The condition carries over to candidates without instance data, as the game does.
    float fCondition = 1.0f;
    // The base container's weapons: kept unless the changes removed all of them (a negative entry always counts).
    for (; pNode; pNode = pNode->GetNext()) {
        ContainerObject* pEntry = pNode->GetItem();
        TESObjectWEAP* pWeapon = nullptr;
        if (pEntry && pEntry->pObj->GetFormType() == WEAP_ID) {
            pWeapon = static_cast<TESObjectWEAP*>(pEntry->pObj);
        }
        if (!pWeapon) {
            continue;
        }
        ItemChange* pItem = GetObjectInList(pWeapon, true, 0);
        if (!(!pItem || (pItem && pItem->iNumber + pNode->GetItem()->i_count > 0) || pNode->GetItem()->i_count < 0)) {
            continue;
        }
        if (!pWeapon) {
            continue;
        }
        if (aeType != COMBAT_WEAPON_TYPE_COUNT && aeType != pWeapon->GetCombatWeaponType()) {
            continue;
        }
        bool bUsable = true;
        bool bNeedsAmmo = false;
        // Asks again whether the reference is an actor and drops the answer, as the game does.
        pRef->IsActor();
        TESAmmo* pAmmo = GetBestAmmoForWeapon(pWeapon, bNeedsAmmo);
        if (bNeedsAmmo && !pAmmo) {
            bUsable = false;
        }
        if (pActor && (!pActor->CanUseWeapon(pWeapon, true) || !bUsable)) {
            continue;
        }
        static_cast<TESAttackDamageForm&>(*pWeapon).GetAttackDamage();
        TESEnchantableForm* pEnchantable = pWeapon ? static_cast<TESEnchantableForm*>(pWeapon) : nullptr;
        EnchantmentItem* pEnchantment = pEnchantable ? pEnchantable->pFormEnchanting : nullptr;
        rActorValues.GetActorFloatValue(pWeapon->GetWeaponSkill());
        if (pItem && pItem->pExtraObjectList && pItem->pExtraObjectList->GetItem()) {
            // Drops the enchantment once its cost exceeds the charge; the enchantment is not used again.
            if (pEnchantment) {
                const double dCharge = pItem->GetItemCharge();
                if (static_cast<EffectItemList&>(*pEnchantment).GetCost(nullptr) > dCharge) {
                    pEnchantment = nullptr;
                }
            }
            // Each instance scores at its own condition; the best one gets a change of its own.
            for (BSSimpleList<ExtraDataList*>* pExtraNode = pItem->pExtraObjectList; pExtraNode;
                 pExtraNode = pExtraNode->GetNext()) {
                ExtraDataList* pExtra = pExtraNode->GetItem();
                if (!pExtra) {
                    continue;
                }
                BSExtraData* pHealthExtra = pExtra->GetExtraData(EXTRA_HEALTH);
                float fHealth = 0.0f;
                if (pHealthExtra) {
                    fHealth = pExtra->GetHealth();
                }
                else {
                    fHealth = static_cast<float>(TESHealthForm::GetFormHealth(pWeapon));
                }
                if (!(fHealth > 0.0)) {
                    continue;
                }
                // Divides by the base health loaded exactly, without rounding it to a float first.
                fCondition = static_cast<float>(fHealth / static_cast<double>(TESHealthForm::GetFormHealth(pWeapon)));
                const float fScore =
                    CombatFormulas::CalcCombatWeaponDPS(pActorForm, pWeapon, fCondition, nullptr, pItem, pAmmo);
                if (fBestScore < fScore) {
                    fBestScore = fScore;
                    if (pBest) {
                        delete pBest;
                    }
                    void* pMemory = ourMalloc(sizeof(ItemChange));
                    pBest = pMemory ? ::new (pMemory) ItemChange : nullptr;
                    pBest->pContainerObj = pWeapon;
                    void* pListMemory = ourMalloc(sizeof(BSSimpleList<ExtraDataList*>));
                    pBest->pExtraObjectList = pListMemory ? new (pListMemory) BSSimpleList<ExtraDataList*>() : nullptr;
                    pBest->pExtraObjectList->AddHead(pExtra);
                }
            }
            continue;
        }
        const float fScore =
            CombatFormulas::CalcCombatWeaponDPS(pActorForm, pWeapon, fCondition, nullptr, pItem, pAmmo);
        if (fBestScore < fScore) {
            fBestScore = fScore;
            if (pBest) {
                delete pBest;
            }
            void* pMemory = ourMalloc(sizeof(ItemChange));
            pBest = pMemory ? ::new (pMemory) ItemChange : nullptr;
            pBest->pContainerObj = pWeapon;
        }
    }
    // Weapons only the changes hold. Here the actor is asked about the weapon without a null test.
    for (BSSimpleList<ItemChange*>* pNode = pListofChanges; pNode && !pNode->IsEmpty(); pNode = pNode->GetNext()) {
        TESObjectWEAP* pWeapon = nullptr;
        ItemChange* pItem = pNode->GetItem();
        if (pItem->pContainerObj->GetFormType() == WEAP_ID) {
            pWeapon = static_cast<TESObjectWEAP*>(pItem->pContainerObj);
        }
        if (!pWeapon) {
            continue;
        }
        bool bUsable = true;
        bool bNeedsAmmo = false;
        pRef->IsActor();
        TESAmmo* pAmmo = GetBestAmmoForWeapon(pWeapon, bNeedsAmmo);
        if (bNeedsAmmo && !pAmmo) {
            bUsable = false;
        }
        if (aeType != COMBAT_WEAPON_TYPE_COUNT && aeType != pWeapon->GetCombatWeaponType()) {
            continue;
        }
        if (!pItem->GetCount() || GetOriginalContainer()->HasObject(pWeapon) || !pActor->CanUseWeapon(pWeapon, true)) {
            continue;
        }
        if (pItem->HasLeveledExtra() && pItem->iNumber < 0) {
            continue;
        }
        if (!bUsable) {
            continue;
        }
        // The score starts at -1 (float) and is skipped when it is below -1 (double), which never happens; the check is
        // kept as the game does it.
        const float fStartScore = -1.0f;
        if (fStartScore < -1.0) {
            continue;
        }
        rActorValues.GetActorFloatValue(pWeapon->GetWeaponSkill());
        static_cast<TESAttackDamageForm&>(*pWeapon).GetAttackDamage();
        if (pItem && pItem->pExtraObjectList && pItem->pExtraObjectList->GetItem()) {
            for (BSSimpleList<ExtraDataList*>* pExtraNode = pItem->pExtraObjectList;
                 pExtraNode && pExtraNode->GetItem(); pExtraNode = pExtraNode->GetNext()) {
                ExtraDataList* pExtra = pExtraNode->GetItem();
                BSExtraData* pHealthExtra = pExtra->GetExtraData(EXTRA_HEALTH);
                float fHealth = 0.0f;
                if (pHealthExtra) {
                    fHealth = pExtra->GetHealth();
                }
                else {
                    fHealth = static_cast<float>(TESHealthForm::GetFormHealth(pWeapon));
                }
                if (!(fHealth > 0.0)) {
                    continue;
                }
                fCondition = static_cast<float>(fHealth / static_cast<double>(TESHealthForm::GetFormHealth(pWeapon)));
                const float fScore =
                    CombatFormulas::CalcCombatWeaponDPS(pActorForm, pWeapon, fCondition, nullptr, pItem, pAmmo);
                if (fBestScore < fScore) {
                    fBestScore = fScore;
                    if (pBest) {
                        delete pBest;
                    }
                    void* pMemory = ourMalloc(sizeof(ItemChange));
                    pBest = pMemory ? ::new (pMemory) ItemChange : nullptr;
                    pBest->pContainerObj = pWeapon;
                    void* pListMemory = ourMalloc(sizeof(BSSimpleList<ExtraDataList*>));
                    pBest->pExtraObjectList = pListMemory ? new (pListMemory) BSSimpleList<ExtraDataList*>() : nullptr;
                    pBest->pExtraObjectList->AddHead(pExtra);
                }
            }
            continue;
        }
        const float fScore =
            CombatFormulas::CalcCombatWeaponDPS(pActorForm, pWeapon, fCondition, nullptr, pItem, pAmmo);
        if (fBestScore < fScore) {
            fBestScore = fScore;
            if (pBest) {
                delete pBest;
            }
            void* pMemory = ourMalloc(sizeof(ItemChange));
            pBest = pMemory ? ::new (pMemory) ItemChange : nullptr;
            pBest->pContainerObj = pWeapon;
        }
    }
    afvalue = CombatFormulas::ChooseMeleeWeapon(fBestScore, 0, false);
    // Explosives and thrown weapons are chosen as a stack: the change takes the inventory's count.
    if (pBest) {
        TESObjectWEAP* pWeapon = static_cast<TESObjectWEAP*>(pBest->GetContainerObject());
        if (pWeapon->GetWeaponType() == WEAPON_ONE_HAND_GRENADE || pWeapon->GetWeaponType() == WEAPON_ONE_HAND_MINE ||
            pWeapon->GetWeaponType() == WEAPON_ONE_HAND_THROWN) {
            pBest->SetCount(GetObjectCount(pBest->GetContainerObject()));
        }
    }
    pContext->~AutoMemContext();
    return pBest;
}

ItemChange* InventoryChanges::GetBestArmor(TESActorBase* apActorBase, BIPED_OBJECT aeBodySlot, bool abUseCanNotWear) {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_INVENTORY, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp",
        5694
    );
    float fBestScore = -FLT_MAX;
    ItemChange* pBest = nullptr;
    // A worn item that may not be unequipped wins outright; otherwise the probe change is freed.
    if (abUseCanNotWear) {
        ItemChange* pWorn = GetWornItem(aeBodySlot, false);
        if (pWorn && pWorn->GetExtraDataList()->GetItem()->GetCanNotWear()) {
            pContext->~AutoMemContext();
            return pWorn;
        }
        if (pWorn) {
            delete pWorn;
        }
    }
    // The base container's armor: kept unless the changes removed all of it (a negative entry always counts).
    for (BSSimpleList<ContainerObject*>* pNode = GetOriginalContainer()->GetObjectList(); pNode && !pNode->IsEmpty();
         pNode = pNode->GetNext()) {
        TESObjectARMO* pArmor = dynamic_cast<TESObjectARMO*>(pNode->GetItem()->pObj);
        ItemChange* pItem = GetObjectInList(pArmor, true, 0);
        if (!(!pItem || (pItem && pItem->iNumber + pNode->GetItem()->i_count > 0) || pNode->GetItem()->i_count < 0)) {
            continue;
        }
        if (!pArmor || aeBodySlot == -1 || !pArmor->FillsBipedSlot(aeBodySlot, false, nullptr)) {
            continue;
        }
        if (pItem && pItem->pExtraObjectList && pItem->pExtraObjectList->GetItem()) {
            // Each instance scores at its own health; the best one gets a change of its own.
            for (BSSimpleList<ExtraDataList*>* pExtraNode = pItem->pExtraObjectList;
                 pExtraNode && pExtraNode->GetItem(); pExtraNode = pExtraNode->GetNext()) {
                ExtraDataList* pExtra = pExtraNode->GetItem();
                BSExtraData* pHealthExtra = pExtra->GetExtraData(EXTRA_HEALTH);
                float fHealth = 0.0f;
                if (pHealthExtra) {
                    fHealth = pExtra->GetHealth();
                }
                else {
                    fHealth = static_cast<float>(TESHealthForm::GetFormHealth(pArmor));
                }
                if (!(fHealth > 0.0)) {
                    continue;
                }
                const float fRating = CombatFormulas::CalcArmorRating(
                    static_cast<uint16_t>(static_cast<int32_t>(pArmor->GetArmorRating())), fHealth
                );
                const float fScore = pArmor->GetDamageThreshold() + fRating;
                if (fBestScore < fScore) {
                    fBestScore = fScore;
                    if (pBest) {
                        delete pBest;
                    }
                    void* pMemory = ourMalloc(sizeof(ItemChange));
                    pBest = pMemory ? ::new (pMemory) ItemChange : nullptr;
                    pBest->pContainerObj = pArmor;
                    void* pListMemory = ourMalloc(sizeof(BSSimpleList<ExtraDataList*>));
                    pBest->pExtraObjectList = pListMemory ? new (pListMemory) BSSimpleList<ExtraDataList*>() : nullptr;
                    pBest->pExtraObjectList->AddHead(pExtra);
                }
            }
            continue;
        }
        const float fHealth = static_cast<float>(TESHealthForm::GetFormHealth(pArmor));
        const float fRating = CombatFormulas::CalcArmorRating(
            static_cast<uint16_t>(static_cast<int32_t>(pArmor->GetArmorRating())), fHealth
        );
        const float fScore = pArmor->GetDamageThreshold() + fRating;
        if (fBestScore < fScore) {
            fBestScore = fScore;
            if (pBest) {
                delete pBest;
            }
            void* pMemory = ourMalloc(sizeof(ItemChange));
            pBest = pMemory ? ::new (pMemory) ItemChange : nullptr;
            pBest->pContainerObj = pArmor;
        }
    }
    // Armor only the changes hold, with a non-negative count change.
    for (BSSimpleList<ItemChange*>* pNode = pListofChanges; pNode && !pNode->IsEmpty(); pNode = pNode->GetNext()) {
        TESObjectARMO* pArmor = dynamic_cast<TESObjectARMO*>(pNode->GetItem()->pContainerObj);
        ItemChange* pItem = pNode->GetItem();
        if (!pArmor || !pItem->GetCount() || GetOriginalContainer()->HasObject(pArmor)) {
            continue;
        }
        if (aeBodySlot == -1 || !pArmor->FillsBipedSlot(aeBodySlot, false, nullptr) || pItem->iNumber < 0) {
            continue;
        }
        if (pItem && pItem->pExtraObjectList && pItem->pExtraObjectList->GetItem()) {
            for (BSSimpleList<ExtraDataList*>* pExtraNode = pItem->pExtraObjectList;
                 pExtraNode && pExtraNode->GetItem(); pExtraNode = pExtraNode->GetNext()) {
                ExtraDataList* pExtra = pExtraNode->GetItem();
                BSExtraData* pHealthExtra = pExtra->GetExtraData(EXTRA_HEALTH);
                float fHealth = 0.0f;
                if (pHealthExtra) {
                    fHealth = pExtra->GetHealth();
                }
                else {
                    fHealth = static_cast<float>(TESHealthForm::GetFormHealth(pArmor));
                }
                if (!(fHealth > 0.0)) {
                    continue;
                }
                const float fRating = CombatFormulas::CalcArmorRating(
                    static_cast<uint16_t>(static_cast<int32_t>(pArmor->GetArmorRating())), fHealth
                );
                const float fScore = pArmor->GetDamageThreshold() + fRating;
                if (fBestScore < fScore) {
                    fBestScore = fScore;
                    if (pBest) {
                        delete pBest;
                    }
                    void* pMemory = ourMalloc(sizeof(ItemChange));
                    pBest = pMemory ? ::new (pMemory) ItemChange : nullptr;
                    pBest->pContainerObj = pArmor;
                    void* pListMemory = ourMalloc(sizeof(BSSimpleList<ExtraDataList*>));
                    pBest->pExtraObjectList = pListMemory ? new (pListMemory) BSSimpleList<ExtraDataList*>() : nullptr;
                    pBest->pExtraObjectList->AddHead(pExtra);
                }
            }
            continue;
        }
        const float fHealth = static_cast<float>(TESHealthForm::GetFormHealth(pArmor));
        const float fRating = CombatFormulas::CalcArmorRating(
            static_cast<uint16_t>(static_cast<int32_t>(pArmor->GetArmorRating())), fHealth
        );
        const float fScore = pArmor->GetDamageThreshold() + fRating;
        if (fBestScore < fScore) {
            fBestScore = fScore;
            if (pBest) {
                delete pBest;
            }
            void* pMemory = ourMalloc(sizeof(ItemChange));
            pBest = pMemory ? ::new (pMemory) ItemChange : nullptr;
            pBest->pContainerObj = pArmor;
        }
    }
    pContext->~AutoMemContext();
    return pBest;
}

// Constructs and destroys an AutoMemContext on a stack buffer and ignores the constructor's result.
ItemChange* InventoryChanges::GetBestFood() {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_INVENTORY, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp",
        6906
    );
    // The casts live across the iterations of both loops: an ingredient leaves the last alchemy cast in place,
    // and the food test still looks at it.
    IngredientItem* pIngredient = nullptr;
    TESBoundObject* pFood = nullptr;
    AlchemyItem* pAlchemy = nullptr;
    // The base container's food, unless the changes removed all of it; a new change when there is none.
    for (BSSimpleList<ContainerObject*>* pNode = GetOriginalContainer()->GetObjectList(); pNode && !pNode->IsEmpty();
         pNode = pNode->GetNext()) {
        pIngredient = dynamic_cast<IngredientItem*>(pNode->GetItem()->pObj);
        if (!pIngredient) {
            pAlchemy = dynamic_cast<AlchemyItem*>(pNode->GetItem()->pObj);
            pFood = pAlchemy;
        }
        else {
            pFood = pIngredient;
        }
        if (!(pAlchemy && pAlchemy->IsFood()) && !(pIngredient && pIngredient->IsFood())) {
            continue;
        }
        ItemChange* pItem = GetObjectInList(pFood, true, 0);
        if (pItem && (!pItem || pItem->GetCount() + pNode->GetItem()->i_count == 0)) {
            continue;
        }
        if (!pItem) {
            void* pMemory = ourMalloc(sizeof(ItemChange));
            pItem = pMemory ? ::new (pMemory) ItemChange(pFood, 0) : nullptr;
        }
        pContext->~AutoMemContext();
        return pItem;
    }
    // Food only the changes hold, with a positive count change.
    for (BSSimpleList<ItemChange*>* pNode = pListofChanges; pNode && !pNode->IsEmpty(); pNode = pNode->GetNext()) {
        pIngredient = dynamic_cast<IngredientItem*>(pNode->GetItem()->pContainerObj);
        if (!pIngredient) {
            pAlchemy = dynamic_cast<AlchemyItem*>(pNode->GetItem()->pContainerObj);
            pFood = pAlchemy;
        }
        else {
            pFood = pIngredient;
        }
        if (!(pIngredient && pIngredient->IsFood()) && !(pAlchemy && pAlchemy->IsFood())) {
            continue;
        }
        if (GetOriginalContainer()->HasObject(pFood)) {
            continue;
        }
        if (pNode->GetItem()->GetCount() <= 0) {
            continue;
        }
        ItemChange* pResult = pNode->GetItem();
        pContext->~AutoMemContext();
        return pResult;
    }
    pContext->~AutoMemContext();
    return nullptr;
}

void InventoryChanges::ClearAllChangeItems() {
    BSSimpleList<ItemChange*>* pkFirst = pListofChanges;
    BSSimpleList<ItemChange*>* pkHead = pListofChanges;
    ItemChange* pitem = 0;
    while (pkFirst && pkFirst->m_item) {
        pitem = pkFirst->m_item;
        ExtraDataList* pextra = 0;
        while (pitem->pExtraObjectList && pitem->pExtraObjectList->m_item) {
            pextra = pitem->pExtraObjectList->m_item;
            pitem->pExtraObjectList->RemoveHead();
            TESObjectREFR* pref = 0;
            if (pextra) {
                pref = pextra->GetOriginalReference();
            }
            if (pref) {
                if (pref->HasContainer()) {
                    pextra->RemoveOriginalReferenceExtra();
                    if (pextra->ItemsInList() < 2) {
                        if (pextra->GetCount()) {
                            pextra->RemoveCount();
                        }
                    }
                    pitem->pExtraObjectList->Remove(pextra);
                    InventoryChanges* pchangeref = (&pref->m_Extra)->GetContainerChanges();
                    bool bdelete = 1;
                    if (pchangeref) {
                        ItemChange* ptempitem = pchangeref->GetObjectInList(pitem->pContainerObj, 1, 0);
                        if (ptempitem) {
                            if (ptempitem->pExtraObjectList) {
                                BSSimpleList<ExtraDataList*>* ptempObj = ptempitem->pExtraObjectList;
                                while (ptempObj && (ptempObj->m_item && bdelete)) {
                                    BSSimpleList<ExtraDataList*>* v1 = ptempObj;
                                    if (v1->m_item == pextra) {
                                        bdelete = 0;
                                    }
                                    else {
                                        ptempObj = ptempObj->m_pkNext;
                                    }
                                }
                            }
                        }
                    }
                    InventoryChanges* pchangeref_2 = (&pPlayer->m_Extra)->GetContainerChanges();
                    if (pchangeref_2) {
                        ItemChange* ptempitem_2 = pchangeref_2->GetObjectInList(pitem->pContainerObj, 1, 0);
                        if (ptempitem_2) {
                            if (ptempitem_2->pExtraObjectList) {
                                BSSimpleList<ExtraDataList*>* ptempObj_2 = ptempitem_2->pExtraObjectList;
                                while (ptempObj_2 && (ptempObj_2->m_item && bdelete)) {
                                    ExtraDataList* m_item = ptempObj_2->m_item;
                                    if (m_item == pextra) {
                                        bdelete = 0;
                                    }
                                    else {
                                        ptempObj_2 = ptempObj_2->m_pkNext;
                                    }
                                }
                            }
                        }
                    }
                    if (bdelete) {
                        if (pextra) {
                            delete pextra;
                        }
                    }
                    else {
                        pextra = 0;
                    }
                }
                else {
                    pextra->RemoveOriginalReferenceExtra();
                    if (!pextra->GetScript()) {
                        if (pextra) {
                            delete pextra;
                        }
                    }
                    pextra = 0;
                }
            }
            else {
                pextra = 0;
            }
        }
        pkFirst->Remove(pitem);
        if (pitem) {
            delete pitem;
        }
        pitem = 0;
        pkFirst = pkHead;
    }
}

float InventoryChanges::GetInventoryWeight(bool abIsHardcore) {
    int32_t v20;
    int32_t v21;
    int32_t v25;
    int32_t fPackRat;
    Actor* pactor = 0;
    if (pRef->IsActor()) {
        pactor = static_cast<Actor*>(pRef);
    }
    bool bHasLightItemsReduced = 0;
    *reinterpret_cast<float*>(&fPackRat) = 0.0;
    BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::HAS_LIGHT_ITEMS_WEIGH_NOTHING, pactor, &fPackRat);
    float fPackRatMod = fPackRatModifier.Float();
    float fPackRatThresh = fPackRatThreshold.Float();
    if (*reinterpret_cast<float*>(&fPackRat) > 0.0) {
        bHasLightItemsReduced = 1;
    }
    if (fcontainerweight == -1.0) {
        float famount = 0.0f;
        float fweight = 0.0f;
        float fModWeight = 0.0f;
        for (BSSimpleList<ContainerObject*>* pList = &GetOriginalContainer()->objectList; pList && !pList->IsEmpty();
             pList = pList->m_pkNext) {
            TESBoundObject* pObj = pList->m_item->pObj;
            ItemChange* pitem = GetObjectInList(pObj, 1, 0);
            if (!pObj->GetQuestObject()) {
                if (pObj->GetFormType() == WEAP_ID && pitem) {
                    TESBoundObject* pObj_2 = pObj;
                    bool HasModEffectActive_0 = pitem->HasModEffectActive(4);
                    fweight = static_cast<TESObjectWEAP*>(pObj_2)->TESObjectWEAP::GetFormWeight(HasModEffectActive_0);
                    if (fweight >= 10.0) {
                        *reinterpret_cast<float*>(&v25) = 1.0f;
                        BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::NOT_YET_IMPLEMENTED, pPlayer, &v25);
                        fweight = fweight * *reinterpret_cast<float*>(&v25);
                    }
                }
                else {
                    fweight = TESWeightForm::GetFormWeight(pObj, abIsHardcore);
                }
                if (fweight == -1.0) {
                    fweight = 0.0f;
                }
                if (bHasLightItemsReduced && fPackRatThresh >= static_cast<double>(fweight)) {
                    fweight = fweight * fPackRatMod;
                }
                if (pitem) {
                    uint32_t FormHealth_0 = pitem->iNumber;
                    if (pList->m_item->i_count + FormHealth_0) {
                        uint32_t v5 = pitem->iNumber;
                        int32_t v12 = pList->m_item->i_count + v5;
                        famount = static_cast<double>(v12) * fweight + famount;
                    }
                }
                else {
                    BSSimpleList<ContainerObject*>* v3 = pList;
                    famount = static_cast<double>(v3->m_item->i_count) * fweight + famount;
                }
            }
        }
        for (BSSimpleList<ItemChange*>* pInvList = pListofChanges;
             pInvList && (!pInvList->IsEmpty() && pInvList->m_item); pInvList = pInvList->m_pkNext) {
            bool bIsWeapon = 0;
            ItemChange* pitem_2 = pInvList->m_item;
            TESBoundObject* pObj_3 = pitem_2->pContainerObj;
            if (pObj_3) {
                TESContainer* OriginalContainer;
                TESBoundObject* v10;
                if (!GetOriginalContainer() ||
                    (v10 = pObj_3, (OriginalContainer = GetOriginalContainer(), !OriginalContainer->HasObject(v10)))) {
                    if (!pObj_3->GetQuestObject()) {
                        if (pObj_3->GetFormType() == WEAP_ID) {
                            bIsWeapon = 1;
                            bool v7 = pitem_2->HasModEffectActive(4);
                            fModWeight = static_cast<TESObjectWEAP*>(pObj_3)->TESObjectWEAP::GetFormWeight(v7);
                            fweight = TESWeightForm::GetFormWeight(pObj_3, abIsHardcore);
                            if (fweight >= 10.0) {
                                *reinterpret_cast<float*>(&v21) = 1.0f;
                                BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::NOT_YET_IMPLEMENTED, pPlayer, &v21);
                                fweight = fweight * *reinterpret_cast<float*>(&v21);
                            }
                            if (fModWeight >= 10.0) {
                                *reinterpret_cast<float*>(&v20) = 1.0f;
                                BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::NOT_YET_IMPLEMENTED, pPlayer, &v20);
                                fModWeight = fModWeight * *reinterpret_cast<float*>(&v20);
                            }
                        }
                        else {
                            fweight = TESWeightForm::GetFormWeight(pObj_3, abIsHardcore);
                        }
                        if (fweight > 0.0) {
                            if (bHasLightItemsReduced && fPackRatThresh >= static_cast<double>(fweight)) {
                                fweight = fweight * fPackRatMod;
                            }
                            if (bHasLightItemsReduced && fPackRatThresh >= static_cast<double>(fModWeight)) {
                                fModWeight = fModWeight * fPackRatMod;
                            }
                            TESBoundObject* v18 = 0;
                            if (pObj_3) {
                                if (pObj_3->GetFormType() == ARMO_ID) {
                                    v18 = pObj_3;
                                }
                            }
                            int32_t count = pitem_2->iNumber;
                            if (pitem_2->pExtraObjectList && (bIsWeapon && pitem_2->GetExtraTotalCount(1) > 0)) {
                                TESObjectWEAP* pweap = static_cast<TESObjectWEAP*>(pitem_2->pContainerObj);
                                int32_t modCount = 0;
                                for (ExtraDataList* pExtra : *pitem_2->pExtraObjectList) {
                                    if (pExtra) {
                                        if (pExtra->HasWeaponMods()) {
                                            if (pExtra->GetWeaponModSlotActive(1) &&
                                                pweap->GetModEffect(1u) == WEAPON_MOD_DECREASE_WEIGHT) {
                                                modCount += pExtra->GetCount();
                                            }
                                            if (pExtra->GetWeaponModSlotActive(2) &&
                                                pweap->GetModEffect(2u) == WEAPON_MOD_DECREASE_WEIGHT) {
                                                modCount += pExtra->GetCount();
                                            }
                                            if (pExtra->GetWeaponModSlotActive(4) &&
                                                pweap->GetModEffect(4u) == WEAPON_MOD_DECREASE_WEIGHT) {
                                                modCount += pExtra->GetCount();
                                            }
                                        }
                                    }
                                }
                                fweight = static_cast<double>(count - modCount) * fweight +
                                    static_cast<double>(modCount) * fModWeight;
                            }
                            else {
                                if (bIsWeapon) {
                                    fweight = static_cast<double>(count) * fweight;
                                }
                            }
                            if (pactor) {
                                if (v18) {
                                    if (pitem_2->GetWorn(0) && count > 0) {
                                        famount = famount + fweight;
                                        --count;
                                    }
                                }
                            }
                            if (count <= 0 || bIsWeapon) {
                                if (count) {
                                    famount = famount + fweight;
                                }
                            }
                            else {
                                famount = static_cast<double>(count) * fweight + famount;
                            }
                        }
                    }
                }
            }
        }
        fcontainerweight = famount;
        if (pactor) {
            float a4 = fcontainerweight - fpreviousContainerWeight;
            Actor::HandleChange_008BA2F0(pactor, ActorValue::INVENTORY_WEIGHT, fpreviousContainerWeight, a4, 0);
        }
    }
    return fcontainerweight;
}

float InventoryChanges::RemoveAllItems(
    TESObjectREFR* apRef,
    TESObjectREFR* apStolenContainer,
    bool abStealing,
    bool abowner,
    bool abRemoveOwnership,
    bool abSilent,
    int32_t aiFormType,
    BGSListForm* apDoNotRemoveList
) {
    TESForm* pcontainerForm;
    bool v106;
    bool abRemoved;
    bool a10;
    bool bremoved;
    TESBoundObject* pObj = 0;
    float fvalue = 0.0f;
    bool bquest = 0;
    bool busefilteredlist = apDoNotRemoveList != 0;
    BSSimpleList<ContainerObject*>* pList = &GetOriginalContainer()->objectList;
    TESLevItem* v134 = 0;
    while (pList && !pList->IsEmpty()) {
        TESForm* item;
        pObj = pList->m_item->pObj;
        TESLevItem* pleveled = dynamic_cast<TESLevItem*>(pList->m_item->pObj);
        TESBipedModelForm* pbipedform = TESBipedModelForm::GetFormAsBipedModel(pObj);
        if (!busefilteredlist || (item = (TESForm*)pObj, !(&apDoNotRemoveList->ListOfForms)->IsInList(item))) {
            if (aiFormType <= -1 || (!pObj || pObj->GetFormType() == aiFormType)) {
                if (pObj->GetQuestObject() && apRef == pPlayer) {
                    ItemChange* pitem = GetObjectInList(pObj, 1, 0);
                    if (pitem) {
                        if (pitem->GetWorn(0)) {
                            TESBoundObject* v44 = pList->m_item->pObj;
                            TESContainer* OriginalContainer = GetOriginalContainer();
                            int32_t iorigcount = OriginalContainer->GetObjectCount(v44);
                            uint32_t FormHealth_0 = pitem->iNumber;
                            int32_t iamount = iorigcount + FormHealth_0;
                            bremoved = 0;
                            RemoveWorn(
                                &bremoved, pObj, iorigcount + FormHealth_0, static_cast<Actor*>(apRef), 0, 0, 0, 0, 0
                            );
                        }
                    }
                }
                else {
                    if (pObj) {
                        if (!pleveled && (!pbipedform || pbipedform->GetPlayable())) {
                            ItemChange* pitem_2 = GetObjectInList(pObj, 1, 0);
                            if (!pitem_2 || !pitem_2->HasLeveledExtra()) {
                                int32_t iamount_2 = 0;
                                TESBoundObject* v45 = pList->m_item->pObj;
                                TESContainer* v11 = GetOriginalContainer();
                                int32_t iorigcount_2 = v11->GetObjectCount(v45);
                                if (iorigcount_2 < 0) {
                                    iorigcount_2 = -iorigcount_2;
                                }
                                if (pitem_2) {
                                    uint32_t v12 = pitem_2->iNumber;
                                    iamount_2 += iorigcount_2 + v12;
                                }
                                else {
                                    iamount_2 += iorigcount_2;
                                }
                                if (iamount_2 > 0) {
                                    int32_t FormValue = TESValueForm::GetFormValue(pObj);
                                    fvalue = static_cast<double>(iamount_2 * FormValue) + fvalue;
                                    char v123 = 0;
                                    if (pitem_2) {
                                        if (pitem_2->pExtraObjectList) {
                                            if (pitem_2->pExtraObjectList->ItemsInList()) {
                                                BSSimpleList<ExtraDataList*>* plistextra = pitem_2->pExtraObjectList;
                                                if (plistextra) {
                                                    if (plistextra->m_item) {
                                                        bool bflag = 0;
                                                        while (plistextra && (plistextra->m_item && !bflag)) {
                                                            ExtraDataList* pextra = plistextra->m_item;
                                                            if (pextra->ItemsInList()) {
                                                                uint32_t v15;
                                                                Actor* v31;
                                                                ExtraDataList* v32;
                                                                ItemChange* time;
                                                                bool bextrabad = 0;
                                                                if (static_cast<uint32_t>(
                                                                        pitem_2->pExtraObjectList->ItemsInList()
                                                                    ) > 1) {
                                                                    bextrabad = 1;
                                                                }
                                                                else {
                                                                    bflag = 1;
                                                                }
                                                                TESForm* powner = 0;
                                                                if (apRef->IsActor()) {
                                                                    TESBoundObject* v116 = 0;
                                                                    TESBoundObject* pcompobj =
                                                                        (TESBoundObject*)(&apRef->m_Extra)
                                                                            ->GetLevCreaOriginalBase();
                                                                    if (!pcompobj) {
                                                                        pcompobj = apRef->GetObjectReference();
                                                                    }
                                                                    powner = (TESForm*)pcompobj;
                                                                }
                                                                else {
                                                                    powner = apRef->GetOwner();
                                                                }
                                                                if ((abStealing || abowner) &&
                                                                    !TESContainer::IsGold(pObj)) {
                                                                    if (!pextra->GetOwner()) {
                                                                        pextra->SetOwnership(powner);
                                                                    }
                                                                }
                                                                else {
                                                                    if (abRemoveOwnership) {
                                                                        int32_t inumb = pextra->ItemsInList();
                                                                        if (pextra->GetOwner() &&
                                                                            ((pextra->GetCount() > 1 && inumb <= 2) ||
                                                                             inumb <= 1)) {
                                                                            pextra->RemoveOwnership();
                                                                            pextra = 0;
                                                                        }
                                                                        else {
                                                                            pextra->RemoveOwnership();
                                                                        }
                                                                    }
                                                                }
                                                                int16_t Count = pextra->GetCount();
                                                                iamount_2 -= Count;
                                                                if (!pitem_2->GetWorn(0) ||
                                                                    (abRemoved = 0,
                                                                     (a10 = 0,
                                                                      (time = pitem_2,
                                                                       (v32 = pextra,
                                                                        (v31 = apRef->IsActor()
                                                                             ? static_cast<Actor*>(apRef)
                                                                             : 0,
                                                                         (v15 = pitem_2->iNumber,
                                                                          (RemoveWorn(
                                                                               &abRemoved, pObj, v15, v31, v32, 0, 0,
                                                                               time, &a10
                                                                           ),
                                                                           !a10)))))))) {
                                                                    ItemChange* v46 = pitem_2;
                                                                    ExtraDataList* v27 = pextra;
                                                                    int16_t v16 = pextra->GetCount();
                                                                    RemoveItem(
                                                                        apRef, pObj, abStealing, v16, v27, 0,
                                                                        apStolenContainer, 0, 0, 1, 0, v46
                                                                    );
                                                                }
                                                                v123 = 1;
                                                                if (bflag) {
                                                                    pitem_2 = 0;
                                                                    plistextra = 0;
                                                                }
                                                                else {
                                                                    BSSimpleList<ExtraDataList*>* ExtraDataList_;
                                                                    if (bextrabad) {
                                                                        ExtraDataList_ = pitem_2->pExtraObjectList;
                                                                    }
                                                                    else {
                                                                        ExtraDataList_ = plistextra->m_pkNext;
                                                                    }
                                                                    plistextra = ExtraDataList_;
                                                                }
                                                            }
                                                            else {
                                                                plistextra->RemoveHead();
                                                                if (plistextra->IsEmpty()) {
                                                                    plistextra->RemoveAll();
                                                                    BSSimpleList<ExtraDataList*>* v81 = plistextra;
                                                                    BSSimpleList<ExtraDataList*>* plistextra_2 =
                                                                        plistextra;
                                                                    if (plistextra) {
                                                                        delete plistextra_2;
                                                                    }
                                                                    plistextra = 0;
                                                                    pitem_2->pExtraObjectList = 0;
                                                                }
                                                                else {
                                                                    plistextra = pitem_2->pExtraObjectList;
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                    if (iamount_2 > 0) {
                                        if (abStealing || abowner) {
                                            ItemChange* pitem_3;
                                            void* apMemory = ourMalloc(sizeof(ItemChange));
                                            if (apMemory) {
                                                pitem_3 = ::new (apMemory) ItemChange(pObj, iamount_2);
                                            }
                                            else {
                                                pitem_3 = 0;
                                            }
                                            ItemChange* v80 = pitem_3;
                                            pitem_2 = pitem_3;
                                            TESForm* apowner = 0;
                                            TESBoundObject* pcompobj_2 =
                                                (TESBoundObject*)(&apRef->m_Extra)->GetLevCreaOriginalBase();
                                            if (!pcompobj_2) {
                                                pcompobj_2 = apRef->GetObjectReference();
                                            }
                                            pitem_2->SetItemOwnership(pcompobj_2);
                                            ExtraDataList* pextra_2 = pitem_2->pExtraObjectList->m_item;
                                            pextra_2->SetCount(iamount_2);
                                            ExtraDataList* v28 = pextra_2;
                                            int32_t v25 = pextra_2->GetCount();
                                            RemoveItem(
                                                apRef, pObj, abStealing, v25, v28, 0, apStolenContainer, 0, 0, 1, 0, 0
                                            );
                                        }
                                        else {
                                            RemoveItem(
                                                apRef, pObj, 0, iamount_2, 0, 0, apStolenContainer, 0, 0, 1, 0, 0
                                            );
                                        }
                                        if (apStolenContainer == pPlayer) {
                                            if (pObj) {
                                                if (!abSilent) {
                                                    BSStringT<char> pstring;
                                                    if (iamount_2 <= 1) {
                                                        const char* v48 = sAddItemtoInventory.String();
                                                        pstring.SPrintF("%s %s", TESFullName::GetFullName(pObj), v48);
                                                    }
                                                    else {
                                                        const char* v47 = sAddItemtoInventory.String();
                                                        const char* timea = sPlural.String();
                                                        const char* FullName = TESFullName::GetFullName(pObj);
                                                        pstring.SPrintF("%i %s%s %s", iamount_2, FullName, timea, v47);
                                                    }
                                                    switch (pObj->GetFormType()) {
                                                        case ARMO_ID: {
                                                            const char* apSoundName =
                                                                pPlayer->GetPickUpSoundName(pObj, 1, 0);
                                                            Interface::ShowMessage(
                                                                pstring.pString, 0,
                                                                "Interface\\Icons\\Message "
                                                                "Icons\\glow_message_vaultboy_neutral.dds",
                                                                apSoundName, 2.0f, 0
                                                            );
                                                            break;
                                                        }
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
                                                            const char* soundName =
                                                                pPlayer->GetPickUpSoundName(pObj, 1, 0);
                                                            Interface::ShowMessage(
                                                                pstring.pString, 0,
                                                                "Interface\\Icons\\Message "
                                                                "Icons\\glow_message_vaultboy_neutral.dds",
                                                                soundName, 2.0f, 0
                                                            );
                                                            break;
                                                        }
                                                        default: {
                                                            break;
                                                        }
                                                    }
                                                }
                                            }
                                            Interface::UpdateInventory();
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        pList = pList->m_pkNext;
    }
    ItemChange* v132 = 0;
    BSSimpleList<ItemChange*>* pListInv = pListofChanges;
LABEL_96:;
    while (pListInv && !pListInv->IsEmpty()) {
        TESContainer* v18;
        TESObject* v49;
        TESBipedModelForm* pbipedform_2;
        char v108 = 0;
        ItemChange* pInventoryItem = pListInv->m_item;
        if ((busefilteredlist &&
             (pcontainerForm = pInventoryItem->pContainerObj,
              (&apDoNotRemoveList->ListOfForms)->IsInList(pcontainerForm))) ||
            ((aiFormType > -1 && (pInventoryItem && pInventoryItem->pContainerObj->GetFormType() != aiFormType)) ||
             (!pInventoryItem ||
              (((pbipedform_2 = TESBipedModelForm::GetFormAsBipedModel(pInventoryItem->pContainerObj)) != 0 &&
                !pbipedform_2->GetPlayable()) ||
               (!pInventoryItem->HasLeveledExtra() &&
                (GetOriginalContainer() &&
                 (v49 = (TESObject*)pInventoryItem->pContainerObj,
                  (v18 = GetOriginalContainer(), v18->HasObject(v49))))))))) {
        LABEL_103:;
            pListInv = pListInv->m_pkNext;
        }
        else {
            TESBoundObject* FormFlags = pInventoryItem->pContainerObj;
            if (FormFlags->GetQuestObject() && apRef == pPlayer) {
                if (pInventoryItem) {
                    if (pInventoryItem->GetWorn(0)) {
                        uint32_t v105 = pInventoryItem->iNumber;
                        v106 = 0;
                        char v103 = 0;
                        bool bleft = pInventoryItem->GetWorn(1);
                        RemoveWorn(
                            &v106, pInventoryItem->pContainerObj, v105, static_cast<Actor*>(apRef), 0, bleft,
                            pInventoryItem != 0, 0, 0
                        );
                    }
                }
                goto LABEL_103;
            }
            if (pInventoryItem) {
                if (pInventoryItem->iNumber > 0) {
                    TESBoundObject* v71;
                    if (!bquest || (v71 = pInventoryItem->pContainerObj, !v71->GetQuestObject())) {
                        pObj = pInventoryItem->pContainerObj;
                        int32_t v19 = TESValueForm::GetFormValue(pObj);
                        int32_t v70 = pInventoryItem->iNumber * v19;
                        fvalue = static_cast<double>(v70) + fvalue;
                        bool bflag_2 = 0;
                        int32_t ioriginalcount = 0;
                        if (GetOriginalContainer()) {
                            if (pInventoryItem) {
                                TESObject* pContainerObj = (TESObject*)pInventoryItem->pContainerObj;
                                TESContainer* v20 = GetOriginalContainer();
                                ioriginalcount = v20->GetObjectCount(pContainerObj);
                            }
                        }
                        int32_t inumberremove = pInventoryItem->iNumber + ioriginalcount;
                        BSSimpleList<ExtraDataList*>* plistextra_3 = pInventoryItem->pExtraObjectList;
                        if (!plistextra_3 || !plistextra_3->m_item) {
                            ExtraDataList* v89 = 0;
                            TESForm* powner_3 = 0;
                            if (apRef->IsActor()) {
                                TESBoundObject* v85 = 0;
                                TESBoundObject* pcompobj_4 =
                                    (TESBoundObject*)(&apRef->m_Extra)->GetLevCreaOriginalBase();
                                if (!pcompobj_4) {
                                    pcompobj_4 = apRef->GetObjectReference();
                                }
                                powner_3 = (TESForm*)pcompobj_4;
                            }
                            else {
                                powner_3 = apRef->GetOwner();
                            }
                            if ((abStealing || abowner) && !TESContainer::IsGold(pObj)) {
                                ExtraDataList* pextra_4;
                                ExtraDataList* v77 = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
                                if (v77) {
                                    pextra_4 = ::new (v77) ExtraDataList;
                                }
                                else {
                                    pextra_4 = 0;
                                }
                                v89 = pextra_4;
                                pextra_4->SetOwnership(powner_3);
                            }
                            int32_t v87 = pInventoryItem->iNumber;
                            ItemChange* v58 = v108 == 0 ? pInventoryItem : 0;
                            ExtraDataList* v30 = v89;
                            uint32_t v22 = pInventoryItem->iNumber;
                            RemoveItem(apRef, pObj, abStealing, v22, v30, 0, apStolenContainer, 0, 0, 1, 0, v58);
                            if (apStolenContainer == pPlayer) {
                                if (pObj) {
                                    if (!abSilent) {
                                        BSStringT<char> pstring_3;
                                        if (v87 <= 1) {
                                            const char* v60 = sAddItemtoInventory.String();
                                            pstring_3.SPrintF("%s %s", TESFullName::GetFullName(pObj), v60);
                                        }
                                        else {
                                            const char* v59 = sAddItemtoInventory.String();
                                            const char* timee = sPlural.String();
                                            const char* v37 = TESFullName::GetFullName(pObj);
                                            pstring_3.SPrintF("%i %s%s %s", v87, v37, timee, v59);
                                        }
                                        switch (pObj->GetFormType()) {
                                            case ARMO_ID: {
                                                const char* PickUpSoundName = pPlayer->GetPickUpSoundName(pObj, 1, 0);
                                                Interface::ShowMessage(
                                                    pstring_3.pString, 0,
                                                    "Interface\\Icons\\Message "
                                                    "Icons\\glow_message_vaultboy_neutral.dds",
                                                    PickUpSoundName, 2.0f, 0
                                                );
                                                break;
                                            }
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
                                                const char* v62 = pPlayer->GetPickUpSoundName(pObj, 1, 0);
                                                Interface::ShowMessage(
                                                    pstring_3.pString, 0,
                                                    "Interface\\Icons\\Message "
                                                    "Icons\\glow_message_vaultboy_neutral.dds",
                                                    v62, 2.0f, 0
                                                );
                                                break;
                                            }
                                            default: {
                                                break;
                                            }
                                        }
                                    }
                                }
                                Interface::UpdateInventory();
                            }
                            pListInv = pListofChanges;
                            continue;
                        }
                        while (1) {
                            bool bextrabad_2;
                            ExtraDataList* pextra_3;
                            while (1) {
                            LABEL_125:;
                                if (!plistextra_3 || (!plistextra_3->m_item || bflag_2)) {
                                LABEL_192:;
                                    if (inumberremove > 0) {
                                        RemoveItem(
                                            apRef, pObj, abStealing, inumberremove, 0, 0, apStolenContainer, 0, 0, 1, 0,
                                            v108 == 0 ? pInventoryItem : 0
                                        );
                                        pListInv = pListofChanges;
                                        if (apStolenContainer == pPlayer) {
                                            if (pObj) {
                                                if (!abSilent) {
                                                    BSStringT<char> v90;
                                                    if (inumberremove <= 1) {
                                                        const char* v57 = sAddItemtoInventory.String();
                                                        v90.SPrintF("%s %s", TESFullName::GetFullName(pObj), v57);
                                                    }
                                                    else {
                                                        const char* v56 = sAddItemtoInventory.String();
                                                        const char* timed = sPlural.String();
                                                        const char* v36 = TESFullName::GetFullName(pObj);
                                                        v90.SPrintF("%i %s%s %s", inumberremove, v36, timed, v56);
                                                    }
                                                    switch (pObj->GetFormType()) {
                                                        case ARMO_ID: {
                                                            const char* v64 = pPlayer->GetPickUpSoundName(pObj, 1, 0);
                                                            Interface::ShowMessage(
                                                                v90.pString, 0,
                                                                "Interface\\Icons\\Message "
                                                                "Icons\\glow_message_vaultboy_neutral.dds",
                                                                v64, 2.0f, 0
                                                            );
                                                            break;
                                                        }
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
                                                            const char* v65 = pPlayer->GetPickUpSoundName(pObj, 1, 0);
                                                            Interface::ShowMessage(
                                                                v90.pString, 0,
                                                                "Interface\\Icons\\Message "
                                                                "Icons\\glow_message_vaultboy_neutral.dds",
                                                                v65, 2.0f, 0
                                                            );
                                                            break;
                                                        }
                                                        default: {
                                                            break;
                                                        }
                                                    }
                                                }
                                            }
                                            Interface::UpdateInventory();
                                        }
                                    }
                                    goto LABEL_96;
                                }
                                pextra_3 = plistextra_3->m_item;
                                bextrabad_2 = 0;
                                if (pextra_3->GetCount() >= 1) {
                                    break;
                                }
                                plistextra_3 = plistextra_3->m_pkNext;
                                if (!plistextra_3) {
                                    RemoveItem(
                                        apRef, pObj, abStealing, pInventoryItem->iNumber, 0, 0, apStolenContainer, 0, 0,
                                        1, 0, pInventoryItem
                                    );
                                    v108 = 1;
                                    if (apStolenContainer == pPlayer) {
                                        if (pObj) {
                                            if (!abSilent) {
                                                BSStringT<char> pstring_2;
                                                if (inumberremove <= 1) {
                                                    const char* v52 = sAddItemtoInventory.String();
                                                    pstring_2.SPrintF("%s %s", TESFullName::GetFullName(pObj), v52);
                                                }
                                                else {
                                                    const char* v51 = sAddItemtoInventory.String();
                                                    const char* timeb = sPlural.String();
                                                    const char* v34 = TESFullName::GetFullName(pObj);
                                                    pstring_2.SPrintF(
                                                        "%i %s%s %s", pInventoryItem->iNumber, v34, timeb, v51
                                                    );
                                                }
                                                switch (pObj->GetFormType()) {
                                                    case ARMO_ID: {
                                                        const char* v68 = pPlayer->GetPickUpSoundName(pObj, 1, 0);
                                                        Interface::ShowMessage(
                                                            pstring_2.pString, 0,
                                                            "Interface\\Icons\\Message "
                                                            "Icons\\glow_message_vaultboy_neutral.dds",
                                                            v68, 2.0f, 0
                                                        );
                                                        break;
                                                    }
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
                                                        const char* v69 = pPlayer->GetPickUpSoundName(pObj, 1, 0);
                                                        Interface::ShowMessage(
                                                            pstring_2.pString, 0,
                                                            "Interface\\Icons\\Message "
                                                            "Icons\\glow_message_vaultboy_neutral.dds",
                                                            v69, 2.0f, 0
                                                        );
                                                        break;
                                                    }
                                                    default: {
                                                        break;
                                                    }
                                                }
                                            }
                                        }
                                        Interface::UpdateInventory();
                                    }
                                    inumberremove = 0;
                                    pListInv = pListofChanges;
                                    goto LABEL_192;
                                }
                            }
                            if (pextra_3->ItemsInList()) {
                                if (static_cast<uint32_t>(plistextra_3->ItemsInList()) > 1) {
                                    bextrabad_2 = 1;
                                }
                                else {
                                    bflag_2 = 1;
                                }
                                TESForm* powner_2 = 0;
                                if (apRef->IsActor()) {
                                    TESBoundObject* v93 = 0;
                                    TESBoundObject* pcompobj_3 =
                                        (TESBoundObject*)(&apRef->m_Extra)->GetLevCreaOriginalBase();
                                    if (!pcompobj_3) {
                                        pcompobj_3 = apRef->GetObjectReference();
                                    }
                                    powner_2 = (TESForm*)pcompobj_3;
                                }
                                else {
                                    powner_2 = apRef->GetOwner();
                                }
                                if ((abStealing || abowner) && !TESContainer::IsGold(pObj)) {
                                    if (!pextra_3->GetOwner()) {
                                        pextra_3->SetOwnership(powner_2);
                                    }
                                }
                                else {
                                    if (abRemoveOwnership) {
                                        int32_t inumb_2 = pextra_3->ItemsInList();
                                        if (pextra_3->GetOwner() &&
                                            ((pextra_3->GetCount() > 1 && inumb_2 <= 2) || inumb_2 <= 1)) {
                                            pextra_3->RemoveOwnership();
                                            pextra_3 = 0;
                                        }
                                        else {
                                            pextra_3->RemoveOwnership();
                                        }
                                    }
                                }
                                if (pextra_3) {
                                    int16_t v21 = pextra_3->GetCount();
                                    inumberremove -= v21;
                                    ItemChange* v53 = pInventoryItem;
                                    ExtraDataList* v29 = pextra_3;
                                    int32_t v26 = pextra_3->GetCount();
                                    RemoveItem(
                                        apRef, pObj, abStealing, v26, v29, 0, apStolenContainer, 0, 0, 1, 0, v53
                                    );
                                    v108 = 1;
                                    if (apStolenContainer == pPlayer) {
                                        if (pObj) {
                                            if (!abSilent) {
                                                BSStringT<char> v91;
                                                if (inumberremove <= 1) {
                                                    const char* v55 = sAddItemtoInventory.String();
                                                    v91.SPrintF("%s %s", TESFullName::GetFullName(pObj), v55);
                                                }
                                                else {
                                                    const char* v54 = sAddItemtoInventory.String();
                                                    const char* timec = sPlural.String();
                                                    const char* v35 = TESFullName::GetFullName(pObj);
                                                    v91.SPrintF("%i %s%s %s", pextra_3->GetCount(), v35, timec, v54);
                                                }
                                                switch (pObj->GetFormType()) {
                                                    case ARMO_ID: {
                                                        const char* v66 = pPlayer->GetPickUpSoundName(pObj, 1, 0);
                                                        Interface::ShowMessage(
                                                            v91.pString, 0,
                                                            "Interface\\Icons\\Message "
                                                            "Icons\\glow_message_vaultboy_neutral.dds",
                                                            v66, 2.0f, 0
                                                        );
                                                        break;
                                                    }
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
                                                        const char* v67 = pPlayer->GetPickUpSoundName(pObj, 1, 0);
                                                        Interface::ShowMessage(
                                                            v91.pString, 0,
                                                            "Interface\\Icons\\Message "
                                                            "Icons\\glow_message_vaultboy_neutral.dds",
                                                            v67, 2.0f, 0
                                                        );
                                                        break;
                                                    }
                                                    default: {
                                                        break;
                                                    }
                                                }
                                            }
                                        }
                                        Interface::UpdateInventory();
                                    }
                                }
                                pInventoryItem = GetObjectInList(pObj, 1, 0);
                                if (inumberremove > 0 || pInventoryItem) {
                                    if (inumberremove <= 0) {
                                        if (pInventoryItem) {
                                            pListInv = pListInv->m_pkNext;
                                        }
                                    }
                                }
                                else {
                                    pListInv = pListofChanges;
                                }
                                if (!pInventoryItem || inumberremove <= 0) {
                                    plistextra_3 = 0;
                                    goto LABEL_125;
                                }
                            }
                            else {
                                plistextra_3->RemoveHead();
                                if (plistextra_3->IsEmpty()) {
                                    plistextra_3->RemoveAll();
                                    if (plistextra_3) {
                                        delete plistextra_3;
                                    }
                                    plistextra_3 = 0;
                                    pInventoryItem->pExtraObjectList = 0;
                                    goto LABEL_125;
                                }
                            }
                            plistextra_3 = pInventoryItem->pExtraObjectList;
                        }
                    }
                }
            }
            pListInv = pListInv->m_pkNext;
        }
    }
    return fvalue;
}

void InventoryChanges::RemoveItemType(ENUM_FORM_ID aeType, bool abStealing, int32_t ainumber) {
    int32_t inum;
    fpreviousContainerWeight = fcontainerweight;
    fcontainerweight = -1.0f;
    inum = 0;
    while (ainumber > 0) {
        TESBoundObject* pobject = GetObjectbyType(aeType, inum);
        if (inum > ainumber) {
            inum = ainumber;
        }
        RemoveItem(0, pobject, abStealing, inum, 0, 0, 0, 0, 0, 1, 0, 0);
        ainumber -= inum;
    }
}

TESObjectREFR* InventoryChanges::RemoveItem(
    TESObjectREFR* apRef,
    TESBoundObject* apObject,
    bool abstealing,
    int32_t ainumber,
    ExtraDataList* apExtra,
    bool abDropWorld,
    TESObjectREFR* apOtherContainer,
    const NiPoint3* apPoint,
    const NiPoint3* apRotate,
    bool abdelete,
    bool abPreferStolen,
    ItemChange* apItemChange
) {
    ItemChange* pitem;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_INVENTORY, 1, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp", 3700
    );
    bcountdirty = 1;
    fpreviousContainerWeight = fcontainerweight;
    fcontainerweight = -1.0f;
    TESObjectREFR* pdroppedref = 0;
    if (pRef) {
        pRef->AddChange(32);
    }
    if (apOtherContainer) {
        apOtherContainer->AddChange(32);
    }
    TESContainer* porigContainer = GetOriginalContainer();
    if (apItemChange) {
        pitem = apItemChange;
    }
    else {
        pitem = GetObjectInList(apObject, 1, 0);
    }
    ItemChange* pitem_2 = pitem;
    ItemChange* pnewitem = 0;
    bool breload = 0;
    Actor* pact = 0;
    if (apRef->IsActor() && apRef == pPlayer) {
        pact = static_cast<Actor*>(apRef);
    }
    if (pact) {
        if (abDropWorld || apOtherContainer) {
            BaseProcess* CurrentAIProcess = pact->pCurrentProcess;
            if (CurrentAIProcess->GetCurrentAmmo()) {
                BaseProcess* v78 = pact->pCurrentProcess;
                if (v78->GetCurrentAmmo()->pContainerObj == apObject) {
                    breload = 1;
                }
            }
        }
    }
    if (abDropWorld || apOtherContainer) {
        if (apExtra) {
            if (apExtra->GetExtraData(74)) {
                if (ainumber == apExtra->GetCount()) {
                    apExtra->RemoveHotKey();
                }
            }
        }
    }
    if (apExtra) {
        if (!apExtra->ItemsInList()) {
            if (pitem_2) {
                if (pitem_2->pExtraObjectList) {
                    pitem_2->pExtraObjectList->Remove(apExtra);
                }
            }
            if (apExtra) {
                delete apExtra;
            }
            apExtra = 0;
        }
    }
    int32_t iorgCount = 0;
    if (apRef) {
        int32_t iorgCount_2;
        if (porigContainer) {
            iorgCount_2 = porigContainer->GetObjectCount(apObject);
        }
        else {
            iorgCount_2 = 0;
        }
        iorgCount = iorgCount_2;
        if (pitem_2) {
            if (!pitem_2->GetWorn(0)) {
                int32_t v13 = pitem_2->iNumber + iorgCount;
                int32_t ExtraTotalDefaultCount = pitem_2->GetExtraTotalDefaultCount();
                if (v13 > pitem_2->GetExtraTotalCount(0) + ExtraTotalDefaultCount) {
                    if (pitem_2->GetScript()) {
                        iorgCount = 0;
                    }
                }
            }
        }
    }
    int32_t inegativecount = 0;
    if (iorgCount < 0) {
        inegativecount = iorgCount;
        iorgCount = -iorgCount;
    }
    if (iorgCount < 0 && apOtherContainer != pPlayer) {
        return 0;
    }
    if ((pitem_2 && (pitem_2->iNumber && !(iorgCount + pitem_2->iNumber))) || (!pitem_2 && !iorgCount)) {
        TESObjectREFR* v104 = pdroppedref;
        return v104;
    }
    ExtraLeveledItem* plevelextra = 0;
    bool busebase = 0;
    ItemChange* pcleareditem = 0;
    while (pitem_2) {
        ItemChange* pnewitem_2;
        if (ainumber <= 0) {
            break;
        }
        int32_t FormHealth_0 = pitem_2->iNumber;
        if (iorgCount + FormHealth_0 <= 0) {
            break;
        }
        if (pnewitem) {
            delete pnewitem;
        }
        ItemChange* v103 = static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
        if (v103) {
            pnewitem_2 = ::new (v103) ItemChange;
        }
        else {
            pnewitem_2 = 0;
        }
        pnewitem = pnewitem_2;
        TESBoundObject* FormFlags = pitem_2->pContainerObj;
        pnewitem->pContainerObj = FormFlags;
        BSSimpleList<ExtraDataList*>* pkFirst = pitem_2->pExtraObjectList;
        bool bflag = 1;
        if (((apExtra && apExtra->ItemsInList()) || abPreferStolen) && (pkFirst && pkFirst->m_item)) {
            while (pkFirst && (pkFirst->m_item && bflag)) {
                ExtraDataList* pCurrent = pkFirst->m_item;
                ExtraDataList* pNewExtra = 0;
                if (apExtra == pCurrent || (!apExtra && (abPreferStolen && pCurrent->GetOwner()))) {
                    apExtra = 0;
                    bflag = 0;
                    int32_t icount = pCurrent->GetCount();
                    if (pCurrent->GetWorn(0) && pCurrent->GetCount() <= ainumber) {
                        bool bextraflag = 0;
                        if (pCurrent->ItemsInList() <= 1 ||
                            ((pCurrent->ItemsInList() == 2 && pCurrent->GetCount() > 1) ||
                             (pCurrent->HasLeveledItem() && pCurrent->ItemsInList() == 2))) {
                            bextraflag = 1;
                        }
                        bool abflag = 0;
                        if (!apRef->IsActor() || apRef->GetDisabled()) {
                            ExtraDataList* v59 = pCurrent;
                            int16_t Count = pCurrent->GetCount();
                            abflag = apRef->RemoveWornItem(pitem_2->pContainerObj, Count, v59);
                        }
                        else {
                            Actor* ptemp = static_cast<Actor*>(apRef);
                            if (ptemp->GetCurrentAIProcess()) {
                                ExtraDataList* v41 = pCurrent;
                                int32_t v38 = pCurrent->GetCount();
                                TESBoundObject* afTime = pitem_2->pContainerObj;
                                abflag = ptemp->UnEquipObject(afTime, v38, v41, 0, 0, 1);
                            }
                        }
                        if (!abflag || bextraflag) {
                            pCurrent = 0;
                        }
                        pitem_2 = GetObjectInList(apObject, 1, 0);
                        if (!pitem_2 || !pitem_2->GetWorn(0)) {
                            pdroppedref = RemoveItem(
                                apRef, apObject, abstealing, ainumber, pCurrent, abDropWorld, apOtherContainer, apPoint,
                                apRotate, 1, 0, 0
                            );
                        }
                        if (pitem_2) {
                            if (icount > 1 && icount - ainumber > 0 && pRef->IsActor()) {
                                static_cast<Actor*>(pRef)->Actor::EquipObject(
                                    pitem_2->pContainerObj, icount - ainumber, pCurrent, 1, 0, 0
                                );
                            }
                        }
                        if (pnewitem) {
                            delete pnewitem;
                        }
                        pnewitem = 0;
                        TESObjectREFR* v102 = pdroppedref;
                        return v102;
                    }
                    if (icount > ainumber || pCurrent->GetLeveledItem()) {
                        plevelextra = pCurrent->GetLeveledItem();
                        int32_t FormType = pitem_2->pContainerObj->GetFormType();
                        if (!pCurrent->GetLeveledItem() ||
                            (!pCurrent->GetScript() || (FormType == 24 || FormType == 26))) {
                            ExtraDataList* pNewExtra_2;
                            if (pCurrent->HasLeveledItem() && ainumber == icount) {
                                pitem_2->iNumber -= icount;
                                pCurrent->SetCount(-static_cast<int16_t>(icount));
                            }
                            else {
                                pitem_2->iNumber -= ainumber;
                                pCurrent->SetCount(icount - ainumber);
                            }
                            ExtraDataList* v100 = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
                            if (v100) {
                                pNewExtra_2 = ::new (v100) ExtraDataList;
                            }
                            else {
                                pNewExtra_2 = 0;
                            }
                            pNewExtra = pNewExtra_2;
                            pNewExtra_2->CopyListForReference(pCurrent, 0);
                            pNewExtra->RemoveLeveledItem();
                            pNewExtra->SetCount(ainumber);
                            if ((pNewExtra->ItemsInList() == 1 && pNewExtra->GetCount() > 1) ||
                                !pNewExtra->ItemsInList()) {
                                if (pNewExtra) {
                                    delete pNewExtra;
                                }
                                pNewExtra = 0;
                            }
                        }
                        else {
                            ExtraDataList* plevel;
                            if (FormType == 115) {
                                pCurrent->SetCount(1);
                            }
                            pNewExtra = pCurrent;
                            pitem_2->pExtraObjectList->Remove(pCurrent);
                            ExtraDataList* v101 = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
                            if (v101) {
                                plevel = ::new (v101) ExtraDataList;
                            }
                            else {
                                plevel = 0;
                            }
                            ExtraDataList* plevel_2 = plevel;
                            ExtraLeveledItem* plevelextra_2 = pCurrent->GetLeveledItem();
                            plevel_2->SetLevelItemExtra(plevelextra_2->iIndex);
                            pitem_2->pExtraObjectList->AddHead(plevel_2);
                            pNewExtra->RemoveLeveledItem();
                            int16_t v20 = pCurrent->GetCount();
                            pitem_2->iNumber = -v20;
                        }
                        Script::SetActionFlag(apRef, pNewExtra, SCRIPT_ACTION_ONDROP);
                        icount = ainumber;
                        ainumber = 0;
                    }
                    else {
                        pitem_2->iNumber -= icount;
                        uint32_t v21 = pnewitem->iNumber;
                        pnewitem->iNumber = icount + v21;
                        pNewExtra = pCurrent;
                        ainumber -= icount;
                        pitem_2->pExtraObjectList->Remove(pCurrent);
                        if (pCurrent) {
                            if (pCurrent->GetHotKey() >= 0 && pitem_2->iNumber > 0) {
                                ExtraDataList* apExtraa;
                                if (pitem_2->pExtraObjectList) {
                                    apExtraa = pitem_2->pExtraObjectList->m_item;
                                }
                                else {
                                    apExtraa = 0;
                                }
                                char HotKey = pCurrent->GetHotKey();
                                SetHotKeyItem(pitem_2, apExtraa, HotKey);
                            }
                        }
                        pCurrent = 0;
                        Script::SetActionFlag(apRef, pNewExtra, SCRIPT_ACTION_ONDROP);
                        if (pNewExtra->IsExtraListEmpty()) {
                            if (pNewExtra) {
                                delete pNewExtra;
                            }
                            pNewExtra = 0;
                        }
                        if (!pitem_2->iNumber) {
                            pcleareditem = pitem_2;
                        }
                    }
                    if (pNewExtra) {
                        if (pNewExtra->GetWorn(0)) {
                            pNewExtra->RemoveWorn(0);
                            if (pNewExtra->IsExtraListEmpty()) {
                                if (pNewExtra) {
                                    delete pNewExtra;
                                }
                                pNewExtra = 0;
                            }
                        }
                    }
                    if (pNewExtra) {
                        if (pNewExtra->ItemsInList()) {
                            if (!pnewitem->pExtraObjectList) {
                                BSSimpleList<ExtraDataList*>* v72;
                                BSSimpleList<ExtraDataList*>* v99 = static_cast<BSSimpleList<ExtraDataList*>*>(
                                    ourMalloc(sizeof(BSSimpleList<ExtraDataList*>))
                                );
                                if (v99) {
                                    v72 = ::new (v99) BSSimpleList<ExtraDataList*>;
                                }
                                else {
                                    v72 = 0;
                                }
                                pnewitem->pExtraObjectList = v72;
                            }
                            pnewitem->pExtraObjectList->AddHead(pNewExtra);
                        }
                    }
                    if (abDropWorld) {
                        if (pNewExtra && pNewExtra->GetReferencePointer()) {
                            if (pNewExtra) {
                                pdroppedref = pNewExtra->GetReferencePointer();
                            }
                            else {
                                if (pCurrent) {
                                    pdroppedref = pCurrent->GetReferencePointer();
                                }
                            }
                            if (pdroppedref) {
                                pdroppedref->SetDelete(0);
                                pNewExtra->RemoveReferencePointer();
                                bool abdelete_3 = pCurrent == 0;
                                float newscaleb = pNewExtra->GetScale();
                                pdroppedref->SetScale(newscaleb);
                                (&pdroppedref->m_Extra)->CopyListForReference(pNewExtra, abdelete_3);
                                (&pdroppedref->m_Extra)->RemoveScale();
                                (&pdroppedref->m_Extra)->RemoveReferencePointer();
                                if (!(&pdroppedref->m_Extra)->GetStartingWorldOrCell()) {
                                    (&pdroppedref->m_Extra)->SetStartingWorldOrCellForRef(pdroppedref);
                                    pdroppedref->AddChange(1024);
                                }
                                DropItemIntoWorld(
                                    apRef, pitem_2->pContainerObj, icount, pdroppedref, apPoint, apRotate
                                );
                                if (pnewitem) {
                                    delete pnewitem;
                                }
                                pnewitem = 0;
                                if (pNewExtra) {
                                    delete pNewExtra;
                                }
                                pNewExtra = 0;
                                bool baddOwnership_2 = 0;
                                TESObjectCELL* pcell_2 = apRef->GetParentCell();
                                if (!pdroppedref->GetMyOwner()) {
                                    if (pcell_2) {
                                        if (pcell_2->GetOwner()) {
                                            if (!pcell_2->IsActorCellOwner(static_cast<Actor*>(apRef))) {
                                                baddOwnership_2 = 1;
                                            }
                                        }
                                    }
                                }
                                if (!baddOwnership_2 && apRef->GetObjectReference()->GetFormType() == NPC__ID &&
                                    static_cast<TESActorBase*>(apRef->GetObjectReference())->TESActorBase::GetSex() ==
                                        TESActorBase::SEX_FEMALE) {
                                    TESBipedModelForm* pbipedModel_2 =
                                        dynamic_cast<TESBipedModelForm*>(pdroppedref->GetObjectReference());
                                    if (pbipedModel_2) {
                                        TESModelTextureSwap* WorldTESModel_0 =
                                            pbipedModel_2->GetWorldTESModel(TESActorBase::SEX_FEMALE);
                                        if (WorldTESModel_0 !=
                                            pbipedModel_2->GetWorldTESModel(TESActorBase::SEX_MALE)) {
                                            baddOwnership_2 = 1;
                                        }
                                    }
                                }
                                if (baddOwnership_2) {
                                    TESBoundObject* ObjectReference = apRef->GetObjectReference();
                                    pdroppedref->SetOwner(ObjectReference);
                                }
                            }
                        }
                        else {
                            pdroppedref =
                                DropItemIntoWorld(apRef, pitem_2->pContainerObj, icount, 0, apPoint, apRotate);
                            pdroppedref->AddChange(1024);
                            if (pNewExtra) {
                                if (pNewExtra->ItemsInList()) {
                                    bool abdelete_2 = pCurrent == 0;
                                    float newscale = pNewExtra->GetScale();
                                    pdroppedref->SetScale(newscale);
                                    (&pdroppedref->m_Extra)->CopyListForReference(pNewExtra, 1);
                                    (&pdroppedref->m_Extra)->RemoveScale();
                                }
                            }
                            bool baddOwnership = 0;
                            TESObjectCELL* pcell = apRef->GetParentCell();
                            if (!pdroppedref->GetMyOwner()) {
                                if (pcell) {
                                    if (pcell->GetOwner()) {
                                        if (!pcell->IsActorCellOwner(static_cast<Actor*>(apRef))) {
                                            baddOwnership = 1;
                                        }
                                    }
                                }
                            }
                            if (!baddOwnership && apRef->GetObjectReference()->GetFormType() == NPC__ID &&
                                static_cast<TESActorBase*>(apRef->GetObjectReference())->TESActorBase::GetSex() ==
                                    TESActorBase::SEX_FEMALE) {
                                TESBipedModelForm* pbipedModel =
                                    dynamic_cast<TESBipedModelForm*>(pdroppedref->GetObjectReference());
                                if (pbipedModel) {
                                    TESModelTextureSwap* v23 = pbipedModel->GetWorldTESModel(TESActorBase::SEX_FEMALE);
                                    if (v23 != pbipedModel->GetWorldTESModel(TESActorBase::SEX_MALE)) {
                                        baddOwnership = 1;
                                    }
                                }
                            }
                            if (baddOwnership) {
                                TESBoundObject* newscalea = apRef->GetObjectReference();
                                pdroppedref->SetOwner(newscalea);
                            }
                            if (pnewitem) {
                                delete pnewitem;
                            }
                            pnewitem = 0;
                            if (pNewExtra) {
                                delete pNewExtra;
                            }
                            pNewExtra = 0;
                        }
                    }
                    else {
                        if (apOtherContainer) {
                            ExtraDataList* pextra = 0;
                            if (pnewitem->pExtraObjectList) {
                                for (ExtraDataList*& rpItem : pnewitem->pExtraObjectList->ItemsUntilNull()) {
                                    TESForm* powner = 0;
                                    pextra = rpItem;
                                    if (!pextra->GetOwner()) {
                                        if (apRef->IsActor()) {
                                            TESBoundObject* v148 = 0;
                                            TESBoundObject* pcompobj =
                                                (TESBoundObject*)(&apRef->m_Extra)->GetLevCreaOriginalBase();
                                            if (!pcompobj) {
                                                pcompobj = apRef->GetObjectReference();
                                            }
                                            powner = (TESForm*)pcompobj;
                                        }
                                        else {
                                            powner = apRef->GetOwner();
                                        }
                                        if (abstealing && !TESContainer::IsGold(pnewitem->pContainerObj)) {
                                            pextra->SetOwnership(powner);
                                        }
                                    }
                                    int32_t afKarma = icount;
                                    ExtraDataList* newscalec = pextra;
                                    TESBoundObject* v42 = pnewitem->pContainerObj;
                                    apOtherContainer->AddObjecttoContainer(v42, newscalec, afKarma);
                                }
                                if (pnewitem) {
                                    delete pnewitem;
                                }
                                pnewitem = 0;
                            }
                            else {
                                TESForm* powner_2 = 0;
                                if (apRef->IsActor()) {
                                    TESBoundObject* v145 = 0;
                                    TESBoundObject* pcompobj_2 =
                                        (TESBoundObject*)(&apRef->m_Extra)->GetLevCreaOriginalBase();
                                    if (!pcompobj_2) {
                                        pcompobj_2 = apRef->GetObjectReference();
                                    }
                                    powner_2 = (TESForm*)pcompobj_2;
                                }
                                else {
                                    powner_2 = apRef->GetOwner();
                                }
                                if (abstealing && !TESContainer::IsGold(pnewitem->pContainerObj)) {
                                    ExtraDataList* pextra_2;
                                    ExtraDataList* v98 = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
                                    if (v98) {
                                        pextra_2 = ::new (v98) ExtraDataList;
                                    }
                                    else {
                                        pextra_2 = 0;
                                    }
                                    pextra = pextra_2;
                                    pextra_2->SetOwnership(powner_2);
                                    pextra->SetCount(ainumber);
                                }
                                int32_t afKarmaa = icount;
                                ExtraDataList* newscaled = pextra;
                                TESBoundObject* v43 = pnewitem->pContainerObj;
                                apOtherContainer->AddObjecttoContainer(v43, newscaled, afKarmaa);
                                if (pnewitem) {
                                    delete pnewitem;
                                }
                                pnewitem = 0;
                            }
                            bflag = 0;
                        }
                        else {
                            if (!Interface::ContainerIsBarter() && !Interface::ContainerIsRecipe() &&
                                !Interface::IsMenuIDVisible(Interface::ePipBoyRepair, 0) &&
                                !Interface::IsMenuIDVisible(Interface::eRepairServicesMenu, 0)) {
                                pnewitem->DeleteAllExtra();
                            }
                            if (pnewitem) {
                                delete pnewitem;
                            }
                            pnewitem = 0;
                        }
                    }
                }
                else {
                    pkFirst = pkFirst->m_pkNext;
                }
                if (ainumber > 0 && abPreferStolen) {
                    TESObjectREFR* v97 = RemoveItem(
                        apRef, apObject, abstealing, ainumber, 0, abDropWorld, apOtherContainer, apPoint, apRotate, 1,
                        0, 0
                    );
                    return v97;
                }
                if (ainumber > 0 && pdroppedref) {
                    TESObjectREFR* v96 = pdroppedref;
                    return v96;
                }
                if (ainumber > 0 && !pkFirst) {
                    apExtra = 0;
                    break;
                }
            }
        }
        else {
            int32_t icount_2 = 0;
            if (iorgCount >= 0) {
                icount_2 = pitem_2->iNumber + iorgCount;
            }
            else {
                icount_2 = pitem_2->iNumber - iorgCount;
            }
            if (icount_2 < 0 && iorgCount >= 0) {
                TESObjectREFR* v95 = pdroppedref;
                return v95;
            }
            int32_t iextracount = 0;
            if (pitem_2->pExtraObjectList) {
                int32_t ExtraTotalCount = pitem_2->GetExtraTotalCount(0);
                iextracount = pitem_2->GetWorn(0) + ExtraTotalCount;
            }
            int32_t idefaultcount = icount_2 - iextracount;
            if (icount_2 - iextracount < 0) {
                idefaultcount = 0;
            }
            ExtraDataList* pCurrent_2 = 0;
            ExtraDataList* pNewExtra_3 = 0;
            BSSimpleList<ExtraDataList*>* pkFirst_2 = pitem_2->pExtraObjectList;
            if (idefaultcount > 0) {
                if (pitem_2->GetWorn(0)) {
                    ExtraDataList* pCurrent_3 = 0;
                    int32_t pNewExtra_4 = 0;
                    for (BSSimpleList<ExtraDataList*>* pkFirst_3 = pitem_2->pExtraObjectList;
                         pkFirst_3 && pkFirst_3->m_item; pkFirst_3 = pkFirst_3->GetNext()) {
                        pCurrent_3 = pkFirst_3->m_item;
                        int16_t scount = pCurrent_3->GetCount();
                        if (pCurrent_3->GetWorn(0) && scount > 1) {
                            pCurrent_3->SetCount(scount - ainumber);
                        }
                    }
                }
                else {
                    if (pitem_2->HasLeveledExtra()) {
                        ExtraDataList* pCurrent_4 = 0;
                        int32_t pNewExtra_5 = 0;
                        for (BSSimpleList<ExtraDataList*>* pkFirst_4 = pitem_2->pExtraObjectList;
                             pkFirst_4 && pkFirst_4->m_item; pkFirst_4 = pkFirst_4->GetNext()) {
                            pCurrent_4 = pkFirst_4->m_item;
                            int16_t scount_2 = pCurrent_4->GetCount();
                            if (pCurrent_4->HasLeveledItem() && scount_2 > 1) {
                                pCurrent_4->SetCount(scount_2 - ainumber);
                            }
                        }
                    }
                }
                if (idefaultcount < ainumber) {
                    if (porigContainer) {
                        if (porigContainer->GetObjectCount(apObject) > 0) {
                            pitem_2->iNumber -= idefaultcount;
                        }
                    }
                    icount_2 = idefaultcount;
                    ainumber -= idefaultcount;
                    if (iorgCount + pitem_2->iNumber >= 0) {
                        pitem_2->iNumber -= icount_2;
                    }
                    else {
                        if (iorgCount) {
                            pitem_2->iNumber = -iorgCount;
                        }
                        else {
                            pitem_2->iNumber = 0;
                        }
                    }
                }
                else {
                    if (!inegativecount) {
                        pitem_2->iNumber -= ainumber;
                    }
                    icount_2 = ainumber;
                    ainumber = 0;
                }
                if (abDropWorld) {
                    pdroppedref = DropItemIntoWorld(apRef, apObject, icount_2, 0, apPoint, apRotate);
                    TESObjectCELL* pcell_3 = apRef->GetParentCell();
                    bool baddOwnership_3 = 0;
                    if (!pdroppedref->GetMyOwner()) {
                        if (pcell_3) {
                            if (pcell_3->GetOwner()) {
                                if (!pcell_3->IsActorCellOwner(static_cast<Actor*>(apRef))) {
                                    baddOwnership_3 = 1;
                                }
                            }
                        }
                    }
                    if (!baddOwnership_3 && apRef->GetObjectReference()->GetFormType() == NPC__ID &&
                        static_cast<TESActorBase*>(apRef->GetObjectReference())->TESActorBase::GetSex() ==
                            TESActorBase::SEX_FEMALE) {
                        TESBipedModelForm* pbipedModel_3 =
                            dynamic_cast<TESBipedModelForm*>(pdroppedref->GetObjectReference());
                        if (pbipedModel_3) {
                            TESModelTextureSwap* v26 = pbipedModel_3->GetWorldTESModel(TESActorBase::SEX_FEMALE);
                            if (v26 != pbipedModel_3->GetWorldTESModel(TESActorBase::SEX_MALE)) {
                                baddOwnership_3 = 1;
                            }
                        }
                    }
                    if (baddOwnership_3) {
                        TESBoundObject* v60 = apRef->GetObjectReference();
                        pdroppedref->SetOwner(v60);
                    }
                    if (pnewitem) {
                        delete pnewitem;
                    }
                    pnewitem = 0;
                    if (pNewExtra_3) {
                        delete pNewExtra_3;
                    }
                    pNewExtra_3 = 0;
                }
                else {
                    if (apOtherContainer) {
                        TESForm* powner_3 = 0;
                        if (apRef->IsActor()) {
                            TESBoundObject* v125 = 0;
                            TESBoundObject* pcompobj_3 = (TESBoundObject*)(&apRef->m_Extra)->GetLevCreaOriginalBase();
                            if (!pcompobj_3) {
                                pcompobj_3 = apRef->GetObjectReference();
                            }
                            powner_3 = (TESForm*)pcompobj_3;
                        }
                        else {
                            powner_3 = apRef->GetOwner();
                        }
                        if (abstealing && !TESContainer::IsGold(apObject) &&
                            (apObject->GetFormType() != AMMO_ID || apOtherContainer != pPlayer)) {
                            if (!apExtra) {
                                ExtraDataList* v70;
                                ExtraDataList* v94 = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
                                if (v94) {
                                    v70 = ::new (v94) ExtraDataList;
                                }
                                else {
                                    v70 = 0;
                                }
                                apExtra = v70;
                            }
                            apExtra->SetOwnership(powner_3);
                            apExtra->SetCount(icount_2);
                        }
                        if (apExtra) {
                            if (apOtherContainer == pPlayer && apObject->GetFormType() == AMMO_ID) {
                                apExtra->RemoveOwnership();
                            }
                        }
                        apOtherContainer->AddObjecttoContainer(apObject, apExtra, icount_2);
                        if (pnewitem) {
                            delete pnewitem;
                        }
                        pnewitem = 0;
                    }
                }
                if (pnewitem) {
                    delete pnewitem;
                }
                pnewitem = 0;
            }
            if (ainumber > 0 && (pitem_2 && (pitem_2->pExtraObjectList && !pitem_2->pExtraObjectList->IsEmpty()))) {
                pkFirst_2 = pitem_2->pExtraObjectList;
                while (pkFirst_2 && (pkFirst_2->m_item && ainumber > 0)) {
                    ItemChange* pnewitem_3;
                    bool bextraflag_2 = 1;
                    pCurrent_2 = pkFirst_2->m_item;
                    if (pnewitem) {
                        delete pnewitem;
                    }
                    ItemChange* v93 = static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
                    if (v93) {
                        pnewitem_3 = ::new (v93) ItemChange;
                    }
                    else {
                        pnewitem_3 = 0;
                    }
                    pnewitem = pnewitem_3;
                    if (pitem_2) {
                        TESBoundObject* v27 = pitem_2->pContainerObj;
                        pnewitem->pContainerObj = v27;
                    }
                    if (pCurrent_2->GetCount() >= 0) {
                        if (pCurrent_2->GetWorn(0) && pCurrent_2->GetCount() <= 1) {
                            bool abflag_2 = 0;
                            if (pCurrent_2->ItemsInList() <= 1 ||
                                (pCurrent_2->ItemsInList() == 2 && pCurrent_2->GetCount() > 1)) {
                                apExtra = 0;
                                abflag_2 = 1;
                            }
                            ExtraDataList* afKarmab = pCurrent_2;
                            int16_t v28 = pCurrent_2->GetCount();
                            if (apRef->RemoveWornItem(pitem_2->pContainerObj, v28, afKarmab)) {
                                pdroppedref = RemoveItem(
                                    apRef, apObject, abstealing, ainumber, apExtra, abDropWorld, apOtherContainer,
                                    apPoint, apRotate, 1, 0, 0
                                );
                            }
                            else {
                                const char* v29 = sCantRemoveWornItem.operator const char*();
                                Interface::ShowMessage(
                                    v29, 0, "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds", 0, 2.0f, 0
                                );
                            }
                            if (pnewitem) {
                                delete pnewitem;
                            }
                            pnewitem = 0;
                            TESObjectREFR* v92 = pdroppedref;
                            return v92;
                        }
                        icount_2 = pCurrent_2->GetCount();
                        if (icount_2 > ainumber || pCurrent_2->GetLeveledItem()) {
                            ExtraDataList* pNewExtra_6;
                            plevelextra = pCurrent_2->GetLeveledItem();
                            if (plevelextra && (ainumber == icount_2 || ainumber > icount_2)) {
                                pitem_2->iNumber -= icount_2;
                                pCurrent_2->SetCount(-static_cast<int16_t>(icount_2));
                            }
                            else {
                                pitem_2->iNumber -= ainumber;
                                pCurrent_2->SetCount(icount_2 - ainumber);
                            }
                            ExtraDataList* v91 = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
                            if (v91) {
                                pNewExtra_6 = ::new (v91) ExtraDataList;
                            }
                            else {
                                pNewExtra_6 = 0;
                            }
                            pNewExtra_3 = pNewExtra_6;
                            pNewExtra_6->CopyListForReference(pCurrent_2, 0);
                            pNewExtra_3->RemoveLeveledItem();
                            if (icount_2 >= ainumber) {
                                pNewExtra_3->SetCount(ainumber);
                            }
                            else {
                                pNewExtra_3->SetCount(icount_2);
                            }
                            if (ainumber < icount_2) {
                                icount_2 = ainumber;
                            }
                            ainumber -= icount_2;
                            if (pNewExtra_3->IsExtraListEmpty()) {
                                if (pNewExtra_3) {
                                    delete pNewExtra_3;
                                }
                                pNewExtra_3 = 0;
                                goto LABEL_404;
                            }
                            if (!pnewitem->pExtraObjectList) {
                                BSSimpleList<ExtraDataList*>* v67;
                                BSSimpleList<ExtraDataList*>* v90 = static_cast<BSSimpleList<ExtraDataList*>*>(
                                    ourMalloc(sizeof(BSSimpleList<ExtraDataList*>))
                                );
                                if (v90) {
                                    v67 = ::new (v90) BSSimpleList<ExtraDataList*>;
                                }
                                else {
                                    v67 = 0;
                                }
                                pnewitem->pExtraObjectList = v67;
                            }
                        }
                        else {
                            pitem_2->iNumber -= icount_2;
                            uint32_t v30 = pnewitem->iNumber;
                            pnewitem->iNumber = icount_2 + v30;
                            pNewExtra_3 = pCurrent_2;
                            ainumber -= icount_2;
                            pitem_2->pExtraObjectList->Remove(pNewExtra_3);
                            pCurrent_2 = 0;
                            if (pNewExtra_3->IsExtraListEmpty()) {
                                if (pNewExtra_3) {
                                    delete pNewExtra_3;
                                }
                                pNewExtra_3 = 0;
                                goto LABEL_404;
                            }
                            if (!pnewitem->pExtraObjectList) {
                                BSSimpleList<ExtraDataList*>* v66;
                                BSSimpleList<ExtraDataList*>* v89 = static_cast<BSSimpleList<ExtraDataList*>*>(
                                    ourMalloc(sizeof(BSSimpleList<ExtraDataList*>))
                                );
                                if (v89) {
                                    v66 = ::new (v89) BSSimpleList<ExtraDataList*>;
                                }
                                else {
                                    v66 = 0;
                                }
                                pnewitem->pExtraObjectList = v66;
                            }
                        }
                        pnewitem->pExtraObjectList->AddHead(pNewExtra_3);
                    }
                    else {
                        bextraflag_2 = 0;
                        pkFirst_2 = pkFirst_2->GetNext();
                    }
                LABEL_404:;
                    if (pitem_2 && bextraflag_2) {
                        if (abDropWorld) {
                            if (pNewExtra_3 && pNewExtra_3->GetReferencePointer()) {
                                if (pNewExtra_3) {
                                    pdroppedref = pNewExtra_3->GetReferencePointer();
                                }
                                else {
                                    if (pCurrent_2) {
                                        pdroppedref = pCurrent_2->GetReferencePointer();
                                    }
                                }
                                if (pdroppedref) {
                                    pdroppedref->SetDelete(0);
                                    pNewExtra_3->RemoveReferencePointer();
                                    bool abdelete_4 = pCurrent_2 == 0;
                                    float newscalee = pNewExtra_3->GetScale();
                                    pdroppedref->SetScale(newscalee);
                                    (&pdroppedref->m_Extra)->CopyListForReference(pNewExtra_3, abdelete_4);
                                    (&pdroppedref->m_Extra)->RemoveScale();
                                    (&pdroppedref->m_Extra)->RemoveReferencePointer();
                                    if (!(&pdroppedref->m_Extra)->GetStartingWorldOrCell()) {
                                        (&pdroppedref->m_Extra)->SetStartingWorldOrCellForRef(pdroppedref);
                                        pdroppedref->AddChange(1024);
                                    }
                                    pdroppedref = DropItemIntoWorld(
                                        apRef, pitem_2->pContainerObj, icount_2, pdroppedref, apPoint, apRotate
                                    );
                                    if (pnewitem) {
                                        delete pnewitem;
                                    }
                                    pnewitem = 0;
                                    if (pNewExtra_3) {
                                        delete pNewExtra_3;
                                    }
                                    pNewExtra_3 = 0;
                                    bool baddOwnership_5 = 0;
                                    TESObjectCELL* pcell_5 = apRef->GetParentCell();
                                    if (!pdroppedref->GetMyOwner()) {
                                        if (pcell_5) {
                                            if (pcell_5->GetOwner()) {
                                                if (!pcell_5->IsActorCellOwner(static_cast<Actor*>(apRef))) {
                                                    baddOwnership_5 = 1;
                                                }
                                            }
                                        }
                                    }
                                    if (!baddOwnership_5 && apRef->GetObjectReference()->GetFormType() == NPC__ID &&
                                        static_cast<TESActorBase*>(apRef->GetObjectReference())
                                                ->TESActorBase::GetSex() == TESActorBase::SEX_FEMALE) {
                                        TESBipedModelForm* pbipedModel_5 =
                                            dynamic_cast<TESBipedModelForm*>(pdroppedref->GetObjectReference());
                                        if (pbipedModel_5) {
                                            TESModelTextureSwap* v32 =
                                                pbipedModel_5->GetWorldTESModel(TESActorBase::SEX_FEMALE);
                                            if (v32 != pbipedModel_5->GetWorldTESModel(TESActorBase::SEX_MALE)) {
                                                baddOwnership_5 = 1;
                                            }
                                        }
                                    }
                                    if (baddOwnership_5) {
                                        TESBoundObject* v40 = apRef->GetObjectReference();
                                        pdroppedref->SetOwner(v40);
                                    }
                                }
                            }
                            else {
                                pdroppedref =
                                    DropItemIntoWorld(apRef, pitem_2->pContainerObj, icount_2, 0, apPoint, apRotate);
                                TESObjectCELL* pcell_4 = apRef->GetParentCell();
                                bool baddOwnership_4 = 0;
                                if (!pdroppedref->GetMyOwner()) {
                                    if (pcell_4) {
                                        if (pcell_4->GetOwner()) {
                                            if (!pcell_4->IsActorCellOwner(static_cast<Actor*>(apRef))) {
                                                baddOwnership_4 = 1;
                                            }
                                        }
                                    }
                                }
                                if (!baddOwnership_4 && apRef->GetObjectReference()->GetFormType() == NPC__ID &&
                                    static_cast<TESActorBase*>(apRef->GetObjectReference())->TESActorBase::GetSex() ==
                                        TESActorBase::SEX_FEMALE) {
                                    TESBipedModelForm* pbipedModel_4 =
                                        dynamic_cast<TESBipedModelForm*>(pdroppedref->GetObjectReference());
                                    if (pbipedModel_4) {
                                        TESModelTextureSwap* v31 =
                                            pbipedModel_4->GetWorldTESModel(TESActorBase::SEX_FEMALE);
                                        if (v31 != pbipedModel_4->GetWorldTESModel(TESActorBase::SEX_MALE)) {
                                            baddOwnership_4 = 1;
                                        }
                                    }
                                }
                                if (baddOwnership_4) {
                                    TESBoundObject* afKarmac = apRef->GetObjectReference();
                                    pdroppedref->SetOwner(afKarmac);
                                }
                                if (pNewExtra_3) {
                                    if (pNewExtra_3->ItemsInList()) {
                                        float afKarmad = pNewExtra_3->GetScale();
                                        pdroppedref->SetScale(afKarmad);
                                        (&pdroppedref->m_Extra)->CopyListForReference(pNewExtra_3, 1);
                                        (&pdroppedref->m_Extra)->RemoveScale();
                                    }
                                }
                                if (pnewitem) {
                                    delete pnewitem;
                                }
                                pnewitem = 0;
                                if (pNewExtra_3) {
                                    delete pNewExtra_3;
                                }
                                pNewExtra_3 = 0;
                            }
                            if (pnewitem) {
                                delete pnewitem;
                            }
                            pnewitem = 0;
                        }
                        else {
                            if (apOtherContainer) {
                                TESForm* powner_4;
                                ExtraDataList* pextra_3 = 0;
                                if (pnewitem->pExtraObjectList) {
                                    pextra_3 = pnewitem->pExtraObjectList->m_item;
                                }
                                else {
                                    BSSimpleList<ExtraDataList*>* v64;
                                    ExtraDataList* pextra_4;
                                    ExtraDataList* v88 = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
                                    if (v88) {
                                        pextra_4 = ::new (v88) ExtraDataList;
                                    }
                                    else {
                                        pextra_4 = 0;
                                    }
                                    pextra_3 = pextra_4;
                                    BSSimpleList<ExtraDataList*>* v87 = static_cast<BSSimpleList<ExtraDataList*>*>(
                                        ourMalloc(sizeof(BSSimpleList<ExtraDataList*>))
                                    );
                                    if (v87) {
                                        v64 = ::new (v87) BSSimpleList<ExtraDataList*>;
                                    }
                                    else {
                                        v64 = 0;
                                    }
                                    pnewitem->pExtraObjectList = v64;
                                    pnewitem->pExtraObjectList->AddHead(pextra_3);
                                }
                                if (apRef->IsActor()) {
                                    TESBoundObject* pcompobj_4 =
                                        (TESBoundObject*)(&apRef->m_Extra)->GetLevCreaOriginalBase();
                                    if (!pcompobj_4) {
                                        pcompobj_4 = apRef->GetObjectReference();
                                    }
                                    powner_4 = (TESForm*)pcompobj_4;
                                }
                                else {
                                    powner_4 = apRef->GetOwner();
                                }
                                if (!abstealing || TESContainer::IsGold(pnewitem->pContainerObj)) {
                                    pextra_3->RemoveOwnership();
                                }
                                else {
                                    pextra_3->SetOwnership(powner_4);
                                    pextra_3->SetCount(icount_2);
                                }
                                int32_t afKarmae = icount_2;
                                ExtraDataList* newscalef = pextra_3;
                                TESBoundObject* v44 = pnewitem->pContainerObj;
                                apOtherContainer->AddObjecttoContainer(v44, newscalef, afKarmae);
                                if (pnewitem) {
                                    delete pnewitem;
                                }
                                pnewitem = 0;
                            }
                            else {
                                if (abdelete) {
                                    pnewitem->DeleteAllExtra();
                                }
                                if (pnewitem) {
                                    delete pnewitem;
                                }
                                pnewitem = 0;
                            }
                        }
                        if (pCurrent_2) {
                            pkFirst_2 = pkFirst_2->GetNext();
                        }
                        if (ainumber) {
                            busebase = 1;
                        }
                    }
                    else {
                        if (pnewitem) {
                            delete pnewitem;
                        }
                        pnewitem = 0;
                        if (pNewExtra_3) {
                            delete pNewExtra_3;
                        }
                        pNewExtra_3 = 0;
                    }
                    continue;
                }
            }
        }
        pitem_2 = GetObjectInList(apObject, 1, 0);
    }
    if (abPreferStolen && ainumber > 0) {
        TESObjectREFR* v86 = RemoveItem(
            apRef, apObject, abstealing, ainumber, apExtra, abDropWorld, apOtherContainer, apPoint, apRotate, 1, 0, 0
        );
        return v86;
    }
    if ((!pitem_2 || busebase) && iorgCount) {
        if (pitem_2) {
            iorgCount += pitem_2->iNumber;
        }
        if (!iorgCount) {
            pitem_2->pExtraObjectList->RemoveAll();
            TESObjectREFR* v85 = pdroppedref;
            return v85;
        }
        if (abDropWorld) {
            pdroppedref = DropItemIntoWorld(apRef, apObject, ainumber, 0, apPoint, apRotate);
            TESObjectCELL* pcell_6 = apRef->GetParentCell();
            bool baddOwnership_6 = 0;
            if (!pdroppedref->GetMyOwner()) {
                if (pcell_6) {
                    if (pcell_6->GetOwner()) {
                        baddOwnership_6 = !pcell_6->IsActorCellOwner(static_cast<Actor*>(apRef));
                    }
                }
            }
            if (!baddOwnership_6 && apRef->GetObjectReference()->GetFormType() == NPC__ID &&
                static_cast<TESActorBase*>(apRef->GetObjectReference())->TESActorBase::GetSex() ==
                    TESActorBase::SEX_FEMALE) {
                TESBipedModelForm* pbipedModel_6 = dynamic_cast<TESBipedModelForm*>(pdroppedref->GetObjectReference());
                if (pbipedModel_6) {
                    TESModelTextureSwap* v33 = pbipedModel_6->GetWorldTESModel(TESActorBase::SEX_FEMALE);
                    baddOwnership_6 = v33 != pbipedModel_6->GetWorldTESModel(TESActorBase::SEX_MALE);
                }
            }
            if (baddOwnership_6) {
                TESBoundObject* v61 = apRef->GetObjectReference();
                pdroppedref->SetOwner(v61);
            }
        }
        else {
            if (apOtherContainer) {
                TESForm* powner_5;
                if (apRef->IsActor()) {
                    TESBoundObject* pcompobj_5 = (TESBoundObject*)(&apRef->m_Extra)->GetLevCreaOriginalBase();
                    if (!pcompobj_5) {
                        pcompobj_5 = apRef->GetObjectReference();
                    }
                    powner_5 = (TESForm*)pcompobj_5;
                }
                else {
                    powner_5 = apRef->GetOwner();
                }
                if (!abstealing || TESContainer::IsGold(apObject)) {
                    if (apExtra) {
                        apExtra->RemoveOwnership();
                    }
                }
                else {
                    if (!apExtra) {
                        ExtraDataList* v63;
                        ExtraDataList* v84 = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
                        if (v84) {
                            v63 = ::new (v84) ExtraDataList;
                        }
                        else {
                            v63 = 0;
                        }
                        apExtra = v63;
                    }
                    apExtra->SetOwnership(powner_5);
                    apExtra->SetCount(ainumber);
                }
                if (ainumber > 0) {
                    if (apObject->GetFormType() == NOTE_ID && apOtherContainer == pPlayer) {
                        bool v106 = 0;
                        if (abstealing) {
                            if (powner_5) {
                                if (powner_5->GetFormType() == FACT_ID) {
                                    v106 = !static_cast<TESFaction*>(powner_5)->TESFaction::GetIsEvil();
                                }
                                else {
                                    if (powner_5) {
                                        float afKarmaf =
                                            static_cast<ActorValueOwner*>(static_cast<TESActorBase*>(powner_5))
                                                ->GetActorFloatValue(ActorValue::KARMA);
                                        TESActorBaseData::ALIGNMENT ealign =
                                            TESActorBaseData::GetAlignmentForKarma(afKarmaf);
                                        if (ealign != TESActorBaseData::ALIGNMENT_EVIL) {
                                            v106 = ealign != TESActorBaseData::ALIGNMENT_VERY_EVIL;
                                        }
                                    }
                                }
                            }
                        }
                        if (v106) {
                            double v34 = fKarmaModStealing.Float();
                            pPlayer->RewardKarma(static_cast<int32_t>(v34));
                        }
                        pPlayer->AddNote(static_cast<BGSNote*>(apObject), 1);
                    }
                    else {
                        apOtherContainer->AddObjecttoContainer(apObject, apExtra, ainumber);
                    }
                }
            }
        }
        if (pnewitem) {
            delete pnewitem;
        }
        pnewitem = 0;
        if (pitem_2) {
            pitem_2->iNumber -= ainumber;
        }
        else {
            ItemChange* pitem_3;
            ItemChange* v83 = static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
            if (v83) {
                pitem_3 = ::new (v83) ItemChange(apObject, -ainumber);
            }
            else {
                pitem_3 = 0;
            }
            pitem_2 = pitem_3;
            pListofChanges->AddTail(pitem_2);
        }
    }
    pitem_2 = GetObjectInList(apObject, 1, 0);
    if (!pitem_2 ||
        ((plevelextra && !apRef->IsActor()) ||
         ((pitem_2->iNumber || iorgCount) &&
          (!pitem_2->pExtraObjectList || (!pitem_2->pExtraObjectList->IsEmpty() || pitem_2->iNumber))))) {
        if (pitem_2) {
            int32_t v35 = pitem_2->iNumber;
            if (iorgCount + v35 < 0 && !pitem_2->GetWorn(0)) {
                if (pitem_2->pExtraObjectList) {
                    if (!plevelextra) {
                        pitem_2->pExtraObjectList->RemoveAll();
                    }
                }
            }
        }
    }
    else {
        if (!Interface::ContainerIsBarter() && !Interface::ContainerIsRecipe() &&
            !Interface::IsMenuIDVisible(Interface::ePipBoyRepair, 0) &&
            !Interface::IsMenuIDVisible(Interface::ePipBoyRepair, 0)) {
            pitem_2->DeleteAllExtra();
        }
        pListofChanges->Remove(pitem_2);
        if (pitem_2) {
            delete pitem_2;
        }
    }
    if (breload) {
        if (pact->GetCurrentWeapon()) {
            TESObjectWEAP* CurrentWeapon = pact->GetCurrentWeapon();
            pact->ReloadWeaponNV(CurrentWeapon, 0, 0, 0);
        }
    }
    return pdroppedref;
}

TESContainer* InventoryChanges::GetOriginalContainer() {
    if (!pRef) {
        return nullptr;
    }
    return pRef->HasContainer();
}

void InventoryChanges::ResetCachedWeights() {
    fpreviousContainerWeight = fcontainerweight;
    fcontainerweight = -1.0f;
    if (pRef && pRef->IsActor()) {
        Actor* pActor = static_cast<Actor*>(pRef);
        pActor->SetEquippedWeight(-1.0f);
    }
}

void InventoryChanges::ForceWeightRecalculation() {
    fcontainerweight = (-0x1p+0f);
}

bool InventoryChanges::IsLevelItemInited(int32_t aiIndex) {
    bool bFound = false;
    for (BSSimpleList<ItemChange*>* pNode = pListofChanges; pNode && !pNode->IsEmpty() && pNode->GetItem() && !bFound;
         pNode = pNode->GetNext()) {
        ItemChange* pChange = pNode->GetItem();
        if (!pChange) {
            continue;
        }
        for (BSSimpleList<ExtraDataList*>* pExtraNode = pChange->pExtraObjectList; pExtraNode && pExtraNode->GetItem();
             pExtraNode = pExtraNode->GetNext()) {
            ExtraDataList* pExtra = pExtraNode->GetItem();
            if (pExtra && pExtra->GetExtraLevelItemIndex() == aiIndex) {
                bFound = true;
            }
        }
    }
    return bFound;
}

// The leveled items are numbered in base container order; each one not rolled yet is rolled into a temporary container
// for the reference's level (an actor's own level, else the reference's calculated level), takes the entry's health
// when it is not 1, and is added to the changes under its number.
void InventoryChanges::InitLeveledItems() {
    BSSimpleList<ContainerObject*>* pNode = GetOriginalContainer()->GetObjectList();
    int32_t iLevItemIndex = 0;
    int32_t iLevel = 0;
    if (GetRef()->IsActor()) {
        iLevel = static_cast<Actor*>(GetRef())->GetLevel();
    }
    else {
        iLevel = GetRef()->GetCalcLevel(true);
    }
    for (; pNode && !pNode->IsEmpty(); pNode = pNode->GetNext()) {
        ContainerObject* pObject = pNode->GetItem();
        TESLevItem* pLevItem = nullptr;
        if (pObject->pObj && pObject->pObj->GetFormType() == LVLI_ID) {
            pLevItem = static_cast<TESLevItem*>(pObject->pObj);
        }
        if (!pLevItem) {
            continue;
        }
        if (!IsLevelItemInited(iLevItemIndex)) {
            alignas(TESContainer) uint8_t aContainerStorage[sizeof(TESContainer)];
            TESContainer* pContainer = reinterpret_cast<TESContainer*>(aContainerStorage);
            ::new (aContainerStorage) TESContainer;
            pLevItem->CalculateCurrentFormList(
                static_cast<uint16_t>(iLevel), std::abs(pObject->i_count), pContainer, 0
            );
            if (pObject && pObject->pItemExtra && pObject->pItemExtra->fHealthMult != 1.0) {
                pContainer->ApplyHealthPercentToContainer(pObject->pItemExtra->fHealthMult);
            }
            pContainer->AddLevItemContainerToChanges(iLevItemIndex, this);
            pContainer->~TESContainer();
        }
        ++iLevItemIndex;
    }
}

// Each base container entry with per-entry extra data (leveled items excepted) becomes a new change of 0 with
// one instance holding the entry's count, owner and health.
void InventoryChanges::InitFromContainerExtra() {
    BSSimpleList<ContainerObject*>* pNode = GetOriginalContainer()->GetObjectList();
    while (pNode && !pNode->IsEmpty()) {
        ContainerObject* pObject = pNode->GetItem();
        pNode = pNode->GetNext();
        if (!pObject->pObj || pObject->pObj->GetFormType() == LVLI_ID || !pObject->pItemExtra) {
            continue;
        }
        void* pExtraMemory = ourMalloc(sizeof(ExtraDataList));
        ExtraDataList* pExtra = pExtraMemory ? ::new (pExtraMemory) ExtraDataList : nullptr;
        pExtra->SetCount(static_cast<uint16_t>(pObject->i_count));
        TESHealthForm* pHealthForm = TESHealthForm::GetFormAsHealthForm(pObject->pObj);
        pObject->pItemExtra->AddToExtraDataList(pExtra, pHealthForm);
        void* pChangeMemory = ourMalloc(sizeof(ItemChange));
        ItemChange* pChange = pChangeMemory ? ::new (pChangeMemory) ItemChange(pObject->pObj, 0) : nullptr;
        pChange->pExtraObjectList->AddHead(pExtra);
        AddChangeItem(pChange, true);
    }
}

// Resolves the form IDs of every change after a load. While they resolve, the buffer's change flags are 0x400 and its
// form change is 0; both are saved first and put back afterwards, and the form the buffer was loading is cleared and
// restored too.
void InventoryChanges::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    BGSLoadGameBuffer* pBaseBuffer = apLoadGameBuffer;
    TESForm* pForm = pBaseBuffer->GetForm();
    BGSChangeFlags kChangeFlags = apLoadGameBuffer->GetChangeFlags();
    BGSChangeFlags kFormChange = apLoadGameBuffer->GetOldChangeFlags();
    apLoadGameBuffer->SetForm(nullptr);
    BGSChangeFlags kLoadChangeFlags;
    kLoadChangeFlags.~BGSChangeFlags();
    ::new (&kLoadChangeFlags) BGSChangeFlags(0x400);
    apLoadGameBuffer->SetChangeFlags(kLoadChangeFlags);
    BGSChangeFlags kNoFormChange;
    kNoFormChange.~BGSChangeFlags();
    ::new (&kNoFormChange) BGSChangeFlags(0);
    apLoadGameBuffer->SetOldChangeFlags(kNoFormChange);
    for (BSSimpleList<ItemChange*>* pNode = pListofChanges; pNode;) {
        ItemChange* pChange;
        pChange = pNode->GetItem();
        if (pChange) {
            pChange->InitLoadGame(apLoadGameBuffer);
        }
        pNode = pNode->GetNext();
    }
    apLoadGameBuffer->SetForm(pForm);
    apLoadGameBuffer->SetChangeFlags(kChangeFlags);
    apLoadGameBuffer->SetOldChangeFlags(kFormChange);
}

// Removes the leveled item extra from every instance that has one, deletes the instance and takes its count off
// the change (restarting the change's instance walk), then removes and deletes a change left with no instance
// and no count (restarting the walk over the changes).
void InventoryChanges::RemoveDefaultLeveledItems() {
    pRef->AddChange(CHANGE_REFR_LEVELED_INVENTORY);
    BSSimpleList<ItemChange*>* pNode = pListofChanges;
    while (pNode && !pNode->IsEmpty() && pNode->GetItem()) {
        ItemChange* pChange = pNode->GetItem();
        BSSimpleList<ExtraDataList*>* pExtraNode = pChange->pExtraObjectList;
        while (pExtraNode && pExtraNode->GetItem()) {
            ExtraDataList* pExtra = pExtraNode->GetItem();
            if (pExtra && pExtra->GetLeveledItem()) {
                pExtra->RemoveLeveledItem();
                int32_t iNumber = pChange->GetCount();
                pChange->SetCount(iNumber - pExtra->GetCount());
                // Removes from the current node, not from the head of the list.
                pExtraNode->Remove(pExtra);
                delete pExtra;
                pExtraNode = pChange->pExtraObjectList;
            }
            else {
                pExtraNode = pExtraNode->GetNext();
            }
        }
        if (pChange && pChange->pExtraObjectList && pChange->pExtraObjectList->IsEmpty() && !pChange->GetCount()) {
            pListofChanges->Remove(pChange);
            if (pChange) {
                delete pChange;
            }
            pNode = pListofChanges;
        }
        else {
            pNode = pNode->GetNext();
        }
    }
}

// Every scripted object with a nonzero count, first of the base container, then of the changes, gets its count
// of instances with the script and fresh locals. Each instance that receives the script, and each instance of a
// change whose instances already carry a script, runs it once on a new temporary reference that is deleted after.
// A base container object without a change gets a new change, added to the changes at the end.
void InventoryChanges::InitScripts() {
    for (BSSimpleList<ContainerObject*>* pNode = GetOriginalContainer()->GetObjectList(); pNode && !pNode->IsEmpty();
         pNode = pNode->GetNext()) {
        ContainerObject* pObject = pNode->GetItem();
        Script* pScript = TESScriptableForm::GetFormScript(pObject->pObj);
        int32_t iCount = std::abs(pObject->i_count);
        if (!pScript || iCount <= 0) {
            continue;
        }
        ItemChange* pChange = GetObjectInList(pObject->pObj, true, 0);
        if (!pChange || !pChange->GetScript()) {
            bool bNewChange = false;
            if (!pChange) {
                bNewChange = true;
                void* pChangeMemory = ourMalloc(sizeof(ItemChange));
                pChange = pChangeMemory ? ::new (pChangeMemory) ItemChange(pObject->pObj, 0) : nullptr;
            }
            if (!pChange->pExtraObjectList) {
                void* pListMemory = ourMalloc(sizeof(BSSimpleList<ExtraDataList*>));
                pChange->pExtraObjectList = pListMemory ? new (pListMemory) BSSimpleList<ExtraDataList*>() : nullptr;
            }
            else {
                for (BSSimpleList<ExtraDataList*>* pExtraNode = pChange->pExtraObjectList;
                     pExtraNode && pExtraNode->GetItem(); pExtraNode = pExtraNode->GetNext()) {
                    ExtraDataList* pExtra = pExtraNode->GetItem();
                    if (!pExtra || pExtra->GetScript()) {
                        continue;
                    }
                    pExtra->SetScript(pScript);
                    pExtra->SetScriptLocals(pExtra->GetScript()->CloneLocals());
                    void* pRefMemory = ourMalloc(sizeof(TESObjectREFR));
                    TESObjectREFR* pTempObject = pRefMemory ? ::new (pRefMemory) TESObjectREFR : nullptr;
                    pTempObject->SetTemporary();
                    pScript->Run(pTempObject, pExtra->GetScriptLocals(), nullptr, false);
                    delete pTempObject;
                    --iCount;
                }
            }
            for (int32_t i = 0; i < iCount; ++i) {
                void* pExtraMemory = ourMalloc(sizeof(ExtraDataList));
                ExtraDataList* pExtra = pExtraMemory ? ::new (pExtraMemory) ExtraDataList : nullptr;
                pChange->pExtraObjectList->AddHead(pExtra);
                pExtra->SetCount(1);
                if (!pExtra || pExtra->GetScript()) {
                    continue;
                }
                pExtra->SetScript(pScript);
                pExtra->SetScriptLocals(pExtra->GetScript()->CloneLocals());
                void* pRefMemory = ourMalloc(sizeof(TESObjectREFR));
                TESObjectREFR* pTempObject = pRefMemory ? ::new (pRefMemory) TESObjectREFR : nullptr;
                pTempObject->SetTemporary();
                pScript->Run(pTempObject, pExtra->GetScriptLocals(), nullptr, false);
                delete pTempObject;
            }
            if (bNewChange) {
                AddChangeItem(pChange, true);
            }
        }
        else if (pChange) {
            for (BSSimpleList<ExtraDataList*>* pExtraNode = pChange->pExtraObjectList;
                 pExtraNode && pExtraNode->GetItem(); pExtraNode = pExtraNode->GetNext()) {
                ExtraDataList* pExtra = pExtraNode->GetItem();
                if (!pExtra) {
                    continue;
                }
                void* pRefMemory = ourMalloc(sizeof(TESObjectREFR));
                TESObjectREFR* pTempObject = pRefMemory ? ::new (pRefMemory) TESObjectREFR : nullptr;
                pTempObject->SetTemporary();
                pScript->Run(pTempObject, pExtra->GetScriptLocals(), nullptr, false);
                delete pTempObject;
                --iCount;
            }
        }
    }

    for (BSSimpleList<ItemChange*>* pNode = pListofChanges; pNode && pNode->GetItem(); pNode = pNode->GetNext()) {
        ItemChange* pChange = pNode->GetItem();
        Script* pScript = TESScriptableForm::GetFormScript(pChange->GetContainerObject());
        int32_t iCount = std::abs(pChange->GetCount());
        if (!pScript || iCount <= 0) {
            continue;
        }
        if (pChange && !pChange->GetScript()) {
            if (!pChange->pExtraObjectList) {
                void* pListMemory = ourMalloc(sizeof(BSSimpleList<ExtraDataList*>));
                pChange->pExtraObjectList = pListMemory ? new (pListMemory) BSSimpleList<ExtraDataList*>() : nullptr;
            }
            else {
                for (BSSimpleList<ExtraDataList*>* pExtraNode = pChange->pExtraObjectList;
                     pExtraNode && pExtraNode->GetItem(); pExtraNode = pExtraNode->GetNext()) {
                    ExtraDataList* pExtra = pExtraNode->GetItem();
                    if (!pExtra || pExtra->GetScript()) {
                        continue;
                    }
                    pExtra->SetScript(pScript);
                    pExtra->SetScriptLocals(pExtra->GetScript()->CloneLocals());
                    void* pRefMemory = ourMalloc(sizeof(TESObjectREFR));
                    TESObjectREFR* pTempObject = pRefMemory ? ::new (pRefMemory) TESObjectREFR : nullptr;
                    pTempObject->SetTemporary();
                    pScript->Run(pTempObject, pExtra->GetScriptLocals(), nullptr, false);
                    delete pTempObject;
                    --iCount;
                }
            }
            for (int32_t i = 0; i < iCount; ++i) {
                void* pExtraMemory = ourMalloc(sizeof(ExtraDataList));
                ExtraDataList* pExtra = pExtraMemory ? ::new (pExtraMemory) ExtraDataList : nullptr;
                pChange->pExtraObjectList->AddHead(pExtra);
                pExtra->SetCount(1);
                if (!pExtra || pExtra->GetScript()) {
                    continue;
                }
                pExtra->SetScript(pScript);
                pExtra->SetScriptLocals(pExtra->GetScript()->CloneLocals());
                void* pRefMemory = ourMalloc(sizeof(TESObjectREFR));
                TESObjectREFR* pTempObject = pRefMemory ? ::new (pRefMemory) TESObjectREFR : nullptr;
                pTempObject->SetTemporary();
                pScript->Run(pTempObject, pExtra->GetScriptLocals(), nullptr, false);
                delete pTempObject;
            }
        }
        else if (pChange) {
            for (BSSimpleList<ExtraDataList*>* pExtraNode = pChange->pExtraObjectList;
                 pExtraNode && pExtraNode->GetItem(); pExtraNode = pExtraNode->GetNext()) {
                ExtraDataList* pExtra = pExtraNode->GetItem();
                if (!pExtra) {
                    continue;
                }
                void* pRefMemory = ourMalloc(sizeof(TESObjectREFR));
                TESObjectREFR* pTempObject = pRefMemory ? ::new (pRefMemory) TESObjectREFR : nullptr;
                pTempObject->SetTemporary();
                pScript->Run(pTempObject, pExtra->GetScriptLocals(), nullptr, false);
                delete pTempObject;
                --iCount;
            }
        }
    }
}

// Runs each instance script on pTempRef, which takes the owner's cell and position and, per instance, the instance's
// copyable extra data and object. A script that sets bcountdirty restarts the walk at the second change (the head is
// skipped, as the game does).
bool InventoryChanges::RunScripts(TESObjectREFR* pOwner) {
    bcountdirty = false;
    if (!pOwner) {
        return false;
    }
    TESObjectCELL* pCell = pOwner->GetParentCell();
    if (!pCell) {
        return false;
    }
    kRunScriptsLock.Lock(nullptr);
    BSSimpleList<ItemChange*>* pNode = pListofChanges;
    bool bRan = false;
    {
        alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
        AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
        ::new (aucContext) AutoMemContext(
            MEM_CONTEXT_SCRIPTS, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp",
            9803
        );
        pTempRef->SetParentCellSimple(pCell);
        // Calls GetLocationOnReference's code non-virtually.
        pTempRef->SetLocationOnReferenceSimple(*pOwner->QLocationOnReference());
        while (pNode && pNode->GetItem()) {
            ItemChange* pChange = pNode->GetItem();
            for (BSSimpleList<ExtraDataList*>* pExtraNode = pChange->pExtraObjectList;
                 pExtraNode && pExtraNode->GetItem(); pExtraNode = pExtraNode->GetNext()) {
                ExtraDataList* pExtra = pExtraNode->GetItem();
                if (!pExtra) {
                    continue;
                }
                Script* pScript = pExtra->GetScript();
                if (!pScript || !pScript->GetInitialized()) {
                    continue;
                }
                if (pScript->GetFormType() == SCPT_ID) {
                    pTempRef->CopyExtraDataList(pExtra);
                    pTempRef->SetObjectReference(pChange->GetContainerObject());
                    if (pScript->Run(pTempRef, pExtra->GetScriptLocals(), pOwner, false)) {
                        bRan = true;
                    }
                    pTempRef->GetExtra()->SetScriptLocals(nullptr);
                    pTempRef->GetExtra()->RemoveAllCopyableExtra(true);
                }
                if (bcountdirty) {
                    break;
                }
            }
            if (bcountdirty) {
                pNode = pListofChanges;
                bcountdirty = false;
            }
            pNode = pNode->GetNext();
        }
        pTempRef->SetParentCellSimple(nullptr);
        pTempRef->SetObjectReference(nullptr);
        pContext->~AutoMemContext();
    }
    kRunScriptsLock.Unlock();
    return bRan;
}

void InventoryChanges::CleanUpBoundItems(Actor* apActor) {}

void InventoryChanges::RemoveStolenItems(TESObjectREFR* apRef, TESObjectREFR* apStolenContainer, TESForm* apOwner) {
    BSSimpleList<ItemChange*>* pListInv = pListofChanges;
    while (pListInv && !pListInv->IsEmpty()) {
        TESBoundObject* FormFlags;
        BSSimpleList<ItemChange*>* pnext = pListInv->m_pkNext;
        ItemChange* pInventoryItem = pListInv->m_item;
        bool bflag = 0;
        if (!pInventoryItem ||
            (pInventoryItem->iNumber <= 0 ||
             (FormFlags = pInventoryItem->pContainerObj, FormFlags->GetQuestObject()))) {
            BSSimpleList<ItemChange*>* FormHealth_0 = pListInv->m_pkNext;
            if (pnext == FormHealth_0) {
                pListInv = pnext;
            }
            else {
                pListInv = pListofChanges;
            }
        }
        else {
            BSSimpleList<ExtraDataList*>* plistextra = pInventoryItem->pExtraObjectList;
            TESBoundObject* pObj = pInventoryItem->pContainerObj;
            Actor* pactor = 0;
            if (apStolenContainer->IsActor()) {
                pactor = static_cast<Actor*>(apStolenContainer);
            }
            if (plistextra && plistextra->m_item) {
                while (plistextra && (plistextra->m_item && !bflag)) {
                    BSSimpleList<ExtraDataList*>* plistextranext = plistextra->m_pkNext;
                    ExtraDataList* pextra = plistextra->m_item;
                    char v21 = 0;
                    TESForm* pform = pextra->GetOwner();
                    TESForm* pform_2 = 0;
                    TESFaction* pfac = 0;
                    if (pform) {
                        if (pform->GetFormType() == FACT_ID) {
                            pfac = static_cast<TESFaction*>(pform);
                        }
                        else {
                            pform_2 = pform;
                        }
                    }
                    if (!apOwner || apOwner == pextra->GetOwner()) {
                        if (pactor) {
                            if (!apOwner) {
                                apOwner = pactor->GetObjectReference();
                            }
                        }
                        if (pextra->GetOwner()) {
                            if (pextra->GetOwner() != apRef) {
                                TESBoundObject* ObjectReference;
                                if (!pactor ||
                                    ((ObjectReference = pactor->GetObjectReference(), ObjectReference == pform_2) ||
                                     pactor->IsInFaction(pfac))) {
                                    // The compare is unsigned, although ItemsInList returns int.
                                    if (static_cast<uint32_t>(pInventoryItem->pExtraObjectList->ItemsInList()) > 1) {
                                        v21 = 1;
                                    }
                                    else {
                                        bflag = 1;
                                    }
                                    if (apOwner) {
                                        char apDest[264];
                                        BSStringT<char> v18;
                                        int32_t iCount = pextra->GetCount();
                                        if (iCount <= 1) {
                                            const char* v15 = sRemoveItemfromInventory.String();
                                            v18.SPrintF("%s %s", TESFullName::GetFullName(pObj), v15);
                                        }
                                        else {
                                            const char* v14 = sRemoveItemfromInventory.String();
                                            const char* time = sPlural.String();
                                            const char* FullName = TESFullName::GetFullName(pObj);
                                            v18.SPrintF("%i %s%s %s", iCount, FullName, time, v14);
                                        }
                                        BSsprintf(
                                            apDest, 260, "%s\\%s", "Interface\\Icons",
                                            TESTexture::GetTextureName(pObj, pPlayer)
                                        );
                                        const char* PickUpSoundName = pPlayer->GetPickUpSoundName(pObj, 0, 0);
                                        Interface::ShowMessage(
                                            v18.pString, 0,
                                            "Interface\\Icons\\Message Icons\\glow_message_vaultboy_neutral.dds",
                                            PickUpSoundName, 2.0f, 0
                                        );
                                    }
                                    ExtraDataList* v11 = pextra;
                                    int16_t Count = pextra->GetCount();
                                    RemoveItem(apRef, pObj, 0, Count, v11, 0, apStolenContainer, 0, 0, 1, 0, 0);
                                    pListInv = pListofChanges;
                                }
                            }
                        }
                    }
                    if (bflag) {
                        pListInv = pListofChanges;
                        plistextra = 0;
                    }
                    else {
                        if (v21) {
                            plistextra = pInventoryItem->pExtraObjectList;
                        }
                        else {
                            BSSimpleList<ExtraDataList*>* v7 = plistextra->m_pkNext;
                            if (plistextranext == v7) {
                                plistextra = plistextranext;
                            }
                            else {
                                plistextra = pInventoryItem->pExtraObjectList;
                            }
                            if (!plistextra) {
                                BSSimpleList<ItemChange*>* v8 = pListInv->m_pkNext;
                                if (pnext == v8) {
                                    pListInv = pnext;
                                }
                                else {
                                    pListInv = pListofChanges;
                                }
                            }
                        }
                    }
                }
            }
            else {
                BSSimpleList<ItemChange*>* v9 = pListInv->m_pkNext;
                if (pnext == v9) {
                    pListInv = pnext;
                }
                else {
                    pListInv = pListofChanges;
                }
            }
        }
    }
}

// Loads the item changes of the form-list save format. The buffer's form, change flags and old change flags are saved,
// set to (no form, 0x400, 0) while the changes load, and put back afterwards. A change whose object did not load is
// deleted and a null entry added in its place.
void InventoryChanges::LoadGame_ObjectsList(BGSLoadFormBuffer* a1) {
    BGSLoadGameBuffer* pBaseBuffer = a1;
    TESForm* pForm = pBaseBuffer->GetForm();
    BGSChangeFlags kChangeFlags = a1->GetChangeFlags();
    BGSChangeFlags kOldChangeFlags = a1->GetOldChangeFlags();
    a1->SetForm(nullptr);
    BGSChangeFlags kLoadChangeFlags;
    kLoadChangeFlags.~BGSChangeFlags();
    ::new (&kLoadChangeFlags) BGSChangeFlags(0x400);
    a1->SetChangeFlags(kLoadChangeFlags);
    BGSChangeFlags kNoOldChangeFlags;
    kNoOldChangeFlags.~BGSChangeFlags();
    ::new (&kNoOldChangeFlags) BGSChangeFlags(0);
    a1->SetOldChangeFlags(kNoOldChangeFlags);
    uint32_t uiCount = a1->LoadVariableSizedValue();
    for (uint32_t i = 0; i < uiCount; ++i) {
        ItemChange* pNew;
        void* pMemory = ourMalloc(sizeof(ItemChange));
        if (pMemory) {
            pNew = ::new (pMemory) ItemChange;
        }
        else {
            pNew = nullptr;
        }
        ItemChange* pChange = pNew;
        pChange->LoadGame(a1);
        if (!pChange->pContainerObj) {
            pChange->DeleteAllExtra();
            if (pChange) {
                delete pChange;
            }
            pChange = nullptr;
        }
        pListofChanges->AddHead(pChange);
    }
    a1->SetForm(pForm);
    a1->SetChangeFlags(kChangeFlags);
    a1->SetOldChangeFlags(kOldChangeFlags);
}

ItemChange* InventoryChanges::GetItembyIndex(int32_t aiIndex) {
    ExtraDataList* pcurextra;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_INVENTORY, 1, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp", 0x187F
    );
    BSSimpleList<ContainerObject*>* pOriginalList = 0;
    void* ecx0 = 0;
    if (GetOriginalContainer()) {
        TESContainer* OriginalContainer = GetOriginalContainer();
        pOriginalList = OriginalContainer->GetFilteredList();
    }
    int32_t inumber = 0;
    ItemChange* preturnitem = 0;
    TESContainer* pcontainer = GetOriginalContainer();
    for (BSSimpleList<ContainerObject*>* plist = pOriginalList; plist && (plist->m_item && !preturnitem);
         plist = plist->m_pkNext) {
        ContainerObject* pobject = plist->m_item;
        TESLevItem* pObj = 0;
        if (pobject->pObj->GetFormType() == LVLI_ID) {
            pObj = static_cast<TESLevItem*>(pobject->pObj);
        }
        if (pobject) {
            if (!pObj) {
                ItemChange* pitem = GetObjectInList(pobject->pObj, 1, 0);
                int32_t iorgCount = pcontainer->GetObjectCount(pobject->pObj);
                if (pobject && (pitem && iorgCount <= 0)) {
                    preturnitem = 0;
                }
                else {
                    int32_t v3;
                    if ((iorgCount < 0 && pitem) ||
                        ((pitem && (v3 = pitem->iNumber, iorgCount + v3 <= 0)) ||
                         (pitem &&
                          (pitem->pExtraObjectList &&
                           (pitem->pExtraObjectList->m_item && pitem->pExtraObjectList->m_item->HasLeveledItem()))))) {
                        preturnitem = 0;
                    }
                    else {
                        if (!pobject || (inumber != aiIndex || (pitem && pitem->GetAmountNonDefaultExtra()))) {
                            if (pitem) {
                                int32_t inumb = 0;
                                if (pitem->pExtraObjectList) {
                                    inumb = pitem->GetAmountNonDefaultExtra();
                                }
                                if (aiIndex >= inumber + inumb) {
                                    int32_t AmountNonDefaultExtra = pitem->GetAmountNonDefaultExtra();
                                    inumber += AmountNonDefaultExtra;
                                    bool bflag = 0;
                                    TESObjectWEAP* pweap = 0;
                                    if (pitem->pContainerObj->GetFormType() == WEAP_ID) {
                                        pweap = static_cast<TESObjectWEAP*>(pitem->pContainerObj);
                                    }
                                    if (!pitem->GetExtraTotalCount(0) && pitem->GetWorn(0) &&
                                        (pitem->iNumber <= 0 ||
                                         (pitem->pContainerObj->GetFormType() == AMMO_ID ||
                                          (pweap &&
                                           (pweap->GetWeaponType() == WEAPON_ONE_HAND_GRENADE ||
                                            (pweap->GetWeaponType() == WEAPON_ONE_HAND_MINE ||
                                             pweap->GetWeaponType() == WEAPON_ONE_HAND_THROWN)))))) {
                                        bflag = 1;
                                    }
                                    if (!bflag) {
                                        int32_t ExtraTotalCount = pitem->GetExtraTotalCount(1);
                                        if (ExtraTotalCount < pitem->iNumber + iorgCount) {
                                            int32_t FormHealth_0;
                                            if (!pitem->HasLeveledExtra() ||
                                                (FormHealth_0 = pitem->iNumber, FormHealth_0 <= iorgCount)) {
                                                if (inumber == aiIndex) {
                                                    ItemChange* preturnitem_4;
                                                    ItemChange* v37 =
                                                        static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
                                                    if (v37) {
                                                        preturnitem_4 = ::new (v37) ItemChange;
                                                    }
                                                    else {
                                                        preturnitem_4 = 0;
                                                    }
                                                    preturnitem = preturnitem_4;
                                                    if (pitem) {
                                                        int32_t v12 = pitem->iNumber + iorgCount;
                                                        int32_t v13 = pitem->GetExtraTotalCount(0);
                                                        preturnitem->iNumber = v12 - v13;
                                                    }
                                                    else {
                                                        preturnitem->iNumber = iorgCount;
                                                    }
                                                    preturnitem->pContainerObj = pobject->pObj;
                                                    if (pitem) {
                                                        if (pitem->GetScript()) {
                                                            if (pitem->pExtraObjectList) {
                                                                if (pitem->pExtraObjectList->ItemsInList()) {
                                                                    if (!preturnitem->pExtraObjectList) {
                                                                        BSSimpleList<ExtraDataList*>* v27;
                                                                        BSSimpleList<ExtraDataList*>* v36 =
                                                                            static_cast<BSSimpleList<ExtraDataList*>*>(
                                                                                ourMalloc(
                                                                                    sizeof(BSSimpleList<ExtraDataList*>)
                                                                                )
                                                                            );
                                                                        if (v36) {
                                                                            v27 = ::new (v36)
                                                                                BSSimpleList<ExtraDataList*>;
                                                                        }
                                                                        else {
                                                                            v27 = 0;
                                                                        }
                                                                        preturnitem->pExtraObjectList = v27;
                                                                    }
                                                                    for (BSSimpleList<ExtraDataList*>* plist_3 =
                                                                             pitem->pExtraObjectList;
                                                                         plist_3 && plist_3->m_item;
                                                                         plist_3 = plist_3->m_pkNext) {
                                                                        BSSimpleList<ExtraDataList*>* v22 = plist_3;
                                                                        preturnitem->pExtraObjectList->AddHead(
                                                                            v22->m_item
                                                                        );
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                                else {
                                                    ++inumber;
                                                }
                                            }
                                        }
                                    }
                                }
                                else {
                                    for (BSSimpleList<ExtraDataList*>* pitem__pextralist = pitem->pExtraObjectList;
                                         pitem__pextralist && (pitem__pextralist->m_item && !preturnitem);
                                         pitem__pextralist = pitem__pextralist->GetNext()) {
                                        pcurextra = pitem__pextralist->m_item;
                                        if (!pcurextra->IsExtraDefaultforContainer(0)) {
                                            if (inumber == aiIndex) {
                                                ItemChange* preturnitem_3;
                                                int32_t icount = pcurextra->GetCount();
                                                int32_t v7 = pitem->iNumber;
                                                if (icount > iorgCount + v7) {
                                                    int32_t v8 = pitem->iNumber;
                                                    icount = iorgCount + v8;
                                                }
                                                ItemChange* v38 =
                                                    static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
                                                if (v38) {
                                                    preturnitem_3 =
                                                        ::new (v38) ItemChange(pitem->pContainerObj, icount);
                                                }
                                                else {
                                                    preturnitem_3 = 0;
                                                }
                                                preturnitem = preturnitem_3;
                                                preturnitem_3->pExtraObjectList->AddHead(pcurextra);
                                            }
                                            ++inumber;
                                        }
                                    }
                                }
                            }
                            else {
                                ++inumber;
                            }
                        }
                        else {
                            ItemChange* preturnitem_2;
                            ItemChange* v40 = static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
                            if (v40) {
                                preturnitem_2 = ::new (v40) ItemChange;
                            }
                            else {
                                preturnitem_2 = 0;
                            }
                            preturnitem = preturnitem_2;
                            if (pitem) {
                                if (iorgCount >= 0) {
                                    int32_t v4 = pitem->iNumber + iorgCount;
                                    int32_t v5 = pitem->GetExtraTotalCount(0);
                                    preturnitem->iNumber = v4 - v5;
                                }
                                else {
                                    preturnitem->iNumber = pitem->iNumber + iorgCount;
                                }
                            }
                            else {
                                if (iorgCount >= 0) {
                                    preturnitem->iNumber = iorgCount;
                                }
                                else {
                                    preturnitem->iNumber = -iorgCount;
                                }
                            }
                            preturnitem->pContainerObj = pobject->pObj;
                            if (pitem) {
                                TESObjectREFR* ItemOwnership;
                                if (pitem->GetScript() ||
                                    (pitem->GetHotKey() >= 0 ||
                                     (ItemOwnership = static_cast<TESObjectREFR*>(pitem->GetItemOwnership()),
                                      ItemOwnership != pRef))) {
                                    if (pitem->pExtraObjectList) {
                                        if (pitem->pExtraObjectList->ItemsInList()) {
                                            if (!preturnitem->pExtraObjectList) {
                                                BSSimpleList<ExtraDataList*>* v30;
                                                BSSimpleList<ExtraDataList*>* v39 =
                                                    static_cast<BSSimpleList<ExtraDataList*>*>(
                                                        ourMalloc(sizeof(BSSimpleList<ExtraDataList*>))
                                                    );
                                                if (v39) {
                                                    v30 = ::new (v39) BSSimpleList<ExtraDataList*>;
                                                }
                                                else {
                                                    v30 = 0;
                                                }
                                                preturnitem->pExtraObjectList = v30;
                                            }
                                            for (BSSimpleList<ExtraDataList*>* plist_2 = pitem->pExtraObjectList;
                                                 plist_2 && plist_2->m_item; plist_2 = plist_2->m_pkNext) {
                                                BSSimpleList<ExtraDataList*>* v21 = plist_2;
                                                preturnitem->pExtraObjectList->AddHead(v21->m_item);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    if (pOriginalList) {
        pOriginalList->RemoveAll();
        if (pOriginalList) {
            delete pOriginalList;
        }
    }
    if (!preturnitem) {
        for (BSSimpleList<ItemChange*>* pkFirst = pListofChanges; pkFirst && (pkFirst->m_item && !preturnitem);
             pkFirst = pkFirst->m_pkNext) {
            ItemChange* pitem_2 = pkFirst->m_item;
            int32_t ioriginalcount = 0;
            if (GetOriginalContainer()) {
                if (pitem_2) {
                    TESBoundObject* pContainerObj = pitem_2->pContainerObj;
                    TESContainer* pcontainer_2 = GetOriginalContainer();
                    ioriginalcount = pcontainer_2->GetObjectCount(pContainerObj);
                }
            }
            if (ioriginalcount < 0 && pitem_2->GetWorn(0)) {
                pitem_2->pExtraObjectList->m_item->SetCount(-static_cast<int16_t>(ioriginalcount));
            }
            if (pitem_2) {
                if (pitem_2->iNumber > 0 || ioriginalcount < 0) {
                    TESContainer* v15;
                    TESBoundObject* v24;
                    if ((pitem_2->pExtraObjectList &&
                         (pitem_2->pExtraObjectList->m_item && pitem_2->pExtraObjectList->m_item->HasLeveledItem())) ||
                        (ioriginalcount < 0 ||
                         (!GetOriginalContainer() ||
                          (v24 = pitem_2->pContainerObj, (v15 = GetOriginalContainer(), !v15->HasObject(v24)))))) {
                        int32_t inumb_2;
                        int32_t itotalNonDefaultCount = 0;
                        for (ExtraDataList* pExtra : pitem_2->pExtraObjectList->ItemsUntilNull()) {
                            if (!pExtra->IsExtraDefaultforContainer(0)) {
                                int16_t Count = pExtra->GetCount();
                                if (Count > 0) {
                                    itotalNonDefaultCount += Count;
                                }
                            }
                        }
                        if (pitem_2->pExtraObjectList &&
                            (pitem_2->pExtraObjectList->m_item &&
                             (inumb_2 = pitem_2->GetAmountNonDefaultExtra(), aiIndex < inumber + inumb_2))) {
                            for (BSSimpleList<ExtraDataList*>* pextralist = pitem_2->pExtraObjectList;
                                 pextralist && (pextralist->m_item && !preturnitem);
                                 pextralist = pextralist->m_pkNext) {
                                ExtraDataList* pcurextra_2 = pextralist->m_item;
                                if (inumber == aiIndex) {
                                    if (!pcurextra_2->IsExtraDefaultforContainer(0)) {
                                        int16_t v44 = pcurextra_2->GetCount();
                                        if (v44 > 0) {
                                            ItemChange* preturnitem_5;
                                            ItemChange* v35 = static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
                                            if (v35) {
                                                preturnitem_5 = ::new (v35) ItemChange(pitem_2->pContainerObj, v44);
                                            }
                                            else {
                                                preturnitem_5 = 0;
                                            }
                                            preturnitem = preturnitem_5;
                                            preturnitem_5->pExtraObjectList->AddHead(pcurextra_2);
                                        }
                                    }
                                }
                                else {
                                    if (!pcurextra_2->IsExtraDefaultforContainer(0) && pcurextra_2->GetCount() > 0) {
                                        ++inumber;
                                    }
                                }
                            }
                        }
                        else {
                            int32_t v17 = pitem_2->iNumber;
                            if (v17 == itotalNonDefaultCount) {
                                int32_t v18 = pitem_2->GetAmountNonDefaultExtra();
                                inumber += v18;
                            }
                            else {
                                int32_t v43 = 0;
                                int32_t v19 = pitem_2->iNumber;
                                if (v19 > itotalNonDefaultCount) {
                                    int32_t iamount = pitem_2->GetAmountNonDefaultExtra();
                                    if (iamount > 0) {
                                        inumber += iamount;
                                    }
                                }
                                if (inumber != aiIndex ||
                                    (preturnitem || pitem_2->iNumber - itotalNonDefaultCount <= 0)) {
                                    ++inumber;
                                }
                                else {
                                    ItemChange* preturnitem_6;
                                    ExtraDataList* pextra = 0;
                                    ItemChange* v34 = static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
                                    if (v34) {
                                        preturnitem_6 = ::new (v34) ItemChange(pitem_2->pContainerObj, 0);
                                    }
                                    else {
                                        preturnitem_6 = 0;
                                    }
                                    preturnitem = preturnitem_6;
                                    preturnitem_6->iNumber = pitem_2->iNumber - itotalNonDefaultCount;
                                    if (pitem_2->pExtraObjectList) {
                                        for (ExtraDataList*& rpItem : pitem_2->pExtraObjectList->ItemsUntilNull()) {
                                            pextra = rpItem;
                                            if (!pextra->GetWorn(0) && pextra->IsExtraDefaultforContainer(1)) {
                                                preturnitem->pExtraObjectList->AddTail(pextra);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return preturnitem;
}

void InventoryChanges::GetItemModGroup(TESBoundObject* apObj, BSSimpleList<ItemChange*>* apReturnList) {
    if (apObj) {
        if (apReturnList) {
            if (apObj->GetFormType() == WEAP_ID) {
                InventoryChanges::IterData* icurrIndex = GetIterator();
                ItemChange* pcurrItem = 0;
                TESBoundObject* itemmod = 0;
                do {
                    pcurrItem = Iterate(icurrIndex);
                    if (pcurrItem) {
                        if (pcurrItem->pContainerObj->GetFormType() == IMOD_ID) {
                            TESObjectIMOD* v4;
                            itemmod = pcurrItem->pContainerObj;
                            TESObjectIMOD* ModItem = static_cast<TESObjectWEAP*>(apObj)->TESObjectWEAP::GetModItem(1u);
                            if (itemmod == ModItem ||
                                (v4 = static_cast<TESObjectWEAP*>(apObj)->TESObjectWEAP::GetModItem(2u),
                                 itemmod == v4)) {
                                apReturnList->AddHead(pcurrItem);
                            }
                            else {
                                TESObjectIMOD* v5 = static_cast<TESObjectWEAP*>(apObj)->TESObjectWEAP::GetModItem(4u);
                                if (itemmod == v5) {
                                    apReturnList->AddHead(pcurrItem);
                                }
                            }
                        }
                        else {
                            if (pcurrItem) {
                                delete pcurrItem;
                            }
                        }
                    }
                }
                while (pcurrItem);
                if (icurrIndex) {
                    delete icurrIndex;
                }
            }
        }
    }
}

void InventoryChanges::RemoveHotKeyItem(ItemChange* apItem, int32_t aiHotkey) {
    ItemChange* pItemGroup = GetObjectInList(apItem->pContainerObj, 1, 0);
    if (pItemGroup) {
        BSSimpleList<ExtraDataList*>* pNode = pItemGroup->pExtraObjectList;
        while (pNode) {
            ExtraDataList* pextra = pNode->m_item;
            if (pextra &&
                (((aiHotkey != -1 && pextra->GetHotKey() == aiHotkey) ||
                  (aiHotkey == -1 && pextra->GetHotKey() >= 0)) &&
                 ((pextra->RemoveHotKey(), pextra->IsExtraListEmpty()) ||
                  (pextra->GetCount() > 1 && pextra->ItemsInList() == 1)))) {
                pItemGroup->pExtraObjectList->Remove(pextra);
                pNode = pItemGroup->pExtraObjectList;
            }
            else {
                pNode = pNode->m_pkNext;
            }
        }
        if (pItemGroup->pExtraObjectList) {
            if (pItemGroup->pExtraObjectList->IsEmpty()) {
                if (pItemGroup->pExtraObjectList) {
                    delete pItemGroup->pExtraObjectList;
                }
                pItemGroup->pExtraObjectList = 0;
                if (!pItemGroup->iNumber) {
                    pListofChanges->Remove(pItemGroup);
                    if (pItemGroup) {
                        delete pItemGroup;
                    }
                }
            }
        }
    }
}

void InventoryChanges::SetHotKeyItem(ItemChange* apItem, ExtraDataList* apExtra, int32_t aiKey) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_INVENTORY, 1, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp", 1974
    );
    if (apItem && aiKey >= 0 && aiKey <= 7) {
        if (pPlayer->GetAmmoForHotKey(aiKey)) {
            pPlayer->SetAmmoForHotKey(aiKey, 0);
        }
        RemoveHotKeyItem(apItem, -1);
        ItemChange* pitemGroup = GetObjectInList(apItem->pContainerObj, 1, 0);
        if (pitemGroup) {
            ExtraDataList* pfoundExtra = 0;
            if (!pitemGroup->pExtraObjectList) {
                BSSimpleList<ExtraDataList*>* v13;
                BSSimpleList<ExtraDataList*>* v19 =
                    static_cast<BSSimpleList<ExtraDataList*>*>(ourMalloc(sizeof(BSSimpleList<ExtraDataList*>)));
                if (v19) {
                    v13 = ::new (v19) BSSimpleList<ExtraDataList*>;
                }
                else {
                    v13 = 0;
                }
                pitemGroup->pExtraObjectList = v13;
            }
            BSSimpleList<ExtraDataList*>* pNode = pitemGroup->pExtraObjectList;
            while (pNode && pNode->m_item) {
                if (!pfoundExtra) {
                    if (pNode->m_item->IsExtraDefaultforContainer(0)) {
                        pfoundExtra = pNode->m_item;
                    }
                }
                if (pNode->m_item == apExtra) {
                    pfoundExtra = apExtra;
                }
                pNode = pNode->m_pkNext;
            }
            if (pfoundExtra) {
                pfoundExtra->SetHotKey(aiKey);
            }
            else {
                ExtraDataList* pnewExtra;
                ExtraDataList* v18 = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
                if (v18) {
                    pnewExtra = ::new (v18) ExtraDataList;
                }
                else {
                    pnewExtra = 0;
                }
                ExtraDataList* pnewExtra_2 = pnewExtra;
                pnewExtra->SetHotKey(aiKey);
                int16_t v5 = pitemGroup->iNumber;
                int16_t v6 = v5 - pitemGroup->GetExtraTotalCount(0);
                bool Worn = pitemGroup->GetWorn(0);
                pnewExtra->SetCount(v6 - Worn);
                pitemGroup->pExtraObjectList->AddHead(pnewExtra_2);
            }
        }
        else {
            BSSimpleList<ExtraDataList*>* v9;
            ExtraDataList* pnewChangeExtra;
            ItemChange* pnewChange;
            ItemChange* v17 = static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
            if (v17) {
                uint32_t v8 = apItem->iNumber;
                pnewChange = ::new (v17) ItemChange(apItem->pContainerObj, v8);
            }
            else {
                pnewChange = 0;
            }
            ItemChange* pnewChange_2 = pnewChange;
            ExtraDataList* v16 = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
            if (v16) {
                pnewChangeExtra = ::new (v16) ExtraDataList;
            }
            else {
                pnewChangeExtra = 0;
            }
            ExtraDataList* pnewChangeExtra_2 = pnewChangeExtra;
            BSSimpleList<ExtraDataList*>* v15 =
                static_cast<BSSimpleList<ExtraDataList*>*>(ourMalloc(sizeof(BSSimpleList<ExtraDataList*>)));
            if (v15) {
                v9 = ::new (v15) BSSimpleList<ExtraDataList*>;
            }
            else {
                v9 = 0;
            }
            pnewChange_2->pExtraObjectList = v9;
            pnewChangeExtra_2->SetHotKey(aiKey);
            pnewChange_2->pExtraObjectList->AddHead(pnewChangeExtra_2);
            pListofChanges->AddHead(pnewChange_2);
        }
    }
}

void InventoryChanges::AddItem(TESObjectREFR* apObject, int32_t ainumber, bool abAddRefId, bool abWearit) {
    ExtraDataList* pExtra;
    ExtraDataList* pExtra_2;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_INVENTORY, 1, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp", 3131
    );
    bcountdirty = 1;
    fpreviousContainerWeight = fcontainerweight;
    fcontainerweight = -1.0f;
    if (apObject->GetObjectReference()->GetFormType() == WEAP_ID) {
        if (TESHealthForm::GetFormHealth(apObject->GetObjectReference()) && (&apObject->m_Extra)->GetHealth() == -1.0) {
            if (static_cast<TESObjectWEAP*>(apObject->GetObjectReference())->TESObjectWEAP::GetWeaponType() <
                WEAPON_ONE_HAND_GRENADE) {
                int32_t afScale = pRef->iFormID;
                const char* v13 = pRef->GetFormEditorID();
                int32_t CastingType = apObject->iFormID;
                const char* v5 = apObject->GetFormEditorID();
                Warning("Adding full health weapon ref '%s' (%08X) to ref '%s' (%08X).", v5, CastingType, v13, afScale);
                TESHealthForm* pweap = dynamic_cast<TESHealthForm*>(apObject->GetObjectReference());
                uint32_t v50 = pweap->GetFormHealth();
                if ((&apObject->m_Extra)->HasWeaponMods()) {
                    v50 = static_cast<uint32_t>(static_cast<int64_t>(
                        static_cast<TESObjectWEAP*>(apObject->GetObjectReference())
                            ->TESObjectWEAP::GetModEffectValue(WEAPON_MOD_MAX_HEALTH, 0) +
                        static_cast<double>(v50)
                    ));
                }
                v50 = BSMIN(static_cast<uint32_t>(static_cast<int64_t>(static_cast<double>(v50) * 0.99)), v50 - 1);
                apObject->SetHealth(static_cast<float>(v50));
            }
        }
        else {
            if (static_cast<TESObjectWEAP*>(apObject->GetObjectReference())->TESObjectWEAP::IsThrownWeapon() &&
                (&apObject->m_Extra)->GetHealth() != -1.0) {
                int32_t afScaleb = pRef->iFormID;
                const char* v14 = pRef->GetFormEditorID();
                int32_t v11 = apObject->iFormID;
                const char* v6 = apObject->GetFormEditorID();
                Warning("Adding damaged throwing weapon ref '%s' (%08X) to ref '%s' (%08X).", v6, v11, v14, afScaleb);
                TESHealthForm* v49 = dynamic_cast<TESHealthForm*>(apObject->GetObjectReference());
                apObject->SetHealth(static_cast<float>(v49->GetFormHealth()));
            }
        }
    }
    if (apObject->GetObjectReference()->GetFormType() == ARMO_ID ||
        apObject->GetObjectReference()->GetFormType() == CLOT_ID) {
        if (TESHealthForm::GetFormHealth(apObject->GetObjectReference())) {
            if ((&apObject->m_Extra)->GetHealth() == -1.0) {
                int32_t afScaled = pRef->iFormID;
                const char* v15 = pRef->GetFormEditorID();
                int32_t v12 = apObject->iFormID;
                const char* v7 = apObject->GetFormEditorID();
                Warning("Adding full health armor ref '%s' (%08X) to ref '%s' (%08X).", v7, v12, v15, afScaled);
                TESHealthForm* v48 = dynamic_cast<TESHealthForm*>(apObject->GetObjectReference());
                uint32_t v8 = v48->GetFormHealth();
                uint32_t v47 =
                    BSMIN(static_cast<uint32_t>(static_cast<int64_t>(static_cast<double>(v8) / 0.99)), v8 - 1);
                apObject->SetHealth(static_cast<float>(v47));
            }
        }
    }
    int32_t iammocount = 0;
    ExtraAmmo* pammo = (&apObject->m_Extra)->GetAmmoExtra();
    if (pammo) {
        iammocount = pammo->iCount;
        (&apObject->m_Extra)->SetAmmoExtra(0, 0);
        apObject->AddChange(2048);
    }
    iammocount *= ainumber;
    if (pRef) {
        pRef->AddChange(32);
    }
    ItemChange* pitem = GetObjectInList(apObject->GetObjectReference(), 1, 0);
    (&apObject->m_Extra)->RemoveCount();
    ExtraDataList* v37 = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
    if (v37) {
        pExtra = ::new (v37) ExtraDataList;
    }
    else {
        pExtra = 0;
    }
    pExtra_2 = pExtra;
    bool bIsThrown = 0;
    if (!(&apObject->m_Extra)->IsExtraListEmpty()) {
        TESObjectWEAP* ObjectReference = 0;
        if (apObject->GetObjectReference()->GetFormType() == WEAP_ID) {
            ObjectReference = static_cast<TESObjectWEAP*>(apObject->GetObjectReference());
            bIsThrown = ObjectReference->IsThrownWeapon();
        }
        pExtra_2->CopyListForContainer(&apObject->m_Extra, 0);
        if (apObject->GetObjectReference()->GetFormType() != AMMO_ID &&
            (!ObjectReference || !ObjectReference->IsThrownWeapon())) {
            float afScalef = apObject->fRefScale;
            pExtra_2->SetScale(afScalef);
        }
        if (pExtra_2->GetOwner() == pPlayer && pRef == pPlayer) {
            pExtra_2->RemoveOwnership();
        }
    }
    if (abWearit) {
        pExtra_2->SetWorn(1, 0);
    }
    if (!pitem) {
        ItemChange* pnewItem;
        ItemChange* v34 = static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
        if (v34) {
            pnewItem = ::new (v34) ItemChange(apObject->GetObjectReference(), ainumber);
        }
        else {
            pnewItem = 0;
        }
        ItemChange* pnewItem_2 = pnewItem;
        if (ainumber > 1) {
            pExtra_2->SetCount(ainumber);
        }
        if (apObject->GetRefPersists()) {
            pExtra_2->SetReferencePointer(apObject);
            if (!pnewItem_2->pExtraObjectList) {
                BSSimpleList<ExtraDataList*>* v25;
                BSSimpleList<ExtraDataList*>* v33 =
                    static_cast<BSSimpleList<ExtraDataList*>*>(ourMalloc(sizeof(BSSimpleList<ExtraDataList*>)));
                if (v33) {
                    v25 = ::new (v33) BSSimpleList<ExtraDataList*>;
                }
                else {
                    v25 = 0;
                }
                pnewItem_2->pExtraObjectList = v25;
            }
            pnewItem_2->pExtraObjectList->AddHead(pExtra_2);
        }
        else if (pExtra_2) {
            if (pExtra_2->IsExtraListEmpty()) {
                delete pExtra_2;
                pExtra_2 = 0;
            }
            else {
                if (!pnewItem_2->pExtraObjectList) {
                    BSSimpleList<ExtraDataList*>* v24;
                    BSSimpleList<ExtraDataList*>* v32 =
                        static_cast<BSSimpleList<ExtraDataList*>*>(ourMalloc(sizeof(BSSimpleList<ExtraDataList*>)));
                    if (v32) {
                        v24 = ::new (v32) BSSimpleList<ExtraDataList*>;
                    }
                    else {
                        v24 = 0;
                    }
                    pnewItem_2->pExtraObjectList = v24;
                }
                pnewItem_2->pExtraObjectList->AddHead(pExtra_2);
            }
        }
        pListofChanges->AddTail(pnewItem_2);
    }
    else {
        pitem->iNumber += ainumber;
        if (apObject->GetRefPersists()) {
            pExtra_2->SetReferencePointer(apObject);
            if (ainumber > 1) {
                pExtra_2->SetCount(ainumber);
            }
            if (!pitem->pExtraObjectList) {
                BSSimpleList<ExtraDataList*>* v29;
                BSSimpleList<ExtraDataList*>* v36 =
                    static_cast<BSSimpleList<ExtraDataList*>*>(ourMalloc(sizeof(BSSimpleList<ExtraDataList*>)));
                if (v36) {
                    v29 = ::new (v36) BSSimpleList<ExtraDataList*>;
                }
                else {
                    v29 = 0;
                }
                pitem->pExtraObjectList = v29;
            }
            pitem->pExtraObjectList->AddHead(pExtra_2);
        }
        else {
            BSSimpleList<ExtraDataList*>* pNode = pitem->pExtraObjectList;
            bool bAddNew = 1;
            while (pNode && pNode->m_item && bAddNew) {
                ExtraDataList* pCurrent = pNode->m_item;
                ExtraDataList* pcompare = pExtra_2;
                bool bDiffers = 1;
                if (pcompare) {
                    bDiffers = pcompare->CompareListForContainer(pCurrent, 0, bIsThrown) ||
                        pCurrent->CompareListForContainer(pcompare, 0, bIsThrown);
                }
                if (bDiffers) {
                    pNode = pNode->m_pkNext;
                }
                else {
                    int16_t Count = pCurrent->GetCount();
                    pCurrent->SetCount(ainumber + Count);
                    if (pCurrent->BaseExtraList::IsExtraListEmpty()) {
                        pitem->pExtraObjectList->Remove(pCurrent);
                        if (pCurrent) {
                            delete pCurrent;
                        }
                    }
                    bAddNew = 0;
                }
            }
            if (bAddNew) {
                if (pExtra_2 && pExtra_2->IsExtraListEmpty()) {
                    if (pExtra_2) {
                        delete pExtra_2;
                    }
                    pExtra_2 = 0;
                }
                else {
                    if (!pitem->pExtraObjectList) {
                        BSSimpleList<ExtraDataList*>* v27;
                        BSSimpleList<ExtraDataList*>* v35 =
                            static_cast<BSSimpleList<ExtraDataList*>*>(ourMalloc(sizeof(BSSimpleList<ExtraDataList*>)));
                        if (v35) {
                            v27 = ::new (v35) BSSimpleList<ExtraDataList*>;
                        }
                        else {
                            v27 = 0;
                        }
                        pitem->pExtraObjectList = v27;
                    }
                    pitem->pExtraObjectList->AddHead(pExtra_2);
                }
            }
            else {
                if (pExtra_2) {
                    delete pExtra_2;
                }
                pExtra_2 = 0;
            }
        }
        if (pitem->pExtraObjectList) {
            if (pitem->pExtraObjectList->IsEmpty() && !pitem->iNumber) {
                pListofChanges->Remove(pitem);
                if (pExtra_2) {
                    delete pExtra_2;
                }
                pExtra_2 = 0;
                if (pitem) {
                    delete pitem;
                }
            }
        }
    }
    if (pExtra_2) {
        if (pExtra_2->IsExtraListEmpty()) {
            if (pExtra_2) {
                delete pExtra_2;
            }
        }
    }
    if (iammocount > 0) {
        TESObjectWEAP* pweapon = static_cast<TESObjectWEAP*>(apObject->GetObjectReference());
        // Computes the holder and passes 0 to GetCurrentAmmo all the same, as the game does.
        [[maybe_unused]] Actor* pActor = pRef->IsActor() ? static_cast<Actor*>(pRef) : 0;
        TESBoundObject* pammoObj = pweapon->GetCurrentAmmo(0);
        if (pammoObj) {
            AddItem(pammoObj, 0, iammocount);
        }
    }
}

TESBoundObject* InventoryChanges::GetObjectbyPackObjType(PACKAGE_OBJECT_TYPE aeObjectType, int32_t& ainumb) {
    BSSimpleList<ContainerObject*>* pList = &GetOriginalContainer()->objectList;
    TESBoundObject* pObj = 0;
    while (pList && !pObj) {
        pObj = pList->m_item->pObj;
        if (pObj && TESPackage::GetObjectTypeFromForm(pObj) == aeObjectType) {
            uint32_t v3;
            ItemChange* pitem = GetObjectInList(pObj, 1, 0);
            if (!pitem || (v3 = pitem->iNumber, pList->m_item->i_count + v3)) {
                if (pitem) {
                    uint32_t FormHealth_0 = pitem->iNumber;
                    ainumb = pList->m_item->i_count + FormHealth_0;
                }
                else {
                    ainumb = pList->m_item->i_count;
                }
            }
        }
        else {
            pObj = 0;
        }
        pList = pList->m_pkNext;
    }
    for (BSSimpleList<ItemChange*>* pListInv = pListofChanges; pListInv && !pObj; pListInv = pListInv->m_pkNext) {
        pObj = pListInv->m_item->pContainerObj;
        if (pObj && TESPackage::GetObjectTypeFromForm(pObj) == aeObjectType) {
            TESContainer* OriginalContainer = GetOriginalContainer();
            if (!OriginalContainer->HasObject(pObj) && pListInv->m_item->iNumber > 0) {
                ainumb = pListInv->m_item->iNumber;
            }
        }
        else {
            pObj = 0;
        }
    }
    return pObj;
}

TESObjectREFR* InventoryChanges::DropItemIntoWorld(
    TESObjectREFR* a1,
    TESBoundObject* a2,
    int32_t a3,
    TESObjectREFR* a4,
    const NiPoint3* a5,
    const NiPoint3* a6
) {
    const NiPoint3* v11;
    NiPoint3 angle;
    NiPoint3 point;
    if (a5) {
        point.x = a5->x;
        point.y = a5->y;
        point.z = a5->z;
    }
    else {
        NiPoint3 inFrontAndUp_2;
        NiMatrix3 rot;
        point = a1->GetLocationOnReference();
        NiPoint3 v13 = a1->GetAngleOnReference();
        rot.FromEulerAnglesXYZ(v13.x, v13.y, v13.z);
        NiPoint3 inFrontAndUp(0.0f, 50.0f, 30.0f);
        point.operator+=((inFrontAndUp_2 = rot * inFrontAndUp));
    }
    if (a6) {
        v11 = a6;
    }
    else {
        const NiPoint3* AngleOnReference;
        if (a1) {
            AngleOnReference = &a1->GetAngleOnReference();
        }
        else {
            AngleOnReference = &NiPoint3_ZERO;
        }
        v11 = AngleOnReference;
    }
    angle = *v11;
    TESWorldSpace* WorldSpace = a1->GetWorldSpace();
    TESObjectCELL* ParentCell = a1->GetParentCell();
    TESObjectREFR* pObjectRef =
        pDataHandler->CreateReferenceAtLocation(a2, point, angle, ParentCell, WorldSpace, a4, 0, 0);
    if (a3 > 1) {
        (&pObjectRef->m_Extra)->SetCount(a3);
    }
    pObjectRef->AddChange(1024);
    return pObjectRef;
}

TESBoundObject* InventoryChanges::GetLeveledCalculatedBaseObject(TESLevItem* apLevelItem) {
    TESBoundObject* pObject = 0;
    if (apLevelItem) {
        BSSimpleList<ContainerObject*>* pList = &GetOriginalContainer()->objectList;
        int32_t index = 0;
        while (pList) {
            if (pList->IsEmpty()) {
                break;
            }
            TESLevItem* pObj = dynamic_cast<TESLevItem*>(pList->m_item->pObj);
            if (pObj) {
                if (pObj == apLevelItem) {
                    break;
                }
            }
            if (pObj) {
                ++index;
            }
            pList = pList->m_pkNext;
        }
        for (BSSimpleList<ItemChange*>* pInvList = pListofChanges;
             pInvList && (!pInvList->IsEmpty() && pInvList->m_item); pInvList = pInvList->m_pkNext) {
            ItemChange* pitem = pInvList->m_item;
            if (pitem) {
                for (ExtraDataList* pextra : pitem->pExtraObjectList->ItemsUntilNull()) {
                    if (pextra && pextra->GetExtraLevelItemIndex() == index) {
                        pObject = pitem->pContainerObj;
                        break;
                    }
                }
            }
        }
    }
    return pObject;
}

void InventoryChanges::CleanUpBadData() {
    TESBoundObject* pObj = 0;
    BSSimpleList<ContainerObject*>* pList = &GetOriginalContainer()->objectList;
    while (pList && !pList->IsEmpty()) {
        pObj = pList->m_item->pObj;
        TESLevItem* pleveled = dynamic_cast<TESLevItem*>(pList->m_item->pObj);
        TESBipedModelForm* pbipedform = TESBipedModelForm::GetFormAsBipedModel(pObj);
        if (pObj && (!pleveled && (!pbipedform || pbipedform->GetPlayable()))) {
            ItemChange* pitem = GetObjectInList(pObj, 1, 0);
            if (pitem) {
                TESHealthForm* phealth = dynamic_cast<TESHealthForm*>(pitem->pContainerObj);
                if (phealth) {
                    if (pitem->pExtraObjectList) {
                        BSSimpleList<ExtraDataList*>* plist = pitem->pExtraObjectList;
                        while (plist && plist->m_item) {
                            double Health;
                            ExtraDataList* pext = plist->m_item;
                            if (pext->GetHealth() > -1.0 &&
                                (Health = pext->GetHealth(), Health == static_cast<double>(phealth->GetFormHealth()))) {
                                pext->RemoveHealth();
                                int32_t icount = pext->GetCount();
                                pext->RemoveCount();
                                if (pext->IsExtraListEmpty()) {
                                    plist->Remove(pext);
                                    if (pext) {
                                        delete pext;
                                    }
                                    plist = pitem->pExtraObjectList;
                                }
                                else {
                                    pext->SetCount(icount);
                                    plist = plist->m_pkNext;
                                }
                            }
                            else {
                                plist = plist->m_pkNext;
                            }
                        }
                    }
                }
            }
            if (pitem) {
                if (TESContainer::IsGold(pObj) && pRef == pPlayer) {
                    if (pitem->pExtraObjectList) {
                        BSSimpleList<ExtraDataList*>* plist = pitem->pExtraObjectList;
                        while (plist && plist->m_item) {
                            ExtraDataList* pext = plist->m_item;
                            plist->RemoveHead();
                            if (pext) {
                                delete pext;
                            }
                        }
                    }
                }
            }
            if (pitem) {
                if (pitem->GetScript()) {
                    for (BSSimpleList<ExtraDataList*>* plist = pitem->pExtraObjectList; plist && plist->m_item;
                         plist = plist->m_pkNext) {
                        ExtraDataList* pext = plist->m_item;
                        if (pext) {
                            if (pext->GetScript()) {
                                BSSimpleList<ExtraDataList*>* plist2 = plist->m_pkNext;
                                while (plist2 && plist2->m_item) {
                                    ExtraDataList* pextra2 = plist2->m_item;
                                    if (pext == pextra2) {
                                        TESObjectREFR* pref;
                                        ExtraDataList* pextra;
                                        plist2->RemoveHead();
                                        ExtraDataList* apMemory =
                                            static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
                                        if (apMemory) {
                                            pextra = ::new (apMemory) ExtraDataList;
                                        }
                                        else {
                                            pextra = 0;
                                        }
                                        ExtraDataList* pextra_2 = pextra;
                                        Script* Script_ = pext->GetScript();
                                        pextra_2->SetScript(Script_);
                                        Script* v2 = pextra_2->GetScript();
                                        ScriptLocals* v3 = v2->CloneLocals();
                                        pextra_2->SetScriptLocals(v3);
                                        TESObjectREFR* v70 =
                                            static_cast<TESObjectREFR*>(ourMalloc(sizeof(TESObjectREFR)));
                                        if (v70) {
                                            pref = ::new (v70) TESObjectREFR;
                                        }
                                        else {
                                            pref = 0;
                                        }
                                        pref->SetTemporary();
                                        ScriptLocals* ScriptLocals_ = pextra_2->GetScriptLocals();
                                        Script* v4 = pext->GetScript();
                                        v4->Run(pref, ScriptLocals_, 0, 0);
                                        plist2->AddTail(pextra_2);
                                        plist2 = plist->m_pkNext;
                                    }
                                    else {
                                        plist2 = plist2->m_pkNext;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            Script* pScript = TESScriptableForm::GetFormScript(pObj);
            if (pitem || !pScript) {
                if (pitem && (pitem->iNumber >= 0 && pScript)) {
                    BSSimpleList<ExtraDataList*>* plist = pitem->pExtraObjectList;
                    if (!plist || plist->IsEmpty()) {
                        goto LABEL_54;
                    }
                    int32_t iNumber = pitem->iNumber;
                    int32_t iadd = iNumber - plist->ItemsInList();
                    while (plist && plist->m_item) {
                        ExtraDataList* pext = plist->m_item;
                        if (pext) {
                            if (!pext->GetScript()) {
                                TESObjectREFR* pref_2;
                                pext->SetScript(pScript);
                                Script* v6 = pext->GetScript();
                                ScriptLocals* v7 = v6->CloneLocals();
                                pext->SetScriptLocals(v7);
                                TESObjectREFR* v69 = static_cast<TESObjectREFR*>(ourMalloc(sizeof(TESObjectREFR)));
                                if (v69) {
                                    pref_2 = ::new (v69) TESObjectREFR;
                                }
                                else {
                                    pref_2 = 0;
                                }
                                TESObjectREFR* pref_3 = pref_2;
                                pref_2->SetTemporary();
                                ScriptLocals* v44 = pext->GetScriptLocals();
                                Script* v8 = pitem->GetScript();
                                v8->Run(pref_3, v44, 0, 0);
                                if (pref_3) {
                                    delete pref_3;
                                }
                            }
                        }
                        plist = plist->m_pkNext;
                    }
                    plist = pitem->pExtraObjectList;
                    for (int32_t i = 0; i < iadd; ++i) {
                        TESObjectREFR* pref_4;
                        ExtraDataList* pextra_3;
                        ExtraDataList* v68 = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
                        if (v68) {
                            pextra_3 = ::new (v68) ExtraDataList;
                        }
                        else {
                            pextra_3 = 0;
                        }
                        ExtraDataList* pextra_4 = pextra_3;
                        Script* v9 = pitem->GetScript();
                        pextra_4->SetScript(v9);
                        Script* v10 = pextra_4->GetScript();
                        ScriptLocals* v11 = v10->CloneLocals();
                        pextra_4->SetScriptLocals(v11);
                        TESObjectREFR* v67 = static_cast<TESObjectREFR*>(ourMalloc(sizeof(TESObjectREFR)));
                        if (v67) {
                            pref_4 = ::new (v67) TESObjectREFR;
                        }
                        else {
                            pref_4 = 0;
                        }
                        TESObjectREFR* pref_5 = pref_4;
                        pref_4->SetTemporary();
                        ScriptLocals* v12 = pextra_4->GetScriptLocals();
                        pScript->Run(pref_5, v12, 0, 0);
                        if (pref_5) {
                            delete pref_5;
                        }
                        plist->AddHead(pextra_4);
                    }
                }
            }
            else {
            LABEL_54:;
                InitScripts();
            }
            if (pitem) {
                if (pObj->GetFormType() == AMMO_ID) {
                    // One stack slot: the worn list kept below starts as the last item the ownership loop visited.
                    ExtraDataList* pwornextra = 0;
                    if (pRef == pPlayer) {
                        BSSimpleList<ExtraDataList*>* plistextra = pitem->pExtraObjectList;
                        while (plistextra && plistextra->m_item) {
                            pwornextra = plistextra->m_item;
                            plistextra = plistextra->m_pkNext;
                            pwornextra->RemoveOwnership();
                        }
                    }
                    if (!pitem->GetWorn(0) || pitem->HasLeveledExtra()) {
                        if (pitem->GetWorn(0) && pitem->HasLeveledExtra()) {
                            if (pitem->iNumber < 0) {
                                pitem->SetWorn(0, 0, 1);
                            }
                            int32_t iCount = pitem->GetAmountNonDefaultExtra();
                            if (iCount > 1) {
                                BSSimpleList<ExtraDataList*>* plistextra_2 = pitem->pExtraObjectList;
                                while (plistextra_2 && plistextra_2->m_item) {
                                    ExtraDataList* pextraDataList = plistextra_2->m_item;
                                    plistextra_2 = plistextra_2->m_pkNext;
                                    if (pextraDataList && (pextraDataList->GetWorn(0) && pwornextra == 0)) {
                                        pwornextra = pextraDataList;
                                    }
                                    else {
                                        if (pwornextra != 0) {
                                            pwornextra->RemoveAll(1);
                                            pwornextra->DuplicateExtraListForContainer(pextraDataList);
                                            pwornextra->SetWorn(1, 0);
                                            pitem->pExtraObjectList->RemoveAll();
                                            pitem->pExtraObjectList->AddHead(pwornextra);
                                        }
                                    }
                                }
                            }
                            int32_t ExtraTotalCount = pitem->GetExtraTotalCount(0);
                            if (pitem->iNumber < ExtraTotalCount) {
                                int32_t v15 = pitem->GetExtraTotalCount(0);
                                pitem->iNumber = v15;
                            }
                        }
                        else {
                            if (pitem->GetWorn(0)) {
                                int32_t v16 = pitem->GetExtraTotalCount(0);
                                if (pitem->iNumber >= v16) {
                                    int32_t v18 = pitem->GetExtraTotalCount(0);
                                    if (pitem->iNumber > v18) {
                                        BSSimpleList<ExtraDataList*>* plistextra_3 = pitem->pExtraObjectList;
                                        while (plistextra_3 && plistextra_3->m_item) {
                                            ExtraDataList* pextraDataList_2 = plistextra_3->m_item;
                                            plistextra_3 = plistextra_3->m_pkNext;
                                            if (pextraDataList_2) {
                                                if (pextraDataList_2->GetWorn(0)) {
                                                    pextraDataList_2->SetCount(pitem->iNumber);
                                                }
                                            }
                                        }
                                    }
                                }
                                else {
                                    int32_t v17 = pitem->GetExtraTotalCount(0);
                                    pitem->iNumber = v17;
                                }
                            }
                        }
                    }
                    else {
                        if (pitem->iNumber > 0) {
                            ContainerObject* m_item = pList->m_item;
                            if (pitem->iNumber >= m_item->i_count) {
                                pitem->iNumber = 0;
                            }
                        }
                    }
                }
            }
            int32_t iamount = 0;
            int32_t iorigcount = pList->m_item->i_count;
            if (iorigcount < 0) {
                iorigcount = -iorigcount;
            }
            if (pitem) {
                int32_t v19 = pitem->iNumber;
                iamount += iorigcount + v19;
            }
            else {
                iamount += iorigcount;
            }
            if (iamount > 0) {
                if (pitem) {
                    if (pitem->pExtraObjectList) {
                        if (pitem->pExtraObjectList->ItemsInList()) {
                            BSSimpleList<ExtraDataList*>* plistextra_4 = pitem->pExtraObjectList;
                            while (1) {
                                // The compare is unsigned.
                                uint32_t v20 = plistextra_4->ItemsInList();
                                if (static_cast<uint32_t>(iamount) >= v20) {
                                    break;
                                }
                                ExtraDataList* pextraDataList_3 = plistextra_4->m_item;
                                plistextra_4->RemoveHead();
                                if (pextraDataList_3) {
                                    ExtraDataList* pnextExtraDataList = plistextra_4->m_item;
                                    if (pnextExtraDataList) {
                                        while (1) {
                                            BSExtraData* pextra_6 = pextraDataList_3->pHead;
                                            if (!pextra_6) {
                                                break;
                                            }
                                            uint8_t ExtraType = pextra_6->cEtype;
                                            if (pnextExtraDataList->GetExtraData(ExtraType)) {
                                                pextraDataList_3->RemoveExtra(pextra_6, 1);
                                            }
                                            else {
                                                pextraDataList_3->RemoveExtra(pextra_6, 0);
                                                pnextExtraDataList->AddExtra(pextra_6);
                                            }
                                        }
                                    }
                                    delete pextraDataList_3;
                                }
                            }
                            if (plistextra_4->IsEmpty()) {
                                if (pitem->pExtraObjectList) {
                                    delete pitem->pExtraObjectList;
                                }
                                pitem->pExtraObjectList = 0;
                            }
                            else {
                                plistextra_4 = pitem->pExtraObjectList;
                            }
                            if (plistextra_4) {
                                if (plistextra_4->m_item) {
                                    while (plistextra_4 && plistextra_4->m_item) {
                                        ExtraDataList* pextra_7 = plistextra_4->m_item;
                                        if (pextra_7->ItemsInList()) {
                                            plistextra_4 = plistextra_4->m_pkNext;
                                        }
                                        else {
                                            plistextra_4->RemoveHead();
                                            if (pitem->pExtraObjectList->IsEmpty()) {
                                                plistextra_4->RemoveAll();
                                                delete plistextra_4;
                                                plistextra_4 = 0;
                                                pitem->pExtraObjectList = 0;
                                            }
                                            else {
                                                plistextra_4 = pitem->pExtraObjectList;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        pList = pList->m_pkNext;
    }
    for (BSSimpleList<ItemChange*>* pListInv = pListofChanges; pListInv && !pListInv->IsEmpty();
         pListInv = pListInv->m_pkNext) {
        ItemChange* pInventoryItem = pListInv->m_item;
        int32_t ioriginalcount = 0;
        if (GetOriginalContainer()) {
            if (pInventoryItem) {
                TESBoundObject* pContainerObj = pInventoryItem->pContainerObj;
                TESContainer* OriginalContainer = GetOriginalContainer();
                ioriginalcount = OriginalContainer->GetObjectCount(pContainerObj);
            }
        }
        // The count is read before the null test of the change, as the game does.
        int32_t v23 = pInventoryItem->iNumber;
        int32_t iamount_2 = ioriginalcount + v23;
        if (pInventoryItem) {
            TESHealthForm* phealth_2 = dynamic_cast<TESHealthForm*>(pInventoryItem->pContainerObj);
            if (phealth_2) {
                if (pInventoryItem->pExtraObjectList) {
                    BSSimpleList<ExtraDataList*>* plist = pInventoryItem->pExtraObjectList;
                    while (plist && plist->m_item) {
                        double v54;
                        ExtraDataList* pext_5 = plist->m_item;
                        if (pext_5->GetHealth() > -1.0 &&
                            (v54 = pext_5->GetHealth(), v54 == static_cast<double>(phealth_2->GetFormHealth()))) {
                            pext_5->RemoveHealth();
                            int32_t icount_2 = pext_5->GetCount();
                            pext_5->RemoveCount();
                            if (pext_5->IsExtraListEmpty()) {
                                plist->Remove(pext_5);
                                if (pext_5) {
                                    delete pext_5;
                                }
                                plist = pInventoryItem->pExtraObjectList;
                            }
                            else {
                                pext_5->SetCount(icount_2);
                                plist = plist->m_pkNext;
                            }
                        }
                        else {
                            plist = plist->m_pkNext;
                        }
                    }
                }
            }
        }
        int32_t iextracount = pInventoryItem->GetExtraTotalCount(0);
        pObj = pInventoryItem->pContainerObj;
        if (pInventoryItem) {
            if (pInventoryItem->GetScript()) {
                for (BSSimpleList<ExtraDataList*>* plist_2 = pInventoryItem->pExtraObjectList;
                     plist_2 && plist_2->m_item; plist_2 = plist_2->m_pkNext) {
                    ExtraDataList* pext_6 = plist_2->m_item;
                    if (pext_6) {
                        if (pext_6->GetScript()) {
                            BSSimpleList<ExtraDataList*>* plist2_2 = plist_2->m_pkNext;
                            while (plist2_2 && plist2_2->m_item) {
                                ExtraDataList* pextra2_2 = plist2_2->m_item;
                                if (pext_6 == pextra2_2) {
                                    TESObjectREFR* pref_6;
                                    ExtraDataList* pextra_8;
                                    plist2_2->RemoveHead();
                                    ExtraDataList* v66 = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
                                    if (v66) {
                                        pextra_8 = ::new (v66) ExtraDataList;
                                    }
                                    else {
                                        pextra_8 = 0;
                                    }
                                    ExtraDataList* pnewextra = pextra_8;
                                    Script* v24 = pext_6->GetScript();
                                    pnewextra->SetScript(v24);
                                    Script* v25 = pnewextra->GetScript();
                                    ScriptLocals* v26 = v25->CloneLocals();
                                    pnewextra->SetScriptLocals(v26);
                                    TESObjectREFR* v65 = static_cast<TESObjectREFR*>(ourMalloc(sizeof(TESObjectREFR)));
                                    if (v65) {
                                        pref_6 = ::new (v65) TESObjectREFR;
                                    }
                                    else {
                                        pref_6 = 0;
                                    }
                                    TESObjectREFR* pnewref = pref_6;
                                    pref_6->SetTemporary();
                                    ScriptLocals* v45 = pnewextra->GetScriptLocals();
                                    Script* v27 = pext_6->GetScript();
                                    v27->Run(pnewref, v45, 0, 0);
                                    plist2_2->AddTail(pnewextra);
                                    plist2_2 = plist_2->m_pkNext;
                                }
                                else {
                                    plist2_2 = plist2_2->m_pkNext;
                                }
                            }
                        }
                    }
                }
            }
        }
        if (pInventoryItem) {
            if (pInventoryItem->iNumber >= 0) {
                Script* pScript_2 = TESScriptableForm::GetFormScript(pInventoryItem->pContainerObj);
                if (pScript_2) {
                    BSSimpleList<ExtraDataList*>* plist_3 = pInventoryItem->pExtraObjectList;
                    if (plist_3 && !plist_3->IsEmpty()) {
                        int32_t v28 = pInventoryItem->iNumber;
                        int32_t iadd_2 = v28 - plist_3->ItemsInList();
                        while (plist_3 && plist_3->m_item) {
                            ExtraDataList* pext_7 = plist_3->m_item;
                            if (pext_7) {
                                if (!pext_7->GetScript()) {
                                    TESObjectREFR* pref_7;
                                    pext_7->SetScript(pScript_2);
                                    Script* v29 = pext_7->GetScript();
                                    ScriptLocals* v30 = v29->CloneLocals();
                                    pext_7->SetScriptLocals(v30);
                                    TESObjectREFR* v64 = static_cast<TESObjectREFR*>(ourMalloc(sizeof(TESObjectREFR)));
                                    if (v64) {
                                        pref_7 = ::new (v64) TESObjectREFR;
                                    }
                                    else {
                                        pref_7 = 0;
                                    }
                                    pref_7->SetTemporary();
                                    ScriptLocals* v46 = pext_7->GetScriptLocals();
                                    Script* v31 = pInventoryItem->GetScript();
                                    v31->Run(pref_7, v46, 0, 0);
                                }
                            }
                            plist_3 = plist_3->m_pkNext;
                        }
                        plist_3 = pInventoryItem->pExtraObjectList;
                        for (int32_t i_2 = 0; i_2 < iadd_2; ++i_2) {
                            TESObjectREFR* pref_8;
                            ExtraDataList* pextra_9;
                            ExtraDataList* v63 = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
                            if (v63) {
                                pextra_9 = ::new (v63) ExtraDataList;
                            }
                            else {
                                pextra_9 = 0;
                            }
                            ExtraDataList* pextra_10 = pextra_9;
                            Script* v32 = pInventoryItem->GetScript();
                            pextra_10->SetScript(v32);
                            Script* v33 = pextra_10->GetScript();
                            ScriptLocals* v34 = v33->CloneLocals();
                            pextra_10->SetScriptLocals(v34);
                            TESObjectREFR* v62 = static_cast<TESObjectREFR*>(ourMalloc(sizeof(TESObjectREFR)));
                            if (v62) {
                                pref_8 = ::new (v62) TESObjectREFR;
                            }
                            else {
                                pref_8 = 0;
                            }
                            TESObjectREFR* pref_9 = pref_8;
                            pref_8->SetTemporary();
                            ScriptLocals* v35 = pextra_10->GetScriptLocals();
                            pScript_2->Run(pref_9, v35, 0, 0);
                            if (pref_9) {
                                delete pref_9;
                            }
                            plist_3->AddHead(pextra_10);
                        }
                    }
                    else {
                        InitScripts();
                    }
                }
            }
        }
        if (pInventoryItem) {
            if (pObj->GetFormType() == AMMO_ID) {
                int32_t ioriginalcount_2 = 0;
                if (GetOriginalContainer()) {
                    if (pInventoryItem) {
                        TESBoundObject* v48 = pInventoryItem->pContainerObj;
                        TESContainer* v36 = GetOriginalContainer();
                        ioriginalcount_2 = v36->GetObjectCount(v48);
                    }
                }
                if (pRef == pPlayer) {
                    BSSimpleList<ExtraDataList*>* plistextra_5 = pInventoryItem->pExtraObjectList;
                    while (plistextra_5 && plistextra_5->m_item) {
                        ExtraDataList* pextra_11 = plistextra_5->m_item;
                        plistextra_5 = plistextra_5->m_pkNext;
                        pextra_11->RemoveOwnership();
                    }
                }
                if (pInventoryItem->GetWorn(0)) {
                    BSSimpleList<ExtraDataList*>* plistextra_6 = pInventoryItem->pExtraObjectList;
                    int32_t icount_3 = 0;
                    while (plistextra_6 && plistextra_6->m_item) {
                        icount_3 += plistextra_6->m_item->GetCount();
                        plistextra_6 = plistextra_6->m_pkNext;
                    }
                    if (ioriginalcount_2 + pInventoryItem->iNumber >= icount_3) {
                        if (ioriginalcount_2 + pInventoryItem->iNumber > icount_3) {
                            BSSimpleList<ExtraDataList*>* plistextra_7 = pInventoryItem->pExtraObjectList;
                            while (plistextra_7 && plistextra_7->m_item) {
                                ExtraDataList* pextraDataList_4 = plistextra_7->m_item;
                                plistextra_7 = plistextra_7->m_pkNext;
                                if (pextraDataList_4) {
                                    if (pextraDataList_4->GetWorn(0)) {
                                        pextraDataList_4->SetCount(pInventoryItem->iNumber);
                                    }
                                }
                            }
                        }
                    }
                    else {
                        pInventoryItem->iNumber = icount_3 - (ioriginalcount_2 + pInventoryItem->iNumber);
                    }
                }
            }
        }
        if (pInventoryItem) {
            if (pInventoryItem->pExtraObjectList) {
                for (ExtraDataList* pext_8 : pInventoryItem->pExtraObjectList->ItemsUntilNull()) {
                    if (pext_8) {
                        if (pext_8->GetScript()) {
                            pext_8->RemoveCount();
                        }
                    }
                }
            }
        }
        if (pInventoryItem) {
            if (TESContainer::IsGold(pObj) && pRef == pPlayer) {
                if (pInventoryItem->pExtraObjectList) {
                    BSSimpleList<ExtraDataList*>* plist_5 = pInventoryItem->pExtraObjectList;
                    while (plist_5 && plist_5->m_item) {
                        ExtraDataList* pext_9 = plist_5->m_item;
                        plist_5->RemoveHead();
                        if (pext_9) {
                            delete pext_9;
                        }
                    }
                }
            }
        }
        if (pInventoryItem) {
            if (iamount_2 < iextracount) {
                if (pInventoryItem->pExtraObjectList) {
                    int32_t inum = 0;
                    for (ExtraDataList*& rpItem : pInventoryItem->pExtraObjectList->ItemsUntilNull()) {
                        inum += rpItem->GetCount();
                    }
                    pInventoryItem->iNumber = inum;
                }
            }
        }
        if (pInventoryItem) {
            if (iamount_2 > 0) {
                BSSimpleList<ExtraDataList*>* plistextra_8 = pInventoryItem->pExtraObjectList;
                if (plistextra_8) {
                    if (plistextra_8->m_item) {
                        while (1) {
                            // The compare is unsigned.
                            uint32_t v37 = plistextra_8->ItemsInList();
                            if (static_cast<uint32_t>(iamount_2) >= v37) {
                                break;
                            }
                            ExtraDataList* pextraDataList_5 = plistextra_8->m_item;
                            plistextra_8->RemoveHead();
                            if (pextraDataList_5) {
                                ExtraDataList* pnextExtraDataList_2 = plistextra_8->m_item;
                                if (pnextExtraDataList_2) {
                                    while (1) {
                                        BSExtraData* pextra_12 = pextraDataList_5->pHead;
                                        if (!pextra_12) {
                                            break;
                                        }
                                        uint8_t v38 = pextra_12->cEtype;
                                        if (pnextExtraDataList_2->GetExtraData(v38)) {
                                            pextraDataList_5->RemoveExtra(pextra_12, 1);
                                        }
                                        else {
                                            pextraDataList_5->RemoveExtra(pextra_12, 0);
                                            pnextExtraDataList_2->AddExtra(pextra_12);
                                        }
                                    }
                                }
                                delete pextraDataList_5;
                            }
                        }
                        if (plistextra_8->IsEmpty()) {
                            if (pInventoryItem->pExtraObjectList) {
                                delete pInventoryItem->pExtraObjectList;
                            }
                            pInventoryItem->pExtraObjectList = 0;
                            goto LABEL_296;
                        }
                    LABEL_295:;
                        plistextra_8 = pInventoryItem->pExtraObjectList;
                    LABEL_296:;
                        while (plistextra_8 && plistextra_8->m_item) {
                            if (plistextra_8->m_item->ItemsInList()) {
                                plistextra_8 = plistextra_8->m_pkNext;
                            }
                            else {
                                plistextra_8->RemoveHead();
                                if (!pInventoryItem->pExtraObjectList->IsEmpty()) {
                                    goto LABEL_295;
                                }
                                plistextra_8->RemoveAll();
                                delete plistextra_8;
                                plistextra_8 = 0;
                                pInventoryItem->pExtraObjectList = 0;
                            }
                        }
                    }
                }
            }
        }
    }
}

// Each extra list is copied with two plain locals: the new list (its address passed to AddTail) and the source node.
void InventoryChanges::DuplicateAllItems(TESObjectREFR* apRef, TESObjectREFR* apDupContainer) {
    if (apDupContainer) {
        if (apRef) {
            TESBoundObject* pObj = 0;
            ItemChange* pNewItem = 0;
            InventoryChanges* pdupchange = InventoryChanges::GetInventoryChanges(apDupContainer);
            if (pdupchange) {
                BSSimpleList<ContainerObject*>* pList = &GetOriginalContainer()->objectList;
                TESLevItem* pleveled = 0;
                while (pList && !pList->IsEmpty()) {
                    pObj = pList->m_item->pObj;
                    pleveled = dynamic_cast<TESLevItem*>(pList->m_item->pObj);
                    if (pObj) {
                        TESBoundObject* v14 = pObj;
                        TESContainer* OriginalContainer = GetOriginalContainer();
                        int32_t ioriginalcount = OriginalContainer->GetObjectCount(v14);
                        ItemChange* pitem = GetObjectInList(pObj, 1, 0);
                        int32_t iamount = 0;
                        int32_t iorigcount = ioriginalcount;
                        if (ioriginalcount < 0) {
                            iorigcount = -iorigcount;
                        }
                        if (pitem) {
                            uint32_t FormHealth_0 = pitem->iNumber;
                            iamount += iorigcount + FormHealth_0;
                        }
                        else {
                            iamount += iorigcount;
                        }
                        if (iamount > 0) {
                            TESForm* pNewForm = 0;
                            if (TESScriptableForm::GetFormScript(pObj)) {
                                ItemChange* pNewItem_2;
                                uint8_t FormType = pObj->GetFormType();
                                pNewForm = TESDataHandler::CreateFormOfType(FormType);
                                pNewForm->Copy(pObj);
                                TESScriptableForm::SetFormScript(pNewForm, 0);
                                pDataHandler->AddFormToDataHandler(pNewForm);
                                pSaveLoadGameOLD->AddCreatedBaseObject(pNewForm);
                                int32_t icount = ioriginalcount;
                                if (pitem) {
                                    uint32_t v6 = pitem->iNumber;
                                    icount += v6;
                                }
                                ItemChange* apMemory = static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
                                if (apMemory) {
                                    pNewItem_2 =
                                        ::new (apMemory) ItemChange(static_cast<TESBoundObject*>(pNewForm), icount);
                                }
                                else {
                                    pNewItem_2 = 0;
                                }
                                pNewItem = pNewItem_2;
                                if (pitem) {
                                    if (pitem->pExtraObjectList) {
                                        if (pitem->pExtraObjectList->ItemsInList()) {
                                            BSSimpleList<ExtraDataList*>* pnode = pitem->pExtraObjectList;
                                            if (pnode) {
                                                if (pnode->m_item) {
                                                    while (pnode && pnode->m_item) {
                                                        ExtraDataList* pextra;
                                                        ExtraDataList* v31 = static_cast<ExtraDataList*>(
                                                            ourMalloc(sizeof(ExtraDataList))
                                                        );
                                                        if (v31) {
                                                            pextra = ::new (v31) ExtraDataList;
                                                        }
                                                        else {
                                                            pextra = 0;
                                                        }
                                                        BSExtraData* apExtra =
                                                            static_cast<BaseExtraList*>(pnode->m_item)
                                                                ->GetExtraData(EXTRA_SCRIPT);
                                                        if (apExtra) {
                                                            static_cast<BaseExtraList*>(pnode->m_item)
                                                                ->RemoveExtra(apExtra, 0);
                                                        }
                                                        pextra->CopyListForContainer(pnode->m_item, 1);
                                                        pNewItem->pExtraObjectList->AddTail(pextra);
                                                        if (apExtra) {
                                                            static_cast<BaseExtraList*>(pnode->m_item)
                                                                ->AddExtra(apExtra);
                                                        }
                                                        pnode = pnode->m_pkNext;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            else {
                                ItemChange* pNewItem_3;
                                ItemChange* v30 = static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
                                if (v30) {
                                    pNewItem_3 = ::new (v30) ItemChange(pObj, iamount);
                                }
                                else {
                                    pNewItem_3 = 0;
                                }
                                pNewItem = pNewItem_3;
                                if (pitem) {
                                    if (pitem->pExtraObjectList) {
                                        if (pitem->pExtraObjectList->ItemsInList()) {
                                            BSSimpleList<ExtraDataList*>* pnode = pitem->pExtraObjectList;
                                            if (pnode) {
                                                if (pnode->m_item) {
                                                    while (pnode && pnode->m_item) {
                                                        ExtraDataList* pextra_2;
                                                        ExtraDataList* v29 = static_cast<ExtraDataList*>(
                                                            ourMalloc(sizeof(ExtraDataList))
                                                        );
                                                        if (v29) {
                                                            pextra_2 = ::new (v29) ExtraDataList;
                                                        }
                                                        else {
                                                            pextra_2 = 0;
                                                        }
                                                        pextra_2->CopyListForContainer(pnode->m_item, 1);
                                                        pNewItem->pExtraObjectList->AddTail(pextra_2);
                                                        pnode = pnode->m_pkNext;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            pdupchange->AddChangeItem(pNewItem, 1);
                        }
                    }
                    pList = pList->m_pkNext;
                }
                for (BSSimpleList<ItemChange*>* pListInv = pListofChanges; pListInv && !pListInv->IsEmpty();
                     pListInv = pListInv->m_pkNext) {
                    ItemChange* pInventoryItem = pListInv->m_item;
                    if (pInventoryItem) {
                        if (pInventoryItem->iNumber > 0) {
                            pObj = pInventoryItem->pContainerObj;
                            pInventoryItem->RemoveLeveledItemExtra();
                            TESForm* pNewForm_2 = 0;
                            if (pInventoryItem->GetScript()) {
                                ItemChange* pNewItem_4;
                                uint8_t v7 = pInventoryItem->pContainerObj->GetFormType();
                                pNewForm_2 = TESDataHandler::CreateFormOfType(v7);
                                TESBoundObject* FormFlags = pInventoryItem->pContainerObj;
                                pNewForm_2->Copy(FormFlags);
                                TESScriptableForm::SetFormScript(pNewForm_2, 0);
                                pDataHandler->AddFormToDataHandler(pNewForm_2);
                                pSaveLoadGameOLD->AddCreatedBaseObject(pNewForm_2);
                                ItemChange* v28 = static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
                                if (v28) {
                                    uint32_t v8 = pInventoryItem->iNumber;
                                    pNewItem_4 = ::new (v28) ItemChange(static_cast<TESBoundObject*>(pNewForm_2), v8);
                                }
                                else {
                                    pNewItem_4 = 0;
                                }
                                pNewItem = pNewItem_4;
                                if (pInventoryItem) {
                                    if (pInventoryItem->pExtraObjectList) {
                                        if (pInventoryItem->pExtraObjectList->ItemsInList()) {
                                            BSSimpleList<ExtraDataList*>* pnode = pInventoryItem->pExtraObjectList;
                                            if (pnode) {
                                                if (pnode->m_item) {
                                                    while (pnode && pnode->m_item) {
                                                        ExtraDataList* pextra_3;
                                                        ExtraDataList* v27 = static_cast<ExtraDataList*>(
                                                            ourMalloc(sizeof(ExtraDataList))
                                                        );
                                                        if (v27) {
                                                            pextra_3 = ::new (v27) ExtraDataList;
                                                        }
                                                        else {
                                                            pextra_3 = 0;
                                                        }
                                                        BSExtraData* ExtraData =
                                                            static_cast<BaseExtraList*>(pnode->m_item)
                                                                ->GetExtraData(EXTRA_SCRIPT);
                                                        if (ExtraData) {
                                                            static_cast<BaseExtraList*>(pnode->m_item)
                                                                ->RemoveExtra(ExtraData, 0);
                                                        }
                                                        pextra_3->CopyListForContainer(pnode->m_item, 1);
                                                        pNewItem->pExtraObjectList->AddTail(pextra_3);
                                                        if (ExtraData) {
                                                            static_cast<BaseExtraList*>(pnode->m_item)
                                                                ->AddExtra(ExtraData);
                                                        }
                                                        pnode = pnode->m_pkNext;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            else {
                                ItemChange* pNewItem_5;
                                ItemChange* v26 = static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
                                if (v26) {
                                    uint32_t v9 = pInventoryItem->iNumber;
                                    pNewItem_5 = ::new (v26) ItemChange(pInventoryItem->pContainerObj, v9);
                                }
                                else {
                                    pNewItem_5 = 0;
                                }
                                pNewItem = pNewItem_5;
                                if (pInventoryItem) {
                                    if (pInventoryItem->pExtraObjectList) {
                                        if (pInventoryItem->pExtraObjectList->ItemsInList()) {
                                            BSSimpleList<ExtraDataList*>* pnode = pInventoryItem->pExtraObjectList;
                                            if (pnode) {
                                                if (pnode->m_item) {
                                                    while (pnode && pnode->m_item) {
                                                        ExtraDataList* pextra_4;
                                                        ExtraDataList* v25 = static_cast<ExtraDataList*>(
                                                            ourMalloc(sizeof(ExtraDataList))
                                                        );
                                                        if (v25) {
                                                            pextra_4 = ::new (v25) ExtraDataList;
                                                        }
                                                        else {
                                                            pextra_4 = 0;
                                                        }
                                                        pextra_4->CopyListForContainer(pnode->m_item, 1);
                                                        pNewItem->pExtraObjectList->AddTail(pextra_4);
                                                        pnode = pnode->m_pkNext;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            pdupchange->AddChangeItem(pNewItem, 1);
                            pNewItem = 0;
                        }
                    }
                }
            }
        }
    }
}

float InventoryChanges::RemoveAllTypedItems(
    TESObjectREFR* apRef,
    TESObjectREFR* apStolenContainer,
    bool abStealing,
    bool abowner,
    bool abRemoveOwnership,
    bool abSilent,
    int32_t aiFormType,
    BGSListForm* apDoNotRemoveList
) {
    return RemoveAllItems(
        apRef, apStolenContainer, abStealing, abowner, abRemoveOwnership, abSilent, aiFormType, apDoNotRemoveList
    );
}

float InventoryChanges::GetEquippedWeight(Actor* apOwner) {
    float parmor;
    float famount = 0.0f;
    float v15 = 0.0f;
    for (BSSimpleList<ItemChange*>* pInvList = pListofChanges; pInvList && (!pInvList->IsEmpty() && pInvList->m_item);
         pInvList = pInvList->m_pkNext) {
        ItemChange* pitem = pInvList->m_item;
        TESObject* pObj = pitem->pContainerObj;
        if (pitem->GetWorn(0) && pObj->GetFormType() != AMMO_ID) {
            float fweight;
            if (pObj->GetFormType() == WEAP_ID) {
                bool HasModEffectActive_0 = pitem->HasModEffectActive(4);
                fweight = static_cast<TESObjectWEAP*>(pObj)->TESObjectWEAP::GetFormWeight(HasModEffectActive_0);
                if (fweight >= 10.0) {
                    parmor = 1.0f;
                    BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::NOT_YET_IMPLEMENTED, pPlayer, &parmor);
                    fweight = fweight * parmor;
                }
            }
            else {
                bool IsHardcore = pPlayer->IsHardcore();
                fweight = TESWeightForm::GetFormWeight(pObj, IsHardcore);
            }
            int32_t icount = GetObjectCount(pitem->pContainerObj);
            if (pObj->GetFormType() == ARMO_ID) {
                FNVD_DYNAMIC_CAST_VOID(TESObjectARMO)(pObj, RTTI_TESObject, RTTI_TESObjectARMO);
            }
            else {
                if (pObj->GetFormType() == WEAP_ID) {
                    BaseProcess* pprocess = apOwner->pCurrentProcess;
                    if (pprocess) {
                        ItemChange* pweap = pprocess->GetCurrentWeapon();
                        ItemChange* pammo = pprocess->GetCurrentAmmo();
                        if (pweap) {
                            if (pammo) {
                                TESBoundObject* FormFlags = pweap->pContainerObj;
                                if (FormFlags == pammo->pContainerObj) {
                                    icount = 1;
                                }
                            }
                        }
                    }
                }
            }
            famount = static_cast<double>(icount) * fweight + famount;
        }
    }
    return famount;
}

void InventoryChanges::CreateCopyofList(InventoryChanges* apCopy, TESObjectREFR* apRef, bool bdontcopyquestitems) {
    ExtraDataList* pcurextra_3;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_INVENTORY, 1, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp", 9911
    );
    BSSimpleList<ContainerObject*>* plist = &GetOriginalContainer()->objectList;
    ItemChange* preturnitem = 0;
    while (plist && plist->m_item) {
        ItemChange* preturnitem_2;
        ContainerObject* pobject = plist->m_item;
        TESLevItem* pObj = dynamic_cast<TESLevItem*>(pobject->pObj);
        ItemChange* v57 = static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
        if (v57) {
            preturnitem_2 = ::new (v57) ItemChange;
        }
        else {
            preturnitem_2 = 0;
        }
        preturnitem = preturnitem_2;
        TESBoundObject* v25 = pobject->pObj;
        TESContainer* OriginalContainer = GetOriginalContainer();
        int32_t iOriginalcount = OriginalContainer->GetObjectCount(v25);
        if (!preturnitem->pExtraObjectList) {
            BSSimpleList<ExtraDataList*>* v42;
            BSSimpleList<ExtraDataList*>* v56 =
                static_cast<BSSimpleList<ExtraDataList*>*>(ourMalloc(sizeof(BSSimpleList<ExtraDataList*>)));
            if (v56) {
                v42 = ::new (v56) BSSimpleList<ExtraDataList*>;
            }
            else {
                v42 = 0;
            }
            preturnitem->pExtraObjectList = v42;
        }
        if (!pobject || pObj) {
            if (preturnitem) {
                delete preturnitem;
            }
            preturnitem = 0;
        }
        else {
            TESBoundObject* FormFlags;
            ItemChange* pitem = GetObjectInList(pobject->pObj, 1, 0);
            if (pitem) {
                pitem->RemoveOriginalChanges();
            }
            if (pobject &&
                (pitem &&
                 (((!pitem->HasLeveledExtra() || pitem->CountLeveledItems() < 1) &&
                   !(iOriginalcount + pitem->iNumber)) ||
                  (((FormFlags = pitem->pContainerObj, FormFlags->GetQuestObject()) && bdontcopyquestitems) ||
                   pitem->GetWorn(0))))) {
                if (preturnitem) {
                    delete preturnitem;
                }
                preturnitem = 0;
            }
            else {
                if (!pobject || (pitem && pitem->GetExtraTotalCount(0))) {
                    if (pitem) {
                        int32_t inumb = 0;
                        if (pitem->pExtraObjectList) {
                            inumb = pitem->GetExtraTotalCount(0);
                        }
                        // The loop runs only while preturnitem is null, and then uses it, as the game does.
                        for (BSSimpleList<ExtraDataList*>* pextralist = pitem->pExtraObjectList;
                             pextralist && (pextralist->m_item && !preturnitem); pextralist = pextralist->m_pkNext) {
                            pcurextra_3 = pextralist->m_item;
                            preturnitem->pContainerObj = pitem->pContainerObj;
                            int32_t Count = pcurextra_3->GetCount();
                            preturnitem->iNumber = Count;
                            if (!preturnitem->pExtraObjectList) {
                                BSSimpleList<ExtraDataList*>* v39;
                                BSSimpleList<ExtraDataList*>* v54 = static_cast<BSSimpleList<ExtraDataList*>*>(
                                    ourMalloc(sizeof(BSSimpleList<ExtraDataList*>))
                                );
                                if (v54) {
                                    v39 = ::new (v54) BSSimpleList<ExtraDataList*>;
                                }
                                else {
                                    v39 = 0;
                                }
                                preturnitem->pExtraObjectList = v39;
                            }
                            pcurextra_3->SetOriginalReferenceExtra(apRef);
                            preturnitem->pExtraObjectList->AddHead(pcurextra_3);
                            if (!preturnitem->GetWorn(0)) {
                                apCopy->AddChangeItem(preturnitem, 1);
                            }
                        }
                        int32_t ExtraTotalCount = pitem->GetExtraTotalCount(0);
                        if (ExtraTotalCount < pitem->iNumber + iOriginalcount) {
                            if (!preturnitem->pExtraObjectList) {
                                BSSimpleList<ExtraDataList*>* v38;
                                BSSimpleList<ExtraDataList*>* v53 = static_cast<BSSimpleList<ExtraDataList*>*>(
                                    ourMalloc(sizeof(BSSimpleList<ExtraDataList*>))
                                );
                                if (v53) {
                                    v38 = ::new (v53) BSSimpleList<ExtraDataList*>;
                                }
                                else {
                                    v38 = 0;
                                }
                                preturnitem->pExtraObjectList = v38;
                            }
                            if (pitem) {
                                int32_t v9 = pitem->iNumber + iOriginalcount;
                                int32_t v10 = pitem->GetExtraTotalCount(0);
                                preturnitem->iNumber = v9 - v10;
                            }
                            else {
                                preturnitem->iNumber = iOriginalcount;
                            }
                            preturnitem->pContainerObj = pobject->pObj;
                            if (preturnitem->GetWorn(0)) {
                                if (preturnitem) {
                                    delete preturnitem;
                                }
                                preturnitem = 0;
                            }
                            else {
                                ExtraDataList* pcurextra_4;
                                ExtraDataList* v52 = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
                                if (v52) {
                                    pcurextra_4 = ::new (v52) ExtraDataList;
                                }
                                else {
                                    pcurextra_4 = 0;
                                }
                                ExtraDataList* pcurextra_5 = pcurextra_4;
                                pcurextra_4->SetOriginalReferenceExtra(apRef);
                                int16_t FormHealth_0 = preturnitem->iNumber;
                                pcurextra_5->SetCount(FormHealth_0);
                                preturnitem->pExtraObjectList->AddHead(pcurextra_5);
                                apCopy->AddChangeItem(preturnitem, 1);
                            }
                        }
                    }
                    else {
                        if (preturnitem) {
                            delete preturnitem;
                        }
                        preturnitem = 0;
                    }
                }
                else {
                    int32_t icount = iOriginalcount;
                    if (iOriginalcount < 0) {
                        icount = -icount;
                    }
                    if (pitem) {
                        int32_t v5 = pitem->iNumber + icount;
                        int32_t v6 = pitem->GetExtraTotalCount(0);
                        preturnitem->iNumber = v5 - v6;
                    }
                    else {
                        preturnitem->iNumber = icount;
                    }
                    preturnitem->pContainerObj = pobject->pObj;
                    if (pitem && pitem->GetScript()) {
                        for (BSSimpleList<ExtraDataList*>* pextlist = pitem->pExtraObjectList;
                             pextlist && pextlist->m_item; pextlist = pextlist->m_pkNext) {
                            pextlist->m_item->SetOriginalReferenceExtra(apRef);
                            BSSimpleList<ExtraDataList*>* v26 = pextlist;
                            preturnitem->pExtraObjectList->AddTail(v26->m_item);
                        }
                        apCopy->AddChangeItem(preturnitem, 1);
                    }
                    else {
                        if (preturnitem->GetWorn(0)) {
                            if (preturnitem) {
                                delete preturnitem;
                            }
                            preturnitem = 0;
                        }
                        else {
                            ExtraDataList* pcurextra;
                            ExtraDataList* v55 = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
                            if (v55) {
                                pcurextra = ::new (v55) ExtraDataList;
                            }
                            else {
                                pcurextra = 0;
                            }
                            ExtraDataList* pcurextra_2 = pcurextra;
                            pcurextra->SetOriginalReferenceExtra(apRef);
                            int16_t v7 = preturnitem->iNumber;
                            pcurextra_2->SetCount(v7);
                            preturnitem->pExtraObjectList->AddHead(pcurextra_2);
                            apCopy->AddChangeItem(preturnitem, 1);
                        }
                    }
                }
            }
        }
        plist = plist->m_pkNext;
    }
    for (ItemChange*& rpItem : pListofChanges->ItemsUntilNull()) {
        TESContainer* v12;
        TESBoundObject* pContainerObj;
        ItemChange* preturnitem_3;
        ItemChange* v51 = static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
        if (v51) {
            preturnitem_3 = ::new (v51) ItemChange;
        }
        else {
            preturnitem_3 = 0;
        }
        preturnitem = preturnitem_3;
        if (!preturnitem_3->pExtraObjectList) {
            BSSimpleList<ExtraDataList*>* v35;
            BSSimpleList<ExtraDataList*>* v50 =
                static_cast<BSSimpleList<ExtraDataList*>*>(ourMalloc(sizeof(BSSimpleList<ExtraDataList*>)));
            if (v50) {
                v35 = ::new (v50) BSSimpleList<ExtraDataList*>;
            }
            else {
                v35 = 0;
            }
            preturnitem->pExtraObjectList = v35;
        }
        ItemChange* pitem_2 = rpItem;
        if (!pitem_2 ||
            (pitem_2->iNumber <= 0 ||
             (GetOriginalContainer() &&
              (pContainerObj = pitem_2->pContainerObj,
               (v12 = GetOriginalContainer(), v12->HasObject(pContainerObj)))))) {
            if (preturnitem) {
                delete preturnitem;
            }
            preturnitem = 0;
        }
        else {
            if (pitem_2->GetExtraTotalCount(0) || pitem_2->GetScript()) {
                BSSimpleList<ExtraDataList*>* pextralist = pitem_2->pExtraObjectList;
                int32_t icount_2 = 0;
                bool leveledlist = 0;
                while (pextralist && pextralist->m_item) {
                    if (pextralist->m_item->GetExtraData(47)) {
                        leveledlist = 1;
                    }
                    pextralist = pextralist->m_pkNext;
                }
                pextralist = pitem_2->pExtraObjectList;
                if (leveledlist && pitem_2->GetTypeforMenu() == 4) {
                    if (pitem_2->GetTypeforMenu() == 4) {
                        if (pitem_2->GetScript()) {
                            pitem_2->iNumber = 1;
                        }
                    }
                    while (pextralist && pextralist->m_item) {
                        ExtraDataList* pcurextra_7 = pextralist->m_item;
                        pcurextra_7->RemoveCount();
                        pcurextra_7->SetOriginalReferenceExtra(apRef);
                        preturnitem->pExtraObjectList->AddHead(pcurextra_7);
                        pextralist = pextralist->m_pkNext;
                        icount_2 = 1;
                    }
                }
                else {
                    while (pextralist && pextralist->m_item) {
                        ExtraDataList* pcurextra_6 = pextralist->m_item;
                        pcurextra_6->SetOriginalReferenceExtra(apRef);
                        preturnitem->pExtraObjectList->AddHead(pcurextra_6);
                        pextralist = pextralist->m_pkNext;
                        int16_t v13 = pcurextra_6->GetCount();
                        icount_2 += v13;
                    }
                }
                preturnitem->pContainerObj = pitem_2->pContainerObj;
                uint32_t v14 = pitem_2->iNumber;
                preturnitem->iNumber = v14;
                if (!preturnitem->GetWorn(0)) {
                    int32_t v15 = pitem_2->iNumber;
                    if (icount_2 < v15) {
                        ExtraDataList* pcurextra_8;
                        ExtraDataList* v49 = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
                        if (v49) {
                            pcurextra_8 = ::new (v49) ExtraDataList;
                        }
                        else {
                            pcurextra_8 = 0;
                        }
                        ExtraDataList* pcurextra_9 = pcurextra_8;
                        pcurextra_8->SetOriginalReferenceExtra(apRef);
                        int16_t v16 = pitem_2->iNumber;
                        pcurextra_9->SetCount(v16 - icount_2);
                        preturnitem->pExtraObjectList->AddHead(pcurextra_9);
                    }
                }
                if (preturnitem->GetWorn(0)) {
                    if (preturnitem->pExtraObjectList->m_item) {
                        preturnitem->pExtraObjectList->m_item->RemoveOriginalReferenceExtra();
                    }
                    if (preturnitem) {
                        delete preturnitem;
                    }
                    preturnitem = 0;
                }
                else {
                    apCopy->AddChangeItem(preturnitem, 1);
                }
            }
            else {
                ExtraDataList* pcurextra_10 = 0;
                if (pitem_2->pExtraObjectList) {
                    BSSimpleList<ExtraDataList*>* pextralist_2 = pitem_2->pExtraObjectList;
                    int32_t icount_3 = 0;
                    while (pextralist_2 && (pextralist_2->m_item && !pcurextra_10)) {
                        pcurextra_10 = pextralist_2->m_item;
                        int16_t v17 = pcurextra_10->GetCount();
                        icount_3 += v17;
                        if (pcurextra_10->IsExtraDefaultforContainer(0)) {
                            pcurextra_10 = 0;
                        }
                        pextralist_2 = pextralist_2->m_pkNext;
                    }
                    if (pcurextra_10) {
                        if (!preturnitem->pExtraObjectList) {
                            BSSimpleList<ExtraDataList*>* v33;
                            BSSimpleList<ExtraDataList*>* v48 = static_cast<BSSimpleList<ExtraDataList*>*>(
                                ourMalloc(sizeof(BSSimpleList<ExtraDataList*>))
                            );
                            if (v48) {
                                v33 = ::new (v48) BSSimpleList<ExtraDataList*>;
                            }
                            else {
                                v33 = 0;
                            }
                            preturnitem->pExtraObjectList = v33;
                        }
                        pcurextra_10->SetOriginalReferenceExtra(apRef);
                        preturnitem->pExtraObjectList->AddHead(pcurextra_10);
                        if (!preturnitem->GetWorn(0)) {
                            int32_t v18 = pitem_2->iNumber;
                            if (icount_3 < v18) {
                                ExtraDataList* pcurextra_11;
                                ExtraDataList* v47 = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
                                if (v47) {
                                    pcurextra_11 = ::new (v47) ExtraDataList;
                                }
                                else {
                                    pcurextra_11 = 0;
                                }
                                ExtraDataList* pcurextra_12 = pcurextra_11;
                                pcurextra_11->SetOriginalReferenceExtra(apRef);
                                int16_t v19 = pitem_2->iNumber;
                                pcurextra_12->SetCount(v19 - icount_3);
                                preturnitem->pExtraObjectList->AddHead(pcurextra_12);
                            }
                        }
                    }
                    else {
                        ExtraDataList* pcurextra_13;
                        ExtraDataList* v46 = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
                        if (v46) {
                            pcurextra_13 = ::new (v46) ExtraDataList;
                        }
                        else {
                            pcurextra_13 = 0;
                        }
                        ExtraDataList* pcurextra_14 = pcurextra_13;
                        pcurextra_13->SetOriginalReferenceExtra(apRef);
                        int16_t v20 = pitem_2->iNumber;
                        pcurextra_14->SetCount(v20);
                        preturnitem->pExtraObjectList->AddHead(pcurextra_14);
                    }
                    preturnitem->pContainerObj = pitem_2->pContainerObj;
                    uint32_t v21 = pitem_2->iNumber;
                    preturnitem->iNumber = v21;
                    if (preturnitem->GetWorn(0)) {
                        if (preturnitem->pExtraObjectList->m_item) {
                            preturnitem->pExtraObjectList->m_item->RemoveOriginalReferenceExtra();
                        }
                        if (preturnitem) {
                            delete preturnitem;
                        }
                        preturnitem = 0;
                    }
                    else {
                        uint32_t v22 = pitem_2->iNumber;
                        preturnitem->iNumber = v22;
                        apCopy->AddChangeItem(preturnitem, 1);
                    }
                }
                else {
                    ExtraDataList* pcurextra_15;
                    TESBoundObject* v29 = pitem_2->pContainerObj;
                    preturnitem->pContainerObj = v29;
                    uint32_t v23 = pitem_2->iNumber;
                    preturnitem->iNumber = v23;
                    ExtraDataList* v45 = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
                    if (v45) {
                        pcurextra_15 = ::new (v45) ExtraDataList;
                    }
                    else {
                        pcurextra_15 = 0;
                    }
                    pcurextra_10 = pcurextra_15;
                    pcurextra_15->SetOriginalReferenceExtra(apRef);
                    int16_t v24 = preturnitem->iNumber;
                    pcurextra_10->SetCount(v24);
                    preturnitem->pExtraObjectList->AddHead(pcurextra_10);
                    apCopy->AddChangeItem(preturnitem, 1);
                }
            }
        }
    }
}

void InventoryChanges::GetRepairItemGroup(TESBoundObject* apObj, BSSimpleList<ItemChange*>* apReturnList) {
    if (apObj) {
        if (apReturnList) {
            IterData* apIterator = GetIterator();
            ItemChange* a2 = 0;
            do {
                a2 = Iterate(apIterator);
                if (a2) {
                    if (BGSRepairItemList::CanItemRepairForm(a2->pContainerObj, apObj)) {
                        apReturnList->AddHead(a2);
                    }
                    else {
                        if (a2) {
                            delete a2;
                        }
                    }
                }
            }
            while (a2);
            if (apIterator) {
                delete apIterator;
            }
        }
    }
}

// The index of apExtra among the extra data lists of apObject's change, or -1 when either is null or the change or
// the list is not there.
int32_t InventoryChanges::GetExtraDataListIndexForItem(TESBoundObject* apObject, ExtraDataList* apExtra) {
    if (!apObject || !apExtra) {
        return -1;
    }
    BSSimpleList<ItemChange*>* pNode = pListofChanges;
    while (pNode) {
        ItemChange* pChange = pNode->m_item;
        pNode = pNode->m_pkNext;
        if (pChange && pChange->pContainerObj == apObject) {
            int32_t iIndex = 0;
            for (BSSimpleList<ExtraDataList*>* pExtraNode = pChange->pExtraObjectList; pExtraNode;
                 pExtraNode = pExtraNode->m_pkNext) {
                ExtraDataList* pExtra = pExtraNode->m_item;
                if (pExtra && pExtra == apExtra) {
                    return iIndex;
                }
                ++iIndex;
            }
            return -1;
        }
    }
    return -1;
}

ExtraDataList* InventoryChanges::GetExtraDataListFromIndexForItem(TESBoundObject* apObject, int32_t aiExtraIndex) {
    if (!apObject || aiExtraIndex == -1) {
        return 0;
    }
    BSSimpleList<ItemChange*>* pchangeList = pListofChanges;
    while (pchangeList) {
        ItemChange* pitemChange = pchangeList->m_item;
        pchangeList = pchangeList->m_pkNext;
        if (pitemChange && pitemChange->pContainerObj == apObject) {
            int32_t icount = 0;
            for (ExtraDataList* pextraData : *pitemChange->pExtraObjectList) {
                if (icount == aiExtraIndex) {
                    return pextraData;
                }
                ++icount;
            }
            return 0;
        }
    }
    return 0;
}

// Only references that are not mobile objects, or are actors, get changes.
InventoryChanges* InventoryChanges::GetInventoryChanges(TESObjectREFR* apRef) {
    InventoryChanges* pChanges = nullptr;
    if (!apRef->IsMobileObject() || apRef->IsActor()) {
        pChanges = apRef->GetExtra()->GetContainerChanges();
        if (!pChanges) {
            alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
            AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
            ::new (aucContext) AutoMemContext(
                MEM_CONTEXT_INVENTORY, true,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp", 1821
            );
            void* pMemory = ourMalloc(sizeof(InventoryChanges));
            pChanges = pMemory ? ::new (pMemory) InventoryChanges(apRef) : nullptr;
            apRef->GetExtra()->SetContainerChanges(pChanges);
            pContext->~AutoMemContext();
        }
    }
    return pChanges;
}

TESObjectREFR* InventoryChanges::GetTempRef() {
    return InventoryChanges::pTempRef;
}

void InventoryChanges::operator delete(void* apMemory) {
    ourFree(apMemory);
}

void InventoryChanges::KillTempRef() {
    if (InventoryChanges::pTempRef) {
        delete InventoryChanges::pTempRef;
    }
    InventoryChanges::pTempRef = 0;
}

// Whether the ammo is playable: the non-playable flag (2) of data.cFlags is clear.
bool TESAmmo::GetIsPlayable() {
    return (data.cFlags & 2) == 0;
}

// Returns whether the heavy-armor flag is set.
bool TESBipedModelForm::IsHeavy() {
    return (bipedModelData.sFlags & 0x80) != 0;
}

EnchantmentItem* TESEnchantableForm::GetFormEnchanting(TESForm* apForm) {
    auto* pEnchantable = dynamic_cast<TESEnchantableForm*>(apForm);
    return pEnchantable ? pEnchantable->GetFormEnchanting() : nullptr;
}

float TESObjectARMO::GetArmorRating() {
    return GetCalculatedArmorRating();
}

float TESObjectARMO::GetDamageThreshold() {
    return data.fDamageThreshold;
}

void TESObjectREFR::SetParentCellSimple(TESObjectCELL* apCell) {
    pParentCell = apCell;
}

// Keeps the base weight as a double until the subtraction, then rounds to float.
float TESObjectWEAP::GetFormWeight(bool bModValue) {
    if (bModValue) {
        double fWeight = static_cast<TESWeightForm*>(this)->GetWeight();
        float fResult = static_cast<float>(fWeight - GetModEffectValue(WEAPON_MOD_DECREASE_WEIGHT, 0));
        return fResult;
    }
    return static_cast<TESWeightForm*>(this)->GetWeight();
}

// Calls TESHealthForm's getter qualified, after the mod value; the sum is in double and truncated to 32 bits.
int32_t TESObjectWEAP::GetFormHealth(bool bModValue) {
    if (bModValue) {
        double fModValue = GetModEffectValue(WEAPON_MOD_MAX_HEALTH, 0);
        return static_cast<int32_t>(TESHealthForm::GetFormHealth() + fModValue);
    }
    return int32_t(TESHealthForm::GetFormHealth());
}

void TESScriptableForm::SetFormScript(Script* apScript) {
    pFormScript = apScript;
}

void TESScriptableForm::SetFormScript(TESForm* apForm, Script* apScript) {
    TESScriptableForm* pSF = dynamic_cast<TESScriptableForm*>(apForm);
    if (pSF) {
        pSF->pFormScript = apScript;
    }
}

void PlayerCharacter::QueueAttachWeapon(TESObjectWEAP* apWeapon) {
    pQueuedWeaponAttach = apWeapon;
}

void BGSSaveFormBuffer::SetForm(TESForm* apForm) {
    pForm = apForm;
}
