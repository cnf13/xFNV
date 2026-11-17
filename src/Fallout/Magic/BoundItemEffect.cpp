#include "Fallout/Magic/BoundItemEffect.h"

#include "BSAudio/BSGameSound.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/IOManager.h"
#include "BSMain/QueuedFiles.h"
#include "BSMain/Setting.h"
#include "BSShader/BSFadeNode.h"
#include "Fallout Shared/BGSBipedModelList.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/EffectItemList.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESImageSpaceModifier.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectARMO.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Character.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/AssociatedItemEffect.h"
#include "Fallout/Magic/CureEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicModelHitEffect.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/rtti.h"

static char bBoundItemEffectInitialized =
    EffectArchetypes::RegisterActiveEffect(EffectArchetypes::BOUND_ITEM, &BoundItemEffect::Instantiate);

BoundItemEffect::BoundItemEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect)
  : AssociatedItemEffect(apCaster, apSpell, apEffect), pDisplacedWeapon(nullptr), pDisplacedItems{}, fAppearTime(0.0f),
    bItemSummoned(false), bSpellModelCached(false), bWeaponCached(false), bWornCached(false), bSetupItem(false) {}

BoundItemEffect::~BoundItemEffect() {
    if (pDisplacedWeapon) {
        if (bWeaponCached) {
            TESObjectWEAP* pweapon = dynamic_cast<TESObjectWEAP*>(pDisplacedWeapon->pContainerObj);
            if (pweapon) {
                if (pweapon->GetModelLength()) {
                    const char* v1 = pweapon->GetModel();
                    pModelLoader->ReleaseModel(v1, 0, 1u);
                }
            }
        }
    }
    if (bWornCached) {
        TESActorBase::SEX esex = TESActorBase::SEX_MALE;
        if (pTarget) {
            if (pTarget->GetTargetStatsObject()) {
                TESObjectREFR* v2 = pTarget->GetTargetStatsObject();
                TESNPC* pnpc = dynamic_cast<TESNPC*>(v2->GetObjectReference());
                if (pnpc) {
                    esex = pnpc->GetSex();
                }
            }
        }
        for (int32_t i = 0; i < 20; ++i) {
            if (pDisplacedItems[i]) {
                if (pDisplacedItems[i]->pContainerObj) {
                    TESBipedModelForm* pbipModel = dynamic_cast<TESBipedModelForm*>(pDisplacedItems[i]->pContainerObj);
                    if (pbipModel) {
                        bWornCached = 1;
                        const char* BipedModel = pbipModel->GetBipedModel(esex);
                        pModelLoader->ReleaseModel(BipedModel, 0, 1u);
                    }
                }
            }
        }
    }
    if (pDisplacedWeapon) {
        ExtraDataList* pextra;
        if (pDisplacedWeapon->pExtraObjectList) {
            pextra = pDisplacedWeapon->pExtraObjectList->m_item;
        }
        else {
            pextra = 0;
        }
        if (pextra) {
            pextra->ClearScriptLocals();
        }
        pDisplacedWeapon->DeleteAllExtra();
        delete pDisplacedWeapon;
    }
    for (BIPED_OBJECT ebip = BO_HEAD; ebip < BIPED_OBJECT_COUNT; ebip = static_cast<BIPED_OBJECT>(ebip + 1)) {
        if (pDisplacedItems[ebip]) {
            ExtraDataList* pextra_2;
            if (pDisplacedItems[ebip]->pExtraObjectList) {
                pextra_2 = pDisplacedItems[ebip]->pExtraObjectList->m_item;
            }
            else {
                pextra_2 = 0;
            }
            if (pextra_2) {
                pextra_2->ClearScriptLocals();
            }
            pDisplacedItems[ebip]->DeleteAllExtra();
            delete pDisplacedItems[ebip];
        }
    }
    if (bSpellModelCached) {
        if (pSpell) {
            if (pEffect) {
                EffectSetting* pavEffect = pSpell->GetAVEffect(MagicSystem::SELF);
                MagicSystem::Range CastingType = pEffect->data.iRange;
                EffectItem* pdominantEffect = pSpell->GetCostliestEffect(CastingType, 0);
                if (pavEffect->GetModelLength()) {
                    if (pEffect == pdominantEffect) {
                        const char* v5 = pavEffect->GetModel();
                        pModelLoader->ReleaseModel(v5, 0, 1u);
                    }
                }
                bSpellModelCached = 0;
            }
        }
    }
}

ActiveEffect* BoundItemEffect::Copy() {
    BoundItemEffect* presult;
    BoundItemEffect* apMemory = static_cast<BoundItemEffect*>(ourMalloc(sizeof(BoundItemEffect)));
    if (apMemory) {
        presult = ::new (apMemory) BoundItemEffect(pCaster, pSpell, pEffect);
    }
    else {
        presult = 0;
    }
    AssociatedItemEffect::CopyData(presult);
    if (pDisplacedWeapon) {
        ItemChange* v8;
        ItemChange* v12 = static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
        if (v12) {
            v8 = ::new (v12) ItemChange(pDisplacedWeapon);
        }
        else {
            v8 = 0;
        }
        presult->pDisplacedWeapon = v8;
    }
    for (BIPED_OBJECT ebip = BO_HEAD; ebip < BIPED_OBJECT_COUNT; ebip = static_cast<BIPED_OBJECT>(ebip + 1)) {
        if (pDisplacedItems[ebip]) {
            ItemChange* v7;
            ItemChange* v11 = static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
            if (v11) {
                v7 = ::new (v11) ItemChange(pDisplacedItems[ebip]);
            }
            else {
                v7 = 0;
            }
            presult->pDisplacedItems[ebip] = v7;
        }
    }
    if (pDisplacedWeapon) {
        if (bWeaponCached) {
            TESObjectWEAP* pweapon = dynamic_cast<TESObjectWEAP*>(pDisplacedWeapon->pContainerObj);
            if (pweapon) {
                if (pweapon->GetModelLength()) {
                    presult->bWeaponCached = 1;
                    const char* v1 = pweapon->GetModel();
                    pModelLoader->QueueModel(
                        v1, IO_TASK_PRIORITY_CRITICAL, static_cast<QueuedFile*>(0), LOD_MULT_NONE, static_cast<bool>(1),
                        static_cast<bool>(1), static_cast<bool>(0)
                    );
                }
            }
        }
    }
    if (bWornCached) {
        TESActorBase::SEX esex = TESActorBase::SEX_MALE;
        MagicTarget* Target = pTarget;
        if (Target->GetTargetStatsObject()) {
            MagicTarget* v5 = pTarget;
            TESObjectREFR* v2 = v5->GetTargetStatsObject();
            TESNPC* pnpc = dynamic_cast<TESNPC*>(v2->GetObjectReference());
            if (pnpc) {
                esex = pnpc->GetSex();
            }
        }
        for (int32_t i = 0; i < 20; ++i) {
            if (pDisplacedItems[i]) {
                if (pDisplacedItems[i]->pContainerObj) {
                    TESBipedModelForm* pbipModel = dynamic_cast<TESBipedModelForm*>(pDisplacedItems[i]->pContainerObj);
                    if (pbipModel) {
                        presult->bWornCached = 1;
                        const char* BipedModel = pbipModel->GetBipedModel(esex);
                        pModelLoader->QueueModel(
                            BipedModel, IO_TASK_PRIORITY_CRITICAL, static_cast<QueuedFile*>(0), LOD_MULT_NONE,
                            static_cast<bool>(1), static_cast<bool>(1), static_cast<bool>(0)
                        );
                    }
                }
            }
        }
    }
    presult->bItemSummoned = bItemSummoned;
    return presult;
}

void BoundItemEffect::Update(float a1) {
    if (bItemSummoned) {
        Actor* pactor;
        if (pTarget) {
            pactor = pTarget->GetTargetAsActor();
        }
        else {
            pactor = 0;
        }
        TESObjectWEAP* pboundWeapon = dynamic_cast<TESObjectWEAP*>(pAssociatedItem);
        if (pactor && (pactor->pCurrentProcess && pboundWeapon)) {
            BaseProcess* CurrentAIProcess = pactor->pCurrentProcess;
            CurrentAIProcess->SetLastBoundWeapon(pboundWeapon);
            if (bSetupItem) {
                BaseProcess* v5 = pactor->pCurrentProcess;
                if (v5->GetCurrentWeapon()) {
                    BaseProcess* v4 = pactor->pCurrentProcess;
                    ItemChange* pitem = v4->GetCurrentWeapon();
                    if (pboundWeapon == pitem->pContainerObj) {
                        if (pitem->pExtraObjectList) {
                            bSetupItem = 0;
                            ExtraDataList** v2 = &pitem->pExtraObjectList->m_item;
                            (*v2)->SetCanNotWear(1);
                        }
                    }
                }
            }
            if (pactor == pPlayer) {
                BaseProcess* v3 = pactor->pCurrentProcess;
                if (!v3->GetWeaponDrawn() && !pactor->GetWantWeaponDrawn()) {
                    Dispel(0);
                }
            }
        }
        else {
            if (bSetupItem) {
                TESObjectARMO* pboundArmor = dynamic_cast<TESObjectARMO*>(pAssociatedItem);
                if (pboundArmor) {
                    InventoryChanges* pinventory = InventoryChanges::GetInventoryChanges(pactor);
                    ExtraDataList* pextra_2 = pinventory->WearingObject(pboundArmor, 1);
                    if (pextra_2) {
                        bSetupItem = 0;
                        pextra_2->SetCanNotWear(1);
                    }
                }
            }
        }
    }
    else {
        if (fAppearTime <= static_cast<double>(fElapsedSeconds)) {
            DoSummon();
            bItemSummoned = 1;
        }
    }
}

uint16_t BoundItemEffect::GetSaveSize(Actor* a1) {
    uint16_t SaveSize = AssociatedItemEffect::GetSaveSize(a1);
    uint16_t v6 = SaveSize;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        SaveSize += 6;
    }
    uint16_t isize = SaveSize + 1;
    if (pDisplacedWeapon) {
        isize += pDisplacedWeapon->GetSaveSize();
    }
    for (int32_t i = 0; i < 20; ++i) {
        ++isize;
        if (pDisplacedItems[i]) {
            isize += pDisplacedItems[i]->GetSaveSize();
        }
    }
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(pheader->iFormID);
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s", isize - v6,
                pheader->iFormID, FormByNumericID->GetFormEditorID(), pheader->iFlags, 667,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\BoundItemEffect.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s", isize - v6, 667,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\BoundItemEffect.cpp"
            );
        }
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 65u) {
        isize += 5;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 107u) {
        ++isize;
    }
    return isize;
}

void BoundItemEffect::SaveGame(Actor* a1) {
    uint32_t iblockHeader;
    uint16_t iblockSize;
    bool bdisplaced;
    AssociatedItemEffect::SaveGame(a1);
    iblockSize = 0;
    uint16_t* pblockSize = 0;
    uint8_t* itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    if (bDebugSaveBuffer.Bool()) {
        itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        iblockHeader = 0x424C4F4B;
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockHeader, 4u);
        pblockSize = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockSize, 2u);
    }
    bdisplaced = pDisplacedWeapon != 0;
    pSaveLoadGameOLD->SaveGameDataOLD(&bdisplaced, 1u);
    if (bdisplaced) {
        pDisplacedWeapon->SaveGameTES();
    }
    for (int32_t i = 0; i < 20; ++i) {
        bdisplaced = pDisplacedItems[i] != 0;
        pSaveLoadGameOLD->SaveGameDataOLD(&bdisplaced, 1u);
        if (bdisplaced) {
            pDisplacedItems[i]->SaveGameTES();
        }
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 65u) {
        pSaveLoadGameOLD->SaveGameDataOLD(&fAppearTime, 4u);
        pSaveLoadGameOLD->SaveGameDataOLD(&bItemSummoned, 1u);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 107u) {
        pSaveLoadGameOLD->SaveGameDataOLD(&bSetupItem, 1u);
    }
    if (bDebugSaveBuffer.Bool()) {
        uint8_t* BufferPointer = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            uint32_t iFlags = pheader->iFlags;
            const char* v2 = pform->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                static_cast<int32_t>(BufferPointer - itempStartPosition), pheader->iFormID, v2, iFlags, 721,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\BoundItemEffect.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s",
                static_cast<int32_t>(BufferPointer - itempStartPosition), 721,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\BoundItemEffect.cpp"
            );
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        // The block size counts from the size field itself to the current write position. pblockSize points into the
        // save buffer, which holds untyped raw bytes.
        uint8_t* pcurrentPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        uint8_t* pblockStart = reinterpret_cast<uint8_t*>(pblockSize);
        if (pcurrentPosition > pblockStart + 0xFFFF) {
            Warning(
                "SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\BoundItemEffect.cpp", 721
            );
        }
        *pblockSize = static_cast<uint16_t>(pcurrentPosition - pblockStart);
    }
}

// Loads the displaced weapon and items (each saved behind a presence byte), the appear time and the summoned and set-up
// flags from the old-format save, checking the block header and the bytes read when the save uses blocks.
void BoundItemEffect::LoadGame(Actor* apActor) {
    AssociatedItemEffect::LoadGame(apActor);
    uint16_t usBlockSize = 0;
    // The block start and end are the save buffer's read positions; they are compared unsigned.
    uint8_t* pBlockStart = nullptr;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint32_t uiBlockID;
        pSaveLoadGameOLD->LoadGameDataOLD(&uiBlockID, 4);
        // 'BLOK'
        if (uiBlockID != 0x424C4F4B) {
            LoadFormHeader* pHeader = pSaveLoadGameOLD->GetCurrentlyLoadingFormHeader();
            if (pHeader) {
                TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
                uint32_t uiFlags = pHeader->iFlags;
                uint32_t uiVersion = pHeader->cVersion;
                const char* pEditorID = pForm->GetFormEditorID();
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  "
                    "Currently loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\BoundItemEffect.cpp", 730,
                    pHeader->iFormID, pEditorID, uiVersion, uiFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  "
                    "Current version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\BoundItemEffect.cpp", 730,
                    static_cast<uint32_t>(pSaveLoadGameOLD->GetCurrentVersion())
                );
            }
        }
        pBlockStart = static_cast<uint8_t*>(pSaveLoadGameOLD->GetBufferPointer());
        pSaveLoadGameOLD->LoadGameDataOLD(&usBlockSize, 2);
    }
    bool bPresent;
    pSaveLoadGameOLD->LoadGameDataOLD(&bPresent, 1);
    if (bPresent) {
        void* pMemory = ourMalloc(sizeof(ItemChange));
        ItemChange* pWeapon;
        if (pMemory) {
            pWeapon = ::new (pMemory) ItemChange;
        }
        else {
            pWeapon = nullptr;
        }
        pDisplacedWeapon = pWeapon;
        pDisplacedWeapon->LoadGame();
    }
    for (int32_t i = 0; i < 20; ++i) {
        pSaveLoadGameOLD->LoadGameDataOLD(&bPresent, 1);
        if (bPresent) {
            void* pMemory = ourMalloc(sizeof(ItemChange));
            ItemChange* pItem;
            if (pMemory) {
                pItem = ::new (pMemory) ItemChange;
            }
            else {
                pItem = nullptr;
            }
            pDisplacedItems[i] = pItem;
            pDisplacedItems[i]->LoadGame();
            if (!pDisplacedItems[i]->GetItemChangeObject()) {
                ItemChange* pDelete = pDisplacedItems[i];
                if (pDelete) {
                    delete pDelete;
                }
                pDisplacedItems[i] = nullptr;
            }
        }
    }
    bItemSummoned = true;
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x41) {
        pSaveLoadGameOLD->LoadGameDataOLD(&fAppearTime, 4);
        pSaveLoadGameOLD->LoadGameDataOLD(&bItemSummoned, 1);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x6B) {
        pSaveLoadGameOLD->LoadGameDataOLD(&bSetupItem, 1);
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* pBlockEnd = static_cast<uint8_t*>(pSaveLoadGameOLD->GetBufferPointer());
        LoadFormHeader* pHeader = pSaveLoadGameOLD->GetCurrentlyLoadingFormHeader();
        if (pHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            if (pBlockEnd > &pBlockStart[usBlockSize]) {
                uint32_t uiFlags = pHeader->iFlags;
                uint32_t uiVersion = pHeader->cVersion;
                const char* pEditorID = pForm->GetFormEditorID();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading form "
                    "is %08X %s with version %i and flags %08X",
                    static_cast<int32_t>(pBlockEnd - &pBlockStart[usBlockSize]),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\BoundItemEffect.cpp", 770,
                    pHeader->iFormID, pEditorID, uiVersion, uiFlags
                );
            }
            else if (pBlockEnd < &pBlockStart[usBlockSize]) {
                uint32_t uiFlags = pHeader->iFlags;
                uint32_t uiVersion = pHeader->cVersion;
                const char* pEditorID = pForm->GetFormEditorID();
                Warning(
                    "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading "
                    "form is %08X %s with version %i and flags %08X",
                    static_cast<int32_t>(&pBlockStart[usBlockSize] - pBlockEnd),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\BoundItemEffect.cpp", 770,
                    pHeader->iFormID, pEditorID, uiVersion, uiFlags
                );
            }
        }
        else if (pBlockEnd > &pBlockStart[usBlockSize]) {
            Warning(
                "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                static_cast<int32_t>(pBlockEnd - &pBlockStart[usBlockSize]),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\BoundItemEffect.cpp", 770,
                static_cast<uint32_t>(pSaveLoadGameOLD->GetCurrentVersion())
            );
        }
        else if (pBlockEnd < &pBlockStart[usBlockSize]) {
            Warning(
                "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is %i",
                static_cast<int32_t>(&pBlockStart[usBlockSize] - pBlockEnd),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\BoundItemEffect.cpp", 770,
                static_cast<uint32_t>(pSaveLoadGameOLD->GetCurrentVersion())
            );
        }
    }
}

void BoundItemEffect::InitLoadGame(Actor* a1) {
    Actor* pactor;
    AssociatedItemEffect::InitLoadGame(a1);
    if (pTarget) {
        pactor = pTarget->GetTargetAsActor();
    }
    else {
        pactor = 0;
    }
    if (bItemSummoned) {
        if (pDisplacedWeapon) {
            TESObjectWEAP* pweapon = dynamic_cast<TESObjectWEAP*>(pDisplacedWeapon->pContainerObj);
            if (pweapon) {
                if (pweapon->GetModelLength()) {
                    bWeaponCached = 1;
                    const char* v2 = pweapon->GetModel();
                    pModelLoader->QueueModel(
                        v2, IO_TASK_PRIORITY_CRITICAL, static_cast<QueuedFile*>(0), LOD_MULT_NONE, static_cast<bool>(1),
                        static_cast<bool>(0), static_cast<bool>(0)
                    );
                }
            }
        }
        TESActorBase::SEX esex = TESActorBase::SEX_MALE;
        if (pactor) {
            TESNPC* pnpc = dynamic_cast<TESNPC*>(pactor->GetObjectReference());
            if (pnpc) {
                esex = pnpc->GetSex();
            }
        }
        for (int32_t i = 0; i < 20; ++i) {
            if (pDisplacedItems[i]) {
                if (pDisplacedItems[i]->pContainerObj) {
                    TESBipedModelForm* pbipModel = dynamic_cast<TESBipedModelForm*>(pDisplacedItems[i]->pContainerObj);
                    if (pbipModel) {
                        bWornCached = 1;
                        const char* BipedModel = pbipModel->GetBipedModel(esex);
                        pModelLoader->QueueModel(
                            BipedModel, IO_TASK_PRIORITY_CRITICAL, static_cast<QueuedFile*>(0), LOD_MULT_NONE,
                            static_cast<bool>(1), static_cast<bool>(1), static_cast<bool>(0)
                        );
                    }
                }
            }
        }
    }
    if (pDisplacedWeapon) {
        pDisplacedWeapon->InitLoadGame_0();
        if (pSaveLoadGameOLD->GetCurrentVersion() < 97u) {
            ItemChange* pcopy;
            ItemChange* apMemory = static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
            if (apMemory) {
                pcopy = ::new (apMemory) ItemChange(pDisplacedWeapon);
            }
            else {
                pcopy = 0;
            }
            if (pDisplacedWeapon) {
                delete pDisplacedWeapon;
            }
            pDisplacedWeapon = pcopy;
        }
        pactor->QueueUnEquipObject(pDisplacedWeapon->pContainerObj, 1, 0, 0, 0, 1);
    }
    for (int32_t i_2 = 0; i_2 < 20; ++i_2) {
        if (pDisplacedItems[i_2]) {
            pDisplacedItems[i_2]->InitLoadGame_0();
            if (pSaveLoadGameOLD->GetCurrentVersion() < 97u) {
                ItemChange* pcopy_2;
                ItemChange* v8 = static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
                if (v8) {
                    pcopy_2 = ::new (v8) ItemChange(pDisplacedItems[i_2]);
                }
                else {
                    pcopy_2 = 0;
                }
                if (pDisplacedItems[i_2]) {
                    delete pDisplacedItems[i_2];
                }
                pDisplacedItems[i_2] = pcopy_2;
            }
            pactor->QueueUnEquipObject(pDisplacedItems[i_2]->pContainerObj, 1, 0, 0, 0, 1);
        }
    }
    if (bItemSummoned) {
        InventoryChanges* pinventory = (&pactor->m_Extra)->GetContainerChanges();
        TESBoundObject* pbound = dynamic_cast<TESBoundObject*>(pAssociatedItem);
        if (pinventory) {
            if (!pinventory->GetObjectCount(pbound)) {
                pactor->AddObjecttoContainer(pbound, 0, 1);
                pactor->QueueEquipObject(pbound, 1, 0, 1, 0, 1);
            }
        }
    }
}

bool BoundItemEffect::CheckTarget(MagicTarget* apTarget) {
    return (dynamic_cast<Character*>(apTarget) != 0);
}

void BoundItemEffect::Start() {
    Actor* pactor;
    if (pTarget) {
        pactor = pTarget->GetTargetAsActor();
    }
    else {
        pactor = 0;
    }
    if (pAssociatedItem && pactor) {
        if (pSpell) {
            if (pEffect) {
                EffectSetting* pavEffect = pSpell->GetAVEffect(MagicSystem::SELF);
                MagicSystem::Range CastingType = pEffect->data.iRange;
                EffectItem* pdominantEffect = pSpell->GetCostliestEffect(CastingType, 0);
                if (pavEffect->GetModelLength()) {
                    const char* v2 = pavEffect->GetModel();
                    NiAVObject* peffectModel = pModelLoader->LoadFile(v2, LOD_MULT_NONE, 1, 0, 0, 0);
                    MagicItem* Spell = pSpell;
                    if (Spell->GetSpellType() == MagicSystem::ABILITY) {
                        fAppearTime = 0.0f;
                    }
                    else {
                        if (peffectModel) {
                            fAppearTime = FindTimeForNoteKey(peffectModel, "SpecialIdle_HitEffect", "Hit");
                        }
                    }
                    if (pEffect != pdominantEffect) {
                        const char* v3 = pavEffect->GetModel();
                        pModelLoader->ReleaseModel(v3, 0, 1u);
                    }
                }
            }
        }
    }
    else {
        Dispel(0);
    }
}

void BoundItemEffect::Finish() {
    Actor* pactor;
    FNVD_INLINED_LOCAL(BSSoundHandle, dst, std::nullopt, std::nullopt);
    if (pTarget) {
        pactor = pTarget->GetTargetAsActor();
    }
    else {
        pactor = 0;
    }
    if (bItemSummoned) {
        if (pactor) {
            TESObjectWEAP* pboundWeapon = dynamic_cast<TESObjectWEAP*>(pAssociatedItem);
            TESObjectARMO* pboundArmor = dynamic_cast<TESObjectARMO*>(pAssociatedItem);
            if (pboundWeapon) {
                InventoryChanges* pchanges = InventoryChanges::GetInventoryChanges(pactor);
                ExtraDataList* pextra_2 = pchanges->WearingObject(pboundWeapon, 0);
                if (pextra_2) {
                    pextra_2->RemoveCannotWearExtra();
                }
                pactor->QueueUnEquipObject(pboundWeapon, 1, 0, 0, 0, 1);
                pactor->RemoveItem(pboundWeapon, 0, 1, 0, 0, 0, 0, 0, 1, 0);
                if (this->pDisplacedWeapon) {
                    ExtraDataList* pextra;
                    bool bsuccess = 0;
                    if (this->pDisplacedWeapon->pExtraObjectList) {
                        pextra = this->pDisplacedWeapon->pExtraObjectList->m_item;
                    }
                    else {
                        pextra = 0;
                    }
                    if (pactor->GetInventoryObjectCount(this->pDisplacedWeapon->pContainerObj) >= 1) {
                        bsuccess = 1;
                        pactor->QueueEquipObject(this->pDisplacedWeapon->pContainerObj, 1, 0, 1, 0, 1);
                    }
                    if (bWeaponCached) {
                        TESBoundObject* pobject = this->pDisplacedWeapon->pContainerObj;
                        TESObjectWEAP* pweapon = dynamic_cast<TESObjectWEAP*>(pobject);
                        if (pweapon) {
                            if (pweapon->GetModelLength()) {
                                const char* v1 = pweapon->GetModel();
                                pModelLoader->ReleaseModel(v1, 0, 1u);
                            }
                        }
                        bWeaponCached = 0;
                    }
                    if (pextra) {
                        pextra->ClearScriptLocals();
                    }
                    this->pDisplacedWeapon->DeleteAllExtra();
                    ItemChange* pDisplacedWeapon = this->pDisplacedWeapon;
                    if (pDisplacedWeapon) {
                        delete pDisplacedWeapon;
                    }
                    this->pDisplacedWeapon = 0;
                    BaseProcess* CurrentAIProcess = pactor->pCurrentProcess;
                    CurrentAIProcess->SetWantWeaponDrawn(1);
                }
            }
            else {
                if (pboundArmor) {
                    TESActorBase::SEX esex = TESActorBase::SEX_MALE;
                    TESNPC* pnpc = dynamic_cast<TESNPC*>(pactor->GetObjectReference());
                    if (pnpc) {
                        esex = pnpc->GetSex();
                    }
                    InventoryChanges* pchanges_2 = InventoryChanges::GetInventoryChanges(pactor);
                    ExtraDataList* pextra_4 = pchanges_2->WearingObject(pboundArmor, 0);
                    if (pextra_4) {
                        pextra_4->RemoveCannotWearExtra();
                    }
                    pactor->QueueUnEquipObject(pboundArmor, 1, 0, 0, 0, 1);
                    pactor->RemoveItem(pboundArmor, 0, 1, 0, 0, 0, 0, 0, 1, 0);
                    for (BIPED_OBJECT ebip = BO_HEAD; ebip < BIPED_OBJECT_COUNT;
                         ebip = static_cast<BIPED_OBJECT>(ebip + 1)) {
                        if (pboundArmor->FillsBipedSlot(ebip, 0, 0)) {
                            if (pDisplacedItems[ebip]) {
                                ItemChange* pitem = pDisplacedItems[ebip];
                                ExtraDataList* pextra_5 = 0;
                                if (pitem->pExtraObjectList) {
                                    pextra_5 = pitem->pExtraObjectList->m_item;
                                }
                                if (pitem) {
                                    bool bmoved = 0;
                                    bool bsuccess_2 = 0;
                                    if (pactor->GetInventoryObjectCount(pitem->pContainerObj) >= 1) {
                                        TESBoundObject* FormFlags;
                                        pactor->QueueEquipObject(pitem->pContainerObj, 1, 0, 1, 0, 1);
                                        InventoryChanges* ContainerChanges = (&pactor->m_Extra)->GetContainerChanges();
                                        ItemChange* pcurrentItem = ContainerChanges->GetWornItem(ebip, 0);
                                        if (!pcurrentItem ||
                                            (FormFlags = pcurrentItem->pContainerObj,
                                             FormFlags != pitem->pContainerObj)) {
                                            bmoved = MoveItemToOtherBoundEffect(pactor, pitem, this);
                                        }
                                        ItemChange* pcurrentItem_2 = pcurrentItem;
                                        ItemChange* pcurrentItem_3 = pcurrentItem;
                                        if (pcurrentItem) {
                                            delete pcurrentItem_3;
                                        }
                                        bsuccess_2 = 1;
                                    }
                                    if (bWornCached) {
                                        if (pitem->pContainerObj) {
                                            if (!bmoved) {
                                                TESBipedModelForm* pbipModel =
                                                    dynamic_cast<TESBipedModelForm*>(pitem->pContainerObj);
                                                if (pbipModel) {
                                                    const char* BipedModel = pbipModel->GetBipedModel(esex);
                                                    pModelLoader->ReleaseModel(BipedModel, 0, 1u);
                                                }
                                            }
                                        }
                                    }
                                    if (pextra_5) {
                                        pextra_5->ClearScriptLocals();
                                    }
                                    pitem->DeleteAllExtra();
                                    ItemChange* v18 = pitem;
                                    ItemChange* v19 = pitem;
                                    delete pitem;
                                    pitem = 0;
                                }
                                pDisplacedItems[ebip] = 0;
                            }
                        }
                    }
                    bWornCached = 0;
                }
            }
            if (pactor->Get3D()) {
                if (pactor->pCurrentProcess) {
                    if (!pactor->pCurrentProcess->eLevel) {
                        if (pSpell) {
                            if (pEffect) {
                                EffectSetting* pavEffect = pSpell->GetAVEffect(MagicSystem::SELF);
                                MagicSystem::Range CastingType = pEffect->data.iRange;
                                EffectItem* pdominantEffect = pSpell->GetCostliestEffect(CastingType, 0);
                                if (pEffect == pdominantEffect) {
                                    if (GetEffectSetting()->GetModelLength()) {
                                        MagicHitEffect* peffect;
                                        BSTempEffect* apEffect = 0;
                                        void* apMemory = NiMemObject::operator new(sizeof(MagicModelHitEffect));
                                        if (apMemory) {
                                            const char* v6 = pavEffect->GetModel();
                                            peffect = (MagicHitEffect*)::new (apMemory)
                                                MagicModelHitEffect(pactor, v6, -1.0f);
                                        }
                                        else {
                                            peffect = 0;
                                        }
                                        if (peffect->Init()) {
                                            ReferenceProcessLists.AddTempEffect(peffect);
                                        }
                                        else {
                                            if (peffect) {
                                                delete peffect;
                                            }
                                        }
                                        if (bSpellModelCached) {
                                            const char* v7 = pavEffect->GetModel();
                                            pModelLoader->ReleaseModel(v7, 0, 1u);
                                            bSpellModelCached = 0;
                                        }
                                    }
                                    pactor->PlaySoundByEditorName(
                                        &dst, "ITMBoundDisappear", 0,
                                        BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT |
                                            BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN,
                                        1
                                    );
                                }
                            }
                        }
                    }
                }
            }
            pTarget = 0;
            bItemSummoned = 0;
        }
    }
}

void BoundItemEffect::DoSummon() {
    Actor* pactor;
    if (pTarget) {
        pactor = pTarget->GetTargetAsActor();
    }
    else {
        pactor = 0;
    }
    TESObjectWEAP* pboundWeapon = dynamic_cast<TESObjectWEAP*>(pAssociatedItem);
    TESObjectARMO* pboundArmor = dynamic_cast<TESObjectARMO*>(pAssociatedItem);
    if (pboundWeapon) {
        TESObjectWEAP* FormFlags;
        BaseProcess* v14;
        BaseProcess* v15;
        BaseProcess* v16;
        BaseProcess* CurrentAIProcess = pactor->pCurrentProcess;
        if (CurrentAIProcess->GetCurrentWeapon()) {
            ItemChange* v19;
            ExtraDataList* pextra = 0;
            BaseProcess* v23 = pactor->pCurrentProcess;
            ItemChange* v1 = v23->GetCurrentWeapon();
            pextra = v1->pExtraObjectList->m_item;
            if (pextra->ItemsInList() < 2 || (pextra->ItemsInList() == 2 && pextra->GetCount() > 1)) {
                pextra = 0;
            }
            BaseProcess* v22 = pactor->pCurrentProcess;
            TESBoundObject* pobject = v22->GetCurrentWeapon()->pContainerObj;
            if (pobject) {
                if (!bWeaponCached) {
                    TESObjectWEAP* pweapon = dynamic_cast<TESObjectWEAP*>(pobject);
                    if (pweapon) {
                        if (pweapon->GetModelLength()) {
                            const char* v2 = pweapon->GetModel();
                            pModelLoader->LoadFile(v2, LOD_MULT_NONE, 1, 0, 0, 0);
                            bWeaponCached = 1;
                        }
                    }
                }
            }
            BaseProcess* v21 = pactor->pCurrentProcess;
            BaseProcess* v20 = pactor->pCurrentProcess;
            ItemChange* v3 = v21->GetCurrentWeapon();
            ExtraDataList* v8 = v3->pExtraObjectList->m_item;
            ItemChange* v4 = v20->GetCurrentWeapon();
            pactor->QueueUnEquipObject(v4->pContainerObj, 1, v8, 0, 0, 1);
            ItemChange* apMemory = static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
            if (apMemory) {
                v19 = ::new (apMemory) ItemChange(pobject, 0);
            }
            else {
                v19 = 0;
            }
            pDisplacedWeapon = v19;
            if (pextra) {
                ExtraDataList* pcopyExtra;
                ExtraDataList* v29 = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
                if (v29) {
                    pcopyExtra = ::new (v29) ExtraDataList;
                }
                else {
                    pcopyExtra = 0;
                }
                ExtraDataList* pcopyExtra_2 = pcopyExtra;
                pcopyExtra->DuplicateExtraListForContainer(pextra);
                pDisplacedWeapon->pExtraObjectList->AddHead(pcopyExtra_2);
            }
        }
        pactor->AddObjecttoContainer(pboundWeapon, 0, 1);
        BaseProcess* v17 = pactor->pCurrentProcess;
        if (!v17->GetCurrentWeapon() ||
            (v16 = pactor->pCurrentProcess,
             (FormFlags = static_cast<TESObjectWEAP*>(v16->GetCurrentWeapon()->pContainerObj),
              FormFlags != pboundWeapon))) {
            pactor->QueueEquipObject(pboundWeapon, 1, 0, 1, 1, 1);
        }
        ExtraDataList* v46 = 0;
        if (pactor->pCurrentProcess &&
            ((v15 = pactor->pCurrentProcess, v15->GetCurrentWeapon()) &&
             (v14 = pactor->pCurrentProcess, v14->GetCurrentWeapon()->pExtraObjectList))) {
            BaseProcess* v13 = pactor->pCurrentProcess;
            ItemChange* v6 = v13->GetCurrentWeapon();
            ExtraDataList* pextra_2 = v6->pExtraObjectList->m_item;
            pextra_2->SetCanNotWear(1);
        }
        else {
            bSetupItem = 1;
        }
        BaseProcess* v12 = pactor->pCurrentProcess;
        v12->SetWantWeaponDrawn(1);
        BaseProcess* v11 = pactor->pCurrentProcess;
        v11->SetLastBoundWeapon(pboundWeapon);
    }
    else {
        if (pboundArmor) {
            ExtraDataList* pextra_4;
            TESActorBase::SEX esex = TESActorBase::SEX_MALE;
            TESNPC* pnpc = dynamic_cast<TESNPC*>(pactor->GetObjectReference());
            if (pnpc) {
                esex = pnpc->GetSex();
            }
            bool balreadyCached = bWornCached;
            for (BIPED_OBJECT ebip = BO_HEAD; ebip < BIPED_OBJECT_COUNT; ebip = static_cast<BIPED_OBJECT>(ebip + 1)) {
                ItemChange* pitem = 0;
                if (pboundArmor->FillsBipedSlot(ebip, 0, 0)) {
                    InventoryChanges* pinventory = (&pactor->m_Extra)->GetContainerChanges();
                    if (pinventory) {
                        pitem = pinventory->GetWornItem(ebip, 0);
                    }
                }
                if (pitem) {
                    ItemChange* v10;
                    if (pitem->pContainerObj) {
                        if (!balreadyCached) {
                            TESBipedModelForm* pbipModel = dynamic_cast<TESBipedModelForm*>(pitem->pContainerObj);
                            if (pbipModel) {
                                const char* BipedModel = pbipModel->GetBipedModel(esex);
                                pModelLoader->LoadFile(BipedModel, LOD_MULT_NONE, 1, 0, 0, 0);
                                bWornCached = 1;
                            }
                        }
                    }
                    ExtraDataList* pextra_3 = pitem->pExtraObjectList->m_item;
                    if (pextra_3) {
                        pextra_3->RemoveWorn(0);
                        if (pextra_3->IsExtraListEmpty()) {
                            pitem->pExtraObjectList->Remove(pextra_3);
                        }
                    }
                    ItemChange* v28 = static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
                    if (v28) {
                        v10 = ::new (v28) ItemChange(pitem);
                    }
                    else {
                        v10 = 0;
                    }
                    pDisplacedItems[ebip] = v10;
                    if (pitem) {
                        delete pitem;
                    }
                }
            }
            ExtraDataList* v27 = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
            if (v27) {
                pextra_4 = ::new (v27) ExtraDataList;
            }
            else {
                pextra_4 = 0;
            }
            pactor->AddObjecttoContainer(pboundArmor, pextra_4, 1);
            pactor->QueueEquipObject(pboundArmor, 1, 0, 1, 0, 1);
            InventoryChanges* pinventory_2 = InventoryChanges::GetInventoryChanges(pactor);
            ExtraDataList* pextra_5 = pinventory_2->WearingObject(pboundArmor, 1);
            if (pextra_5) {
                pextra_5->SetCanNotWear(1);
            }
            else {
                bSetupItem = 1;
            }
        }
        else {
            Dispel(0);
        }
    }
}

ActiveEffect* BoundItemEffect::Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
    void* v5 = ourMalloc(sizeof(BoundItemEffect));
    if (v5) {
        return ::new (v5) BoundItemEffect(apCaster, apSpell, apEffect);
    }
    else {
        return nullptr;
    }
}

bool MoveItemToOtherBoundEffect(Actor* apActor, ItemChange* apItem, BoundItemEffect* apEffect) {
    TESBipedModelForm* pmodel;
    if (apItem) {
        pmodel = dynamic_cast<TESBipedModelForm*>(apItem->pContainerObj);
    }
    else {
        pmodel = 0;
    }
    if (!apActor || !pmodel) {
        return false;
    }
    MagicTarget* ptarget = apActor;
    for (BSSimpleList<ActiveEffect*>* plist = ptarget->GetActiveEffectList(); plist && !plist->IsEmpty();
         plist = plist->m_pkNext) {
        BoundItemEffect* pboundEffect = dynamic_cast<BoundItemEffect*>(plist->m_item);
        if (pboundEffect) {
            if (pboundEffect != apEffect) {
                TESObjectARMO* pboundArmor = dynamic_cast<TESObjectARMO*>(pboundEffect->pAssociatedItem);
                if (pboundArmor) {
                    for (BIPED_OBJECT ebip = BO_HEAD; ebip < BIPED_OBJECT_COUNT;
                         ebip = static_cast<BIPED_OBJECT>(ebip + 1)) {
                        if (pmodel->FillsBipedSlot(ebip, 0, 0) &&
                            (pboundArmor->FillsBipedSlot(ebip, 0, 0) && !pboundEffect->pDisplacedItems[ebip])) {
                            ItemChange* v5;
                            void* apMemory = ourMalloc(sizeof(ItemChange));
                            if (apMemory) {
                                v5 = ::new (apMemory) ItemChange(apItem);
                            }
                            else {
                                v5 = 0;
                            }
                            pboundEffect->pDisplacedItems[ebip] = v5;
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

void ActiveEffect::SetTarget(MagicTarget* apTarget) {
    pTarget = apTarget;
}

TESForm* AssociatedItemEffect::GetAssociatedItem() {
    return pAssociatedItem;
}
