#include "Fallout Shared/Package.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/BSFurnitureMarker.h"
#include "BSMain/MessageHandler.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "Fallout Shared/AiFormulas.h"
#include "Fallout Shared/BGSIdleCollection.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Magic/AlchemyItem.h"
#include "Fallout Shared/Magic/EffectItemList.h"
#include "Fallout Shared/Magic/IngredientItem.h"
#include "Fallout Shared/Magic/SpellItem.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/PackageData.h"
#include "Fallout Shared/PackageEventAction.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout Shared/PackageSchedule.h"
#include "Fallout Shared/PackageTarget.h"
#include "Fallout Shared/TESCombatStyle.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/AlarmPackage.h"
#include "Fallout/AI/BackUpPackage.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/DialoguePackage.h"
#include "Fallout/AI/FleePackage.h"
#include "Fallout/AI/GuardPackage.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/AI/SpectatorPackage.h"
#include "Fallout/AI/TrespassPackage.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Misc/Calendar.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/FaceGen/assert.hpp"
#include "SDK/MSVC/rtti.h"
#include "xfnv/Compiler.h"

// The reference EvaluateEscortReference accepted last; GetEscortTargetForPackage takes it and clears it.
static TESObjectREFR* pTempTarget;

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<GameSettingCollection> fTargetSearchRadius("fTargetSearchRadius", 20000.0f);
static SettingT<GameSettingCollection> sCrimeTypeSteal("sCrimeTypeSteal", "Steal");
static SettingT<GameSettingCollection> sCrimeTypePickpocket("sCrimeTypePickpocket", "Pickpocket");
static SettingT<GameSettingCollection> sCrimeTypeTrespass("sCrimeTypeTrespass", "Trespass");
static SettingT<GameSettingCollection> sCrimeTypeAttack("sCrimeTypeAttack", "Attack");
static SettingT<GameSettingCollection> sCrimeTypeMurder("sCrimeTypeMurder", "Murder");

void PACKAGE_DATA::Endian() {
    ENDIAN(iPackFlags, false);
    ENDIAN(iFOBehaviorFlags, false);
    ENDIAN(iPackageSpecificFlags, false);
}

TESPackage::TESPackage() {
    SetFormType(73);
    TESPackage::InitializeData();
    uiRefCount = 0;
}

// Unless the data handler is clearing its data, takes the package off the actors running it (always while a save
// loads, and for its types 21 and 23) under a held reference; then clears the data.
TESPackage::~TESPackage() {
    if (!pDataHandler->GetClearingData()) {
        ++uiRefCount;
        if (pSaveLoadGame->GetSaveGameLoading() ||
            ((((int32_t)GetPackType()) == 21) || (((int32_t)GetPackType()) == 23))) {
            (&ReferenceProcessLists)->NullOutActorsPackage(this);
        }
        --uiRefCount;
    }
    SetIsCreated(false);
    TESPackage::ClearData();
}

void TESPackage::InitializeData() {
    PackData.iPackFlags = 0;
    PackData.iPackageSpecificFlags = 0;
    PackData.iFOBehaviorFlags = 0;
    pPackLoc = nullptr;
    pPackTarg = nullptr;
    pIdleCollection = nullptr;
    pCombatStyle = nullptr;
    pPackData = nullptr;
    SetPackageSchedule(nullptr);
    ePROCEDURE_TYPE = PACKAGE_PROCEDURE_NONE;
    PackData.cPackType = static_cast<int8_t>(PACKAGE_TYPE_NONE);
    SetIsCreated(false);
}

// The location, target, idles and type data go through their own deleting destructors with flag 1, the type data's
// through the vtable; then the conditions and the base's data component.
void TESPackage::ClearData() {
    if (pPackLoc) {
        delete pPackLoc;
    }
    if (pPackTarg) {
        delete pPackTarg;
    }
    if (pIdleCollection) {
        delete pIdleCollection;
    }
    if (pPackData) {
        delete pPackData;
    }
    GetConditions()->ClearAllConditionItems();
    TESForm::InitializeDataComponent();
}

// Reads a PACK record. The type data chunks replace the package's type data; the second location and second target
// chunks go into the type data of the types that keep them. Location, target and schedule chunks are built into a
// temporary with the file-data constructor, copied in by the setter and destroyed, as the game does.
bool TESPackage::Load(TESFile* apFile) {
    // Compares only the low byte of the record type.
    if (static_cast<uint8_t>(apFile->GetTESForm()) != PACK_ID) {
        return false;
    }
    TESForm::LoadForm(apFile);
    do {
        switch (apFile->GetTESChunk()) {
            case EDID_ID: {
                // The buffer is sized by the chunk, but the read is deliberately capped at 0x200 bytes, as the game
                // does.
                char* pMessage = static_cast<char*>(XFNV_ALLOCA(apFile->GetChunkSize()));
                apFile->GetChunkData(pMessage, 0x200);
                SetFormEditorID(pMessage);
                break;
            }
            case OBND_ID: LoadObjectBound(apFile); break;
            case PKDT_ID: {
                PACKAGE_DATA PData;
                ourMemset(&PData, 0, sizeof(PACKAGE_DATA));
                apFile->GetChunkData(&PData, sizeof(PACKAGE_DATA));
                if (apFile->QEndian()) {
                    PData.Endian();
                }
                SetPackType(static_cast<PACKAGE_TYPE>(PData.cPackType));
                PackData = PData;
                break;
            }
            case PLDT_ID: {
                PACK_LOC_FILE PLData;
                ourMemset(&PLData, 0, sizeof(PACK_LOC_FILE));
                apFile->GetChunkData(&PLData, sizeof(PACK_LOC_FILE));
                if (apFile->QEndian()) {
                    PLData.Endian();
                }
                alignas(PackageLocation) uint8_t packLocationStorage[sizeof(PackageLocation)];
                PackageLocation* pPackLocation = ::new (packLocationStorage) PackageLocation(&PLData);
                SetPackageLocation(pPackLocation);
                pPackLocation->~PackageLocation();
                break;
            }
            case PKDD_ID: {
                PACK_DIALOGUE_DATA PLData;
                ourMemset(&PLData, 0, sizeof(PACK_DIALOGUE_DATA));
                apFile->GetChunkData(&PLData, sizeof(PACK_DIALOGUE_DATA));
                if (apFile->QEndian()) {
                    PLData.Endian();
                }
                if (pPackData) {
                    delete pPackData;
                }
                pPackData = nullptr;
                void* pMemory = ourMalloc(sizeof(TESDialoguePackageData));
                pPackData = pMemory ? ::new (pMemory) TESDialoguePackageData(PLData) : nullptr;
                break;
            }
            case PKFD_ID: {
                PACK_FOLLOW_DATA PLData;
                ourMemset(&PLData, 0, sizeof(PACK_FOLLOW_DATA));
                apFile->GetChunkData(&PLData, sizeof(PACK_FOLLOW_DATA));
                if (apFile->QEndian()) {
                    PLData.Endian();
                }
                if (pPackData) {
                    delete pPackData;
                }
                pPackData = nullptr;
                void* pMemory = ourMalloc(sizeof(TESFollowPackageData));
                pPackData = pMemory ? ::new (pMemory) TESFollowPackageData(PLData) : nullptr;
                break;
            }
            case PKES_ID: {
                PACK_ESCORT_DATA PEData;
                ourMemset(&PEData, 0, sizeof(PACK_ESCORT_DATA));
                apFile->GetChunkData(&PEData, sizeof(PACK_ESCORT_DATA));
                if (apFile->QEndian()) {
                    PEData.Endian();
                }
                if (pPackData) {
                    delete pPackData;
                }
                pPackData = nullptr;
                void* pMemory = ourMalloc(sizeof(TESEscortPackageData));
                pPackData = pMemory ? ::new (pMemory) TESEscortPackageData(&PEData) : nullptr;
                break;
            }
            // The use weapon and patrol chunks free the old type data without clearing the pointer first.
            case PKW2_ID: {
                PACK_USE_WEAPON_DATA_PKW2 useWeaponData;
                ourMemset(&useWeaponData, 0, sizeof(PACK_USE_WEAPON_DATA_PKW2));
                apFile->GetChunkData(&useWeaponData, sizeof(PACK_USE_WEAPON_DATA_PKW2));
                if (apFile->QEndian()) {
                    useWeaponData.Endian();
                }
                if (pPackData) {
                    delete pPackData;
                }
                void* pMemory = ourMalloc(sizeof(TESUseWeaponPackageData));
                pPackData = pMemory ? ::new (pMemory) TESUseWeaponPackageData(useWeaponData) : nullptr;
                break;
            }
            case PKW3_ID: {
                PACK_USE_WEAPON_DATA_PKW3 useWeaponData;
                ourMemset(&useWeaponData, 0, sizeof(PACK_USE_WEAPON_DATA_PKW3));
                apFile->GetChunkData(&useWeaponData, sizeof(PACK_USE_WEAPON_DATA_PKW3));
                if (apFile->QEndian()) {
                    useWeaponData.Endian();
                }
                if (pPackData) {
                    delete pPackData;
                }
                void* pMemory = ourMalloc(sizeof(TESUseWeaponPackageData));
                pPackData = pMemory ? ::new (pMemory) TESUseWeaponPackageData(useWeaponData) : nullptr;
                break;
            }
            case PKPT_ID: {
                PACK_PATROL_DATA patrolData;
                ourMemset(&patrolData, 0, sizeof(PACK_PATROL_DATA));
                apFile->GetChunkData(&patrolData, sizeof(PACK_PATROL_DATA));
                if (apFile->QEndian()) {
                    patrolData.Endian();
                }
                if (pPackData) {
                    delete pPackData;
                }
                void* pMemory = ourMalloc(sizeof(TESPatrolPackageData));
                pPackData = pMemory ? ::new (pMemory) TESPatrolPackageData(patrolData) : nullptr;
                break;
            }
            // The dialogue and use weapon data take a new second location without freeing the one they hold.
            case PLD2_ID: {
                PACK_LOC_FILE PLData;
                ourMemset(&PLData, 0, sizeof(PACK_LOC_FILE));
                apFile->GetChunkData(&PLData, sizeof(PACK_LOC_FILE));
                if (apFile->QEndian()) {
                    PLData.Endian();
                }
                switch (GetPackType()) {
                    case PACKAGE_TYPE_DIALOGUE:
                    case PACKAGE_TYPE_IN_GAME_DIALOGUE: {
                        TESDialoguePackageData* pddata = static_cast<TESDialoguePackageData*>(pPackData);
                        void* pMemory = ourMalloc(sizeof(PackageLocation));
                        pddata->pTargetLocation = pMemory ? ::new (pMemory) PackageLocation(&PLData) : nullptr;
                        break;
                    }
                    case PACKAGE_TYPE_USE_WEAPON: {
                        TESUseWeaponPackageData* pddata = static_cast<TESUseWeaponPackageData*>(pPackData);
                        void* pMemory = ourMalloc(sizeof(PackageLocation));
                        pddata->pTargetLocation = pMemory ? ::new (pMemory) PackageLocation(&PLData) : nullptr;
                        break;
                    }
                    default: {
                        alignas(PackageLocation) uint8_t locStorage[sizeof(PackageLocation)];
                        PackageLocation* pLoc = ::new (locStorage) PackageLocation(&PLData);
                        SetPackageSecondLocation(pLoc);
                        pLoc->~PackageLocation();
                        break;
                    }
                }
                break;
            }
            // Only a use weapon package keeps a second target, in its type data.
            case PTD2_ID: {
                PACK_TARG_FILE PTData;
                ourMemset(&PTData, 0, sizeof(PACK_TARG_FILE));
                apFile->GetChunkData(&PTData, sizeof(PACK_TARG_FILE));
                if (apFile->QEndian()) {
                    PTData.Endian();
                }
                if (GetPackType() == PACKAGE_TYPE_USE_WEAPON) {
                    TESUseWeaponPackageData* pddata = static_cast<TESUseWeaponPackageData*>(pPackData);
                    PackageTarget* pOldTarget = pddata->pAttackTarget;
                    if (pOldTarget) {
                        delete pOldTarget;
                    }
                    void* pMemory = ourMalloc(sizeof(PackageTarget));
                    pddata->pAttackTarget = pMemory ? ::new (pMemory) PackageTarget(&PTData) : nullptr;
                }
                break;
            }
            case PSDT_ID: {
                PACK_SCHED_DATA PSData;
                ourMemset(&PSData, 0, sizeof(PACK_SCHED_DATA));
                apFile->GetChunkData(&PSData, sizeof(PACK_SCHED_DATA));
                if (apFile->QEndian()) {
                    PSData.Endian();
                }
                PackageSchedule packSchedule;
                packSchedule.~PackageSchedule();
                ::new (&packSchedule) PackageSchedule(&PSData);
                SetPackageSchedule(&packSchedule);
                break;
            }
            case PTDT_ID: {
                PACK_TARG_FILE PTData;
                ourMemset(&PTData, 0, sizeof(PACK_TARG_FILE));
                apFile->GetChunkData(&PTData, sizeof(PACK_TARG_FILE));
                if (apFile->QEndian()) {
                    PTData.Endian();
                }
                alignas(PackageTarget) uint8_t packTargetStorage[sizeof(PackageTarget)];
                PackageTarget* pPackTarget = ::new (packTargetStorage) PackageTarget(&PTData);
                SetPackageTarget(pPackTarget);
                pPackTarget->~PackageTarget();
                break;
            }
            case CNAM_ID: {
                // The combat style's form ID is kept in the pointer until InitItem resolves it.
                uint32_t itempint = 0;
                apFile->GetChunkData(itempint);
                SetCombatStyle(reinterpret_cast<TESCombatStyle*>(itempint));
                break;
            }
            case CIDA_ID: GetConditions()->Load(apFile); break;
            case IDLF_ID:
            case IDLA_ID:
            case IDLC_ID:
            case IDLT_ID:
            case IDLB_ID:
            case IDLN_ID:
                if (!pIdleCollection) {
                    void* pMemory = ourMalloc(sizeof(BGSIdleCollection));
                    pIdleCollection = pMemory ? ::new (pMemory) BGSIdleCollection : nullptr;
                }
                pIdleCollection->Load(apFile);
                break;
            // The event action chunks start with a marker chunk; the action's own chunks follow it.
            case POBA_ID:
                apFile->NextChunk();
                OnBegin.Load(apFile);
                break;
            case POEA_ID:
                apFile->NextChunk();
                OnEnd.Load(apFile);
                break;
            case POCA_ID:
                apFile->NextChunk();
                OnChange.Load(apFile);
                break;
            default: break;
        }
    }
    while (apFile->NextChunk());
    return true;
}

void TESPackage::Save() {
    StartForm();
    if (TESForm::QEndianSwapOnSave()) {
        PackData.Endian();
    }
    TESForm::__AddChunkData(PKDT_ID, &PackData, 12u);
    if (TESForm::QEndianSwapOnSave()) {
        PackData.Endian();
    }
    if (pPackLoc) {
        pPackLoc->Save(PLDT_ID);
    }
    if (GetPackageSecondLocation()) {
        PackageLocation* PackageSecondLocation = GetPackageSecondLocation();
        PackageSecondLocation->Save(PLD2_ID);
    }
    if (GetPackageSchedule()) {
        GetPackageSchedule()->Save();
    }
    if (GetPackageTarget()) {
        GetPackageTarget()->Save(PTDT_ID);
    }
    // The game tests this address (&packConditions) for null. It never is null, so Save() is called unconditionally.
    (&packConditions)->Save();
    if (pIdleCollection) {
        pIdleCollection->Save();
    }
    if (pCombatStyle) {
        TESForm::AddChunk(CNAM_ID, pCombatStyle->iFormID);
    }
    if (pPackData) {
        pPackData->Save();
    }
    OnBegin.Save(POBA_ID);
    OnEnd.Save(POEA_ID);
    OnChange.Save(POCA_ID);
    CloseForm();
}

void TESPackage::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
    BGSChangeFlags* v4;
    BGSChangeFlags v9;
    if (!pDataHandler->IsFormIDCreated(TESForm::GetFormID())) {
        v4 = &(v9 = apLoadGameBuffer->GetOldChangeFlags());
        if (v4->CheckFlags(0x80000000u)) {
            PackData.iPackFlags &= (~0x8000u);
        }
        if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_PACKAGE_WAITING)) {
            PackData.iPackFlags &= ~0x10000u;
        }
    }
}

void TESPackage::Revert(uint32_t aiFlags) {
    if ((aiFlags & CHANGE_PACKAGE_NEVER_RUN) != 0) {
        PackData.iPackFlags &= ~static_cast<uint32_t>(kPackageFlag_NeverToRun);
    }
    if ((aiFlags & CHANGE_PACKAGE_WAITING) != 0) {
        PackData.iPackFlags &= ~0x10000u;
    }
}

void TESPackage::FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESForm::FinishLoadGame(apLoadGameBuffer);
}

// Resolves every form ID the package holds; a package that raised warnings on the way is never run.
void TESPackage::InitItem() {
    if (GetInitialized()) {
        return;
    }
    uint32_t uiWarnings = MessageHandler::QWarningCount();
    if (GetLocation()) {
        GetLocation()->InitItem(this);
    }
    if (GetPackageSecondLocation()) {
        GetPackageSecondLocation()->InitItem(this);
    }
    if (GetPackageTarget()) {
        GetPackageTarget()->InitItem(this);
    }
    if (GetConditions()) {
        GetConditions()->InitItem(this);
    }
    if (GetPackageIdles()) {
        GetPackageIdles()->InitItem(this);
    }
    if (pPackData) {
        pPackData->InitItem(this);
    }

    // Until now the combat style field holds the style's form ID.
    uint32_t uiCombatStyleID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(GetCombatStyle()));
    if (uiCombatStyleID) {
        TESForm::AddCompileIndex(uiCombatStyleID, GetFile(-1));
        SetCombatStyle(dynamic_cast<TESCombatStyle*>(TESForm::GetFormByNumericID(uiCombatStyleID)));
        if (!GetCombatStyle()) {
            Warning(
                "FORMS: Could not find combat style (%08X) on package '%s' (%08X).", uiCombatStyleID, GetFormEditorID(),
                GetFormID()
            );
        }
    }

    OnBegin.SetType(0);
    OnEnd.SetType(1);
    OnChange.SetType(2);
    OnBegin.InitItem(this);
    OnEnd.InitItem(this);
    OnChange.InitItem(this);

    if (uiWarnings != MessageHandler::QWarningCount()) {
        Warning(
            "MASTERFILE: Warnings were generated for package '%s' (%08). This package will be disabled in game.",
            GetFormEditorID(), GetFormID()
        );
        PackData.iPackFlags |= kPackageFlag_NeverToRun;
    }

    switch (GetPackType()) {
        case PACKAGE_TYPE_AMBUSH:
            if (GuardGetTargetIsLinkedRef()) {
                GetLocation()->SetLocType(PLT_LINKEDREF);
            }
            break;
        case PACKAGE_TYPE_SANDBOX:
            if (SandboxGetLocationIsLinkedRef()) {
                GetLocation()->SetLocType(PLT_LINKEDREF);
            }
            break;
        case PACKAGE_TYPE_PATROL:
            if (static_cast<TESPatrolPackageData*>(pPackData)->bStartingLocationAtLinkedRef) {
                GetLocation()->SetLocType(PLT_LINKEDREF);
            }
            if (!GetLocation()) {
                void* pMemory = ourMalloc(sizeof(PackageLocation));
                PackageLocation* pLinked = pMemory ? ::new (pMemory) PackageLocation : nullptr;
                pLinked->SetLocType(PLT_LINKEDREF);
                SetPackageLocation(pLinked);
                if (pLinked) {
                    delete pLinked;
                }
            }
            break;
        case PACKAGE_TYPE_GUARD:
            // The target built here is never freed, as the game does: SetPackageTarget copies it into a new one.
            if (GuardGetTargetIsLinkedRef()) {
                if (!GetPackageTarget()) {
                    void* pMemory = ourMalloc(sizeof(PackageTarget));
                    SetPackageTarget(pMemory ? ::new (pMemory) PackageTarget : nullptr);
                }
                GetPackageTarget()->SetTargType(PTT_LINKEDREF);
            }
            break;
        default: break;
    }
    SetInitialized(true);
}

void TESPackage::Copy(TESForm* apCopy) {
    TESPackage* pcopy = dynamic_cast<TESPackage*>(apCopy);
    if (pcopy) {
        CopyAllComponents(pcopy);
        SetPackageLocation(pcopy->pPackLoc);
        SetPackageTarget(pcopy->GetPackageTarget());
        SetPackageSchedule(pcopy->GetPackageSchedule());
        PackData.iPackFlags = pcopy->PackData.iPackFlags;
        PackData.iFOBehaviorFlags = pcopy->PackData.iFOBehaviorFlags;
        SetPackageConditions(&pcopy->packConditions);
        SetPackageIdles(pcopy->pIdleCollection);
        pCombatStyle = pcopy->pCombatStyle;
        PACKAGE_TYPE PackType = pcopy->GetPackType();
        SetPackType(PackType);
        PackData.iPackageSpecificFlags = pcopy->PackData.iPackageSpecificFlags;
        if (pPackData) {
            pPackData->Copy(pcopy->pPackData);
        }
        // The event actions are read through apCopy itself, not the cast's result, as the game does.
        OnBegin.Copy(&static_cast<TESPackage*>(apCopy)->OnBegin);
        OnEnd.Copy(&static_cast<TESPackage*>(apCopy)->OnEnd);
        OnChange.Copy(&static_cast<TESPackage*>(apCopy)->OnChange);
        ePROCEDURE_TYPE = pcopy->ePROCEDURE_TYPE;
    }
}

bool TESPackage::Compare(TESForm* apCompare) {
    TESPackage* pcomp = dynamic_cast<TESPackage*>(apCompare);
    if (!pcomp) {
        return true;
    }
    if (CompareAllComponents(pcomp)) {
        return true;
    }
    PACKAGE_TYPE PackType = GetPackType();
    if (PackType != pcomp->GetPackType()) {
        return true;
    }
    int32_t ShaderPropertyType = PackData.iPackFlags;
    if (ShaderPropertyType != pcomp->PackData.iPackFlags) {
        return true;
    }
    if (PackData.iFOBehaviorFlags != pcomp->PackData.iFOBehaviorFlags) {
        return true;
    }
    if (PackData.iPackageSpecificFlags != pcomp->PackData.iPackageSpecificFlags) {
        return true;
    }
    if (pIdleCollection) {
        BGSIdleCollection* Interior = pIdleCollection;
        if (Interior->CompareComponent(pcomp->pIdleCollection)) {
            return true;
        }
    }
    else {
        if (pcomp->pIdleCollection) {
            return true;
        }
    }
    if (pPackLoc) {
        if (pPackLoc->Compare(pcomp->pPackLoc)) {
            return true;
        }
    }
    else {
        if (pcomp->pPackLoc) {
            return true;
        }
    }
    if (GetPackageSecondLocation()) {
        PackageLocation* PackageSecondLocation = pcomp->GetPackageSecondLocation();
        PackageLocation* v5 = GetPackageSecondLocation();
        if (v5->Compare(PackageSecondLocation)) {
            return true;
        }
    }
    else {
        if (pcomp->GetPackageSecondLocation()) {
            return true;
        }
    }
    if (GetPackageTarget()) {
        if (GetPackageTarget()->Compare(pcomp->GetPackageTarget())) {
            return true;
        }
    }
    else {
        if (pcomp->GetPackageTarget()) {
            return true;
        }
    }
    if (GetPackageSchedule()) {
        if (GetPackageSchedule()->Compare(pcomp->GetPackageSchedule())) {
            return true;
        }
    }
    else {
        if (pcomp->GetPackageSchedule()) {
            return true;
        }
    }
    // The game tests this address (&packConditions) for null. It never is null, so Save() is called unconditionally.
    if ((&packConditions)->Compare(&pcomp->packConditions)) {
        return true;
    }
    if (pCombatStyle) {
        TESCombatStyle* CombatStyle = pCombatStyle;
        if (CombatStyle->Compare(pcomp->pCombatStyle)) {
            return true;
        }
    }
    else {
        if (pcomp->pCombatStyle) {
            return true;
        }
    }
    if (pPackData && pcomp->pPackData) {
        if (pPackData->Compare(pcomp->pPackData)) {
            return true;
        }
    }
    else {
        if (pPackData || pcomp->pPackData) {
            return true;
        }
    }
    return OnBegin.Compare(&pcomp->OnBegin) || (OnEnd.Compare(&pcomp->OnEnd) || OnChange.Compare(&pcomp->OnChange));
}

const char* TESPackage::GetObjectTypeName() {
    return const_cast<char*>(packageTypeToString[(int32_t)GetPackType()]);
}

// Travel, sandbox and patrol packages whose location is the actor's linked reference wait until he has one.
bool TESPackage::IsReadyToRun(MobileObject* apActor) {
    PACKAGE_TYPE eType = GetPackType();
    switch (eType) {
        case PACKAGE_TYPE_TRAVEL:
            return GetLocation()->GetLocType() != PLT_LINKEDREF || apActor->GetLinkedRef() != nullptr;
        case PACKAGE_TYPE_PATROL:
            return GetLocation()->GetLocType() != PLT_LINKEDREF || apActor->GetLinkedRef() != nullptr;
        case PACKAGE_TYPE_SANDBOX:
            return (!SandboxGetLocationIsLinkedRef() &&
                    (!GetLocation() || GetLocation()->GetLocType() != PLT_LINKEDREF)) ||
                apActor->GetLinkedRef() != nullptr;
        default: return true;
    }
}

// Interior cells must match exactly; otherwise the worldspaces must. Then the actor must be within the location's
// radius, which is derived from the reference when the package gives none.
bool TESPackage::IsActorAtLocation(
    MobileObject* apMobileObject,
    bool bignoredistance,
    float afExtraRadius,
    bool bInfurniture
) {
    if (!apMobileObject) {
        return false;
    }
    Actor* pActor = nullptr;
    if (apMobileObject->IsActor()) {
        pActor = static_cast<Actor*>(apMobileObject);
    }
    if ((!bInfurniture && pActor) &&
        (pActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING || pActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING)) {
        bInfurniture = true;
    }

    PackageLocation* pLocation = GetLocation();
    if (pLocation && pLocation->GetLocType() == PLT_EDITORLOC && !apMobileObject->HasEditorLocation()) {
        return true;
    }
    TESWorldSpace* pLocationWorld = GetLocationWorld(apMobileObject);
    TESObjectCELL* pLocationCell = GetLocationCell(apMobileObject);
    if (pLocationCell && !pLocationCell->GetInterior()) {
        pLocationCell = nullptr;
    }
    TESWorldSpace* pWorld = apMobileObject->GetWorldSpace();
    TESObjectCELL* pCell = apMobileObject->GetParentCell();
    if (pCell && !pCell->GetInterior()) {
        pCell = nullptr;
    }
    bool bSameSpace = false;
    if (pCell || pLocationCell) {
        if (pCell == pLocationCell) {
            bSameSpace = true;
            if (pLocation && pLocation->GetLocType() == PLT_CELL) {
                return true;
            }
        }
    }
    else if (pWorld == pLocationWorld) {
        bSameSpace = true;
    }
    if (!bSameSpace) {
        return false;
    }

    bool bInsideTrigger = false;
    if (pLocation && pLocation->GetRadius() == 0 &&
        pLocation->IsRefAtLocationTrigger(apMobileObject, apMobileObject, bInsideTrigger)) {
        return bInsideTrigger;
    }
    int32_t iRadius = static_cast<int32_t>(GetLocationRadius(apMobileObject));
    if (bignoredistance) {
        iRadius = 0;
    }
    else if (afExtraRadius != -1.0) {
        // The extra radius is truncated to an integer before being added, not added as a float.
        iRadius = static_cast<int32_t>(afExtraRadius) + iRadius;
    }
    if (iRadius > 0) {
        bInfurniture = false;
    }
    // An exact radius is not widened by IsAtPoint.
    bool bExactRadius = false;
    NiPoint3 kPos = GetLocationCoord(pActor);
    TESObjectREFR* pLocationRef = nullptr;
    if (pLocation) {
        pLocationRef = GetLocationReference(apMobileObject);
    }

    if (!iRadius && pLocation) {
        if (pLocationRef) {
            TESBoundObject* pFurniture = nullptr;
            if (pLocationRef->IsFurniture()) {
                pFurniture = pLocationRef->GetObjectReference();
            }
            if (pFurniture && apMobileObject->GetCurrentAIProcess() &&
                apMobileObject->GetCurrentAIProcess()->GetFurnitureMarker()) {
                if (apMobileObject->GetCurrentAIProcess()->GetProcessLevel() == PROCESS_TYPE_HIGH) {
                    kPos = apMobileObject->GetCurrentAIProcess()->GetFurnitureMarker()->Position;
                }
                iRadius = 20;
                bExactRadius = true;
            }
            else if (
                pLocationRef->GetObjectReference() == pXMarkerHeading || pLocationRef->GetObjectReference() == pXMarker
            ) {
                iRadius = 20;
                bExactRadius = true;
            }
            else if (pLocationRef->IsActor()) {
                if (pLocationRef->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
                    iRadius = 90;
                    bExactRadius = true;
                }
                else {
                    iRadius = static_cast<int32_t>(pLocationRef->GetModelBoundSize()) + 20;
                }
            }
            else {
                float fPadding = 20.0f;
                switch (pLocationRef->GetObjectReference()->GetFormType()) {
                    case ACTI_ID:
                    case CONT_ID:
                    case STAT_ID:
                    case SCOL_ID: fPadding = 0.0f; break;
                    default: break;
                }
                // Both terms are truncated to integers before they are added; this is not a float add.
                int32_t iBound = static_cast<int32_t>(pLocationRef->GetModelBoundSize());
                iRadius = iBound + static_cast<int32_t>(fPadding);
                if (iRadius < 0) {
                    iRadius = 0;
                }
            }
        }
        else if (pLocation->GetLocType() == PLT_EDITORLOC) {
            bExactRadius = true;
            iRadius = 10;
        }
    }
    if ((GetLocationRadius(apMobileObject) <= 20 && pLocationRef) &&
        (pLocationRef->GetObjectReference() == pXMarkerHeading || pLocationRef->GetObjectReference() == pXMarker)) {
        bExactRadius = true;
    }

    if (!iRadius) {
        iRadius = static_cast<int32_t>(iAIDistanceRadiusMinLocation.UInt());
    }

    bool bAtLocation = false;
    if (bInfurniture && pLocationRef && pLocationRef->IsFurniture() && apMobileObject->GetCurrentAIProcess() &&
        apMobileObject->GetCurrentAIProcess()->GetFurnitureMarker()) {
        // The difference is a NiPoint3 whose x and y are measured as a NiPoint2.
        NiPoint3 kOffset = kPos - pActor->GetLocationOnReference();
        if ((reinterpret_cast<const NiPoint2*>(&kOffset)->Length() < 100.0 &&
             apMobileObject->GetSitSleepState() == SIT_SLEEP_IS_SITTING) ||
            apMobileObject->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
            bAtLocation = true;
        }
    }
    else if (apMobileObject->IsAtPoint(kPos, static_cast<float>(iRadius), !bExactRadius, true)) {
        bAtLocation = true;
    }
    return bAtLocation;
}

bool TESPackage::IsActorAtSecondLocation(
    MobileObject* apMobileObject,
    MobileObject* apPackageowner,
    bool bignoredistance,
    float afOtherRadius,
    bool bInfurniture
) {
    BaseProcess* v14;
    if (!apMobileObject) {
        return false;
    }
    Actor* apActor = nullptr;
    if (apMobileObject->IsActor()) {
        apActor = static_cast<Actor*>(apMobileObject);
    }
    PackageLocation* pLoc = GetPackageSecondLocation();
    if (!pLoc) {
        return true;
    }
    if (pLoc->GetLocType() == PLT_EDITORLOC && !apPackageowner->HasEditorLocation()) {
        return true;
    }
    TESWorldSpace* plocworld = GetSecondLocationWorld(apPackageowner);
    TESObjectCELL* plocintcell = GetSecondLocationCell(apPackageowner);
    if ((plocintcell) && (!plocintcell->GetInterior())) {
        plocintcell = nullptr;
    }

    TESWorldSpace* pactorworld = apMobileObject->GetWorldSpace();
    TESObjectCELL* pactorintcell = apMobileObject->GetParentCell();
    if ((pactorintcell) && (!pactorintcell->GetInterior())) {
        pactorintcell = nullptr;
    }

    bool bsamespace = false;
    if (pactorintcell || plocintcell) {
        if (pactorintcell == plocintcell) {
            bsamespace = true;
            if (pLoc->GetLocType() == PLT_CELL) {
                return true;
            }
        }
    }
    else {
        bsamespace = pactorworld == plocworld;
    }
    if (!bsamespace) {
        return false;
    }
    bool btriggered = false;
    if (pLoc->IsRefAtLocationTrigger(apPackageowner, apMobileObject, btriggered)) {
        return btriggered;
    }
    int32_t iRadius = GetSecondLocationRadius();
    if (iRadius > 0) {
        bInfurniture = false;
    }
    if (bignoredistance) {
        iRadius = 0;
    }
    else {
        if (afOtherRadius != -1.0) {
            iRadius = static_cast<int32_t>(afOtherRadius);
        }
    }
    bool bexactpos = false;
    NiPoint3 loccoord = GetSecondLocationCoord(apPackageowner);
    TESObjectREFR* pRef = nullptr;
    if (pLoc) {
        pRef = GetSecondLocationReference(apPackageowner);
    }
    if ((!iRadius) && (pLoc)) {
        if (pRef) {
            BaseProcess* v17;
            if (pRef->IsFurniture() &&
                (apMobileObject->pCurrentProcess &&
                 (v17 = apMobileObject->pCurrentProcess, v17->GetFurnitureMarker()))) {
                if (!apPackageowner->pCurrentProcess->eLevel) {
                    BaseProcess* CurrentAIProcess = apPackageowner->pCurrentProcess;
                    loccoord = CurrentAIProcess->GetFurnitureMarker()->Position;
                }
                iRadius = 20;
                bexactpos = true;
            }
            else {
                TESBoundObject* v8;
                TESBoundObject* ObjectReference = pRef->GetObjectReference();
                if (ObjectReference == pXMarkerHeading || (v8 = pRef->GetObjectReference(), v8 == pXMarker)) {
                    iRadius = 20;
                    bexactpos = true;
                }
                else {
                    if (pRef->IsActor()) {
                        if (pRef->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
                            iRadius = 90;
                            bexactpos = true;
                        }
                        else {
                            iRadius = static_cast<int32_t>(pRef->GetModelBoundSize()) + 20;
                        }
                    }
                    else {
                        float ffudge = 20.0f;
                        switch (pRef->GetObjectReference()->GetFormType()) {
                            case ACTI_ID:
                            case CONT_ID:
                            case STAT_ID:
                            case SCOL_ID: {
                                ffudge = 0.0f;
                                break;
                            }
                            default: {
                                break;
                            }
                        }
                        iRadius = static_cast<int32_t>(ffudge) + static_cast<int32_t>(pRef->GetModelBoundSize());
                        if (iRadius < 0) {
                            iRadius = 0;
                        }
                    }
                }
            }
        }
        else {
            if (pLoc->GetLocType() == PLT_EDITORLOC) {
                bexactpos = true;
                iRadius = 10;
            }
        }
    }

    if (!iRadius) {
        iRadius = iAIDistanceRadiusMinLocation.Int();
    }
    bool bGoodPt = false;
    if ((GetPackType() == PACKAGE_TYPE_WANDER) && (pactorintcell)) {
        const NiPoint3* v9 = &apMobileObject->GetLocationOnReference();
        float v15 = v9->z - GetSecondLocationCoord(apPackageowner).z;
        if (std::fabs(v15) > 100.0) {
            return false;
        }
    }

    bool boverride = false;
    if (bInfurniture &&
        (pRef &&
         (pRef->IsFurniture() &&
          (apMobileObject->pCurrentProcess && (v14 = apMobileObject->pCurrentProcess, v14->GetFurnitureMarker()))))) {
        const NiPoint3* v10 = &apMobileObject->GetLocationOnReference();
        NiPoint3 dist = loccoord - *v10;
        if ((apActor) &&
            (apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
             apActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING)) {
            BaseProcess* v13 = apActor->pCurrentProcess;
            TESObjectREFR* v11 = v13->GetCurrentFurniture();
            if (v11 == pRef) {
                return true;
            }
        }
    }
    else {
        float v12 = static_cast<float>(iRadius);
        if (apMobileObject->IsAtPoint(loccoord, v12, !bexactpos, true) && !boverride) {
            return true;
        }
    }
    return bGoodPt;
}

bool TESPackage::IsActorAtRefTarget(MobileObject* apMobileObject, int32_t aiExtraRadius) {
    BaseProcess* v13;
    if (!apMobileObject || !apMobileObject->GetCurrentTarget()) {
        return false;
    }
    MobileObject* apActor = nullptr;
    if (apMobileObject->IsActor()) {
        apActor = apMobileObject;
    }
    bool bexactpos = false;
    TESObjectREFR* ptargref = apMobileObject->GetCurrentTarget();
    NiPoint3 targloc = ptargref->GetLocationOnReference();
    if (ptargref->GetTeleport()) {
        targloc = ptargref->GetLinkedDoorTeleportPosition();
    }
    TESWorldSpace* ptargworld = ptargref->GetWorldSpace();
    TESObjectCELL* ptargintcell = ptargref->GetParentCell();
    if ((ptargintcell) && (!ptargintcell->GetInterior())) {
        ptargintcell = nullptr;
    }

    TESWorldSpace* pactorworld = apMobileObject->GetWorldSpace();
    TESObjectCELL* pactorintcell = apMobileObject->GetParentCell();
    if ((pactorintcell) && (!pactorintcell->GetInterior())) {
        pactorintcell = nullptr;
    }

    bool bsamespace = false;
    if (pactorintcell || ptargintcell) {
        if (pactorintcell == ptargintcell) {
            bsamespace = true;
        }
    }
    else {
        bsamespace = pactorworld == ptargworld;
    }
    if (!bsamespace) {
        return false;
    }
    int32_t iRadius = static_cast<int32_t>(GetPackageRadiusActorToRefTarget(apMobileObject, false));
    bool bGoodPt = false;
    const NiPoint3* v4 = &apMobileObject->GetLocationOnReference();
    NiPoint3 dist = targloc - *v4;
    if (ptargref->IsFurniture() &&
        (apMobileObject->pCurrentProcess && (v13 = apMobileObject->pCurrentProcess, v13->GetFurnitureMarker()))) {
        if (!apMobileObject->pCurrentProcess->eLevel) {
            BaseProcess* CurrentAIProcess = apMobileObject->pCurrentProcess;
            targloc = CurrentAIProcess->GetFurnitureMarker()->Position;
        }
        iRadius = 10;
        bexactpos = true;
    }
    else {
        if (ptargref->GetObjectReference()->GetFormType() == IDLM_ID) {
            iRadius = 10;
            bexactpos = true;
        }
        else {
            if (iRadius > 20 || (!ptargref->IsActor() || ptargref == pPlayer)) {
                TESBoundObject* v6;
                TESBoundObject* ObjectReference = ptargref->GetObjectReference();
                if (ObjectReference == pXMarkerHeading || (v6 = ptargref->GetObjectReference(), v6 == pXMarker)) {
                    iRadius = 20;
                    bexactpos = true;
                }
            }
            else {
                if (ptargref->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
                    iRadius = 90;
                    bexactpos = true;
                }
                else {
                    if (apMobileObject->GetSitSleepState() == SIT_SLEEP_IS_SITTING) {
                        iRadius = 200;
                        bexactpos = true;
                    }
                    else {
                        iRadius = static_cast<int32_t>(ptargref->GetModelBoundSize()) + 20;
                    }
                }
            }
        }
    }
    iRadius += aiExtraRadius;
    double v11 = std::fabs(dist.z);
    if (fPathPointZDistanceAllowance.Float() > v11) {
        dist.z = 0.0f;
    }
    bool boverride = false;
    if (apMobileObject->IsAtPoint(targloc, static_cast<float>(iRadius), !bexactpos, false) && !boverride) {
        bGoodPt = true;
    }
    if ((ptargref) && (ptargref->IsFurniture()) && (apMobileObject->pCurrentProcess)) {
        BaseProcess* a1_4 = apMobileObject->pCurrentProcess;
        if (a1_4->GetFurnitureMarker()) {
            const NiPoint3* v8 = &apMobileObject->GetLocationOnReference();
            NiPoint3 dist_2 = targloc - *v8;
            // The length is compared as a double against 100.0.
            if ((dist_2.XYLength() < 100.0 && apActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING) ||
                apActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
                bGoodPt = true;
            }
        }
    }

    if (Interface::GetConsoleSelection() == apActor) {
        float fdist = dist.XYLength();
        if (std::isfinite(fdist)) {
            char a[308];
            std::sprintf(a, "radius %d distance %.02f", iRadius, fdist);
            InitializationFlow(a);
        }
    }
    return bGoodPt;
}

bool TESPackage::IsTargetAtLocation(MobileObject* apActor, int32_t aiExtraRadius) {
    NiPoint3 result;
    NiPoint3 v14;
    bool abRefInsideTriggerOut;
    if (!apActor || !apActor->GetCurrentTarget()) {
        return false;
    }
    TESObjectREFR* apRef = apActor->GetCurrentTarget();
    TESWorldSpace* WorldSpace = apRef->GetWorldSpace();
    TESObjectCELL* ParentCell = apRef->GetParentCell();
    if ((ParentCell) && (!ParentCell->GetInterior())) {
        ParentCell = nullptr;
    }

    TESWorldSpace* LocationWorld = GetLocationWorld(apActor);
    TESObjectCELL* LocationCell = GetLocationCell(apActor);
    if ((LocationCell) && (!LocationCell->GetInterior())) {
        LocationCell = nullptr;
    }

    bool v13 = false;
    if (LocationCell || ParentCell) {
        if (LocationCell == ParentCell) {
            v13 = true;
        }
    }
    else {
        v13 = LocationWorld == WorldSpace;
    }
    if (!v13) {
        return false;
    }
    PackageLocation* Location_ = GetLocation();
    abRefInsideTriggerOut = false;
    if (Location_ && Location_->IsRefAtLocationTrigger(apActor, apRef, abRefInsideTriggerOut)) {
        return abRefInsideTriggerOut;
    }
    int32_t LocationRadius = GetLocationRadius(apActor);
    if ((!LocationRadius) && (Location_)) {
        TESObjectREFR* LocationReference = GetLocationReference(apActor);
        if (LocationReference) {
            LocationRadius = static_cast<int32_t>(LocationReference->GetModelBoundSize());
        }
    }

    if (LocationRadius <= 0) {
        LocationRadius = iAIDistanceRadiusMinLocation.UInt();
    }
    LocationRadius += aiExtraRadius;
    bool v17 = false;
    const NiPoint3* v6 = &apRef->GetLocationOnReference();
    NiPoint3* LocationCoord = &(result = GetLocationCoord(apActor));
    v14 = *LocationCoord - *v6;
    int32_t v5 = static_cast<int32_t>(v14.XYLength());
    if (v5 <= LocationRadius) {
        return true;
    }
    return v17;
}

uint16_t TESPackage::GetSaveSize() {
    uint16_t sSize = 0;
    uint16_t sStartSize = sSize;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        sSize += 4;
        sSize += 2;
    }
    sSize += 12;
    sSize += 1;
    if (pPackLoc) {
        sSize += pPackLoc->GetSaveSize();
    }
    if (pPackTarg) {
        sSize += pPackTarg->GetSaveSize();
    }
    sSize += 4;
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* pHeader = pSaveLoadGameOLD->GetCurrentlySavingFormHeader();
        if (pHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            const char* pEditorID = pForm->GetFormEditorID();
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s", sSize - sStartSize,
                pHeader->iFormID, pEditorID, pHeader->iFlags, 7505,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Package.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s", sSize - sStartSize, 7505,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Package.cpp"
            );
        }
    }
    return sSize;
}

// With save blocks this writes the 'BLOK' marker and a 2-byte size placeholder, then patches the placeholder with the
// block's size at the end. The debug log goes through an empty printf stub.
void TESPackage::SaveGame() {
    uint16_t sBlockSize = 0;
    void* pBlockStart = nullptr;
    char* pStart = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
    if (bDebugSaveBuffer.Bool()) {
        pStart = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint32_t iBlockID = 0x424C4F4B;
        pSaveLoadGameOLD->SaveGameDataOLD(&iBlockID, 4);
        pBlockStart = pSaveLoadGameOLD->GetBufferPointer();
        pSaveLoadGameOLD->SaveGameDataOLD(&sBlockSize, 2);
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&PackData, 12);
    // Bit 0: a location follows; bit 1: a target follows.
    uint8_t cHasParts = 0;
    if (pPackLoc) {
        cHasParts |= 1;
    }
    if (pPackTarg) {
        cHasParts |= 2;
    }
    TESForm::SaveGameDataOLD(&cHasParts, 1);
    if (pPackLoc) {
        pPackLoc->SaveGame();
    }
    if (pPackTarg) {
        pPackTarg->SaveGame();
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&ePROCEDURE_TYPE, 4);
    if (bDebugSaveBuffer.Bool()) {
        char* pEnd = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
        SaveFormHeader* pHeader = pSaveLoadGameOLD->GetCurrentlySavingFormHeader();
        if (pHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            const char* pEditorID = pForm->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                static_cast<int32_t>(pEnd - pStart), pHeader->iFormID, pEditorID, pHeader->iFlags, 7534,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Package.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s", static_cast<int32_t>(pEnd - pStart), 7534,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Package.cpp"
            );
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        char* pEnd = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
        char* pBlockDataStart = static_cast<char*>(pBlockStart);
        if (pEnd > pBlockDataStart + 0xFFFF) {
            Warning(
                "SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Package.cpp", 7534
            );
        }
        *static_cast<uint16_t*>(pBlockStart) = static_cast<uint16_t>(pEnd - pBlockDataStart);
    }
}

// Reads the package from an old-format save. With save blocks it checks the 'BLOK' marker first and, at the end, that
// it read exactly the block size it was given.
void TESPackage::LoadGame() {
    uint16_t sBlockSize = 0;
    char* pBlockStart = nullptr;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint32_t iMarker;
        pSaveLoadGameOLD->LoadGameDataOLD(&iMarker, 4);
        if (iMarker != 0x424C4F4B) {
            LoadFormHeader* pHeader = pSaveLoadGameOLD->GetCurrentlyLoadingFormHeader();
            if (pHeader) {
                TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
                uint32_t iFlags = pHeader->iFlags;
                uint8_t cVersion = pHeader->cVersion;
                const char* pEditorID = pForm->GetFormEditorID();
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  "
                    "Currently loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Package.cpp", 7540, pHeader->iFormID,
                    pEditorID, cVersion, iFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  "
                    "Current version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Package.cpp", 7540,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        pBlockStart = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
        pSaveLoadGameOLD->LoadGameDataOLD(&sBlockSize, 2);
    }
    pSaveLoadGameOLD->LoadGameDataOLD(&PackData, 12);
    char cHasParts;
    TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&cHasParts), 1);
    if (static_cast<uint8_t>(cHasParts) & 1) {
        void* pMemory = ourMalloc(sizeof(PackageLocation));
        pPackLoc = pMemory ? ::new (pMemory) PackageLocation : nullptr;
        pPackLoc->LoadGame();
    }
    if (static_cast<uint8_t>(cHasParts) & 2) {
        void* pMemory = ourMalloc(sizeof(PackageTarget));
        pPackTarg = pMemory ? ::new (pMemory) PackageTarget : nullptr;
        pPackTarg->LoadGame();
    }
    pSaveLoadGameOLD->LoadGameDataOLD(&ePROCEDURE_TYPE, 4);
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        char* pBlockEnd = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
        LoadFormHeader* pHeader = pSaveLoadGameOLD->GetCurrentlyLoadingFormHeader();
        if (pHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            if (pBlockEnd > pBlockStart + sBlockSize) {
                uint32_t iFlags = pHeader->iFlags;
                uint8_t cVersion = pHeader->cVersion;
                const char* pEditorID = pForm->GetFormEditorID();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading "
                    "form is %08X %s with version %i and flags %08X",
                    static_cast<int32_t>(pBlockEnd - (pBlockStart + sBlockSize)),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Package.cpp", 7563, pHeader->iFormID,
                    pEditorID, cVersion, iFlags
                );
            }
            else if (pBlockEnd < pBlockStart + sBlockSize) {
                uint32_t iFlags = pHeader->iFlags;
                uint8_t cVersion = pHeader->cVersion;
                const char* pEditorID = pForm->GetFormEditorID();
                Warning(
                    "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading "
                    "form is %08X %s with version %i and flags %08X",
                    static_cast<int32_t>((pBlockStart + sBlockSize) - pBlockEnd),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Package.cpp", 7563, pHeader->iFormID,
                    pEditorID, cVersion, iFlags
                );
            }
        }
        else if (pBlockEnd > pBlockStart + sBlockSize) {
            uint8_t cVersion = pSaveLoadGameOLD->GetCurrentVersion();
            Warning(
                "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                static_cast<int32_t>(pBlockEnd - (pBlockStart + sBlockSize)),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Package.cpp", 7563, cVersion
            );
        }
        else if (pBlockEnd < pBlockStart + sBlockSize) {
            uint8_t cVersion = pSaveLoadGameOLD->GetCurrentVersion();
            Warning(
                "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is %i",
                static_cast<int32_t>((pBlockStart + sBlockSize) - pBlockEnd),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Package.cpp", 7563, cVersion
            );
        }
    }
}

void TESPackage::InitLoadGame() {
    if (pPackLoc) {
        pPackLoc->InitLoadGame();
    }
    if (pPackTarg) {
        pPackTarg->InitLoadGame();
    }
}

// The package's part of the buffer SaveGame slot.
void TESPackage::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    int8_t apData;
    uint32_t v2 = TESForm::GetFormID();
    if (pDataHandler->IsFormIDCreated(v2)) {
        apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&PackData)), 0xCu, 0);
        apData = 0;
        if (pPackLoc) {
            apData |= 1u;
        }
        if (pPackTarg) {
            apData |= 2u;
        }
        if (pPackData) {
            apData |= 4u;
        }
        apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&apData)), 1u, 0);
        if (pPackLoc) {
            pPackLoc->SaveGame(apSaveGameBuffer);
        }
        if (pPackTarg) {
            pPackTarg->SaveGame(apSaveGameBuffer);
        }
        if (pPackData) {
            pPackData->SaveGame(apSaveGameBuffer);
        }
        apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&ePROCEDURE_TYPE)), 4u, 0);
    }
}

// The package's part of the buffer LoadGame slot.
void TESPackage::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    uint8_t csavedDataFlags;
    uint32_t CastingType = iFormID;
    if (pDataHandler->IsFormIDCreated(CastingType)) {
        apLoadGameBuffer->LoadData(&PackData, 12u);
        csavedDataFlags = 0;
        apLoadGameBuffer->LoadData(&csavedDataFlags, 1u);
        if ((csavedDataFlags & 1) != 0) {
            PackageLocation* v4;
            PackageLocation* v7 = static_cast<PackageLocation*>(ourMalloc(sizeof(PackageLocation)));
            if (v7) {
                v4 = ::new (v7) PackageLocation;
            }
            else {
                v4 = nullptr;
            }
            pPackLoc = v4;
            pPackLoc->LoadGame(apLoadGameBuffer);
        }
        if ((csavedDataFlags & 2) != 0) {
            PackageTarget* v3;
            PackageTarget* v6 = static_cast<PackageTarget*>(ourMalloc(sizeof(PackageTarget)));
            if (v6) {
                v3 = ::new (v6) PackageTarget;
            }
            else {
                v3 = nullptr;
            }
            pPackTarg = v3;
            pPackTarg->LoadGame(apLoadGameBuffer);
        }
        if ((csavedDataFlags & 4) != 0) {
            pPackData->LoadGame(apLoadGameBuffer);
        }
        apLoadGameBuffer->LoadData(&ePROCEDURE_TYPE, 4u);
    }
    else {
        if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_PACKAGE_NEVER_RUN)) {
            PackData.iPackFlags |= kPackageFlag_NeverToRun;
        }
        if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_PACKAGE_WAITING)) {
            PackData.iPackFlags |= 0x10000;
        }
    }
}

void TESPackage::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    TESForm::LoadGame(aiFlags, aiCurrentFlags);
    if ((aiFlags & CHANGE_PACKAGE_NEVER_RUN) != 0) {
        PackData.iPackFlags |= kPackageFlag_NeverToRun;
    }
    if ((aiFlags & CHANGE_PACKAGE_WAITING) != 0) {
        PackData.iPackFlags |= 0x10000;
    }
}

void TESPackage::InitLoadGame(BGSLoadFormBuffer* apLoadFormBuffer) {
    uint32_t CastingType = GetFormID();
    if (pDataHandler->IsFormIDCreated(CastingType)) {
        if (pPackLoc) {
            pPackLoc->InitLoadGame(apLoadFormBuffer);
        }
        if (pPackTarg) {
            pPackTarg->InitLoadGame(apLoadFormBuffer);
        }
        if (pPackData) {
            pPackData->InitLoadGame(apLoadFormBuffer);
        }
    }
}

uint32_t TESPackage::GetSecondLocationRadius() {
    PackageLocation* packageSecondLocation = GetPackageSecondLocation();
    if (packageSecondLocation) {
        return packageSecondLocation->GetRadius();
    }

    return (uint32_t)0;
}

uint32_t TESPackage::IncRefCount() {
    uiRefCount = NiAtomicIncrement(uiRefCount);
    return uiRefCount;
}

void TESPackage::SetIsCreated(bool abVal) {
    if (!pDataHandler->IsFormIDCreated(iFormID)) {
        return;
    }

    if (abVal) {
        PackData.iPackFlags |= kPackageFlag_IsCreated;
    }
    else {
        PackData.iPackFlags &= ~static_cast<uint32_t>(kPackageFlag_IsCreated);
    }
}

bool TESPackage::GetIsCreated() {
    return (PackData.iPackFlags & kPackageFlag_IsCreated) != 0;
}

bool TESPackage::IsNeverToRun() {
    return (PackData.iPackFlags & kPackageFlag_NeverToRun) != 0;
}

bool TESPackage::GetMustComplete() {
    return (PackData.iPackFlags & kPackageFlag_MustComplete) != 0;
}

bool TESPackage::GetMustReachLocation() {
    return (PackData.iPackFlags & kPackageFlag_MustReachLocation) != 0;
}

uint32_t TESPackage::GetPackageFlags() {
    return PackData.iPackFlags;
}

bool TESPackage::GetShouldReserveTarget() {
    int32_t eType = (int32_t)TESPackage::GetProcedureType();
    return (eType >= 2) && ((eType <= 5) || (eType == 26));
}

bool TESPackage::GetForceUnequipWeapon() {
    return (PackData.iPackFlags & kPackageFlag_ForceUnequipWeapon) != 0;
}

bool TESPackage::GetDrawWeaponFlag() {
    return (PackData.iPackFlags & kPackageFlag_DrawWeapon) != 0;
}

void TESPackage::SetDrawWeaponFlag(bool abVal) {
    if (abVal) {
        PackData.iPackFlags |= kPackageFlag_DrawWeapon;
    }
    else {
        PackData.iPackFlags &= ~static_cast<uint32_t>(kPackageFlag_DrawWeapon);
    }
}

bool TESPackage::GetUnlockDoorsAtStart() {
    return (PackData.iPackFlags & kPackageFlag_UnlockDoorsAtStart) != 0;
}

bool TESPackage::GetLockDoorsAtStart() {
    return ((PackData.iPackFlags & 8) != 0);
}

bool TESPackage::GetLockDoorsAtEnd() {
    return (PackData.iPackFlags & kPackageFlag_LockDoorsAtEnd) != 0;
}

bool TESPackage::GetUnlockDoorsAtEnd() {
    return (PackData.iPackFlags & kPackageFlag_UnlockDoorsAtEnd) != 0;
}

bool TESPackage::GetLockDoorsAtLocation() {
    return (PackData.iPackFlags & kPackageFlag_LockDoorsAtLocation) != 0;
}

bool TESPackage::GetUnlockDoorsAtLocation() {
    return (PackData.iPackFlags & kPackageFlag_UnlockDoorsAtLocation) != 0;
}

bool TESPackage::GetOncePerDay() {
    return (PackData.iPackFlags & kPackageFlag_OncePerDay) != 0;
}

bool TESPackage::GetSelectiveFalloutBehavior() {
    return (PackData.iPackFlags & kPackageFlag_SelectiveFalloutBehavior) != 0;
}

bool TESPackage::GetAlwaysSneak() {
    return (PackData.iPackFlags & kPackageFlag_AlwaysSneak) != 0;
}

bool TESPackage::GetNoHeadTrack() {
    return (PackData.iPackFlags & kPackageFlag_NoHeadTrack) != 0;
}

bool TESPackage::GetFOEnableRandomConversations() {
    return (PackData.iFOBehaviorFlags & kFOBehaviorFlag_EnableRandomConversations) != 0;
}

bool TESPackage::IsScriptPackage() {
    return (PackData.iPackFlags & kPackageFlag_ScriptPackage) != 0;
}

// The sandbox setters below take the activity as "allowed" and clear the bit that forbids it.
void TESPackage::SandboxSetLocationIsLinkedRef(bool abVal) {
    if (abVal) {
        PackData.iPackageSpecificFlags |= kPackSpecificFlag_LocationIsLinkedRef;
    }
    else {
        PackData.iPackageSpecificFlags &= ~static_cast<uint16_t>(kPackSpecificFlag_LocationIsLinkedRef);
    }
}

void TESPackage::SandboxSetAllowEating(bool abVal) {
    if (abVal) {
        PackData.iPackageSpecificFlags &= ~static_cast<uint16_t>(kPackSpecificFlag_NoEating);
    }
    else {
        PackData.iPackageSpecificFlags |= kPackSpecificFlag_NoEating;
    }
}

void TESPackage::SandboxSetAllowSleeping(bool abVal) {
    if (abVal) {
        PackData.iPackageSpecificFlags &= ~static_cast<uint16_t>(kPackSpecificFlag_NoSleeping);
    }
    else {
        PackData.iPackageSpecificFlags |= kPackSpecificFlag_NoSleeping;
    }
}

void TESPackage::SandboxSetAllowConversation(bool abVal) {
    if (abVal) {
        PackData.iPackageSpecificFlags &= ~static_cast<uint16_t>(kPackSpecificFlag_NoConversation);
    }
    else {
        PackData.iPackageSpecificFlags |= kPackSpecificFlag_NoConversation;
    }
}

void TESPackage::SandboxSetAllowIdleMarkers(bool abVal) {
    if (abVal) {
        PackData.iPackageSpecificFlags &= ~static_cast<uint16_t>(kPackSpecificFlag_NoIdleMarkers);
    }
    else {
        PackData.iPackageSpecificFlags |= kPackSpecificFlag_NoIdleMarkers;
    }
}

void TESPackage::SandboxSetAllowFurniture(bool abVal) {
    if (abVal) {
        PackData.iPackageSpecificFlags &= ~static_cast<uint16_t>(kPackSpecificFlag_NoFurniture);
    }
    else {
        PackData.iPackageSpecificFlags |= kPackSpecificFlag_NoFurniture;
    }
}

void TESPackage::SandboxSetAllowWandering(bool abVal) {
    if (abVal) {
        PackData.iPackageSpecificFlags &= ~static_cast<uint16_t>(kPackSpecificFlag_NoWandering);
    }
    else {
        PackData.iPackageSpecificFlags |= kPackSpecificFlag_NoWandering;
    }
}

bool TESPackage::SandboxGetLocationIsLinkedRef() {
    return (PackData.iPackageSpecificFlags & kPackSpecificFlag_LocationIsLinkedRef) != 0;
}

bool TESPackage::GuardGetTargetIsLinkedRef() {
    return (PackData.iPackageSpecificFlags & kPackSpecificFlag_NoSleeping) != 0;
}

void TESPackage::SetSelectiveFalloutBehavior(bool abVal) {
    if (abVal) {
        PackData.iPackFlags |= kPackageFlag_SelectiveFalloutBehavior;
    }
    else {
        PackData.iPackFlags &= ~static_cast<uint32_t>(kPackageFlag_SelectiveFalloutBehavior);
    }
}

void TESPackage::SetCombatAlert(bool abVal) {
    if (abVal) {
        PackData.iPackFlags &= ~0x8000000u;
    }
    else {
        PackData.iPackFlags |= 0x8000000u;
    }
}

void TESPackage::SetGuardWarnBehavior(bool abVal) {
    if (abVal) {
        PackData.iPackFlags &= ~0x10000000u;
    }
    else {
        PackData.iPackFlags |= 0x10000000u;
    }
}

void TESPackage::SetFOEnableHellosToPlayer(bool abVal) {
    uint16_t iFOBehaviorFlags;
    uint16_t v3;
    if (abVal) {
        PackData.iFOBehaviorFlags |= 1u;
        iFOBehaviorFlags = PackData.iFOBehaviorFlags;
    }
    else {
        PackData.iFOBehaviorFlags &= (~1u);
        v3 = PackData.iFOBehaviorFlags;
    }
    uint16_t v4 = PackData.iFOBehaviorFlags;
}

void TESPackage::SetFOEnableAggroRadiusBehavior(bool abVal) {
    uint16_t iFOBehaviorFlags;
    uint16_t v3;
    if (abVal) {
        PackData.iFOBehaviorFlags |= 0x40u;
        iFOBehaviorFlags = PackData.iFOBehaviorFlags;
    }
    else {
        PackData.iFOBehaviorFlags &= (~0x40u);
        v3 = PackData.iFOBehaviorFlags;
    }
    uint16_t v4 = PackData.iFOBehaviorFlags;
}

const TESEscortPackageData* TESPackage::GetEscortPackageData() {
    if (((int32_t)GetPackType()) == 2) {
        return static_cast<const TESEscortPackageData*>(pPackData);
    }

    return nullptr;
}

const TESPatrolPackageData* TESPackage::GetPatrolPackageData() {
    if (((int32_t)GetPackType()) == 13) {
        return static_cast<const TESPatrolPackageData*>(pPackData);
    }

    return nullptr;
}

PACKAGE_TYPE TESPackage::GetPackType() {
    return static_cast<PACKAGE_TYPE>(PackData.cPackType);
}

const TESUseWeaponPackageData* TESPackage::GetUseWeaponPackageData() {
    if (((int32_t)GetPackType()) == 16) {
        return static_cast<TESUseWeaponPackageData*>(pPackData);
    }

    return nullptr;
}

PACKAGE_PROCEDURE_TYPE TESPackage::GetProcedureType() {
    return ePROCEDURE_TYPE;
}

void TESPackage::CalculateProcedureType(TESObjectREFR* apLinkedRef) {
    switch (GetPackType()) {
        case PACKAGE_TYPE_EXPLORE: {
            if (!GetPackageTarget()) {
                // No target: wander when the location has a radius, else travel.
                if (pPackLoc && pPackLoc->IsRadiusNonZero()) {
                    ePROCEDURE_TYPE = PACKAGE_PROCEDURE_EXPLORE_WANDER;
                }
                else {
                    ePROCEDURE_TYPE = PACKAGE_PROCEDURE_EXPLORE_TRAVEL;
                }
                goto LABEL_98;
            }
            bool bflag = false;
            if (!GetPackageTarget() ||
                (!GetPackageTarget()->GetTargReference() &&
                 (!GetPackageTarget()->GetTargObject() && GetPackageTarget()->GetTargObjectType() == POTYPE_NONE))) {
                goto LABEL_98;
            }
            PACK_TARG_TYPE etype = GetPackageTarget()->GetTargType();
            switch (etype) {
                case PTT_REFERENCE: {
                    TESObjectREFR* TargReference = GetPackageTarget()->GetTargReference();
                    switch (TargReference->GetObjectReference()->GetFormType()) {
                        case ACTI_ID:
                        case CONT_ID:
                        case DOOR_ID:
                        case STAT_ID:
                        case SCOL_ID:
                        case TREE_ID:
                        case FLOR_ID:
                        case FURN_ID:
                        case CREA_ID: {
                            bflag = true;
                            goto LABEL_30;
                        }
                        case NPC__ID: {
                            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_EXPLORE_NPC;
                            break;
                        }
                        default: {
                            goto LABEL_30;
                        }
                    }
                    break;
                }
                case PTT_OBJECT: {
                    TESForm* TargObject = GetPackageTarget()->GetTargObject();
                    switch (TargObject->GetFormType()) {
                        case ACTI_ID:
                        case DOOR_ID:
                        case STAT_ID:
                        case SCOL_ID:
                        case TREE_ID:
                        case FLOR_ID:
                        case FURN_ID:
                        case CREA_ID: {
                            bflag = true;
                            goto LABEL_30;
                        }
                        case NPC__ID: {
                            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_EXPLORE_NPC;
                            break;
                        }
                        default: {
                            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_EXPLORE_ACQUIRE_GENERIC;
                            break;
                        }
                    }
                    break;
                }
                case PTT_TYPE: {
                    switch (GetPackageTarget()->GetTargObjectType()) {
                        case POTYPE_ACTI:
                        case POTYPE_DOOR:
                        case POTYPE_FLOR:
                        case POTYPE_FURN:
                        case POTYPE_CREA: {
                            bflag = true;
                            goto LABEL_30;
                        }
                        case POTYPE_NPC: {
                            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_EXPLORE_NPC;
                            break;
                        }
                        default: {
                            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_EXPLORE_ACQUIRE_GENERIC;
                            break;
                        }
                    }
                    break;
                }
                case PTT_LINKEDREF: {
                    if (!apLinkedRef) {
                        goto LABEL_30;
                    }
                    switch (apLinkedRef->GetObjectReference()->GetFormType()) {
                        case ACTI_ID:
                        case CONT_ID:
                        case DOOR_ID:
                        case STAT_ID:
                        case SCOL_ID:
                        case TREE_ID:
                        case FLOR_ID:
                        case FURN_ID:
                        case CREA_ID: {
                            bflag = true;
                            goto LABEL_30;
                        }
                        case NPC__ID: {
                            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_EXPLORE_NPC;
                            break;
                        }
                        default: {
                            goto LABEL_30;
                        }
                    }
                    break;
                }
                default: {
                LABEL_30:;
                    if (bflag) {
                        ePROCEDURE_TYPE = PACKAGE_PROCEDURE_EXPLORE_ACTIVATE;
                    }
                    else {
                        ePROCEDURE_TYPE = PACKAGE_PROCEDURE_EXPLORE_ACQUIRE;
                    }
                    goto LABEL_98;
                }
            }
            return;
        }
        case PACKAGE_TYPE_FOLLOW: {
            if (GetPackageTarget()) {
                PackageLocation* ploc = GetPackageSecondLocation();
                if (ploc &&
                    (ploc->GetLocType() == PLT_LINKEDREF ||
                     (ploc->GetLocReference() || (ploc->GetLocObject() || ploc->GetLocObjectType())))) {
                    ePROCEDURE_TYPE = PACKAGE_PROCEDURE_AMBUSH_FOLLOW;
                }
                else {
                    ePROCEDURE_TYPE = PACKAGE_PROCEDURE_FOLLOW_WITHOUT_ESCORT;
                }
            }
            else {
                ePROCEDURE_TYPE = PACKAGE_PROCEDURE_NONE;
            }
            goto LABEL_98;
        }
        case PACKAGE_TYPE_ESCORT: {
            bool bflag_2 = false;
            if (GetPackageTarget()) {
                PACK_TARG_TYPE etype_2 = GetPackageTarget()->GetTargType();
                switch (etype_2) {
                    case PTT_REFERENCE: {
                        if ((GetPackageTarget()->GetTargReference()) &&
                            (GetPackageTarget()->GetTargReference()->GetObjectReference())) {
                            TESObjectREFR* v5;
                            TESObjectREFR* v4 = GetPackageTarget()->GetTargReference();
                            if (v4->GetObjectReference()->GetFormType() == NPC__ID ||
                                (v5 = GetPackageTarget()->GetTargReference(),
                                 v5->GetObjectReference()->GetFormType() == CREA_ID)) {
                                bflag_2 = true;
                            }
                        }

                        break;
                    }
                    case PTT_OBJECT: {
                        TESForm* v6;
                        TESForm* v7;
                        if ((GetPackageTarget()->GetTargObject() &&
                             (v6 = GetPackageTarget()->GetTargObject(), v6->GetFormType() == NPC__ID)) ||
                            (v7 = GetPackageTarget()->GetTargObject(), v7->GetFormType() == CREA_ID)) {
                            bflag_2 = true;
                        }
                        break;
                    }
                    case PTT_TYPE: {
                        if (GetPackageTarget()->GetTargObjectType() == POTYPE_NPC ||
                            GetPackageTarget()->GetTargObjectType() == POTYPE_CREA) {
                            bflag_2 = true;
                        }
                        break;
                    }
                    case PTT_LINKEDREF: {
                        if ((apLinkedRef) && (apLinkedRef->GetObjectReference()) &&
                            (apLinkedRef->GetObjectReference()->GetFormType() == NPC__ID ||
                             apLinkedRef->GetObjectReference()->GetFormType() == CREA_ID)) {
                            bflag_2 = true;
                        }

                        break;
                    }
                    default: {
                        break;
                    }
                }
                if (bflag_2) {
                    ePROCEDURE_TYPE = PACKAGE_PROCEDURE_ESCORT_ACTOR;
                }
                else {
                    ePROCEDURE_TYPE = PACKAGE_PROCEDURE_ESCORT_OBJECT;
                }
            }
            else {
                ePROCEDURE_TYPE = PACKAGE_PROCEDURE_NONE;
            }
            goto LABEL_98;
        }
        case PACKAGE_TYPE_EAT: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_EAT;
            goto LABEL_98;
        }
        case PACKAGE_TYPE_SLEEP: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_SLEEP;
            goto LABEL_98;
        }
        case PACKAGE_TYPE_WANDER: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_EXPLORE_WANDER;
            goto LABEL_98;
        }
        case PACKAGE_TYPE_TRAVEL: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_EXPLORE_TRAVEL;
            goto LABEL_98;
        }
        case PACKAGE_TYPE_ACCOMPANY: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_ACCOMPANY;
            goto LABEL_98;
        }
        case PACKAGE_TYPE_USE_ITEM_AT: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_USEITEMAT;
            goto LABEL_98;
        }
        case PACKAGE_TYPE_AMBUSH: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_AMBUSH;
            goto LABEL_98;
        }
        case PACKAGE_TYPE_FLEE_NON_COMBAT: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_FLEE_NON_COMBAT;
            goto LABEL_98;
        }
        case PACKAGE_TYPE_SANDBOX: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_SANDBOX;
            goto LABEL_98;
        }
        case PACKAGE_TYPE_PATROL: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_PATROL;
            goto LABEL_98;
        }
        case PACKAGE_TYPE_GUARD: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_GUARD;
            goto LABEL_98;
        }
        case PACKAGE_TYPE_DIALOGUE: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_DIALOGUE;
            goto LABEL_98;
        }
        case PACKAGE_TYPE_USE_WEAPON: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_USEWEAPON;
            goto LABEL_98;
        }
        case PACKAGE_TYPE_COMBAT: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_COMBAT;
            goto LABEL_98;
        }
        case PACKAGE_TYPE_ACTIVATE: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_ACTIVATE;
            goto LABEL_98;
        }
        case PACKAGE_TYPE_ALARM: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_ALARM;
            goto LABEL_98;
        }
        case PACKAGE_TYPE_FLEE: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_FLEE;
            goto LABEL_98;
        }
        case PACKAGE_TYPE_TRESPASS: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_TRESPASS;
            goto LABEL_98;
        }
        case PACKAGE_TYPE_SPECTATOR: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_OBSERVE_COMBAT;
            goto LABEL_98;
        }
        case PACKAGE_TYPE_GET_UP: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_GET_UP;
            goto LABEL_98;
        }
        case PACKAGE_TYPE_DO_NOTHING: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_DO_NOTHING;
            goto LABEL_98;
        }
        case PACKAGE_TYPE_IN_GAME_DIALOGUE: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_DIALOGUE;
            goto LABEL_98;
        }
        case PACKAGE_TYPE_SURFACE: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_SURFACE;
            goto LABEL_98;
        }
        case PACKAGE_TYPE_SEARCH_FOR_ATTACKER: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_SEARCH;
            goto LABEL_98;
        }
        case PACKAGE_TYPE_AVOID_RADIATION: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_AVOID_RADIATION;
            goto LABEL_98;
        }
        case PACKAGE_TYPE_REACT_TO_DESTROYED_OBJECT: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_EXPLOSION_REACTION;
            goto LABEL_98;
        }
        case PACKAGE_TYPE_REACT_TO_GRENADE_OR_MINE: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_GRENADE_MINE_REACTION;
            goto LABEL_98;
        }
        case PACKAGE_TYPE_STEAL_WARNING: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_STEAL_WARN;
            goto LABEL_98;
        }
        case PACKAGE_TYPE_PICKPOCKET_WARNING: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_PICKPOCKET_WARN;
            goto LABEL_98;
        }
        case PACKAGE_TYPE_MOVEMENT_BLOCKED: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_MOVEMENT_BLOCKED;
            goto LABEL_98;
        }
        default: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_NONE;
        LABEL_98:;
            if (ePROCEDURE_TYPE == PACKAGE_PROCEDURE_NONE) {
                char v10[264];
                const char* v8 = GetFormEditorID();
                std::sprintf(
                    v10, "Package '%s' is not a valid package because it is missing necessary Target Or Location Info.",
                    v8
                );
                ePROCEDURE_TYPE = PACKAGE_PROCEDURE_EXPLORE_TRAVEL;
                SetPackType(PACKAGE_TYPE_TRAVEL);
            }
            return;
        }
    }
}

// The first switch clears what the new type has no use for (asking for the old type, which is still set); the second
// creates what it needs. Each "create" asks the getter first and stores the new object directly.
void TESPackage::SetPackType(PACKAGE_TYPE aeType) {
    if (aeType == PACKAGE_TYPE_FIND) {
        aeType = PACKAGE_TYPE_EXPLORE;
    }
    if (aeType == PackData.cPackType) {
        return;
    }
    if (pPackData) {
        delete pPackData;
    }
    pPackData = nullptr;

    switch (aeType) {
        case PACKAGE_TYPE_SLEEP:
        case PACKAGE_TYPE_WANDER:
        case PACKAGE_TYPE_TRAVEL:
        case PACKAGE_TYPE_PATROL: SetPackageTarget(nullptr); break;
        case PACKAGE_TYPE_SANDBOX:
            SetPackageTarget(nullptr);
            SetPackageSecondLocation(nullptr);
            break;
        case PACKAGE_TYPE_ACCOMPANY: SetPackageLocation(nullptr); break;
        case PACKAGE_TYPE_EAT:
        case PACKAGE_TYPE_DIALOGUE: break;
        case PACKAGE_TYPE_EXPLORE:
        case PACKAGE_TYPE_FOLLOW:
        case PACKAGE_TYPE_ESCORT:
        case PACKAGE_TYPE_USE_ITEM_AT:
        case PACKAGE_TYPE_AMBUSH:
        case PACKAGE_TYPE_FLEE_NON_COMBAT:
        case PACKAGE_TYPE_CAST_MAGIC:
        case PACKAGE_TYPE_GUARD:
        case PACKAGE_TYPE_USE_WEAPON: SetPackageSecondLocation(nullptr); break;
        default: break;
    }

    void* pMemory;
    switch (aeType) {
        case PACKAGE_TYPE_EXPLORE:
        case PACKAGE_TYPE_ACCOMPANY:
        case PACKAGE_TYPE_GUARD:
        case PACKAGE_TYPE_FIND:
            if (!GetPackageTarget()) {
                pMemory = ourMalloc(sizeof(PackageTarget));
                pPackTarg = pMemory ? ::new (pMemory) PackageTarget : nullptr;
            }
            break;
        case PACKAGE_TYPE_FOLLOW:
            pMemory = ourMalloc(sizeof(TESFollowPackageData));
            pPackData = pMemory ? ::new (pMemory) TESFollowPackageData : nullptr;
            if (!GetPackageTarget()) {
                pMemory = ourMalloc(sizeof(PackageTarget));
                pPackTarg = pMemory ? ::new (pMemory) PackageTarget : nullptr;
            }
            break;
        case PACKAGE_TYPE_ESCORT:
            if (!GetPackageTarget()) {
                pMemory = ourMalloc(sizeof(PackageTarget));
                pPackTarg = pMemory ? ::new (pMemory) PackageTarget : nullptr;
            }
            if (!GetLocation()) {
                pMemory = ourMalloc(sizeof(PackageLocation));
                pPackLoc = pMemory ? ::new (pMemory) PackageLocation : nullptr;
            }
            pMemory = ourMalloc(sizeof(TESEscortPackageData));
            pPackData = pMemory ? ::new (pMemory) TESEscortPackageData : nullptr;
            break;
        case PACKAGE_TYPE_EAT:
            if (!GetPackageTarget()) {
                pMemory = ourMalloc(sizeof(PackageTarget));
                pPackTarg = pMemory ? ::new (pMemory) PackageTarget : nullptr;
            }
            if (pPackTarg->GetTargType() == PTT_REFERENCE) {
                pPackTarg->SetTargType(PTT_TYPE);
            }
            pMemory = ourMalloc(sizeof(TESEatPackageData));
            pPackData = pMemory ? ::new (pMemory) TESEatPackageData : nullptr;
            if (!GetLocation()) {
                pMemory = ourMalloc(sizeof(PackageLocation));
                pPackLoc = pMemory ? ::new (pMemory) PackageLocation : nullptr;
            }
            break;
        case PACKAGE_TYPE_SLEEP:
        case PACKAGE_TYPE_WANDER:
        case PACKAGE_TYPE_TRAVEL:
        case PACKAGE_TYPE_SANDBOX:
            if (!GetLocation()) {
                pMemory = ourMalloc(sizeof(PackageLocation));
                pPackLoc = pMemory ? ::new (pMemory) PackageLocation : nullptr;
            }
            break;
        case PACKAGE_TYPE_USE_ITEM_AT:
            pMemory = ourMalloc(sizeof(TESUseItemPackageData));
            pPackData = pMemory ? ::new (pMemory) TESUseItemPackageData : nullptr;
            if (!GetLocation()) {
                pMemory = ourMalloc(sizeof(PackageLocation));
                pPackLoc = pMemory ? ::new (pMemory) PackageLocation : nullptr;
            }
            if (!GetPackageTarget()) {
                pMemory = ourMalloc(sizeof(PackageTarget));
                pPackTarg = pMemory ? ::new (pMemory) PackageTarget : nullptr;
            }
            break;
        case PACKAGE_TYPE_AMBUSH:
            if (!GetLocation()) {
                pMemory = ourMalloc(sizeof(PackageLocation));
                pPackLoc = pMemory ? ::new (pMemory) PackageLocation : nullptr;
            }
            pMemory = ourMalloc(sizeof(TESAmbushPackageData));
            pPackData = pMemory ? ::new (pMemory) TESAmbushPackageData : nullptr;
            break;
        case PACKAGE_TYPE_PATROL:
            pMemory = ourMalloc(sizeof(TESPatrolPackageData));
            pPackData = pMemory ? ::new (pMemory) TESPatrolPackageData : nullptr;
            break;
        case PACKAGE_TYPE_DIALOGUE:
        case PACKAGE_TYPE_IN_GAME_DIALOGUE:
            if (!GetPackageTarget()) {
                pMemory = ourMalloc(sizeof(PackageTarget));
                pPackTarg = pMemory ? ::new (pMemory) PackageTarget : nullptr;
            }
            pMemory = ourMalloc(sizeof(TESDialoguePackageData));
            pPackData = pMemory ? ::new (pMemory) TESDialoguePackageData : nullptr;
            break;
        case PACKAGE_TYPE_USE_WEAPON:
            SetDrawWeaponFlag(true);
            if (!GetPackageTarget()) {
                pMemory = ourMalloc(sizeof(PackageTarget));
                pPackTarg = pMemory ? ::new (pMemory) PackageTarget : nullptr;
            }
            pMemory = ourMalloc(sizeof(TESUseWeaponPackageData));
            pPackData = pMemory ? ::new (pMemory) TESUseWeaponPackageData : nullptr;
            break;
        default: break;
    }

    PackData.iPackageSpecificFlags = 0;
    if (aeType == PACKAGE_TYPE_SANDBOX) {
        SandboxSetAllowEating(true);
        SandboxSetAllowSleeping(true);
        SandboxSetAllowConversation(true);
        SandboxSetAllowIdleMarkers(true);
        SandboxSetAllowFurniture(true);
        SandboxSetAllowWandering(true);
        SandboxSetLocationIsLinkedRef(false);
    }
    PackData.cPackType = static_cast<int8_t>(aeType);
    // An ambush package always has a second location: an empty one is copied in.
    if (aeType == PACKAGE_TYPE_AMBUSH && !GetPackageSecondLocation()) {
        pMemory = ourMalloc(sizeof(PackageLocation));
        PackageLocation* pEmpty = pMemory ? ::new (pMemory) PackageLocation : nullptr;
        SetPackageSecondLocation(pEmpty);
        if (pEmpty) {
            delete pEmpty;
        }
    }
}

void TESPackage::SetProcedureType(PACKAGE_PROCEDURE_TYPE aetype) {
    ePROCEDURE_TYPE = aetype;
}

bool TESPackage::IsInterruptPackage() {
    switch (GetPackType()) {
        case PACKAGE_TYPE_COMBAT:
        case PACKAGE_TYPE_COMBAT_LOW:
        case PACKAGE_TYPE_ACTIVATE:
        case PACKAGE_TYPE_ALARM:
        case PACKAGE_TYPE_FLEE:
        case PACKAGE_TYPE_TRESPASS:
        case PACKAGE_TYPE_SPECTATOR:
        case PACKAGE_TYPE_REACT_TO_DEAD:
        case PACKAGE_TYPE_DO_NOTHING:
        case PACKAGE_TYPE_IN_GAME_DIALOGUE:
        case PACKAGE_TYPE_SURFACE:
        case PACKAGE_TYPE_AVOID_RADIATION:
        case PACKAGE_TYPE_REACT_TO_DESTROYED_OBJECT:
        case PACKAGE_TYPE_REACT_TO_GRENADE_OR_MINE:
        case PACKAGE_TYPE_STEAL_WARNING:
        case PACKAGE_TYPE_PICKPOCKET_WARNING:
        case PACKAGE_TYPE_MOVEMENT_BLOCKED: return true;
        default: return false;
    }
}

bool TESPackage::HasActorCompletedPackage(Actor* apActor) {
    bool bComplete = false;
    if (apActor != nullptr) {
        BaseProcess* pProcess = apActor->GetCurrentAIProcess();
        if (pProcess != nullptr) {
            bComplete = pProcess->GetCurrentActionComplete();
        }
    }
    return bComplete;
}

PackageSchedule* TESPackage::GetPackageSchedule() {
    return &packSched;
}

PackageLocation* TESPackage::GetLocation() {
    return pPackLoc;
}

PackageTarget* TESPackage::GetPackageTarget() {
    return pPackTarg;
}

int32_t TESPackage::GetInitialTargetCount() {
    PackageTarget* ptarget = GetPackageTarget();
    if (!ptarget) {
        return 0;
    }
    if (ptarget->GetTargType() == PTT_REFERENCE || ptarget->GetTargType() == PTT_LINKEDREF) {
        return 1;
    }
    int32_t count = ptarget->iValue;
    if (count > 0) {
        return count;
    }
    if (ePROCEDURE_TYPE == PACKAGE_PROCEDURE_USEITEMAT) {
        return 1;
    }
    if (ePROCEDURE_TYPE == PACKAGE_PROCEDURE_EXPLORE_ACTIVATE ||
        (ePROCEDURE_TYPE == PACKAGE_PROCEDURE_EXPLORE_ACQUIRE ||
         ePROCEDURE_TYPE == PACKAGE_PROCEDURE_EXPLORE_ACQUIRE_GENERIC)) {
        return 0x7FFFFFFF;
    }
    return 0;
}

BGSIdleCollection* TESPackage::GetPackageIdles() {
    return pIdleCollection;
}

TESCombatStyle* TESPackage::GetCombatStyle() {
    return pCombatStyle;
}

void TESPackage::SetCombatStyle(TESCombatStyle* apStyle) {
    pCombatStyle = apStyle;
}

TESObjectREFR* TESPackage::ComputeInitialTarget(MobileObject* apMob) {
    TESObjectREFR* pTarget = nullptr;
    if (GetPackageTarget() == nullptr) {
        return pTarget;
    }

    const PACK_TARG_TYPE eTargType = GetPackageTarget()->GetTargType();
    if (eTargType == PTT_REFERENCE) {
        pTarget = GetPackageTarget()->GetTargReference();
    }
    else if (eTargType == PTT_LINKEDREF) {
        pTarget = apMob->GetLinkedRef();
    }

    return pTarget;
}

Script* TESPackage::GetOnBeginScript() {
    return OnBegin.pResultScript;
}

TESTopic* TESPackage::GetOnBeginTopicInfo() {
    return OnBegin.pTopic;
}

TESIdleForm* TESPackage::GetOnBeginIdle() {
    return OnBegin.QIdle();
}

// The default schedule is a temporary.
void TESPackage::SetPackageSchedule(PackageSchedule* apPackSched) {
    if (apPackSched) {
        packSched.Copy(apPackSched);
    }
    else {
        PackageSchedule kDefault;
        kDefault.~PackageSchedule();
        ::new (&kDefault) PackageSchedule;
        packSched.Copy(&kDefault);
    }
}

void TESPackage::SetPackageLocation(PackageLocation* apPackLoc) {
    if (apPackLoc) {
        if (!pPackLoc) {
            void* pMemory = ourMalloc(sizeof(PackageLocation));
            pPackLoc = pMemory ? ::new (pMemory) PackageLocation : nullptr;
        }
        pPackLoc->Copy(apPackLoc, false);
    }
    else {
        PackageLocation* pOld = pPackLoc;
        if (pOld) {
            delete pOld;
        }
        pPackLoc = nullptr;
    }
}

// The second location lives in the type data, which is created first when the package has none.
void TESPackage::SetPackageSecondLocation(PackageLocation* apPackLoc) {
    PackageLocation** ppSecondLoc = nullptr;
    void* pMemory;
    if (GetPackType() == PACKAGE_TYPE_DIALOGUE || GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
        TESDialoguePackageData* pData;
        if (!pPackData) {
            pMemory = ourMalloc(sizeof(TESDialoguePackageData));
            pData = pMemory ? ::new (pMemory) TESDialoguePackageData : nullptr;
            pPackData = pData;
        }
        else {
            pData = static_cast<TESDialoguePackageData*>(pPackData);
        }
        ppSecondLoc = &pData->pTargetLocation;
    }
    else if (GetPackType() == PACKAGE_TYPE_EAT) {
        TESEatPackageData* pData;
        if (!pPackData) {
            pMemory = ourMalloc(sizeof(TESEatPackageData));
            pData = pMemory ? ::new (pMemory) TESEatPackageData : nullptr;
            pPackData = pData;
        }
        else {
            pData = static_cast<TESEatPackageData*>(pPackData);
        }
        ppSecondLoc = &pData->pSearchLocation;
    }
    else if (GetPackType() == PACKAGE_TYPE_ESCORT) {
        TESEscortPackageData* pData;
        if (!pPackData) {
            pMemory = ourMalloc(sizeof(TESEscortPackageData));
            pData = pMemory ? ::new (pMemory) TESEscortPackageData : nullptr;
            pPackData = pData;
        }
        else {
            pData = static_cast<TESEscortPackageData*>(pPackData);
        }
        ppSecondLoc = &pData->pSearchLocation;
    }
    else if (GetPackType() == PACKAGE_TYPE_AMBUSH) {
        TESAmbushPackageData* pData;
        if (!pPackData) {
            pMemory = ourMalloc(sizeof(TESAmbushPackageData));
            pData = pMemory ? ::new (pMemory) TESAmbushPackageData : nullptr;
            pPackData = pData;
        }
        else {
            pData = static_cast<TESAmbushPackageData*>(pPackData);
        }
        ppSecondLoc = &pData->pAmbushLocation;
    }
    else if (GetPackType() == PACKAGE_TYPE_FOLLOW) {
        TESFollowPackageData* pData;
        if (!pPackData) {
            pMemory = ourMalloc(sizeof(TESFollowPackageData));
            pData = pMemory ? ::new (pMemory) TESFollowPackageData : nullptr;
            pPackData = pData;
        }
        else {
            pData = static_cast<TESFollowPackageData*>(pPackData);
        }
        ppSecondLoc = &pData->pStartLocation;
    }
    else if (GetPackType() == PACKAGE_TYPE_USE_ITEM_AT) {
        TESUseItemPackageData* pData;
        if (!pPackData) {
            pMemory = ourMalloc(sizeof(TESUseItemPackageData));
            pData = pMemory ? ::new (pMemory) TESUseItemPackageData : nullptr;
            pPackData = pData;
        }
        else {
            pData = static_cast<TESUseItemPackageData*>(pPackData);
        }
        ppSecondLoc = &pData->pSearchLocation;
    }
    else if (GetPackType() == PACKAGE_TYPE_USE_WEAPON) {
        ppSecondLoc = &GetOrCreateUseWeaponPackageData()->pTargetLocation;
    }
    if (!ppSecondLoc) {
        return;
    }

    if (apPackLoc) {
        if (!*ppSecondLoc) {
            pMemory = ourMalloc(sizeof(PackageLocation));
            *ppSecondLoc = pMemory ? ::new (pMemory) PackageLocation : nullptr;
        }
        (*ppSecondLoc)->Copy(apPackLoc, false);
    }
    else {
        PackageLocation* pOld = *ppSecondLoc;
        if (pOld) {
            delete pOld;
        }
        *ppSecondLoc = nullptr;
    }
}

void TESPackage::SetPackageTarget(PackageTarget* apPackTarg) {
    if (apPackTarg) {
        if (!pPackTarg) {
            void* pMemory = ourMalloc(sizeof(PackageTarget));
            pPackTarg = pMemory ? ::new (pMemory) PackageTarget : nullptr;
        }
        pPackTarg->Copy(apPackTarg);
    }
    else {
        PackageTarget* pOld = pPackTarg;
        if (pOld) {
            delete pOld;
        }
        pPackTarg = nullptr;
    }
}

// Each test reloads the type data and asks for the type again, as the game does.
PackageLocation* TESPackage::GetPackageSecondLocation() {
    if (pPackData && (GetPackType() == PACKAGE_TYPE_DIALOGUE || GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE)) {
        return static_cast<TESDialoguePackageData*>(pPackData)->pTargetLocation;
    }
    if (pPackData && GetPackType() == PACKAGE_TYPE_EAT) {
        return static_cast<TESEatPackageData*>(pPackData)->pSearchLocation;
    }
    if (pPackData && GetPackType() == PACKAGE_TYPE_ESCORT) {
        return static_cast<TESEscortPackageData*>(pPackData)->pSearchLocation;
    }
    if (pPackData && GetPackType() == PACKAGE_TYPE_AMBUSH) {
        return static_cast<TESAmbushPackageData*>(pPackData)->pAmbushLocation;
    }
    if (pPackData && GetPackType() == PACKAGE_TYPE_USE_ITEM_AT) {
        return static_cast<TESUseItemPackageData*>(pPackData)->pSearchLocation;
    }
    if (pPackData && GetPackType() == PACKAGE_TYPE_FOLLOW) {
        return static_cast<TESFollowPackageData*>(pPackData)->pStartLocation;
    }
    if (pPackData && GetPackType() == PACKAGE_TYPE_USE_WEAPON) {
        return static_cast<TESUseWeaponPackageData*>(pPackData)->pTargetLocation;
    }
    return nullptr;
}

PackageLocation* TESPackage::GetPackageSearchLocation() {
    PackageLocation* result;
    switch (GetPackType()) {
        case PACKAGE_TYPE_EXPLORE:
        case PACKAGE_TYPE_SANDBOX:
        case PACKAGE_TYPE_FIND:
        case PACKAGE_TYPE_IN_GAME_DIALOGUE: {
            goto LABEL_2;
        }
        case PACKAGE_TYPE_ESCORT:
        case PACKAGE_TYPE_EAT:
        case PACKAGE_TYPE_USE_ITEM_AT:
        case PACKAGE_TYPE_USE_WEAPON: {
            PackageLocation* PackageSecondLocation;
            PackageSecondLocation = GetPackageSecondLocation();
            if (PackageSecondLocation && PackageSecondLocation->GetLocType() == PLT_FIRSTLOC) {
            LABEL_2:;
                result = GetLocation();
            }
            else {
                result = GetPackageSecondLocation();
            }
            break;
        }
        default: {
            result = nullptr;
            break;
        }
    }
    return result;
}

TESUseWeaponPackageData* TESPackage::GetOrCreateUseWeaponPackageData() {
    if (((int32_t)GetPackType()) != 16) {
        fr2Assert(const_cast<char*>("D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Package.cpp"), 1543);
    }
    if (pPackData) {
        return static_cast<TESUseWeaponPackageData*>(pPackData);
    }
    TESUseWeaponPackageData* v4 = static_cast<TESUseWeaponPackageData*>(ourMalloc(sizeof(TESUseWeaponPackageData)));
    if (v4) {
        v4 = ::new (v4) TESUseWeaponPackageData;
    }
    else {
        v4 = nullptr;
    }
    pPackData = v4;
    return v4;
}

TESWorldSpace* TESPackage::GetLocationWorld(MobileObject* apActor) {
    TESWorldSpace* pWorld = nullptr;
    PackageLocation* pLocation = GetLocation();
    if (pLocation && pLocation->GetLocType() != PLT_CURRLOC) {
        switch (pLocation->GetLocType()) {
            case PLT_REFERENCE:
                if (pLocation->GetLocReference()) {
                    pWorld = pLocation->GetLocReference()->GetWorldSpace();
                }
                else if (apActor) {
                    pWorld = apActor->GetWorldSpace();
                }
                break;
            case PLT_CELL:
                if (pLocation->GetLocCell()) {
                    pWorld = pLocation->GetLocCell()->GetWorldSpace();
                }
                break;
            case PLT_EDITORLOC:
                if (apActor) {
                    pWorld = apActor->GetEditorLocationWorld();
                }
                break;
            case PLT_OBJECT:
            case PLT_TYPE:
                if (apActor && apActor->GetCurrentAIProcess() &&
                    apActor->GetCurrentAIProcess()->GetPackageThatIsRunning() == this) {
                    TESObjectREFR* pGeneric = apActor->GetCurrentAIProcess()->GetGenericLocation();
                    if (pGeneric) {
                        pWorld = pGeneric->GetWorldSpace();
                    }
                    else {
                        pWorld = apActor->GetWorldSpace();
                    }
                }
                break;
            case PLT_LINKEDREF:
                if (apActor && apActor->GetLinkedRef()) {
                    pWorld = apActor->GetLinkedRef()->GetWorldSpace();
                }
                else if (apActor) {
                    pWorld = apActor->GetWorldSpace();
                }
                break;
            default: break;
        }
    }
    else if (apActor) {
        pWorld = static_cast<TESWorldSpace*>(apActor->GetPackageStartLocationWorld());
        if (!pWorld) {
            apActor->SetPackageStartLocationFromCurrentLocation();
            pWorld = apActor->GetWorldSpace();
        }
    }
    return pWorld;
}

TESObjectCELL* TESPackage::GetLocationCell(MobileObject* apActor) {
    TESObjectCELL* pCell = nullptr;
    PackageLocation* pLocation = GetLocation();
    if (pLocation && pLocation->GetLocType() != PLT_CURRLOC) {
        switch (pLocation->GetLocType()) {
            case PLT_REFERENCE:
                if (pLocation->GetLocReference()) {
                    pCell = pLocation->GetLocReference()->GetParentCell();
                }
                else if (apActor) {
                    pCell = apActor->GetParentCell();
                }
                break;
            case PLT_CELL: pCell = pLocation->GetLocCell(); break;
            case PLT_EDITORLOC:
                if (apActor) {
                    pCell = apActor->GetEditorLocationInteriorCell();
                }
                break;
            case PLT_OBJECT:
            case PLT_TYPE:
                if (apActor && apActor->GetCurrentAIProcess() &&
                    apActor->GetCurrentAIProcess()->GetPackageThatIsRunning() == this) {
                    TESObjectREFR* pGeneric = apActor->GetCurrentAIProcess()->GetGenericLocation();
                    if (pGeneric) {
                        pCell = pGeneric->GetParentCell();
                    }
                    else {
                        pCell = apActor->GetParentCell();
                    }
                }
                break;
            case PLT_LINKEDREF:
                if (apActor && apActor->GetLinkedRef()) {
                    pCell = apActor->GetLinkedRef()->GetParentCell();
                }
                else if (apActor) {
                    pCell = apActor->GetParentCell();
                }
                break;
            default: break;
        }
    }
    else if (apActor) {
        pCell = static_cast<TESObjectCELL*>(apActor->GetPackageStartLocationInteriorCell());
        if (!pCell) {
            apActor->SetPackageStartLocationFromCurrentLocation();
            pCell = apActor->GetParentCell();
        }
    }
    return pCell;
}

// An exterior cell with no reference in it stands for its south-west corner (4096 units a cell).
NiPoint3 TESPackage::GetLocationCoord(MobileObject* apActor) {
    PackageLocation* pLocation = GetLocation();
    NiPoint3 kCoord = NiPoint3_ZERO;
    if (pLocation && pLocation->GetLocType() != PLT_CURRLOC) {
        switch (pLocation->GetLocType()) {
            case PLT_CELL: {
                TESObjectCELL* pCell = pLocation->GetLocCell();
                if (pCell) {
                    TESObjectREFR* pFirst = pCell->GetFirstRefr();
                    if (pFirst) {
                        kCoord = pFirst->GetLocationOnReference();
                    }
                    else if (!pCell->GetInterior()) {
                        kCoord.x = static_cast<float>(pLocation->GetLocCell()->GetDataX() << 12);
                        kCoord.y = static_cast<float>(pLocation->GetLocCell()->GetDataY() << 12);
                        kCoord.z = 0.0f;
                    }
                }
                break;
            }
            case PLT_REFERENCE:
                if (pLocation->GetLocReference()) {
                    kCoord = pLocation->GetLocReference()->GetLocationOnReference();
                }
                else if (apActor) {
                    Warning(
                        "AI: %s is assigned a reference location that doesnt exist for a package\n",
                        apActor->GetFormEditorID()
                    );
                    kCoord = apActor->GetStartingLocation();
                }
                break;
            case PLT_LINKEDREF:
                if (apActor && apActor->GetLinkedRef()) {
                    kCoord = apActor->GetLinkedRef()->GetLocationOnReference();
                }
                else if (apActor) {
                    Warning(
                        "AI: %s is assigned a linked reference and has none for a package\n", apActor->GetFormEditorID()
                    );
                    kCoord = apActor->GetStartingLocation();
                }
                break;
            case PLT_EDITORLOC:
                if (apActor) {
                    kCoord = apActor->GetStartingLocation();
                }
                break;
            case PLT_OBJECT:
            case PLT_TYPE:
                if (apActor && apActor->GetCurrentAIProcess() &&
                    apActor->GetCurrentAIProcess()->GetPackageThatIsRunning() == this) {
                    TESObjectREFR* pGeneric = apActor->GetCurrentAIProcess()->GetGenericLocation();
                    if (pGeneric) {
                        kCoord = pGeneric->GetLocationOnReference();
                    }
                    else {
                        kCoord = apActor->GetLocationOnReference();
                    }
                }
                break;
            default: break;
        }
    }
    else {
        if (apActor && !apActor->GetExtra()->GetPackageStartLocation()) {
            apActor->SetPackageStartLocationFromCurrentLocation();
        }
        if (apActor) {
            kCoord = apActor->GetPackageStartLocationCoord();
        }
    }
    return static_cast<const NiPoint3&>(kCoord);
}

TESObjectREFR* TESPackage::GetLocationReference(MobileObject* apActor) {
    PackageLocation* pLocation = GetLocation();
    TESObjectREFR* pRef = nullptr;
    if (pLocation && pLocation->GetLocType() != PLT_CURRLOC) {
        switch (pLocation->GetLocType()) {
            case PLT_REFERENCE:
                if (pLocation->GetLocReference()) {
                    pRef = pLocation->GetLocReference();
                }
                break;
            case PLT_LINKEDREF:
                if (apActor && apActor->GetLinkedRef()) {
                    pRef = apActor->GetLinkedRef();
                }
                break;
            case PLT_EDITORLOC:
                if (apActor) {
                    pRef = apActor;
                }
                break;
            case PLT_OBJECT:
            case PLT_TYPE:
                if (apActor && apActor->GetCurrentAIProcess() &&
                    apActor->GetCurrentAIProcess()->GetPackageThatIsRunning() == this) {
                    TESObjectREFR* pGeneric = apActor->GetCurrentAIProcess()->GetGenericLocation();
                    if (pGeneric) {
                        pRef = pGeneric;
                    }
                    else {
                        pRef = apActor;
                    }
                }
                break;
            default: break;
        }
    }
    else if (apActor) {
        pRef = apActor;
    }
    return pRef;
}

// An activator location with no radius uses half the smallest side of the activator's bound box.
uint32_t TESPackage::GetLocationRadius(MobileObject* apActor) {
    PackageLocation* pLocation = GetLocation();
    if (!pLocation) {
        return 0;
    }
    TESObjectREFR* pRef = pLocation->GetRunningPackageLocReference(apActor);
    if (pRef) {
        TESBoundObject* pObject = pRef->GetObjectReference();
        if (pObject->GetFormType() == ACTI_ID && pLocation->GetRadius() == 0) {
            NiPoint3 kSize = pObject->GetBoundMax();
            kSize -= pObject->GetBoundMin();
            float fRadius = BSMIN(BSMIN(kSize.x, kSize.y), kSize.z);
            fRadius = fRadius * 0.5;
            return static_cast<uint32_t>(static_cast<int64_t>(fRadius));
        }
    }
    return pLocation->GetRadius();
}

double TESPackage::GetPackageRadiusActorToRefTarget(MobileObject* apMobileObject, bool abSecondLoc) {
    PackageLocation* ploc;
    int32_t iRadius = 0;
    if (abSecondLoc) {
        ploc = GetPackageSecondLocation();
    }
    else {
        ploc = pPackLoc;
    }
    Actor* apActor = nullptr;
    if (apMobileObject->IsActor()) {
        apActor = static_cast<Actor*>(apMobileObject);
    }
    TESObjectREFR* ptargref = apMobileObject->GetCurrentTarget();
    if (ptargref) {
        if (ptargref->IsFurniture()) {
            iRadius = 10;
        }
        else {
            TESBoundObject* v4;
            TESBoundObject* ObjectReference = ptargref->GetObjectReference();
            if (ObjectReference == pXMarkerHeading || (v4 = ptargref->GetObjectReference(), v4 == pXMarker)) {
                iRadius = 20;
            }
            else {
                // apActor is null when the object is not an actor; GetExtra is called on it anyway, as the game does.
                // The call stays out of line so clang cannot fold the null case into apMobileObject.
                TESPackage* pedipack = apActor->GetExtra()->GetPackageExtra();
                if (GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE && (pedipack && pedipack->GetPackageTarget())) {
                    iRadius = pedipack->GetPackageTarget()->iValue;
                }
                else {
                    if ((GetPackageTarget()) && (ePROCEDURE_TYPE != PACKAGE_PROCEDURE_EXPLORE_ACQUIRE_GENERIC)) {
                        iRadius = GetPackageTarget()->iValue;
                    }
                }
                if (!iRadius && !ptargref->IsActor()) {
                    iRadius = static_cast<int32_t>(ptargref->GetModelBoundSize());
                    switch (ptargref->GetObjectReference()->GetFormType()) {
                        case ACTI_ID:
                        case CONT_ID:
                        case STAT_ID:
                        case SCOL_ID: {
                            break;
                        }
                        default: {
                            if (apActor) {
                                float freach = apActor->GetReach();
                                iRadius += static_cast<int32_t>(freach);
                                double EyeLevel = apActor->GetEyeLevel();
                                float factz = (&apActor->GetLocationOnReference())->z + EyeLevel - freach;
                                float fitemz = (&ptargref->GetLocationOnReference())->z;
                                if (fitemz < static_cast<double>(factz)) {
                                    double v7 = factz - fitemz;
                                    if (apMobileObject->GetBoundMax().z / 2.0 > v7) {
                                        int32_t v5 = static_cast<int32_t>(apMobileObject->GetBoundMax().z / 2.0);
                                        iRadius += v5;
                                    }
                                }
                            }
                            break;
                        }
                    }
                }
            }
        }
    }
    if (!iRadius) {
        return static_cast<double>(iAIDistanceRadiusMinLocation.Int());
    }
    return static_cast<double>(iRadius);
}

void TESPackage::SetNeverRun(TESObjectREFR* pref, bool abVal) {
    if ((GetPackageTarget()) && (GetPackageTarget()->GetTargReference() == pref)) {
        if (abVal) {
            PackData.iPackFlags |= kPackageFlag_NeverToRun;
            uint32_t CastingType = iFormID;
            if (!pDataHandler->IsFormIDCreated(CastingType)) {
                AddChange(CHANGE_PACKAGE_NEVER_RUN);
            }
        }
        else {
            PackData.iPackFlags &= ~static_cast<uint32_t>(kPackageFlag_NeverToRun);
            uint32_t v4 = iFormID;
            if (!pDataHandler->IsFormIDCreated(v4)) {
                RemoveChange(CHANGE_PACKAGE_NEVER_RUN);
            }
        }
    }
}

TESObjectCELL* TESPackage::GetSecondLocationCell(MobileObject* apActor) {
    TESObjectCELL* pRetCell = nullptr;
    PackageLocation* pLoc = GetPackageSecondLocation();
    if (!pLoc || pLoc->GetLocType() == PLT_CURRLOC) {
        if (apActor) {
            return apActor->GetPackageStartLocationInteriorCell();
        }
    }
    else {
        switch (pLoc->GetLocType()) {
            case PLT_REFERENCE: {
                if (pLoc->GetLocReference()) {
                    pRetCell = pLoc->GetLocReference()->GetParentCell();
                }
                break;
            }
            case PLT_CELL: {
                pRetCell = pLoc->GetLocCell();
                break;
            }
            case PLT_EDITORLOC: {
                if (apActor) {
                    pRetCell = apActor->GetEditorLocationInteriorCell();
                }
                break;
            }
            case PLT_OBJECT:
            case PLT_TYPE: {
                BaseProcess* v5;
                if (apActor &&
                    (apActor->pCurrentProcess &&
                     (v5 = apActor->pCurrentProcess, v5->GetPackageThatIsRunning() == this))) {
                    TESObjectCELL* ParentCell;
                    BaseProcess* CurrentAIProcess = apActor->pCurrentProcess;
                    TESObjectREFR* pref = CurrentAIProcess->GetGenericLocation();
                    if (pref) {
                        ParentCell = pref->GetParentCell();
                    }
                    else {
                        ParentCell = apActor->GetParentCell();
                    }
                    pRetCell = ParentCell;
                }
                else {
                    if (apActor == pPlayer && GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
                        DialoguePackage* pDialogue = static_cast<DialoguePackage*>(this);
                        if (pDialogue->pTargetLocation) {
                            pRetCell = pDialogue->pTargetLocation->GetParentCell();
                        }
                    }
                }
                break;
            }
            case PLT_LINKEDREF: {
                if ((apActor) && (apActor->GetLinkedRef())) {
                    pRetCell = apActor->GetLinkedRef()->GetParentCell();
                }

                break;
            }
            default: {
                return pRetCell;
            }
        }
    }
    return pRetCell;
}

TESObjectCELL* TESPackage::GetSearchLocationCell(MobileObject* apActor) {
    if (GetPackageSearchLocation() == GetLocation()) {
        return GetLocationCell(apActor);
    }
    return GetSecondLocationCell(apActor);
}

bool TESPackage::GetAcquireRadius(MobileObject* apActor, float& afResult) {
    afResult = 0.0f;
    if (GetPackType() == PACKAGE_TYPE_SANDBOX) {
        afResult = (float)GetLocationRadius(apActor);
        return true;
    }
    PackageTarget* pTarget = GetPackageTarget();
    if (!pTarget) {
        return false;
    }
    float fRadius = pTarget->GetTargAcquireRadius();
    // A NaN radius is taken as found, as the game does.
    if (!(fRadius <= 0.0)) {
        afResult = fRadius;
        return true;
    }
    return false;
}

// Reads the location's radius twice, as the game does.
double TESPackage::GetPackageRadiusActorToLocation(MobileObject* apMobileObject, bool abSecondLoc) {
    PackageLocation* pLocation = abSecondLoc ? GetPackageSecondLocation() : GetLocation();
    int32_t iRadius = 0;
    if (pLocation) {
        iRadius = pLocation->GetRadius();
        iRadius = pLocation->GetRadius();
        if (iRadius == 0 && pLocation) {
            TESObjectREFR* pRef = pLocation->GetRunningPackageLocReference(apMobileObject);
            if (pRef) {
                if (pRef->IsFurniture()) {
                    iRadius = 10;
                }
                else if (pRef->IsActor()) {
                    if (pRef->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
                        iRadius = 90;
                    }
                    else {
                        iRadius = static_cast<int32_t>(pRef->GetModelBoundSize()) + 20;
                    }
                }
                else if (pRef->GetObjectReference() == pXMarkerHeading || pRef->GetObjectReference() == pXMarker) {
                    iRadius = 20;
                }
                else {
                    iRadius = static_cast<int32_t>(pRef->GetModelBoundSize()) + 20;
                }
                if (iRadius < 0) {
                    iRadius = 0;
                }
            }
            else if (pLocation->GetLocType() == PLT_EDITORLOC) {
                iRadius = 10;
            }
        }
        if (iRadius == 0) {
            iRadius = static_cast<int32_t>(iAIDistanceRadiusMinLocation.UInt());
        }
    }
    return iRadius;
}

bool TESPackage::IsTargetAnActor(MobileObject* apActor) {
    bool bResult = false;
    PackageTarget* pTarget = GetPackageTarget();
    if (!pTarget) {
        return false;
    }
    switch (pTarget->GetTargType()) {
        case PTT_REFERENCE:
            if (pTarget->GetTargReference() && pTarget->GetTargReference()->IsActor()) {
                bResult = true;
            }
            break;
        case PTT_LINKEDREF:
            if (apActor->GetLinkedRef()) {
                bResult = apActor->GetLinkedRef()->IsActor();
            }
            break;
        case PTT_OBJECT: {
            uint8_t cFormType = pTarget->GetTargObject()->GetFormType();
            if (cFormType == NPC__ID || cFormType == CREA_ID) {
                bResult = true;
            }
            break;
        }
        case PTT_TYPE: {
            PACKAGE_OBJECT_TYPE eObjectType = pTarget->GetTargObjectType();
            if (eObjectType >= POTYPE_NPC && eObjectType <= POTYPE_CREA) {
                bResult = true;
            }
            break;
        }
        default: break;
    }
    return bResult;
}

// KEEP (unregistered): uninitialised read. For a type target the escort context's pObject is never written, and for
// an object target it is only written when the object is a bound object; EvaluateEscortReference (0x6784F0) reads
// it and calls its IsBoundObject regardless, so the exe passes whatever its stack slot held.
TESObjectREFR* TESPackage::GetEscortTargetForPackage(Actor* apActor, bool abSkipExistingFollowers) {
    if (!GetPackageTarget()) {
        return nullptr;
    }
    PACK_TARG_TYPE etype = GetPackageTarget()->GetTargType();
    TESObjectREFR* pTarget = nullptr;
    switch (etype) {
        case PTT_REFERENCE:
            pTarget = GetPackageTarget()->GetTargReference();
            if (pTarget && pTarget->GetDelete() && pTarget->GetExtra()) {
                pTarget = pTarget->GetExtra()->GetReferencePointer();
            }
            break;
        case PTT_OBJECT: {
            TESEvaluateEscortContext escortContext;
            escortContext.pRequester = apActor;
            escortContext.bSkipExistingFollowers = abSkipExistingFollowers;
            TESObject* pobject = nullptr;
            if (GetPackageTarget()->GetTargObject() && GetPackageTarget()->GetTargObject()->IsBoundObject()) {
                pobject = static_cast<TESObject*>(GetPackageTarget()->GetTargObject());
                escortContext.pObject = pobject;
            }
            if (pPlayer && pobject == pPlayer->GetObjectReference()) {
                pTarget = pPlayer;
            }
            else {
                // The arguments are evaluated right to left, as the game does.
                float afRadius2 = fTargetSearchRadius.Float();
                const NiPoint3* pPoint2 = &apActor->GetLocationOnReference();
                float afRadius1 = fTargetSearchRadius.Float();
                const NiPoint3* pPoint1 = &apActor->GetLocationOnReference();
                TESObjectCELL* pCell = apActor->GetParentCell();
                pDataHandler->EnumReferencesCloseToPoint(
                    pCell, *pPoint1, afRadius1, *pPoint2, afRadius2, &TESPackage::EvaluateEscortReference,
                    &escortContext
                );
                pTarget = pTempTarget;
                pTempTarget = nullptr;
            }
            break;
        }
        case PTT_TYPE: {
            TESEvaluateEscortContext escortContext;
            escortContext.pRequester = apActor;
            escortContext.bSkipExistingFollowers = abSkipExistingFollowers;
            // Read and never used.
            PACKAGE_OBJECT_TYPE etargetObjectType = GetPackageTarget()->GetTargObjectType();
            (void)etargetObjectType;
            float afRadius2 = fTargetSearchRadius.Float();
            const NiPoint3* pPoint2 = &apActor->GetLocationOnReference();
            float afRadius1 = fTargetSearchRadius.Float();
            const NiPoint3* pPoint1 = &apActor->GetLocationOnReference();
            TESObjectCELL* pCell = apActor->GetParentCell();
            pDataHandler->EnumReferencesCloseToPoint(
                pCell, *pPoint1, afRadius1, *pPoint2, afRadius2, &TESPackage::EvaluateEscortReference, &escortContext
            );
            pTarget = pTempTarget;
            pTempTarget = nullptr;
            break;
        }
        case PTT_LINKEDREF:
            pTarget = apActor->GetLinkedRef();
            if (pTarget && pTarget->GetDelete() && pTarget->GetExtra()) {
                pTarget = pTarget->GetExtra()->GetReferencePointer();
            }
            break;
        default: break;
    }
    if (!pTarget || !pTarget->IsActor() || GetIsCreated()) {
        return pTarget;
    }
    Actor* pact = static_cast<Actor*>(pTarget);
    if (GetPackType() == PACKAGE_TYPE_FOLLOW || GetPackType() == PACKAGE_TYPE_ACCOMPANY) {
        // The player takes a new follower only while he has room for one, unless he commands the actor already.
        if (pTarget != pPlayer || apActor->GetCurrentAIProcess()->GetCommandingActor() == pPlayer ||
            pPlayer->IsPlayerFollower(apActor) ||
            pPlayer->GetNumberOfFollowers() <= static_cast<int32_t>(iNumberActorsAllowedToFollowPlayer.UInt())) {
            pact->AddFollower(apActor);
        }
        else {
            apActor->GetCurrentAIProcess()->AddToProcedureIndexRunning(apActor, 0xFFFFFFFF);
            Interface::ShowMessage(sNoMoreFollowers.String(), 0, nullptr, nullptr, 2.0f, false);
        }
    }
    else if (GetPackType() == PACKAGE_TYPE_ESCORT && pact != pPlayer) {
        apActor->AddFollower(pact);
    }
    return pTarget;
}

bool TESPackage::IsitDayofTheWeek(bool bTestPreviousDay) {
    PackageSchedule* pSchedule = GetPackageSchedule();
    int8_t cDay = appCalendar.GetDayOfTheWeek();
    if (bTestPreviousDay) {
        --cDay;
        if (cDay < 0) {
            cDay = 6;
        }
    }
    bool bResult = false;
    switch (pSchedule->GetStartDayOfWeek()) {
        case 0:
            if (cDay == 0) {
                bResult = true;
            }
            break;
        case 1:
            if (cDay == 1) {
                bResult = true;
            }
            break;
        case 2:
            if (cDay == 2) {
                bResult = true;
            }
            break;
        case 3:
            if (cDay == 3) {
                bResult = true;
            }
            break;
        case 4:
            if (cDay == 4) {
                bResult = true;
            }
            break;
        case 5:
            if (cDay == 5) {
                bResult = true;
            }
            break;
        case 6:
            if (cDay == 6) {
                bResult = true;
            }
            break;
        // Weekdays.
        case 7:
            bResult = true;
            if (cDay == 0 || cDay == 6) {
                bResult = false;
            }
            break;
        // Weekends.
        case 8:
            if (cDay == 0 || cDay == 6) {
                bResult = true;
            }
            break;
        // Monday, Wednesday and Friday.
        case 9:
            if (cDay == 1 || cDay == 3 || cDay == 5) {
                bResult = true;
            }
            break;
        // Tuesday and Thursday.
        case 10:
            if (cDay == 2 || cDay == 4) {
                bResult = true;
            }
            break;
        default: break;
    }
    return bResult;
}

TESCondition* TESPackage::GetConditions() {
    return &packConditions;
}

bool TESPackage::GetAllowsSwim() {
    return ((((int32_t)PackData.iPackFlags) & 262144) != 0);
}

void TESPackage::SetAllowsSwim(bool abVal) {
    uint32_t iPackFlags;
    uint32_t v3;
    if (abVal) {
        PackData.iPackFlags |= 0x40000u;
        iPackFlags = PackData.iPackFlags;
    }
    else {
        PackData.iPackFlags &= (~0x40000u);
        v3 = PackData.iPackFlags;
    }
    uint32_t v4 = PackData.iPackFlags;
}

bool TESPackage::GetAllowFalls() {
    return ((((int32_t)PackData.iPackFlags) & 524288) != 0);
}

void TESPackage::SetAllowFalls(bool abVal) {
    uint32_t iPackFlags;
    uint32_t v3;
    if (abVal) {
        PackData.iPackFlags |= 0x80000u;
        iPackFlags = PackData.iPackFlags;
    }
    else {
        PackData.iPackFlags &= (~0x80000u);
        v3 = PackData.iPackFlags;
    }
    uint32_t v4 = PackData.iPackFlags;
}

void TESPackage::SetForceUnequipWeapon(bool abVal) {
    uint32_t iPackFlags;
    uint32_t v3;
    if (abVal) {
        PackData.iPackFlags |= 0x200000u;
        iPackFlags = PackData.iPackFlags;
    }
    else {
        PackData.iPackFlags &= (~0x200000u);
        v3 = PackData.iPackFlags;
    }
    uint32_t v4 = PackData.iPackFlags;
}

void TESPackage::SetNoHeadTrack(bool abVal) {
    uint32_t iPackFlags;
    uint32_t v3;
    if (abVal) {
        PackData.iPackFlags |= 1048576;
        iPackFlags = PackData.iPackFlags;
    }
    else {
        PackData.iPackFlags &= (~0x100000u);
        v3 = PackData.iPackFlags;
    }
    uint32_t v4 = PackData.iPackFlags;
}

bool TESPackage::GetContinueDuringCombat() {
    return ((((int32_t)PackData.iPackFlags) & 67108864) != 0);
}

void TESPackage::SetContinueDuringCombat(bool abVal) {
    uint32_t iPackFlags;
    uint32_t v3;
    if (abVal) {
        PackData.iPackFlags |= 0x4000000u;
        iPackFlags = PackData.iPackFlags;
    }
    else {
        PackData.iPackFlags &= (~0x4000000u);
        v3 = PackData.iPackFlags;
    }
    uint32_t v4 = PackData.iPackFlags;
}

bool TESPackage::GetCombatAlert() {
    return ((((int32_t)PackData.iPackFlags) & 134217728) == 0);
}

bool TESPackage::GetGuardWarnBehavior() {
    return ((((int32_t)PackData.iPackFlags) & 268435456) == 0);
}

bool TESPackage::GetFOEnableHellosToPlayer() {
    return ((PackData.iFOBehaviorFlags & 1) != 0);
}

void TESPackage::SetFOEnableRandomConversations(bool abVal) {
    uint16_t iFOBehaviorFlags;
    uint16_t v3;
    if (abVal) {
        PackData.iFOBehaviorFlags |= 2u;
        iFOBehaviorFlags = PackData.iFOBehaviorFlags;
    }
    else {
        PackData.iFOBehaviorFlags &= (~2u);
        v3 = PackData.iFOBehaviorFlags;
    }
    uint16_t v4 = PackData.iFOBehaviorFlags;
}

bool TESPackage::GetFOEnableObserveCombatBehavior() {
    return ((PackData.iFOBehaviorFlags & 4) != 0);
}

void TESPackage::SetFOEnableObserveCombatBehavior(bool abVal) {
    uint16_t iFOBehaviorFlags;
    uint16_t v3;
    if (abVal) {
        PackData.iFOBehaviorFlags |= 4u;
        iFOBehaviorFlags = PackData.iFOBehaviorFlags;
    }
    else {
        PackData.iFOBehaviorFlags &= (~4u);
        v3 = PackData.iFOBehaviorFlags;
    }
    uint16_t v4 = PackData.iFOBehaviorFlags;
}

bool TESPackage::GetFOEnableGreetCorpseBehavior() {
    return ((PackData.iFOBehaviorFlags & 8) != 0);
}

void TESPackage::SetFOEnableGreetCorpseBehavior(bool abVal) {
    uint16_t iFOBehaviorFlags;
    uint16_t v3;
    if (abVal) {
        PackData.iFOBehaviorFlags |= 8u;
        iFOBehaviorFlags = PackData.iFOBehaviorFlags;
    }
    else {
        PackData.iFOBehaviorFlags &= (~8u);
        v3 = PackData.iFOBehaviorFlags;
    }
    uint16_t v4 = PackData.iFOBehaviorFlags;
}

bool TESPackage::GetFOEnableReactionToPlayerActions() {
    return ((PackData.iFOBehaviorFlags & 16) != 0);
}

void TESPackage::SetFOEnableReactionToPlayerActions(bool abVal) {
    uint16_t iFOBehaviorFlags;
    uint16_t v3;
    if (abVal) {
        PackData.iFOBehaviorFlags |= 0x10u;
        iFOBehaviorFlags = PackData.iFOBehaviorFlags;
    }
    else {
        PackData.iFOBehaviorFlags &= (~0x10u);
        v3 = PackData.iFOBehaviorFlags;
    }
    uint16_t v4 = PackData.iFOBehaviorFlags;
}

bool TESPackage::GetFOEnableFriendlyFireComments() {
    return ((PackData.iFOBehaviorFlags & 32) != 0);
}

void TESPackage::SetFOEnableFriendlyFireComments(bool abVal) {
    uint16_t iFOBehaviorFlags;
    uint16_t v3;
    if (abVal) {
        PackData.iFOBehaviorFlags |= 0x20u;
        iFOBehaviorFlags = PackData.iFOBehaviorFlags;
    }
    else {
        PackData.iFOBehaviorFlags &= (~0x20u);
        v3 = PackData.iFOBehaviorFlags;
    }
    uint16_t v4 = PackData.iFOBehaviorFlags;
}

bool TESPackage::GetFOEnableAggroRadiusBehavior() {
    return ((PackData.iFOBehaviorFlags & 64) != 0);
}

bool TESPackage::GetFOEnableIdleChatterBehavior() {
    return ((PackData.iFOBehaviorFlags & 128) != 0);
}

void TESPackage::SetFOEnableIdleChatterBehavior(bool abVal) {
    uint16_t iFOBehaviorFlags;
    uint16_t v3;
    if (abVal) {
        PackData.iFOBehaviorFlags |= 0x80u;
        iFOBehaviorFlags = PackData.iFOBehaviorFlags;
    }
    else {
        PackData.iFOBehaviorFlags &= (~0x80u);
        v3 = PackData.iFOBehaviorFlags;
    }
    uint16_t v4 = PackData.iFOBehaviorFlags;
}

bool TESPackage::GetFOEnableAvoidRadiationBehavior() {
    return ((PackData.iFOBehaviorFlags & 256) != 0);
}

void TESPackage::SetFOEnableAvoidRadiationBehavior(bool abVal) {
    uint16_t iFOBehaviorFlags;
    uint16_t v3;
    if (abVal) {
        PackData.iFOBehaviorFlags |= 0x100u;
        iFOBehaviorFlags = PackData.iFOBehaviorFlags;
    }
    else {
        PackData.iFOBehaviorFlags &= (~0x100u);
        v3 = PackData.iFOBehaviorFlags;
    }
    uint16_t v4 = PackData.iFOBehaviorFlags;
}

void TESPackage::SetScriptPackage(bool abVal) {
    uint32_t v2;
    if (abVal) {
        v2 = PackData.iPackFlags | 16384;
    }
    else {
        v2 = PackData.iPackFlags & 0xFFFFBFFFu;
    }
    PackData.iPackFlags = v2;
}

void TESPackage::SetWaiting(bool abVal) {
    if (abVal) {
        PackData.iPackFlags |= 0x10000;
        uint32_t CastingType = iFormID;
        if (!pDataHandler->IsFormIDCreated(CastingType)) {
            AddChange(CHANGE_PACKAGE_WAITING);
        }
    }
    else {
        PackData.iPackFlags &= ~0x10000u;
        uint32_t v3 = iFormID;
        if (!pDataHandler->IsFormIDCreated(v3)) {
            RemoveChange(CHANGE_PACKAGE_WAITING);
        }
    }
}

bool TESPackage::GetDefensive() {
    return ((((int32_t)PackData.iPackFlags) & 4194304) != 0);
}

void TESPackage::SetDefensive(bool abVal) {
    uint32_t iPackFlags;
    uint32_t v3;
    if (abVal) {
        PackData.iPackFlags |= 0x400000u;
        iPackFlags = PackData.iPackFlags;
    }
    else {
        PackData.iPackFlags &= (~0x400000u);
        v3 = PackData.iPackFlags;
    }
    uint32_t v4 = PackData.iPackFlags;
}

bool TESPackage::GetAlwaysRun() {
    return ((((int32_t)PackData.iPackFlags) & 8192) != 0);
}

void TESPackage::SetAlwaysRun(bool abVal) {
    uint32_t iPackFlags;
    uint32_t v3;
    if (abVal) {
        PackData.iPackFlags |= 0x2000u;
        iPackFlags = PackData.iPackFlags;
    }
    else {
        PackData.iPackFlags &= (~0x2000u);
        v3 = PackData.iPackFlags;
    }
    uint32_t v4 = PackData.iPackFlags;
}

void TESPackage::SetAlwaysSneak(bool abVal) {
    uint32_t iPackFlags;
    uint32_t v3;
    if (abVal) {
        PackData.iPackFlags |= 0x20000u;
        iPackFlags = PackData.iPackFlags;
    }
    else {
        PackData.iPackFlags &= (~0x20000u);
        v3 = PackData.iPackFlags;
    }
    uint32_t v4 = PackData.iPackFlags;
}

void TESPackage::SetMustReachLocation(bool abVal) {
    int32_t uiPackageFlags;
    int32_t v3;
    if (abVal) {
        PackData.iPackFlags |= 2u;
        uiPackageFlags = (int32_t)PackData.iPackFlags;
    }
    else {
        PackData.iPackFlags &= (~2u);
        v3 = (int32_t)PackData.iPackFlags;
    }
    int32_t v4 = (int32_t)PackData.iPackFlags;
}

void TESPackage::SetMustComplete(bool abVal) {
    int32_t uiPackageFlags;
    int32_t v3;
    if (abVal) {
        PackData.iPackFlags |= 4u;
        uiPackageFlags = (int32_t)PackData.iPackFlags;
    }
    else {
        PackData.iPackFlags &= (~4u);
        v3 = (int32_t)PackData.iPackFlags;
    }
    int32_t v4 = (int32_t)PackData.iPackFlags;
}

bool TESPackage::GetServicesEnabled() {
    return ((((int32_t)PackData.iPackFlags) & 1) != 0);
}

void TESPackage::SetPretendInCombat(bool abVal) {
    uint32_t iPackFlags;
    uint32_t v3;
    if (abVal) {
        PackData.iPackFlags |= 0x2000000u;
        iPackFlags = PackData.iPackFlags;
    }
    else {
        PackData.iPackFlags &= (~0x2000000u);
        v3 = PackData.iPackFlags;
    }
    uint32_t v4 = PackData.iPackFlags;
}

bool TESPackage::GetWaiting() {
    return ((((int32_t)PackData.iPackFlags) & 65536) != 0);
}

uint32_t TESPackage::DecRefCount() {
    return NiAtomicDecrement(uiRefCount);
}

bool TESPackage::SearchGetAllowBuying() {
    return ((PackData.iPackageSpecificFlags & 256) != 0);
}

bool TESPackage::SearchGetAllowKilling() {
    return ((PackData.iPackageSpecificFlags & 512) != 0);
}

bool TESPackage::SearchGetAllowStealing() {
    return ((PackData.iPackageSpecificFlags & 1024) != 0);
}

bool TESPackage::AmbushGetHideWhileAmbushing() {
    return ((PackData.iPackageSpecificFlags & 1) != 0);
}

bool TESPackage::SandboxGetAllowConversation() {
    return ((PackData.iPackageSpecificFlags & 4) == 0);
}

bool TESPackage::GetPretendInCombat() {
    return ((((int32_t)PackData.iPackFlags) & 33554432) != 0);
}

bool TESPackage::SandboxGetAllowEating() {
    return ((PackData.iPackageSpecificFlags & 1) == 0);
}

bool TESPackage::SandboxGetAllowSleeping() {
    return ((PackData.iPackageSpecificFlags & 2) == 0);
}

bool TESPackage::SandboxGetAllowFurniture() {
    return ((PackData.iPackageSpecificFlags & 16) == 0);
}

bool TESPackage::SandboxGetAllowIdleMarkers() {
    return ((PackData.iPackageSpecificFlags & 8) == 0);
}

bool TESPackage::SandboxGetAllowWandering() {
    return ((PackData.iPackageSpecificFlags & 32) == 0);
}

TESIdleForm* TESPackage::GetOnChangeIdle() {
    return OnChange.pIdle;
}

TESIdleForm* TESPackage::GetOnEndIdle() {
    return OnEnd.pIdle;
}

bool TESPackage::GetDialogueSayToFlag() {
    int8_t v5 = 0;
    if ((((int32_t)GetPackType()) != 15) && (((int32_t)GetPackType()) != 28)) {
        return (bool)v5;
    }
    TESDialoguePackageData* pPackageData = static_cast<TESDialoguePackageData*>(pPackData);
    if (!pPackageData) {
        return (bool)v5;
    }
    TESPackageData* v1 = pPackData;
    return (bool)((uint8_t)pPackageData->bSayTo);
}

TESTopic* TESPackage::GetDialogueTopic() {
    int32_t v5 = 0;
    if ((((int32_t)GetPackType()) != 15) && (((int32_t)GetPackType()) != 28)) {
        return reinterpret_cast<TESTopic*>(static_cast<uintptr_t>(v5));
    }
    TESDialoguePackageData* pPackageData = static_cast<TESDialoguePackageData*>(pPackData);
    if (!pPackageData) {
        return reinterpret_cast<TESTopic*>(static_cast<uintptr_t>(v5));
    }
    TESPackageData* v1 = pPackData;
    return pPackageData->ptopic;
}

bool TESPackage::GetDialogueDoNotControlTarget() {
    uint8_t v5 = 0;
    if ((((int32_t)GetPackType()) != 15) && (((int32_t)GetPackType()) != 28)) {
        return (bool)v5;
    }
    TESDialoguePackageData* pPackageData = static_cast<TESDialoguePackageData*>(pPackData);
    if (!pPackageData) {
        return (bool)v5;
    }
    TESPackageData* v1 = pPackData;
    return (bool)((uint8_t)pPackageData->bDoNotControlTarget);
}

bool TESPackage::GetDialogueNoHeadtrack() {
    int8_t v5 = 0;
    if ((((int32_t)GetPackType()) != 15) && (((int32_t)GetPackType()) != 28)) {
        return (bool)v5;
    }
    TESDialoguePackageData* pPackageData = static_cast<TESDialoguePackageData*>(pPackData);
    if (!pPackageData) {
        return (bool)v5;
    }
    TESPackageData* v1 = pPackData;
    return (bool)((uint8_t)pPackageData->bNOHeadtracking);
}

float TESPackage::GetDialogueFOV() {
    float v5 = (float)1.0;
    if ((((int32_t)GetPackType()) != 15) && (((int32_t)GetPackType()) != 28)) {
        return v5;
    }
    TESDialoguePackageData* pPackageData = static_cast<TESDialoguePackageData*>(pPackData);
    if (!pPackageData) {
        return v5;
    }
    TESPackageData* v1 = pPackData;
    return pPackageData->fFov;
}

uint32_t TESPackage::GetSearchLocationRadius(MobileObject* apActor) {
    PackageLocation* packageSearchLocation = GetPackageSearchLocation();
    if (packageSearchLocation == GetLocation()) {
        return GetLocationRadius(apActor);
    }

    return GetSecondLocationRadius();
}

void TESPackage::CopyAllFlags(TESPackage* apCopy) {
    int8_t skipFalloutBehaviour = (int8_t)apCopy->GetSelectiveFalloutBehavior();
    SetSelectiveFalloutBehavior(skipFalloutBehaviour);
    uint8_t fOEnableHellosToPlayer = (uint8_t)apCopy->GetFOEnableHellosToPlayer();
    SetFOEnableHellosToPlayer(fOEnableHellosToPlayer);
    uint8_t fOEnableRandomConversations = (uint8_t)apCopy->GetFOEnableRandomConversations();
    SetFOEnableRandomConversations(fOEnableRandomConversations);
    uint8_t fOEnableObserveCombatBehavior = (uint8_t)apCopy->GetFOEnableObserveCombatBehavior();
    SetFOEnableObserveCombatBehavior(fOEnableObserveCombatBehavior);
    uint8_t fOEnableGreetCorpseBehavior = (uint8_t)apCopy->GetFOEnableGreetCorpseBehavior();
    SetFOEnableGreetCorpseBehavior(fOEnableGreetCorpseBehavior);
    uint8_t fOEnableReactionToPlayerActions = (uint8_t)apCopy->GetFOEnableReactionToPlayerActions();
    SetFOEnableReactionToPlayerActions(fOEnableReactionToPlayerActions);
    uint8_t fOEnableFriendlyFireComments = (uint8_t)apCopy->GetFOEnableFriendlyFireComments();
    SetFOEnableFriendlyFireComments(fOEnableFriendlyFireComments);
    uint8_t fOEnableAggroRadiusBehavior = (uint8_t)apCopy->GetFOEnableAggroRadiusBehavior();
    SetFOEnableAggroRadiusBehavior(fOEnableAggroRadiusBehavior);
    uint8_t fOEnableIdleChatterBehavior = (uint8_t)apCopy->GetFOEnableIdleChatterBehavior();
    SetFOEnableIdleChatterBehavior(fOEnableIdleChatterBehavior);
    uint8_t fOEnableAvoidRadiationBehavior = (uint8_t)apCopy->GetFOEnableAvoidRadiationBehavior();
    SetFOEnableAvoidRadiationBehavior(fOEnableAvoidRadiationBehavior);
    uint8_t drawWeaponFlag = (uint8_t)apCopy->GetDrawWeaponFlag();
    SetDrawWeaponFlag(drawWeaponFlag);
    uint8_t noHeadTrack = (uint8_t)apCopy->GetNoHeadTrack();
    SetNoHeadTrack(noHeadTrack);
    uint8_t forceUnequipWeapon = (uint8_t)apCopy->GetForceUnequipWeapon();
    SetForceUnequipWeapon(forceUnequipWeapon);
    uint8_t alwaysRun = (uint8_t)apCopy->GetAlwaysRun();
    SetAlwaysRun(alwaysRun);
    uint8_t alwaysSneak = (uint8_t)apCopy->GetAlwaysSneak();
    SetAlwaysSneak(alwaysSneak);
    int8_t combatAlert = (int8_t)apCopy->GetCombatAlert();
    SetCombatAlert(combatAlert);
    TESCombatStyle* combatStyle = apCopy->TESPackage::GetCombatStyle();
    TESPackage::SetCombatStyle(combatStyle);
    uint8_t continueDuringCombat = (uint8_t)apCopy->GetContinueDuringCombat();
    SetContinueDuringCombat(continueDuringCombat);
    uint8_t defensive = (uint8_t)apCopy->GetDefensive();
    SetDefensive(defensive);
    uint8_t allowFalls = (uint8_t)apCopy->GetAllowFalls();
    SetAllowFalls(allowFalls);
    uint8_t allowsSwim = (uint8_t)apCopy->GetAllowsSwim();
    SetAllowsSwim(allowsSwim);
    int8_t noWarnAttackBehavior = (int8_t)apCopy->GetGuardWarnBehavior();
    SetGuardWarnBehavior(noWarnAttackBehavior);
}

void TESPackage::SetDialogueTopic(TESTopic* apTopic) {
    TESDialoguePackageData* v2;
    void* v4;
    TESDialoguePackageData* pPackageData;
    if ((((int32_t)GetPackType()) == 15) || (((int32_t)GetPackType()) == 28)) {
        if (pPackData) {
            pPackageData = static_cast<TESDialoguePackageData*>(pPackData);
        }
        else {
            v4 = ourMalloc(sizeof(TESDialoguePackageData));
            if (v4) {
                v2 = ::new (v4) TESDialoguePackageData;
            }
            else {
                v2 = nullptr;
            }
            pPackageData = v2;
            pPackData = v2;
        }
        pPackageData->ptopic = apTopic;
    }
}

void TESPackage::SetDialogueDoNotControlTarget(bool abControl) {
    TESDialoguePackageData* v2;
    void* v4;
    TESDialoguePackageData* pPackData;
    if ((((int32_t)GetPackType()) == 15) || (((int32_t)GetPackType()) == 28)) {
        if (this->pPackData) {
            pPackData = static_cast<TESDialoguePackageData*>(this->pPackData);
        }
        else {
            v4 = ourMalloc(sizeof(TESDialoguePackageData));
            if (v4) {
                v2 = ::new (v4) TESDialoguePackageData;
            }
            else {
                v2 = nullptr;
            }
            pPackData = v2;
            this->pPackData = v2;
        }
        pPackData->bDoNotControlTarget = (bool)((uint8_t)abControl);
    }
}

void TESPackage::SetPackageConditions(TESCondition* apPackCond) {
    if (apPackCond) {
        (&packConditions)->Copy(apPackCond, nullptr);
    }
    else {
        TESCondition kTemp;
        (&packConditions)->Copy(&kTemp, nullptr);
    }
}

TESTopic* TESPackage::GetOnChangeTopicInfo() {
    return OnChange.pTopic;
}

TESWorldSpace* TESPackage::GetSecondLocationWorld(MobileObject* apActor) {
    TESWorldSpace* pretworld = nullptr;
    PackageLocation* ploc = GetPackageSecondLocation();
    if (!ploc || ploc->GetLocType() == PLT_CURRLOC) {
        if (apActor) {
            return apActor->GetPackageStartLocationWorld();
        }
    }
    else {
        switch (ploc->GetLocType()) {
            case PLT_REFERENCE: {
                if (ploc->GetLocReference()) {
                    TESObjectREFR* LocReference = ploc->GetLocReference();
                    pretworld = LocReference->GetWorldSpace();
                }
                break;
            }
            case PLT_CELL: {
                if (ploc->GetLocCell()) {
                    TESObjectCELL* LocCell = ploc->GetLocCell();
                    pretworld = LocCell->GetWorldSpace();
                }
                break;
            }
            case PLT_EDITORLOC: {
                if (apActor) {
                    pretworld = apActor->GetEditorLocationWorld();
                }
                break;
            }
            case PLT_OBJECT:
            case PLT_TYPE: {
                BaseProcess* v7;
                if (apActor &&
                    (apActor->pCurrentProcess &&
                     (v7 = apActor->pCurrentProcess, v7->GetPackageThatIsRunning() == this))) {
                    BaseProcess* CurrentAIProcess = apActor->pCurrentProcess;
                    TESObjectREFR* pref = CurrentAIProcess->GetSecondGenericLocation();
                    if (pref) {
                        pretworld = pref->GetWorldSpace();
                    }
                    else {
                        pretworld = apActor->GetWorldSpace();
                    }
                }
                else {
                    if (apActor == pPlayer && GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
                        DialoguePackage* pDialogue = static_cast<DialoguePackage*>(this);
                        if (pDialogue->pTargetLocation) {
                            pretworld = pDialogue->pTargetLocation->GetWorldSpace();
                        }
                    }
                }
                break;
            }
            case PLT_LINKEDREF: {
                if ((apActor) && (apActor->GetLinkedRef())) {
                    TESObjectREFR* LinkedRef = apActor->GetLinkedRef();
                    pretworld = LinkedRef->GetWorldSpace();
                }

                break;
            }
            default: {
                return pretworld;
            }
        }
    }
    return pretworld;
}

NiPoint3 TESPackage::GetSecondLocationCoord(MobileObject* apActor) {
    PackageLocation* pLoc = GetPackageSecondLocation();
    NiPoint3 locPt = NiPoint3_ZERO;
    if (!pLoc || pLoc->GetLocType() == PLT_CURRLOC) {
        if (apActor) {
            locPt = apActor->GetPackageStartLocationCoord();
        }
    }
    else {
        switch (pLoc->GetLocType()) {
            case PLT_REFERENCE: {
                if (pLoc->GetLocReference()) {
                    TESObjectREFR* LocReference = pLoc->GetLocReference();
                    locPt = LocReference->GetLocationOnReference();
                }
                break;
            }
            case PLT_CELL: {
                TESObjectCELL* pcell = pLoc->GetLocCell();
                if (pcell) {
                    TESObjectREFR* pref = pcell->GetFirstRefr();
                    if (pref) {
                        locPt = pref->GetLocationOnReference();
                    }
                    else {
                        if (!pcell->GetInterior()) {
                            TESObjectCELL* pcell_2 = pLoc->GetLocCell();
                            locPt.x = static_cast<float>(pcell_2->GetDataX() << 12);
                            TESObjectCELL* pcell_3 = pLoc->GetLocCell();
                            locPt.y = static_cast<float>(pcell_3->GetDataY() << 12);
                            locPt.z = 0.0f;
                        }
                    }
                }
                break;
            }
            case PLT_EDITORLOC: {
                if (apActor) {
                    locPt = apActor->GetStartingLocation();
                }
                break;
            }
            case PLT_OBJECT:
            case PLT_TYPE: {
                BaseProcess* v9;
                if (apActor &&
                    (apActor->pCurrentProcess &&
                     (v9 = apActor->pCurrentProcess, v9->GetPackageThatIsRunning() == this))) {
                    const NiPoint3* v6;
                    BaseProcess* CurrentAIProcess = apActor->pCurrentProcess;
                    TESObjectREFR* pref_2 = CurrentAIProcess->GetSecondGenericLocation();
                    if (pref_2) {
                        v6 = &pref_2->GetLocationOnReference();
                    }
                    else {
                        v6 = &apActor->GetLocationOnReference();
                    }
                    locPt = *v6;
                }
                else {
                    if (apActor == pPlayer && GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
                        DialoguePackage* pDialogue = static_cast<DialoguePackage*>(this);
                        if (pDialogue->pTargetLocation) {
                            locPt = pDialogue->pTargetLocation->GetLocationOnReference();
                        }
                    }
                }
                break;
            }
            case PLT_LINKEDREF: {
                if ((apActor) && (apActor->GetLinkedRef())) {
                    TESObjectREFR* LinkedRef = apActor->GetLinkedRef();
                    locPt = LinkedRef->GetLocationOnReference();
                }

                break;
            }
            default: {
                break;
            }
        }
    }
    return static_cast<const NiPoint3&>(locPt);
}

bool TESPackage::IsTargetAtSecondLocation(MobileObject* apActor, float aiRadius) {
    NiPoint3 a1;
    NiPoint3 v14;
    bool abRefInsideTriggerOut;
    TESObjectREFR* apRef = apActor->GetCurrentTarget();
    if (!apActor || !apRef) {
        return false;
    }
    TESWorldSpace* WorldSpace = apRef->GetWorldSpace();
    TESObjectCELL* ParentCell = apRef->GetParentCell();
    if ((ParentCell) && (!ParentCell->GetInterior())) {
        ParentCell = nullptr;
    }

    TESWorldSpace* LocationWorld = GetLocationWorld(apActor);
    TESObjectCELL* LocationCell = GetLocationCell(apActor);
    if ((LocationCell) && (!LocationCell->GetInterior())) {
        LocationCell = nullptr;
    }

    bool v13 = false;
    if (LocationCell || ParentCell) {
        if (LocationCell == ParentCell) {
            v13 = true;
        }
    }
    else {
        v13 = LocationWorld == WorldSpace;
    }
    if (!v13) {
        return false;
    }
    PackageLocation* PackageSecondLocation = GetPackageSecondLocation();
    abRefInsideTriggerOut = false;
    if (PackageSecondLocation && PackageSecondLocation->IsRefAtLocationTrigger(apActor, apRef, abRefInsideTriggerOut)) {
        return abRefInsideTriggerOut;
    }
    if (aiRadius < 0.0) {
        int32_t SecondLocationRadius = GetSecondLocationRadius();
        if (!SecondLocationRadius) {
            uint32_t v9 = GetSecondLocationRadius();
            if (PackageSecondLocation) {
                TESObjectREFR* SecondLocationReference = GetSecondLocationReference(apActor);
                if (SecondLocationReference) {
                    SecondLocationRadius = static_cast<int32_t>(SecondLocationReference->GetModelBoundSize());
                }
            }
        }
        if (SecondLocationRadius <= 0) {
            SecondLocationRadius = iAIDistanceRadiusMinLocation.UInt();
        }
        aiRadius = static_cast<float>(SecondLocationRadius);
    }
    bool v17 = false;
    const NiPoint3* v5 = &apRef->GetLocationOnReference();
    NiPoint3* SecondLocationCoord = &(a1 = GetSecondLocationCoord(apActor));
    v14 = *SecondLocationCoord - *v5;
    if (aiRadius >= static_cast<double>(static_cast<int32_t>(v14.XYLength()))) {
        return true;
    }
    return v17;
}

int32_t TESPackage::GetEscortFollowDistance() {
    if (GetPackType() == PACKAGE_TYPE_ESCORT) {
        return static_cast<TESEscortPackageData*>(pPackData)->iFollowPackRadius;
    }

    return 256;
}

void TESPackage::SetDialogueNoHeadTrack(bool abHead) {
    if (GetPackType() == PACKAGE_TYPE_DIALOGUE || GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
        TESDialoguePackageData* pPackData;
        if (this->pPackData) {
            pPackData = static_cast<TESDialoguePackageData*>(this->pPackData);
        }
        else {
            TESDialoguePackageData* v2;
            TESDialoguePackageData* apMemory =
                static_cast<TESDialoguePackageData*>(ourMalloc(sizeof(TESDialoguePackageData)));
            if (apMemory) {
                v2 = ::new (apMemory) TESDialoguePackageData;
            }
            else {
                v2 = nullptr;
            }
            pPackData = v2;
            this->pPackData = v2;
        }
        pPackData->bNOHeadtracking = abHead;
    }
}

void TESPackage::SetDialogueFOV(float afFov) {
    if (GetPackType() == PACKAGE_TYPE_DIALOGUE || GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
        TESDialoguePackageData* pdata_2;
        if (pPackData) {
            pdata_2 = static_cast<TESDialoguePackageData*>(pPackData);
        }
        else {
            TESDialoguePackageData* pdata;
            TESDialoguePackageData* apMemory =
                static_cast<TESDialoguePackageData*>(ourMalloc(sizeof(TESDialoguePackageData)));
            if (apMemory) {
                pdata = ::new (apMemory) TESDialoguePackageData;
            }
            else {
                pdata = nullptr;
            }
            pdata_2 = pdata;
            pPackData = pdata;
        }
        pdata_2->fFov = afFov;
    }
}

void TESPackage::SetPackageIdles(BGSIdleCollection* apIdles) {
    if (apIdles) {
        if (!pIdleCollection) {
            BGSIdleCollection* v2;
            BGSIdleCollection* apMemory = static_cast<BGSIdleCollection*>(ourMalloc(sizeof(BGSIdleCollection)));
            if (apMemory) {
                v2 = ::new (apMemory) BGSIdleCollection;
            }
            else {
                v2 = nullptr;
            }
            pIdleCollection = v2;
        }
        pIdleCollection->CopyComponent(apIdles);
    }
    else {
        if (pIdleCollection) {
            delete pIdleCollection;
        }
        pIdleCollection = nullptr;
    }
}

TESObjectREFR* TESPackage::GetSecondLocationReference(MobileObject* apActor) {
    PackageLocation* PackageSecondLocation = GetPackageSecondLocation();
    MobileObject* LocReference = nullptr;
    if (!PackageSecondLocation || PackageSecondLocation->GetLocType() == PLT_CURRLOC) {
        if (apActor) {
            return apActor;
        }
    }
    else {
        switch (PackageSecondLocation->GetLocType()) {
            case PLT_REFERENCE: {
                if (PackageSecondLocation->GetLocReference()) {
                    LocReference = static_cast<MobileObject*>(PackageSecondLocation->GetLocReference());
                }
                break;
            }
            case PLT_EDITORLOC: {
                if (apActor) {
                    LocReference = apActor;
                }
                break;
            }
            case PLT_OBJECT:
            case PLT_TYPE: {
                if ((apActor) && (apActor->pCurrentProcess)) {
                    BaseProcess* CurrentAIProcess = apActor->pCurrentProcess;
                    if (CurrentAIProcess->GetPackageThatIsRunning() == this) {
                        BaseProcess* v3 = apActor->pCurrentProcess;
                        TESObjectREFR* v6 = v3->GetSecondGenericLocation();
                        if (v6) {
                            LocReference = static_cast<MobileObject*>(v6);
                        }
                        else {
                            LocReference = apActor;
                        }
                    }
                }

                break;
            }
            case PLT_LINKEDREF: {
                if ((apActor) && (apActor->GetLinkedRef())) {
                    LocReference = static_cast<MobileObject*>(apActor->GetLinkedRef());
                }

                break;
            }
            default: {
                return LocReference;
            }
        }
    }
    return LocReference;
}

const NiPoint3 TESPackage::GetSearchLocationCoord(MobileObject* apActor) {
    PackageLocation* ploc = GetPackageSearchLocation();
    if (ploc == pPackLoc) {
        return GetLocationCoord(apActor);
    }
    return GetSecondLocationCoord(apActor);
}

const TESFollowPackageData* TESPackage::GetFollowPackageData() {
    if (GetPackType() == PACKAGE_TYPE_FOLLOW) {
        return static_cast<TESFollowPackageData*>(pPackData);
    }

    return nullptr;
}

// Orders this package's schedule against another: month, then date, weekday and hour, each a signed byte where -1 is
// "any"; 0 when all match. Where the dates are tested for "both any", this schedule's date is tested for 0, not -1, as
// the game does.
int32_t TESPackage::ComparePackageSchedule(PackageSchedule* apSchedule) {
    PackageSchedule* pSchedule = GetPackageSchedule();
    const PACK_SCHED_DATA& rMine = pSchedule->PSData;
    const PACK_SCHED_DATA& rOther = apSchedule->PSData;
    if ((rMine.eMonth == PSM_NONE && rOther.eMonth == PSM_NONE) || rMine.eMonth == rOther.eMonth) {
        if ((rMine.cDate == 0 && rOther.cDate == -1) || rMine.cDate == rOther.cDate) {
            if ((rMine.eDayOfWeek == PSD_NONE && rOther.eDayOfWeek == PSD_NONE) ||
                rMine.eDayOfWeek == rOther.eDayOfWeek) {
                if ((rMine.cTime == -1 && rOther.cTime == -1) || rMine.cTime == rOther.cTime) {
                    return 0;
                }
                if (rMine.cTime == -1) {
                    return 1;
                }
                if (rOther.cTime != -1 && rMine.cTime >= rOther.cTime) {
                    return 1;
                }
                return -1;
            }
            if (rMine.eDayOfWeek == PSD_NONE) {
                return 1;
            }
            if (rOther.eDayOfWeek != PSD_NONE && rMine.eDayOfWeek >= rOther.eDayOfWeek) {
                return 1;
            }
            return -1;
        }
        if (rMine.cDate == -1) {
            return 1;
        }
        if (rOther.cDate != -1 && rMine.cDate >= rOther.cDate) {
            return 1;
        }
        return -1;
    }
    if (rMine.eMonth == PSM_NONE) {
        return 1;
    }
    if (rOther.eMonth != PSM_NONE && rMine.eMonth >= rOther.eMonth) {
        return 1;
    }
    return -1;
}

const GuardPackageData* TESPackage::GetGuardPackageData() {
    if (GetPackType() == PACKAGE_TYPE_GUARD) {
        return static_cast<GuardPackageData*>(pPackData);
    }

    return nullptr;
}

// Ambush and find only set aPackageType and leave the package null, so no type is applied and the result is null.
TESPackage* TESPackage::CreatePackage(PACKAGE_TYPE aPackageType) {
    TESPackage* pPackage = nullptr;
    void* pMemory;
    switch (aPackageType) {
        case PACKAGE_TYPE_DIALOGUE:
        case PACKAGE_TYPE_IN_GAME_DIALOGUE:
            pMemory = ourMalloc(sizeof(DialoguePackage));
            pPackage = pMemory ? ::new (pMemory) DialoguePackage : nullptr;
            break;
        case PACKAGE_TYPE_COMBAT:
            pMemory = ourMalloc(sizeof(CombatController));
            pPackage = pMemory ? ::new (pMemory) CombatController(nullptr, nullptr, nullptr) : nullptr;
            break;
        case PACKAGE_TYPE_ALARM:
            pMemory = ourMalloc(sizeof(AlarmPackage));
            pPackage = pMemory ? ::new (pMemory) AlarmPackage : nullptr;
            break;
        case PACKAGE_TYPE_FLEE:
            pMemory = ourMalloc(sizeof(FleePackage));
            pPackage = pMemory ? ::new (pMemory) FleePackage(nullptr, nullptr, nullptr) : nullptr;
            break;
        case PACKAGE_TYPE_TRESPASS:
            pMemory = ourMalloc(sizeof(TrespassPackage));
            pPackage = pMemory ? ::new (pMemory) TrespassPackage : nullptr;
            break;
        case PACKAGE_TYPE_SPECTATOR:
            pMemory = ourMalloc(sizeof(SpectatorPackage));
            pPackage = pMemory ? ::new (pMemory) SpectatorPackage : nullptr;
            break;
        case PACKAGE_TYPE_NONE:
        case PACKAGE_TYPE_COUNT: break;
        case PACKAGE_TYPE_FIND: aPackageType = PACKAGE_TYPE_EXPLORE; break;
        case PACKAGE_TYPE_AMBUSH: aPackageType = PACKAGE_TYPE_AMBUSH; break;
        case PACKAGE_TYPE_BACKUP:
            pMemory = ourMalloc(sizeof(BackUpPackage));
            pPackage = pMemory ? ::new (pMemory) BackUpPackage : nullptr;
            break;
        default:
            pMemory = ourMalloc(sizeof(TESPackage));
            pPackage = pMemory ? ::new (pMemory) TESPackage : nullptr;
            break;
    }
    if (pPackage) {
        pPackage->SetPackType(aPackageType);
    }
    return pPackage;
}

// The package object type of a form; ingredients and potions that are food count as food, weapons as melee or ranged
// where they are one.
PACKAGE_OBJECT_TYPE TESPackage::GetObjectTypeFromForm(TESForm* apForm) {
    PACKAGE_OBJECT_TYPE aeType = POTYPE_NONE;
    if (apForm) {
        switch (apForm->GetFormType()) {
            case SPEL_ID: aeType = POTYPE_SPELL; break;
            case ACTI_ID: aeType = POTYPE_ACTI; break;
            case ARMO_ID: aeType = POTYPE_ARMO; break;
            case BOOK_ID: aeType = POTYPE_BOOK; break;
            case CLOT_ID: aeType = POTYPE_CLOT; break;
            case CONT_ID: aeType = POTYPE_CONT; break;
            case DOOR_ID: aeType = POTYPE_DOOR; break;
            case INGR_ID:
                if (static_cast<EffectItemList*>(static_cast<IngredientItem*>(apForm))->IsFood()) {
                    aeType = POTYPE_FOOD;
                }
                else {
                    aeType = POTYPE_INGR;
                }
                break;
            case LIGH_ID: aeType = POTYPE_LIGH; break;
            case MISC_ID:
            case COBJ_ID:
            case IMOD_ID: aeType = POTYPE_MISC; break;
            case FLOR_ID: aeType = POTYPE_FLOR; break;
            case FURN_ID: aeType = POTYPE_FURN; break;
            case WEAP_ID: {
                aeType = POTYPE_WEAP;
                TESObjectWEAP* pWeapon = static_cast<TESObjectWEAP*>(apForm);
                if (pWeapon) {
                    if (pWeapon->TESObjectWEAP::IsMeleeWeapon()) {
                        aeType = POTYPE_WEAP_MELEE;
                    }
                    else if (pWeapon->TESObjectWEAP::IsRangedWeapon()) {
                        aeType = POTYPE_WEAP_RANGED;
                    }
                }
                break;
            }
            case AMMO_ID: aeType = POTYPE_AMMO; break;
            case NPC__ID: aeType = POTYPE_NPC; break;
            case CREA_ID: aeType = POTYPE_CREA; break;
            case KEYM_ID: aeType = POTYPE_KEYM; break;
            case ALCH_ID:
                if (static_cast<EffectItemList*>(static_cast<AlchemyItem*>(apForm))->IsFood()) {
                    aeType = POTYPE_FOOD;
                }
                else {
                    aeType = POTYPE_ALCH;
                }
                break;
            case CHIP_ID:
            case CCRD_ID:
            case CMNY_ID: aeType = POTYPE_MISC; break;
            default: break;
        }
    }
    return aeType;
}

// Whether a form is of a package object type. Tests its cast spell and weapon pointers for null again before using
// them.
bool TESPackage::FormMatchesPackageObjectType(TESForm* apForm, PACKAGE_OBJECT_TYPE aeType) {
    bool bretval = false;
    if (apForm && aeType != POTYPE_NONE) {
        switch (apForm->GetFormType()) {
            case SPEL_ID: {
                if (aeType == POTYPE_SPELL) {
                    bretval = true;
                }
                else if (aeType > POTYPE_SPELL && aeType <= POTYPE_SPELL_SELF) {
                    SpellItem* pSpell = static_cast<SpellItem*>(apForm);
                    if (pSpell) {
                        switch (aeType) {
                            case POTYPE_SPELL_RANGED:
                                if (pSpell->HasTargetEffect()) {
                                    bretval = true;
                                }
                                break;
                            case POTYPE_SPELL_TOUCH:
                                if (pSpell->HasTouchEffect()) {
                                    bretval = true;
                                }
                                break;
                            case POTYPE_SPELL_SELF:
                                if (pSpell->HasSelfEffect()) {
                                    bretval = true;
                                }
                                break;
                            default: break;
                        }
                    }
                }
                break;
            }
            case ACTI_ID: {
                if (aeType == POTYPE_ACTI) {
                    bretval = true;
                }
                break;
            }
            case ARMO_ID: {
                if (aeType == POTYPE_ARMO || (aeType > POTYPE_FOOD && aeType <= POTYPE_WEAR)) {
                    bretval = true;
                }
                break;
            }
            case BOOK_ID: {
                if (aeType == POTYPE_BOOK) {
                    bretval = true;
                }
                break;
            }
            case CLOT_ID: {
                if (aeType == POTYPE_CLOT || aeType == POTYPE_WEAR) {
                    bretval = true;
                }
                break;
            }
            case CONT_ID: {
                if (aeType == POTYPE_CONT) {
                    bretval = true;
                }
                break;
            }
            case DOOR_ID: {
                if (aeType == POTYPE_DOOR) {
                    bretval = true;
                }
                break;
            }
            case INGR_ID: {
                if (aeType == POTYPE_INGR) {
                    bretval = true;
                }
                else if (
                    (aeType == POTYPE_FOOD) &&
                    (static_cast<EffectItemList*>(static_cast<IngredientItem*>(apForm))->IsFood())
                ) {
                    bretval = true;
                }

                break;
            }
            case LIGH_ID: {
                if (aeType == POTYPE_LIGH) {
                    bretval = true;
                }
                break;
            }
            case MISC_ID:
            case COBJ_ID:
            case IMOD_ID: {
                if (aeType == POTYPE_MISC) {
                    bretval = true;
                }
                break;
            }
            case FLOR_ID: {
                if (aeType == POTYPE_FLOR) {
                    bretval = true;
                }
                break;
            }
            case FURN_ID: {
                if (aeType == POTYPE_FURN) {
                    bretval = true;
                }
                break;
            }
            case WEAP_ID: {
                switch (aeType) {
                    case POTYPE_WEAP:
                    case POTYPE_COMBATWEAR:
                    case POTYPE_WEAR: bretval = true; break;
                    case POTYPE_WEAP_MELEE:
                    case POTYPE_WEAP_RANGED: {
                        TESObjectWEAP* pWeapon = static_cast<TESObjectWEAP*>(apForm);
                        if ((pWeapon) &&
                            ((pWeapon->TESObjectWEAP::IsMeleeWeapon() && aeType == POTYPE_WEAP_MELEE) ||
                             (pWeapon->TESObjectWEAP::IsRangedWeapon() && aeType == POTYPE_WEAP_RANGED))) {
                            bretval = true;
                        }

                        break;
                    }
                    default: break;
                }
                break;
            }
            case AMMO_ID: {
                if (aeType == POTYPE_AMMO || (aeType > POTYPE_FOOD && aeType <= POTYPE_WEAR)) {
                    bretval = true;
                }
                break;
            }
            case NPC__ID: {
                if (aeType == POTYPE_NPC || aeType == POTYPE_ACTOR) {
                    bretval = true;
                }
                break;
            }
            case CREA_ID: {
                if (aeType == POTYPE_CREA || aeType == POTYPE_ACTOR) {
                    bretval = true;
                }
                break;
            }
            case KEYM_ID: {
                if (aeType == POTYPE_KEYM) {
                    bretval = true;
                }
                break;
            }
            case ALCH_ID: {
                if (aeType == POTYPE_ALCH) {
                    bretval = true;
                }
                else if (
                    (aeType == POTYPE_FOOD) &&
                    (static_cast<EffectItemList*>(static_cast<AlchemyItem*>(apForm))->IsFood())
                ) {
                    bretval = true;
                }

                break;
            }
            case CHIP_ID:
            case CCRD_ID:
            case CMNY_ID: {
                if (aeType == POTYPE_MISC) {
                    bretval = true;
                }
                break;
            }
            default: break;
        }
    }
    return bretval;
}

bool TESPackage::EvaluateEscortReference(TESObjectREFR* apRef, void* aiEnumData) {
    if (!aiEnumData) {
        return false;
    }
    // aiEnumData is the context GetEscortTargetForPackage hands the reference enumeration.
    TESEvaluateEscortContext* pContext = static_cast<TESEvaluateEscortContext*>(aiEnumData);
    TESObject* pObject = pContext->pObject;
    Actor* pRequester = pContext->pRequester;
    bool bSkipExistingFollowers = pContext->bSkipExistingFollowers;
    if (!pObject->IsBoundObject()) {
        return false;
    }
    TESBoundObject* pBoundObject = static_cast<TESBoundObject*>(pObject);
    if (!apRef->GetIsOfType(pBoundObject) || apRef->GetDelete()) {
        return false;
    }
    if (apRef->IsActor()) {
        if (apRef == pRequester) {
            return false;
        }
        if (apRef->IsDead(true)) {
            return false;
        }
        if (bSkipExistingFollowers) {
            TESPackage* ppack = static_cast<MobileObject*>(apRef)->MobileObject::GetCurrentPackage();
            if (ppack && ppack->GetPackType() == PACKAGE_TYPE_FOLLOW) {
                return false;
            }
            if (pRequester->HasFollower(static_cast<Actor*>(apRef))) {
                return false;
            }
        }
    }
    pTempTarget = apRef;
    return true;
}

void PackageEventAction::SetType(int32_t aeType) {
    eType = static_cast<PACK_EVENT_ACTION_TYPE>(aeType);
}

PSDAYOFWEEK PackageSchedule::GetStartDayOfWeek() {
    return static_cast<PSDAYOFWEEK>(PSData.eDayOfWeek);
}
