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
#include "SDK/MSVC/rtti.h"

static char bBoundItemEffectInitialized = EffectArchetypes::RegisterActiveEffect(
    EffectArchetypes::BOUND_ITEM,
    &BoundItemEffect::Instantiate
);

BoundItemEffect::BoundItemEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect)
  : AssociatedItemEffect(apCaster, apSpell, apEffect), pDisplacedWeapon(nullptr), pDisplacedItems{}, fAppearTime(0.0f),
    bItemSummoned(false), bSpellModelCached(false), bWeaponCached(false), bWornCached(false), bSetupItem(false) {}

BoundItemEffect::~BoundItemEffect() {
    if (pDisplacedWeapon && bWeaponCached) {
        TESObjectWEAP* pWeapon = dynamic_cast<TESObjectWEAP*>(pDisplacedWeapon->pContainerObj);
        if (pWeapon && pWeapon->GetModelLength()) {
            const char* pModel = pWeapon->GetModel();
            pModelLoader->ReleaseModel(pModel, false, 1u);
        }
    }

    if (bWornCached) {
        TESActorBase::SEX eSex = TESActorBase::SEX_MALE;
        if (pTarget && pTarget->GetTargetStatsObject()) {
            TESObjectREFR* pTargetRef = pTarget->GetTargetStatsObject();
            TESNPC* pNPC = dynamic_cast<TESNPC*>(pTargetRef->GetObjectReference());
            if (pNPC) {
                eSex = pNPC->GetSex();
            }
        }

        for (int32_t i = 0; i < 20; ++i) {
            if (pDisplacedItems[i] && pDisplacedItems[i]->pContainerObj) {
                TESBipedModelForm* pBipModel = dynamic_cast<TESBipedModelForm*>(pDisplacedItems[i]->pContainerObj);
                if (pBipModel) {
                    bWornCached = true;
                    const char* pModel = pBipModel->GetBipedModel(eSex);
                    pModelLoader->ReleaseModel(pModel, false, 1u);
                }
            }
        }
    }
    if (pDisplacedWeapon) {
        ExtraDataList* pExtra = pDisplacedWeapon->pExtraObjectList ? pDisplacedWeapon->pExtraObjectList->m_item
                                                                   : nullptr;
        if (pExtra) {
            pExtra->ClearScriptLocals();
        }
        pDisplacedWeapon->DeleteAllExtra();
        delete pDisplacedWeapon;
    }
    for (BIPED_OBJECT eBip = BO_HEAD; eBip < BIPED_OBJECT_COUNT; eBip = static_cast<BIPED_OBJECT>(eBip + 1)) {
        if (pDisplacedItems[eBip]) {
            ExtraDataList* pExtra = pDisplacedItems[eBip]->pExtraObjectList
                ? pDisplacedItems[eBip]->pExtraObjectList->m_item
                : nullptr;
            if (pExtra) {
                pExtra->ClearScriptLocals();
            }
            pDisplacedItems[eBip]->DeleteAllExtra();
            delete pDisplacedItems[eBip];
        }
    }
    if (bSpellModelCached && pSpell && pEffect) {
        EffectSetting* pAVEffect = pSpell->GetAVEffect(MagicSystem::SELF);
        MagicSystem::Range eRange = pEffect->data.iRange;
        EffectItem* pDominantEffect = pSpell->GetCostliestEffect(eRange, false);
        if (pAVEffect->GetModelLength() && pEffect == pDominantEffect) {
            const char* pModel = pAVEffect->GetModel();
            pModelLoader->ReleaseModel(pModel, false, 1u);
        }

        bSpellModelCached = false;
    }
}

ActiveEffect* BoundItemEffect::Copy() {
    BoundItemEffect* pCopy = new BoundItemEffect(pCaster, pSpell, pEffect);
    AssociatedItemEffect::CopyData(pCopy);
    if (pDisplacedWeapon) {
        pCopy->pDisplacedWeapon = new ItemChange(pDisplacedWeapon);
    }
    for (BIPED_OBJECT eBip = BO_HEAD; eBip < BIPED_OBJECT_COUNT; eBip = static_cast<BIPED_OBJECT>(eBip + 1)) {
        if (pDisplacedItems[eBip]) {
            pCopy->pDisplacedItems[eBip] = new ItemChange(pDisplacedItems[eBip]);
        }
    }
    if (pDisplacedWeapon && bWeaponCached) {
        TESObjectWEAP* pWeapon = dynamic_cast<TESObjectWEAP*>(pDisplacedWeapon->pContainerObj);
        if (pWeapon && pWeapon->GetModelLength()) {
            pCopy->bWeaponCached = true;
            const char* pModel = pWeapon->GetModel();
            pModelLoader->QueueModel(
                pModel,
                IO_TASK_PRIORITY_CRITICAL,
                static_cast<QueuedFile*>(nullptr),
                LOD_MULT_NONE,
                true,
                true,
                false
            );
        }
    }

    if (bWornCached) {
        TESActorBase::SEX eSex = TESActorBase::SEX_MALE;
        if (pTarget->GetTargetStatsObject()) {
            TESObjectREFR* pTargetRef = pTarget->GetTargetStatsObject();
            TESNPC* pNPC = dynamic_cast<TESNPC*>(pTargetRef->GetObjectReference());
            if (pNPC) {
                eSex = pNPC->GetSex();
            }
        }
        for (int32_t i = 0; i < 20; ++i) {
            if (pDisplacedItems[i] && pDisplacedItems[i]->pContainerObj) {
                TESBipedModelForm* pBipModel = dynamic_cast<TESBipedModelForm*>(pDisplacedItems[i]->pContainerObj);
                if (pBipModel) {
                    pCopy->bWornCached = true;
                    const char* pModel = pBipModel->GetBipedModel(eSex);
                    pModelLoader->QueueModel(
                        pModel,
                        IO_TASK_PRIORITY_CRITICAL,
                        static_cast<QueuedFile*>(nullptr),
                        LOD_MULT_NONE,
                        true,
                        true,
                        false
                    );
                }
            }
        }
    }
    pCopy->bItemSummoned = bItemSummoned;
    return pCopy;
}

void BoundItemEffect::Update(float afSecondsElapsed) {
    if (bItemSummoned) {
        Actor* pActor = pTarget ? pTarget->GetTargetAsActor() : nullptr;
        TESObjectWEAP* pBoundWeapon = dynamic_cast<TESObjectWEAP*>(pAssociatedItem);
        if (pActor && pActor->pCurrentProcess && pBoundWeapon) {
            pActor->pCurrentProcess->SetLastBoundWeapon(pBoundWeapon);
            if (bSetupItem) {
                if (pActor->pCurrentProcess->GetCurrentWeapon()) {
                    ItemChange* pItem = pActor->pCurrentProcess->GetCurrentWeapon();
                    if (pBoundWeapon == pItem->pContainerObj && pItem->pExtraObjectList) {
                        bSetupItem = false;
                        pItem->pExtraObjectList->m_item->SetCanNotWear(true);
                    }
                }
            }
            if (pActor == pPlayer) {
                if (!pActor->pCurrentProcess->GetWeaponDrawn() && !pActor->GetWantWeaponDrawn()) {
                    Dispel(false);
                }
            }
        }
        else if (bSetupItem) {
            TESObjectARMO* pBoundArmor = dynamic_cast<TESObjectARMO*>(pAssociatedItem);
            if (pBoundArmor) {
                InventoryChanges* pInventory = InventoryChanges::GetInventoryChanges(pActor);
                ExtraDataList* pExtra = pInventory->WearingObject(pBoundArmor, true);
                if (pExtra) {
                    bSetupItem = false;
                    pExtra->SetCanNotWear(true);
                }
            }
        }
    }
    else if (fAppearTime <= static_cast<double>(fElapsedSeconds)) {
        DoSummon();
        bItemSummoned = true;
    }
}

uint16_t BoundItemEffect::GetSaveSize(Actor* apActor) {
    uint16_t usSize = AssociatedItemEffect::GetSaveSize(apActor);
    uint16_t usStartSize = usSize;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        usSize += 6;
    }
    usSize = usSize + 1;
    if (pDisplacedWeapon) {
        usSize += pDisplacedWeapon->GetSaveSize();
    }
    for (int32_t i = 0; i < 20; ++i) {
        ++usSize;
        if (pDisplacedItems[i]) {
            usSize += pDisplacedItems[i]->GetSaveSize();
        }
    }
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                usSize - usStartSize,
                pHeader->iFormID,
                pForm->GetFormEditorID(),
                pHeader->iFlags,
                667,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\BoundItemEffect.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s",
                usSize - usStartSize,
                667,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\BoundItemEffect.cpp"
            );
        }
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 65u) {
        usSize += 5;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 107u) {
        ++usSize;
    }
    return usSize;
}

void BoundItemEffect::SaveGame(Actor* apActor) {
    AssociatedItemEffect::SaveGame(apActor);
    uint16_t usBlockSize = 0;
    uint16_t* pBlockSize = nullptr;
    uint8_t* pTempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    if (bDebugSaveBuffer.Bool()) {
        pTempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint32_t uiBlockHeader = 0x424C4F4B;
        pSaveLoadGameOLD->SaveGameDataOLD(&uiBlockHeader, sizeof(uiBlockHeader));
        pBlockSize = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->SaveGameDataOLD(&usBlockSize, sizeof(usBlockSize));
    }
    bool bDisplaced = pDisplacedWeapon != nullptr;
    pSaveLoadGameOLD->SaveGameDataOLD(&bDisplaced, sizeof(bDisplaced));
    if (bDisplaced) {
        pDisplacedWeapon->SaveGame();
    }
    for (int32_t i = 0; i < 20; ++i) {
        bDisplaced = pDisplacedItems[i] != nullptr;
        pSaveLoadGameOLD->SaveGameDataOLD(&bDisplaced, sizeof(bDisplaced));
        if (bDisplaced) {
            pDisplacedItems[i]->SaveGame();
        }
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 65u) {
        pSaveLoadGameOLD->SaveGameDataOLD(&fAppearTime, sizeof(fAppearTime));
        pSaveLoadGameOLD->SaveGameDataOLD(&bItemSummoned, sizeof(bItemSummoned));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 107u) {
        pSaveLoadGameOLD->SaveGameDataOLD(&bSetupItem, sizeof(bSetupItem));
    }
    if (bDebugSaveBuffer.Bool()) {
        uint8_t* pEndPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            uint32_t uiFlags = pHeader->iFlags;
            const char* pEditorID = pForm->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                static_cast<int32_t>(pEndPosition - pTempStartPosition),
                pHeader->iFormID,
                pEditorID,
                uiFlags,
                721,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\BoundItemEffect.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s",
                static_cast<int32_t>(pEndPosition - pTempStartPosition),
                721,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\BoundItemEffect.cpp"
            );
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        // The block size counts from the size field itself to the current write position. pBlockSize points into the
        // save buffer, which holds untyped raw bytes.
        uint8_t* pCurrentPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        uint8_t* pBlockStart = reinterpret_cast<uint8_t*>(pBlockSize);
        if (pCurrentPosition > pBlockStart + 0xFFFF) {
            Warning(
                "SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\BoundItemEffect.cpp",
                721
            );
        }
        *pBlockSize = static_cast<uint16_t>(pCurrentPosition - pBlockStart);
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
        pSaveLoadGameOLD->LoadGameDataOLD(&uiBlockID, sizeof(uiBlockID));
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
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\BoundItemEffect.cpp",
                    730,
                    pHeader->iFormID,
                    pEditorID,
                    uiVersion,
                    uiFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  "
                    "Current version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\BoundItemEffect.cpp",
                    730,
                    static_cast<uint32_t>(pSaveLoadGameOLD->GetCurrentVersion())
                );
            }
        }
        pBlockStart = static_cast<uint8_t*>(pSaveLoadGameOLD->GetBufferPointer());
        pSaveLoadGameOLD->LoadGameDataOLD(&usBlockSize, sizeof(usBlockSize));
    }
    bool bPresent;
    pSaveLoadGameOLD->LoadGameDataOLD(&bPresent, sizeof(bPresent));
    if (bPresent) {
        ItemChange* pWeapon = new ItemChange;
        pDisplacedWeapon = pWeapon;
        pDisplacedWeapon->LoadGame();
    }
    for (int32_t i = 0; i < 20; ++i) {
        pSaveLoadGameOLD->LoadGameDataOLD(&bPresent, sizeof(bPresent));
        if (bPresent) {
            ItemChange* pItem = new ItemChange;
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
        pSaveLoadGameOLD->LoadGameDataOLD(&fAppearTime, sizeof(fAppearTime));
        pSaveLoadGameOLD->LoadGameDataOLD(&bItemSummoned, sizeof(bItemSummoned));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x6B) {
        pSaveLoadGameOLD->LoadGameDataOLD(&bSetupItem, sizeof(bSetupItem));
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
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\BoundItemEffect.cpp",
                    770,
                    pHeader->iFormID,
                    pEditorID,
                    uiVersion,
                    uiFlags
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
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\BoundItemEffect.cpp",
                    770,
                    pHeader->iFormID,
                    pEditorID,
                    uiVersion,
                    uiFlags
                );
            }
        }
        else if (pBlockEnd > &pBlockStart[usBlockSize]) {
            Warning(
                "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                static_cast<int32_t>(pBlockEnd - &pBlockStart[usBlockSize]),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\BoundItemEffect.cpp",
                770,
                static_cast<uint32_t>(pSaveLoadGameOLD->GetCurrentVersion())
            );
        }
        else if (pBlockEnd < &pBlockStart[usBlockSize]) {
            Warning(
                "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is %i",
                static_cast<int32_t>(&pBlockStart[usBlockSize] - pBlockEnd),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\BoundItemEffect.cpp",
                770,
                static_cast<uint32_t>(pSaveLoadGameOLD->GetCurrentVersion())
            );
        }
    }
}

void BoundItemEffect::InitLoadGame(Actor* apActor) {
    AssociatedItemEffect::InitLoadGame(apActor);
    Actor* pActor = pTarget ? pTarget->GetTargetAsActor() : nullptr;
    if (bItemSummoned) {
        if (pDisplacedWeapon) {
            TESObjectWEAP* pWeapon = dynamic_cast<TESObjectWEAP*>(pDisplacedWeapon->pContainerObj);
            if (pWeapon && pWeapon->GetModelLength()) {
                bWeaponCached = true;
                const char* pModel = pWeapon->GetModel();
                pModelLoader->QueueModel(
                    pModel,
                    IO_TASK_PRIORITY_CRITICAL,
                    static_cast<QueuedFile*>(nullptr),
                    LOD_MULT_NONE,
                    true,
                    false,
                    false
                );
            }
        }
        TESActorBase::SEX eSex = TESActorBase::SEX_MALE;
        if (pActor) {
            TESNPC* pNPC = dynamic_cast<TESNPC*>(pActor->GetObjectReference());
            if (pNPC) {
                eSex = pNPC->GetSex();
            }
        }
        for (int32_t i = 0; i < 20; ++i) {
            if (pDisplacedItems[i] && pDisplacedItems[i]->pContainerObj) {
                TESBipedModelForm* pBipModel = dynamic_cast<TESBipedModelForm*>(pDisplacedItems[i]->pContainerObj);
                if (pBipModel) {
                    bWornCached = true;
                    const char* pModel = pBipModel->GetBipedModel(eSex);
                    pModelLoader->QueueModel(
                        pModel,
                        IO_TASK_PRIORITY_CRITICAL,
                        static_cast<QueuedFile*>(nullptr),
                        LOD_MULT_NONE,
                        true,
                        true,
                        false
                    );
                }
            }
        }
    }
    if (pDisplacedWeapon) {
        pDisplacedWeapon->InitLoadGame_0();
        if (pSaveLoadGameOLD->GetCurrentVersion() < 97u) {
            ItemChange* pCopy = new ItemChange(pDisplacedWeapon);
            if (pDisplacedWeapon) {
                delete pDisplacedWeapon;
            }
            pDisplacedWeapon = pCopy;
        }
        pActor->QueueUnEquipObject(pDisplacedWeapon->pContainerObj, 1, nullptr, false, false, true);
    }
    for (int32_t i = 0; i < 20; ++i) {
        if (pDisplacedItems[i]) {
            pDisplacedItems[i]->InitLoadGame_0();
            if (pSaveLoadGameOLD->GetCurrentVersion() < 97u) {
                ItemChange* pCopy = new ItemChange(pDisplacedItems[i]);
                if (pDisplacedItems[i]) {
                    delete pDisplacedItems[i];
                }
                pDisplacedItems[i] = pCopy;
            }
            pActor->QueueUnEquipObject(pDisplacedItems[i]->pContainerObj, 1, nullptr, false, false, true);
        }
    }
    if (bItemSummoned) {
        InventoryChanges* pInventory = pActor->m_Extra.GetContainerChanges();
        TESBoundObject* pBound = dynamic_cast<TESBoundObject*>(pAssociatedItem);
        if (pInventory && !pInventory->GetObjectCount(pBound)) {
            pActor->AddObjecttoContainer(pBound, nullptr, 1);
            pActor->QueueEquipObject(pBound, 1, nullptr, true, false, true);
        }
    }
}

bool BoundItemEffect::CheckTarget(MagicTarget* apTarget) {
    return dynamic_cast<Character*>(apTarget) != nullptr;
}

void BoundItemEffect::Start() {
    Actor* pActor = pTarget ? pTarget->GetTargetAsActor() : nullptr;
    if (pAssociatedItem && pActor) {
        if (pSpell && pEffect) {
            EffectSetting* pAVEffect = pSpell->GetAVEffect(MagicSystem::SELF);
            MagicSystem::Range eRange = pEffect->data.iRange;
            EffectItem* pDominantEffect = pSpell->GetCostliestEffect(eRange, false);
            if (pAVEffect->GetModelLength()) {
                const char* pModel = pAVEffect->GetModel();
                NiAVObject* pEffectModel = pModelLoader->LoadFile(pModel, LOD_MULT_NONE, true, false, false, false);
                if (pSpell->GetSpellType() == MagicSystem::ABILITY) {
                    fAppearTime = 0.0f;
                }
                else if (pEffectModel) {
                    fAppearTime = FindTimeForNoteKey(pEffectModel, "SpecialIdle_HitEffect", "Hit");
                }
                if (pEffect != pDominantEffect) {
                    pModel = pAVEffect->GetModel();
                    pModelLoader->ReleaseModel(pModel, false, 1u);
                }
            }
        }
    }
    else {
        Dispel(false);
    }
}

void BoundItemEffect::Finish() {
    Actor* pActor = pTarget ? pTarget->GetTargetAsActor() : nullptr;
    if (bItemSummoned && pActor) {
        TESObjectWEAP* pBoundWeapon = dynamic_cast<TESObjectWEAP*>(pAssociatedItem);
        TESObjectARMO* pBoundArmor = dynamic_cast<TESObjectARMO*>(pAssociatedItem);
        if (pBoundWeapon) {
            InventoryChanges* pChanges = InventoryChanges::GetInventoryChanges(pActor);
            ExtraDataList* pWornExtra = pChanges->WearingObject(pBoundWeapon, false);
            if (pWornExtra) {
                pWornExtra->RemoveCannotWearExtra();
            }
            pActor->QueueUnEquipObject(pBoundWeapon, 1, nullptr, false, false, true);
            pActor->RemoveItem(pBoundWeapon, nullptr, 1, false, false, nullptr, nullptr, nullptr, true, false);
            if (this->pDisplacedWeapon) {
                ExtraDataList* pExtra = nullptr;
                if (this->pDisplacedWeapon->pExtraObjectList) {
                    pExtra = this->pDisplacedWeapon->pExtraObjectList->m_item;
                }
                if (pActor->GetInventoryObjectCount(this->pDisplacedWeapon->pContainerObj) >= 1) {
                    pActor->QueueEquipObject(this->pDisplacedWeapon->pContainerObj, 1, nullptr, true, false, true);
                }
                if (bWeaponCached) {
                    TESObjectWEAP* pWeapon = dynamic_cast<TESObjectWEAP*>(this->pDisplacedWeapon->pContainerObj);
                    if (pWeapon && pWeapon->GetModelLength()) {
                        const char* pModel = pWeapon->GetModel();
                        pModelLoader->ReleaseModel(pModel, false, 1u);
                    }

                    bWeaponCached = false;
                }
                if (pExtra) {
                    pExtra->ClearScriptLocals();
                }
                this->pDisplacedWeapon->DeleteAllExtra();
                ItemChange* pDisplacedWeapon = this->pDisplacedWeapon;
                if (pDisplacedWeapon) {
                    delete pDisplacedWeapon;
                }
                this->pDisplacedWeapon = nullptr;
                pActor->pCurrentProcess->SetWantWeaponDrawn(true);
            }
        }
        else if (pBoundArmor) {
            TESActorBase::SEX eSex = TESActorBase::SEX_MALE;
            TESNPC* pNPC = dynamic_cast<TESNPC*>(pActor->GetObjectReference());
            if (pNPC) {
                eSex = pNPC->GetSex();
            }
            InventoryChanges* pChanges = InventoryChanges::GetInventoryChanges(pActor);
            ExtraDataList* pWornExtra = pChanges->WearingObject(pBoundArmor, false);
            if (pWornExtra) {
                pWornExtra->RemoveCannotWearExtra();
            }
            pActor->QueueUnEquipObject(pBoundArmor, 1, nullptr, false, false, true);
            pActor->RemoveItem(pBoundArmor, nullptr, 1, false, false, nullptr, nullptr, nullptr, true, false);
            for (BIPED_OBJECT eBip = BO_HEAD; eBip < BIPED_OBJECT_COUNT; eBip = static_cast<BIPED_OBJECT>(eBip + 1)) {
                if (pBoundArmor->FillsBipedSlot(eBip, false, nullptr) && pDisplacedItems[eBip]) {
                    ItemChange* pItem = pDisplacedItems[eBip];
                    ExtraDataList* pExtra = nullptr;
                    if (pItem->pExtraObjectList) {
                        pExtra = pItem->pExtraObjectList->m_item;
                    }
                    if (pItem) {
                        bool bMoved = false;
                        if (pActor->GetInventoryObjectCount(pItem->pContainerObj) >= 1) {
                            pActor->QueueEquipObject(pItem->pContainerObj, 1, nullptr, true, false, true);
                            ItemChange* pCurrentItem = pActor->m_Extra.GetContainerChanges()->GetWornItem(eBip, false);
                            if (!pCurrentItem || pCurrentItem->pContainerObj != pItem->pContainerObj) {
                                bMoved = MoveItemToOtherBoundEffect(pActor, pItem, this);
                            }
                            if (pCurrentItem) {
                                delete pCurrentItem;
                            }
                        }
                        if (bWornCached && pItem->pContainerObj && !bMoved) {
                            TESBipedModelForm* pBipModel = dynamic_cast<TESBipedModelForm*>(pItem->pContainerObj);
                            if (pBipModel) {
                                const char* pModel = pBipModel->GetBipedModel(eSex);
                                pModelLoader->ReleaseModel(pModel, false, 1u);
                            }
                        }

                        if (pExtra) {
                            pExtra->ClearScriptLocals();
                        }
                        pItem->DeleteAllExtra();
                        delete pItem;
                        pItem = nullptr;
                    }
                    pDisplacedItems[eBip] = nullptr;
                }
            }
            bWornCached = false;
        }
        if (pActor->Get3D() && pActor->pCurrentProcess && !pActor->pCurrentProcess->eLevel && pSpell && pEffect) {
            EffectSetting* pAVEffect = pSpell->GetAVEffect(MagicSystem::SELF);
            MagicSystem::Range eRange = pEffect->data.iRange;
            EffectItem* pDominantEffect = pSpell->GetCostliestEffect(eRange, false);
            if (pEffect == pDominantEffect) {
                if (GetEffectSetting()->GetModelLength()) {
                    MagicHitEffect* pHitEffect = new MagicModelHitEffect(pActor, pAVEffect->GetModel(), -1.0f);
                    if (pHitEffect->Init()) {
                        ReferenceProcessLists.AddTempEffect(pHitEffect);
                    }
                    else if (pHitEffect) {
                        delete pHitEffect;
                    }
                    if (bSpellModelCached) {
                        const char* pModel = pAVEffect->GetModel();
                        pModelLoader->ReleaseModel(pModel, false, 1u);
                        bSpellModelCached = false;
                    }
                }
                pActor->PlaySoundByEditorName(
                    "ITMBoundDisappear",
                    false,
                    BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT |
                        BSGameSound::SOUND_FLAG_ANIMATION_DRIVEN,
                    true
                );
            }
        }

        pTarget = nullptr;
        bItemSummoned = false;
    }
}

void BoundItemEffect::DoSummon() {
    Actor* pActor = pTarget ? pTarget->GetTargetAsActor() : nullptr;
    TESObjectWEAP* pBoundWeapon = dynamic_cast<TESObjectWEAP*>(pAssociatedItem);
    TESObjectARMO* pBoundArmor = dynamic_cast<TESObjectARMO*>(pAssociatedItem);
    if (pBoundWeapon) {
        if (pActor->pCurrentProcess->GetCurrentWeapon()) {
            ExtraDataList* pExtra = pActor->pCurrentProcess->GetCurrentWeapon()->pExtraObjectList->m_item;
            if (pExtra->ItemsInList() < 2 || (pExtra->ItemsInList() == 2 && pExtra->GetCount() > 1)) {
                pExtra = nullptr;
            }
            TESBoundObject* pObject = pActor->pCurrentProcess->GetCurrentWeapon()->pContainerObj;
            if (pObject && !bWeaponCached) {
                TESObjectWEAP* pWeapon = dynamic_cast<TESObjectWEAP*>(pObject);
                if (pWeapon && pWeapon->GetModelLength()) {
                    const char* pModel = pWeapon->GetModel();
                    pModelLoader->LoadFile(pModel, LOD_MULT_NONE, true, false, false, false);
                    bWeaponCached = true;
                }
            }

            BaseProcess* pExtraProcess = pActor->pCurrentProcess;
            BaseProcess* pObjectProcess = pActor->pCurrentProcess;
            ExtraDataList* pWeaponExtra = pExtraProcess->GetCurrentWeapon()->pExtraObjectList->m_item;
            pActor->QueueUnEquipObject(
                pObjectProcess->GetCurrentWeapon()->pContainerObj,
                1,
                pWeaponExtra,
                false,
                false,
                true
            );
            pDisplacedWeapon = new ItemChange(pObject, 0);
            if (pExtra) {
                ExtraDataList* pCopyExtra = new ExtraDataList;
                pCopyExtra->DuplicateExtraListForContainer(pExtra);
                pDisplacedWeapon->pExtraObjectList->AddHead(pCopyExtra);
            }
        }
        pActor->AddObjecttoContainer(pBoundWeapon, nullptr, 1);
        if (!pActor->pCurrentProcess->GetCurrentWeapon() ||
            static_cast<TESObjectWEAP*>(pActor->pCurrentProcess->GetCurrentWeapon()->pContainerObj) != pBoundWeapon) {
            pActor->QueueEquipObject(pBoundWeapon, 1, nullptr, true, true, true);
        }
        if (pActor->pCurrentProcess &&
            pActor->pCurrentProcess->GetCurrentWeapon() &&
            pActor->pCurrentProcess->GetCurrentWeapon()->pExtraObjectList) {
            ExtraDataList* pExtra = pActor->pCurrentProcess->GetCurrentWeapon()->pExtraObjectList->m_item;
            pExtra->SetCanNotWear(true);
        }
        else {
            bSetupItem = true;
        }
        pActor->pCurrentProcess->SetWantWeaponDrawn(true);
        pActor->pCurrentProcess->SetLastBoundWeapon(pBoundWeapon);
    }
    else if (pBoundArmor) {
        TESActorBase::SEX eSex = TESActorBase::SEX_MALE;
        TESNPC* pNPC = dynamic_cast<TESNPC*>(pActor->GetObjectReference());
        if (pNPC) {
            eSex = pNPC->GetSex();
        }
        bool bAlreadyCached = bWornCached;
        for (BIPED_OBJECT eBip = BO_HEAD; eBip < BIPED_OBJECT_COUNT; eBip = static_cast<BIPED_OBJECT>(eBip + 1)) {
            ItemChange* pItem = nullptr;
            if (pBoundArmor->FillsBipedSlot(eBip, false, nullptr)) {
                InventoryChanges* pInventory = pActor->m_Extra.GetContainerChanges();
                if (pInventory) {
                    pItem = pInventory->GetWornItem(eBip, false);
                }
            }
            if (pItem) {
                if (pItem->pContainerObj && !bAlreadyCached) {
                    TESBipedModelForm* pBipModel = dynamic_cast<TESBipedModelForm*>(pItem->pContainerObj);
                    if (pBipModel) {
                        const char* pModel = pBipModel->GetBipedModel(eSex);
                        pModelLoader->LoadFile(pModel, LOD_MULT_NONE, true, false, false, false);
                        bWornCached = true;
                    }
                }

                ExtraDataList* pExtra = pItem->pExtraObjectList->m_item;
                if (pExtra) {
                    pExtra->RemoveWorn(false);
                    if (pExtra->IsExtraListEmpty()) {
                        pItem->pExtraObjectList->Remove(pExtra);
                    }
                }
                pDisplacedItems[eBip] = new ItemChange(pItem);
                if (pItem) {
                    delete pItem;
                }
            }
        }
        ExtraDataList* pArmorExtra = new ExtraDataList;
        pActor->AddObjecttoContainer(pBoundArmor, pArmorExtra, 1);
        pActor->QueueEquipObject(pBoundArmor, 1, nullptr, true, false, true);
        InventoryChanges* pInventory = InventoryChanges::GetInventoryChanges(pActor);
        ExtraDataList* pWornExtra = pInventory->WearingObject(pBoundArmor, true);
        if (pWornExtra) {
            pWornExtra->SetCanNotWear(true);
        }
        else {
            bSetupItem = true;
        }
    }
    else {
        Dispel(false);
    }
}

ActiveEffect* BoundItemEffect::Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
    return new BoundItemEffect(apCaster, apSpell, apEffect);
}

bool MoveItemToOtherBoundEffect(Actor* apActor, ItemChange* apItem, BoundItemEffect* apEffect) {
    TESBipedModelForm* pModel = nullptr;
    if (apItem) {
        pModel = dynamic_cast<TESBipedModelForm*>(apItem->pContainerObj);
    }
    if (!apActor || !pModel) {
        return false;
    }
    MagicTarget* pTarget = apActor;
    for (BSSimpleList<ActiveEffect*>* pList = pTarget->GetActiveEffectList(); pList && !pList->IsEmpty();
         pList = pList->m_pkNext) {
        BoundItemEffect* pBoundEffect = dynamic_cast<BoundItemEffect*>(pList->m_item);
        if (pBoundEffect && pBoundEffect != apEffect) {
            TESObjectARMO* pBoundArmor = dynamic_cast<TESObjectARMO*>(pBoundEffect->pAssociatedItem);
            if (pBoundArmor) {
                for (BIPED_OBJECT eBip = BO_HEAD; eBip < BIPED_OBJECT_COUNT;
                     eBip = static_cast<BIPED_OBJECT>(eBip + 1)) {
                    if (pModel->FillsBipedSlot(eBip, false, nullptr) &&
                        pBoundArmor->FillsBipedSlot(eBip, false, nullptr) &&
                        !pBoundEffect->pDisplacedItems[eBip]) {
                        pBoundEffect->pDisplacedItems[eBip] = new ItemChange(apItem);
                        return true;
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
