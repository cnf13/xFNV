#include "Fallout Shared/Magic/MagicItem.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/BGSTextureUseMap.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/EffectItemList.h"
#include "Fallout Shared/Magic/MagicItemForm.h"
#include "Fallout Shared/Magic/MagicItemObject.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObjectARMO.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout/Interface/Interface.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"

SettingT<GameSettingCollection> gs_sMagicCastOKText("sMagicCastOKText", "");
SettingT<GameSettingCollection>
    gs_sMagicCastInsufficientMagicka("sMagicCastInsufficientMagicka", "You don't have enough Magicka");
SettingT<GameSettingCollection>
    gs_sMagicCastInsufficientSkill("sMagicCastInsufficientSkill", "Your Skill level is too low");
SettingT<GameSettingCollection> gs_sMagicCastSilenced("sMagicCastSilenced", "You are currently Silenced");
SettingT<GameSettingCollection> gs_sMagicCastPowerUsed("sMagicCastPowerUsed", "You can only cast Powers once per day");
SettingT<GameSettingCollection>
    gs_sMagicCastMultipleBoundEffects("sMagicCastMultipleBoundEffects", "You already have that kind of Bound Item");
SettingT<GameSettingCollection>
    gs_sMagicCastRangedUnderwater("sMagicCastRangedUnderwater", "You cannot cast ranged spells underwater");
static SettingT<GameSettingCollection> iMagicMaxSummonedCreatureTypes("iMagicMaxSummonedCreatureTypes", 10);

// The effect list's own destructor and TESFullName's run after this.
MagicItem::~MagicItem() {
    RemoveAllEffects();
}

void MagicItem::Preload(bool abNow) {
    EffectSetting* pavEffectSetting = GetAVEffect(MagicSystem::SELF);
    bool brunNow = false;
    bool bloadNow = false;
    if (abNow || IsLoaded()) {
        brunNow = true;
        bloadNow = true;
    }
    else {
        if (IsLoading()) {
            brunNow = true;
            bloadNow = false;
        }
    }
    if (brunNow) {
        EffectItemList* plist_2;
        if (pavEffectSetting) {
            if ((bloadNow) && (pavEffectSetting->GetModelLength())) {
                const char* v2 = pavEffectSetting->GetModel();
                pModelLoader->LoadFile(v2, LOD_MULT_NONE, true, false, false, false);
            }

            pavEffectSetting->IncEffectLoadedCount();
        }
        if (this) {
            plist_2 = static_cast<EffectItemList*>(this);
        }
        else {
            plist_2 = nullptr;
        }
        EffectItemList* plist_3 = plist_2;
        if (!plist_2->IsEmpty()) {
            while (plist_3) {
                EffectItem* pitem = plist_3->m_item;
                EffectSetting* pes = pitem->pEffectSetting;
                if (pes->getArchType()) {
                    if (bloadNow) {
                        TESForm* pform = pes->GetAssociatedItem();
                        if (pform && pes->IsFlagSet(EffectSetting::USE_ASSOCIATED_WEAPON)) {
                            TESObjectWEAP* pweapon = dynamic_cast<TESObjectWEAP*>(pform);
                            if ((pweapon) && (pweapon->GetModelLength())) {
                                const char* v3 = pweapon->GetModel();
                                pModelLoader->LoadFile(v3, LOD_MULT_NONE, true, false, false, false);
                            }
                        }
                        else {
                            if (pform && pes->IsFlagSet(EffectSetting::USE_ASSOCIATED_ARMOR)) {
                                TESObjectARMO* parmor = dynamic_cast<TESObjectARMO*>(pform);
                                if (parmor) {
                                    const char* BipedModel = parmor->GetBipedModel(TESActorBase::SEX_MALE);
                                    pModelLoader->LoadFile(BipedModel, LOD_MULT_NONE, true, false, false, false);
                                    const char* v5 = parmor->GetBipedModel(TESActorBase::SEX_FEMALE);
                                    pModelLoader->LoadFile(v5, LOD_MULT_NONE, true, false, false, false);
                                }
                            }
                            else {
                                if ((pform) && (pes->IsFlagSet(EffectSetting::USE_ASSOCIATED_CREATURE))) {
                                    TESNPC* pnpc = dynamic_cast<TESNPC*>(pform);
                                    if (pnpc) {
                                        BSSimpleList<const char*>* pmodelList = pnpc->BuildDefaultModelList(true, true);
                                        for (BSSimpleList<const char*>* plist_4 = pmodelList;
                                             plist_4 && !plist_4->IsEmpty(); plist_4 = plist_4->GetNext()) {
                                            const char* pfilename = plist_4->m_item;
                                            pModelLoader->LoadFile(pfilename, LOD_MULT_NONE, true, false, false, false);
                                        }
                                        pmodelList->RemoveAll();
                                        if (pmodelList) {
                                            delete pmodelList;
                                        }
                                    }
                                    TESCreature* pcreature = dynamic_cast<TESCreature*>(pform);
                                    if (pcreature) {
                                        char Dst[264];
                                        const char* v6 = pcreature->GetModel();
                                        pModelLoader->LoadFile(v6, LOD_MULT_NONE, true, false, false, false);
                                        const char* pfilename_2 = pcreature->GetModel();
                                        BSSimpleList<const char*>* pcreatureFileList = &pcreature->nifFilenames;
                                        BSstrcpy(Dst, 260, pfilename_2);
                                        char* plastSlash = std::strrchr(Dst, 92);
                                        for (BSSimpleList<const char*>* pfilelist = pcreatureFileList;
                                             plastSlash && pfilelist; pfilelist = pfilelist->GetNext()) {
                                            plastSlash[1] = 0;
                                            const char* pitem_2 = pfilelist->m_item;
                                            if (pitem_2) {
                                                BSstrcat(Dst, 260, pitem_2);
                                                pModelLoader->LoadFile(Dst, LOD_MULT_NONE, true, false, false, false);
                                            }
                                        }
                                        const char* plastBackSlash = std::strrchr(pfilename_2, 92);
                                        if ((plastBackSlash) && (!_strnicmp(plastBackSlash + 1, "Skeleton", 8u))) {
                                            BSSimpleList<const char*>* pFilenameList =
                                                pModelLoader->BuildKFFileList(pfilename_2, false, false, AGW_NONE);
                                            for (BSSimpleList<const char*>* pnode = pFilenameList; pnode;
                                                 pnode = pnode->GetNext()) {
                                                const char* pitem_3 = pnode->m_item;
                                                if (pitem_3) {
                                                    pModelLoader->LoadKF(pitem_3);
                                                }
                                                // The list holds the heap copies BuildKFFileList made, typed
                                                // const char*; the item is freed here.
                                                ourFree(const_cast<char*>(pitem_3));
                                            }
                                            pFilenameList->RemoveAll();
                                            if (pFilenameList) {
                                                delete pFilenameList;
                                            }
                                        }

                                        if (pcreature->HasKFFiles()) {
                                            plastSlash[1] = 0;
                                            BSstrcat(Dst, 260, "SpecialAnims");
                                            BSstrcat(Dst, 260, "\\");
                                            plastSlash = std::strrchr(Dst, 92);
                                            for (char* pitem_4 : pcreature->kfFilenames) {
                                                plastSlash[1] = 0;
                                                if (pitem_4) {
                                                    BSstrcat(Dst, 260, pitem_4);
                                                    pModelLoader->LoadKF(Dst);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    if (!pes->IsAssociatedItemLoaded() && !pes->IsAssociatedItemQueued() &&
                        pes->IsFlagSet(EffectSetting::USE_ASSOCIATED_CREATURE)) {
                        MagicItem::IncSummonedCreatureLoadedCount();
                    }
                    pes->IncAssociatedItemLoadedCount();
                }
                BSSimpleList<EffectItem*>* pNext = plist_3->m_pkNext;
                plist_3 = pNext ? static_cast<EffectItemList*>(pNext) : nullptr;
            }
        }
    }
    else {
        pModelLoader->QueueMagicItem(this, pavEffectSetting, IO_TASK_PRIORITY_CRITICAL);
    }
}

void MagicItem::Unload(bool abRelease) {
    if (IsLoadingOrLoaded()) {
        EffectItemList* plist_2;
        EffectSetting* pavEffectSetting = GetAVEffect(MagicSystem::SELF);
        bool binProgess = IsLoading();
        if ((pavEffectSetting) && (pavEffectSetting->GetModelLength())) {
            if (!binProgess) {
                const char* v2 = pavEffectSetting->GetModel();
                pModelLoader->ReleaseModel(v2, abRelease, 1u);
            }
            pavEffectSetting->DecEffectLoadedCount();
        }

        if (this) {
            plist_2 = static_cast<EffectItemList*>(this);
        }
        else {
            plist_2 = nullptr;
        }
        EffectItemList* plist_3 = plist_2;
        if (!plist_2->IsEmpty()) {
            while (plist_3) {
                EffectItem* pitem = plist_3->m_item;
                EffectSetting* pes = pitem->pEffectSetting;
                if (pes->getArchType()) {
                    if (!binProgess) {
                        TESForm* pform = pes->GetAssociatedItem();
                        if (pform && pes->IsFlagSet(EffectSetting::USE_ASSOCIATED_WEAPON)) {
                            TESObjectWEAP* pweapon = dynamic_cast<TESObjectWEAP*>(pform);
                            if ((pweapon) && (pweapon->GetModelLength())) {
                                const char* v3 = pweapon->GetModel();
                                pModelLoader->ReleaseModel(v3, abRelease, 1u);
                            }
                        }
                        else {
                            if (pform && pes->IsFlagSet(EffectSetting::USE_ASSOCIATED_ARMOR)) {
                                TESObjectARMO* parmor = dynamic_cast<TESObjectARMO*>(pform);
                                if (parmor) {
                                    const char* BipedModel = parmor->GetBipedModel(TESActorBase::SEX_MALE);
                                    pModelLoader->ReleaseModel(BipedModel, abRelease, 1u);
                                    const char* v5 = parmor->GetBipedModel(TESActorBase::SEX_FEMALE);
                                    pModelLoader->ReleaseModel(v5, abRelease, 1u);
                                }
                            }
                            else {
                                if ((pform) && (pes->IsFlagSet(EffectSetting::USE_ASSOCIATED_CREATURE))) {
                                    TESNPC* pnpc = dynamic_cast<TESNPC*>(pform);
                                    if (pnpc) {
                                        BSSimpleList<const char*>* pmodelList = pnpc->BuildDefaultModelList(true, true);
                                        for (BSSimpleList<const char*>* plist_4 = pmodelList;
                                             plist_4 && !plist_4->IsEmpty(); plist_4 = plist_4->GetNext()) {
                                            const char* pfilename = plist_4->m_item;
                                            pModelLoader->ReleaseModel(pfilename, abRelease, 1u);
                                        }
                                        pmodelList->RemoveAll();
                                        if (pmodelList) {
                                            delete pmodelList;
                                        }
                                    }
                                    TESCreature* pcreature = dynamic_cast<TESCreature*>(pform);
                                    if (pcreature) {
                                        char Dst[268];
                                        const char* v6 = pcreature->GetModel();
                                        pModelLoader->ReleaseModel(v6, abRelease, 1u);
                                        const char* pfilename_2 = pcreature->GetModel();
                                        BSSimpleList<const char*>* pcreatureFileList = &pcreature->nifFilenames;
                                        BSstrcpy(Dst, 260, pfilename_2);
                                        char* plastSlash = std::strrchr(Dst, 92);
                                        for (BSSimpleList<const char*>* pfilelist = pcreatureFileList;
                                             plastSlash && pfilelist; pfilelist = pfilelist->GetNext()) {
                                            plastSlash[1] = 0;
                                            const char* pitem_2 = pfilelist->m_item;
                                            if (pitem_2) {
                                                BSstrcat(Dst, 260, pitem_2);
                                                pModelLoader->ReleaseModel(Dst, abRelease, 1u);
                                            }
                                        }
                                        const char* plastBackSlash = std::strrchr(pfilename_2, 92);
                                        if ((plastBackSlash) && (!_strnicmp(plastBackSlash + 1, "Skeleton", 8u))) {
                                            BSSimpleList<const char*>* pFilenameList =
                                                pModelLoader->BuildKFFileList(pfilename_2, false, false, AGW_NONE);
                                            for (BSSimpleList<const char*>* pnode = pFilenameList; pnode;
                                                 pnode = pnode->GetNext()) {
                                                const char* pitem_3 = pnode->m_item;
                                                if (pitem_3) {
                                                    pModelLoader->ReleaseKF(pitem_3, abRelease);
                                                }
                                                // The list holds the heap copies BuildKFFileList made, typed
                                                // const char*; the item is freed here.
                                                ourFree(const_cast<char*>(pitem_3));
                                            }
                                            pFilenameList->RemoveAll();
                                            if (pFilenameList) {
                                                delete pFilenameList;
                                            }
                                        }

                                        if (pcreature->HasKFFiles()) {
                                            plastSlash[1] = 0;
                                            BSstrcat(Dst, 260, "SpecialAnims");
                                            BSstrcat(Dst, 260, "\\");
                                            plastSlash = std::strrchr(Dst, 92);
                                            for (char* pitem_4 : pcreature->kfFilenames) {
                                                plastSlash[1] = 0;
                                                if (pitem_4) {
                                                    BSstrcat(Dst, 260, pitem_4);
                                                    pModelLoader->ReleaseKF(Dst, abRelease);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    pes->DecAssociatedItemLoadedCount();
                    if (!pes->IsAssociatedItemLoaded() && !pes->IsAssociatedItemQueued() &&
                        pes->IsFlagSet(EffectSetting::USE_ASSOCIATED_CREATURE)) {
                        MagicItem::DecSummonedCreatureLoadedCount();
                    }
                }
                BSSimpleList<EffectItem*>* pNext = plist_3->m_pkNext;
                plist_3 = pNext ? static_cast<EffectItemList*>(pNext) : nullptr;
            }
        }
    }
}

bool MagicItem::IsLoaded() {
    EffectItemList* plist_2;
    EffectSetting* pavEffectSetting = GetAVEffect(MagicSystem::SELF);
    if (pavEffectSetting && (pavEffectSetting->GetModelLength() && !pavEffectSetting->IsEffectLoaded())) {
        return false;
    }
    if (this) {
        plist_2 = static_cast<EffectItemList*>(this);
    }
    else {
        plist_2 = nullptr;
    }
    EffectItemList* plist_3 = plist_2;
    if (!plist_2->IsEmpty()) {
        while (plist_3) {
            EffectSetting* pes;
            EffectItem* pitem = plist_3->m_item;
            if (pitem) {
                pes = pitem->pEffectSetting;
            }
            else {
                pes = nullptr;
            }
            if (pes && (pes->getArchType() && !pes->IsAssociatedItemLoaded())) {
                return false;
            }
            BSSimpleList<EffectItem*>* pNext = plist_3->m_pkNext;
            plist_3 = pNext ? static_cast<EffectItemList*>(pNext) : nullptr;
        }
    }
    return true;
}

bool MagicItem::IsLoading() {
    EffectItemList* plist_2;
    EffectSetting* pavEffectSetting = GetAVEffect(MagicSystem::SELF);
    bool bfinished = true;
    if ((pavEffectSetting) && (pavEffectSetting->GetModelLength()) && (!pavEffectSetting->IsEffectLoaded())) {
        bfinished = false;
        if (!pavEffectSetting->IsEffectQueued()) {
            return false;
        }
    }

    if (this) {
        plist_2 = static_cast<EffectItemList*>(this);
    }
    else {
        plist_2 = nullptr;
    }
    EffectItemList* plist_3 = plist_2;
    if (!plist_2->IsEmpty()) {
        while (plist_3) {
            EffectSetting* pes;
            EffectItem* pitem = plist_3->m_item;
            if (pitem) {
                pes = pitem->pEffectSetting;
            }
            else {
                pes = nullptr;
            }
            if ((pes) && (pes->getArchType() && !pes->IsAssociatedItemLoaded())) {
                bfinished = false;
                if (!pes->IsAssociatedItemQueued()) {
                    return false;
                }
            }

            BSSimpleList<EffectItem*>* pNext = plist_3->m_pkNext;
            plist_3 = pNext ? static_cast<EffectItemList*>(pNext) : nullptr;
        }
    }
    return !bfinished;
}

void MagicItem::FinishedLoading() {
    EffectSetting* pavEffectSetting = GetAVEffect(MagicSystem::SELF);
    if (IsLoading()) {
        EffectItemList* plist_2;
        if (pavEffectSetting) {
            int32_t icount = 0;
            if (pavEffectSetting->IsEffectQueued()) {
                icount = pavEffectSetting->QueueEffectFinished();
            }
            if (pavEffectSetting->GetModelLength()) {
                NiPointer<Model> spmodel;
                const char* v1 = pavEffectSetting->GetModel();
                pModelLoader->FindModel(v1, spmodel);
                if (spmodel.m_pObject) {
                    spmodel.m_pObject->ModManualRefCount(icount);
                }
            }
        }
        if (this) {
            plist_2 = static_cast<EffectItemList*>(this);
        }
        else {
            plist_2 = nullptr;
        }
        EffectItemList* plist_3 = plist_2;
        if (!plist_2->IsEmpty()) {
            while (plist_3) {
                EffectItem* pitem = plist_3->m_item;
                EffectSetting* pes = pitem->pEffectSetting;
                TESForm* pform = pes->GetAssociatedItem();
                int32_t icount_2 = 0;
                if (pes->IsAssociatedItemQueued()) {
                    icount_2 = pes->QueueAssociatedItemFinished();
                }
                if (icount_2) {
                    if (pform && pes->IsFlagSet(EffectSetting::USE_ASSOCIATED_WEAPON)) {
                        TESObjectWEAP* pweapon = dynamic_cast<TESObjectWEAP*>(pform);
                        if ((pweapon) && (pweapon->GetModelLength())) {
                            NiPointer<Model> spmodel_2;
                            const char* v2 = pweapon->GetModel();
                            pModelLoader->FindModel(v2, spmodel_2);
                            if (spmodel_2.m_pObject) {
                                spmodel_2.m_pObject->ModManualRefCount(icount_2);
                            }
                        }
                    }
                    else {
                        if (pform && pes->IsFlagSet(EffectSetting::USE_ASSOCIATED_ARMOR)) {
                            TESObjectARMO* parmor = dynamic_cast<TESObjectARMO*>(pform);
                            if (parmor) {
                                NiPointer<Model> spmodel_3;
                                const char* BipedModel = parmor->GetBipedModel(TESActorBase::SEX_MALE);
                                pModelLoader->FindModel(BipedModel, spmodel_3);
                                if (spmodel_3.m_pObject) {
                                    spmodel_3.m_pObject->ModManualRefCount(icount_2);
                                }
                                const char* v4 = parmor->GetBipedModel(TESActorBase::SEX_FEMALE);
                                pModelLoader->FindModel(v4, spmodel_3);
                                if (spmodel_3.m_pObject) {
                                    spmodel_3.m_pObject->ModManualRefCount(icount_2);
                                }
                            }
                        }
                        else {
                            if ((pform) && (pes->IsFlagSet(EffectSetting::USE_ASSOCIATED_CREATURE))) {
                                TESNPC* pnpc = dynamic_cast<TESNPC*>(pform);
                                if (pnpc) {
                                    BSSimpleList<const char*>* pmodelList = pnpc->BuildDefaultModelList(true, true);
                                    BSSimpleList<const char*>* plist_4 = pmodelList;
                                    while (plist_4 && !plist_4->IsEmpty()) {
                                        const char* pfilename = plist_4->m_item;
                                        NiPointer<Model> spmodel_4;
                                        pModelLoader->FindModel(pfilename, spmodel_4);
                                        if (spmodel_4.m_pObject) {
                                            spmodel_4.m_pObject->ModManualRefCount(icount_2);
                                        }
                                        plist_4 = plist_4->GetNext();
                                    }
                                    pmodelList->RemoveAll();
                                    if (pmodelList) {
                                        delete pmodelList;
                                    }
                                }
                                TESCreature* pcreature = dynamic_cast<TESCreature*>(pform);
                                if (pcreature) {
                                    NiPointer<Model> spmodel_5;
                                    const char* v5 = pcreature->GetModel();
                                    pModelLoader->FindModel(v5, spmodel_5);
                                    if (spmodel_5.m_pObject) {
                                        char Dst[268];
                                        for (int32_t i = 0; i < icount_2; ++i) {
                                            spmodel_5.m_pObject->IncManualRefCount();
                                        }
                                        const char* pfilename_2 = spmodel_5.m_pObject->pFilename;
                                        BSSimpleList<const char*>* pcreatureFileList = &pcreature->nifFilenames;
                                        BSstrcpy(Dst, 260, pfilename_2);
                                        char* plastSlash = std::strrchr(Dst, 92);
                                        for (BSSimpleList<const char*>* pfilelist = pcreatureFileList;
                                             plastSlash && pfilelist; pfilelist = pfilelist->GetNext()) {
                                            plastSlash[1] = 0;
                                            const char* pitem_2 = pfilelist->m_item;
                                            if (pitem_2) {
                                                BSstrcat(Dst, 260, pitem_2);
                                                NiPointer<Model> spmodel_6;
                                                pModelLoader->FindModel(Dst, spmodel_6);
                                                if (spmodel_6.m_pObject) {
                                                    spmodel_6.m_pObject->ModManualRefCount(icount_2);
                                                }
                                            }
                                        }
                                        const char* plastBackSlash = std::strrchr(pfilename_2, 92);
                                        if ((plastBackSlash) && (!_strnicmp(plastBackSlash + 1, "Skeleton", 8u))) {
                                            BSSimpleList<const char*>* pFilenameList =
                                                pModelLoader->BuildKFFileList(pfilename_2, false, false, AGW_NONE);
                                            for (BSSimpleList<const char*>* pnode = pFilenameList; pnode;
                                                 pnode = pnode->GetNext()) {
                                                const char* pitem_3 = pnode->m_item;
                                                if (pitem_3) {
                                                    for (int32_t i_2 = 0; i_2 < icount_2; ++i_2) {
                                                        pModelLoader->LoadKF(pitem_3);
                                                    }
                                                }
                                                // The list holds the heap copies BuildKFFileList made, typed
                                                // const char*; the item is freed here.
                                                ourFree(const_cast<char*>(pitem_3));
                                            }
                                            pFilenameList->RemoveAll();
                                            if (pFilenameList) {
                                                delete pFilenameList;
                                            }
                                        }

                                        if (pcreature->HasKFFiles()) {
                                            plastSlash[1] = 0;
                                            BSstrcat(Dst, 260, "SpecialAnims");
                                            BSstrcat(Dst, 260, "\\");
                                            plastSlash = std::strrchr(Dst, 92);
                                            for (char* pitem_4 : pcreature->kfFilenames) {
                                                plastSlash[1] = 0;
                                                if (pitem_4) {
                                                    BSstrcat(Dst, 260, pitem_4);
                                                    for (int32_t i_3 = 0; i_3 < icount_2; ++i_3) {
                                                        pModelLoader->LoadKF(Dst);
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
                BSSimpleList<EffectItem*>* pNext = plist_3->m_pkNext;
                plist_3 = pNext ? static_cast<EffectItemList*>(pNext) : nullptr;
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
        return (uint32_t)((int32_t)pForm->TESForm::GetFormID());
    }
    if (pObject) {
        return (uint32_t)((int32_t)pObject->TESForm::GetFormID());
    }
    return (uint32_t)0;
}

EffectSetting* MagicItem::GetAVEffect(MagicSystem::Range aeRange) {
    EffectItemList* plist_2;
    EffectItem* pavEffect = nullptr;
    if (this) {
        plist_2 = static_cast<EffectItemList*>(this);
    }
    else {
        plist_2 = nullptr;
    }
    EffectItemList* plist_3 = plist_2;
    if (plist_2->IsEmpty()) {
        return nullptr;
    }
    int32_t imaxCost = -1;
    while (plist_3) {
        EffectItem* pitem = plist_3->m_item;
        int32_t ithisCost = static_cast<int32_t>(pitem->GetCost(nullptr));
        MagicSystem::Range ethisRange = pitem->data.iRange;
        if (!pitem->IsDisabled() && (ethisRange > aeRange || (ethisRange == aeRange && ithisCost > imaxCost))) {
            pavEffect = pitem;
            imaxCost = ithisCost;
            aeRange = pitem->data.iRange;
        }
        BSSimpleList<EffectItem*>* pNext = plist_3->m_pkNext;
        plist_3 = pNext ? static_cast<EffectItemList*>(pNext) : nullptr;
    }
    if (pavEffect) {
        return pavEffect->pEffectSetting;
    }

    return nullptr;
}

BSStringT<char> MagicItem::GetCannotCastString(CannotCastReason aeReason) {
    switch (aeReason) {
        case 1: return gs_sMagicCastInsufficientMagicka.String();
        case 2: return gs_sMagicCastSilenced.String();
        case 3: return gs_sMagicCastInsufficientSkill.String();
        case 4: return gs_sMagicCastPowerUsed.String();
        case 5: return gs_sMagicCastMultipleBoundEffects.String();
        case 6: return gs_sMagicCastRangedUnderwater.String();
        default: return gs_sMagicCastOKText.String();
    }
}

void MagicItem::MagicItemInitItem(TESForm* apForm) {
    EffectItemList* plist_2;
    if (this) {
        plist_2 = static_cast<EffectItemList*>(this);
    }
    else {
        plist_2 = nullptr;
    }
    EffectItemList* plist_3 = plist_2;
    iHostileCount = 0;
    while (plist_3) {
        EffectItem* pitem = plist_3->m_item;
        if (pitem) {
            pitem->InitItem(apForm);
            if (pitem->IsHostile()) {
                ++iHostileCount;
            }
        }
        BSSimpleList<EffectItem*>* pNext = plist_3->m_pkNext;
        plist_3 = pNext ? static_cast<EffectItemList*>(pNext) : nullptr;
    }
}

bool MagicItem::IsLoadingOrLoaded() {
    EffectItemList* plist_2;
    EffectSetting* pavEffectSetting = GetAVEffect(MagicSystem::SELF);
    if (pavEffectSetting &&
        (pavEffectSetting->GetModelLength() &&
         (!pavEffectSetting->IsEffectLoaded() && !pavEffectSetting->IsEffectQueued()))) {
        return false;
    }
    if (this) {
        plist_2 = static_cast<EffectItemList*>(this);
    }
    else {
        plist_2 = nullptr;
    }
    EffectItemList* plist_3 = plist_2;
    if (!plist_2->IsEmpty()) {
        while (plist_3) {
            EffectSetting* pes;
            EffectItem* pitem = plist_3->m_item;
            if (pitem) {
                pes = pitem->pEffectSetting;
            }
            else {
                pes = nullptr;
            }
            if (pes && (pes->getArchType() && (!pes->IsAssociatedItemLoaded() && !pes->IsAssociatedItemQueued()))) {
                return false;
            }
            BSSimpleList<EffectItem*>* pNext = plist_3->m_pkNext;
            plist_3 = pNext ? static_cast<EffectItemList*>(pNext) : nullptr;
        }
    }
    return true;
}

void MagicItem::DecSummonedCreatureLoadedCount() {
    --MagicItem::iSummonedCreatureLoadedCount;
}

MagicItem* MagicItem::GetMagicItemByNumericID(uint32_t aiFormID) {
    TESForm* formByNumericID = TESForm::GetFormByNumericID(aiFormID);
    if (!formByNumericID) {
        return nullptr;
    }
    MagicItemForm* v2 = dynamic_cast<MagicItemForm*>(formByNumericID);
    MagicItemObject* v3 = dynamic_cast<MagicItemObject*>(formByNumericID);
    if (v2) {
        return static_cast<MagicItem*>(v2);
    }
    if (v3) {
        return static_cast<MagicItem*>(v3);
    }

    return nullptr;
}

int32_t MagicItem::IncSummonedCreatureLoadedCount() {
    return ++MagicItem::iSummonedCreatureLoadedCount;
}

EffectItemList::~EffectItemList() {}
