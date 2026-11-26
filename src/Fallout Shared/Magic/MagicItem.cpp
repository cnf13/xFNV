#include "Fallout Shared/Magic/MagicItem.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/BGSTextureUseMap.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/EffectItemList.h"
#include "Fallout Shared/Magic/MagicItemForm.h"
#include "Fallout Shared/Magic/MagicItemObject.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/Settings/XGameSetting.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObjectARMO.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout/Interface/Interface.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"

SettingT<GameSettingCollection> sMagicCastOKText("sMagicCastOKText", "");
SettingT<GameSettingCollection>
    sMagicCastInsufficientMagicka("sMagicCastInsufficientMagicka", "You don't have enough Magicka");
SettingT<GameSettingCollection>
    sMagicCastInsufficientSkill("sMagicCastInsufficientSkill", "Your Skill level is too low");
SettingT<GameSettingCollection> sMagicCastSilenced("sMagicCastSilenced", "You are currently Silenced");
SettingT<GameSettingCollection> sMagicCastPowerUsed("sMagicCastPowerUsed", "You can only cast Powers once per day");
SettingT<GameSettingCollection>
    sMagicCastMultipleBoundEffects("sMagicCastMultipleBoundEffects", "You already have that kind of Bound Item");
SettingT<GameSettingCollection>
    sMagicCastRangedUnderwater("sMagicCastRangedUnderwater", "You cannot cast ranged spells underwater");
static SettingT<GameSettingCollection> iMagicMaxSummonedCreatureTypes("iMagicMaxSummonedCreatureTypes", 10);

// The effect list's own destructor and TESFullName's run after this.
MagicItem::~MagicItem() {
    RemoveAllEffects();
}

void MagicItem::Preload(bool abNow) {
    EffectSetting* pAVEffectSetting = GetAVEffect(MagicSystem::SELF);
    bool bRunNow = false;
    bool bLoadNow = false;
    if (abNow || IsLoaded()) {
        bRunNow = true;
        bLoadNow = true;
    }
    else if (IsLoading()) {
        bRunNow = true;
        bLoadNow = false;
    }
    if (bRunNow) {
        if (pAVEffectSetting) {
            if (bLoadNow && pAVEffectSetting->GetModelLength()) {
                const char* pModel = pAVEffectSetting->GetModel();
                pModelLoader->LoadFile(pModel, LOD_MULT_NONE, true, false, false, false);
            }

            pAVEffectSetting->IncEffectLoadedCount();
        }
        EffectItemList* pList = nullptr;
        if (this) {
            pList = static_cast<EffectItemList*>(this);
        }
        if (!pList->IsEmpty()) {
            while (pList) {
                EffectItem* pItem = pList->m_item;
                EffectSetting* pES = pItem->pEffectSetting;
                if (pES->getArchType()) {
                    if (bLoadNow) {
                        TESForm* pForm = pES->GetAssociatedItem();
                        if (pForm && pES->IsFlagSet(EffectSetting::USE_ASSOCIATED_WEAPON)) {
                            TESObjectWEAP* pWeapon = dynamic_cast<TESObjectWEAP*>(pForm);
                            if (pWeapon && pWeapon->GetModelLength()) {
                                const char* pModel = pWeapon->GetModel();
                                pModelLoader->LoadFile(pModel, LOD_MULT_NONE, true, false, false, false);
                            }
                        }
                        else if (pForm && pES->IsFlagSet(EffectSetting::USE_ASSOCIATED_ARMOR)) {
                            TESObjectARMO* pArmor = dynamic_cast<TESObjectARMO*>(pForm);
                            if (pArmor) {
                                const char* pMaleModel = pArmor->GetBipedModel(TESActorBase::SEX_MALE);
                                pModelLoader->LoadFile(pMaleModel, LOD_MULT_NONE, true, false, false, false);
                                const char* pFemaleModel = pArmor->GetBipedModel(TESActorBase::SEX_FEMALE);
                                pModelLoader->LoadFile(pFemaleModel, LOD_MULT_NONE, true, false, false, false);
                            }
                        }
                        else if (pForm && pES->IsFlagSet(EffectSetting::USE_ASSOCIATED_CREATURE)) {
                            TESNPC* pNPC = dynamic_cast<TESNPC*>(pForm);
                            if (pNPC) {
                                BSSimpleList<const char*>* pModelList = pNPC->BuildDefaultModelList(true, true);
                                for (BSSimpleList<const char*>* pModelNode = pModelList;
                                     pModelNode && !pModelNode->IsEmpty();
                                     pModelNode = pModelNode->GetNext()) {
                                    const char* pFilename = pModelNode->m_item;
                                    pModelLoader->LoadFile(pFilename, LOD_MULT_NONE, true, false, false, false);
                                }
                                pModelList->RemoveAll();
                                if (pModelList) {
                                    delete pModelList;
                                }
                            }
                            TESCreature* pCreature = dynamic_cast<TESCreature*>(pForm);
                            if (pCreature) {
                                char acPath[264];
                                const char* pModel = pCreature->GetModel();
                                pModelLoader->LoadFile(pModel, LOD_MULT_NONE, true, false, false, false);
                                const char* pFilename = pCreature->GetModel();
                                BSstrcpy(acPath, 260, pFilename);
                                char* pLastSlash = std::strrchr(acPath, 92);
                                for (BSSimpleList<const char*>* pFileList = &pCreature->nifFilenames;
                                     pLastSlash && pFileList;
                                     pFileList = pFileList->GetNext()) {
                                    pLastSlash[1] = 0;
                                    const char* pNifName = pFileList->m_item;
                                    if (pNifName) {
                                        BSstrcat(acPath, 260, pNifName);
                                        pModelLoader->LoadFile(acPath, LOD_MULT_NONE, true, false, false, false);
                                    }
                                }
                                const char* pLastBackSlash = std::strrchr(pFilename, 92);
                                if (pLastBackSlash && !_strnicmp(pLastBackSlash + 1, "Skeleton", 8u)) {
                                    BSSimpleList<const char*>* pFilenameList = pModelLoader->BuildKFFileList(
                                        pFilename,
                                        false,
                                        false,
                                        AGW_NONE
                                    );
                                    for (BSSimpleList<const char*>* pNode = pFilenameList; pNode;
                                         pNode = pNode->GetNext()) {
                                        const char* pKFName = pNode->m_item;
                                        if (pKFName) {
                                            pModelLoader->LoadKF(pKFName);
                                        }
                                        // The list holds the heap copies BuildKFFileList made, typed
                                        // const char*; the item is freed here.
                                        ourFree(const_cast<char*>(pKFName));
                                    }
                                    pFilenameList->RemoveAll();
                                    if (pFilenameList) {
                                        delete pFilenameList;
                                    }
                                }

                                if (pCreature->HasKFFiles()) {
                                    pLastSlash[1] = 0;
                                    BSstrcat(acPath, 260, "SpecialAnims");
                                    BSstrcat(acPath, 260, "\\");
                                    pLastSlash = std::strrchr(acPath, 92);
                                    for (char* pAnimName : pCreature->kfFilenames) {
                                        pLastSlash[1] = 0;
                                        if (pAnimName) {
                                            BSstrcat(acPath, 260, pAnimName);
                                            pModelLoader->LoadKF(acPath);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    if (!pES->IsAssociatedItemLoaded() &&
                        !pES->IsAssociatedItemQueued() &&
                        pES->IsFlagSet(EffectSetting::USE_ASSOCIATED_CREATURE)) {
                        MagicItem::IncSummonedCreatureLoadedCount();
                    }
                    pES->IncAssociatedItemLoadedCount();
                }
                pList = static_cast<EffectItemList*>(pList->m_pkNext);
            }
        }
    }
    else {
        pModelLoader->QueueMagicItem(this, pAVEffectSetting, IO_TASK_PRIORITY_CRITICAL);
    }
}

void MagicItem::Unload(bool abRelease) {
    if (IsLoadingOrLoaded()) {
        EffectSetting* pAVEffectSetting = GetAVEffect(MagicSystem::SELF);
        bool bInProgess = IsLoading();
        if (pAVEffectSetting && pAVEffectSetting->GetModelLength()) {
            if (!bInProgess) {
                const char* pModel = pAVEffectSetting->GetModel();
                pModelLoader->ReleaseModel(pModel, abRelease, 1u);
            }
            pAVEffectSetting->DecEffectLoadedCount();
        }

        EffectItemList* pList = nullptr;
        if (this) {
            pList = static_cast<EffectItemList*>(this);
        }
        if (!pList->IsEmpty()) {
            while (pList) {
                EffectItem* pItem = pList->m_item;
                EffectSetting* pES = pItem->pEffectSetting;
                if (pES->getArchType()) {
                    if (!bInProgess) {
                        TESForm* pForm = pES->GetAssociatedItem();
                        if (pForm && pES->IsFlagSet(EffectSetting::USE_ASSOCIATED_WEAPON)) {
                            TESObjectWEAP* pWeapon = dynamic_cast<TESObjectWEAP*>(pForm);
                            if (pWeapon && pWeapon->GetModelLength()) {
                                const char* pModel = pWeapon->GetModel();
                                pModelLoader->ReleaseModel(pModel, abRelease, 1u);
                            }
                        }
                        else if (pForm && pES->IsFlagSet(EffectSetting::USE_ASSOCIATED_ARMOR)) {
                            TESObjectARMO* pArmor = dynamic_cast<TESObjectARMO*>(pForm);
                            if (pArmor) {
                                const char* pMaleModel = pArmor->GetBipedModel(TESActorBase::SEX_MALE);
                                pModelLoader->ReleaseModel(pMaleModel, abRelease, 1u);
                                const char* pFemaleModel = pArmor->GetBipedModel(TESActorBase::SEX_FEMALE);
                                pModelLoader->ReleaseModel(pFemaleModel, abRelease, 1u);
                            }
                        }
                        else if (pForm && pES->IsFlagSet(EffectSetting::USE_ASSOCIATED_CREATURE)) {
                            TESNPC* pNPC = dynamic_cast<TESNPC*>(pForm);
                            if (pNPC) {
                                BSSimpleList<const char*>* pModelList = pNPC->BuildDefaultModelList(true, true);
                                for (BSSimpleList<const char*>* pModelNode = pModelList;
                                     pModelNode && !pModelNode->IsEmpty();
                                     pModelNode = pModelNode->GetNext()) {
                                    const char* pFilename = pModelNode->m_item;
                                    pModelLoader->ReleaseModel(pFilename, abRelease, 1u);
                                }
                                pModelList->RemoveAll();
                                if (pModelList) {
                                    delete pModelList;
                                }
                            }
                            TESCreature* pCreature = dynamic_cast<TESCreature*>(pForm);
                            if (pCreature) {
                                char acPath[268];
                                const char* pModel = pCreature->GetModel();
                                pModelLoader->ReleaseModel(pModel, abRelease, 1u);
                                const char* pFilename = pCreature->GetModel();
                                BSstrcpy(acPath, 260, pFilename);
                                char* pLastSlash = std::strrchr(acPath, 92);
                                for (BSSimpleList<const char*>* pFileList = &pCreature->nifFilenames;
                                     pLastSlash && pFileList;
                                     pFileList = pFileList->GetNext()) {
                                    pLastSlash[1] = 0;
                                    const char* pNifName = pFileList->m_item;
                                    if (pNifName) {
                                        BSstrcat(acPath, 260, pNifName);
                                        pModelLoader->ReleaseModel(acPath, abRelease, 1u);
                                    }
                                }
                                const char* pLastBackSlash = std::strrchr(pFilename, 92);
                                if (pLastBackSlash && !_strnicmp(pLastBackSlash + 1, "Skeleton", 8u)) {
                                    BSSimpleList<const char*>* pFilenameList = pModelLoader->BuildKFFileList(
                                        pFilename,
                                        false,
                                        false,
                                        AGW_NONE
                                    );
                                    for (BSSimpleList<const char*>* pNode = pFilenameList; pNode;
                                         pNode = pNode->GetNext()) {
                                        const char* pKFName = pNode->m_item;
                                        if (pKFName) {
                                            pModelLoader->ReleaseKF(pKFName, abRelease);
                                        }
                                        // The list holds the heap copies BuildKFFileList made, typed
                                        // const char*; the item is freed here.
                                        ourFree(const_cast<char*>(pKFName));
                                    }
                                    pFilenameList->RemoveAll();
                                    if (pFilenameList) {
                                        delete pFilenameList;
                                    }
                                }

                                if (pCreature->HasKFFiles()) {
                                    pLastSlash[1] = 0;
                                    BSstrcat(acPath, 260, "SpecialAnims");
                                    BSstrcat(acPath, 260, "\\");
                                    pLastSlash = std::strrchr(acPath, 92);
                                    for (char* pAnimName : pCreature->kfFilenames) {
                                        pLastSlash[1] = 0;
                                        if (pAnimName) {
                                            BSstrcat(acPath, 260, pAnimName);
                                            pModelLoader->ReleaseKF(acPath, abRelease);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    pES->DecAssociatedItemLoadedCount();
                    if (!pES->IsAssociatedItemLoaded() &&
                        !pES->IsAssociatedItemQueued() &&
                        pES->IsFlagSet(EffectSetting::USE_ASSOCIATED_CREATURE)) {
                        MagicItem::DecSummonedCreatureLoadedCount();
                    }
                }
                pList = static_cast<EffectItemList*>(pList->m_pkNext);
            }
        }
    }
}

bool MagicItem::IsLoaded() {
    EffectSetting* pAVEffectSetting = GetAVEffect(MagicSystem::SELF);
    if (pAVEffectSetting && pAVEffectSetting->GetModelLength() && !pAVEffectSetting->IsEffectLoaded()) {
        return false;
    }
    EffectItemList* pList = nullptr;
    if (this) {
        pList = static_cast<EffectItemList*>(this);
    }
    if (!pList->IsEmpty()) {
        while (pList) {
            EffectItem* pItem = pList->m_item;
            EffectSetting* pES = pItem ? pItem->pEffectSetting : nullptr;
            if (pES && pES->getArchType() && !pES->IsAssociatedItemLoaded()) {
                return false;
            }
            pList = static_cast<EffectItemList*>(pList->m_pkNext);
        }
    }
    return true;
}

bool MagicItem::IsLoading() {
    EffectSetting* pAVEffectSetting = GetAVEffect(MagicSystem::SELF);
    bool bFinished = true;
    if (pAVEffectSetting && pAVEffectSetting->GetModelLength() && !pAVEffectSetting->IsEffectLoaded()) {
        bFinished = false;
        if (!pAVEffectSetting->IsEffectQueued()) {
            return false;
        }
    }

    EffectItemList* pList = nullptr;
    if (this) {
        pList = static_cast<EffectItemList*>(this);
    }
    if (!pList->IsEmpty()) {
        while (pList) {
            EffectItem* pItem = pList->m_item;
            EffectSetting* pES = pItem ? pItem->pEffectSetting : nullptr;
            if (pES && pES->getArchType() && !pES->IsAssociatedItemLoaded()) {
                bFinished = false;
                if (!pES->IsAssociatedItemQueued()) {
                    return false;
                }
            }

            pList = static_cast<EffectItemList*>(pList->m_pkNext);
        }
    }
    return !bFinished;
}

void MagicItem::FinishedLoading() {
    EffectSetting* pAVEffectSetting = GetAVEffect(MagicSystem::SELF);
    if (IsLoading()) {
        if (pAVEffectSetting) {
            int32_t iCount = 0;
            if (pAVEffectSetting->IsEffectQueued()) {
                iCount = pAVEffectSetting->QueueEffectFinished();
            }
            if (pAVEffectSetting->GetModelLength()) {
                NiPointer<Model> spModel;
                const char* pModel = pAVEffectSetting->GetModel();
                pModelLoader->FindModel(pModel, spModel);
                if (spModel.m_pObject) {
                    spModel->ModManualRefCount(iCount);
                }
            }
        }
        EffectItemList* pList = nullptr;
        if (this) {
            pList = static_cast<EffectItemList*>(this);
        }
        if (!pList->IsEmpty()) {
            while (pList) {
                EffectItem* pItem = pList->m_item;
                EffectSetting* pES = pItem->pEffectSetting;
                TESForm* pForm = pES->GetAssociatedItem();
                int32_t iCount = 0;
                if (pES->IsAssociatedItemQueued()) {
                    iCount = pES->QueueAssociatedItemFinished();
                }
                if (iCount) {
                    if (pForm && pES->IsFlagSet(EffectSetting::USE_ASSOCIATED_WEAPON)) {
                        TESObjectWEAP* pWeapon = dynamic_cast<TESObjectWEAP*>(pForm);
                        if (pWeapon && pWeapon->GetModelLength()) {
                            NiPointer<Model> spModel;
                            const char* pModel = pWeapon->GetModel();
                            pModelLoader->FindModel(pModel, spModel);
                            if (spModel.m_pObject) {
                                spModel->ModManualRefCount(iCount);
                            }
                        }
                    }
                    else if (pForm && pES->IsFlagSet(EffectSetting::USE_ASSOCIATED_ARMOR)) {
                        TESObjectARMO* pArmor = dynamic_cast<TESObjectARMO*>(pForm);
                        if (pArmor) {
                            NiPointer<Model> spModel;
                            const char* pMaleModel = pArmor->GetBipedModel(TESActorBase::SEX_MALE);
                            pModelLoader->FindModel(pMaleModel, spModel);
                            if (spModel.m_pObject) {
                                spModel->ModManualRefCount(iCount);
                            }
                            const char* pFemaleModel = pArmor->GetBipedModel(TESActorBase::SEX_FEMALE);
                            pModelLoader->FindModel(pFemaleModel, spModel);
                            if (spModel.m_pObject) {
                                spModel->ModManualRefCount(iCount);
                            }
                        }
                    }
                    else if (pForm && pES->IsFlagSet(EffectSetting::USE_ASSOCIATED_CREATURE)) {
                        TESNPC* pNPC = dynamic_cast<TESNPC*>(pForm);
                        if (pNPC) {
                            BSSimpleList<const char*>* pModelList = pNPC->BuildDefaultModelList(true, true);
                            BSSimpleList<const char*>* pModelNode = pModelList;
                            while (pModelNode && !pModelNode->IsEmpty()) {
                                const char* pFilename = pModelNode->m_item;
                                NiPointer<Model> spModel;
                                pModelLoader->FindModel(pFilename, spModel);
                                if (spModel.m_pObject) {
                                    spModel->ModManualRefCount(iCount);
                                }
                                pModelNode = pModelNode->GetNext();
                            }
                            pModelList->RemoveAll();
                            if (pModelList) {
                                delete pModelList;
                            }
                        }
                        TESCreature* pCreature = dynamic_cast<TESCreature*>(pForm);
                        if (pCreature) {
                            NiPointer<Model> spModel;
                            const char* pModel = pCreature->GetModel();
                            pModelLoader->FindModel(pModel, spModel);
                            if (spModel.m_pObject) {
                                char acPath[268];
                                for (int32_t i = 0; i < iCount; ++i) {
                                    spModel->IncManualRefCount();
                                }
                                const char* pFilename = spModel->pFilename;
                                BSstrcpy(acPath, 260, pFilename);
                                char* pLastSlash = std::strrchr(acPath, 92);
                                for (BSSimpleList<const char*>* pFileList = &pCreature->nifFilenames;
                                     pLastSlash && pFileList;
                                     pFileList = pFileList->GetNext()) {
                                    pLastSlash[1] = 0;
                                    const char* pNifName = pFileList->m_item;
                                    if (pNifName) {
                                        BSstrcat(acPath, 260, pNifName);
                                        NiPointer<Model> spNifModel;
                                        pModelLoader->FindModel(acPath, spNifModel);
                                        if (spNifModel.m_pObject) {
                                            spNifModel->ModManualRefCount(iCount);
                                        }
                                    }
                                }
                                const char* pLastBackSlash = std::strrchr(pFilename, 92);
                                if (pLastBackSlash && !_strnicmp(pLastBackSlash + 1, "Skeleton", 8u)) {
                                    BSSimpleList<const char*>* pFilenameList = pModelLoader->BuildKFFileList(
                                        pFilename,
                                        false,
                                        false,
                                        AGW_NONE
                                    );
                                    for (BSSimpleList<const char*>* pNode = pFilenameList; pNode;
                                         pNode = pNode->GetNext()) {
                                        const char* pKFName = pNode->m_item;
                                        if (pKFName) {
                                            for (int32_t i = 0; i < iCount; ++i) {
                                                pModelLoader->LoadKF(pKFName);
                                            }
                                        }
                                        // The list holds the heap copies BuildKFFileList made, typed
                                        // const char*; the item is freed here.
                                        ourFree(const_cast<char*>(pKFName));
                                    }
                                    pFilenameList->RemoveAll();
                                    if (pFilenameList) {
                                        delete pFilenameList;
                                    }
                                }

                                if (pCreature->HasKFFiles()) {
                                    pLastSlash[1] = 0;
                                    BSstrcat(acPath, 260, "SpecialAnims");
                                    BSstrcat(acPath, 260, "\\");
                                    pLastSlash = std::strrchr(acPath, 92);
                                    for (char* pAnimName : pCreature->kfFilenames) {
                                        pLastSlash[1] = 0;
                                        if (pAnimName) {
                                            BSstrcat(acPath, 260, pAnimName);
                                            for (int32_t i = 0; i < iCount; ++i) {
                                                pModelLoader->LoadKF(acPath);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                pList = static_cast<EffectItemList*>(pList->m_pkNext);
            }
        }
    }
    else {
        Preload(true);
        Unload(true);
    }
}

uint32_t MagicItem::GetMagicItemFormID() {
    MagicItemForm* pForm = dynamic_cast<MagicItemForm*>(this);
    MagicItemObject* pObject = dynamic_cast<MagicItemObject*>(this);
    if (pForm) {
        return pForm->TESForm::GetFormID();
    }
    if (pObject) {
        return pObject->TESForm::GetFormID();
    }
    return 0;
}

EffectSetting* MagicItem::GetAVEffect(MagicSystem::Range aeRange) {
    EffectItem* pAVEffect = nullptr;
    EffectItemList* pList = nullptr;
    if (this) {
        pList = static_cast<EffectItemList*>(this);
    }
    if (pList->IsEmpty()) {
        return nullptr;
    }
    int32_t iMaxCost = -1;
    while (pList) {
        EffectItem* pItem = pList->m_item;
        int32_t iThisCost = static_cast<int32_t>(pItem->GetCost(nullptr));
        MagicSystem::Range eThisRange = pItem->data.iRange;
        if (!pItem->IsDisabled() && (eThisRange > aeRange || (eThisRange == aeRange && iThisCost > iMaxCost))) {
            pAVEffect = pItem;
            iMaxCost = iThisCost;
            aeRange = pItem->data.iRange;
        }
        pList = static_cast<EffectItemList*>(pList->m_pkNext);
    }
    if (pAVEffect) {
        return pAVEffect->pEffectSetting;
    }

    return nullptr;
}

BSStringT<char> MagicItem::GetCannotCastString(CannotCastReason aeReason) {
    switch (aeReason) {
        case 1: return sMagicCastInsufficientMagicka.String();
        case 2: return sMagicCastSilenced.String();
        case 3: return sMagicCastInsufficientSkill.String();
        case 4: return sMagicCastPowerUsed.String();
        case 5: return sMagicCastMultipleBoundEffects.String();
        case 6: return sMagicCastRangedUnderwater.String();
        default: return sMagicCastOKText.String();
    }
}

void MagicItem::MagicItemInitItem(TESForm* apForm) {
    EffectItemList* pList = nullptr;
    if (this) {
        pList = static_cast<EffectItemList*>(this);
    }
    iHostileCount = 0;
    while (pList) {
        EffectItem* pItem = pList->m_item;
        if (pItem) {
            pItem->InitItem(apForm);
            if (pItem->IsHostile()) {
                ++iHostileCount;
            }
        }
        pList = static_cast<EffectItemList*>(pList->m_pkNext);
    }
}

bool MagicItem::IsLoadingOrLoaded() {
    EffectSetting* pAVEffectSetting = GetAVEffect(MagicSystem::SELF);
    if (pAVEffectSetting &&
        pAVEffectSetting->GetModelLength() &&
        !pAVEffectSetting->IsEffectLoaded() &&
        !pAVEffectSetting->IsEffectQueued()) {
        return false;
    }
    EffectItemList* pList = nullptr;
    if (this) {
        pList = static_cast<EffectItemList*>(this);
    }
    if (!pList->IsEmpty()) {
        while (pList) {
            EffectItem* pItem = pList->m_item;
            EffectSetting* pES = pItem ? pItem->pEffectSetting : nullptr;
            if (pES && pES->getArchType() && !pES->IsAssociatedItemLoaded() && !pES->IsAssociatedItemQueued()) {
                return false;
            }
            pList = static_cast<EffectItemList*>(pList->m_pkNext);
        }
    }
    return true;
}

void MagicItem::DecSummonedCreatureLoadedCount() {
    --MagicItem::iSummonedCreatureLoadedCount;
}

MagicItem* MagicItem::GetMagicItemByNumericID(uint32_t aiFormID) {
    TESForm* pForm = TESForm::GetFormByNumericID(aiFormID);
    if (!pForm) {
        return nullptr;
    }
    MagicItemForm* pMagicItemForm = dynamic_cast<MagicItemForm*>(pForm);
    MagicItemObject* pMagicItemObject = dynamic_cast<MagicItemObject*>(pForm);
    if (pMagicItemForm) {
        return static_cast<MagicItem*>(pMagicItemForm);
    }
    if (pMagicItemObject) {
        return static_cast<MagicItem*>(pMagicItemObject);
    }

    return nullptr;
}

int32_t MagicItem::IncSummonedCreatureLoadedCount() {
    return ++MagicItem::iSummonedCreatureLoadedCount;
}

EffectItemList::~EffectItemList() {}
