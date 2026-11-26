#include "Fallout Shared/InventoryChanges.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
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
#include "Fallout Shared/Settings/XGameSetting.h"
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

#include <limits>

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
    BSSimpleList<ExtraDataList*>* pNode = pExtraObjectList;
    if (pExtraObjectList && pNode->m_item) {
        ExtraDataList* pList = pNode->m_item;
        if (pList && pList->HasWeaponMods()) {
            return pList->GetWeaponModFlags();
        }
    }

    return 0;
}

void ItemChange::DeleteAllExtra() {
    for (BSSimpleList<ExtraDataList*>* pkFirst = pExtraObjectList; pkFirst && pkFirst->m_item;
         pkFirst = pkFirst->m_pkNext) {
        ExtraDataList* pExtra = pkFirst->m_item;
        pExtra->RemoveAll(true);
        pkFirst->Remove(pExtra);
        if (pExtra) {
            delete pExtra;
        }
    }
}

TESForm* ItemChange::GetItemOwnership() {
    BSSimpleList<ExtraDataList*>* pNode = pExtraObjectList;
    if (pExtraObjectList && pNode->m_item) {
        ExtraDataList* pList = pNode->m_item;
        if (pList && pList->GetOwner()) {
            return pList->GetOwner();
        }
    }

    return nullptr;
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
        ExtraDataList* pWorn;
        pWorn = new ExtraDataList;
        pWorn->SetWorn(true, ableft);
        if (!pExtraObjectList) {
            pExtraObjectList = new BSSimpleList<ExtraDataList*>;
        }
        pExtraObjectList->AddHead(pWorn);
    }
    else if (GetWorn(ableft)) {
        for (BSSimpleList<ExtraDataList*>* pkFirst = pExtraObjectList; pkFirst && pkFirst->m_item;
             pkFirst = pkFirst->m_pkNext) {
            ExtraDataList* pExtra = pkFirst->m_item;
            if (pExtra && pExtra->GetWorn(ableft)) {
                pkFirst->Remove(pExtra);
                if (abdelete && pExtra) {
                    delete pExtra;
                }

                return;
            }
        }
    }
}

void ItemChange::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    for (BSSimpleList<ExtraDataList*>* pNode = pExtraObjectList; pNode;) {
        ExtraDataList* pList = pNode->GetItem();
        if (pList) {
            pList->InitLoadGame(apLoadGameBuffer, pContainerObj);
        }
        pNode = pNode->GetNext();
    }
}

void ItemChange::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveFormID(pContainerObj, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iNumber), sizeof(iNumber), 0);
    uint32_t uiCount = 0;
    uint32_t uiPosition = apSaveGameBuffer->StartVariableSizedValue();
    for (BSSimpleList<ExtraDataList*>* pNode = pExtraObjectList; pNode;) {
        ExtraDataList* pList = pNode->GetItem();
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
        ExtraDataList* pList = pNode->GetItem();
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
        ExtraDataList* pList = pNode->GetItem();
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
    ExtraDataList* pList = pNode->GetItem();
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
    ExtraDataList* pList = pNode->GetItem();
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
    ExtraDataList* pList = pNode->GetItem();
    if (!pList) {
        return;
    }
    pList = pNode->GetItem();
    if (pList && pList->GetPoison()) {
        pList->RemovePoison();
    }
}

float ItemChange::GetItemCharge() {
    TESEnchantableForm* pEnchantable = dynamic_cast<TESEnchantableForm*>(pContainerObj);
    if (pEnchantable) {
        BSSimpleList<ExtraDataList*>* pNode = pExtraObjectList;
        if (pExtraObjectList && pNode->m_item) {
            ExtraDataList* pList = pNode->m_item;
            if (pList && pList->GetCharge() != -1.0) {
                return pList->GetCharge();
            }
        }

        return pEnchantable->iAmountofEnchantment;
    }

    return -1.0f;
}

void ItemChange::SetCount(int32_t ainumber) {
    iNumber = ainumber;
}

int32_t ItemChange::GetHotKey() {
    int32_t iValue = -1;
    for (BSSimpleList<ExtraDataList*>* pExtraList = pExtraObjectList; iValue < 0 && pExtraList && pExtraList->m_item;
         pExtraList = pExtraList->m_pkNext) {
        if (pExtraList->m_item->GetHotKey() >= 0) {
            iValue = pExtraList->m_item->GetHotKey();
        }
    }
    return iValue;
}

// Each mod's value is read through TESValueForm's getter and converted as a signed value.
float ItemChange::GetItemValue() {
    float fNearest = GetItemHealth(true);
    float fVal = static_cast<float>(TESValueForm::GetFormValue(pContainerObj));
    float fValue = GamePlayFormulas::CalcItemHealthValue(fVal, fNearest);
    TESObjectWEAP* pWeap = dynamic_cast<TESObjectWEAP*>(pContainerObj);
    if (pWeap) {
        uint8_t ucModSlots = GetModSlots();
        if (ucModSlots) {
            if ((ucModSlots & 1) != 0) {
                TESObjectIMOD* pMod = pWeap->GetModItem(1u);
                fValue = static_cast<double>(pMod->TESValueForm::GetFormValue()) + fValue;
            }
            if ((ucModSlots & 2) != 0) {
                TESObjectIMOD* pMod = pWeap->GetModItem(2u);
                fValue = static_cast<double>(pMod->TESValueForm::GetFormValue()) + fValue;
            }
            if ((ucModSlots & 4) != 0) {
                TESObjectIMOD* pMod = pWeap->GetModItem(4u);
                fValue = static_cast<double>(pMod->TESValueForm::GetFormValue()) + fValue;
            }
        }
    }
    return BSRoundToNearest(fValue, 0.1f);
}

int32_t ItemChange::GetExtraTotalCount(bool abWornIgnore) {
    BSSimpleList<ExtraDataList*>* pkFirst = pExtraObjectList;
    int32_t iCount = 0;
    while (pkFirst && pkFirst->m_item) {
        ExtraDataList* pExtra = pkFirst->m_item;
        if (!pExtra->IsExtraDefaultforContainer(false) && (abWornIgnore || !pExtra->GetWorn(false))) {
            iCount += pExtra->GetCount();
        }
        pkFirst = pkFirst->m_pkNext;
    }
    return iCount;
}

int32_t ItemChange::GetExtraTotalDefaultCount() {
    BSSimpleList<ExtraDataList*>* pkFirst = pExtraObjectList;
    int32_t iCount = 0;
    while (pkFirst && pkFirst->m_item) {
        ExtraDataList* pExtra = pkFirst->m_item;
        if (pExtra->IsExtraDefaultforContainer(false)) {
            int16_t sCount = pExtra->GetCount();
            iCount += sCount;
        }
        pkFirst = pkFirst->m_pkNext;
        if (pExtra->IsExtraListEmpty()) {
            pExtraObjectList->Remove(pExtra);
            if (pExtra) {
                delete pExtra;
            }
            iCount = 0;
            pkFirst = pExtraObjectList;
        }
    }
    return iCount;
}

Script* ItemChange::GetScript() {
    for (BSSimpleList<ExtraDataList*>* pkFirst = pExtraObjectList; pkFirst && pkFirst->m_item;
         pkFirst = pkFirst->m_pkNext) {
        ExtraDataList* pExtra = pkFirst->m_item;
        if (pExtra && pExtra->GetScript()) {
            return pExtra->GetScript();
        }
    }
    return nullptr;
}

ScriptLocals* ItemChange::GetScriptLocals() {
    for (ExtraDataList* pExtra : pExtraObjectList->ItemsUntilNull()) {
        if (pExtra && pExtra->GetScriptLocals()) {
            return pExtra->GetScriptLocals();
        }
    }
    return nullptr;
}

const char* ItemChange::GetIconOne(TESObjectREFR* apRef) {
    const char* pIcon = nullptr;
    TESForm* pOwner = GetItemOwnership();
    TESBipedModelForm* pBipedForm = dynamic_cast<TESBipedModelForm*>(pContainerObj);
    if (pOwner && pBipedForm) {
        TESActorBase* pActorBase = dynamic_cast<TESNPC*>(pOwner);
        if (pActorBase) {
            TESActorBase::SEX eSex = pActorBase->GetSex();
            pIcon = pBipedForm->GetIcon(eSex);
        }
    }

    if (!pIcon || !*pIcon) {
        pIcon = TESTexture::GetTextureName(pContainerObj, apRef);
    }
    if (pIcon && *pIcon) {
        return pIcon;
    }

    return sMissingImage.String();
}

const char* ItemChange::GetFullName() {
    TESFullName* pFull = dynamic_cast<TESFullName*>(pContainerObj);
    if (pFull && pFull->GetFullNameLength()) {
        return pFull->GetFullName();
    }

    return sMissingName.String();
}

bool ItemChange::HasModEffectActive(uint8_t eEffect) {
    BSSimpleList<ExtraDataList*>* pNode = pExtraObjectList;
    if (!pExtraObjectList) {
        return false;
    }
    if (!pNode->m_item) {
        return false;
    }
    ExtraDataList* pList = pNode->m_item;
    if (!pList) {
        return false;
    }
    TESObjectWEAP* pContainerObj = static_cast<TESObjectWEAP*>(this->pContainerObj);
    if (pList->GetWeaponModSlotActive(1u) && pContainerObj->GetModEffect(1u) == eEffect) {
        return true;
    }
    if (pList->GetWeaponModSlotActive(2u) && pContainerObj->GetModEffect(2u) == eEffect) {
        return true;
    }
    return pList->GetWeaponModSlotActive(4u) && pContainerObj->GetModEffect(4u) == eEffect;
}

bool ItemChange::HasModEffectActive(uint8_t eEffect, float& fValue) {
    BSSimpleList<ExtraDataList*>* pNode = pExtraObjectList;
    if (!pExtraObjectList) {
        return false;
    }
    if (!pNode->m_item) {
        return false;
    }
    ExtraDataList* xDataList = pNode->m_item;
    if (!xDataList) {
        return false;
    }
    TESObjectWEAP* pContainerObj = static_cast<TESObjectWEAP*>(this->pContainerObj);
    if (xDataList->GetWeaponModSlotActive(1u) &&
        static_cast<uint32_t>(pContainerObj->GetModEffect(1)) == static_cast<uint32_t>(eEffect)) {
        fValue = static_cast<float>(pContainerObj->GetModValue(1, 0));
        return true;
    }
    if (xDataList->GetWeaponModSlotActive(2u) &&
        static_cast<uint32_t>(pContainerObj->GetModEffect(2)) == static_cast<uint32_t>(eEffect)) {
        fValue = static_cast<float>(pContainerObj->GetModValue(2, 0));
        return true;
    }
    if (!xDataList->GetWeaponModSlotActive(4u) ||
        static_cast<uint32_t>(pContainerObj->GetModEffect(4)) != static_cast<uint32_t>(eEffect)) {
        return false;
    }
    fValue = static_cast<float>(pContainerObj->GetModValue(4, 0));
    return true;
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
        TESObjectWEAP* pWeapon = static_cast<TESObjectWEAP*>(pContainerObj);
        double dHealth = GetItemHealth(false);
        float fCondition = dHealth / static_cast<double>(TESHealthForm::GetFormHealth(pWeapon));
        return CombatFormulas::CalcWeaponDamage(
            apActor,
            pWeapon,
            fCondition,
            afDamageMultiplier,
            false,
            abIgnoreExplosion,
            this,
            true
        );
    }

    return 0.0;
}

float ItemChange::GetModifiedDamageThreshold(ActorValueOwner* apActor) {
    float fDT = -1.0f;
    if (pContainerObj->GetFormType() == ARMO_ID) {
        TESObjectARMO* pArmor = static_cast<TESObjectARMO*>(pContainerObj);
        float fHealth = static_cast<float>(TESHealthForm::GetFormHealth(pArmor));
        double dHealthRatio;
        if (fHealth == 0.0) {
            dHealthRatio = 0.0;
        }
        else {
            dHealthRatio = GetItemHealth(false) / fHealth;
        }
        float fCondition = static_cast<float>(dHealthRatio);
        uint16_t usDT = static_cast<uint16_t>(static_cast<int32_t>(pArmor->GetDamageThreshold()));
        fDT = CombatFormulas::CalcArmorRating(usDT, fCondition);
    }
    return FloatCeil(fDT);
}

void ItemChange::LoadGame() {
    uint32_t uiBlockHeader;
    int32_t iCount;
    uint16_t usBlockSize;
    uint32_t uiBoundID;
    usBlockSize = 0;
    uint8_t* pStartBufferPosition = nullptr;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        pSaveLoadGameOLD->LoadGameDataOLD(&uiBlockHeader, sizeof(uiBlockHeader));
        if (uiBlockHeader != 0x424C4F4B) {
            LoadFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
            if (pHeader) {
                TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
                uint32_t iFlags = pHeader->iFlags;
                int32_t cVersion = pHeader->cVersion;
                const char* pEditorID = pForm->GetFormEditorID();
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Currently "
                    "loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp",
                    1607,
                    pHeader->iFormID,
                    pEditorID,
                    cVersion,
                    iFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Current "
                    "version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp",
                    1607,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        pStartBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->LoadGameDataOLD(&usBlockSize, sizeof(usBlockSize));
    }
    pSaveLoadGameOLD->LoadNumericID(&uiBoundID, 4u);
    pSaveLoadGameOLD->LoadGameDataOLD(&iNumber, sizeof(iNumber));
    TESForm* pBoundForm = TESForm::GetFormByNumericID(uiBoundID);
    pContainerObj = dynamic_cast<TESBoundObject*>(pBoundForm);
    pExtraObjectList = nullptr;
    pSaveLoadGameOLD->LoadGameDataOLD(&iCount, sizeof(iCount));
    if (iCount) {
        pExtraObjectList = new BSSimpleList<ExtraDataList*>;
        for (int32_t i = 0; i < iCount; ++i) {
            ExtraDataList* pExtraDataList;
            pExtraDataList = new ExtraDataList;
            pExtraDataList->LoadGame(0, 0, nullptr);
            pExtraObjectList->AddTail(pExtraDataList);
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* pCurrentBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        LoadFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
        if (pHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            if (pCurrentBufferPosition <= &pStartBufferPosition[usBlockSize]) {
                if (pCurrentBufferPosition < &pStartBufferPosition[usBlockSize]) {
                    uint32_t iFlags = pHeader->iFlags;
                    int32_t cVersion = pHeader->cVersion;
                    const char* pEditorID = pForm->GetFormEditorID();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading "
                        "form is %08X %s with version %i and flags %08X",
                        static_cast<int32_t>(&pStartBufferPosition[usBlockSize] - pCurrentBufferPosition),
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp",
                        1643,
                        pHeader->iFormID,
                        pEditorID,
                        cVersion,
                        iFlags
                    );
                }
            }
            else {
                uint32_t iFlags = pHeader->iFlags;
                int32_t cVersion = pHeader->cVersion;
                const char* pEditorID = pForm->GetFormEditorID();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading form "
                    "is %08X %s with version %i and flags %08X",
                    static_cast<int32_t>(pCurrentBufferPosition - &pStartBufferPosition[usBlockSize]),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp",
                    1643,
                    pHeader->iFormID,
                    pEditorID,
                    cVersion,
                    iFlags
                );
            }
        }
        else if (pCurrentBufferPosition <= &pStartBufferPosition[usBlockSize]) {
            if (pCurrentBufferPosition < &pStartBufferPosition[usBlockSize]) {
                Warning(
                    "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is "
                    "%i",
                    static_cast<int32_t>(&pStartBufferPosition[usBlockSize] - pCurrentBufferPosition),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp",
                    1643,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        else {
            Warning(
                "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                static_cast<int32_t>(pCurrentBufferPosition - &pStartBufferPosition[usBlockSize]),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp",
                1643,
                pSaveLoadGameOLD->GetCurrentVersion()
            );
        }
    }
}

void ItemChange::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    uint32_t uiFormID = apLoadGameBuffer->LoadFormID();
    TESForm* pForm = TESForm::GetFormByNumericID(uiFormID);
    pContainerObj = dynamic_cast<TESBoundObject*>(pForm);
    apLoadGameBuffer->LoadData(&iNumber, sizeof(iNumber));
    DeleteAllExtra();
    uint32_t uiSaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    if (uiSaveGameCount) {
        if (!pExtraObjectList) {
            pExtraObjectList = new BSSimpleList<ExtraDataList*>;
        }
        for (uint32_t i = 0; i < uiSaveGameCount; ++i) {
            ExtraDataList* pExtraDataList;
            pExtraDataList = new ExtraDataList;
            pExtraDataList->LoadGame(apLoadGameBuffer);
            pExtraObjectList->AddTail(pExtraDataList);
        }
    }
}

void ItemChange::SetItemOwnership(TESForm* apowner) {
    if (apowner) {
        if (pExtraObjectList && pExtraObjectList->m_item) {
            pExtraObjectList->m_item->SetOwnership(apowner);
        }
        else {
            ExtraDataList* pExtra;
            pExtra = new ExtraDataList;
            pExtra->SetOwnership(apowner);
            if (!pExtraObjectList) {
                pExtraObjectList = new BSSimpleList<ExtraDataList*>;
            }
            pExtraObjectList->AddHead(pExtra);
        }
    }
    else if (pExtraObjectList) {
        ExtraDataList* pExtra = pExtraObjectList->m_item;
        if (pExtra) {
            pExtraObjectList->Remove(pExtra);
            if (pExtra) {
                delete pExtra;
            }
        }
    }
}

// SaveGame of the old save format. With save blocks, the block's size is written back as a 16-bit value over the slot
// reserved after the 'BLOK' marker.
void ItemChange::SaveGame() {
    uint32_t uiBlockHeader;
    int32_t iCount;
    uint16_t usBlockSize;
    uint32_t uiBoundID;
    usBlockSize = 0;
    // The reserved size slot in the save buffer (untyped bytes there).
    void* pBlockSize = nullptr;
    uint8_t* pTempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    if (bDebugSaveBuffer.Bool()) {
        pTempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uiBlockHeader = 0x424C4F4B;
        pSaveLoadGameOLD->SaveGameDataOLD(&uiBlockHeader, sizeof(uiBlockHeader));
        pBlockSize = pSaveLoadGameOLD->m_pBuffer;
        pSaveLoadGameOLD->SaveGameDataOLD(&usBlockSize, sizeof(usBlockSize));
    }
    uiBoundID = pContainerObj->iFormID;
    pSaveLoadGameOLD->SaveNumericID(&uiBoundID, 4u);
    pSaveLoadGameOLD->SaveGameDataOLD(&iNumber, sizeof(iNumber));
    iCount = 0;
    int32_t* pCount = static_cast<int32_t*>(pSaveLoadGameOLD->m_pBuffer);
    pSaveLoadGameOLD->SaveGameDataOLD(&iCount, sizeof(iCount));
    for (BSSimpleList<ExtraDataList*>* pList = pExtraObjectList; pList && !pList->IsEmpty(); pList = pList->m_pkNext) {
        pList->m_item->SaveGame(0, nullptr);
        ++iCount;
    }
    *pCount = iCount;
    if (bDebugSaveBuffer.Bool()) {
        uint8_t* pBufferPointer = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            uint32_t iFlags = pHeader->iFlags;
            const char* pEditorID = pForm->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                static_cast<int32_t>(pBufferPointer - pTempStartPosition),
                pHeader->iFormID,
                pEditorID,
                iFlags,
                1601,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s",
                static_cast<int32_t>(pBufferPointer - pTempStartPosition),
                1601,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp"
            );
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* pCurrent = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        uint8_t* pBlockStart = static_cast<uint8_t*>(pBlockSize);
        if (pCurrent > pBlockStart + 0xFFFF) {
            Warning(
                "SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp",
                1601
            );
        }
        *static_cast<uint16_t*>(pBlockSize) = static_cast<uint16_t>(pCurrent - pBlockStart);
    }
}

void ItemChange::Copy(ItemChange* apCopy) {
    if (apCopy) {
        BSSimpleList<ExtraDataList*>* pList;
        pContainerObj = apCopy->pContainerObj;
        pList = new BSSimpleList<ExtraDataList*>;
        pExtraObjectList = pList;
        for (ExtraDataList*& rpItem : apCopy->pExtraObjectList->ItemsUntilNull()) {
            pExtraObjectList->AddHead(rpItem);
        }
    }
}

int32_t ItemChange::GetAmountNonDefaultExtra() {
    BSSimpleList<ExtraDataList*>* pkFirst = pExtraObjectList;
    int32_t iCount = 0;
    while (pkFirst && pkFirst->m_item) {
        ExtraDataList* pExtra = pkFirst->m_item;
        int16_t sCount = pExtra->GetCount();
        if (!pExtra->IsExtraDefaultforContainer(false) && sCount > 0) {
            ++iCount;
        }
        pkFirst = pkFirst->m_pkNext;
        if (pExtra->ItemsInList() == 1 && sCount > 1) {
            pExtra->RemoveCount();
            pExtraObjectList->Remove(pExtra);
            if (pExtra) {
                delete pExtra;
            }
            iCount = 0;
            pkFirst = pExtraObjectList;
        }
    }
    return iCount;
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
    BSSimpleList<ExtraDataList*>* pkFirst = pExtraObjectList;
    int32_t iCount = 0;
    while (pkFirst && pkFirst->m_item) {
        ExtraDataList* pExtra = pkFirst->m_item;
        if (pExtra && pExtra->HasLeveledItem()) {
            iCount += pExtra->GetCount();
        }

        pkFirst = pkFirst->m_pkNext;
    }
    return iCount;
}

ExtraLeveledItem* ItemChange::RemoveLeveledItemExtra() {
    BSSimpleList<ExtraDataList*>* pkFirst = pExtraObjectList;
    ExtraLeveledItem* pExtraLevel = nullptr;
    while (pkFirst && pkFirst->m_item) {
        ExtraDataList* pExtra = pkFirst->m_item;
        if (pExtra && pExtra->GetLeveledItem()) {
            pExtraLevel = pExtra->GetLeveledItem();
            pExtra->RemoveExtra(pExtraLevel, false);
            if (pExtra->IsExtraListEmpty()) {
                pExtraObjectList->Remove(pExtra);
            }
            pkFirst = pExtraObjectList;
        }

        pkFirst = pkFirst->m_pkNext;
    }
    return pExtraLevel;
}

void ItemChange::SetItemHealth(float afHealth, InventoryChanges* pchange, ExtraDataList* apextra, bool abdelete) {
    TESHealthForm* pHealth = dynamic_cast<TESHealthForm*>(pContainerObj);
    int32_t iMaxHealth = TESHealthForm::GetFormHealth(pContainerObj);
    int32_t iHealth = static_cast<int32_t>(GetItemHealth(false));
    if (pchange->pRef) {
        pchange->pRef->AddChange(CHANGE_REFR_INVENTORY);
    }
    if (pHealth) {
        if (!(afHealth < static_cast<double>(iMaxHealth)) && iHealth <= iMaxHealth) {
            for (ExtraDataList* pExtra : pExtraObjectList->ItemsUntilNull()) {
                if (pExtra && pExtra == apextra) {
                    pExtra->RemoveHealth();
                    if (pExtra->IsExtraListEmpty() && abdelete) {
                        pchange->GetObjectInList(pContainerObj, true, 0)->pExtraObjectList->Remove(pExtra);
                        if (pExtra) {
                            delete pExtra;
                        }
                    }
                    return;
                }
            }
        }
        else {
            BSSimpleList<ExtraDataList*>* pkFirst = pExtraObjectList;
            if (pkFirst) {
                ExtraDataList* pExtra;
                while (pkFirst && pkFirst->m_item) {
                    ExtraDataList* pCurrent = pkFirst->m_item;
                    if (pCurrent && pCurrent == apextra) {
                        pCurrent->SetHealth(afHealth);
                        return;
                    }
                    pkFirst = pkFirst->m_pkNext;
                }
                pExtra = new ExtraDataList;
                pExtra->SetHealth(afHealth);
                pExtraObjectList->AddHead(pExtra);
            }
            else {
                BSSimpleList<ExtraDataList*>* pList;
                ExtraDataList* pExtra;
                pExtra = new ExtraDataList;
                pList = new BSSimpleList<ExtraDataList*>;
                pExtra->SetCharge(afHealth);
                pList->AddHead(pExtra);
                pExtraObjectList = pList;
            }
        }
    }
}

void ItemChange::SetIsModding() {
    BSSimpleList<ExtraDataList*>* pkFirst = pExtraObjectList;
    if (!pkFirst) {
        pkFirst = new BSSimpleList<ExtraDataList*>;
        pExtraObjectList = pkFirst;
    }
    if (pkFirst->m_item) {
        if (pkFirst && pkFirst->m_item) {
            ExtraDataList* pExtra = pkFirst->m_item;
            if (pExtra) {
                pExtra->SetIsModding(true);
            }
        }
    }
    else {
        ExtraDataList* pExtra;
        pExtra = new ExtraDataList;
        pExtra->SetIsModding(true);
        pExtraObjectList->AddHead(pExtra);
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
    float fDR = -1.0f;
    if (this->pContainerObj->GetFormType() == 24) {
        double dHealthRatio;
        TESObjectARMO* pContainerObj = static_cast<TESObjectARMO*>(this->pContainerObj);
        float fHealth = static_cast<float>(TESHealthForm::GetFormHealth(pContainerObj));
        if (fHealth == 0.0) {
            dHealthRatio = 0.0;
        }
        else {
            dHealthRatio = GetItemHealth(false) / fHealth;
        }
        float fCondition = dHealthRatio;
        int32_t iArmorRating = static_cast<int32_t>(pContainerObj->GetArmorRating());
        fDR = CombatFormulas::CalcArmorRating(iArmorRating, fCondition);
    }
    return FloatCeil(fDR);
}

int32_t ItemChange::GetTypeforMenu() {
    uint32_t uiFormType = pContainerObj->GetFormType();
    if (uiFormType == 24 || uiFormType == 26) {
        return 4;
    }
    if (uiFormType == 40) {
        return 3;
    }
    return 5;
}

uint16_t ItemChange::GetSaveSize() {
    int16_t sBlockSize = 0;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        sBlockSize = 6;
    }
    uint16_t usSize = sBlockSize + 12;
    for (BSSimpleList<ExtraDataList*>* pList = pExtraObjectList; pList && !pList->IsEmpty(); pList = pList->m_pkNext) {
        usSize += pList->m_item->GetSaveSize(0, nullptr);
    }
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                usSize,
                pHeader->iFormID,
                pForm->GetFormEditorID(),
                pHeader->iFlags,
                1570,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s",
                usSize,
                1570,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp"
            );
        }
    }
    return usSize;
}

void ItemChange::InitLoadGame_0() {
    for (BSSimpleList<ExtraDataList*>* pNode = pExtraObjectList; pNode && !pNode->IsEmpty(); pNode = pNode->m_pkNext) {
        ExtraDataList* pList = pNode->m_item;
        if (pList) {
            pList->InitLoadGame(0, 0, nullptr, pContainerObj);
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
        ItemChange* pItem = pkFirst->m_item;
        pkFirst = pkFirst->m_pkNext;
        pItem->DeleteAllExtra();
        delete pItem;
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
    int32_t iAmount = 0;
    if (aId) {
        return HasObjectId(aId);
    }
    Actor* pActor = nullptr;
    if (apRef && apRef->IsActor()) {
        pActor = static_cast<Actor*>(apRef);
    }

    for (BSSimpleList<ContainerObject*>* pList = &GetOriginalContainer()->objectList; pList && !pList->IsEmpty();
         pList = pList->m_pkNext) {
        TESBoundObject* pObj = pList->m_item->pObj;
        if (pObj &&
            ((apObj && pObj == apObj) ||
             (!apObj && aeObjectType && TESPackage::FormMatchesPackageObjectType(pObj, aeObjectType)))) {
            ItemChange* pItem = GetObjectInList(pObj, true, 0);
            if (!pItem) {
                if (pActor && pActor->pCurrentProcess) {
                    pActor->pCurrentProcess->SetItemBeingUsed(pObj);
                }

                aeMatchId = TESPackage::GetObjectTypeFromForm(pObj);
                return true;
            }
            if (pList->m_item->i_count < 0) {
                return true;
            }
            uint32_t uiNumber = pItem->iNumber;
            iAmount += pList->m_item->i_count + uiNumber;
            if (pActor && pActor->pCurrentProcess) {
                pActor->pCurrentProcess->SetItemBeingUsed(pItem->pContainerObj);
            }

            aeMatchId = TESPackage::GetObjectTypeFromForm(pItem->pContainerObj);
        }
    }
    if (iAmount >= ainumber) {
        return true;
    }
    if (!apObj) {
        for (BSSimpleList<ItemChange*>* pInvList = pListofChanges; pInvList && !pInvList->IsEmpty() && pInvList->m_item;
             pInvList = pInvList->m_pkNext) {
            TESBoundObject* pObj = pInvList->m_item->pContainerObj;
            if (pObj &&
                aeObjectType &&
                TESPackage::FormMatchesPackageObjectType(pObj, aeObjectType) &&
                pInvList->m_item->iNumber >= ainumber) {
                if (pActor && pActor->pCurrentProcess) {
                    pActor->pCurrentProcess->SetItemBeingUsed(pInvList->m_item->pContainerObj);
                }

                aeMatchId = TESPackage::GetObjectTypeFromForm(pInvList->m_item->pContainerObj);
                return true;
            }
        }
        return false;
    }
    ItemChange* pItem = GetObjectInList(apObj, true, 0);
    if (!pItem || pItem->iNumber < ainumber) {
        return false;
    }
    if (pActor && pActor->pCurrentProcess) {
        pActor->pCurrentProcess->SetItemBeingUsed(pItem->pContainerObj);
    }

    aeMatchId = TESPackage::GetObjectTypeFromForm(pItem->pContainerObj);
    return true;
}

void InventoryChanges::RemoveGold(TESObjectREFR* apRef, int32_t aiamount, TESObjectREFR* apCont) {
    BSSimpleList<ContainerObject*>* pList;
    TESBoundObject* pObj;
    TESContainer* pOrigContainer = GetOriginalContainer();
    if (pOrigContainer) {
        pList = &pOrigContainer->objectList;
    }
    else {
        pList = nullptr;
    }
    for (; pList && !pList->IsEmpty(); pList = pList->m_pkNext) {
        pObj = pList->m_item->pObj;
        if (pObj && TESContainer::IsGold(pObj)) {
            RemoveItem(apRef, pObj, false, aiamount, nullptr, false, apCont, nullptr, nullptr, true, false, nullptr);
            return;
        }
    }
    for (BSSimpleList<ItemChange*>* pListInv = pListofChanges; pListInv && !pListInv->IsEmpty();
         pListInv = pListInv->m_pkNext) {
        pObj = pListInv->m_item->pContainerObj;
        if (pObj && TESContainer::IsGold(pObj)) {
            RemoveItem(apRef, pObj, false, aiamount, nullptr, false, apCont, nullptr, nullptr, true, false, nullptr);
            return;
        }
    }
}

int32_t InventoryChanges::GetGoldAmount() {
    BSSimpleList<ContainerObject*>* pList;
    int32_t iAmount = 0;
    TESContainer* pOrigContainer = GetOriginalContainer();
    if (pOrigContainer) {
        pList = &pOrigContainer->objectList;
    }
    else {
        pList = nullptr;
    }
    for (; pList && !pList->IsEmpty(); pList = pList->m_pkNext) {
        TESBoundObject* pObj = pList->m_item->pObj;
        if (pObj && TESContainer::IsGold(pObj)) {
            ItemChange* pItem = GetObjectInList(pObj, true, 0);
            if (pItem) {
                uint32_t uiNumber = pItem->iNumber;
                iAmount += pList->m_item->i_count + uiNumber;
            }
            else {
                iAmount += pList->m_item->i_count;
                if (iAmount < 0) {
                    iAmount = -iAmount;
                }
            }
        }
    }
    for (BSSimpleList<ItemChange*>* pListInv = pListofChanges; pListInv && !pListInv->IsEmpty();
         pListInv = pListInv->m_pkNext) {
        TESBoundObject* pObj = pListInv->m_item->pContainerObj;
        if (pObj &&
            TESContainer::IsGold(pObj) &&
            (!pOrigContainer || (!pOrigContainer->HasObject(pObj) && pListInv->m_item->iNumber > 0))) {
            iAmount += pListInv->m_item->iNumber;
        }
    }
    return iAmount;
}

int32_t InventoryChanges::GetCount(bool abcountplayable) {
    int32_t iCount = 0;
    if (GetOriginalContainer()) {
        for (ContainerObject* pObject : GetOriginalContainer()->objectList.ItemsUntilNull()) {
            TESLevItem* pObj = dynamic_cast<TESLevItem*>(pObject->pObj);
            TESBipedModelForm* pBipedForm = nullptr;
            TESBoundObject* pBoundObj = pObject->pObj;
            int32_t iOriginalCount = GetOriginalContainer()->GetObjectCount(pBoundObj);
            if (!abcountplayable) {
                int32_t iFormType = pObject->pObj->GetFormType();
                if (iFormType == 40) {
                    if (!static_cast<TESObjectWEAP*>(pObject->pObj)->TESObjectWEAP::GetIsPlayable()) {
                        pObject = nullptr;
                    }
                }
                else if (iFormType == 41) {
                    if (!static_cast<TESAmmo*>(pObject->pObj)->TESAmmo::GetIsPlayable()) {
                        pObject = nullptr;
                    }
                }
                else {
                    pBipedForm = TESBipedModelForm::GetFormAsBipedModel(pObject->pObj);
                    if (pBipedForm && !pBipedForm->GetPlayable()) {
                        pObject = nullptr;
                    }
                }
            }
            if (pObject && !pObj) {
                ItemChange* pItem = GetObjectInList(pObject->pObj, true, 0);
                int32_t iOrgCount = iOriginalCount;
                if (iOriginalCount < 0) {
                    iOrgCount = -iOrgCount;
                }
                if (pItem &&
                    iOrgCount + pItem->iNumber > 0 &&
                    (iOriginalCount >= 0 || pItem->iNumber > iOriginalCount)) {
                    if (pItem->pExtraObjectList && pItem->pExtraObjectList->m_item) {
                        iCount += pItem->GetAmountNonDefaultExtra();
                        if (pItem->HasStolenData()) {
                            ++iCount;
                        }
                        if (pItem->GetExtraTotalCount(false) < pItem->iNumber + iOriginalCount &&
                            pItem->pExtraObjectList->m_item &&
                            !pItem->GetWorn(false)) {
                            if (!pItem->pExtraObjectList->m_item->HasLeveledItem()) {
                                ++iCount;
                            }
                        }
                    }
                    else {
                        ++iCount;
                    }
                }
                else if (!pItem) {
                    ++iCount;
                }
            }
        }
    }
    BSSimpleList<ItemChange*>* pkFirst = pListofChanges;
    int32_t iTempCount = iCount;
    bool bFlag = true;
    while (pkFirst && pkFirst->m_item) {
        ItemChange* pItem = pkFirst->m_item;
        bFlag = true;
        bool bContinue = true;
        if (pItem) {
            if (pItem->pExtraObjectList &&
                pItem->pExtraObjectList->m_item &&
                pItem->pExtraObjectList->m_item->HasLeveledItem()) {
                bContinue = true;
            }
            else if (GetOriginalContainer()) {
                TESBoundObject* pContainerObj = pItem->pContainerObj;
                if (GetOriginalContainer()->HasObject(pContainerObj)) {
                    bContinue = false;
                }
            }
            if (!abcountplayable) {
                int32_t iFormType = pItem->pContainerObj->GetFormType();
                if (iFormType == 40) {
                    if (!static_cast<TESObjectWEAP*>(pItem->pContainerObj)->TESObjectWEAP::GetIsPlayable()) {
                        bContinue = false;
                    }
                }
                else if (iFormType == 41) {
                    if (!static_cast<TESAmmo*>(pItem->pContainerObj)->TESAmmo::GetIsPlayable()) {
                        bContinue = false;
                    }
                }
                else {
                    TESBipedModelForm* pBipedForm = TESBipedModelForm::GetFormAsBipedModel(pItem->pContainerObj);
                    if (pBipedForm) {
                        bContinue = pBipedForm->GetPlayable();
                    }
                }
            }
        }
        if (pItem && pItem->iNumber > 0 && bContinue) {
            iCount += pItem->GetAmountNonDefaultExtra();
            if (pItem->HasStolenData()) {
                ++iCount;
            }
            int32_t iExtraTotalCount = pItem->GetExtraTotalCount(false);
            if (iExtraTotalCount < pItem->iNumber) {
                ++iCount;
            }
            if ((pItem->iNumber & 0x80000000u) != 0) {
                if (!pItem->pExtraObjectList ||
                    !pItem->pExtraObjectList->m_item ||
                    !pItem->pExtraObjectList->m_item->HasLeveledItem()) {
                    pListofChanges->Remove(pItem);
                    pItem->DeleteAllExtra();
                    if (pItem) {
                        delete pItem;
                    }
                    iCount = iTempCount;
                    pkFirst = pListofChanges;
                    if (pkFirst && pkFirst->m_item) {
                        pItem = pkFirst->m_item;
                    }
                    else {
                        pItem = nullptr;
                    }
                    bFlag = false;
                }
            }
        }

        if (bFlag) {
            pkFirst = pkFirst->m_pkNext;
        }
    }
    return iCount + 1;
}

bool InventoryChanges::WearingVoiceModulateItem() {
    bool bModulate = false;
    for (ItemChange* pItem : pListofChanges->ItemsUntilNull()) {
        if (pItem && pItem->pContainerObj->GetFormType() == ARMO_ID && !bModulate) {
            TESObjectARMO* pArmor = static_cast<TESObjectARMO*>(pItem->pContainerObj);
            for (BSSimpleList<ExtraDataList*>* pkFirst = pItem->pExtraObjectList;
                 pkFirst && pkFirst->m_item && !bModulate;
                 pkFirst = pkFirst->m_pkNext) {
                ExtraDataList* pExtra = pkFirst->m_item;
                if (pExtra && pExtra->GetWorn(false)) {
                    bModulate = pArmor->ModulatesVoice();
                }
            }
        }
    }
    return bModulate;
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
        MEM_CONTEXT_INVENTORY,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp",
        2947
    );
    if (pRef) {
        pRef->AddChange(CHANGE_REFR_INVENTORY);
    }
    pItem = GetObjectInList(apObject, true, 0);
    int32_t iOrgCount = GetOriginalContainer()->GetObjectCount(apObject);
    int32_t iTotalCount = iOrgCount;
    if (pItem) {
        iTotalCount = pItem->iNumber + iOrgCount;
    }
    int32_t iExtraCount = 0;
    if (pItem && pItem->pExtraObjectList) {
        int32_t iExtraTotalCount = pItem->GetExtraTotalCount(false);
        iExtraCount = pItem->GetExtraTotalDefaultCount() + iExtraTotalCount;
    }

    int32_t iDefaultCount = iTotalCount - iExtraCount;
    if (pItem && pItem->pExtraObjectList) {
        if (pItem->pExtraObjectList->m_item && pItem->pExtraObjectList->m_item->HasLeveledItem()) {
            iDefaultCount = iOrgCount - iExtraCount;
        }

        bool bResetFlag = false;
        if (apExtra && !pItem->pExtraObjectList->IsInList(apExtra)) {
            for (ExtraDataList* pextra : pItem->pExtraObjectList->ItemsUntilNull()) {
                if (pextra && !pextra->CompareList(apExtra)) {
                    apExtra = pextra;
                    bResetFlag = true;
                    break;
                }
            }
            if (!bResetFlag) {
                apExtra = nullptr;
            }
        }
    }

    bCreateNew = true;
    pExtra = nullptr;
    if ((apExtra || iDefaultCount <= 0) && pItem) {
        if (pItem->GetWorn(false)) {
            return;
        }
        BSSimpleList<ExtraDataList*>* pkFirst = pItem->pExtraObjectList;
        while (pkFirst && bCreateNew) {
            pExtra = pkFirst->m_item;
            if (apExtra && apExtra == pExtra) {
                bool bThrown = apObject->GetFormType() == WEAP_ID &&
                    static_cast<TESObjectWEAP*>(apObject)->TESObjectWEAP::IsThrownWeapon();
                if (pExtra->GetCount() <= 1 || apObject->GetFormType() == AMMO_ID || bThrown) {
                    pExtra->SetCount(pItem->iNumber);
                }
                else {
                    ExtraDataList* pNewExtra;
                    pNewExtra = new ExtraDataList;
                    apExtra = pNewExtra;
                    pNewExtra->CopyList(pExtra);
                    apExtra->SetCount(1);
                    int16_t sCount = pExtra->GetCount();
                    pExtra->SetCount(sCount - 1);
                    if (pExtra->GetHotKey() >= 0) {
                        pExtra->RemoveHotKey();
                    }
                }
                bCreateNew = false;
            }
            else if (!pExtra || apExtra) {
                pkFirst = pkFirst->m_pkNext;
            }
            else {
                if (pExtra->GetCount() == 1) {
                    apExtra = pExtra;
                }
                else {
                    bool bThrown = apObject->GetFormType() == WEAP_ID &&
                        static_cast<TESObjectWEAP*>(apObject)->TESObjectWEAP::IsThrownWeapon();
                    if (pExtra->GetCount() <= 1 || apObject->GetFormType() == AMMO_ID || bThrown) {
                        apExtra = pExtra;
                    }
                    else {
                        ExtraDataList* pNewExtra;
                        pNewExtra = new ExtraDataList;
                        apExtra = pNewExtra;
                        pNewExtra->CopyList(pExtra);
                        apExtra->SetCount(1);
                        int16_t sCount = pExtra->GetCount();
                        pExtra->SetCount(sCount - 1);
                        if (pExtra->GetHotKey() >= 0) {
                            pExtra->RemoveHotKey();
                        }
                    }
                }
                bCreateNew = false;
            }
        }
    }
    else if (!pItem) {
        pItem = new ItemChange(apObject, 0);
        bCreateNew = true;
        pListofChanges->AddTail(pItem);
        apExtra = nullptr;
    }
    if (apExtra) {
        apExtra->SetCount(aiNumber);
        bCreateNew = false;
        if (!pItem->pExtraObjectList) {
            pItem->pExtraObjectList = new BSSimpleList<ExtraDataList*>;
        }
        if (!pItem->pExtraObjectList->IsInList(apExtra)) {
            pItem->pExtraObjectList->AddHead(apExtra);
        }
        if (pChange && !pChange->pExtraObjectList->IsInList(apExtra)) {
            pChange->pExtraObjectList->AddHead(apExtra);
        }

        pExtra = apExtra;
    }
    if (bCreateNew) {
        pExtra = new ExtraDataList;
        if (aiNumber > 1) {
            pExtra->SetCount(aiNumber);
        }
        if (!pItem->pExtraObjectList) {
            pItem->pExtraObjectList = new BSSimpleList<ExtraDataList*>;
        }
        if (!pItem->pExtraObjectList->IsInList(pExtra)) {
            pItem->pExtraObjectList->AddHead(pExtra);
        }
        if (pChange) {
            if (!pChange->pExtraObjectList) {
                pChange->pExtraObjectList = new BSSimpleList<ExtraDataList*>;
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
        if (pContainerObject && !GetObjectInList(pContainerObject->pObj, true, 0)) {
            TESBipedModelForm* pBipedForm;
            if ((aiIterateFlags & ITERATE_SKIP_QUEST_AND_UNPLAYABLE) == 0 ||
                (!pContainerObject->pObj->GetQuestObject() &&
                 (((pBipedForm = TESBipedModelForm::GetFormAsBipedModel(pContainerObject->pObj)) == nullptr ||
                   pBipedForm->GetPlayable()) &&
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
    BSSimpleList<ItemChange*>* pChangeList = pListofChanges;
    while (pChangeList) {
        ItemChange* pItemChange = pChangeList->m_item;
        pChangeList = pChangeList->m_pkNext;
        if (pItemChange) {
            TESBipedModelForm* pBipedForm;
            TESObjectWEAP* pWeapon = static_cast<TESObjectWEAP*>(pItemChange->pContainerObj);
            if ((aiIterateFlags & ITERATE_SKIP_QUEST_AND_UNPLAYABLE) == 0 ||
                (!pWeapon->GetQuestObject() &&
                 (((pBipedForm = TESBipedModelForm::GetFormAsBipedModel(pWeapon)) == nullptr ||
                   pBipedForm->GetPlayable()) &&
                  (pWeapon->GetFormType() != WEAP_ID || !pWeapon->IsEmbeddedWeapon())))) {
                uint32_t uiNumber = pItemChange->iNumber;
                int32_t iItemCount = GetOriginalContainer()->GetObjectCount(pWeapon) + uiNumber;
                if (iItemCount) {
                    ItemChange kItemChange;
                    kItemChange.pExtraObjectList = pItemChange->pExtraObjectList;
                    kItemChange.iNumber = iItemCount;
                    kItemChange.pContainerObj = pWeapon;
                    ++uiCalls;
                    // The temporary shares the change's extra data lists, so they are cleared before the destructor
                    // runs.
                    if (apCallback(&kItemChange, apData)) {
                        kItemChange.pExtraObjectList = nullptr;
                        return uiCalls;
                    }
                    kItemChange.pExtraObjectList = nullptr;
                }
            }
        }
    }
    return uiCalls;
}

uint32_t InventoryChanges::GetInventoryValue(bool abCountQuest, bool abCountGold) {
    uint32_t uiRetVal = 0;
    for (BSSimpleList<ContainerObject*>* pList = &GetOriginalContainer()->objectList; pList && !pList->IsEmpty();
         pList = pList->m_pkNext) {
        TESBoundObject* pObj = pList->m_item->pObj;
        if ((abCountQuest || !pObj->GetQuestObject()) && (abCountGold || pObj->iFormID != 15)) {
            int32_t iVal = TESValueForm::GetFormValue(pObj);
            if (iVal == -1) {
                iVal = 0;
            }
            if (iVal) {
                ItemChange* pItem = GetObjectInList(pObj, true, 0);
                if (pItem) {
                    uint32_t uiNumber = pItem->iNumber;
                    if (pList->m_item->i_count + uiNumber) {
                        uiRetVal += iVal * (pList->m_item->i_count + uiNumber);
                    }
                }
                else {
                    uiRetVal += pList->m_item->i_count * iVal;
                }
            }
        }
    }
    for (BSSimpleList<ItemChange*>* pInvList = pListofChanges; pInvList && !pInvList->IsEmpty() && pInvList->m_item;
         pInvList = pInvList->m_pkNext) {
        ItemChange* pItem = pInvList->m_item;
        TESBoundObject* pObj = pItem->pContainerObj;
        if (pObj) {
            if ((!GetOriginalContainer() || !GetOriginalContainer()->HasObject(pObj)) &&
                (abCountQuest || !pObj->GetQuestObject()) &&
                (abCountGold || pObj->iFormID != 15)) {
                int32_t iVal = TESValueForm::GetFormValue(pObj);
                if (iVal == -1) {
                    iVal = 0;
                }
                if (iVal && pItem->iNumber) {
                    uiRetVal += iVal * pItem->iNumber;
                }
            }
        }
    }
    return uiRetVal;
}

void InventoryChanges::GetItemGroup(TESBoundObject* apObj, BSSimpleList<ItemChange*>* apReturnList) {
    if (apObj && apReturnList) {
        IterData* pIter = GetIterator();
        ItemChange* pItem = nullptr;
        char cFound = 0;
        while (true) {
            pItem = Iterate(pIter);
            if (pItem) {
                if (pItem->pContainerObj == apObj) {
                    cFound = 1;
                    apReturnList->AddHead(pItem);
                }
                else {
                    if (pItem) {
                        delete pItem;
                    }
                    if (cFound) {
                        break;
                    }
                }
            }
            if (!pItem) {
                break;
            }
        }
        if (pIter) {
            delete pIter;
        }
    }
}

TESBoundObject* InventoryChanges::GetObjectbyType(ENUM_FORM_ID aeFormID, int32_t& ainumb) {
    BSSimpleList<ContainerObject*>* pList = &GetOriginalContainer()->objectList;
    TESBoundObject* pObj = nullptr;
    while (pList && !pObj && pList->m_item) {
        pObj = pList->m_item->pObj;
        if (pObj && pObj->GetFormType() == aeFormID) {
            ItemChange* pItem = GetObjectInList(pObj, true, 0);
            if (pItem) {
                uint32_t uiNumber = pItem->iNumber;
                if (pList->m_item->i_count + uiNumber) {
                    ainumb = pList->m_item->i_count + uiNumber;
                }
            }
            else {
                ainumb = pList->m_item->i_count;
            }
        }
        else {
            pObj = nullptr;
        }
        pList = pList->m_pkNext;
    }
    for (BSSimpleList<ItemChange*>* pListInv = pListofChanges; pListInv && !pObj && pListInv->m_item;
         pListInv = pListInv->m_pkNext) {
        pObj = pListInv->m_item->pContainerObj;
        if (pObj && pObj->GetFormType() == aeFormID) {
            if (!GetOriginalContainer()->HasObject(pObj) && pListInv->m_item->iNumber > 0) {
                ainumb = pListInv->m_item->iNumber;
            }
        }
        else {
            pObj = nullptr;
        }
    }
    return pObj;
}

void InventoryChanges::InitLoadGame() {
    for (BSSimpleList<ItemChange*>* pNode = pListofChanges; pNode && !pNode->IsEmpty(); pNode = pNode->m_pkNext) {
        ItemChange* pChange = pNode->m_item;
        if (pChange) {
            pChange->InitLoadGame_0();
        }
    }
}

void InventoryChanges::SaveGame() {
    uint32_t uiBlockHeader;
    uint16_t usCount;
    uint16_t usBlockSize;
    usBlockSize = 0;
    void* pBlockSize = nullptr;
    uint8_t* pTempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    if (bDebugSaveBuffer.Bool()) {
        pTempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        // 'BLOK', the block header.
        uiBlockHeader = 0x424C4F4B;
        pSaveLoadGameOLD->SaveGameDataOLD(&uiBlockHeader, sizeof(uiBlockHeader));
        pBlockSize = pSaveLoadGameOLD->m_pBuffer;
        pSaveLoadGameOLD->SaveGameDataOLD(&usBlockSize, sizeof(usBlockSize));
    }
    usCount = 0;
    uint16_t* pCount = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
    pSaveLoadGameOLD->SaveGameDataOLD(&usCount, sizeof(usCount));
    for (BSSimpleList<ItemChange*>* pList = pListofChanges; pList && !pList->IsEmpty(); pList = pList->m_pkNext) {
        ItemChange* pChange = pList->m_item;
        if (pChange) {
            pChange->SaveGame();
            ++usCount;
        }
    }
    *pCount = usCount;
    if (bDebugSaveBuffer.Bool()) {
        uint8_t* pBufferPointer = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            uint32_t iFlags = pHeader->iFlags;
            const char* pEditorID = pForm->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                static_cast<int32_t>(pBufferPointer - pTempStartPosition),
                pHeader->iFormID,
                pEditorID,
                iFlags,
                10346,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s",
                static_cast<int32_t>(pBufferPointer - pTempStartPosition),
                10346,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp"
            );
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* pCurrent = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        uint8_t* pBlockStart = static_cast<uint8_t*>(pBlockSize);
        if (pCurrent > pBlockStart + 0xFFFF) {
            Warning(
                "SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp",
                10346
            );
        }
        *static_cast<uint16_t*>(pBlockSize) = static_cast<uint16_t>(pCurrent - pBlockStart);
    }
}

// Loads the item changes of the old save format: a block header and size when the save uses blocks, then the count and
// the changes. A change whose object did not load is deleted. The block's end is checked against its size afterwards,
// with a warning on an underrun or overrun.
void InventoryChanges::LoadGame() {
    uint32_t uiBlockHeader;
    uint16_t usCount;
    uint16_t usBlockSize;
    usBlockSize = 0;
    uint8_t* pStartBufferPosition = nullptr;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        pSaveLoadGameOLD->LoadGameDataOLD(&uiBlockHeader, sizeof(uiBlockHeader));
        if (uiBlockHeader != 0x424C4F4B) {
            LoadFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
            if (pHeader) {
                TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Currently "
                    "loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp",
                    10353,
                    pHeader->iFormID,
                    pForm->GetFormEditorID(),
                    pHeader->cVersion,
                    pHeader->iFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Current "
                    "version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp",
                    10353,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        pStartBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->LoadGameDataOLD(&usBlockSize, sizeof(usBlockSize));
    }
    pSaveLoadGameOLD->LoadGameDataOLD(&usCount, sizeof(usCount));
    for (int32_t i = 0; i < usCount; ++i) {
        ItemChange* pChange;
        pChange = new ItemChange;
        pChange->LoadGame();
        if (pChange->pContainerObj) {
            pListofChanges->AddHead(pChange);
        }
        else {
            pChange->DeleteAllExtra();
            if (pChange) {
                delete pChange;
            }
        }
    }
    CleanUpBadData();
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* pCurrentBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        LoadFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
        if (pHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            if (pCurrentBufferPosition <= &pStartBufferPosition[usBlockSize]) {
                if (pCurrentBufferPosition < &pStartBufferPosition[usBlockSize]) {
                    uint32_t iFlags = pHeader->iFlags;
                    int32_t cVersion = pHeader->cVersion;
                    const char* pEditorID = pForm->GetFormEditorID();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading "
                        "form is %08X %s with version %i and flags %08X",
                        &pStartBufferPosition[usBlockSize] - pCurrentBufferPosition,
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp",
                        10372,
                        pHeader->iFormID,
                        pEditorID,
                        cVersion,
                        iFlags
                    );
                }
            }
            else {
                uint32_t iFlags = pHeader->iFlags;
                int32_t cVersion = pHeader->cVersion;
                const char* pEditorID = pForm->GetFormEditorID();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading form "
                    "is %08X %s with version %i and flags %08X",
                    pCurrentBufferPosition - &pStartBufferPosition[usBlockSize],
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp",
                    10372,
                    pHeader->iFormID,
                    pEditorID,
                    cVersion,
                    iFlags
                );
            }
        }
        else if (pCurrentBufferPosition <= &pStartBufferPosition[usBlockSize]) {
            if (pCurrentBufferPosition < &pStartBufferPosition[usBlockSize]) {
                int32_t iCurrentVersion = pSaveLoadGameOLD->GetCurrentVersion();
                Warning(
                    "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is "
                    "%i",
                    &pStartBufferPosition[usBlockSize] - pCurrentBufferPosition,
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp",
                    10372,
                    iCurrentVersion
                );
            }
        }
        else {
            int32_t iCurrentVersion = pSaveLoadGameOLD->GetCurrentVersion();
            Warning(
                "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                pCurrentBufferPosition - &pStartBufferPosition[usBlockSize],
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp",
                10372,
                iCurrentVersion
            );
        }
    }
}

// Saves each change with the buffer's form cleared and its change flags set to 0x400, then puts both back.
void InventoryChanges::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    TESForm* pForm = apSaveGameBuffer->GetForm();
    BGSChangeFlags kChangeFlags = apSaveGameBuffer->GetChangeFlags();
    apSaveGameBuffer->SetForm(nullptr);
    BGSChangeFlags kTempFlags(0x400);
    apSaveGameBuffer->SetChangeFlags(kTempFlags);
    uint32_t uiCount = 0;
    uint32_t uiPosition = apSaveGameBuffer->StartVariableSizedValue();
    for (BSSimpleList<ItemChange*>* pNode = pListofChanges; pNode;) {
        ItemChange* pChange = pNode->GetItem();
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
    int16_t sBlockSize = 0;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        sBlockSize = 6;
    }
    uint16_t usSize = sBlockSize + 2;
    for (BSSimpleList<ItemChange*>* pList = pListofChanges; pList && !pList->IsEmpty(); pList = pList->m_pkNext) {
        ItemChange* pChange = pList->m_item;
        if (pChange) {
            usSize += pChange->GetSaveSize();
        }
    }
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                usSize,
                pHeader->iFormID,
                pForm->GetFormEditorID(),
                pHeader->iFlags,
                0x284E,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s",
                usSize,
                0x284E,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp"
            );
        }
    }
    return usSize;
}

ItemChange* InventoryChanges::GetHotKeyItem(int32_t aiKey) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_INVENTORY,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp",
        1898
    );
    ItemChange* pHotKey = nullptr;
    bool bFound = false;
    InventoryChanges::IterData* pIter = GetIterator();
    while (!bFound) {
        ItemChange* pItem = Iterate(pIter);
        if (!pItem) {
            break;
        }
        for (BSSimpleList<ExtraDataList*>* pExtraList = pItem->GetExtraDataList(); pExtraList && pExtraList->GetItem();
             pExtraList = pExtraList->GetNext()) {
            if (pExtraList->GetItem()->GetHotKey() == aiKey) {
                pHotKey = pItem;
                bFound = true;
                break;
            }
        }
        if (!bFound) {
            delete pItem;
        }
    }
    if (pIter) {
        delete pIter;
    }
    return pHotKey;
}

// A new array of the changes on the hotkeys aiFirst through aiLast, null per hotkey without one; a change is stored
// under every hotkey in range, and a change with none is freed.
ItemChange** InventoryChanges::GetHotKeyItem(int32_t aiFirst, int32_t aiLast) {
    if (aiFirst > aiLast) {
        return nullptr;
    }
    const uint32_t uiCount = static_cast<uint32_t>(aiLast - aiFirst + 1);
    // Allocates ItemChange*[uiCount] with a checked multiply: the size becomes 0xFFFFFFFF when the
    // bytes do not fit in 32 bits.
    const uint32_t uiBytes = uiCount > UINT32_MAX / sizeof(ItemChange*)
        ? 0xFFFFFFFFu
        : static_cast<uint32_t>(sizeof(ItemChange*) * uiCount);
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
    AutoMemContext tempMemContext(
        MEM_CONTEXT_INVENTORY,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp",
        0x19B2
    );
    InventoryChanges::IterData* pIter = static_cast<InventoryChanges::IterData*>(
        ourMalloc(sizeof(InventoryChanges::IterData))
    );
    pIter->pFilteredList = nullptr;
    pIter->pItemList = nullptr;
    if (GetOriginalContainer()) {
        pIter->pFilteredList = GetOriginalContainer()->GetFilteredList();
    }
    pIter->dword8 = 0;
    pIter->pCurrentItem = nullptr;
    pIter->pContainer = GetOriginalContainer();
    pIter->pItemList = pIter->pFilteredList;
    pIter->uiPhase = 0;
    pIter->pExtraLists = nullptr;
    pIter->pAddExtraLists = nullptr;
    pIter->pCountExtraLists = nullptr;
    return pIter;
}

// The next change of the iterator, or null at the end.
ItemChange* InventoryChanges::Iterate(InventoryChanges::IterData* apIterator) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_INVENTORY,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp",
        0x19C9
    );
    apIterator->pCurrentItem = nullptr;
    if (!apIterator->uiPhase) {
        while (apIterator->pItemList && apIterator->pItemList->m_item && !apIterator->pCurrentItem) {
            ContainerObject* pObject = apIterator->pItemList->m_item;
            TESLevItem* pObj = nullptr;
            char cAdvance = 1;
            if (pObject->pObj->GetFormType() == LVLI_ID) {
                pObj = static_cast<TESLevItem*>(pObject->pObj);
            }
            if (pObject && !pObj) {
                ItemChange* pItem = GetObjectInList(pObject->pObj, true, 0);
                int32_t iOrgCount = apIterator->pContainer->GetObjectCount(pObject->pObj);
                if (pObject && pItem && iOrgCount <= 0) {
                    apIterator->pCurrentItem = nullptr;
                }
                else if (
                    (iOrgCount < 0 && pItem) ||
                    (pItem && iOrgCount + pItem->iNumber <= 0) ||
                    (pItem &&
                     pItem->pExtraObjectList &&
                     pItem->pExtraObjectList->m_item &&
                     pItem->pExtraObjectList->m_item->HasLeveledItem())
                ) {
                    apIterator->pCurrentItem = nullptr;
                }
                else if (!pObject || (pItem && pItem->GetAmountNonDefaultExtra())) {
                    if (pItem) {
                        if (pItem->pExtraObjectList) {
                            pItem->GetAmountNonDefaultExtra();
                        }
                        if (!apIterator->pExtraLists) {
                            apIterator->pExtraLists = pItem->pExtraObjectList;
                        }
                        while (
                            apIterator->pExtraLists && apIterator->pExtraLists->m_item && !apIterator->pCurrentItem) {
                            ExtraDataList* pCurExtra = apIterator->pExtraLists->m_item;
                            if (!pCurExtra->IsExtraDefaultforContainer(false)) {
                                ItemChange* pReturnItem;
                                int32_t iCount = pCurExtra->GetCount();
                                if (iCount > iOrgCount + pItem->iNumber) {
                                    iCount = iOrgCount + pItem->iNumber;
                                }
                                pReturnItem = new ItemChange(pItem->pContainerObj, iCount);
                                apIterator->pCurrentItem = pReturnItem;
                                apIterator->pCurrentItem->pExtraObjectList->AddHead(pCurExtra);
                            }
                            apIterator->pExtraLists = apIterator->pExtraLists->m_pkNext;
                        }
                        cAdvance = !apIterator->pExtraLists || !apIterator->pExtraLists->m_item;
                    }
                    else {
                        // Reached only with a null pObject, so pItem is null too: this calls it on null
                        // deliberately.
                        pItem->GetAmountNonDefaultExtra();
                        bool bFlag = false;
                        TESObjectWEAP* pWeap = nullptr;
                        if (pItem->pContainerObj->GetFormType() == WEAP_ID) {
                            pWeap = static_cast<TESObjectWEAP*>(pItem->pContainerObj);
                        }
                        if (!pItem->GetExtraTotalCount(false) &&
                            pItem->GetWorn(false) &&
                            (pItem->iNumber <= 0 ||
                             pItem->pContainerObj->GetFormType() == AMMO_ID ||
                             (pWeap &&
                              (pWeap->GetWeaponType() == WEAPON_ONE_HAND_GRENADE ||
                               pWeap->GetWeaponType() == WEAPON_ONE_HAND_MINE ||
                               pWeap->GetWeaponType() == WEAPON_ONE_HAND_THROWN)))) {
                            bFlag = true;
                        }
                        if (!bFlag) {
                            int32_t iExtraTotalCount = pItem->GetExtraTotalCount(true);
                            if (iExtraTotalCount < pItem->iNumber + iOrgCount) {
                                if (!pItem->HasLeveledExtra() || pItem->iNumber <= iOrgCount) {
                                    ItemChange* pReturnItem;
                                    pReturnItem = new ItemChange;
                                    apIterator->pCurrentItem = pReturnItem;
                                    if (pItem) {
                                        int32_t iTotalCount = pItem->iNumber + iOrgCount;
                                        apIterator->pCurrentItem->iNumber = iTotalCount -
                                            pItem->GetExtraTotalCount(false);
                                    }
                                    else {
                                        apIterator->pCurrentItem->iNumber = iOrgCount;
                                    }
                                    apIterator->pCurrentItem->pContainerObj = pObject->pObj;
                                    if (pItem &&
                                        pItem->GetScript() &&
                                        pItem->pExtraObjectList &&
                                        pItem->pExtraObjectList->ItemsInList()) {
                                        if (!apIterator->pCurrentItem->pExtraObjectList) {
                                            apIterator->pCurrentItem
                                                ->pExtraObjectList = new BSSimpleList<ExtraDataList*>;
                                        }
                                        for (ExtraDataList*& rpItem : pItem->pExtraObjectList->ItemsUntilNull()) {
                                            apIterator->pCurrentItem->pExtraObjectList->AddHead(rpItem);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                else {
                    ItemChange* pReturnItem;
                    pReturnItem = new ItemChange;
                    apIterator->pCurrentItem = pReturnItem;
                    if (pItem) {
                        if (iOrgCount >= 0) {
                            int32_t iTotalCount = pItem->iNumber + iOrgCount;
                            apIterator->pCurrentItem->iNumber = iTotalCount - pItem->GetExtraTotalCount(false);
                        }
                        else {
                            apIterator->pCurrentItem->iNumber = pItem->iNumber + iOrgCount;
                        }
                    }
                    else if (iOrgCount >= 0) {
                        apIterator->pCurrentItem->iNumber = iOrgCount;
                    }
                    else {
                        apIterator->pCurrentItem->iNumber = -iOrgCount;
                    }
                    apIterator->pCurrentItem->pContainerObj = pObject->pObj;
                    if (pItem) {
                        if ((pItem->GetScript() ||
                             pItem->GetHotKey() >= 0 ||
                             static_cast<TESObjectREFR*>(pItem->GetItemOwnership()) != pRef) &&
                            pItem->pExtraObjectList &&
                            pItem->pExtraObjectList->ItemsInList()) {
                            if (!apIterator->pCurrentItem->pExtraObjectList) {
                                apIterator->pCurrentItem->pExtraObjectList = new BSSimpleList<ExtraDataList*>;
                            }
                            for (ExtraDataList*& rpItem : pItem->pExtraObjectList->ItemsUntilNull()) {
                                apIterator->pCurrentItem->pExtraObjectList->AddHead(rpItem);
                            }
                        }
                    }
                }
            }

            if (cAdvance) {
                apIterator->pItemList = apIterator->pItemList->m_pkNext;
            }
        }
        if (!apIterator->pCurrentItem) {
            apIterator->uiPhase = 1;
            apIterator->pChangeList = pListofChanges;
        }
    }
    if (apIterator->uiPhase == 1 && !apIterator->pCurrentItem) {
        while (apIterator->pChangeList && apIterator->pChangeList->m_item && !apIterator->pCurrentItem) {
            char cAdvance = 1;
            ItemChange* pItem = apIterator->pChangeList->m_item;
            int32_t iOriginalCount = 0;
            if (GetOriginalContainer() && pItem) {
                TESBoundObject* pContainerObj = pItem->pContainerObj;
                iOriginalCount = GetOriginalContainer()->GetObjectCount(pContainerObj);
            }

            if (iOriginalCount < 0 && pItem->GetWorn(false)) {
                pItem->pExtraObjectList->m_item->SetCount(-static_cast<int16_t>(iOriginalCount));
            }
            if (pItem && (pItem->iNumber > 0 || iOriginalCount < 0)) {
                TESBoundObject* pContainerObj;
                if ((pItem->pExtraObjectList &&
                     pItem->pExtraObjectList->m_item &&
                     pItem->pExtraObjectList->m_item->HasLeveledItem()) ||
                    iOriginalCount < 0 ||
                    !GetOriginalContainer() ||
                    (pContainerObj = pItem->pContainerObj, !GetOriginalContainer()->HasObject(pContainerObj))) {
                    int32_t iTotalNonDefaultCount = 0;
                    if (!apIterator->pCountExtraLists) {
                        apIterator->pCountExtraLists = pItem->pExtraObjectList;
                    }
                    while (apIterator->pCountExtraLists && apIterator->pCountExtraLists->m_item) {
                        ExtraDataList* pExtra = apIterator->pCountExtraLists->m_item;
                        if (!pExtra->IsExtraDefaultforContainer(false)) {
                            int16_t sCount = pExtra->GetCount();
                            if (sCount > 0) {
                                iTotalNonDefaultCount += sCount;
                            }
                        }
                        apIterator->pCountExtraLists = apIterator->pCountExtraLists->m_pkNext;
                    }
                    apIterator->pCountExtraLists = nullptr;
                    if (pItem->pExtraObjectList && pItem->pExtraObjectList->m_item) {
                        pItem->GetAmountNonDefaultExtra();
                        if (!apIterator->pAddExtraLists) {
                            apIterator->pAddExtraLists = pItem->pExtraObjectList;
                        }
                        while (apIterator->pAddExtraLists &&
                               apIterator->pAddExtraLists->m_item &&
                               !apIterator->pCurrentItem) {
                            ExtraDataList* pCurExtra = apIterator->pAddExtraLists->m_item;
                            if (!pCurExtra->IsExtraDefaultforContainer(false)) {
                                int16_t sCount = pCurExtra->GetCount();
                                if (sCount > 0) {
                                    ItemChange* pReturnItem;
                                    pReturnItem = new ItemChange(pItem->pContainerObj, sCount);
                                    apIterator->pCurrentItem = pReturnItem;
                                    apIterator->pCurrentItem->pExtraObjectList->AddHead(pCurExtra);
                                }
                            }
                            apIterator->pAddExtraLists = apIterator->pAddExtraLists->m_pkNext;
                        }
                        cAdvance = !apIterator->pAddExtraLists || !apIterator->pAddExtraLists->m_item;
                    }
                    else {
                        uint32_t uiNumber = pItem->iNumber;
                        if (uiNumber == iTotalNonDefaultCount) {
                            pItem->GetAmountNonDefaultExtra();
                        }
                    }
                    if (!apIterator->pCurrentItem) {
                        if (pItem->iNumber > iTotalNonDefaultCount) {
                            // The count is computed and never read.
                            pItem->GetAmountNonDefaultExtra();
                        }
                        if (!apIterator->pCurrentItem && pItem->iNumber - iTotalNonDefaultCount > 0) {
                            ItemChange* pReturnItem;
                            ExtraDataList* pExtra = nullptr;
                            pReturnItem = new ItemChange(pItem->pContainerObj, 0);
                            apIterator->pCurrentItem = pReturnItem;
                            apIterator->pCurrentItem->iNumber = pItem->iNumber - iTotalNonDefaultCount;
                            if (pItem->pExtraObjectList) {
                                for (ExtraDataList*& rpItem : pItem->pExtraObjectList->ItemsUntilNull()) {
                                    pExtra = rpItem;
                                    if (!pExtra->GetWorn(false) && pExtra->IsExtraDefaultforContainer(true)) {
                                        apIterator->pCurrentItem->pExtraObjectList->AddTail(pExtra);
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if (cAdvance) {
                apIterator->pChangeList = apIterator->pChangeList->m_pkNext;
            }
        }
    }
    return apIterator->pCurrentItem;
}

bool InventoryChanges::HasObjectId(uint32_t aId) {
    int32_t iAmount = 0;
    for (BSSimpleList<ContainerObject*>* pList = &GetOriginalContainer()->objectList; pList && !pList->IsEmpty();
         pList = pList->m_pkNext) {
        TESBoundObject* pObj = pList->m_item->pObj;
        if (pObj && pObj->iFormID == aId) {
            ItemChange* pItem = GetObjectInList(pObj, true, 0);
            if (pList->m_item->i_count < 0) {
                return true;
            }
            if (pItem && pList->m_item->i_count + static_cast<uint32_t>(pItem->iNumber)) {
                iAmount += pList->m_item->i_count + static_cast<uint32_t>(pItem->iNumber);
            }
            else {
                iAmount += pList->m_item->i_count;
            }
            if (iAmount > 0) {
                return true;
            }
        }
    }
    for (BSSimpleList<ItemChange*>* pListInv = pListofChanges; pListInv && !pListInv->IsEmpty();
         pListInv = pListInv->m_pkNext) {
        for (ExtraDataList* pExtra : pListInv->m_item->pExtraObjectList->ItemsUntilNull()) {
            if (pExtra && pExtra->GetReferencePointer()) {
                if (pExtra->GetReferencePointer()->iFormID == aId) {
                    return true;
                }
            }
        }
    }
    return false;
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
// change with a positive count. The base container is deliberately not null-checked.
bool InventoryChanges::HasQuestObject() {
    bool bHasQuestObject = false;
    for (BSSimpleList<ContainerObject*>* pNode = GetOriginalContainer()->GetObjectList();
         pNode && pNode->GetItem() && !bHasQuestObject;
         pNode = pNode->GetNext()) {
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
         pNode && !pNode->IsEmpty() && !bHasScript;
         pNode = pNode->GetNext()) {
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
            if (pChange->iNumber &&
                !pChecked->GetAt(pChange->pContainerObj, bChecked) &&
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
// null-checked.
ItemChange* InventoryChanges::GetInventoryItem(TESBoundObject* apObject, uint32_t aId) {
    ItemChange* pItem = GetObjectInList(apObject, true, aId);
    ItemChange* pNewItem = nullptr;
    if (pItem) {
        int32_t iBaseCount = GetOriginalContainer()->GetObjectCount(apObject);
        pNewItem = new ItemChange(apObject, pItem->GetCount() + iBaseCount);
        if (pItem->GetExtraDataList() && pItem->GetExtraDataList()->ItemsInList()) {
            if (!pNewItem->GetExtraDataList()) {
                pNewItem->pExtraObjectList = new BSSimpleList<ExtraDataList*>();
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
             pNode && !pNode->IsEmpty();
             pNode = pNode->GetNext()) {
            if (pNode->GetItem()->pObj == apObject) {
                pNewItem = new ItemChange(apObject, pNode->GetItem()->i_count);
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
        MEM_CONTEXT_INVENTORY,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp",
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
                    TESBipedModelForm* pBipedModel = TESBipedModelForm::GetFormAsBipedModel(
                        pItem->GetContainerObject()
                    );
                    if (aeBodyPart >= BO_HEAD && (aeBodyPart <= BO_HAIR || aeBodyPart == BO_HAT)) {
                        BGSBipedModelList* pModelList = BGSBipedModelList::GetFormAsBipedModelList(
                            pItem->GetContainerObject()
                        );
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
        pWornItem = new ItemChange;
        pWornItem->pContainerObj = pItem->GetContainerObject();
        if (!pWornItem->pExtraObjectList) {
            pWornItem->pExtraObjectList = new BSSimpleList<ExtraDataList*>;
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
        return false;
    }
    if (!arIterator.pContainerObjectList && !arIterator.pInventoryChangeList) {
        arIterator.pContainerObjectList = &GetOriginalContainer()->objectList;
    }
    if (arIterator.pContainerObjectList) {
        while (arIterator.pContainerObjectList) {
            ContainerObject* pContainerObject = arIterator.pContainerObjectList->m_item;
            arIterator.pContainerObjectList = arIterator.pContainerObjectList->m_pkNext;
            if (!arIterator.pContainerObjectList) {
                if (pListofChanges) {
                    arIterator.pInventoryChangeList = pListofChanges;
                }
                else {
                    arIterator.bFinished = true;
                }
            }
            if (pContainerObject) {
                TESBipedModelForm* pBipedForm;
                if (!GetObjectInList(pContainerObject->pObj, true, 0) &&
                    ((aiIterateFlags & ITERATE_SKIP_QUEST_AND_UNPLAYABLE) == 0 ||
                     (!pContainerObject->pObj->GetQuestObject() &&
                      (((pBipedForm = TESBipedModelForm::GetFormAsBipedModel(pContainerObject->pObj)) == nullptr ||
                        pBipedForm->GetPlayable()) &&
                       (pContainerObject->pObj->GetFormType() != WEAP_ID ||
                        !static_cast<TESObjectWEAP*>(pContainerObject->pObj)->TESObjectWEAP::IsEmbeddedWeapon()))))) {
                    arIterator.TempItemChange.pExtraObjectList = nullptr;
                    arIterator.TempItemChange.iNumber = pContainerObject->i_count;
                    arIterator.TempItemChange.pContainerObj = pContainerObject->pObj;
                    arpItemChange = &arIterator.TempItemChange;
                    return true;
                }
            }
        }
    }
    if (arIterator.pInventoryChangeList) {
        while (arIterator.pInventoryChangeList) {
            ItemChange* pItemChange = arIterator.pInventoryChangeList->m_item;
            arIterator.pInventoryChangeList = arIterator.pInventoryChangeList->m_pkNext;
            if (!arIterator.pInventoryChangeList) {
                arIterator.bFinished = true;
            }
            if (pItemChange) {
                TESBipedModelForm* pBipedForm;
                TESObjectWEAP* pWeapon = static_cast<TESObjectWEAP*>(pItemChange->pContainerObj);
                if ((aiIterateFlags & ITERATE_SKIP_QUEST_AND_UNPLAYABLE) == 0 ||
                    (!pWeapon->GetQuestObject() &&
                     (((pBipedForm = TESBipedModelForm::GetFormAsBipedModel(pWeapon)) == nullptr ||
                       pBipedForm->GetPlayable()) &&
                      (pWeapon->GetFormType() != WEAP_ID || !pWeapon->IsEmbeddedWeapon())))) {
                    uint32_t uiNumber = pItemChange->iNumber;
                    int32_t iCount = GetOriginalContainer()->GetObjectCount(pWeapon) + uiNumber;
                    if (iCount > 0) {
                        arpItemChange = pItemChange;
                        arIterator.TempItemChange.pExtraObjectList = pItemChange->pExtraObjectList;
                        arIterator.TempItemChange.iNumber = iCount;
                        arIterator.TempItemChange.pContainerObj = pWeapon;
                        arpItemChange = &arIterator.TempItemChange;
                        return true;
                    }
                }
            }
        }
    }
    return false;
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
    ItemChange* pItem;
    pRestackedList = nullptr;
    ResetCachedWeights();
    if (apItemChange) {
        pItem = apItemChange;
    }
    else {
        pItem = GetObjectInList(apObject, true, 0);
    }
    if (!pItem) {
        return true;
    }
    *abRemoved = true;
    BSSimpleList<ExtraDataList*>* pkFirst = pItem->pExtraObjectList;
    bool bExtraGood = true;
    bool bRemovedPowerArmor = false;
    bool bIsThrown = false;
    while (pkFirst && pkFirst->m_item) {
        ExtraDataList* pExtra = pkFirst->m_item;
        if (pExtra->GetWorn(ableft) && (!apExtra || apExtra == pExtra)) {
            ItemChange* pChangeItem = nullptr;
            if (pExtra->GetCanNotWear() && !apActor->IsDead(false)) {
                *abRemoved = false;
                return bExtraGood;
            }
            if (apActor && apActor->pCurrentProcess) {
                char cResetModel = 1;
                switch (apObject->GetFormType()) {
                    case ARMO_ID: {
                        TESObjectARMO* pArmor = static_cast<TESObjectARMO*>(apObject);
                        if (pArmor->TESBipedModelForm::IsPowerArmor() || pArmor->TESBipedModelForm::IsHeavy()) {
                            bRemovedPowerArmor = true;
                        }
                        ExtraDismemberedLimbs* pDismemberment = apActor->m_Extra.GetDismembermentExtra();
                        if (pDismemberment &&
                            pDismemberment->DismemberedLimbs.iSize &&
                            pDismemberment->GetDismemberedLimb(0)) {
                            for (uint32_t uiIndex = 0; uiIndex < pDismemberment->DismemberedLimbs.iSize; ++uiIndex) {
                                DismemberedLimb* pLimb = pDismemberment->GetDismemberedLimb(uiIndex);
                                if (!pLimb->cLimb && pLimb->bLimbExploded) {
                                    cResetModel = 0;
                                }
                            }
                        }

                        if (cResetModel) {
                            apActor->pCurrentProcess->Set3DUpdateFlag(RESET_MODEL);
                        }
                        break;
                    }
                    case CLOT_ID: {
                        ExtraDismemberedLimbs* pDismemberment = apActor->m_Extra.GetDismembermentExtra();
                        if (pDismemberment &&
                            pDismemberment->DismemberedLimbs.iSize &&
                            pDismemberment->GetDismemberedLimb(0)) {
                            for (int32_t iIndex = 0; iIndex < pDismemberment->DismemberedLimbs.iSize; ++iIndex) {
                                DismemberedLimb* pLimb = pDismemberment->GetDismemberedLimb(iIndex);
                                if (!pLimb->cLimb && pLimb->bLimbExploded) {
                                    cResetModel = 0;
                                }
                            }
                        }

                        if (cResetModel) {
                            apActor->pCurrentProcess->Set3DUpdateFlag(RESET_MODEL);
                        }
                        break;
                    }
                    case WEAP_ID: {
                        pChangeItem = apActor->pCurrentProcess->GetCurrentWeapon();
                        if (pChangeItem) {
                            if (pChangeItem->pExtraObjectList->m_item == pExtra || !apExtra) {
                                if (apActor == pPlayer) {
                                    pPlayer->pQueuedWeaponAttach = nullptr;
                                }
                                if (static_cast<TESObjectWEAP*>(apObject)->TESObjectWEAP::IsThrownWeapon()) {
                                    bIsThrown = true;
                                }
                                if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
                                    TESBoundObject* pWeaponObj = pChangeItem->pContainerObj;
                                    TaskQueueInterface::GetInstance()->QueueRemoveWeapon(apActor, pWeaponObj);
                                }
                                else {
                                    apActor->RemoveWeapon();
                                    apActor->pCurrentProcess->SetCurrentWeapon(nullptr, nullptr, false);
                                    apActor->pCurrentProcess->SetCurrentAmmo(nullptr);
                                }
                            }
                        }
                        break;
                    }
                    case AMMO_ID: {
                        pChangeItem = apActor->pCurrentProcess->GetCurrentAmmo();
                        if (pChangeItem) {
                            if (pChangeItem->pExtraObjectList->m_item == pExtra) {
                                apActor->pCurrentProcess->SetCurrentAmmo(nullptr);
                            }
                        }
                        break;
                    }
                    default: {
                        break;
                    }
                }
            }

            pExtra->SetWorn(false, ableft);
            if (pExtra->ItemsInList() <= 1) {
                if (pExtra->GetHotKey() < 0) {
                    pExtra->SetCount(0);
                }
                else {
                    int16_t sNumber = pItem->iNumber;
                    int16_t sExtraTotalCount = pItem->GetExtraTotalCount(false);
                    pExtra->SetCount(sNumber - sExtraTotalCount);
                }
            }
            if (pExtra->IsExtraListEmpty()) {
                pItem->pExtraObjectList->Remove(pExtra);
                if (pExtra) {
                    delete pExtra;
                }
                if (pExtra == apExtra && apbExtraDeleted) {
                    *apbExtraDeleted = true;
                }

                pExtra = nullptr;
                bExtraGood = false;
                break;
            }
            BSSimpleList<ExtraDataList*>* pNode = pItem->pExtraObjectList;
            while (pNode && pNode->m_item && bExtraGood) {
                ExtraDataList* pCurrent = pNode->m_item;
                ExtraDataList* pCompare = pExtra;
                bool bComp = true;
                if (pExtra) {
                    bComp = pCompare->CompareListForContainer(pCurrent, false, bIsThrown) ||
                        pCurrent->CompareListForContainer(pCompare, false, bIsThrown);
                }
                if (bComp || pExtra == pCurrent) {
                    pNode = pNode->m_pkNext;
                }
                else {
                    int16_t sCount = pCurrent->GetCount();
                    pCurrent->SetCount(aiNumber + sCount);
                    pRestackedList = pCurrent;
                    if (pExtra && pExtra->GetHotKey() >= 0) {
                        char cHotKey = pExtra->GetHotKey();
                        pCurrent->SetHotKey(cHotKey);
                    }

                    if (pCurrent->IsExtraListEmpty()) {
                        pItem->pExtraObjectList->Remove(pCurrent);
                        if (pCurrent) {
                            delete pCurrent;
                        }
                    }
                    else {
                        pItem->pExtraObjectList->Remove(pExtra);
                    }
                    if (pExtra) {
                        delete pExtra;
                    }
                    pExtra = nullptr;
                    bExtraGood = false;
                }
            }
            pkFirst = nullptr;
        }
        else {
            pkFirst = pkFirst->m_pkNext;
        }
    }
    if (bRemovedPowerArmor && apActor && apActor->pCurrentProcess) {
        apActor->pCurrentProcess->UpdateArmorWorn(apActor);
    }

    if (pRef) {
        pRef->AddChange(CHANGE_REFR_INVENTORY);
    }
    int32_t iOrgCount = GetOriginalContainer()->GetObjectCount(apObject);
    if (pItem &&
        pItem->pExtraObjectList &&
        pItem->pExtraObjectList->IsEmpty() &&
        (!pItem->iNumber || iOrgCount == pItem->iNumber)) {
        pListofChanges->Remove(pItem);
        if (pItem) {
            delete pItem;
        }
        return false;
    }

    if (pItem->iNumber < 0 && pItem->pExtraObjectList) {
        pItem->pExtraObjectList->RemoveAll();
    }

    if (apActor) {
        apActor->InitLightingPropertyPtr(apActor->TESObjectREFR::Get3D());
    }
    return bExtraGood;
}

// Constructs and destroys an AutoMemContext on the stack and ignores the constructor's result. A null result of
// GetOriginalContainer is used without a check.
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
        MEM_CONTEXT_INVENTORY,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp",
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
            pChange = new ItemChange(apObject, 0);
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
                bool bThrown = apObject->GetFormType() == WEAP_ID &&
                    static_cast<TESObjectWEAP*>(apObject)->IsThrownWeapon();
                if (pWornExtra->GetCount() > 1 && apObject->GetFormType() != AMMO_ID && !bThrown) {
                    apExtra = new ExtraDataList;
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
                    bool bThrown = apObject->GetFormType() == WEAP_ID &&
                        static_cast<TESObjectWEAP*>(apObject)->IsThrownWeapon();
                    if (pWornExtra->GetCount() > 1 && apObject->GetFormType() != AMMO_ID && !bThrown) {
                        apExtra = new ExtraDataList;
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
            pChange->pExtraObjectList = new BSSimpleList<ExtraDataList*>;
        }
        bool bInList;
        bInList = pChange->pExtraObjectList->IsInList(apExtra);
        if (!bInList) {
            pChange->pExtraObjectList->AddHead(apExtra);
        }
        pWornExtra = apExtra;
    }

    if (bAddNew) {
        pWornExtra = new ExtraDataList;
        pWornExtra->SetWorn(true, ableft);
        if (aiNumber > 1) {
            pWornExtra->SetCount(static_cast<uint16_t>(aiNumber));
        }
        if (!pChange->pExtraObjectList) {
            pChange->pExtraObjectList = new BSSimpleList<ExtraDataList*>;
        }
        bool bInList = pChange->pExtraObjectList->IsInList(pWornExtra);
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
                pProcessChange = new ItemChange(apObject, aiNumber);
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
                        BGSEntryPoint::ATTACK_SPEED_MODIFIER,
                        apActor,
                        pProcessChange->GetContainerObject(),
                        &fAttackSpeed
                    );
                    if (pAnimation) {
                        pAnimation->SetAttackSpeed(fAttackSpeed);
                    }
                }
            }

            if ((apActor == pPlayer || !apActor->pCurrentProcess->Get3DUpdateFlag(RESET_MODEL)) && pProcessChange) {
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
                pProcessChange = new ItemChange(apObject, 0);
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
        MEM_CONTEXT_INVENTORY,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp",
        3368
    );
    bcountdirty = true;
    fpreviousContainerWeight = fcontainerweight;
    fcontainerweight = -1.0f;

    int32_t iAmmoCount = 0;
    if (!pextra) {
        pextra = new ExtraDataList;
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
        if (pWeapon->IsThrownWeapon() &&
            pProcess->GetCurrentAmmo() &&
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
                    pextra = new ExtraDataList;
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
        pChange = new ItemChange(apObject, ainumber);
        if (!pChange->pExtraObjectList) {
            pChange->pExtraObjectList = new BSSimpleList<ExtraDataList*>();
        }
        pChange->pExtraObjectList->AddHead(pextra);
        AddChangeItem(pChange, true);
    }
    else {
        // A leveled first instance of an uncounted change absorbs the count of an anonymous new instance.
        if (pChange->GetCount() <= 0 &&
            pextra &&
            !pextra->GetOwner() &&
            !pextra->GetScript() &&
            pChange->pExtraObjectList &&
            pChange->pExtraObjectList->ItemsInList() &&
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
                        pChange->pExtraObjectList = new BSSimpleList<ExtraDataList*>();
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
                pChange->pExtraObjectList = new BSSimpleList<ExtraDataList*>();
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
        MEM_CONTEXT_INVENTORY,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp",
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
                    pExisting->pExtraObjectList = new BSSimpleList<ExtraDataList*>();
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
    if (pExisting &&
        (!pExisting->pExtraObjectList || pExisting->pExtraObjectList->IsEmpty()) &&
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
        MEM_CONTEXT_INVENTORY,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp",
        5045
    );
    float fBestDesirability = -std::numeric_limits<float>::max();
    TESBoundObject* pBestAmmo = nullptr;
    TESBoundObject* pBestAddedAmmo = nullptr;
    // The base container's ammo: kept unless the changes removed all of it (a negative entry always counts).
    // Ammo is held as TESBoundObject, its primary base, at the address the TESAmmo cast returns.
    for (BSSimpleList<ContainerObject*>* pNode = GetOriginalContainer()->GetObjectList(); pNode;
         pNode = pNode->GetNext()) {
        ContainerObject* pEntry = pNode->GetItem();
        TESBoundObject* pAmmo = nullptr;
        if (pEntry) {
            pAmmo = dynamic_cast<TESAmmo*>(pEntry->pObj);
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
            pAmmo = dynamic_cast<TESAmmo*>(pItem->pContainerObj);
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
        pBest = new ItemChange;
    }
    if (pChange) {
        pBest->pContainerObj = pChange->GetContainerObject();
        if (pChange->GetExtraDataList() && pChange->pExtraObjectList->GetItem()) {
            pBest->pExtraObjectList = new BSSimpleList<ExtraDataList*>();
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
        MEM_CONTEXT_INVENTORY,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp",
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
    // the calls are kept deliberately.
    rActorValues.GetClampedActorFloatValue(ActorValue::LUCK);
    rActorValues.GetClampedActorValue(ActorValue::STRENGTH);
    if (pActor) {
        pActor->GetFatiguePercentage();
    }
    // The condition carries over to candidates without instance data.
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
        // Asks again whether the reference is an actor and drops the answer.
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
                const float fScore = CombatFormulas::CalcCombatWeaponDPS(
                    pActorForm,
                    pWeapon,
                    fCondition,
                    nullptr,
                    pItem,
                    pAmmo
                );
                if (fBestScore < fScore) {
                    fBestScore = fScore;
                    if (pBest) {
                        delete pBest;
                    }
                    pBest = new ItemChange;
                    pBest->pContainerObj = pWeapon;
                    pBest->pExtraObjectList = new BSSimpleList<ExtraDataList*>();
                    pBest->pExtraObjectList->AddHead(pExtra);
                }
            }
            continue;
        }
        const float
            fScore = CombatFormulas::CalcCombatWeaponDPS(pActorForm, pWeapon, fCondition, nullptr, pItem, pAmmo);
        if (fBestScore < fScore) {
            fBestScore = fScore;
            if (pBest) {
                delete pBest;
            }
            pBest = new ItemChange;
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
        // kept.
        const float fStartScore = -1.0f;
        if (fStartScore < -1.0) {
            continue;
        }
        rActorValues.GetActorFloatValue(pWeapon->GetWeaponSkill());
        static_cast<TESAttackDamageForm&>(*pWeapon).GetAttackDamage();
        if (pItem && pItem->pExtraObjectList && pItem->pExtraObjectList->GetItem()) {
            for (BSSimpleList<ExtraDataList*>* pExtraNode = pItem->pExtraObjectList;
                 pExtraNode && pExtraNode->GetItem();
                 pExtraNode = pExtraNode->GetNext()) {
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
                const float fScore = CombatFormulas::CalcCombatWeaponDPS(
                    pActorForm,
                    pWeapon,
                    fCondition,
                    nullptr,
                    pItem,
                    pAmmo
                );
                if (fBestScore < fScore) {
                    fBestScore = fScore;
                    if (pBest) {
                        delete pBest;
                    }
                    pBest = new ItemChange;
                    pBest->pContainerObj = pWeapon;
                    pBest->pExtraObjectList = new BSSimpleList<ExtraDataList*>();
                    pBest->pExtraObjectList->AddHead(pExtra);
                }
            }
            continue;
        }
        const float
            fScore = CombatFormulas::CalcCombatWeaponDPS(pActorForm, pWeapon, fCondition, nullptr, pItem, pAmmo);
        if (fBestScore < fScore) {
            fBestScore = fScore;
            if (pBest) {
                delete pBest;
            }
            pBest = new ItemChange;
            pBest->pContainerObj = pWeapon;
        }
    }
    afvalue = CombatFormulas::ChooseMeleeWeapon(fBestScore, 0, false);
    // Explosives and thrown weapons are chosen as a stack: the change takes the inventory's count.
    if (pBest) {
        TESObjectWEAP* pWeapon = static_cast<TESObjectWEAP*>(pBest->GetContainerObject());
        if (pWeapon->GetWeaponType() == WEAPON_ONE_HAND_GRENADE ||
            pWeapon->GetWeaponType() == WEAPON_ONE_HAND_MINE ||
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
        MEM_CONTEXT_INVENTORY,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp",
        5694
    );
    float fBestScore = -std::numeric_limits<float>::max();
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
                 pExtraNode && pExtraNode->GetItem();
                 pExtraNode = pExtraNode->GetNext()) {
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
                    static_cast<uint16_t>(static_cast<int32_t>(pArmor->GetArmorRating())),
                    fHealth
                );
                const float fScore = pArmor->GetDamageThreshold() + fRating;
                if (fBestScore < fScore) {
                    fBestScore = fScore;
                    if (pBest) {
                        delete pBest;
                    }
                    pBest = new ItemChange;
                    pBest->pContainerObj = pArmor;
                    pBest->pExtraObjectList = new BSSimpleList<ExtraDataList*>();
                    pBest->pExtraObjectList->AddHead(pExtra);
                }
            }
            continue;
        }
        const float fHealth = static_cast<float>(TESHealthForm::GetFormHealth(pArmor));
        const float fRating = CombatFormulas::CalcArmorRating(
            static_cast<uint16_t>(static_cast<int32_t>(pArmor->GetArmorRating())),
            fHealth
        );
        const float fScore = pArmor->GetDamageThreshold() + fRating;
        if (fBestScore < fScore) {
            fBestScore = fScore;
            if (pBest) {
                delete pBest;
            }
            pBest = new ItemChange;
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
                 pExtraNode && pExtraNode->GetItem();
                 pExtraNode = pExtraNode->GetNext()) {
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
                    static_cast<uint16_t>(static_cast<int32_t>(pArmor->GetArmorRating())),
                    fHealth
                );
                const float fScore = pArmor->GetDamageThreshold() + fRating;
                if (fBestScore < fScore) {
                    fBestScore = fScore;
                    if (pBest) {
                        delete pBest;
                    }
                    pBest = new ItemChange;
                    pBest->pContainerObj = pArmor;
                    pBest->pExtraObjectList = new BSSimpleList<ExtraDataList*>();
                    pBest->pExtraObjectList->AddHead(pExtra);
                }
            }
            continue;
        }
        const float fHealth = static_cast<float>(TESHealthForm::GetFormHealth(pArmor));
        const float fRating = CombatFormulas::CalcArmorRating(
            static_cast<uint16_t>(static_cast<int32_t>(pArmor->GetArmorRating())),
            fHealth
        );
        const float fScore = pArmor->GetDamageThreshold() + fRating;
        if (fBestScore < fScore) {
            fBestScore = fScore;
            if (pBest) {
                delete pBest;
            }
            pBest = new ItemChange;
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
        MEM_CONTEXT_INVENTORY,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp",
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
            pItem = new ItemChange(pFood, 0);
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
    ItemChange* pItem = nullptr;
    while (pkFirst && pkFirst->m_item) {
        pItem = pkFirst->m_item;
        ExtraDataList* pExtra = nullptr;
        while (pItem->pExtraObjectList && pItem->pExtraObjectList->m_item) {
            pExtra = pItem->pExtraObjectList->m_item;
            pItem->pExtraObjectList->RemoveHead();
            TESObjectREFR* pOriginalRef = nullptr;
            if (pExtra) {
                pOriginalRef = pExtra->GetOriginalReference();
            }
            if (pOriginalRef) {
                if (pOriginalRef->HasContainer()) {
                    pExtra->RemoveOriginalReferenceExtra();
                    if (pExtra->ItemsInList() < 2 && pExtra->GetCount()) {
                        pExtra->RemoveCount();
                    }

                    pItem->pExtraObjectList->Remove(pExtra);
                    InventoryChanges* pChangeRef = pOriginalRef->m_Extra.GetContainerChanges();
                    bool bDelete = true;
                    if (pChangeRef) {
                        ItemChange* pTempItem = pChangeRef->GetObjectInList(pItem->pContainerObj, true, 0);
                        if (pTempItem && pTempItem->pExtraObjectList) {
                            BSSimpleList<ExtraDataList*>* pTempObj = pTempItem->pExtraObjectList;
                            while (pTempObj && pTempObj->m_item && bDelete) {
                                if (pTempObj->m_item == pExtra) {
                                    bDelete = false;
                                }
                                else {
                                    pTempObj = pTempObj->m_pkNext;
                                }
                            }
                        }
                    }
                    pChangeRef = pPlayer->m_Extra.GetContainerChanges();
                    if (pChangeRef) {
                        ItemChange* pTempItem = pChangeRef->GetObjectInList(pItem->pContainerObj, true, 0);
                        if (pTempItem && pTempItem->pExtraObjectList) {
                            BSSimpleList<ExtraDataList*>* pTempObj = pTempItem->pExtraObjectList;
                            while (pTempObj && pTempObj->m_item && bDelete) {
                                if (pTempObj->m_item == pExtra) {
                                    bDelete = false;
                                }
                                else {
                                    pTempObj = pTempObj->m_pkNext;
                                }
                            }
                        }
                    }
                    if (bDelete) {
                        if (pExtra) {
                            delete pExtra;
                        }
                    }
                    else {
                        pExtra = nullptr;
                    }
                }
                else {
                    pExtra->RemoveOriginalReferenceExtra();
                    if (!pExtra->GetScript() && pExtra) {
                        delete pExtra;
                    }

                    pExtra = nullptr;
                }
            }
            else {
                pExtra = nullptr;
            }
        }
        pkFirst->Remove(pItem);
        if (pItem) {
            delete pItem;
        }
        pItem = nullptr;
        pkFirst = pkHead;
    }
}

float InventoryChanges::GetInventoryWeight(bool abIsHardcore) {
    int32_t fModWeightMult;
    int32_t fBaseWeightMult;
    int32_t fWeightMult;
    int32_t fPackRat;
    Actor* pActor = nullptr;
    if (pRef->IsActor()) {
        pActor = static_cast<Actor*>(pRef);
    }
    bool bHasLightItemsReduced = false;
    *reinterpret_cast<float*>(&fPackRat) = 0.0;
    BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::HAS_LIGHT_ITEMS_WEIGH_NOTHING, pActor, &fPackRat);
    float fPackRatMod = fPackRatModifier.Float();
    float fPackRatThresh = fPackRatThreshold.Float();
    if (*reinterpret_cast<float*>(&fPackRat) > 0.0) {
        bHasLightItemsReduced = true;
    }
    if (fcontainerweight == -1.0) {
        float fAmount = 0.0f;
        float fWeight = 0.0f;
        float fModWeight = 0.0f;
        for (BSSimpleList<ContainerObject*>* pList = &GetOriginalContainer()->objectList; pList && !pList->IsEmpty();
             pList = pList->m_pkNext) {
            TESBoundObject* pObj = pList->m_item->pObj;
            ItemChange* pItem = GetObjectInList(pObj, true, 0);
            if (!pObj->GetQuestObject()) {
                if (pObj->GetFormType() == WEAP_ID && pItem) {
                    bool bModActive = pItem->HasModEffectActive(4);
                    fWeight = static_cast<TESObjectWEAP*>(pObj)->TESObjectWEAP::GetFormWeight(bModActive);
                    if (fWeight >= 10.0) {
                        *reinterpret_cast<float*>(&fWeightMult) = 1.0f;
                        BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::NOT_YET_IMPLEMENTED, pPlayer, &fWeightMult);
                        fWeight = fWeight * *reinterpret_cast<float*>(&fWeightMult);
                    }
                }
                else {
                    fWeight = TESWeightForm::GetFormWeight(pObj, abIsHardcore);
                }
                if (fWeight == -1.0) {
                    fWeight = 0.0f;
                }
                if (bHasLightItemsReduced && fPackRatThresh >= static_cast<double>(fWeight)) {
                    fWeight = fWeight * fPackRatMod;
                }
                if (pItem) {
                    uint32_t uiNumber = pItem->iNumber;
                    if (pList->m_item->i_count + uiNumber) {
                        int32_t iCount = pList->m_item->i_count + uiNumber;
                        fAmount = static_cast<double>(iCount) * fWeight + fAmount;
                    }
                }
                else {
                    fAmount = static_cast<double>(pList->m_item->i_count) * fWeight + fAmount;
                }
            }
        }
        for (BSSimpleList<ItemChange*>* pInvList = pListofChanges; pInvList && !pInvList->IsEmpty() && pInvList->m_item;
             pInvList = pInvList->m_pkNext) {
            bool bIsWeapon = false;
            ItemChange* pItem = pInvList->m_item;
            TESBoundObject* pObj = pItem->pContainerObj;
            if (pObj) {
                if ((!GetOriginalContainer() || !GetOriginalContainer()->HasObject(pObj)) && !pObj->GetQuestObject()) {
                    if (pObj->GetFormType() == WEAP_ID) {
                        bIsWeapon = true;
                        bool bModActive = pItem->HasModEffectActive(4);
                        fModWeight = static_cast<TESObjectWEAP*>(pObj)->TESObjectWEAP::GetFormWeight(bModActive);
                        fWeight = TESWeightForm::GetFormWeight(pObj, abIsHardcore);
                        if (fWeight >= 10.0) {
                            *reinterpret_cast<float*>(&fBaseWeightMult) = 1.0f;
                            BGSEntryPoint::HandleEntryPoint(
                                BGSEntryPoint::NOT_YET_IMPLEMENTED,
                                pPlayer,
                                &fBaseWeightMult
                            );
                            fWeight = fWeight * *reinterpret_cast<float*>(&fBaseWeightMult);
                        }
                        if (fModWeight >= 10.0) {
                            *reinterpret_cast<float*>(&fModWeightMult) = 1.0f;
                            BGSEntryPoint::HandleEntryPoint(
                                BGSEntryPoint::NOT_YET_IMPLEMENTED,
                                pPlayer,
                                &fModWeightMult
                            );
                            fModWeight = fModWeight * *reinterpret_cast<float*>(&fModWeightMult);
                        }
                    }
                    else {
                        fWeight = TESWeightForm::GetFormWeight(pObj, abIsHardcore);
                    }
                    if (fWeight > 0.0) {
                        if (bHasLightItemsReduced && fPackRatThresh >= static_cast<double>(fWeight)) {
                            fWeight = fWeight * fPackRatMod;
                        }
                        if (bHasLightItemsReduced && fPackRatThresh >= static_cast<double>(fModWeight)) {
                            fModWeight = fModWeight * fPackRatMod;
                        }
                        TESBoundObject* pArmorObj = nullptr;
                        if (pObj && pObj->GetFormType() == ARMO_ID) {
                            pArmorObj = pObj;
                        }

                        int32_t iCount = pItem->iNumber;
                        if (pItem->pExtraObjectList && bIsWeapon && pItem->GetExtraTotalCount(true) > 0) {
                            TESObjectWEAP* pWeap = static_cast<TESObjectWEAP*>(pItem->pContainerObj);
                            int32_t iModCount = 0;
                            for (ExtraDataList* pExtra : *pItem->pExtraObjectList) {
                                if (pExtra && pExtra->HasWeaponMods()) {
                                    if (pExtra->GetWeaponModSlotActive(1) &&
                                        pWeap->GetModEffect(1u) == WEAPON_MOD_DECREASE_WEIGHT) {
                                        iModCount += pExtra->GetCount();
                                    }
                                    if (pExtra->GetWeaponModSlotActive(2) &&
                                        pWeap->GetModEffect(2u) == WEAPON_MOD_DECREASE_WEIGHT) {
                                        iModCount += pExtra->GetCount();
                                    }
                                    if (pExtra->GetWeaponModSlotActive(4) &&
                                        pWeap->GetModEffect(4u) == WEAPON_MOD_DECREASE_WEIGHT) {
                                        iModCount += pExtra->GetCount();
                                    }
                                }
                            }
                            fWeight = static_cast<double>(iCount - iModCount) * fWeight +
                                static_cast<double>(iModCount) * fModWeight;
                        }
                        else if (bIsWeapon) {
                            fWeight = static_cast<double>(iCount) * fWeight;
                        }
                        if (pActor && pArmorObj && pItem->GetWorn(false) && iCount > 0) {
                            fAmount = fAmount + fWeight;
                            --iCount;
                        }

                        if (iCount <= 0 || bIsWeapon) {
                            if (iCount) {
                                fAmount = fAmount + fWeight;
                            }
                        }
                        else {
                            fAmount = static_cast<double>(iCount) * fWeight + fAmount;
                        }
                    }
                }
            }
        }
        fcontainerweight = fAmount;
        if (pActor) {
            float fChange = fcontainerweight - fpreviousContainerWeight;
            Actor::InventoryWeightModifiedCallback(
                pActor,
                ActorValue::INVENTORY_WEIGHT,
                fpreviousContainerWeight,
                fChange,
                nullptr
            );
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
    bool bRemoved;
    bool bExtraDeleted;
    TESBoundObject* pObj = nullptr;
    float fValue = 0.0f;
    bool bQuest = false;
    bool bUseFilteredList = apDoNotRemoveList != nullptr;
    BSSimpleList<ContainerObject*>* pList = &GetOriginalContainer()->objectList;
    while (pList && !pList->IsEmpty()) {
        pObj = pList->m_item->pObj;
        TESLevItem* pLeveled = dynamic_cast<TESLevItem*>(pList->m_item->pObj);
        TESBipedModelForm* pBipedForm = TESBipedModelForm::GetFormAsBipedModel(pObj);
        if ((!bUseFilteredList || !apDoNotRemoveList->ListOfForms.IsInList(pObj)) &&
            (aiFormType <= -1 || !pObj || pObj->GetFormType() == aiFormType)) {
            if (pObj->GetQuestObject() && apRef == pPlayer) {
                ItemChange* pItem = GetObjectInList(pObj, true, 0);
                if (pItem && pItem->GetWorn(false)) {
                    TESBoundObject* pListObj = pList->m_item->pObj;
                    TESContainer* pOrigContainer = GetOriginalContainer();
                    int32_t iOrigCount = pOrigContainer->GetObjectCount(pListObj);
                    uint32_t uiNumber = pItem->iNumber;
                    bRemoved = false;
                    RemoveWorn(
                        &bRemoved,
                        pObj,
                        iOrigCount + uiNumber,
                        static_cast<Actor*>(apRef),
                        nullptr,
                        false,
                        false,
                        nullptr,
                        nullptr
                    );
                }
            }
            else if (pObj && !pLeveled && (!pBipedForm || pBipedForm->GetPlayable())) {
                ItemChange* pItem = GetObjectInList(pObj, true, 0);
                if (!pItem || !pItem->HasLeveledExtra()) {
                    int32_t iAmount = 0;
                    TESBoundObject* pListObj = pList->m_item->pObj;
                    TESContainer* pOrigContainer = GetOriginalContainer();
                    int32_t iOrigCount = pOrigContainer->GetObjectCount(pListObj);
                    if (iOrigCount < 0) {
                        iOrigCount = -iOrigCount;
                    }
                    if (pItem) {
                        uint32_t uiNumber = pItem->iNumber;
                        iAmount += iOrigCount + uiNumber;
                    }
                    else {
                        iAmount += iOrigCount;
                    }
                    if (iAmount > 0) {
                        int32_t iFormValue = TESValueForm::GetFormValue(pObj);
                        fValue = static_cast<double>(iAmount * iFormValue) + fValue;
                        if (pItem && pItem->pExtraObjectList && pItem->pExtraObjectList->ItemsInList()) {
                            BSSimpleList<ExtraDataList*>* pListExtra = pItem->pExtraObjectList;
                            if (pListExtra && pListExtra->m_item) {
                                bool bFlag = false;
                                while (pListExtra && pListExtra->m_item && !bFlag) {
                                    ExtraDataList* pExtra = pListExtra->m_item;
                                    if (pExtra->ItemsInList()) {
                                        uint32_t uiWornNumber;
                                        Actor* pActor;
                                        bool bExtraBad = false;
                                        if (static_cast<uint32_t>(pItem->pExtraObjectList->ItemsInList()) > 1) {
                                            bExtraBad = true;
                                        }
                                        else {
                                            bFlag = true;
                                        }
                                        TESForm* pOwner = nullptr;
                                        if (apRef->IsActor()) {
                                            TESBoundObject* pCompObj = apRef->m_Extra.GetLevCreaOriginalBase();
                                            if (!pCompObj) {
                                                pCompObj = apRef->GetObjectReference();
                                            }
                                            pOwner = pCompObj;
                                        }
                                        else {
                                            pOwner = apRef->GetOwner();
                                        }
                                        if ((abStealing || abowner) && !TESContainer::IsGold(pObj)) {
                                            if (!pExtra->GetOwner()) {
                                                pExtra->SetOwnership(pOwner);
                                            }
                                        }
                                        else if (abRemoveOwnership) {
                                            int32_t iNumb = pExtra->ItemsInList();
                                            if (pExtra->GetOwner() &&
                                                ((pExtra->GetCount() > 1 && iNumb <= 2) || iNumb <= 1)) {
                                                pExtra->RemoveOwnership();
                                                pExtra = nullptr;
                                            }
                                            else {
                                                pExtra->RemoveOwnership();
                                            }
                                        }
                                        int16_t sCount = pExtra->GetCount();
                                        iAmount -= sCount;
                                        if (!pItem->GetWorn(false) ||
                                            (bRemoved = false,
                                             bExtraDeleted = false,
                                             pActor = apRef->IsActor() ? static_cast<Actor*>(apRef) : nullptr,
                                             uiWornNumber = pItem->iNumber,
                                             RemoveWorn(
                                                 &bRemoved,
                                                 pObj,
                                                 uiWornNumber,
                                                 pActor,
                                                 pExtra,
                                                 false,
                                                 false,
                                                 pItem,
                                                 &bExtraDeleted
                                             ),
                                             !bExtraDeleted)) {
                                            RemoveItem(
                                                apRef,
                                                pObj,
                                                abStealing,
                                                pExtra->GetCount(),
                                                pExtra,
                                                false,
                                                apStolenContainer,
                                                nullptr,
                                                nullptr,
                                                true,
                                                false,
                                                pItem
                                            );
                                        }
                                        if (bFlag) {
                                            pItem = nullptr;
                                            pListExtra = nullptr;
                                        }
                                        else {
                                            pListExtra = bExtraBad ? pItem->pExtraObjectList : pListExtra->m_pkNext;
                                        }
                                    }
                                    else {
                                        pListExtra->RemoveHead();
                                        if (pListExtra->IsEmpty()) {
                                            pListExtra->RemoveAll();
                                            if (pListExtra) {
                                                delete pListExtra;
                                            }
                                            pListExtra = nullptr;
                                            pItem->pExtraObjectList = nullptr;
                                        }
                                        else {
                                            pListExtra = pItem->pExtraObjectList;
                                        }
                                    }
                                }
                            }
                        }

                        if (iAmount > 0) {
                            if (abStealing || abowner) {
                                pItem = new ItemChange(pObj, iAmount);
                                TESBoundObject* pCompObj = apRef->m_Extra.GetLevCreaOriginalBase();
                                if (!pCompObj) {
                                    pCompObj = apRef->GetObjectReference();
                                }
                                pItem->SetItemOwnership(pCompObj);
                                ExtraDataList* pExtra = pItem->pExtraObjectList->m_item;
                                pExtra->SetCount(iAmount);
                                RemoveItem(
                                    apRef,
                                    pObj,
                                    abStealing,
                                    pExtra->GetCount(),
                                    pExtra,
                                    false,
                                    apStolenContainer,
                                    nullptr,
                                    nullptr,
                                    true,
                                    false,
                                    nullptr
                                );
                            }
                            else {
                                RemoveItem(
                                    apRef,
                                    pObj,
                                    false,
                                    iAmount,
                                    nullptr,
                                    false,
                                    apStolenContainer,
                                    nullptr,
                                    nullptr,
                                    true,
                                    false,
                                    nullptr
                                );
                            }
                            if (apStolenContainer == pPlayer) {
                                if (pObj && !abSilent) {
                                    BSStringT<char> kMessage;
                                    if (iAmount <= 1) {
                                        const char* pAddText = sAddItemtoInventory.String();
                                        kMessage.SPrintF("%s %s", TESFullName::GetFullName(pObj), pAddText);
                                    }
                                    else {
                                        const char* pAddText = sAddItemtoInventory.String();
                                        const char* pPlural = sPlural.String();
                                        const char* pFullName = TESFullName::GetFullName(pObj);
                                        kMessage.SPrintF("%i %s%s %s", iAmount, pFullName, pPlural, pAddText);
                                    }
                                    switch (pObj->GetFormType()) {
                                        case ARMO_ID: {
                                            const char* pSoundName = pPlayer->GetPickUpSoundName(pObj, true, false);
                                            Interface::ShowMessage(
                                                kMessage.pString,
                                                0,
                                                "Interface\\Icons\\Message "
                                                "Icons\\glow_message_vaultboy_neutral.dds",
                                                pSoundName,
                                                2.0f,
                                                false
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
                                            const char* pSoundName = pPlayer->GetPickUpSoundName(pObj, true, false);
                                            Interface::ShowMessage(
                                                kMessage.pString,
                                                0,
                                                "Interface\\Icons\\Message "
                                                "Icons\\glow_message_vaultboy_neutral.dds",
                                                pSoundName,
                                                2.0f,
                                                false
                                            );
                                            break;
                                        }
                                        default: {
                                            break;
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

        pList = pList->m_pkNext;
    }
    BSSimpleList<ItemChange*>* pListInv = pListofChanges;
    while (pListInv && !pListInv->IsEmpty()) {
        TESContainer* pOrigContainer;
        TESObject* pContainerObj;
        TESBipedModelForm* pBipedForm;
        char cItemRemoved = 0;
        ItemChange* pInventoryItem = pListInv->m_item;
        if ((bUseFilteredList && apDoNotRemoveList->ListOfForms.IsInList(pInventoryItem->pContainerObj)) ||
            (aiFormType > -1 && pInventoryItem && pInventoryItem->pContainerObj->GetFormType() != aiFormType) ||
            !pInventoryItem ||
            ((pBipedForm = TESBipedModelForm::GetFormAsBipedModel(pInventoryItem->pContainerObj)) != nullptr &&
             !pBipedForm->GetPlayable()) ||
            (!pInventoryItem->HasLeveledExtra() &&
             GetOriginalContainer() &&
             (pContainerObj = pInventoryItem->pContainerObj,
              pOrigContainer = GetOriginalContainer(),
              pOrigContainer->HasObject(pContainerObj)))) {
            pListInv = pListInv->m_pkNext;
        }
        else {
            if (pInventoryItem->pContainerObj->GetQuestObject() && apRef == pPlayer) {
                if (pInventoryItem && pInventoryItem->GetWorn(false)) {
                    uint32_t uiNumber = pInventoryItem->iNumber;
                    bRemoved = false;
                    bool bLeft = pInventoryItem->GetWorn(true);
                    RemoveWorn(
                        &bRemoved,
                        pInventoryItem->pContainerObj,
                        uiNumber,
                        static_cast<Actor*>(apRef),
                        nullptr,
                        bLeft,
                        pInventoryItem != nullptr,
                        nullptr,
                        nullptr
                    );
                }
            }
            else if (pInventoryItem && pInventoryItem->iNumber > 0) {
                if (!bQuest || !pInventoryItem->pContainerObj->GetQuestObject()) {
                    pObj = pInventoryItem->pContainerObj;
                    int32_t iFormValue = TESValueForm::GetFormValue(pObj);
                    fValue = static_cast<double>(pInventoryItem->iNumber * iFormValue) + fValue;
                    bool bFlag = false;
                    int32_t iOriginalCount = 0;
                    if (GetOriginalContainer() && pInventoryItem) {
                        pContainerObj = pInventoryItem->pContainerObj;
                        pOrigContainer = GetOriginalContainer();
                        iOriginalCount = pOrigContainer->GetObjectCount(pContainerObj);
                    }

                    int32_t iNumberRemove = pInventoryItem->iNumber + iOriginalCount;
                    BSSimpleList<ExtraDataList*>* pListExtra = pInventoryItem->pExtraObjectList;
                    if (!pListExtra || !pListExtra->m_item) {
                        ExtraDataList* pNewExtra = nullptr;
                        TESForm* pOwner = nullptr;
                        if (apRef->IsActor()) {
                            TESBoundObject* pCompObj = apRef->m_Extra.GetLevCreaOriginalBase();
                            if (!pCompObj) {
                                pCompObj = apRef->GetObjectReference();
                            }
                            pOwner = pCompObj;
                        }
                        else {
                            pOwner = apRef->GetOwner();
                        }
                        if ((abStealing || abowner) && !TESContainer::IsGold(pObj)) {
                            pNewExtra = new ExtraDataList;
                            pNewExtra->SetOwnership(pOwner);
                        }
                        int32_t iCount = pInventoryItem->iNumber;
                        uint32_t uiNumber = pInventoryItem->iNumber;
                        RemoveItem(
                            apRef,
                            pObj,
                            abStealing,
                            uiNumber,
                            pNewExtra,
                            false,
                            apStolenContainer,
                            nullptr,
                            nullptr,
                            true,
                            false,
                            cItemRemoved == 0 ? pInventoryItem : nullptr
                        );
                        if (apStolenContainer == pPlayer) {
                            if (pObj && !abSilent) {
                                BSStringT<char> kMessage;
                                if (iCount <= 1) {
                                    const char* pAddText = sAddItemtoInventory.String();
                                    kMessage.SPrintF("%s %s", TESFullName::GetFullName(pObj), pAddText);
                                }
                                else {
                                    const char* pAddText = sAddItemtoInventory.String();
                                    const char* pPlural = sPlural.String();
                                    const char* pFullName = TESFullName::GetFullName(pObj);
                                    kMessage.SPrintF("%i %s%s %s", iCount, pFullName, pPlural, pAddText);
                                }
                                switch (pObj->GetFormType()) {
                                    case ARMO_ID: {
                                        const char* pSoundName = pPlayer->GetPickUpSoundName(pObj, true, false);
                                        Interface::ShowMessage(
                                            kMessage.pString,
                                            0,
                                            "Interface\\Icons\\Message "
                                            "Icons\\glow_message_vaultboy_neutral.dds",
                                            pSoundName,
                                            2.0f,
                                            false
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
                                        const char* pSoundName = pPlayer->GetPickUpSoundName(pObj, true, false);
                                        Interface::ShowMessage(
                                            kMessage.pString,
                                            0,
                                            "Interface\\Icons\\Message "
                                            "Icons\\glow_message_vaultboy_neutral.dds",
                                            pSoundName,
                                            2.0f,
                                            false
                                        );
                                        break;
                                    }
                                    default: {
                                        break;
                                    }
                                }
                            }

                            Interface::UpdateInventory();
                        }
                        pListInv = pListofChanges;
                        continue;
                    }
                    while (true) {
                        if (!pListExtra || !pListExtra->m_item || bFlag) {
                            break;
                        }
                        ExtraDataList* pExtra = pListExtra->m_item;
                        if (pExtra->GetCount() < 1) {
                            pListExtra = pListExtra->m_pkNext;
                            if (!pListExtra) {
                                RemoveItem(
                                    apRef,
                                    pObj,
                                    abStealing,
                                    pInventoryItem->iNumber,
                                    nullptr,
                                    false,
                                    apStolenContainer,
                                    nullptr,
                                    nullptr,
                                    true,
                                    false,
                                    pInventoryItem
                                );
                                cItemRemoved = 1;
                                if (apStolenContainer == pPlayer) {
                                    if (pObj && !abSilent) {
                                        BSStringT<char> kMessage;
                                        if (iNumberRemove <= 1) {
                                            const char* pAddText = sAddItemtoInventory.String();
                                            kMessage.SPrintF("%s %s", TESFullName::GetFullName(pObj), pAddText);
                                        }
                                        else {
                                            const char* pAddText = sAddItemtoInventory.String();
                                            const char* pPlural = sPlural.String();
                                            const char* pFullName = TESFullName::GetFullName(pObj);
                                            kMessage.SPrintF(
                                                "%i %s%s %s",
                                                pInventoryItem->iNumber,
                                                pFullName,
                                                pPlural,
                                                pAddText
                                            );
                                        }
                                        switch (pObj->GetFormType()) {
                                            case ARMO_ID: {
                                                const char* pSoundName = pPlayer->GetPickUpSoundName(pObj, true, false);
                                                Interface::ShowMessage(
                                                    kMessage.pString,
                                                    0,
                                                    "Interface\\Icons\\Message "
                                                    "Icons\\glow_message_vaultboy_neutral.dds",
                                                    pSoundName,
                                                    2.0f,
                                                    false
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
                                                const char* pSoundName = pPlayer->GetPickUpSoundName(pObj, true, false);
                                                Interface::ShowMessage(
                                                    kMessage.pString,
                                                    0,
                                                    "Interface\\Icons\\Message "
                                                    "Icons\\glow_message_vaultboy_neutral.dds",
                                                    pSoundName,
                                                    2.0f,
                                                    false
                                                );
                                                break;
                                            }
                                            default: {
                                                break;
                                            }
                                        }
                                    }

                                    Interface::UpdateInventory();
                                }
                                iNumberRemove = 0;
                                pListInv = pListofChanges;
                                break;
                            }
                            continue;
                        }
                        if (pExtra->ItemsInList()) {
                            if (static_cast<uint32_t>(pListExtra->ItemsInList()) <= 1) {
                                bFlag = true;
                            }
                            TESForm* pOwner = nullptr;
                            if (apRef->IsActor()) {
                                TESBoundObject* pCompObj = apRef->m_Extra.GetLevCreaOriginalBase();
                                if (!pCompObj) {
                                    pCompObj = apRef->GetObjectReference();
                                }
                                pOwner = pCompObj;
                            }
                            else {
                                pOwner = apRef->GetOwner();
                            }
                            if ((abStealing || abowner) && !TESContainer::IsGold(pObj)) {
                                if (!pExtra->GetOwner()) {
                                    pExtra->SetOwnership(pOwner);
                                }
                            }
                            else if (abRemoveOwnership) {
                                int32_t iNumb = pExtra->ItemsInList();
                                if (pExtra->GetOwner() && ((pExtra->GetCount() > 1 && iNumb <= 2) || iNumb <= 1)) {
                                    pExtra->RemoveOwnership();
                                    pExtra = nullptr;
                                }
                                else {
                                    pExtra->RemoveOwnership();
                                }
                            }
                            if (pExtra) {
                                int16_t sCount = pExtra->GetCount();
                                iNumberRemove -= sCount;
                                RemoveItem(
                                    apRef,
                                    pObj,
                                    abStealing,
                                    pExtra->GetCount(),
                                    pExtra,
                                    false,
                                    apStolenContainer,
                                    nullptr,
                                    nullptr,
                                    true,
                                    false,
                                    pInventoryItem
                                );
                                cItemRemoved = 1;
                                if (apStolenContainer == pPlayer) {
                                    if (pObj && !abSilent) {
                                        BSStringT<char> kMessage;
                                        if (iNumberRemove <= 1) {
                                            const char* pAddText = sAddItemtoInventory.String();
                                            kMessage.SPrintF("%s %s", TESFullName::GetFullName(pObj), pAddText);
                                        }
                                        else {
                                            const char* pAddText = sAddItemtoInventory.String();
                                            const char* pPlural = sPlural.String();
                                            const char* pFullName = TESFullName::GetFullName(pObj);
                                            kMessage.SPrintF(
                                                "%i %s%s %s",
                                                pExtra->GetCount(),
                                                pFullName,
                                                pPlural,
                                                pAddText
                                            );
                                        }
                                        switch (pObj->GetFormType()) {
                                            case ARMO_ID: {
                                                const char* pSoundName = pPlayer->GetPickUpSoundName(pObj, true, false);
                                                Interface::ShowMessage(
                                                    kMessage.pString,
                                                    0,
                                                    "Interface\\Icons\\Message "
                                                    "Icons\\glow_message_vaultboy_neutral.dds",
                                                    pSoundName,
                                                    2.0f,
                                                    false
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
                                                const char* pSoundName = pPlayer->GetPickUpSoundName(pObj, true, false);
                                                Interface::ShowMessage(
                                                    kMessage.pString,
                                                    0,
                                                    "Interface\\Icons\\Message "
                                                    "Icons\\glow_message_vaultboy_neutral.dds",
                                                    pSoundName,
                                                    2.0f,
                                                    false
                                                );
                                                break;
                                            }
                                            default: {
                                                break;
                                            }
                                        }
                                    }

                                    Interface::UpdateInventory();
                                }
                            }
                            pInventoryItem = GetObjectInList(pObj, true, 0);
                            if (iNumberRemove > 0 || pInventoryItem) {
                                if (iNumberRemove <= 0 && pInventoryItem) {
                                    pListInv = pListInv->m_pkNext;
                                }
                            }
                            else {
                                pListInv = pListofChanges;
                            }
                            if (!pInventoryItem || iNumberRemove <= 0) {
                                pListExtra = nullptr;
                                continue;
                            }
                        }
                        else {
                            pListExtra->RemoveHead();
                            if (pListExtra->IsEmpty()) {
                                pListExtra->RemoveAll();
                                if (pListExtra) {
                                    delete pListExtra;
                                }
                                pListExtra = nullptr;
                                pInventoryItem->pExtraObjectList = nullptr;
                                continue;
                            }
                        }
                        pListExtra = pInventoryItem->pExtraObjectList;
                    }
                    if (iNumberRemove > 0) {
                        RemoveItem(
                            apRef,
                            pObj,
                            abStealing,
                            iNumberRemove,
                            nullptr,
                            false,
                            apStolenContainer,
                            nullptr,
                            nullptr,
                            true,
                            false,
                            cItemRemoved == 0 ? pInventoryItem : nullptr
                        );
                        pListInv = pListofChanges;
                        if (apStolenContainer == pPlayer) {
                            if (pObj && !abSilent) {
                                BSStringT<char> kMessage;
                                if (iNumberRemove <= 1) {
                                    const char* pAddText = sAddItemtoInventory.String();
                                    kMessage.SPrintF("%s %s", TESFullName::GetFullName(pObj), pAddText);
                                }
                                else {
                                    const char* pAddText = sAddItemtoInventory.String();
                                    const char* pPlural = sPlural.String();
                                    const char* pFullName = TESFullName::GetFullName(pObj);
                                    kMessage.SPrintF("%i %s%s %s", iNumberRemove, pFullName, pPlural, pAddText);
                                }
                                switch (pObj->GetFormType()) {
                                    case ARMO_ID: {
                                        const char* pSoundName = pPlayer->GetPickUpSoundName(pObj, true, false);
                                        Interface::ShowMessage(
                                            kMessage.pString,
                                            0,
                                            "Interface\\Icons\\Message "
                                            "Icons\\glow_message_vaultboy_neutral.dds",
                                            pSoundName,
                                            2.0f,
                                            false
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
                                        const char* pSoundName = pPlayer->GetPickUpSoundName(pObj, true, false);
                                        Interface::ShowMessage(
                                            kMessage.pString,
                                            0,
                                            "Interface\\Icons\\Message "
                                            "Icons\\glow_message_vaultboy_neutral.dds",
                                            pSoundName,
                                            2.0f,
                                            false
                                        );
                                        break;
                                    }
                                    default: {
                                        break;
                                    }
                                }
                            }

                            Interface::UpdateInventory();
                        }
                    }
                    continue;
                }
            }

            pListInv = pListInv->m_pkNext;
        }
    }
    return fValue;
}

void InventoryChanges::RemoveItemType(ENUM_FORM_ID aeType, bool abStealing, int32_t ainumber) {
    int32_t iNum;
    fpreviousContainerWeight = fcontainerweight;
    fcontainerweight = -1.0f;
    iNum = 0;
    while (ainumber > 0) {
        TESBoundObject* pObject = GetObjectbyType(aeType, iNum);
        if (iNum > ainumber) {
            iNum = ainumber;
        }
        RemoveItem(nullptr, pObject, abStealing, iNum, nullptr, false, nullptr, nullptr, nullptr, true, false, nullptr);
        ainumber -= iNum;
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
    AutoMemContext tempMemContext(
        MEM_CONTEXT_INVENTORY,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp",
        3700
    );
    bcountdirty = true;
    fpreviousContainerWeight = fcontainerweight;
    fcontainerweight = -1.0f;
    TESObjectREFR* pDroppedRef = nullptr;
    if (pRef) {
        pRef->AddChange(CHANGE_REFR_INVENTORY);
    }
    if (apOtherContainer) {
        apOtherContainer->AddChange(CHANGE_REFR_INVENTORY);
    }
    TESContainer* pOrigContainer = GetOriginalContainer();
    ItemChange* pItem = apItemChange ? apItemChange : GetObjectInList(apObject, true, 0);
    ItemChange* pNewItem = nullptr;
    bool bReload = false;
    Actor* pAct = nullptr;
    if (apRef->IsActor() && apRef == pPlayer) {
        pAct = static_cast<Actor*>(apRef);
    }
    if (pAct && (abDropWorld || apOtherContainer)) {
        if (pAct->pCurrentProcess->GetCurrentAmmo()) {
            if (pAct->pCurrentProcess->GetCurrentAmmo()->pContainerObj == apObject) {
                bReload = true;
            }
        }
    }

    if ((abDropWorld || apOtherContainer) && apExtra && apExtra->GetExtraData(74) && ainumber == apExtra->GetCount()) {
        apExtra->RemoveHotKey();
    }

    if (apExtra && !apExtra->ItemsInList()) {
        if (pItem && pItem->pExtraObjectList) {
            pItem->pExtraObjectList->Remove(apExtra);
        }

        if (apExtra) {
            delete apExtra;
        }
        apExtra = nullptr;
    }

    int32_t iOrgCount = 0;
    if (apRef) {
        iOrgCount = pOrigContainer ? pOrigContainer->GetObjectCount(apObject) : 0;
        if (pItem && !pItem->GetWorn(false)) {
            int32_t iTotalCount = pItem->iNumber + iOrgCount;
            int32_t iExtraDefaultCount = pItem->GetExtraTotalDefaultCount();
            if (iTotalCount > pItem->GetExtraTotalCount(false) + iExtraDefaultCount && pItem->GetScript()) {
                iOrgCount = 0;
            }
        }
    }
    int32_t iNegativeCount = 0;
    if (iOrgCount < 0) {
        iNegativeCount = iOrgCount;
        iOrgCount = -iOrgCount;
    }
    if (iOrgCount < 0 && apOtherContainer != pPlayer) {
        return nullptr;
    }
    if ((pItem && pItem->iNumber && !(iOrgCount + pItem->iNumber)) || (!pItem && !iOrgCount)) {
        return pDroppedRef;
    }
    ExtraLeveledItem* pLevelExtra = nullptr;
    bool bUseBase = false;
    while (pItem) {
        if (ainumber <= 0) {
            break;
        }
        if (iOrgCount + pItem->iNumber <= 0) {
            break;
        }
        if (pNewItem) {
            delete pNewItem;
        }
        pNewItem = new ItemChange;
        pNewItem->pContainerObj = pItem->pContainerObj;
        BSSimpleList<ExtraDataList*>* pkFirst = pItem->pExtraObjectList;
        bool bFlag = true;
        if (((apExtra && apExtra->ItemsInList()) || abPreferStolen) && pkFirst && pkFirst->m_item) {
            while (pkFirst && pkFirst->m_item && bFlag) {
                ExtraDataList* pCurrent = pkFirst->m_item;
                ExtraDataList* pNewExtra = nullptr;
                if (apExtra == pCurrent || (!apExtra && abPreferStolen && pCurrent->GetOwner())) {
                    apExtra = nullptr;
                    bFlag = false;
                    int32_t iCount = pCurrent->GetCount();
                    if (pCurrent->GetWorn(false) && pCurrent->GetCount() <= ainumber) {
                        bool bExtraFlag = false;
                        if (pCurrent->ItemsInList() <= 1 ||
                            (pCurrent->ItemsInList() == 2 && pCurrent->GetCount() > 1) ||
                            (pCurrent->HasLeveledItem() && pCurrent->ItemsInList() == 2)) {
                            bExtraFlag = true;
                        }
                        bool bUnequipped = false;
                        if (!apRef->IsActor() || apRef->GetDisabled()) {
                            int16_t sCount = pCurrent->GetCount();
                            bUnequipped = apRef->RemoveWornItem(pItem->pContainerObj, sCount, pCurrent);
                        }
                        else {
                            Actor* pActor = static_cast<Actor*>(apRef);
                            if (pActor->GetCurrentAIProcess()) {
                                int32_t iExtraCount = pCurrent->GetCount();
                                bUnequipped = pActor->UnEquipObject(
                                    pItem->pContainerObj,
                                    iExtraCount,
                                    pCurrent,
                                    false,
                                    false,
                                    true
                                );
                            }
                        }
                        if (!bUnequipped || bExtraFlag) {
                            pCurrent = nullptr;
                        }
                        pItem = GetObjectInList(apObject, true, 0);
                        if (!pItem || !pItem->GetWorn(false)) {
                            pDroppedRef = RemoveItem(
                                apRef,
                                apObject,
                                abstealing,
                                ainumber,
                                pCurrent,
                                abDropWorld,
                                apOtherContainer,
                                apPoint,
                                apRotate,
                                true,
                                false,
                                nullptr
                            );
                        }
                        if (pItem && iCount > 1 && iCount - ainumber > 0 && pRef->IsActor()) {
                            static_cast<Actor*>(pRef)->Actor::EquipObject(
                                pItem->pContainerObj,
                                iCount - ainumber,
                                pCurrent,
                                true,
                                false,
                                false
                            );
                        }

                        if (pNewItem) {
                            delete pNewItem;
                        }
                        pNewItem = nullptr;
                        return pDroppedRef;
                    }
                    if (iCount > ainumber || pCurrent->GetLeveledItem()) {
                        pLevelExtra = pCurrent->GetLeveledItem();
                        int32_t iFormType = pItem->pContainerObj->GetFormType();
                        if (!pCurrent->GetLeveledItem() ||
                            !pCurrent->GetScript() ||
                            iFormType == 24 ||
                            iFormType == 26) {
                            if (pCurrent->HasLeveledItem() && ainumber == iCount) {
                                pItem->iNumber -= iCount;
                                pCurrent->SetCount(-static_cast<int16_t>(iCount));
                            }
                            else {
                                pItem->iNumber -= ainumber;
                                pCurrent->SetCount(iCount - ainumber);
                            }
                            pNewExtra = new ExtraDataList;
                            pNewExtra->CopyListForReference(pCurrent, false);
                            pNewExtra->RemoveLeveledItem();
                            pNewExtra->SetCount(ainumber);
                            if ((pNewExtra->ItemsInList() == 1 && pNewExtra->GetCount() > 1) ||
                                !pNewExtra->ItemsInList()) {
                                if (pNewExtra) {
                                    delete pNewExtra;
                                }
                                pNewExtra = nullptr;
                            }
                        }
                        else {
                            ExtraDataList* pLevel;
                            if (iFormType == 115) {
                                pCurrent->SetCount(1);
                            }
                            pNewExtra = pCurrent;
                            pItem->pExtraObjectList->Remove(pCurrent);
                            pLevel = new ExtraDataList;
                            pLevel->SetLevelItemExtra(pCurrent->GetLeveledItem()->iIndex);
                            pItem->pExtraObjectList->AddHead(pLevel);
                            pNewExtra->RemoveLeveledItem();
                            pItem->iNumber = -pCurrent->GetCount();
                        }
                        Script::SetActionFlag(apRef, pNewExtra, SCRIPT_ACTION_ONDROP);
                        iCount = ainumber;
                        ainumber = 0;
                    }
                    else {
                        pItem->iNumber -= iCount;
                        uint32_t uiNumber = pNewItem->iNumber;
                        pNewItem->iNumber = iCount + uiNumber;
                        pNewExtra = pCurrent;
                        ainumber -= iCount;
                        pItem->pExtraObjectList->Remove(pCurrent);
                        if (pCurrent && pCurrent->GetHotKey() >= 0 && pItem->iNumber > 0) {
                            ExtraDataList* pFirstExtra = pItem->pExtraObjectList ? pItem->pExtraObjectList->m_item
                                                                                 : nullptr;
                            SetHotKeyItem(pItem, pFirstExtra, pCurrent->GetHotKey());
                        }

                        pCurrent = nullptr;
                        Script::SetActionFlag(apRef, pNewExtra, SCRIPT_ACTION_ONDROP);
                        if (pNewExtra->IsExtraListEmpty()) {
                            if (pNewExtra) {
                                delete pNewExtra;
                            }
                            pNewExtra = nullptr;
                        }
                    }
                    if (pNewExtra && pNewExtra->GetWorn(false)) {
                        pNewExtra->RemoveWorn(false);
                        if (pNewExtra->IsExtraListEmpty()) {
                            if (pNewExtra) {
                                delete pNewExtra;
                            }
                            pNewExtra = nullptr;
                        }
                    }

                    if (pNewExtra && pNewExtra->ItemsInList()) {
                        if (!pNewItem->pExtraObjectList) {
                            pNewItem->pExtraObjectList = new BSSimpleList<ExtraDataList*>;
                        }
                        pNewItem->pExtraObjectList->AddHead(pNewExtra);
                    }

                    if (abDropWorld) {
                        if (pNewExtra && pNewExtra->GetReferencePointer()) {
                            if (pNewExtra) {
                                pDroppedRef = pNewExtra->GetReferencePointer();
                            }
                            else if (pCurrent) {
                                pDroppedRef = pCurrent->GetReferencePointer();
                            }
                            if (pDroppedRef) {
                                pDroppedRef->SetDelete(false);
                                pNewExtra->RemoveReferencePointer();
                                bool bDelete = pCurrent == nullptr;
                                float fScale = pNewExtra->GetScale();
                                pDroppedRef->SetScale(fScale);
                                pDroppedRef->m_Extra.CopyListForReference(pNewExtra, bDelete);
                                pDroppedRef->m_Extra.RemoveScale();
                                pDroppedRef->m_Extra.RemoveReferencePointer();
                                if (!pDroppedRef->m_Extra.GetStartingWorldOrCell()) {
                                    pDroppedRef->m_Extra.SetStartingWorldOrCellForRef(pDroppedRef);
                                    pDroppedRef->AddChange(CHANGE_OBJECT_EXTRA_ITEM_DATA);
                                }
                                DropItemIntoWorld(apRef, pItem->pContainerObj, iCount, pDroppedRef, apPoint, apRotate);
                                if (pNewItem) {
                                    delete pNewItem;
                                }
                                pNewItem = nullptr;
                                if (pNewExtra) {
                                    delete pNewExtra;
                                }
                                pNewExtra = nullptr;
                                bool bAddOwnership = false;
                                TESObjectCELL* pCell = apRef->GetParentCell();
                                if (!pDroppedRef->GetMyOwner() &&
                                    pCell &&
                                    pCell->GetOwner() &&
                                    !pCell->IsActorCellOwner(static_cast<Actor*>(apRef))) {
                                    bAddOwnership = true;
                                }

                                if (!bAddOwnership &&
                                    apRef->GetObjectReference()->GetFormType() == NPC__ID &&
                                    static_cast<TESActorBase*>(apRef->GetObjectReference())->TESActorBase::GetSex() ==
                                        TESActorBase::SEX_FEMALE) {
                                    TESBipedModelForm* pBipedModel = dynamic_cast<TESBipedModelForm*>(
                                        pDroppedRef->GetObjectReference()
                                    );
                                    if (pBipedModel) {
                                        TESModelTextureSwap* pFemaleModel = pBipedModel->GetWorldTESModel(
                                            TESActorBase::SEX_FEMALE
                                        );
                                        if (pFemaleModel != pBipedModel->GetWorldTESModel(TESActorBase::SEX_MALE)) {
                                            bAddOwnership = true;
                                        }
                                    }
                                }
                                if (bAddOwnership) {
                                    pDroppedRef->SetOwner(apRef->GetObjectReference());
                                }
                            }
                        }
                        else {
                            pDroppedRef = DropItemIntoWorld(
                                apRef,
                                pItem->pContainerObj,
                                iCount,
                                nullptr,
                                apPoint,
                                apRotate
                            );
                            pDroppedRef->AddChange(CHANGE_OBJECT_EXTRA_ITEM_DATA);
                            if (pNewExtra && pNewExtra->ItemsInList()) {
                                float fScale = pNewExtra->GetScale();
                                pDroppedRef->SetScale(fScale);
                                pDroppedRef->m_Extra.CopyListForReference(pNewExtra, true);
                                pDroppedRef->m_Extra.RemoveScale();
                            }

                            bool bAddOwnership = false;
                            TESObjectCELL* pCell = apRef->GetParentCell();
                            if (!pDroppedRef->GetMyOwner() &&
                                pCell &&
                                pCell->GetOwner() &&
                                !pCell->IsActorCellOwner(static_cast<Actor*>(apRef))) {
                                bAddOwnership = true;
                            }

                            if (!bAddOwnership &&
                                apRef->GetObjectReference()->GetFormType() == NPC__ID &&
                                static_cast<TESActorBase*>(apRef->GetObjectReference())->TESActorBase::GetSex() ==
                                    TESActorBase::SEX_FEMALE) {
                                TESBipedModelForm* pBipedModel = dynamic_cast<TESBipedModelForm*>(
                                    pDroppedRef->GetObjectReference()
                                );
                                if (pBipedModel) {
                                    TESModelTextureSwap* pFemaleModel = pBipedModel->GetWorldTESModel(
                                        TESActorBase::SEX_FEMALE
                                    );
                                    if (pFemaleModel != pBipedModel->GetWorldTESModel(TESActorBase::SEX_MALE)) {
                                        bAddOwnership = true;
                                    }
                                }
                            }
                            if (bAddOwnership) {
                                pDroppedRef->SetOwner(apRef->GetObjectReference());
                            }
                            if (pNewItem) {
                                delete pNewItem;
                            }
                            pNewItem = nullptr;
                            if (pNewExtra) {
                                delete pNewExtra;
                            }
                            pNewExtra = nullptr;
                        }
                    }
                    else if (apOtherContainer) {
                        ExtraDataList* pExtra = nullptr;
                        if (pNewItem->pExtraObjectList) {
                            for (ExtraDataList*& rpItem : pNewItem->pExtraObjectList->ItemsUntilNull()) {
                                TESForm* pOwner = nullptr;
                                pExtra = rpItem;
                                if (!pExtra->GetOwner()) {
                                    if (apRef->IsActor()) {
                                        TESBoundObject* pCompObj = apRef->m_Extra.GetLevCreaOriginalBase();
                                        if (!pCompObj) {
                                            pCompObj = apRef->GetObjectReference();
                                        }
                                        pOwner = pCompObj;
                                    }
                                    else {
                                        pOwner = apRef->GetOwner();
                                    }
                                    if (abstealing && !TESContainer::IsGold(pNewItem->pContainerObj)) {
                                        pExtra->SetOwnership(pOwner);
                                    }
                                }
                                apOtherContainer->AddObjecttoContainer(pNewItem->pContainerObj, pExtra, iCount);
                            }
                            if (pNewItem) {
                                delete pNewItem;
                            }
                            pNewItem = nullptr;
                        }
                        else {
                            TESForm* pOwner = nullptr;
                            if (apRef->IsActor()) {
                                TESBoundObject* pCompObj = apRef->m_Extra.GetLevCreaOriginalBase();
                                if (!pCompObj) {
                                    pCompObj = apRef->GetObjectReference();
                                }
                                pOwner = pCompObj;
                            }
                            else {
                                pOwner = apRef->GetOwner();
                            }
                            if (abstealing && !TESContainer::IsGold(pNewItem->pContainerObj)) {
                                pExtra = new ExtraDataList;
                                pExtra->SetOwnership(pOwner);
                                pExtra->SetCount(ainumber);
                            }
                            apOtherContainer->AddObjecttoContainer(pNewItem->pContainerObj, pExtra, iCount);
                            if (pNewItem) {
                                delete pNewItem;
                            }
                            pNewItem = nullptr;
                        }
                        bFlag = false;
                    }
                    else {
                        if (!Interface::ContainerIsBarter() &&
                            !Interface::ContainerIsRecipe() &&
                            !Interface::IsMenuIDVisible(Interface::ePipBoyRepair, 0) &&
                            !Interface::IsMenuIDVisible(Interface::eRepairServicesMenu, 0)) {
                            pNewItem->DeleteAllExtra();
                        }
                        if (pNewItem) {
                            delete pNewItem;
                        }
                        pNewItem = nullptr;
                    }
                }
                else {
                    pkFirst = pkFirst->m_pkNext;
                }
                if (ainumber > 0 && abPreferStolen) {
                    return RemoveItem(
                        apRef,
                        apObject,
                        abstealing,
                        ainumber,
                        nullptr,
                        abDropWorld,
                        apOtherContainer,
                        apPoint,
                        apRotate,
                        true,
                        false,
                        nullptr
                    );
                }
                if (ainumber > 0 && pDroppedRef) {
                    return pDroppedRef;
                }
                if (ainumber > 0 && !pkFirst) {
                    apExtra = nullptr;
                    break;
                }
            }
        }
        else {
            int32_t iCount = iOrgCount >= 0 ? pItem->iNumber + iOrgCount : pItem->iNumber - iOrgCount;
            if (iCount < 0 && iOrgCount >= 0) {
                return pDroppedRef;
            }
            int32_t iExtraCount = 0;
            if (pItem->pExtraObjectList) {
                int32_t iExtraTotalCount = pItem->GetExtraTotalCount(false);
                iExtraCount = pItem->GetWorn(false) + iExtraTotalCount;
            }
            int32_t iDefaultCount = iCount - iExtraCount;
            if (iCount - iExtraCount < 0) {
                iDefaultCount = 0;
            }
            ExtraDataList* pCurrent = nullptr;
            ExtraDataList* pNewExtra = nullptr;
            if (iDefaultCount > 0) {
                if (pItem->GetWorn(false)) {
                    for (BSSimpleList<ExtraDataList*>* pNode = pItem->pExtraObjectList; pNode && pNode->m_item;
                         pNode = pNode->GetNext()) {
                        ExtraDataList* pExtra = pNode->m_item;
                        int16_t sCount = pExtra->GetCount();
                        if (pExtra->GetWorn(false) && sCount > 1) {
                            pExtra->SetCount(sCount - ainumber);
                        }
                    }
                }
                else if (pItem->HasLeveledExtra()) {
                    for (BSSimpleList<ExtraDataList*>* pNode = pItem->pExtraObjectList; pNode && pNode->m_item;
                         pNode = pNode->GetNext()) {
                        ExtraDataList* pExtra = pNode->m_item;
                        int16_t sCount = pExtra->GetCount();
                        if (pExtra->HasLeveledItem() && sCount > 1) {
                            pExtra->SetCount(sCount - ainumber);
                        }
                    }
                }
                if (iDefaultCount < ainumber) {
                    if (pOrigContainer && pOrigContainer->GetObjectCount(apObject) > 0) {
                        pItem->iNumber -= iDefaultCount;
                    }

                    iCount = iDefaultCount;
                    ainumber -= iDefaultCount;
                    if (iOrgCount + pItem->iNumber >= 0) {
                        pItem->iNumber -= iCount;
                    }
                    else if (iOrgCount) {
                        pItem->iNumber = -iOrgCount;
                    }
                    else {
                        pItem->iNumber = 0;
                    }
                }
                else {
                    if (!iNegativeCount) {
                        pItem->iNumber -= ainumber;
                    }
                    iCount = ainumber;
                    ainumber = 0;
                }
                if (abDropWorld) {
                    pDroppedRef = DropItemIntoWorld(apRef, apObject, iCount, nullptr, apPoint, apRotate);
                    TESObjectCELL* pCell = apRef->GetParentCell();
                    bool bAddOwnership = false;
                    if (!pDroppedRef->GetMyOwner() &&
                        pCell &&
                        pCell->GetOwner() &&
                        !pCell->IsActorCellOwner(static_cast<Actor*>(apRef))) {
                        bAddOwnership = true;
                    }

                    if (!bAddOwnership &&
                        apRef->GetObjectReference()->GetFormType() == NPC__ID &&
                        static_cast<TESActorBase*>(apRef->GetObjectReference())->TESActorBase::GetSex() ==
                            TESActorBase::SEX_FEMALE) {
                        TESBipedModelForm* pBipedModel = dynamic_cast<TESBipedModelForm*>(
                            pDroppedRef->GetObjectReference()
                        );
                        if (pBipedModel) {
                            TESModelTextureSwap* pFemaleModel = pBipedModel->GetWorldTESModel(TESActorBase::SEX_FEMALE);
                            if (pFemaleModel != pBipedModel->GetWorldTESModel(TESActorBase::SEX_MALE)) {
                                bAddOwnership = true;
                            }
                        }
                    }
                    if (bAddOwnership) {
                        pDroppedRef->SetOwner(apRef->GetObjectReference());
                    }
                    if (pNewItem) {
                        delete pNewItem;
                    }
                    pNewItem = nullptr;
                    if (pNewExtra) {
                        delete pNewExtra;
                    }
                    pNewExtra = nullptr;
                }
                else if (apOtherContainer) {
                    TESForm* pOwner = nullptr;
                    if (apRef->IsActor()) {
                        TESBoundObject* pCompObj = apRef->m_Extra.GetLevCreaOriginalBase();
                        if (!pCompObj) {
                            pCompObj = apRef->GetObjectReference();
                        }
                        pOwner = pCompObj;
                    }
                    else {
                        pOwner = apRef->GetOwner();
                    }
                    if (abstealing &&
                        !TESContainer::IsGold(apObject) &&
                        (apObject->GetFormType() != AMMO_ID || apOtherContainer != pPlayer)) {
                        if (!apExtra) {
                            apExtra = new ExtraDataList;
                        }
                        apExtra->SetOwnership(pOwner);
                        apExtra->SetCount(iCount);
                    }
                    if (apExtra && apOtherContainer == pPlayer && apObject->GetFormType() == AMMO_ID) {
                        apExtra->RemoveOwnership();
                    }

                    apOtherContainer->AddObjecttoContainer(apObject, apExtra, iCount);
                    if (pNewItem) {
                        delete pNewItem;
                    }
                    pNewItem = nullptr;
                }
                if (pNewItem) {
                    delete pNewItem;
                }
                pNewItem = nullptr;
            }
            if (ainumber > 0 && pItem && pItem->pExtraObjectList && !pItem->pExtraObjectList->IsEmpty()) {
                pkFirst = pItem->pExtraObjectList;
                while (pkFirst && pkFirst->m_item && ainumber > 0) {
                    bool bExtraFlag = true;
                    pCurrent = pkFirst->m_item;
                    if (pNewItem) {
                        delete pNewItem;
                    }
                    pNewItem = new ItemChange;
                    if (pItem) {
                        pNewItem->pContainerObj = pItem->pContainerObj;
                    }
                    if (pCurrent->GetCount() >= 0) {
                        if (pCurrent->GetWorn(false) && pCurrent->GetCount() <= 1) {
                            if (pCurrent->ItemsInList() <= 1 ||
                                (pCurrent->ItemsInList() == 2 && pCurrent->GetCount() > 1)) {
                                apExtra = nullptr;
                            }
                            int16_t sCount = pCurrent->GetCount();
                            if (apRef->RemoveWornItem(pItem->pContainerObj, sCount, pCurrent)) {
                                pDroppedRef = RemoveItem(
                                    apRef,
                                    apObject,
                                    abstealing,
                                    ainumber,
                                    apExtra,
                                    abDropWorld,
                                    apOtherContainer,
                                    apPoint,
                                    apRotate,
                                    true,
                                    false,
                                    nullptr
                                );
                            }
                            else {
                                const char* pCantRemoveText = static_cast<const char*>(sCantRemoveWornItem);
                                Interface::ShowMessage(
                                    pCantRemoveText,
                                    0,
                                    "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
                                    nullptr,
                                    2.0f,
                                    false
                                );
                            }
                            if (pNewItem) {
                                delete pNewItem;
                            }
                            pNewItem = nullptr;
                            return pDroppedRef;
                        }
                        iCount = pCurrent->GetCount();
                        if (iCount > ainumber || pCurrent->GetLeveledItem()) {
                            pLevelExtra = pCurrent->GetLeveledItem();
                            if (pLevelExtra && (ainumber == iCount || ainumber > iCount)) {
                                pItem->iNumber -= iCount;
                                pCurrent->SetCount(-static_cast<int16_t>(iCount));
                            }
                            else {
                                pItem->iNumber -= ainumber;
                                pCurrent->SetCount(iCount - ainumber);
                            }
                            pNewExtra = new ExtraDataList;
                            pNewExtra->CopyListForReference(pCurrent, false);
                            pNewExtra->RemoveLeveledItem();
                            if (iCount >= ainumber) {
                                pNewExtra->SetCount(ainumber);
                            }
                            else {
                                pNewExtra->SetCount(iCount);
                            }
                            if (ainumber < iCount) {
                                iCount = ainumber;
                            }
                            ainumber -= iCount;
                            if (pNewExtra->IsExtraListEmpty()) {
                                if (pNewExtra) {
                                    delete pNewExtra;
                                }
                                pNewExtra = nullptr;
                            }
                            else {
                                if (!pNewItem->pExtraObjectList) {
                                    pNewItem->pExtraObjectList = new BSSimpleList<ExtraDataList*>;
                                }
                                pNewItem->pExtraObjectList->AddHead(pNewExtra);
                            }
                        }
                        else {
                            pItem->iNumber -= iCount;
                            uint32_t uiNumber = pNewItem->iNumber;
                            pNewItem->iNumber = iCount + uiNumber;
                            pNewExtra = pCurrent;
                            ainumber -= iCount;
                            pItem->pExtraObjectList->Remove(pNewExtra);
                            pCurrent = nullptr;
                            if (pNewExtra->IsExtraListEmpty()) {
                                if (pNewExtra) {
                                    delete pNewExtra;
                                }
                                pNewExtra = nullptr;
                            }
                            else {
                                if (!pNewItem->pExtraObjectList) {
                                    pNewItem->pExtraObjectList = new BSSimpleList<ExtraDataList*>;
                                }
                                pNewItem->pExtraObjectList->AddHead(pNewExtra);
                            }
                        }
                    }
                    else {
                        bExtraFlag = false;
                        pkFirst = pkFirst->GetNext();
                    }
                    if (pItem && bExtraFlag) {
                        if (abDropWorld) {
                            if (pNewExtra && pNewExtra->GetReferencePointer()) {
                                if (pNewExtra) {
                                    pDroppedRef = pNewExtra->GetReferencePointer();
                                }
                                else if (pCurrent) {
                                    pDroppedRef = pCurrent->GetReferencePointer();
                                }
                                if (pDroppedRef) {
                                    pDroppedRef->SetDelete(false);
                                    pNewExtra->RemoveReferencePointer();
                                    bool bDelete = pCurrent == nullptr;
                                    float fScale = pNewExtra->GetScale();
                                    pDroppedRef->SetScale(fScale);
                                    pDroppedRef->m_Extra.CopyListForReference(pNewExtra, bDelete);
                                    pDroppedRef->m_Extra.RemoveScale();
                                    pDroppedRef->m_Extra.RemoveReferencePointer();
                                    if (!pDroppedRef->m_Extra.GetStartingWorldOrCell()) {
                                        pDroppedRef->m_Extra.SetStartingWorldOrCellForRef(pDroppedRef);
                                        pDroppedRef->AddChange(CHANGE_OBJECT_EXTRA_ITEM_DATA);
                                    }
                                    pDroppedRef = DropItemIntoWorld(
                                        apRef,
                                        pItem->pContainerObj,
                                        iCount,
                                        pDroppedRef,
                                        apPoint,
                                        apRotate
                                    );
                                    if (pNewItem) {
                                        delete pNewItem;
                                    }
                                    pNewItem = nullptr;
                                    if (pNewExtra) {
                                        delete pNewExtra;
                                    }
                                    pNewExtra = nullptr;
                                    bool bAddOwnership = false;
                                    TESObjectCELL* pCell = apRef->GetParentCell();
                                    if (!pDroppedRef->GetMyOwner() &&
                                        pCell &&
                                        pCell->GetOwner() &&
                                        !pCell->IsActorCellOwner(static_cast<Actor*>(apRef))) {
                                        bAddOwnership = true;
                                    }

                                    if (!bAddOwnership &&
                                        apRef->GetObjectReference()->GetFormType() == NPC__ID &&
                                        static_cast<TESActorBase*>(apRef->GetObjectReference())
                                                ->TESActorBase::GetSex() == TESActorBase::SEX_FEMALE) {
                                        TESBipedModelForm* pBipedModel = dynamic_cast<TESBipedModelForm*>(
                                            pDroppedRef->GetObjectReference()
                                        );
                                        if (pBipedModel) {
                                            TESModelTextureSwap* pFemaleModel = pBipedModel->GetWorldTESModel(
                                                TESActorBase::SEX_FEMALE
                                            );
                                            if (pFemaleModel != pBipedModel->GetWorldTESModel(TESActorBase::SEX_MALE)) {
                                                bAddOwnership = true;
                                            }
                                        }
                                    }
                                    if (bAddOwnership) {
                                        pDroppedRef->SetOwner(apRef->GetObjectReference());
                                    }
                                }
                            }
                            else {
                                pDroppedRef = DropItemIntoWorld(
                                    apRef,
                                    pItem->pContainerObj,
                                    iCount,
                                    nullptr,
                                    apPoint,
                                    apRotate
                                );
                                TESObjectCELL* pCell = apRef->GetParentCell();
                                bool bAddOwnership = false;
                                if (!pDroppedRef->GetMyOwner() &&
                                    pCell &&
                                    pCell->GetOwner() &&
                                    !pCell->IsActorCellOwner(static_cast<Actor*>(apRef))) {
                                    bAddOwnership = true;
                                }

                                if (!bAddOwnership &&
                                    apRef->GetObjectReference()->GetFormType() == NPC__ID &&
                                    static_cast<TESActorBase*>(apRef->GetObjectReference())->TESActorBase::GetSex() ==
                                        TESActorBase::SEX_FEMALE) {
                                    TESBipedModelForm* pBipedModel = dynamic_cast<TESBipedModelForm*>(
                                        pDroppedRef->GetObjectReference()
                                    );
                                    if (pBipedModel) {
                                        TESModelTextureSwap* pFemaleModel = pBipedModel->GetWorldTESModel(
                                            TESActorBase::SEX_FEMALE
                                        );
                                        if (pFemaleModel != pBipedModel->GetWorldTESModel(TESActorBase::SEX_MALE)) {
                                            bAddOwnership = true;
                                        }
                                    }
                                }
                                if (bAddOwnership) {
                                    pDroppedRef->SetOwner(apRef->GetObjectReference());
                                }
                                if (pNewExtra && pNewExtra->ItemsInList()) {
                                    float fScale = pNewExtra->GetScale();
                                    pDroppedRef->SetScale(fScale);
                                    pDroppedRef->m_Extra.CopyListForReference(pNewExtra, true);
                                    pDroppedRef->m_Extra.RemoveScale();
                                }

                                if (pNewItem) {
                                    delete pNewItem;
                                }
                                pNewItem = nullptr;
                                if (pNewExtra) {
                                    delete pNewExtra;
                                }
                                pNewExtra = nullptr;
                            }
                            if (pNewItem) {
                                delete pNewItem;
                            }
                            pNewItem = nullptr;
                        }
                        else if (apOtherContainer) {
                            TESForm* pOwner;
                            ExtraDataList* pExtra = nullptr;
                            if (pNewItem->pExtraObjectList) {
                                pExtra = pNewItem->pExtraObjectList->m_item;
                            }
                            else {
                                pExtra = new ExtraDataList;
                                pNewItem->pExtraObjectList = new BSSimpleList<ExtraDataList*>;
                                pNewItem->pExtraObjectList->AddHead(pExtra);
                            }
                            if (apRef->IsActor()) {
                                TESBoundObject* pCompObj = apRef->m_Extra.GetLevCreaOriginalBase();
                                if (!pCompObj) {
                                    pCompObj = apRef->GetObjectReference();
                                }
                                pOwner = pCompObj;
                            }
                            else {
                                pOwner = apRef->GetOwner();
                            }
                            if (!abstealing || TESContainer::IsGold(pNewItem->pContainerObj)) {
                                pExtra->RemoveOwnership();
                            }
                            else {
                                pExtra->SetOwnership(pOwner);
                                pExtra->SetCount(iCount);
                            }
                            apOtherContainer->AddObjecttoContainer(pNewItem->pContainerObj, pExtra, iCount);
                            if (pNewItem) {
                                delete pNewItem;
                            }
                            pNewItem = nullptr;
                        }
                        else {
                            if (abdelete) {
                                pNewItem->DeleteAllExtra();
                            }
                            if (pNewItem) {
                                delete pNewItem;
                            }
                            pNewItem = nullptr;
                        }
                        if (pCurrent) {
                            pkFirst = pkFirst->GetNext();
                        }
                        if (ainumber) {
                            bUseBase = true;
                        }
                    }
                    else {
                        if (pNewItem) {
                            delete pNewItem;
                        }
                        pNewItem = nullptr;
                        if (pNewExtra) {
                            delete pNewExtra;
                        }
                        pNewExtra = nullptr;
                    }
                }
            }
        }
        pItem = GetObjectInList(apObject, true, 0);
    }
    if (abPreferStolen && ainumber > 0) {
        return RemoveItem(
            apRef,
            apObject,
            abstealing,
            ainumber,
            apExtra,
            abDropWorld,
            apOtherContainer,
            apPoint,
            apRotate,
            true,
            false,
            nullptr
        );
    }
    if ((!pItem || bUseBase) && iOrgCount) {
        if (pItem) {
            iOrgCount += pItem->iNumber;
        }
        if (!iOrgCount) {
            pItem->pExtraObjectList->RemoveAll();
            return pDroppedRef;
        }
        if (abDropWorld) {
            pDroppedRef = DropItemIntoWorld(apRef, apObject, ainumber, nullptr, apPoint, apRotate);
            TESObjectCELL* pCell = apRef->GetParentCell();
            bool bAddOwnership = false;
            if (!pDroppedRef->GetMyOwner() && pCell && pCell->GetOwner()) {
                bAddOwnership = !pCell->IsActorCellOwner(static_cast<Actor*>(apRef));
            }

            if (!bAddOwnership &&
                apRef->GetObjectReference()->GetFormType() == NPC__ID &&
                static_cast<TESActorBase*>(apRef->GetObjectReference())->TESActorBase::GetSex() ==
                    TESActorBase::SEX_FEMALE) {
                TESBipedModelForm* pBipedModel = dynamic_cast<TESBipedModelForm*>(pDroppedRef->GetObjectReference());
                if (pBipedModel) {
                    TESModelTextureSwap* pFemaleModel = pBipedModel->GetWorldTESModel(TESActorBase::SEX_FEMALE);
                    bAddOwnership = pFemaleModel != pBipedModel->GetWorldTESModel(TESActorBase::SEX_MALE);
                }
            }
            if (bAddOwnership) {
                pDroppedRef->SetOwner(apRef->GetObjectReference());
            }
        }
        else if (apOtherContainer) {
            TESForm* pOwner;
            if (apRef->IsActor()) {
                TESBoundObject* pCompObj = apRef->m_Extra.GetLevCreaOriginalBase();
                if (!pCompObj) {
                    pCompObj = apRef->GetObjectReference();
                }
                pOwner = pCompObj;
            }
            else {
                pOwner = apRef->GetOwner();
            }
            if (!abstealing || TESContainer::IsGold(apObject)) {
                if (apExtra) {
                    apExtra->RemoveOwnership();
                }
            }
            else {
                if (!apExtra) {
                    apExtra = new ExtraDataList;
                }
                apExtra->SetOwnership(pOwner);
                apExtra->SetCount(ainumber);
            }
            if (ainumber > 0) {
                if (apObject->GetFormType() == NOTE_ID && apOtherContainer == pPlayer) {
                    bool bRewardKarma = false;
                    if (abstealing && pOwner) {
                        if (pOwner->GetFormType() == FACT_ID) {
                            bRewardKarma = !static_cast<TESFaction*>(pOwner)->TESFaction::GetIsEvil();
                        }
                        else if (pOwner) {
                            float fKarma = static_cast<ActorValueOwner*>(static_cast<TESActorBase*>(pOwner))
                                               ->GetActorFloatValue(ActorValue::KARMA);
                            TESActorBaseData::ALIGNMENT eAlign = TESActorBaseData::GetAlignmentForKarma(fKarma);
                            if (eAlign != TESActorBaseData::ALIGNMENT_EVIL) {
                                bRewardKarma = eAlign != TESActorBaseData::ALIGNMENT_VERY_EVIL;
                            }
                        }
                    }

                    if (bRewardKarma) {
                        double dKarmaMod = fKarmaModStealing.Float();
                        pPlayer->RewardKarma(static_cast<int32_t>(dKarmaMod));
                    }
                    pPlayer->AddNote(static_cast<BGSNote*>(apObject), true);
                }
                else {
                    apOtherContainer->AddObjecttoContainer(apObject, apExtra, ainumber);
                }
            }
        }
        if (pNewItem) {
            delete pNewItem;
        }
        pNewItem = nullptr;
        if (pItem) {
            pItem->iNumber -= ainumber;
        }
        else {
            pItem = new ItemChange(apObject, -ainumber);
            pListofChanges->AddTail(pItem);
        }
    }
    pItem = GetObjectInList(apObject, true, 0);
    if (!pItem ||
        (pLevelExtra && !apRef->IsActor()) ||
        ((pItem->iNumber || iOrgCount) &&
         (!pItem->pExtraObjectList || !pItem->pExtraObjectList->IsEmpty() || pItem->iNumber))) {
        if (pItem) {
            if (iOrgCount + pItem->iNumber < 0 && !pItem->GetWorn(false) && pItem->pExtraObjectList && !pLevelExtra) {
                pItem->pExtraObjectList->RemoveAll();
            }
        }
    }
    else {
        if (!Interface::ContainerIsBarter() &&
            !Interface::ContainerIsRecipe() &&
            !Interface::IsMenuIDVisible(Interface::ePipBoyRepair, 0) &&
            !Interface::IsMenuIDVisible(Interface::ePipBoyRepair, 0)) {
            pItem->DeleteAllExtra();
        }
        pListofChanges->Remove(pItem);
        if (pItem) {
            delete pItem;
        }
    }
    if (bReload && pAct->GetCurrentWeapon()) {
        TESObjectWEAP* pWeapon = pAct->GetCurrentWeapon();
        pAct->ReloadWeaponNV(pWeapon, 0, false, false);
    }

    return pDroppedRef;
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
            pLevItem
                ->CalculateCurrentFormList(static_cast<uint16_t>(iLevel), std::abs(pObject->i_count), pContainer, 0);
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
        ExtraDataList* pExtra = new ExtraDataList;
        pExtra->SetCount(static_cast<uint16_t>(pObject->i_count));
        TESHealthForm* pHealthForm = TESHealthForm::GetFormAsHealthForm(pObject->pObj);
        pObject->pItemExtra->AddToExtraDataList(pExtra, pHealthForm);
        ItemChange* pChange = new ItemChange(pObject->pObj, 0);
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
    BGSChangeFlags kLoadChangeFlags(0x400);
    apLoadGameBuffer->SetChangeFlags(kLoadChangeFlags);
    BGSChangeFlags kNoFormChange(0);
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
                pChange = new ItemChange(pObject->pObj, 0);
            }
            if (!pChange->pExtraObjectList) {
                pChange->pExtraObjectList = new BSSimpleList<ExtraDataList*>();
            }
            else {
                for (BSSimpleList<ExtraDataList*>* pExtraNode = pChange->pExtraObjectList;
                     pExtraNode && pExtraNode->GetItem();
                     pExtraNode = pExtraNode->GetNext()) {
                    ExtraDataList* pExtra = pExtraNode->GetItem();
                    if (!pExtra || pExtra->GetScript()) {
                        continue;
                    }
                    pExtra->SetScript(pScript);
                    pExtra->SetScriptLocals(pExtra->GetScript()->CloneLocals());
                    TESObjectREFR* pTempObject = new TESObjectREFR;
                    pTempObject->SetTemporary();
                    pScript->Run(pTempObject, pExtra->GetScriptLocals(), nullptr, false);
                    delete pTempObject;
                    --iCount;
                }
            }
            for (int32_t i = 0; i < iCount; ++i) {
                ExtraDataList* pExtra = new ExtraDataList;
                pChange->pExtraObjectList->AddHead(pExtra);
                pExtra->SetCount(1);
                if (!pExtra || pExtra->GetScript()) {
                    continue;
                }
                pExtra->SetScript(pScript);
                pExtra->SetScriptLocals(pExtra->GetScript()->CloneLocals());
                TESObjectREFR* pTempObject = new TESObjectREFR;
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
                 pExtraNode && pExtraNode->GetItem();
                 pExtraNode = pExtraNode->GetNext()) {
                ExtraDataList* pExtra = pExtraNode->GetItem();
                if (!pExtra) {
                    continue;
                }
                TESObjectREFR* pTempObject = new TESObjectREFR;
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
                pChange->pExtraObjectList = new BSSimpleList<ExtraDataList*>();
            }
            else {
                for (BSSimpleList<ExtraDataList*>* pExtraNode = pChange->pExtraObjectList;
                     pExtraNode && pExtraNode->GetItem();
                     pExtraNode = pExtraNode->GetNext()) {
                    ExtraDataList* pExtra = pExtraNode->GetItem();
                    if (!pExtra || pExtra->GetScript()) {
                        continue;
                    }
                    pExtra->SetScript(pScript);
                    pExtra->SetScriptLocals(pExtra->GetScript()->CloneLocals());
                    TESObjectREFR* pTempObject = new TESObjectREFR;
                    pTempObject->SetTemporary();
                    pScript->Run(pTempObject, pExtra->GetScriptLocals(), nullptr, false);
                    delete pTempObject;
                    --iCount;
                }
            }
            for (int32_t i = 0; i < iCount; ++i) {
                ExtraDataList* pExtra = new ExtraDataList;
                pChange->pExtraObjectList->AddHead(pExtra);
                pExtra->SetCount(1);
                if (!pExtra || pExtra->GetScript()) {
                    continue;
                }
                pExtra->SetScript(pScript);
                pExtra->SetScriptLocals(pExtra->GetScript()->CloneLocals());
                TESObjectREFR* pTempObject = new TESObjectREFR;
                pTempObject->SetTemporary();
                pScript->Run(pTempObject, pExtra->GetScriptLocals(), nullptr, false);
                delete pTempObject;
            }
        }
        else if (pChange) {
            for (BSSimpleList<ExtraDataList*>* pExtraNode = pChange->pExtraObjectList;
                 pExtraNode && pExtraNode->GetItem();
                 pExtraNode = pExtraNode->GetNext()) {
                ExtraDataList* pExtra = pExtraNode->GetItem();
                if (!pExtra) {
                    continue;
                }
                TESObjectREFR* pTempObject = new TESObjectREFR;
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
// skipped).
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
            MEM_CONTEXT_SCRIPTS,
            true,
            "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp",
            9803
        );
        pTempRef->SetParentCellSimple(pCell);
        // Calls GetLocationOnReference's code non-virtually.
        pTempRef->SetLocationOnReferenceSimple(*pOwner->QLocationOnReference());
        while (pNode && pNode->GetItem()) {
            ItemChange* pChange = pNode->GetItem();
            for (BSSimpleList<ExtraDataList*>* pExtraNode = pChange->pExtraObjectList;
                 pExtraNode && pExtraNode->GetItem();
                 pExtraNode = pExtraNode->GetNext()) {
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
        BSSimpleList<ItemChange*>* pNext = pListInv->m_pkNext;
        ItemChange* pInventoryItem = pListInv->m_item;
        bool bFlag = false;
        if (!pInventoryItem || pInventoryItem->iNumber <= 0 || pInventoryItem->pContainerObj->GetQuestObject()) {
            if (pNext == pListInv->m_pkNext) {
                pListInv = pNext;
            }
            else {
                pListInv = pListofChanges;
            }
        }
        else {
            BSSimpleList<ExtraDataList*>* pListExtra = pInventoryItem->pExtraObjectList;
            TESBoundObject* pObj = pInventoryItem->pContainerObj;
            Actor* pActor = nullptr;
            if (apStolenContainer->IsActor()) {
                pActor = static_cast<Actor*>(apStolenContainer);
            }
            if (pListExtra && pListExtra->m_item) {
                while (pListExtra && pListExtra->m_item && !bFlag) {
                    BSSimpleList<ExtraDataList*>* pListExtraNext = pListExtra->m_pkNext;
                    ExtraDataList* pExtra = pListExtra->m_item;
                    char cRestart = 0;
                    TESForm* pOwner = pExtra->GetOwner();
                    TESForm* pOwnerNPC = nullptr;
                    TESFaction* pFaction = nullptr;
                    if (pOwner) {
                        if (pOwner->GetFormType() == FACT_ID) {
                            pFaction = static_cast<TESFaction*>(pOwner);
                        }
                        else {
                            pOwnerNPC = pOwner;
                        }
                    }
                    if (!apOwner || apOwner == pExtra->GetOwner()) {
                        if (pActor && !apOwner) {
                            apOwner = pActor->GetObjectReference();
                        }

                        if (pExtra->GetOwner() && pExtra->GetOwner() != apRef) {
                            if (!pActor || pActor->GetObjectReference() == pOwnerNPC || pActor->IsInFaction(pFaction)) {
                                // The compare is unsigned, although ItemsInList returns int.
                                if (static_cast<uint32_t>(pInventoryItem->pExtraObjectList->ItemsInList()) > 1) {
                                    cRestart = 1;
                                }
                                else {
                                    bFlag = true;
                                }
                                if (apOwner) {
                                    char apDest[264];
                                    BSStringT<char> kMessage;
                                    int32_t iCount = pExtra->GetCount();
                                    if (iCount <= 1) {
                                        const char* pRemoveText = sRemoveItemfromInventory.String();
                                        kMessage.SPrintF("%s %s", TESFullName::GetFullName(pObj), pRemoveText);
                                    }
                                    else {
                                        const char* pRemoveText = sRemoveItemfromInventory.String();
                                        const char* pPlural = sPlural.String();
                                        const char* pFullName = TESFullName::GetFullName(pObj);
                                        kMessage.SPrintF("%i %s%s %s", iCount, pFullName, pPlural, pRemoveText);
                                    }
                                    BSsprintf(
                                        apDest,
                                        260,
                                        "%s\\%s",
                                        "Interface\\Icons",
                                        TESTexture::GetTextureName(pObj, pPlayer)
                                    );
                                    const char* pSoundName = pPlayer->GetPickUpSoundName(pObj, false, false);
                                    Interface::ShowMessage(
                                        kMessage.pString,
                                        0,
                                        "Interface\\Icons\\Message Icons\\glow_message_vaultboy_neutral.dds",
                                        pSoundName,
                                        2.0f,
                                        false
                                    );
                                }
                                int16_t sCount = pExtra->GetCount();
                                RemoveItem(
                                    apRef,
                                    pObj,
                                    false,
                                    sCount,
                                    pExtra,
                                    false,
                                    apStolenContainer,
                                    nullptr,
                                    nullptr,
                                    true,
                                    false,
                                    nullptr
                                );
                                pListInv = pListofChanges;
                            }
                        }
                    }
                    if (bFlag) {
                        pListInv = pListofChanges;
                        pListExtra = nullptr;
                    }
                    else if (cRestart) {
                        pListExtra = pInventoryItem->pExtraObjectList;
                    }
                    else {
                        if (pListExtraNext == pListExtra->m_pkNext) {
                            pListExtra = pListExtraNext;
                        }
                        else {
                            pListExtra = pInventoryItem->pExtraObjectList;
                        }
                        if (!pListExtra) {
                            if (pNext == pListInv->m_pkNext) {
                                pListInv = pNext;
                            }
                            else {
                                pListInv = pListofChanges;
                            }
                        }
                    }
                }
            }
            else if (pNext == pListInv->m_pkNext) {
                pListInv = pNext;
            }
            else {
                pListInv = pListofChanges;
            }
        }
    }
}

// Loads the item changes of the form-list save format. The buffer's form, change flags and old change flags are saved,
// set to (no form, 0x400, 0) while the changes load, and put back afterwards. A change whose object did not load is
// deleted and a null entry added in its place.
void InventoryChanges::LoadGame_ObjectsList(BGSLoadFormBuffer* apLoadGameBuffer) {
    BGSLoadGameBuffer* pBaseBuffer = apLoadGameBuffer;
    TESForm* pForm = pBaseBuffer->GetForm();
    BGSChangeFlags kChangeFlags = apLoadGameBuffer->GetChangeFlags();
    BGSChangeFlags kOldChangeFlags = apLoadGameBuffer->GetOldChangeFlags();
    apLoadGameBuffer->SetForm(nullptr);
    BGSChangeFlags kLoadChangeFlags(0x400);
    apLoadGameBuffer->SetChangeFlags(kLoadChangeFlags);
    BGSChangeFlags kNoOldChangeFlags(0);
    apLoadGameBuffer->SetOldChangeFlags(kNoOldChangeFlags);
    uint32_t uiCount = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t i = 0; i < uiCount; ++i) {
        ItemChange* pChange;
        pChange = new ItemChange;
        pChange->LoadGame(apLoadGameBuffer);
        if (!pChange->pContainerObj) {
            pChange->DeleteAllExtra();
            if (pChange) {
                delete pChange;
            }
            pChange = nullptr;
        }
        pListofChanges->AddHead(pChange);
    }
    apLoadGameBuffer->SetForm(pForm);
    apLoadGameBuffer->SetChangeFlags(kChangeFlags);
    apLoadGameBuffer->SetOldChangeFlags(kOldChangeFlags);
}

ItemChange* InventoryChanges::GetItembyIndex(int32_t aiIndex) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_INVENTORY,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp",
        0x187F
    );
    BSSimpleList<ContainerObject*>* pOriginalList = nullptr;
    if (GetOriginalContainer()) {
        pOriginalList = GetOriginalContainer()->GetFilteredList();
    }
    int32_t inumber = 0;
    ItemChange* pReturnItem = nullptr;
    TESContainer* pContainer = GetOriginalContainer();
    for (BSSimpleList<ContainerObject*>* pList = pOriginalList; pList && pList->m_item && !pReturnItem;
         pList = pList->m_pkNext) {
        ContainerObject* pObject = pList->m_item;
        TESLevItem* pObj = nullptr;
        if (pObject->pObj->GetFormType() == LVLI_ID) {
            pObj = static_cast<TESLevItem*>(pObject->pObj);
        }
        if (pObject && !pObj) {
            ItemChange* pItem = GetObjectInList(pObject->pObj, true, 0);
            int32_t iOrgCount = pContainer->GetObjectCount(pObject->pObj);
            if (pObject && pItem && iOrgCount <= 0) {
                pReturnItem = nullptr;
            }
            else if (
                (iOrgCount < 0 && pItem) ||
                (pItem && iOrgCount + pItem->iNumber <= 0) ||
                (pItem &&
                 pItem->pExtraObjectList &&
                 pItem->pExtraObjectList->m_item &&
                 pItem->pExtraObjectList->m_item->HasLeveledItem())
            ) {
                pReturnItem = nullptr;
            }
            else if (!pObject || inumber != aiIndex || (pItem && pItem->GetAmountNonDefaultExtra())) {
                if (pItem) {
                    int32_t iNumb = 0;
                    if (pItem->pExtraObjectList) {
                        iNumb = pItem->GetAmountNonDefaultExtra();
                    }
                    if (aiIndex >= inumber + iNumb) {
                        inumber += pItem->GetAmountNonDefaultExtra();
                        bool bFlag = false;
                        TESObjectWEAP* pWeap = nullptr;
                        if (pItem->pContainerObj->GetFormType() == WEAP_ID) {
                            pWeap = static_cast<TESObjectWEAP*>(pItem->pContainerObj);
                        }
                        if (!pItem->GetExtraTotalCount(false) &&
                            pItem->GetWorn(false) &&
                            (pItem->iNumber <= 0 ||
                             pItem->pContainerObj->GetFormType() == AMMO_ID ||
                             (pWeap &&
                              (pWeap->GetWeaponType() == WEAPON_ONE_HAND_GRENADE ||
                               pWeap->GetWeaponType() == WEAPON_ONE_HAND_MINE ||
                               pWeap->GetWeaponType() == WEAPON_ONE_HAND_THROWN)))) {
                            bFlag = true;
                        }
                        if (!bFlag) {
                            int32_t iExtraTotalCount = pItem->GetExtraTotalCount(true);
                            if (iExtraTotalCount < pItem->iNumber + iOrgCount) {
                                if (!pItem->HasLeveledExtra() || pItem->iNumber <= iOrgCount) {
                                    if (inumber == aiIndex) {
                                        pReturnItem = new ItemChange;
                                        if (pItem) {
                                            int32_t iTotalCount = pItem->iNumber + iOrgCount;
                                            pReturnItem->iNumber = iTotalCount - pItem->GetExtraTotalCount(false);
                                        }
                                        else {
                                            pReturnItem->iNumber = iOrgCount;
                                        }
                                        pReturnItem->pContainerObj = pObject->pObj;
                                        if (pItem &&
                                            pItem->GetScript() &&
                                            pItem->pExtraObjectList &&
                                            pItem->pExtraObjectList->ItemsInList()) {
                                            if (!pReturnItem->pExtraObjectList) {
                                                pReturnItem->pExtraObjectList = new BSSimpleList<ExtraDataList*>;
                                            }
                                            for (BSSimpleList<ExtraDataList*>* pNode = pItem->pExtraObjectList;
                                                 pNode && pNode->m_item;
                                                 pNode = pNode->m_pkNext) {
                                                pReturnItem->pExtraObjectList->AddHead(pNode->m_item);
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
                        for (BSSimpleList<ExtraDataList*>* pExtraList = pItem->pExtraObjectList;
                             pExtraList && pExtraList->m_item && !pReturnItem;
                             pExtraList = pExtraList->GetNext()) {
                            ExtraDataList* pCurExtra = pExtraList->m_item;
                            if (!pCurExtra->IsExtraDefaultforContainer(false)) {
                                if (inumber == aiIndex) {
                                    int32_t iCount = pCurExtra->GetCount();
                                    if (iCount > iOrgCount + pItem->iNumber) {
                                        iCount = iOrgCount + pItem->iNumber;
                                    }
                                    pReturnItem = new ItemChange(pItem->pContainerObj, iCount);
                                    pReturnItem->pExtraObjectList->AddHead(pCurExtra);
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
                pReturnItem = new ItemChange;
                if (pItem) {
                    if (iOrgCount >= 0) {
                        int32_t iTotalCount = pItem->iNumber + iOrgCount;
                        pReturnItem->iNumber = iTotalCount - pItem->GetExtraTotalCount(false);
                    }
                    else {
                        pReturnItem->iNumber = pItem->iNumber + iOrgCount;
                    }
                }
                else if (iOrgCount >= 0) {
                    pReturnItem->iNumber = iOrgCount;
                }
                else {
                    pReturnItem->iNumber = -iOrgCount;
                }
                pReturnItem->pContainerObj = pObject->pObj;
                if (pItem) {
                    if ((pItem->GetScript() ||
                         pItem->GetHotKey() >= 0 ||
                         static_cast<TESObjectREFR*>(pItem->GetItemOwnership()) != pRef) &&
                        pItem->pExtraObjectList &&
                        pItem->pExtraObjectList->ItemsInList()) {
                        if (!pReturnItem->pExtraObjectList) {
                            pReturnItem->pExtraObjectList = new BSSimpleList<ExtraDataList*>;
                        }
                        for (BSSimpleList<ExtraDataList*>* pNode = pItem->pExtraObjectList; pNode && pNode->m_item;
                             pNode = pNode->m_pkNext) {
                            pReturnItem->pExtraObjectList->AddHead(pNode->m_item);
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
    if (!pReturnItem) {
        for (BSSimpleList<ItemChange*>* pkFirst = pListofChanges; pkFirst && pkFirst->m_item && !pReturnItem;
             pkFirst = pkFirst->m_pkNext) {
            ItemChange* pItem = pkFirst->m_item;
            int32_t iOriginalCount = 0;
            if (GetOriginalContainer() && pItem) {
                TESBoundObject* pContainerObj = pItem->pContainerObj;
                iOriginalCount = GetOriginalContainer()->GetObjectCount(pContainerObj);
            }

            if (iOriginalCount < 0 && pItem->GetWorn(false)) {
                pItem->pExtraObjectList->m_item->SetCount(-static_cast<int16_t>(iOriginalCount));
            }
            if (pItem && (pItem->iNumber > 0 || iOriginalCount < 0)) {
                TESBoundObject* pContainerObj;
                if ((pItem->pExtraObjectList &&
                     pItem->pExtraObjectList->m_item &&
                     pItem->pExtraObjectList->m_item->HasLeveledItem()) ||
                    iOriginalCount < 0 ||
                    !GetOriginalContainer() ||
                    (pContainerObj = pItem->pContainerObj, !GetOriginalContainer()->HasObject(pContainerObj))) {
                    int32_t iTotalNonDefaultCount = 0;
                    for (ExtraDataList* pExtra : pItem->pExtraObjectList->ItemsUntilNull()) {
                        if (!pExtra->IsExtraDefaultforContainer(false)) {
                            int16_t sCount = pExtra->GetCount();
                            if (sCount > 0) {
                                iTotalNonDefaultCount += sCount;
                            }
                        }
                    }
                    if (pItem->pExtraObjectList &&
                        pItem->pExtraObjectList->m_item &&
                        aiIndex < inumber + pItem->GetAmountNonDefaultExtra()) {
                        for (BSSimpleList<ExtraDataList*>* pExtraList = pItem->pExtraObjectList;
                             pExtraList && pExtraList->m_item && !pReturnItem;
                             pExtraList = pExtraList->m_pkNext) {
                            ExtraDataList* pCurExtra = pExtraList->m_item;
                            if (inumber == aiIndex) {
                                if (!pCurExtra->IsExtraDefaultforContainer(false)) {
                                    int16_t sCount = pCurExtra->GetCount();
                                    if (sCount > 0) {
                                        pReturnItem = new ItemChange(pItem->pContainerObj, sCount);
                                        pReturnItem->pExtraObjectList->AddHead(pCurExtra);
                                    }
                                }
                            }
                            else if (!pCurExtra->IsExtraDefaultforContainer(false) && pCurExtra->GetCount() > 0) {
                                ++inumber;
                            }
                        }
                    }
                    else if (pItem->iNumber == iTotalNonDefaultCount) {
                        inumber += pItem->GetAmountNonDefaultExtra();
                    }
                    else {
                        if (pItem->iNumber > iTotalNonDefaultCount) {
                            int32_t iAmount = pItem->GetAmountNonDefaultExtra();
                            if (iAmount > 0) {
                                inumber += iAmount;
                            }
                        }
                        if (inumber != aiIndex || pReturnItem || pItem->iNumber - iTotalNonDefaultCount <= 0) {
                            ++inumber;
                        }
                        else {
                            ExtraDataList* pExtra = nullptr;
                            pReturnItem = new ItemChange(pItem->pContainerObj, 0);
                            pReturnItem->iNumber = pItem->iNumber - iTotalNonDefaultCount;
                            if (pItem->pExtraObjectList) {
                                for (ExtraDataList*& rpItem : pItem->pExtraObjectList->ItemsUntilNull()) {
                                    pExtra = rpItem;
                                    if (!pExtra->GetWorn(false) && pExtra->IsExtraDefaultforContainer(true)) {
                                        pReturnItem->pExtraObjectList->AddTail(pExtra);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return pReturnItem;
}

void InventoryChanges::GetItemModGroup(TESBoundObject* apObj, BSSimpleList<ItemChange*>* apReturnList) {
    if (apObj && apReturnList && apObj->GetFormType() == WEAP_ID) {
        InventoryChanges::IterData* pIter = GetIterator();
        ItemChange* pCurrItem = nullptr;
        TESBoundObject* pItemMod = nullptr;
        do {
            pCurrItem = Iterate(pIter);
            if (pCurrItem) {
                if (pCurrItem->pContainerObj->GetFormType() == IMOD_ID) {
                    pItemMod = pCurrItem->pContainerObj;
                    TESObjectIMOD* pMod = static_cast<TESObjectWEAP*>(apObj)->TESObjectWEAP::GetModItem(1u);
                    if (pItemMod == pMod ||
                        pItemMod == static_cast<TESObjectWEAP*>(apObj)->TESObjectWEAP::GetModItem(2u)) {
                        apReturnList->AddHead(pCurrItem);
                    }
                    else {
                        pMod = static_cast<TESObjectWEAP*>(apObj)->TESObjectWEAP::GetModItem(4u);
                        if (pItemMod == pMod) {
                            apReturnList->AddHead(pCurrItem);
                        }
                    }
                }
                else if (pCurrItem) {
                    delete pCurrItem;
                }
            }
        }
        while (pCurrItem);
        if (pIter) {
            delete pIter;
        }
    }
}

void InventoryChanges::RemoveHotKeyItem(ItemChange* apItem, int32_t aiHotkey) {
    ItemChange* pItemGroup = GetObjectInList(apItem->pContainerObj, true, 0);
    if (pItemGroup) {
        BSSimpleList<ExtraDataList*>* pNode = pItemGroup->pExtraObjectList;
        while (pNode) {
            ExtraDataList* pExtra = pNode->m_item;
            if (pExtra &&
                ((aiHotkey != -1 && pExtra->GetHotKey() == aiHotkey) || (aiHotkey == -1 && pExtra->GetHotKey() >= 0)) &&
                ((pExtra->RemoveHotKey(), pExtra->IsExtraListEmpty()) ||
                 (pExtra->GetCount() > 1 && pExtra->ItemsInList() == 1))) {
                pItemGroup->pExtraObjectList->Remove(pExtra);
                pNode = pItemGroup->pExtraObjectList;
            }
            else {
                pNode = pNode->m_pkNext;
            }
        }
        if (pItemGroup->pExtraObjectList && pItemGroup->pExtraObjectList->IsEmpty()) {
            if (pItemGroup->pExtraObjectList) {
                delete pItemGroup->pExtraObjectList;
            }
            pItemGroup->pExtraObjectList = nullptr;
            if (!pItemGroup->iNumber) {
                pListofChanges->Remove(pItemGroup);
                if (pItemGroup) {
                    delete pItemGroup;
                }
            }
        }
    }
}

void InventoryChanges::SetHotKeyItem(ItemChange* apItem, ExtraDataList* apExtra, int32_t aiKey) {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_INVENTORY,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp",
        1974
    );
    if (apItem && aiKey >= 0 && aiKey <= 7) {
        if (pPlayer->GetAmmoForHotKey(aiKey)) {
            pPlayer->SetAmmoForHotKey(aiKey, nullptr);
        }
        RemoveHotKeyItem(apItem, -1);
        ItemChange* pItemGroup = GetObjectInList(apItem->pContainerObj, true, 0);
        if (pItemGroup) {
            ExtraDataList* pFoundExtra = nullptr;
            if (!pItemGroup->pExtraObjectList) {
                pItemGroup->pExtraObjectList = new BSSimpleList<ExtraDataList*>;
            }
            BSSimpleList<ExtraDataList*>* pNode = pItemGroup->pExtraObjectList;
            while (pNode && pNode->m_item) {
                if (!pFoundExtra && pNode->m_item->IsExtraDefaultforContainer(false)) {
                    pFoundExtra = pNode->m_item;
                }

                if (pNode->m_item == apExtra) {
                    pFoundExtra = apExtra;
                }
                pNode = pNode->m_pkNext;
            }
            if (pFoundExtra) {
                pFoundExtra->SetHotKey(aiKey);
            }
            else {
                ExtraDataList* pNewExtra;
                pNewExtra = new ExtraDataList;
                pNewExtra->SetHotKey(aiKey);
                int16_t sNumber = pItemGroup->iNumber;
                int16_t sFreeCount = sNumber - pItemGroup->GetExtraTotalCount(false);
                bool bWorn = pItemGroup->GetWorn(false);
                pNewExtra->SetCount(sFreeCount - bWorn);
                pItemGroup->pExtraObjectList->AddHead(pNewExtra);
            }
        }
        else {
            ExtraDataList* pNewChangeExtra;
            ItemChange* pNewChange = new ItemChange(apItem->pContainerObj, apItem->iNumber);
            pNewChangeExtra = new ExtraDataList;
            pNewChange->pExtraObjectList = new BSSimpleList<ExtraDataList*>;
            pNewChangeExtra->SetHotKey(aiKey);
            pNewChange->pExtraObjectList->AddHead(pNewChangeExtra);
            pListofChanges->AddHead(pNewChange);
        }
    }
}

void InventoryChanges::AddItem(TESObjectREFR* apObject, int32_t ainumber, bool abAddRefId, bool abWearit) {
    ExtraDataList* pExtra;
    AutoMemContext tempMemContext(
        MEM_CONTEXT_INVENTORY,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp",
        3131
    );
    bcountdirty = true;
    fpreviousContainerWeight = fcontainerweight;
    fcontainerweight = -1.0f;
    if (apObject->GetObjectReference()->GetFormType() == WEAP_ID) {
        if (TESHealthForm::GetFormHealth(apObject->GetObjectReference()) && apObject->m_Extra.GetHealth() == -1.0) {
            if (static_cast<TESObjectWEAP*>(apObject->GetObjectReference())->TESObjectWEAP::GetWeaponType() <
                WEAPON_ONE_HAND_GRENADE) {
                int32_t iRefFormID = pRef->iFormID;
                const char* pRefEditorID = pRef->GetFormEditorID();
                int32_t iObjectFormID = apObject->iFormID;
                const char* pObjectEditorID = apObject->GetFormEditorID();
                Warning(
                    "Adding full health weapon ref '%s' (%08X) to ref '%s' (%08X).",
                    pObjectEditorID,
                    iObjectFormID,
                    pRefEditorID,
                    iRefFormID
                );
                TESHealthForm* pHealthForm = dynamic_cast<TESHealthForm*>(apObject->GetObjectReference());
                uint32_t uiHealth = pHealthForm->GetFormHealth();
                if (apObject->m_Extra.HasWeaponMods()) {
                    uiHealth = static_cast<uint32_t>(static_cast<int64_t>(
                        static_cast<TESObjectWEAP*>(apObject->GetObjectReference())
                            ->TESObjectWEAP::GetModEffectValue(WEAPON_MOD_MAX_HEALTH, 0) +
                        static_cast<double>(uiHealth)
                    ));
                }
                uiHealth = BSMIN(
                    static_cast<uint32_t>(static_cast<int64_t>(static_cast<double>(uiHealth) * 0.99)),
                    uiHealth - 1
                );
                apObject->SetHealth(static_cast<float>(uiHealth));
            }
        }
        else if (
            static_cast<TESObjectWEAP*>(apObject->GetObjectReference())->TESObjectWEAP::IsThrownWeapon() &&
            apObject->m_Extra.GetHealth() != -1.0
        ) {
            int32_t iRefFormID = pRef->iFormID;
            const char* pRefEditorID = pRef->GetFormEditorID();
            int32_t iObjectFormID = apObject->iFormID;
            const char* pObjectEditorID = apObject->GetFormEditorID();
            Warning(
                "Adding damaged throwing weapon ref '%s' (%08X) to ref '%s' (%08X).",
                pObjectEditorID,
                iObjectFormID,
                pRefEditorID,
                iRefFormID
            );
            TESHealthForm* pHealthForm = dynamic_cast<TESHealthForm*>(apObject->GetObjectReference());
            apObject->SetHealth(static_cast<float>(pHealthForm->GetFormHealth()));
        }
    }
    if ((apObject->GetObjectReference()->GetFormType() == ARMO_ID ||
         apObject->GetObjectReference()->GetFormType() == CLOT_ID) &&
        TESHealthForm::GetFormHealth(apObject->GetObjectReference()) &&
        apObject->m_Extra.GetHealth() == -1.0) {
        int32_t iRefFormID = pRef->iFormID;
        const char* pRefEditorID = pRef->GetFormEditorID();
        int32_t iObjectFormID = apObject->iFormID;
        const char* pObjectEditorID = apObject->GetFormEditorID();
        Warning(
            "Adding full health armor ref '%s' (%08X) to ref '%s' (%08X).",
            pObjectEditorID,
            iObjectFormID,
            pRefEditorID,
            iRefFormID
        );
        TESHealthForm* pHealthForm = dynamic_cast<TESHealthForm*>(apObject->GetObjectReference());
        uint32_t uiHealth = pHealthForm->GetFormHealth();
        uint32_t uiNewHealth = BSMIN(
            static_cast<uint32_t>(static_cast<int64_t>(static_cast<double>(uiHealth) / 0.99)),
            uiHealth - 1
        );
        apObject->SetHealth(static_cast<float>(uiNewHealth));
    }

    int32_t iAmmoCount = 0;
    ExtraAmmo* pAmmo = apObject->m_Extra.GetAmmoExtra();
    if (pAmmo) {
        iAmmoCount = pAmmo->iCount;
        apObject->m_Extra.SetAmmoExtra(nullptr, 0);
        apObject->AddChange(CHANGE_OBJECT_EXTRA_AMMO);
    }
    iAmmoCount *= ainumber;
    if (pRef) {
        pRef->AddChange(CHANGE_REFR_INVENTORY);
    }
    ItemChange* pItem = GetObjectInList(apObject->GetObjectReference(), true, 0);
    apObject->m_Extra.RemoveCount();
    pExtra = new ExtraDataList;
    bool bIsThrown = false;
    if (!apObject->m_Extra.IsExtraListEmpty()) {
        TESObjectWEAP* pWeapon = nullptr;
        if (apObject->GetObjectReference()->GetFormType() == WEAP_ID) {
            pWeapon = static_cast<TESObjectWEAP*>(apObject->GetObjectReference());
            bIsThrown = pWeapon->IsThrownWeapon();
        }
        pExtra->CopyListForContainer(&apObject->m_Extra, false);
        if (apObject->GetObjectReference()->GetFormType() != AMMO_ID && (!pWeapon || !pWeapon->IsThrownWeapon())) {
            pExtra->SetScale(apObject->fRefScale);
        }
        if (pExtra->GetOwner() == pPlayer && pRef == pPlayer) {
            pExtra->RemoveOwnership();
        }
    }
    if (abWearit) {
        pExtra->SetWorn(true, false);
    }
    if (!pItem) {
        ItemChange* pNewItem;
        pNewItem = new ItemChange(apObject->GetObjectReference(), ainumber);
        if (ainumber > 1) {
            pExtra->SetCount(ainumber);
        }
        if (apObject->GetRefPersists()) {
            pExtra->SetReferencePointer(apObject);
            if (!pNewItem->pExtraObjectList) {
                pNewItem->pExtraObjectList = new BSSimpleList<ExtraDataList*>;
            }
            pNewItem->pExtraObjectList->AddHead(pExtra);
        }
        else if (pExtra) {
            if (pExtra->IsExtraListEmpty()) {
                delete pExtra;
                pExtra = nullptr;
            }
            else {
                if (!pNewItem->pExtraObjectList) {
                    pNewItem->pExtraObjectList = new BSSimpleList<ExtraDataList*>;
                }
                pNewItem->pExtraObjectList->AddHead(pExtra);
            }
        }
        pListofChanges->AddTail(pNewItem);
    }
    else {
        pItem->iNumber += ainumber;
        if (apObject->GetRefPersists()) {
            pExtra->SetReferencePointer(apObject);
            if (ainumber > 1) {
                pExtra->SetCount(ainumber);
            }
            if (!pItem->pExtraObjectList) {
                pItem->pExtraObjectList = new BSSimpleList<ExtraDataList*>;
            }
            pItem->pExtraObjectList->AddHead(pExtra);
        }
        else {
            BSSimpleList<ExtraDataList*>* pNode = pItem->pExtraObjectList;
            bool bAddNew = true;
            while (pNode && pNode->m_item && bAddNew) {
                ExtraDataList* pCurrent = pNode->m_item;
                ExtraDataList* pCompare = pExtra;
                bool bDiffers = true;
                if (pCompare) {
                    bDiffers = pCompare->CompareListForContainer(pCurrent, false, bIsThrown) ||
                        pCurrent->CompareListForContainer(pCompare, false, bIsThrown);
                }
                if (bDiffers) {
                    pNode = pNode->m_pkNext;
                }
                else {
                    int16_t sCount = pCurrent->GetCount();
                    pCurrent->SetCount(ainumber + sCount);
                    if (pCurrent->BaseExtraList::IsExtraListEmpty()) {
                        pItem->pExtraObjectList->Remove(pCurrent);
                        if (pCurrent) {
                            delete pCurrent;
                        }
                    }
                    bAddNew = false;
                }
            }
            if (bAddNew) {
                if (pExtra && pExtra->IsExtraListEmpty()) {
                    if (pExtra) {
                        delete pExtra;
                    }
                    pExtra = nullptr;
                }
                else {
                    if (!pItem->pExtraObjectList) {
                        pItem->pExtraObjectList = new BSSimpleList<ExtraDataList*>;
                    }
                    pItem->pExtraObjectList->AddHead(pExtra);
                }
            }
            else {
                if (pExtra) {
                    delete pExtra;
                }
                pExtra = nullptr;
            }
        }
        if (pItem->pExtraObjectList && pItem->pExtraObjectList->IsEmpty() && !pItem->iNumber) {
            pListofChanges->Remove(pItem);
            if (pExtra) {
                delete pExtra;
            }
            pExtra = nullptr;
            if (pItem) {
                delete pItem;
            }
        }
    }
    if (pExtra && pExtra->IsExtraListEmpty() && pExtra) {
        delete pExtra;
    }

    if (iAmmoCount > 0) {
        TESObjectWEAP* pWeapon = static_cast<TESObjectWEAP*>(apObject->GetObjectReference());
        TESBoundObject* pAmmoObj = pWeapon->GetCurrentAmmo(nullptr);
        if (pAmmoObj) {
            AddItem(pAmmoObj, nullptr, iAmmoCount);
        }
    }
}

TESBoundObject* InventoryChanges::GetObjectbyPackObjType(PACKAGE_OBJECT_TYPE aeObjectType, int32_t& ainumb) {
    BSSimpleList<ContainerObject*>* pList = &GetOriginalContainer()->objectList;
    TESBoundObject* pObj = nullptr;
    while (pList && !pObj) {
        pObj = pList->m_item->pObj;
        if (pObj && TESPackage::GetObjectTypeFromForm(pObj) == aeObjectType) {
            ItemChange* pItem = GetObjectInList(pObj, true, 0);
            if (pItem) {
                uint32_t uiNumber = pItem->iNumber;
                if (pList->m_item->i_count + uiNumber) {
                    ainumb = pList->m_item->i_count + uiNumber;
                }
            }
            else {
                ainumb = pList->m_item->i_count;
            }
        }
        else {
            pObj = nullptr;
        }
        pList = pList->m_pkNext;
    }
    for (BSSimpleList<ItemChange*>* pListInv = pListofChanges; pListInv && !pObj; pListInv = pListInv->m_pkNext) {
        pObj = pListInv->m_item->pContainerObj;
        if (pObj && TESPackage::GetObjectTypeFromForm(pObj) == aeObjectType) {
            if (!GetOriginalContainer()->HasObject(pObj) && pListInv->m_item->iNumber > 0) {
                ainumb = pListInv->m_item->iNumber;
            }
        }
        else {
            pObj = nullptr;
        }
    }
    return pObj;
}

TESObjectREFR* InventoryChanges::DropItemIntoWorld(
    TESObjectREFR* apRef,
    TESBoundObject* apObject,
    int32_t aiCount,
    TESObjectREFR* apReference,
    const NiPoint3* apPoint,
    const NiPoint3* apRotate
) {
    const NiPoint3* pAngle;
    NiPoint3 angle;
    NiPoint3 point;
    if (apPoint) {
        point.x = apPoint->x;
        point.y = apPoint->y;
        point.z = apPoint->z;
    }
    else {
        NiPoint3 kOffset;
        NiMatrix3 rot;
        point = apRef->GetLocationOnReference();
        NiPoint3 kRefAngle = apRef->GetAngleOnReference();
        rot.FromEulerAnglesXYZ(kRefAngle.x, kRefAngle.y, kRefAngle.z);
        NiPoint3 inFrontAndUp(0.0f, 50.0f, 30.0f);
        kOffset = rot * inFrontAndUp;
        point += kOffset;
    }
    if (apRotate) {
        pAngle = apRotate;
    }
    else if (apRef) {
        pAngle = &apRef->GetAngleOnReference();
    }
    else {
        pAngle = &NiPoint3::ZERO;
    }
    angle = *pAngle;
    TESWorldSpace* pWorldSpace = apRef->GetWorldSpace();
    TESObjectCELL* pParentCell = apRef->GetParentCell();
    TESObjectREFR* pObjectRef = pDataHandler->CreateReferenceAtLocation(
        apObject,
        point,
        angle,
        pParentCell,
        pWorldSpace,
        apReference,
        nullptr,
        nullptr
    );
    if (aiCount > 1) {
        pObjectRef->m_Extra.SetCount(aiCount);
    }
    pObjectRef->AddChange(CHANGE_OBJECT_EXTRA_ITEM_DATA);
    return pObjectRef;
}

TESBoundObject* InventoryChanges::GetLeveledCalculatedBaseObject(TESLevItem* apLevelItem) {
    TESBoundObject* pObject = nullptr;
    if (apLevelItem) {
        BSSimpleList<ContainerObject*>* pList = &GetOriginalContainer()->objectList;
        int32_t iIndex = 0;
        while (pList) {
            if (pList->IsEmpty()) {
                break;
            }
            TESLevItem* pObj = dynamic_cast<TESLevItem*>(pList->m_item->pObj);
            if (pObj && pObj == apLevelItem) {
                break;
            }

            if (pObj) {
                ++iIndex;
            }
            pList = pList->m_pkNext;
        }
        for (BSSimpleList<ItemChange*>* pInvList = pListofChanges; pInvList && !pInvList->IsEmpty() && pInvList->m_item;
             pInvList = pInvList->m_pkNext) {
            ItemChange* pItem = pInvList->m_item;
            if (pItem) {
                for (ExtraDataList* pExtra : pItem->pExtraObjectList->ItemsUntilNull()) {
                    if (pExtra && pExtra->GetExtraLevelItemIndex() == iIndex) {
                        pObject = pItem->pContainerObj;
                        break;
                    }
                }
            }
        }
    }
    return pObject;
}

void InventoryChanges::CleanUpBadData() {
    TESBoundObject* pObj = nullptr;
    BSSimpleList<ContainerObject*>* pList = &GetOriginalContainer()->objectList;
    while (pList && !pList->IsEmpty()) {
        pObj = pList->m_item->pObj;
        TESLevItem* pleveled = dynamic_cast<TESLevItem*>(pList->m_item->pObj);
        TESBipedModelForm* pbipedform = TESBipedModelForm::GetFormAsBipedModel(pObj);
        if (pObj && !pleveled && (!pbipedform || pbipedform->GetPlayable())) {
            ItemChange* pitem = GetObjectInList(pObj, true, 0);
            if (pitem) {
                TESHealthForm* phealth = dynamic_cast<TESHealthForm*>(pitem->pContainerObj);
                if (phealth && pitem->pExtraObjectList) {
                    BSSimpleList<ExtraDataList*>* plist = pitem->pExtraObjectList;
                    while (plist && plist->m_item) {
                        ExtraDataList* pext = plist->m_item;
                        if (pext->GetHealth() > -1.0 &&
                            pext->GetHealth() == static_cast<double>(phealth->GetFormHealth())) {
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
            if (pitem && TESContainer::IsGold(pObj) && pRef == pPlayer && pitem->pExtraObjectList) {
                BSSimpleList<ExtraDataList*>* plist = pitem->pExtraObjectList;
                while (plist && plist->m_item) {
                    ExtraDataList* pext = plist->m_item;
                    plist->RemoveHead();
                    if (pext) {
                        delete pext;
                    }
                }
            }

            if (pitem && pitem->GetScript()) {
                for (BSSimpleList<ExtraDataList*>* plist = pitem->pExtraObjectList; plist && plist->m_item;
                     plist = plist->m_pkNext) {
                    ExtraDataList* pext = plist->m_item;
                    if (pext && pext->GetScript()) {
                        BSSimpleList<ExtraDataList*>* plist2 = plist->m_pkNext;
                        while (plist2 && plist2->m_item) {
                            ExtraDataList* pextra2 = plist2->m_item;
                            if (pext == pextra2) {
                                TESObjectREFR* pref;
                                ExtraDataList* pextra;
                                plist2->RemoveHead();
                                pextra = new ExtraDataList;
                                pextra->SetScript(pext->GetScript());
                                pextra->SetScriptLocals(pextra->GetScript()->CloneLocals());
                                pref = new TESObjectREFR;
                                pref->SetTemporary();
                                ScriptLocals* pLocals = pextra->GetScriptLocals();
                                pext->GetScript()->Run(pref, pLocals, nullptr, false);
                                plist2->AddTail(pextra);
                                plist2 = plist->m_pkNext;
                            }
                            else {
                                plist2 = plist2->m_pkNext;
                            }
                        }
                    }
                }
            }

            Script* pScript = TESScriptableForm::GetFormScript(pObj);
            if (pitem || !pScript) {
                if (pitem && pitem->iNumber >= 0 && pScript) {
                    BSSimpleList<ExtraDataList*>* plist = pitem->pExtraObjectList;
                    if (plist && !plist->IsEmpty()) {
                        int32_t iNumber = pitem->iNumber;
                        int32_t iadd = iNumber - plist->ItemsInList();
                        while (plist && plist->m_item) {
                            ExtraDataList* pext = plist->m_item;
                            if (pext && !pext->GetScript()) {
                                TESObjectREFR* pref;
                                pext->SetScript(pScript);
                                pext->SetScriptLocals(pext->GetScript()->CloneLocals());
                                pref = new TESObjectREFR;
                                pref->SetTemporary();
                                ScriptLocals* pLocals = pext->GetScriptLocals();
                                pitem->GetScript()->Run(pref, pLocals, nullptr, false);
                                if (pref) {
                                    delete pref;
                                }
                            }

                            plist = plist->m_pkNext;
                        }
                        plist = pitem->pExtraObjectList;
                        for (int32_t i = 0; i < iadd; ++i) {
                            TESObjectREFR* pref;
                            ExtraDataList* pextra;
                            pextra = new ExtraDataList;
                            pextra->SetScript(pitem->GetScript());
                            pextra->SetScriptLocals(pextra->GetScript()->CloneLocals());
                            pref = new TESObjectREFR;
                            pref->SetTemporary();
                            ScriptLocals* pLocals = pextra->GetScriptLocals();
                            pScript->Run(pref, pLocals, nullptr, false);
                            if (pref) {
                                delete pref;
                            }
                            plist->AddHead(pextra);
                        }
                    }
                    else {
                        InitScripts();
                    }
                }
            }
            else {
                InitScripts();
            }
            if (pitem && pObj->GetFormType() == AMMO_ID) {
                // One stack slot: the worn list kept below starts as the last item the ownership loop visited.
                ExtraDataList* pwornextra = nullptr;
                if (pRef == pPlayer) {
                    BSSimpleList<ExtraDataList*>* plistextra = pitem->pExtraObjectList;
                    while (plistextra && plistextra->m_item) {
                        pwornextra = plistextra->m_item;
                        plistextra = plistextra->m_pkNext;
                        pwornextra->RemoveOwnership();
                    }
                }
                if (!pitem->GetWorn(false) || pitem->HasLeveledExtra()) {
                    if (pitem->GetWorn(false) && pitem->HasLeveledExtra()) {
                        if (pitem->iNumber < 0) {
                            pitem->SetWorn(false, false, true);
                        }
                        int32_t iCount = pitem->GetAmountNonDefaultExtra();
                        if (iCount > 1) {
                            BSSimpleList<ExtraDataList*>* plistextra = pitem->pExtraObjectList;
                            while (plistextra && plistextra->m_item) {
                                ExtraDataList* pextraDataList = plistextra->m_item;
                                plistextra = plistextra->m_pkNext;
                                if (pextraDataList && pextraDataList->GetWorn(false) && pwornextra == nullptr) {
                                    pwornextra = pextraDataList;
                                }
                                else if (pwornextra != nullptr) {
                                    pwornextra->RemoveAll(true);
                                    pwornextra->DuplicateExtraListForContainer(pextraDataList);
                                    pwornextra->SetWorn(true, false);
                                    pitem->pExtraObjectList->RemoveAll();
                                    pitem->pExtraObjectList->AddHead(pwornextra);
                                }
                            }
                        }
                        int32_t iTotalCount = pitem->GetExtraTotalCount(false);
                        if (pitem->iNumber < iTotalCount) {
                            pitem->iNumber = pitem->GetExtraTotalCount(false);
                        }
                    }
                    else if (pitem->GetWorn(false)) {
                        int32_t iTotalCount = pitem->GetExtraTotalCount(false);
                        if (pitem->iNumber >= iTotalCount) {
                            iTotalCount = pitem->GetExtraTotalCount(false);
                            if (pitem->iNumber > iTotalCount) {
                                BSSimpleList<ExtraDataList*>* plistextra = pitem->pExtraObjectList;
                                while (plistextra && plistextra->m_item) {
                                    ExtraDataList* pextraDataList = plistextra->m_item;
                                    plistextra = plistextra->m_pkNext;
                                    if (pextraDataList && pextraDataList->GetWorn(false)) {
                                        pextraDataList->SetCount(pitem->iNumber);
                                    }
                                }
                            }
                        }
                        else {
                            pitem->iNumber = pitem->GetExtraTotalCount(false);
                        }
                    }
                }
                else if (pitem->iNumber > 0) {
                    if (pitem->iNumber >= pList->m_item->i_count) {
                        pitem->iNumber = 0;
                    }
                }
            }

            int32_t iamount = 0;
            int32_t iorigcount = pList->m_item->i_count;
            if (iorigcount < 0) {
                iorigcount = -iorigcount;
            }
            if (pitem) {
                iamount += iorigcount + pitem->iNumber;
            }
            else {
                iamount += iorigcount;
            }
            if (iamount > 0 && pitem && pitem->pExtraObjectList && pitem->pExtraObjectList->ItemsInList()) {
                BSSimpleList<ExtraDataList*>* plistextra = pitem->pExtraObjectList;
                while (true) {
                    // The compare is unsigned.
                    uint32_t uiCount = plistextra->ItemsInList();
                    if (static_cast<uint32_t>(iamount) >= uiCount) {
                        break;
                    }
                    ExtraDataList* pextraDataList = plistextra->m_item;
                    plistextra->RemoveHead();
                    if (pextraDataList) {
                        ExtraDataList* pnextExtraDataList = plistextra->m_item;
                        if (pnextExtraDataList) {
                            while (BSExtraData* pData = pextraDataList->pHead) {
                                if (pnextExtraDataList->GetExtraData(pData->cEtype)) {
                                    pextraDataList->RemoveExtra(pData, true);
                                }
                                else {
                                    pextraDataList->RemoveExtra(pData, false);
                                    pnextExtraDataList->AddExtra(pData);
                                }
                            }
                        }
                        delete pextraDataList;
                    }
                }
                if (plistextra->IsEmpty()) {
                    if (pitem->pExtraObjectList) {
                        delete pitem->pExtraObjectList;
                    }
                    pitem->pExtraObjectList = nullptr;
                }
                else {
                    plistextra = pitem->pExtraObjectList;
                }
                if (plistextra && plistextra->m_item) {
                    while (plistextra && plistextra->m_item) {
                        ExtraDataList* pextraDataList = plistextra->m_item;
                        if (pextraDataList->ItemsInList()) {
                            plistextra = plistextra->m_pkNext;
                        }
                        else {
                            plistextra->RemoveHead();
                            if (pitem->pExtraObjectList->IsEmpty()) {
                                plistextra->RemoveAll();
                                delete plistextra;
                                plistextra = nullptr;
                                pitem->pExtraObjectList = nullptr;
                            }
                            else {
                                plistextra = pitem->pExtraObjectList;
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
        if (GetOriginalContainer() && pInventoryItem) {
            TESBoundObject* pContainerObj = pInventoryItem->pContainerObj;
            TESContainer* pOriginalContainer = GetOriginalContainer();
            ioriginalcount = pOriginalContainer->GetObjectCount(pContainerObj);
        }

        // The count is read before the null test of the change.
        int32_t iamount = ioriginalcount + pInventoryItem->iNumber;
        if (pInventoryItem) {
            TESHealthForm* phealth = dynamic_cast<TESHealthForm*>(pInventoryItem->pContainerObj);
            if (phealth && pInventoryItem->pExtraObjectList) {
                BSSimpleList<ExtraDataList*>* plist = pInventoryItem->pExtraObjectList;
                while (plist && plist->m_item) {
                    ExtraDataList* pext = plist->m_item;
                    if (pext->GetHealth() > -1.0 &&
                        pext->GetHealth() == static_cast<double>(phealth->GetFormHealth())) {
                        pext->RemoveHealth();
                        int32_t icount = pext->GetCount();
                        pext->RemoveCount();
                        if (pext->IsExtraListEmpty()) {
                            plist->Remove(pext);
                            if (pext) {
                                delete pext;
                            }
                            plist = pInventoryItem->pExtraObjectList;
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
        int32_t iextracount = pInventoryItem->GetExtraTotalCount(false);
        pObj = pInventoryItem->pContainerObj;
        if (pInventoryItem && pInventoryItem->GetScript()) {
            for (BSSimpleList<ExtraDataList*>* plist = pInventoryItem->pExtraObjectList; plist && plist->m_item;
                 plist = plist->m_pkNext) {
                ExtraDataList* pext = plist->m_item;
                if (pext && pext->GetScript()) {
                    BSSimpleList<ExtraDataList*>* plist2 = plist->m_pkNext;
                    while (plist2 && plist2->m_item) {
                        ExtraDataList* pextra2 = plist2->m_item;
                        if (pext == pextra2) {
                            TESObjectREFR* pnewref;
                            ExtraDataList* pnewextra;
                            plist2->RemoveHead();
                            pnewextra = new ExtraDataList;
                            pnewextra->SetScript(pext->GetScript());
                            pnewextra->SetScriptLocals(pnewextra->GetScript()->CloneLocals());
                            pnewref = new TESObjectREFR;
                            pnewref->SetTemporary();
                            ScriptLocals* pLocals = pnewextra->GetScriptLocals();
                            pext->GetScript()->Run(pnewref, pLocals, nullptr, false);
                            plist2->AddTail(pnewextra);
                            plist2 = plist->m_pkNext;
                        }
                        else {
                            plist2 = plist2->m_pkNext;
                        }
                    }
                }
            }
        }

        if (pInventoryItem && pInventoryItem->iNumber >= 0) {
            Script* pScript = TESScriptableForm::GetFormScript(pInventoryItem->pContainerObj);
            if (pScript) {
                BSSimpleList<ExtraDataList*>* plist = pInventoryItem->pExtraObjectList;
                if (plist && !plist->IsEmpty()) {
                    int32_t iNumber = pInventoryItem->iNumber;
                    int32_t iadd = iNumber - plist->ItemsInList();
                    while (plist && plist->m_item) {
                        ExtraDataList* pext = plist->m_item;
                        if (pext && !pext->GetScript()) {
                            TESObjectREFR* pref;
                            pext->SetScript(pScript);
                            pext->SetScriptLocals(pext->GetScript()->CloneLocals());
                            pref = new TESObjectREFR;
                            pref->SetTemporary();
                            ScriptLocals* pLocals = pext->GetScriptLocals();
                            pInventoryItem->GetScript()->Run(pref, pLocals, nullptr, false);
                        }

                        plist = plist->m_pkNext;
                    }
                    plist = pInventoryItem->pExtraObjectList;
                    for (int32_t i = 0; i < iadd; ++i) {
                        TESObjectREFR* pref;
                        ExtraDataList* pextra;
                        pextra = new ExtraDataList;
                        pextra->SetScript(pInventoryItem->GetScript());
                        pextra->SetScriptLocals(pextra->GetScript()->CloneLocals());
                        pref = new TESObjectREFR;
                        pref->SetTemporary();
                        ScriptLocals* pLocals = pextra->GetScriptLocals();
                        pScript->Run(pref, pLocals, nullptr, false);
                        if (pref) {
                            delete pref;
                        }
                        plist->AddHead(pextra);
                    }
                }
                else {
                    InitScripts();
                }
            }
        }

        if (pInventoryItem && pObj->GetFormType() == AMMO_ID) {
            ioriginalcount = 0;
            if (GetOriginalContainer() && pInventoryItem) {
                TESBoundObject* pContainerObj = pInventoryItem->pContainerObj;
                TESContainer* pOriginalContainer = GetOriginalContainer();
                ioriginalcount = pOriginalContainer->GetObjectCount(pContainerObj);
            }

            if (pRef == pPlayer) {
                BSSimpleList<ExtraDataList*>* plistextra = pInventoryItem->pExtraObjectList;
                while (plistextra && plistextra->m_item) {
                    ExtraDataList* pextra = plistextra->m_item;
                    plistextra = plistextra->m_pkNext;
                    pextra->RemoveOwnership();
                }
            }
            if (pInventoryItem->GetWorn(false)) {
                BSSimpleList<ExtraDataList*>* plistextra = pInventoryItem->pExtraObjectList;
                int32_t icount = 0;
                while (plistextra && plistextra->m_item) {
                    icount += plistextra->m_item->GetCount();
                    plistextra = plistextra->m_pkNext;
                }
                if (ioriginalcount + pInventoryItem->iNumber >= icount) {
                    if (ioriginalcount + pInventoryItem->iNumber > icount) {
                        plistextra = pInventoryItem->pExtraObjectList;
                        while (plistextra && plistextra->m_item) {
                            ExtraDataList* pextraDataList = plistextra->m_item;
                            plistextra = plistextra->m_pkNext;
                            if (pextraDataList && pextraDataList->GetWorn(false)) {
                                pextraDataList->SetCount(pInventoryItem->iNumber);
                            }
                        }
                    }
                }
                else {
                    pInventoryItem->iNumber = icount - (ioriginalcount + pInventoryItem->iNumber);
                }
            }
        }

        if (pInventoryItem && pInventoryItem->pExtraObjectList) {
            for (ExtraDataList* pext : pInventoryItem->pExtraObjectList->ItemsUntilNull()) {
                if (pext && pext->GetScript()) {
                    pext->RemoveCount();
                }
            }
        }

        if (pInventoryItem && TESContainer::IsGold(pObj) && pRef == pPlayer && pInventoryItem->pExtraObjectList) {
            BSSimpleList<ExtraDataList*>* plist = pInventoryItem->pExtraObjectList;
            while (plist && plist->m_item) {
                ExtraDataList* pext = plist->m_item;
                plist->RemoveHead();
                if (pext) {
                    delete pext;
                }
            }
        }

        if (pInventoryItem && iamount < iextracount && pInventoryItem->pExtraObjectList) {
            int32_t inum = 0;
            for (ExtraDataList*& rpItem : pInventoryItem->pExtraObjectList->ItemsUntilNull()) {
                inum += rpItem->GetCount();
            }
            pInventoryItem->iNumber = inum;
        }

        if (pInventoryItem && iamount > 0) {
            BSSimpleList<ExtraDataList*>* plistextra = pInventoryItem->pExtraObjectList;
            if (plistextra && plistextra->m_item) {
                while (true) {
                    // The compare is unsigned.
                    uint32_t uiCount = plistextra->ItemsInList();
                    if (static_cast<uint32_t>(iamount) >= uiCount) {
                        break;
                    }
                    ExtraDataList* pextraDataList = plistextra->m_item;
                    plistextra->RemoveHead();
                    if (pextraDataList) {
                        ExtraDataList* pnextExtraDataList = plistextra->m_item;
                        if (pnextExtraDataList) {
                            while (BSExtraData* pData = pextraDataList->pHead) {
                                if (pnextExtraDataList->GetExtraData(pData->cEtype)) {
                                    pextraDataList->RemoveExtra(pData, true);
                                }
                                else {
                                    pextraDataList->RemoveExtra(pData, false);
                                    pnextExtraDataList->AddExtra(pData);
                                }
                            }
                        }
                        delete pextraDataList;
                    }
                }
                if (plistextra->IsEmpty()) {
                    if (pInventoryItem->pExtraObjectList) {
                        delete pInventoryItem->pExtraObjectList;
                    }
                    pInventoryItem->pExtraObjectList = nullptr;
                }
                else {
                    plistextra = pInventoryItem->pExtraObjectList;
                }
                while (plistextra && plistextra->m_item) {
                    if (plistextra->m_item->ItemsInList()) {
                        plistextra = plistextra->m_pkNext;
                    }
                    else {
                        plistextra->RemoveHead();
                        if (!pInventoryItem->pExtraObjectList->IsEmpty()) {
                            plistextra = pInventoryItem->pExtraObjectList;
                            continue;
                        }
                        plistextra->RemoveAll();
                        delete plistextra;
                        plistextra = nullptr;
                        pInventoryItem->pExtraObjectList = nullptr;
                    }
                }
            }
        }
    }
}

// Each extra list is copied with two plain locals: the new list (its address passed to AddTail) and the source node.
void InventoryChanges::DuplicateAllItems(TESObjectREFR* apRef, TESObjectREFR* apDupContainer) {
    if (apDupContainer && apRef) {
        TESBoundObject* pObj = nullptr;
        ItemChange* pNewItem = nullptr;
        InventoryChanges* pdupchange = InventoryChanges::GetInventoryChanges(apDupContainer);
        if (pdupchange) {
            BSSimpleList<ContainerObject*>* pList = &GetOriginalContainer()->objectList;
            while (pList && !pList->IsEmpty()) {
                pObj = pList->m_item->pObj;
                if (pObj) {
                    int32_t ioriginalcount = GetOriginalContainer()->GetObjectCount(pObj);
                    ItemChange* pitem = GetObjectInList(pObj, true, 0);
                    int32_t iamount = 0;
                    int32_t iorigcount = ioriginalcount;
                    if (ioriginalcount < 0) {
                        iorigcount = -iorigcount;
                    }
                    if (pitem) {
                        uint32_t uiNumber = pitem->iNumber;
                        iamount += iorigcount + uiNumber;
                    }
                    else {
                        iamount += iorigcount;
                    }
                    if (iamount > 0) {
                        TESForm* pNewForm = nullptr;
                        if (TESScriptableForm::GetFormScript(pObj)) {
                            pNewForm = TESDataHandler::CreateFormOfType(pObj->GetFormType());
                            pNewForm->Copy(pObj);
                            TESScriptableForm::SetFormScript(pNewForm, nullptr);
                            pDataHandler->AddFormToDataHandler(pNewForm);
                            pSaveLoadGameOLD->AddCreatedBaseObject(pNewForm);
                            int32_t icount = ioriginalcount;
                            if (pitem) {
                                uint32_t uiNumber = pitem->iNumber;
                                icount += uiNumber;
                            }
                            pNewItem = new ItemChange(static_cast<TESBoundObject*>(pNewForm), icount);
                            if (pitem && pitem->pExtraObjectList && pitem->pExtraObjectList->ItemsInList()) {
                                BSSimpleList<ExtraDataList*>* pnode = pitem->pExtraObjectList;
                                if (pnode && pnode->m_item) {
                                    while (pnode && pnode->m_item) {
                                        ExtraDataList* pextra = new ExtraDataList;
                                        BSExtraData* pScriptExtra = static_cast<BaseExtraList*>(pnode->m_item)
                                                                        ->GetExtraData(EXTRA_SCRIPT);
                                        if (pScriptExtra) {
                                            static_cast<BaseExtraList*>(pnode->m_item)
                                                ->RemoveExtra(pScriptExtra, false);
                                        }
                                        pextra->CopyListForContainer(pnode->m_item, true);
                                        pNewItem->pExtraObjectList->AddTail(pextra);
                                        if (pScriptExtra) {
                                            static_cast<BaseExtraList*>(pnode->m_item)->AddExtra(pScriptExtra);
                                        }
                                        pnode = pnode->m_pkNext;
                                    }
                                }
                            }
                        }
                        else {
                            pNewItem = new ItemChange(pObj, iamount);
                            if (pitem && pitem->pExtraObjectList && pitem->pExtraObjectList->ItemsInList()) {
                                BSSimpleList<ExtraDataList*>* pnode = pitem->pExtraObjectList;
                                if (pnode && pnode->m_item) {
                                    while (pnode && pnode->m_item) {
                                        ExtraDataList* pextra = new ExtraDataList;
                                        pextra->CopyListForContainer(pnode->m_item, true);
                                        pNewItem->pExtraObjectList->AddTail(pextra);
                                        pnode = pnode->m_pkNext;
                                    }
                                }
                            }
                        }
                        pdupchange->AddChangeItem(pNewItem, true);
                    }
                }
                pList = pList->m_pkNext;
            }
            for (BSSimpleList<ItemChange*>* pListInv = pListofChanges; pListInv && !pListInv->IsEmpty();
                 pListInv = pListInv->m_pkNext) {
                ItemChange* pInventoryItem = pListInv->m_item;
                if (pInventoryItem && pInventoryItem->iNumber > 0) {
                    pObj = pInventoryItem->pContainerObj;
                    pInventoryItem->RemoveLeveledItemExtra();
                    TESForm* pNewForm = nullptr;
                    if (pInventoryItem->GetScript()) {
                        pNewForm = TESDataHandler::CreateFormOfType(pInventoryItem->pContainerObj->GetFormType());
                        pNewForm->Copy(pInventoryItem->pContainerObj);
                        TESScriptableForm::SetFormScript(pNewForm, nullptr);
                        pDataHandler->AddFormToDataHandler(pNewForm);
                        pSaveLoadGameOLD->AddCreatedBaseObject(pNewForm);
                        pNewItem = new ItemChange(static_cast<TESBoundObject*>(pNewForm), pInventoryItem->iNumber);
                        if (pInventoryItem &&
                            pInventoryItem->pExtraObjectList &&
                            pInventoryItem->pExtraObjectList->ItemsInList()) {
                            BSSimpleList<ExtraDataList*>* pnode = pInventoryItem->pExtraObjectList;
                            if (pnode && pnode->m_item) {
                                while (pnode && pnode->m_item) {
                                    ExtraDataList* pextra = new ExtraDataList;
                                    BSExtraData* pScriptExtra = static_cast<BaseExtraList*>(pnode->m_item)
                                                                    ->GetExtraData(EXTRA_SCRIPT);
                                    if (pScriptExtra) {
                                        static_cast<BaseExtraList*>(pnode->m_item)->RemoveExtra(pScriptExtra, false);
                                    }
                                    pextra->CopyListForContainer(pnode->m_item, true);
                                    pNewItem->pExtraObjectList->AddTail(pextra);
                                    if (pScriptExtra) {
                                        static_cast<BaseExtraList*>(pnode->m_item)->AddExtra(pScriptExtra);
                                    }
                                    pnode = pnode->m_pkNext;
                                }
                            }
                        }
                    }
                    else {
                        pNewItem = new ItemChange(pInventoryItem->pContainerObj, pInventoryItem->iNumber);
                        if (pInventoryItem &&
                            pInventoryItem->pExtraObjectList &&
                            pInventoryItem->pExtraObjectList->ItemsInList()) {
                            BSSimpleList<ExtraDataList*>* pnode = pInventoryItem->pExtraObjectList;
                            if (pnode && pnode->m_item) {
                                while (pnode && pnode->m_item) {
                                    ExtraDataList* pextra = new ExtraDataList;
                                    pextra->CopyListForContainer(pnode->m_item, true);
                                    pNewItem->pExtraObjectList->AddTail(pextra);
                                    pnode = pnode->m_pkNext;
                                }
                            }
                        }
                    }
                    pdupchange->AddChangeItem(pNewItem, true);
                    pNewItem = nullptr;
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
        apRef,
        apStolenContainer,
        abStealing,
        abowner,
        abRemoveOwnership,
        abSilent,
        aiFormType,
        apDoNotRemoveList
    );
}

float InventoryChanges::GetEquippedWeight(Actor* apOwner) {
    float farmor;
    float famount = 0.0f;
    for (BSSimpleList<ItemChange*>* pInvList = pListofChanges; pInvList && !pInvList->IsEmpty() && pInvList->m_item;
         pInvList = pInvList->m_pkNext) {
        ItemChange* pitem = pInvList->m_item;
        TESObject* pObj = pitem->pContainerObj;
        if (pitem->GetWorn(false) && pObj->GetFormType() != AMMO_ID) {
            float fweight;
            if (pObj->GetFormType() == WEAP_ID) {
                fweight = static_cast<TESObjectWEAP*>(pObj)->TESObjectWEAP::GetFormWeight(pitem->HasModEffectActive(4));
                if (fweight >= 10.0) {
                    farmor = 1.0f;
                    BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::NOT_YET_IMPLEMENTED, pPlayer, &farmor);
                    fweight = fweight * farmor;
                }
            }
            else {
                fweight = TESWeightForm::GetFormWeight(pObj, pPlayer->IsHardcore());
            }
            int32_t icount = GetObjectCount(pitem->pContainerObj);
            if (pObj->GetFormType() != ARMO_ID) {
                if (pObj->GetFormType() == WEAP_ID) {
                    BaseProcess* pprocess = apOwner->pCurrentProcess;
                    if (pprocess) {
                        ItemChange* pweap = pprocess->GetCurrentWeapon();
                        ItemChange* pammo = pprocess->GetCurrentAmmo();
                        if (pweap && pammo) {
                            if (pweap->pContainerObj == pammo->pContainerObj) {
                                icount = 1;
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
    AutoMemContext tempMemContext(
        MEM_CONTEXT_INVENTORY,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp",
        9911
    );
    BSSimpleList<ContainerObject*>* plist = &GetOriginalContainer()->objectList;
    ItemChange* preturnitem = nullptr;
    while (plist && plist->m_item) {
        ContainerObject* pobject = plist->m_item;
        TESLevItem* pObj = dynamic_cast<TESLevItem*>(pobject->pObj);
        preturnitem = new ItemChange;
        TESBoundObject* pBoundObj = pobject->pObj;
        TESContainer* pOriginalContainer = GetOriginalContainer();
        int32_t iOriginalcount = pOriginalContainer->GetObjectCount(pBoundObj);
        if (!preturnitem->pExtraObjectList) {
            preturnitem->pExtraObjectList = new BSSimpleList<ExtraDataList*>;
        }
        if (!pobject || pObj) {
            if (preturnitem) {
                delete preturnitem;
            }
            preturnitem = nullptr;
        }
        else {
            ItemChange* pitem = GetObjectInList(pobject->pObj, true, 0);
            if (pitem) {
                pitem->RemoveOriginalChanges();
            }
            if (pobject &&
                pitem &&
                (((!pitem->HasLeveledExtra() || pitem->CountLeveledItems() < 1) &&
                  !(iOriginalcount + pitem->iNumber)) ||
                 (pitem->pContainerObj->GetQuestObject() && bdontcopyquestitems) ||
                 pitem->GetWorn(false))) {
                if (preturnitem) {
                    delete preturnitem;
                }
                preturnitem = nullptr;
            }
            else if (!pobject || (pitem && pitem->GetExtraTotalCount(false))) {
                if (pitem) {
                    // The loop runs only while preturnitem is null, and then uses it.
                    for (BSSimpleList<ExtraDataList*>* pextralist = pitem->pExtraObjectList;
                         pextralist && pextralist->m_item && !preturnitem;
                         pextralist = pextralist->m_pkNext) {
                        ExtraDataList* pcurextra = pextralist->m_item;
                        preturnitem->pContainerObj = pitem->pContainerObj;
                        int32_t iCount = pcurextra->GetCount();
                        preturnitem->iNumber = iCount;
                        if (!preturnitem->pExtraObjectList) {
                            preturnitem->pExtraObjectList = new BSSimpleList<ExtraDataList*>;
                        }
                        pcurextra->SetOriginalReferenceExtra(apRef);
                        preturnitem->pExtraObjectList->AddHead(pcurextra);
                        if (!preturnitem->GetWorn(false)) {
                            apCopy->AddChangeItem(preturnitem, true);
                        }
                    }
                    int32_t iTotalCount = pitem->GetExtraTotalCount(false);
                    if (iTotalCount < pitem->iNumber + iOriginalcount) {
                        if (!preturnitem->pExtraObjectList) {
                            preturnitem->pExtraObjectList = new BSSimpleList<ExtraDataList*>;
                        }
                        if (pitem) {
                            int32_t iNumber = pitem->iNumber + iOriginalcount;
                            preturnitem->iNumber = iNumber - pitem->GetExtraTotalCount(false);
                        }
                        else {
                            preturnitem->iNumber = iOriginalcount;
                        }
                        preturnitem->pContainerObj = pobject->pObj;
                        if (preturnitem->GetWorn(false)) {
                            if (preturnitem) {
                                delete preturnitem;
                            }
                            preturnitem = nullptr;
                        }
                        else {
                            ExtraDataList* pcurextra;
                            pcurextra = new ExtraDataList;
                            pcurextra->SetOriginalReferenceExtra(apRef);
                            int16_t sCount = preturnitem->iNumber;
                            pcurextra->SetCount(sCount);
                            preturnitem->pExtraObjectList->AddHead(pcurextra);
                            apCopy->AddChangeItem(preturnitem, true);
                        }
                    }
                }
                else {
                    if (preturnitem) {
                        delete preturnitem;
                    }
                    preturnitem = nullptr;
                }
            }
            else {
                int32_t icount = iOriginalcount;
                if (iOriginalcount < 0) {
                    icount = -icount;
                }
                if (pitem) {
                    int32_t iNumber = pitem->iNumber + icount;
                    preturnitem->iNumber = iNumber - pitem->GetExtraTotalCount(false);
                }
                else {
                    preturnitem->iNumber = icount;
                }
                preturnitem->pContainerObj = pobject->pObj;
                if (pitem && pitem->GetScript()) {
                    for (BSSimpleList<ExtraDataList*>* pextlist = pitem->pExtraObjectList; pextlist && pextlist->m_item;
                         pextlist = pextlist->m_pkNext) {
                        pextlist->m_item->SetOriginalReferenceExtra(apRef);
                        preturnitem->pExtraObjectList->AddTail(pextlist->m_item);
                    }
                    apCopy->AddChangeItem(preturnitem, true);
                }
                else if (preturnitem->GetWorn(false)) {
                    if (preturnitem) {
                        delete preturnitem;
                    }
                    preturnitem = nullptr;
                }
                else {
                    ExtraDataList* pcurextra;
                    pcurextra = new ExtraDataList;
                    pcurextra->SetOriginalReferenceExtra(apRef);
                    int16_t sCount = preturnitem->iNumber;
                    pcurextra->SetCount(sCount);
                    preturnitem->pExtraObjectList->AddHead(pcurextra);
                    apCopy->AddChangeItem(preturnitem, true);
                }
            }
        }
        plist = plist->m_pkNext;
    }
    for (ItemChange*& rpItem : pListofChanges->ItemsUntilNull()) {
        TESContainer* pOriginalContainer;
        TESBoundObject* pContainerObj;
        preturnitem = new ItemChange;
        if (!preturnitem->pExtraObjectList) {
            preturnitem->pExtraObjectList = new BSSimpleList<ExtraDataList*>;
        }
        ItemChange* pitem = rpItem;
        if (!pitem ||
            pitem->iNumber <= 0 ||
            (GetOriginalContainer() &&
             (pContainerObj = pitem->pContainerObj,
              pOriginalContainer = GetOriginalContainer(),
              pOriginalContainer->HasObject(pContainerObj)))) {
            if (preturnitem) {
                delete preturnitem;
            }
            preturnitem = nullptr;
        }
        else if (pitem->GetExtraTotalCount(false) || pitem->GetScript()) {
            BSSimpleList<ExtraDataList*>* pextralist = pitem->pExtraObjectList;
            int32_t icount = 0;
            bool leveledlist = false;
            while (pextralist && pextralist->m_item) {
                if (pextralist->m_item->GetExtraData(47)) {
                    leveledlist = true;
                }
                pextralist = pextralist->m_pkNext;
            }
            pextralist = pitem->pExtraObjectList;
            if (leveledlist && pitem->GetTypeforMenu() == 4) {
                if (pitem->GetTypeforMenu() == 4 && pitem->GetScript()) {
                    pitem->iNumber = 1;
                }

                while (pextralist && pextralist->m_item) {
                    ExtraDataList* pcurextra = pextralist->m_item;
                    pcurextra->RemoveCount();
                    pcurextra->SetOriginalReferenceExtra(apRef);
                    preturnitem->pExtraObjectList->AddHead(pcurextra);
                    pextralist = pextralist->m_pkNext;
                    icount = 1;
                }
            }
            else {
                while (pextralist && pextralist->m_item) {
                    ExtraDataList* pcurextra = pextralist->m_item;
                    pcurextra->SetOriginalReferenceExtra(apRef);
                    preturnitem->pExtraObjectList->AddHead(pcurextra);
                    pextralist = pextralist->m_pkNext;
                    icount += pcurextra->GetCount();
                }
            }
            preturnitem->pContainerObj = pitem->pContainerObj;
            uint32_t uiNumber = pitem->iNumber;
            preturnitem->iNumber = uiNumber;
            if (!preturnitem->GetWorn(false)) {
                if (icount < pitem->iNumber) {
                    ExtraDataList* pcurextra;
                    pcurextra = new ExtraDataList;
                    pcurextra->SetOriginalReferenceExtra(apRef);
                    int16_t sNumber = pitem->iNumber;
                    pcurextra->SetCount(sNumber - icount);
                    preturnitem->pExtraObjectList->AddHead(pcurextra);
                }
            }
            if (preturnitem->GetWorn(false)) {
                if (preturnitem->pExtraObjectList->m_item) {
                    preturnitem->pExtraObjectList->m_item->RemoveOriginalReferenceExtra();
                }
                if (preturnitem) {
                    delete preturnitem;
                }
                preturnitem = nullptr;
            }
            else {
                apCopy->AddChangeItem(preturnitem, true);
            }
        }
        else {
            ExtraDataList* pcurextra = nullptr;
            if (pitem->pExtraObjectList) {
                BSSimpleList<ExtraDataList*>* pextralist = pitem->pExtraObjectList;
                int32_t icount = 0;
                while (pextralist && pextralist->m_item && !pcurextra) {
                    pcurextra = pextralist->m_item;
                    icount += pcurextra->GetCount();
                    if (pcurextra->IsExtraDefaultforContainer(false)) {
                        pcurextra = nullptr;
                    }
                    pextralist = pextralist->m_pkNext;
                }
                if (pcurextra) {
                    if (!preturnitem->pExtraObjectList) {
                        preturnitem->pExtraObjectList = new BSSimpleList<ExtraDataList*>;
                    }
                    pcurextra->SetOriginalReferenceExtra(apRef);
                    preturnitem->pExtraObjectList->AddHead(pcurextra);
                    if (!preturnitem->GetWorn(false)) {
                        if (icount < pitem->iNumber) {
                            ExtraDataList* pnewextra;
                            pnewextra = new ExtraDataList;
                            pnewextra->SetOriginalReferenceExtra(apRef);
                            int16_t sNumber = pitem->iNumber;
                            pnewextra->SetCount(sNumber - icount);
                            preturnitem->pExtraObjectList->AddHead(pnewextra);
                        }
                    }
                }
                else {
                    ExtraDataList* pnewextra;
                    pnewextra = new ExtraDataList;
                    pnewextra->SetOriginalReferenceExtra(apRef);
                    int16_t sNumber = pitem->iNumber;
                    pnewextra->SetCount(sNumber);
                    preturnitem->pExtraObjectList->AddHead(pnewextra);
                }
                preturnitem->pContainerObj = pitem->pContainerObj;
                uint32_t uiNumber = pitem->iNumber;
                preturnitem->iNumber = uiNumber;
                if (preturnitem->GetWorn(false)) {
                    if (preturnitem->pExtraObjectList->m_item) {
                        preturnitem->pExtraObjectList->m_item->RemoveOriginalReferenceExtra();
                    }
                    if (preturnitem) {
                        delete preturnitem;
                    }
                    preturnitem = nullptr;
                }
                else {
                    uiNumber = pitem->iNumber;
                    preturnitem->iNumber = uiNumber;
                    apCopy->AddChangeItem(preturnitem, true);
                }
            }
            else {
                preturnitem->pContainerObj = pitem->pContainerObj;
                uint32_t uiNumber = pitem->iNumber;
                preturnitem->iNumber = uiNumber;
                pcurextra = new ExtraDataList;
                pcurextra->SetOriginalReferenceExtra(apRef);
                int16_t sCount = preturnitem->iNumber;
                pcurextra->SetCount(sCount);
                preturnitem->pExtraObjectList->AddHead(pcurextra);
                apCopy->AddChangeItem(preturnitem, true);
            }
        }
    }
}

void InventoryChanges::GetRepairItemGroup(TESBoundObject* apObj, BSSimpleList<ItemChange*>* apReturnList) {
    if (apObj && apReturnList) {
        IterData* pIterator = GetIterator();
        ItemChange* pitem = nullptr;
        do {
            pitem = Iterate(pIterator);
            if (pitem) {
                if (BGSRepairItemList::CanItemRepairForm(pitem->pContainerObj, apObj)) {
                    apReturnList->AddHead(pitem);
                }
                else if (pitem) {
                    delete pitem;
                }
            }
        }
        while (pitem);
        if (pIterator) {
            delete pIterator;
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
        return nullptr;
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
            return nullptr;
        }
    }
    return nullptr;
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
                MEM_CONTEXT_INVENTORY,
                true,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\InventoryChanges.cpp",
                1821
            );
            pChanges = new InventoryChanges(apRef);
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
    InventoryChanges::pTempRef = nullptr;
}

// Whether the ammo is playable: the non-playable flag of data.cFlags is clear.
bool TESAmmo::GetIsPlayable() {
    return (data.cFlags & AMMO_DATA::FLAG_NON_PLAYABLE) == 0;
}

// Returns whether the heavy-armor flag is set.
bool TESBipedModelForm::IsHeavy() {
    return (bipedModelData.sFlags & BIPED_MODEL::HEAVY) != 0;
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
        double dWeight = static_cast<TESWeightForm*>(this)->GetWeight();
        return static_cast<float>(dWeight - GetModEffectValue(WEAPON_MOD_DECREASE_WEIGHT, 0));
    }
    return static_cast<TESWeightForm*>(this)->GetWeight();
}

// Calls TESHealthForm's getter qualified, after the mod value; the sum is in double and truncated to 32 bits.
int32_t TESObjectWEAP::GetFormHealth(bool bModValue) {
    if (bModValue) {
        double dModValue = GetModEffectValue(WEAPON_MOD_MAX_HEALTH, 0);
        return static_cast<int32_t>(TESHealthForm::GetFormHealth() + dModValue);
    }
    return static_cast<int32_t>(TESHealthForm::GetFormHealth());
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
