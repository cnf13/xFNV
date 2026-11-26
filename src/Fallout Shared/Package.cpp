#include "Fallout Shared/Package.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "BSMain/BSFurnitureMarker.h"
#include "BSMain/MessageHandler.h"
#include "BSMain/Setting.h"
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
#include "Fallout Shared/Settings/XGameSetting.h"
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

#include <limits>

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
    BSEndian(iPackFlags, false);
    BSEndian(iFOBehaviorFlags, false);
    BSEndian(iPackageSpecificFlags, false);
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
            GetPackType() == PACKAGE_TYPE_ALARM ||
            GetPackType() == PACKAGE_TYPE_TRESPASS) {
            ReferenceProcessLists.NullOutActorsPackage(this);
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
// temporary with the file-data constructor, copied in by the setter and destroyed.
bool TESPackage::Load(TESFile* apFile) {
    // Compares only the low byte of the record type.
    if (static_cast<uint8_t>(apFile->GetTESForm()) != PACK_ID) {
        return false;
    }
    TESForm::LoadForm(apFile);
    do {
        switch (apFile->GetTESChunk()) {
            case EDID_ID: {
                // The buffer is sized by the chunk, but the read is deliberately capped at 0x200 bytes.
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
                pPackData = new TESDialoguePackageData(PLData);
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
                pPackData = new TESFollowPackageData(PLData);
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
                pPackData = new TESEscortPackageData(&PEData);
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
                pPackData = new TESUseWeaponPackageData(useWeaponData);
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
                pPackData = new TESUseWeaponPackageData(useWeaponData);
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
                pPackData = new TESPatrolPackageData(patrolData);
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
                        pddata->pTargetLocation = new PackageLocation(&PLData);
                        break;
                    }
                    case PACKAGE_TYPE_USE_WEAPON: {
                        TESUseWeaponPackageData* pddata = static_cast<TESUseWeaponPackageData*>(pPackData);
                        pddata->pTargetLocation = new PackageLocation(&PLData);
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
                    pddata->pAttackTarget = new PackageTarget(&PTData);
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
                PackageSchedule packSchedule(&PSData);
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
                    pIdleCollection = new BGSIdleCollection;
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
        GetPackageSecondLocation()->Save(PLD2_ID);
    }
    if (GetPackageSchedule()) {
        GetPackageSchedule()->Save();
    }
    if (GetPackageTarget()) {
        GetPackageTarget()->Save(PTDT_ID);
    }
    // The game tests this address (&packConditions) for null. It never is null, so Save() is called unconditionally.
    packConditions.Save();
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
    if (!pDataHandler->IsFormIDCreated(TESForm::GetFormID())) {
        if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_PACKAGE_NEVER_RUN)) {
            PackData.iPackFlags &= ~kPackageFlag_NeverToRun;
        }
        if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_PACKAGE_WAITING)) {
            PackData.iPackFlags &= ~kPackageFlag_Waiting;
        }
    }
}

void TESPackage::Revert(uint32_t aiFlags) {
    if ((aiFlags & CHANGE_PACKAGE_NEVER_RUN) != 0) {
        PackData.iPackFlags &= ~kPackageFlag_NeverToRun;
    }
    if ((aiFlags & CHANGE_PACKAGE_WAITING) != 0) {
        PackData.iPackFlags &= ~kPackageFlag_Waiting;
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
                "FORMS: Could not find combat style (%08X) on package '%s' (%08X).",
                uiCombatStyleID,
                GetFormEditorID(),
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
            GetFormEditorID(),
            GetFormID()
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
                PackageLocation* pLinked = new PackageLocation;
                pLinked->SetLocType(PLT_LINKEDREF);
                SetPackageLocation(pLinked);
                if (pLinked) {
                    delete pLinked;
                }
            }
            break;
        case PACKAGE_TYPE_GUARD:
            // The target built here is never freed: SetPackageTarget copies it into a new one.
            if (GuardGetTargetIsLinkedRef()) {
                if (!GetPackageTarget()) {
                    SetPackageTarget(new PackageTarget);
                }
                GetPackageTarget()->SetTargType(PTT_LINKEDREF);
            }
            break;
        default: break;
    }
    SetInitialized(true);
}

void TESPackage::Copy(TESForm* apCopy) {
    TESPackage* pCopy = dynamic_cast<TESPackage*>(apCopy);
    if (pCopy) {
        CopyAllComponents(pCopy);
        SetPackageLocation(pCopy->pPackLoc);
        SetPackageTarget(pCopy->GetPackageTarget());
        SetPackageSchedule(pCopy->GetPackageSchedule());
        PackData.iPackFlags = pCopy->PackData.iPackFlags;
        PackData.iFOBehaviorFlags = pCopy->PackData.iFOBehaviorFlags;
        SetPackageConditions(&pCopy->packConditions);
        SetPackageIdles(pCopy->pIdleCollection);
        pCombatStyle = pCopy->pCombatStyle;
        SetPackType(pCopy->GetPackType());
        PackData.iPackageSpecificFlags = pCopy->PackData.iPackageSpecificFlags;
        if (pPackData) {
            pPackData->Copy(pCopy->pPackData);
        }
        // The event actions are read through apCopy itself, not the cast's result.
        OnBegin.Copy(&static_cast<TESPackage*>(apCopy)->OnBegin);
        OnEnd.Copy(&static_cast<TESPackage*>(apCopy)->OnEnd);
        OnChange.Copy(&static_cast<TESPackage*>(apCopy)->OnChange);
        ePROCEDURE_TYPE = pCopy->ePROCEDURE_TYPE;
    }
}

bool TESPackage::Compare(TESForm* apCompare) {
    TESPackage* pComp = dynamic_cast<TESPackage*>(apCompare);
    if (!pComp) {
        return true;
    }
    if (CompareAllComponents(pComp)) {
        return true;
    }
    PACKAGE_TYPE ePackType = GetPackType();
    if (ePackType != pComp->GetPackType()) {
        return true;
    }
    if (PackData.iPackFlags != pComp->PackData.iPackFlags) {
        return true;
    }
    if (PackData.iFOBehaviorFlags != pComp->PackData.iFOBehaviorFlags) {
        return true;
    }
    if (PackData.iPackageSpecificFlags != pComp->PackData.iPackageSpecificFlags) {
        return true;
    }
    if (pIdleCollection) {
        if (pIdleCollection->CompareComponent(pComp->pIdleCollection)) {
            return true;
        }
    }
    else if (pComp->pIdleCollection) {
        return true;
    }
    if (pPackLoc) {
        if (pPackLoc->Compare(pComp->pPackLoc)) {
            return true;
        }
    }
    else if (pComp->pPackLoc) {
        return true;
    }
    if (GetPackageSecondLocation()) {
        PackageLocation* pCompSecondLocation = pComp->GetPackageSecondLocation();
        if (GetPackageSecondLocation()->Compare(pCompSecondLocation)) {
            return true;
        }
    }
    else if (pComp->GetPackageSecondLocation()) {
        return true;
    }
    if (GetPackageTarget()) {
        if (GetPackageTarget()->Compare(pComp->GetPackageTarget())) {
            return true;
        }
    }
    else if (pComp->GetPackageTarget()) {
        return true;
    }
    if (GetPackageSchedule()) {
        if (GetPackageSchedule()->Compare(pComp->GetPackageSchedule())) {
            return true;
        }
    }
    else if (pComp->GetPackageSchedule()) {
        return true;
    }
    // The game tests this address (&packConditions) for null. It never is null, so Save() is called unconditionally.
    if (packConditions.Compare(&pComp->packConditions)) {
        return true;
    }
    if (pCombatStyle) {
        if (pCombatStyle->Compare(pComp->pCombatStyle)) {
            return true;
        }
    }
    else if (pComp->pCombatStyle) {
        return true;
    }
    if (pPackData && pComp->pPackData) {
        if (pPackData->Compare(pComp->pPackData)) {
            return true;
        }
    }
    else if (pPackData || pComp->pPackData) {
        return true;
    }
    return OnBegin.Compare(&pComp->OnBegin) || OnEnd.Compare(&pComp->OnEnd) || OnChange.Compare(&pComp->OnChange);
}

const char* TESPackage::GetObjectTypeName() {
    return const_cast<char*>(packageTypeToString[GetPackType()]);
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
    if (!bInfurniture &&
        pActor &&
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
    if (pLocation &&
        pLocation->GetRadius() == 0 &&
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
            if (pFurniture &&
                apMobileObject->GetCurrentAIProcess() &&
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
    if (GetLocationRadius(apMobileObject) <= 20 &&
        pLocationRef &&
        (pLocationRef->GetObjectReference() == pXMarkerHeading || pLocationRef->GetObjectReference() == pXMarker)) {
        bExactRadius = true;
    }

    if (!iRadius) {
        iRadius = static_cast<int32_t>(iAIDistanceRadiusMinLocation.UInt());
    }

    bool bAtLocation = false;
    if (bInfurniture &&
        pLocationRef &&
        pLocationRef->IsFurniture() &&
        apMobileObject->GetCurrentAIProcess() &&
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
    if (!apMobileObject) {
        return false;
    }
    Actor* pActor = nullptr;
    if (apMobileObject->IsActor()) {
        pActor = static_cast<Actor*>(apMobileObject);
    }
    PackageLocation* pLoc = GetPackageSecondLocation();
    if (!pLoc) {
        return true;
    }
    if (pLoc->GetLocType() == PLT_EDITORLOC && !apPackageowner->HasEditorLocation()) {
        return true;
    }
    TESWorldSpace* pLocWorld = GetSecondLocationWorld(apPackageowner);
    TESObjectCELL* pLocIntCell = GetSecondLocationCell(apPackageowner);
    if (pLocIntCell && !pLocIntCell->GetInterior()) {
        pLocIntCell = nullptr;
    }

    TESWorldSpace* pActorWorld = apMobileObject->GetWorldSpace();
    TESObjectCELL* pActorIntCell = apMobileObject->GetParentCell();
    if (pActorIntCell && !pActorIntCell->GetInterior()) {
        pActorIntCell = nullptr;
    }

    bool bSameSpace = false;
    if (pActorIntCell || pLocIntCell) {
        if (pActorIntCell == pLocIntCell) {
            bSameSpace = true;
            if (pLoc->GetLocType() == PLT_CELL) {
                return true;
            }
        }
    }
    else {
        bSameSpace = pActorWorld == pLocWorld;
    }
    if (!bSameSpace) {
        return false;
    }
    bool bTriggered = false;
    if (pLoc->IsRefAtLocationTrigger(apPackageowner, apMobileObject, bTriggered)) {
        return bTriggered;
    }
    int32_t iRadius = GetSecondLocationRadius();
    if (iRadius > 0) {
        bInfurniture = false;
    }
    if (bignoredistance) {
        iRadius = 0;
    }
    else if (afOtherRadius != -1.0) {
        iRadius = static_cast<int32_t>(afOtherRadius);
    }
    bool bExactPos = false;
    NiPoint3 loccoord = GetSecondLocationCoord(apPackageowner);
    TESObjectREFR* pRef = nullptr;
    if (pLoc) {
        pRef = GetSecondLocationReference(apPackageowner);
    }
    if (!iRadius && pLoc) {
        if (pRef) {
            if (pRef->IsFurniture() &&
                apMobileObject->pCurrentProcess &&
                apMobileObject->pCurrentProcess->GetFurnitureMarker()) {
                if (!apPackageowner->pCurrentProcess->eLevel) {
                    loccoord = apPackageowner->pCurrentProcess->GetFurnitureMarker()->Position;
                }
                iRadius = 20;
                bExactPos = true;
            }
            else if (pRef->GetObjectReference() == pXMarkerHeading || pRef->GetObjectReference() == pXMarker) {
                iRadius = 20;
                bExactPos = true;
            }
            else if (pRef->IsActor()) {
                if (pRef->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
                    iRadius = 90;
                    bExactPos = true;
                }
                else {
                    iRadius = static_cast<int32_t>(pRef->GetModelBoundSize()) + 20;
                }
            }
            else {
                float fFudge = 20.0f;
                switch (pRef->GetObjectReference()->GetFormType()) {
                    case ACTI_ID:
                    case CONT_ID:
                    case STAT_ID:
                    case SCOL_ID: {
                        fFudge = 0.0f;
                        break;
                    }
                    default: {
                        break;
                    }
                }
                iRadius = static_cast<int32_t>(fFudge) + static_cast<int32_t>(pRef->GetModelBoundSize());
                if (iRadius < 0) {
                    iRadius = 0;
                }
            }
        }
        else if (pLoc->GetLocType() == PLT_EDITORLOC) {
            bExactPos = true;
            iRadius = 10;
        }
    }

    if (!iRadius) {
        iRadius = iAIDistanceRadiusMinLocation.Int();
    }
    bool bGoodPt = false;
    if (GetPackType() == PACKAGE_TYPE_WANDER && pActorIntCell) {
        const NiPoint3& kActorPos = apMobileObject->GetLocationOnReference();
        float fHeightDiff = kActorPos.z - GetSecondLocationCoord(apPackageowner).z;
        if (std::fabs(fHeightDiff) > 100.0) {
            return false;
        }
    }

    bool bOverride = false;
    if (bInfurniture &&
        pRef &&
        pRef->IsFurniture() &&
        apMobileObject->pCurrentProcess &&
        apMobileObject->pCurrentProcess->GetFurnitureMarker()) {
        apMobileObject->GetLocationOnReference();
        if (pActor &&
            (pActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
             pActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING)) {
            if (pActor->pCurrentProcess->GetCurrentFurniture() == pRef) {
                return true;
            }
        }
    }
    else if (apMobileObject->IsAtPoint(loccoord, static_cast<float>(iRadius), !bExactPos, true) && !bOverride) {
        return true;
    }
    return bGoodPt;
}

bool TESPackage::IsActorAtRefTarget(MobileObject* apMobileObject, int32_t aiExtraRadius) {
    if (!apMobileObject || !apMobileObject->GetCurrentTarget()) {
        return false;
    }
    MobileObject* pActor = nullptr;
    if (apMobileObject->IsActor()) {
        pActor = apMobileObject;
    }
    bool bExactPos = false;
    TESObjectREFR* pTargRef = apMobileObject->GetCurrentTarget();
    NiPoint3 targloc = pTargRef->GetLocationOnReference();
    if (pTargRef->GetTeleport()) {
        targloc = pTargRef->GetLinkedDoorTeleportPosition();
    }
    TESWorldSpace* pTargWorld = pTargRef->GetWorldSpace();
    TESObjectCELL* pTargIntCell = pTargRef->GetParentCell();
    if (pTargIntCell && !pTargIntCell->GetInterior()) {
        pTargIntCell = nullptr;
    }

    TESWorldSpace* pActorWorld = apMobileObject->GetWorldSpace();
    TESObjectCELL* pActorIntCell = apMobileObject->GetParentCell();
    if (pActorIntCell && !pActorIntCell->GetInterior()) {
        pActorIntCell = nullptr;
    }

    bool bSameSpace = false;
    if (pActorIntCell || pTargIntCell) {
        if (pActorIntCell == pTargIntCell) {
            bSameSpace = true;
        }
    }
    else {
        bSameSpace = pActorWorld == pTargWorld;
    }
    if (!bSameSpace) {
        return false;
    }
    int32_t iRadius = static_cast<int32_t>(GetPackageRadiusActorToRefTarget(apMobileObject, false));
    bool bGoodPt = false;
    NiPoint3 dist = targloc - apMobileObject->GetLocationOnReference();
    if (pTargRef->IsFurniture() &&
        apMobileObject->pCurrentProcess &&
        apMobileObject->pCurrentProcess->GetFurnitureMarker()) {
        if (!apMobileObject->pCurrentProcess->eLevel) {
            targloc = apMobileObject->pCurrentProcess->GetFurnitureMarker()->Position;
        }
        iRadius = 10;
        bExactPos = true;
    }
    else if (pTargRef->GetObjectReference()->GetFormType() == IDLM_ID) {
        iRadius = 10;
        bExactPos = true;
    }
    else if (iRadius > 20 || !pTargRef->IsActor() || pTargRef == pPlayer) {
        if (pTargRef->GetObjectReference() == pXMarkerHeading || pTargRef->GetObjectReference() == pXMarker) {
            iRadius = 20;
            bExactPos = true;
        }
    }
    else if (pTargRef->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
        iRadius = 90;
        bExactPos = true;
    }
    else if (apMobileObject->GetSitSleepState() == SIT_SLEEP_IS_SITTING) {
        iRadius = 200;
        bExactPos = true;
    }
    else {
        iRadius = static_cast<int32_t>(pTargRef->GetModelBoundSize()) + 20;
    }
    iRadius += aiExtraRadius;
    double dZDist = std::fabs(dist.z);
    if (fPathPointZDistanceAllowance.Float() > dZDist) {
        dist.z = 0.0f;
    }
    bool bOverride = false;
    if (apMobileObject->IsAtPoint(targloc, static_cast<float>(iRadius), !bExactPos, false) && !bOverride) {
        bGoodPt = true;
    }
    if (pTargRef && pTargRef->IsFurniture() && apMobileObject->pCurrentProcess) {
        if (apMobileObject->pCurrentProcess->GetFurnitureMarker()) {
            NiPoint3 kOffset = targloc - apMobileObject->GetLocationOnReference();
            // The length is compared as a double against 100.0.
            if ((kOffset.XYLength() < 100.0 && pActor->GetSitSleepState() == SIT_SLEEP_IS_SITTING) ||
                pActor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
                bGoodPt = true;
            }
        }
    }

    if (Interface::GetPickREFR() == pActor) {
        float fDist = dist.XYLength();
        if (std::isfinite(fDist)) {
            char cMessage[308];
            std::sprintf(cMessage, "radius %d distance %.02f", iRadius, fDist);
            InitializationFlow(cMessage);
        }
    }
    return bGoodPt;
}

bool TESPackage::IsTargetAtLocation(MobileObject* apActor, int32_t aiExtraRadius) {
    if (!apActor || !apActor->GetCurrentTarget()) {
        return false;
    }
    TESObjectREFR* pRef = apActor->GetCurrentTarget();
    TESWorldSpace* pRefWorld = pRef->GetWorldSpace();
    TESObjectCELL* pRefIntCell = pRef->GetParentCell();
    if (pRefIntCell && !pRefIntCell->GetInterior()) {
        pRefIntCell = nullptr;
    }

    TESWorldSpace* pLocWorld = GetLocationWorld(apActor);
    TESObjectCELL* pLocIntCell = GetLocationCell(apActor);
    if (pLocIntCell && !pLocIntCell->GetInterior()) {
        pLocIntCell = nullptr;
    }

    bool bSameSpace = false;
    if (pLocIntCell || pRefIntCell) {
        if (pLocIntCell == pRefIntCell) {
            bSameSpace = true;
        }
    }
    else {
        bSameSpace = pLocWorld == pRefWorld;
    }
    if (!bSameSpace) {
        return false;
    }
    PackageLocation* pLocation = GetLocation();
    bool bInsideTrigger = false;
    if (pLocation && pLocation->IsRefAtLocationTrigger(apActor, pRef, bInsideTrigger)) {
        return bInsideTrigger;
    }
    int32_t iRadius = GetLocationRadius(apActor);
    if (!iRadius && pLocation) {
        TESObjectREFR* pLocationRef = GetLocationReference(apActor);
        if (pLocationRef) {
            iRadius = static_cast<int32_t>(pLocationRef->GetModelBoundSize());
        }
    }

    if (iRadius <= 0) {
        iRadius = iAIDistanceRadiusMinLocation.UInt();
    }
    iRadius += aiExtraRadius;
    bool bGoodPt = false;
    const NiPoint3& kRefPos = pRef->GetLocationOnReference();
    NiPoint3 kOffset = GetLocationCoord(apActor) - kRefPos;
    int32_t iDist = static_cast<int32_t>(kOffset.XYLength());
    if (iDist <= iRadius) {
        return true;
    }
    return bGoodPt;
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
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                sSize - sStartSize,
                pHeader->iFormID,
                pEditorID,
                pHeader->iFlags,
                7505,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Package.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s",
                sSize - sStartSize,
                7505,
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
        pSaveLoadGameOLD->SaveGameDataOLD(&iBlockID, sizeof(iBlockID));
        pBlockStart = pSaveLoadGameOLD->GetBufferPointer();
        pSaveLoadGameOLD->SaveGameDataOLD(&sBlockSize, sizeof(sBlockSize));
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&PackData, sizeof(PackData));
    uint8_t cHasParts = 0;
    if (pPackLoc) {
        cHasParts |= SAVED_LOCATION;
    }
    if (pPackTarg) {
        cHasParts |= SAVED_TARGET;
    }
    TESForm::SaveGameDataOLD(&cHasParts, sizeof(cHasParts));
    if (pPackLoc) {
        pPackLoc->SaveGame();
    }
    if (pPackTarg) {
        pPackTarg->SaveGame();
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&ePROCEDURE_TYPE, sizeof(ePROCEDURE_TYPE));
    if (bDebugSaveBuffer.Bool()) {
        char* pEnd = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
        SaveFormHeader* pHeader = pSaveLoadGameOLD->GetCurrentlySavingFormHeader();
        if (pHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            const char* pEditorID = pForm->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                static_cast<int32_t>(pEnd - pStart),
                pHeader->iFormID,
                pEditorID,
                pHeader->iFlags,
                7534,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Package.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s",
                static_cast<int32_t>(pEnd - pStart),
                7534,
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
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Package.cpp",
                7534
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
        pSaveLoadGameOLD->LoadGameDataOLD(&iMarker, sizeof(iMarker));
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
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Package.cpp",
                    7540,
                    pHeader->iFormID,
                    pEditorID,
                    cVersion,
                    iFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  "
                    "Current version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Package.cpp",
                    7540,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        pBlockStart = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
        pSaveLoadGameOLD->LoadGameDataOLD(&sBlockSize, sizeof(sBlockSize));
    }
    pSaveLoadGameOLD->LoadGameDataOLD(&PackData, sizeof(PackData));
    char cHasParts;
    TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&cHasParts), sizeof(cHasParts));
    if (static_cast<uint8_t>(cHasParts) & SAVED_LOCATION) {
        pPackLoc = new PackageLocation;
        pPackLoc->LoadGame();
    }
    if (static_cast<uint8_t>(cHasParts) & SAVED_TARGET) {
        pPackTarg = new PackageTarget;
        pPackTarg->LoadGame();
    }
    pSaveLoadGameOLD->LoadGameDataOLD(&ePROCEDURE_TYPE, sizeof(ePROCEDURE_TYPE));
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
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Package.cpp",
                    7563,
                    pHeader->iFormID,
                    pEditorID,
                    cVersion,
                    iFlags
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
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Package.cpp",
                    7563,
                    pHeader->iFormID,
                    pEditorID,
                    cVersion,
                    iFlags
                );
            }
        }
        else if (pBlockEnd > pBlockStart + sBlockSize) {
            uint8_t cVersion = pSaveLoadGameOLD->GetCurrentVersion();
            Warning(
                "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                static_cast<int32_t>(pBlockEnd - (pBlockStart + sBlockSize)),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Package.cpp",
                7563,
                cVersion
            );
        }
        else if (pBlockEnd < pBlockStart + sBlockSize) {
            uint8_t cVersion = pSaveLoadGameOLD->GetCurrentVersion();
            Warning(
                "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is %i",
                static_cast<int32_t>((pBlockStart + sBlockSize) - pBlockEnd),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Package.cpp",
                7563,
                cVersion
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
    uint32_t uiFormID = TESForm::GetFormID();
    if (pDataHandler->IsFormIDCreated(uiFormID)) {
        apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&PackData)), sizeof(PackData), 0);
        int8_t cHasParts = 0;
        if (pPackLoc) {
            cHasParts |= SAVED_LOCATION;
        }
        if (pPackTarg) {
            cHasParts |= SAVED_TARGET;
        }
        if (pPackData) {
            cHasParts |= SAVED_DATA;
        }
        apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&cHasParts)), sizeof(cHasParts), 0);
        if (pPackLoc) {
            pPackLoc->SaveGame(apSaveGameBuffer);
        }
        if (pPackTarg) {
            pPackTarg->SaveGame(apSaveGameBuffer);
        }
        if (pPackData) {
            pPackData->SaveGame(apSaveGameBuffer);
        }
        apSaveGameBuffer
            ->SaveDataEndian(static_cast<char*>(static_cast<void*>(&ePROCEDURE_TYPE)), sizeof(ePROCEDURE_TYPE), 0);
    }
}

// The package's part of the buffer LoadGame slot.
void TESPackage::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    if (pDataHandler->IsFormIDCreated(iFormID)) {
        apLoadGameBuffer->LoadData(&PackData, sizeof(PackData));
        uint8_t cSavedDataFlags = 0;
        apLoadGameBuffer->LoadData(&cSavedDataFlags, sizeof(cSavedDataFlags));
        if ((cSavedDataFlags & SAVED_LOCATION) != 0) {
            pPackLoc = new PackageLocation;
            pPackLoc->LoadGame(apLoadGameBuffer);
        }
        if ((cSavedDataFlags & SAVED_TARGET) != 0) {
            pPackTarg = new PackageTarget;
            pPackTarg->LoadGame(apLoadGameBuffer);
        }
        if ((cSavedDataFlags & SAVED_DATA) != 0) {
            pPackData->LoadGame(apLoadGameBuffer);
        }
        apLoadGameBuffer->LoadData(&ePROCEDURE_TYPE, sizeof(ePROCEDURE_TYPE));
    }
    else {
        if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_PACKAGE_NEVER_RUN)) {
            PackData.iPackFlags |= kPackageFlag_NeverToRun;
        }
        if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_PACKAGE_WAITING)) {
            PackData.iPackFlags |= kPackageFlag_Waiting;
        }
    }
}

void TESPackage::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    TESForm::LoadGame(aiFlags, aiCurrentFlags);
    if ((aiFlags & CHANGE_PACKAGE_NEVER_RUN) != 0) {
        PackData.iPackFlags |= kPackageFlag_NeverToRun;
    }
    if ((aiFlags & CHANGE_PACKAGE_WAITING) != 0) {
        PackData.iPackFlags |= kPackageFlag_Waiting;
    }
}

void TESPackage::InitLoadGame(BGSLoadFormBuffer* apLoadFormBuffer) {
    uint32_t uiFormID = GetFormID();
    if (pDataHandler->IsFormIDCreated(uiFormID)) {
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
    PackageLocation* pSecondLocation = GetPackageSecondLocation();
    if (pSecondLocation) {
        return pSecondLocation->GetRadius();
    }

    return 0;
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
        PackData.iPackFlags &= ~kPackageFlag_IsCreated;
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
    int32_t eType = TESPackage::GetProcedureType();
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
        PackData.iPackFlags &= ~kPackageFlag_DrawWeapon;
    }
}

bool TESPackage::GetUnlockDoorsAtStart() {
    return (PackData.iPackFlags & kPackageFlag_UnlockDoorsAtStart) != 0;
}

bool TESPackage::GetLockDoorsAtStart() {
    return (PackData.iPackFlags & kPackageFlag_LockDoorsAtStart) != 0;
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
        PackData.iPackFlags &= ~kPackageFlag_SelectiveFalloutBehavior;
    }
}

void TESPackage::SetCombatAlert(bool abVal) {
    if (abVal) {
        PackData.iPackFlags &= ~kPackageFlag_NoCombatAlert;
    }
    else {
        PackData.iPackFlags |= kPackageFlag_NoCombatAlert;
    }
}

void TESPackage::SetGuardWarnBehavior(bool abVal) {
    if (abVal) {
        PackData.iPackFlags &= ~kPackageFlag_NoWarnAttackBehavior;
    }
    else {
        PackData.iPackFlags |= kPackageFlag_NoWarnAttackBehavior;
    }
}

void TESPackage::SetFOEnableHellosToPlayer(bool abVal) {
    if (abVal) {
        PackData.iFOBehaviorFlags |= kFOBehaviorFlag_EnableHellosToPlayer;
    }
    else {
        PackData.iFOBehaviorFlags &= ~kFOBehaviorFlag_EnableHellosToPlayer;
    }
}

void TESPackage::SetFOEnableAggroRadiusBehavior(bool abVal) {
    if (abVal) {
        PackData.iFOBehaviorFlags |= kFOBehaviorFlag_EnableAggroRadiusBehavior;
    }
    else {
        PackData.iFOBehaviorFlags &= ~kFOBehaviorFlag_EnableAggroRadiusBehavior;
    }
}

const TESEscortPackageData* TESPackage::GetEscortPackageData() {
    if (GetPackType() == PACKAGE_TYPE_ESCORT) {
        return static_cast<const TESEscortPackageData*>(pPackData);
    }

    return nullptr;
}

const TESPatrolPackageData* TESPackage::GetPatrolPackageData() {
    if (GetPackType() == PACKAGE_TYPE_PATROL) {
        return static_cast<const TESPatrolPackageData*>(pPackData);
    }

    return nullptr;
}

PACKAGE_TYPE TESPackage::GetPackType() {
    return static_cast<PACKAGE_TYPE>(PackData.cPackType);
}

const TESUseWeaponPackageData* TESPackage::GetUseWeaponPackageData() {
    if (GetPackType() == PACKAGE_TYPE_USE_WEAPON) {
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
                break;
            }
            bool bFlag = false;
            if (!GetPackageTarget() ||
                (!GetPackageTarget()->GetTargReference() &&
                 !GetPackageTarget()->GetTargObject() &&
                 GetPackageTarget()->GetTargObjectType() == POTYPE_NONE)) {
                break;
            }
            PACK_TARG_TYPE eType = GetPackageTarget()->GetTargType();
            switch (eType) {
                case PTT_REFERENCE: {
                    TESObjectREFR* pTargRef = GetPackageTarget()->GetTargReference();
                    switch (pTargRef->GetObjectReference()->GetFormType()) {
                        case ACTI_ID:
                        case CONT_ID:
                        case DOOR_ID:
                        case STAT_ID:
                        case SCOL_ID:
                        case TREE_ID:
                        case FLOR_ID:
                        case FURN_ID:
                        case CREA_ID: {
                            bFlag = true;
                            break;
                        }
                        case NPC__ID: {
                            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_EXPLORE_NPC;
                            return;
                        }
                        default: {
                            break;
                        }
                    }
                    break;
                }
                case PTT_OBJECT: {
                    TESForm* pTargObject = GetPackageTarget()->GetTargObject();
                    switch (pTargObject->GetFormType()) {
                        case ACTI_ID:
                        case DOOR_ID:
                        case STAT_ID:
                        case SCOL_ID:
                        case TREE_ID:
                        case FLOR_ID:
                        case FURN_ID:
                        case CREA_ID: {
                            bFlag = true;
                            break;
                        }
                        case NPC__ID: {
                            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_EXPLORE_NPC;
                            return;
                        }
                        default: {
                            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_EXPLORE_ACQUIRE_GENERIC;
                            return;
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
                            bFlag = true;
                            break;
                        }
                        case POTYPE_NPC: {
                            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_EXPLORE_NPC;
                            return;
                        }
                        default: {
                            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_EXPLORE_ACQUIRE_GENERIC;
                            return;
                        }
                    }
                    break;
                }
                case PTT_LINKEDREF: {
                    if (apLinkedRef) {
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
                                bFlag = true;
                                break;
                            }
                            case NPC__ID: {
                                ePROCEDURE_TYPE = PACKAGE_PROCEDURE_EXPLORE_NPC;
                                return;
                            }
                            default: {
                                break;
                            }
                        }
                    }
                    break;
                }
                default: {
                    break;
                }
            }
            if (bFlag) {
                ePROCEDURE_TYPE = PACKAGE_PROCEDURE_EXPLORE_ACTIVATE;
            }
            else {
                ePROCEDURE_TYPE = PACKAGE_PROCEDURE_EXPLORE_ACQUIRE;
            }
            break;
        }
        case PACKAGE_TYPE_FOLLOW: {
            if (GetPackageTarget()) {
                PackageLocation* pLoc = GetPackageSecondLocation();
                if (pLoc &&
                    (pLoc->GetLocType() == PLT_LINKEDREF ||
                     pLoc->GetLocReference() ||
                     pLoc->GetLocObject() ||
                     pLoc->GetLocObjectType())) {
                    ePROCEDURE_TYPE = PACKAGE_PROCEDURE_AMBUSH_FOLLOW;
                }
                else {
                    ePROCEDURE_TYPE = PACKAGE_PROCEDURE_FOLLOW_WITHOUT_ESCORT;
                }
            }
            else {
                ePROCEDURE_TYPE = PACKAGE_PROCEDURE_NONE;
            }
            break;
        }
        case PACKAGE_TYPE_ESCORT: {
            bool bFlag = false;
            if (GetPackageTarget()) {
                PACK_TARG_TYPE eType = GetPackageTarget()->GetTargType();
                switch (eType) {
                    case PTT_REFERENCE: {
                        if (GetPackageTarget()->GetTargReference() &&
                            GetPackageTarget()->GetTargReference()->GetObjectReference()) {
                            if (GetPackageTarget()->GetTargReference()->GetObjectReference()->GetFormType() ==
                                    NPC__ID ||
                                GetPackageTarget()->GetTargReference()->GetObjectReference()->GetFormType() ==
                                    CREA_ID) {
                                bFlag = true;
                            }
                        }

                        break;
                    }
                    case PTT_OBJECT: {
                        if ((GetPackageTarget()->GetTargObject() &&
                             GetPackageTarget()->GetTargObject()->GetFormType() == NPC__ID) ||
                            GetPackageTarget()->GetTargObject()->GetFormType() == CREA_ID) {
                            bFlag = true;
                        }
                        break;
                    }
                    case PTT_TYPE: {
                        if (GetPackageTarget()->GetTargObjectType() == POTYPE_NPC ||
                            GetPackageTarget()->GetTargObjectType() == POTYPE_CREA) {
                            bFlag = true;
                        }
                        break;
                    }
                    case PTT_LINKEDREF: {
                        if (apLinkedRef &&
                            apLinkedRef->GetObjectReference() &&
                            (apLinkedRef->GetObjectReference()->GetFormType() == NPC__ID ||
                             apLinkedRef->GetObjectReference()->GetFormType() == CREA_ID)) {
                            bFlag = true;
                        }

                        break;
                    }
                    default: {
                        break;
                    }
                }
                if (bFlag) {
                    ePROCEDURE_TYPE = PACKAGE_PROCEDURE_ESCORT_ACTOR;
                }
                else {
                    ePROCEDURE_TYPE = PACKAGE_PROCEDURE_ESCORT_OBJECT;
                }
            }
            else {
                ePROCEDURE_TYPE = PACKAGE_PROCEDURE_NONE;
            }
            break;
        }
        case PACKAGE_TYPE_EAT: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_EAT;
            break;
        }
        case PACKAGE_TYPE_SLEEP: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_SLEEP;
            break;
        }
        case PACKAGE_TYPE_WANDER: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_EXPLORE_WANDER;
            break;
        }
        case PACKAGE_TYPE_TRAVEL: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_EXPLORE_TRAVEL;
            break;
        }
        case PACKAGE_TYPE_ACCOMPANY: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_ACCOMPANY;
            break;
        }
        case PACKAGE_TYPE_USE_ITEM_AT: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_USEITEMAT;
            break;
        }
        case PACKAGE_TYPE_AMBUSH: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_AMBUSH;
            break;
        }
        case PACKAGE_TYPE_FLEE_NON_COMBAT: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_FLEE_NON_COMBAT;
            break;
        }
        case PACKAGE_TYPE_SANDBOX: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_SANDBOX;
            break;
        }
        case PACKAGE_TYPE_PATROL: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_PATROL;
            break;
        }
        case PACKAGE_TYPE_GUARD: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_GUARD;
            break;
        }
        case PACKAGE_TYPE_DIALOGUE: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_DIALOGUE;
            break;
        }
        case PACKAGE_TYPE_USE_WEAPON: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_USEWEAPON;
            break;
        }
        case PACKAGE_TYPE_COMBAT: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_COMBAT;
            break;
        }
        case PACKAGE_TYPE_ACTIVATE: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_ACTIVATE;
            break;
        }
        case PACKAGE_TYPE_ALARM: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_ALARM;
            break;
        }
        case PACKAGE_TYPE_FLEE: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_FLEE;
            break;
        }
        case PACKAGE_TYPE_TRESPASS: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_TRESPASS;
            break;
        }
        case PACKAGE_TYPE_SPECTATOR: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_OBSERVE_COMBAT;
            break;
        }
        case PACKAGE_TYPE_GET_UP: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_GET_UP;
            break;
        }
        case PACKAGE_TYPE_DO_NOTHING: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_DO_NOTHING;
            break;
        }
        case PACKAGE_TYPE_IN_GAME_DIALOGUE: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_DIALOGUE;
            break;
        }
        case PACKAGE_TYPE_SURFACE: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_SURFACE;
            break;
        }
        case PACKAGE_TYPE_SEARCH_FOR_ATTACKER: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_SEARCH;
            break;
        }
        case PACKAGE_TYPE_AVOID_RADIATION: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_AVOID_RADIATION;
            break;
        }
        case PACKAGE_TYPE_REACT_TO_DESTROYED_OBJECT: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_EXPLOSION_REACTION;
            break;
        }
        case PACKAGE_TYPE_REACT_TO_GRENADE_OR_MINE: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_GRENADE_MINE_REACTION;
            break;
        }
        case PACKAGE_TYPE_STEAL_WARNING: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_STEAL_WARN;
            break;
        }
        case PACKAGE_TYPE_PICKPOCKET_WARNING: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_PICKPOCKET_WARN;
            break;
        }
        case PACKAGE_TYPE_MOVEMENT_BLOCKED: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_MOVEMENT_BLOCKED;
            break;
        }
        default: {
            ePROCEDURE_TYPE = PACKAGE_PROCEDURE_NONE;
            break;
        }
    }
    if (ePROCEDURE_TYPE == PACKAGE_PROCEDURE_NONE) {
        char cMessage[264];
        std::sprintf(
            cMessage,
            "Package '%s' is not a valid package because it is missing necessary Target Or Location Info.",
            GetFormEditorID()
        );
        ePROCEDURE_TYPE = PACKAGE_PROCEDURE_EXPLORE_TRAVEL;
        SetPackType(PACKAGE_TYPE_TRAVEL);
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

    switch (aeType) {
        case PACKAGE_TYPE_EXPLORE:
        case PACKAGE_TYPE_ACCOMPANY:
        case PACKAGE_TYPE_GUARD:
        case PACKAGE_TYPE_FIND:
            if (!GetPackageTarget()) {
                pPackTarg = new PackageTarget;
            }
            break;
        case PACKAGE_TYPE_FOLLOW:
            pPackData = new TESFollowPackageData;
            if (!GetPackageTarget()) {
                pPackTarg = new PackageTarget;
            }
            break;
        case PACKAGE_TYPE_ESCORT:
            if (!GetPackageTarget()) {
                pPackTarg = new PackageTarget;
            }
            if (!GetLocation()) {
                pPackLoc = new PackageLocation;
            }
            pPackData = new TESEscortPackageData;
            break;
        case PACKAGE_TYPE_EAT:
            if (!GetPackageTarget()) {
                pPackTarg = new PackageTarget;
            }
            if (pPackTarg->GetTargType() == PTT_REFERENCE) {
                pPackTarg->SetTargType(PTT_TYPE);
            }
            pPackData = new TESEatPackageData;
            if (!GetLocation()) {
                pPackLoc = new PackageLocation;
            }
            break;
        case PACKAGE_TYPE_SLEEP:
        case PACKAGE_TYPE_WANDER:
        case PACKAGE_TYPE_TRAVEL:
        case PACKAGE_TYPE_SANDBOX:
            if (!GetLocation()) {
                pPackLoc = new PackageLocation;
            }
            break;
        case PACKAGE_TYPE_USE_ITEM_AT:
            pPackData = new TESUseItemPackageData;
            if (!GetLocation()) {
                pPackLoc = new PackageLocation;
            }
            if (!GetPackageTarget()) {
                pPackTarg = new PackageTarget;
            }
            break;
        case PACKAGE_TYPE_AMBUSH:
            if (!GetLocation()) {
                pPackLoc = new PackageLocation;
            }
            pPackData = new TESAmbushPackageData;
            break;
        case PACKAGE_TYPE_PATROL: pPackData = new TESPatrolPackageData; break;
        case PACKAGE_TYPE_DIALOGUE:
        case PACKAGE_TYPE_IN_GAME_DIALOGUE:
            if (!GetPackageTarget()) {
                pPackTarg = new PackageTarget;
            }
            pPackData = new TESDialoguePackageData;
            break;
        case PACKAGE_TYPE_USE_WEAPON:
            SetDrawWeaponFlag(true);
            if (!GetPackageTarget()) {
                pPackTarg = new PackageTarget;
            }
            pPackData = new TESUseWeaponPackageData;
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
        PackageLocation* pEmpty = new PackageLocation;
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
    PackageTarget* pTarget = GetPackageTarget();
    if (!pTarget) {
        return 0;
    }
    if (pTarget->GetTargType() == PTT_REFERENCE || pTarget->GetTargType() == PTT_LINKEDREF) {
        return 1;
    }
    int32_t iCount = pTarget->iValue;
    if (iCount > 0) {
        return iCount;
    }
    if (ePROCEDURE_TYPE == PACKAGE_PROCEDURE_USEITEMAT) {
        return 1;
    }
    if (ePROCEDURE_TYPE == PACKAGE_PROCEDURE_EXPLORE_ACTIVATE ||
        ePROCEDURE_TYPE == PACKAGE_PROCEDURE_EXPLORE_ACQUIRE ||
        ePROCEDURE_TYPE == PACKAGE_PROCEDURE_EXPLORE_ACQUIRE_GENERIC) {
        return std::numeric_limits<int32_t>::max();
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
        packSched.Copy(&kDefault);
    }
}

void TESPackage::SetPackageLocation(PackageLocation* apPackLoc) {
    if (apPackLoc) {
        if (!pPackLoc) {
            pPackLoc = new PackageLocation;
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
    if (GetPackType() == PACKAGE_TYPE_DIALOGUE || GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
        TESDialoguePackageData* pData;
        if (!pPackData) {
            pData = new TESDialoguePackageData;
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
            pData = new TESEatPackageData;
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
            pData = new TESEscortPackageData;
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
            pData = new TESAmbushPackageData;
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
            pData = new TESFollowPackageData;
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
            pData = new TESUseItemPackageData;
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
            *ppSecondLoc = new PackageLocation;
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
            pPackTarg = new PackageTarget;
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

// Each test reloads the type data and asks for the type again.
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
    PackageLocation* pLocation;
    switch (GetPackType()) {
        case PACKAGE_TYPE_ESCORT:
        case PACKAGE_TYPE_EAT:
        case PACKAGE_TYPE_USE_ITEM_AT:
        case PACKAGE_TYPE_USE_WEAPON: {
            PackageLocation* pSecondLocation;
            pSecondLocation = GetPackageSecondLocation();
            if (!pSecondLocation || pSecondLocation->GetLocType() != PLT_FIRSTLOC) {
                pLocation = GetPackageSecondLocation();
                break;
            }
            [[fallthrough]];
        }
        case PACKAGE_TYPE_EXPLORE:
        case PACKAGE_TYPE_SANDBOX:
        case PACKAGE_TYPE_FIND:
        case PACKAGE_TYPE_IN_GAME_DIALOGUE: {
            pLocation = GetLocation();
            break;
        }
        default: {
            pLocation = nullptr;
            break;
        }
    }
    return pLocation;
}

TESUseWeaponPackageData* TESPackage::GetOrCreateUseWeaponPackageData() {
    if (GetPackType() != PACKAGE_TYPE_USE_WEAPON) {
        fr2Assert(const_cast<char*>("D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Package.cpp"), 1543);
    }
    if (pPackData) {
        return static_cast<TESUseWeaponPackageData*>(pPackData);
    }
    TESUseWeaponPackageData* pData = new TESUseWeaponPackageData;
    pPackData = pData;
    return pData;
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
                if (apActor &&
                    apActor->GetCurrentAIProcess() &&
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
                if (apActor &&
                    apActor->GetCurrentAIProcess() &&
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
    NiPoint3 kCoord = NiPoint3::ZERO;
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
                        "AI: %s is assigned a linked reference and has none for a package\n",
                        apActor->GetFormEditorID()
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
                if (apActor &&
                    apActor->GetCurrentAIProcess() &&
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
                if (apActor &&
                    apActor->GetCurrentAIProcess() &&
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
    int32_t iRadius = 0;
    Actor* pActor = nullptr;
    if (apMobileObject->IsActor()) {
        pActor = static_cast<Actor*>(apMobileObject);
    }
    TESObjectREFR* pTargRef = apMobileObject->GetCurrentTarget();
    if (pTargRef) {
        if (pTargRef->IsFurniture()) {
            iRadius = 10;
        }
        else if (pTargRef->GetObjectReference() == pXMarkerHeading || pTargRef->GetObjectReference() == pXMarker) {
            iRadius = 20;
        }
        else {
            // pActor is null when the object is not an actor; GetExtra is called on it anyway.
            // The call stays out of line so clang cannot fold the null case into apMobileObject.
            TESPackage* pEdiPack = pActor->GetExtra()->GetPackageExtra();
            if (GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE && pEdiPack && pEdiPack->GetPackageTarget()) {
                iRadius = pEdiPack->GetPackageTarget()->iValue;
            }
            else if (GetPackageTarget() && ePROCEDURE_TYPE != PACKAGE_PROCEDURE_EXPLORE_ACQUIRE_GENERIC) {
                iRadius = GetPackageTarget()->iValue;
            }
            if (!iRadius && !pTargRef->IsActor()) {
                iRadius = static_cast<int32_t>(pTargRef->GetModelBoundSize());
                switch (pTargRef->GetObjectReference()->GetFormType()) {
                    case ACTI_ID:
                    case CONT_ID:
                    case STAT_ID:
                    case SCOL_ID: {
                        break;
                    }
                    default: {
                        if (pActor) {
                            float fReach = pActor->GetReach();
                            iRadius += static_cast<int32_t>(fReach);
                            double dEyeLevel = pActor->GetEyeLevel();
                            float fActZ = pActor->GetLocationOnReference().z + dEyeLevel - fReach;
                            float fItemZ = pTargRef->GetLocationOnReference().z;
                            if (fItemZ < static_cast<double>(fActZ)) {
                                double dHeightDiff = fActZ - fItemZ;
                                if (apMobileObject->GetBoundMax().z / 2.0 > dHeightDiff) {
                                    iRadius += static_cast<int32_t>(apMobileObject->GetBoundMax().z / 2.0);
                                }
                            }
                        }
                        break;
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
    if (GetPackageTarget() && GetPackageTarget()->GetTargReference() == pref) {
        if (abVal) {
            PackData.iPackFlags |= kPackageFlag_NeverToRun;
            if (!pDataHandler->IsFormIDCreated(iFormID)) {
                AddChange(CHANGE_PACKAGE_NEVER_RUN);
            }
        }
        else {
            PackData.iPackFlags &= ~kPackageFlag_NeverToRun;
            if (!pDataHandler->IsFormIDCreated(iFormID)) {
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
                if (apActor &&
                    apActor->pCurrentProcess &&
                    apActor->pCurrentProcess->GetPackageThatIsRunning() == this) {
                    TESObjectREFR* pRef = apActor->pCurrentProcess->GetGenericLocation();
                    if (pRef) {
                        pRetCell = pRef->GetParentCell();
                    }
                    else {
                        pRetCell = apActor->GetParentCell();
                    }
                }
                else if (apActor == pPlayer && GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
                    DialoguePackage* pDialogue = static_cast<DialoguePackage*>(this);
                    if (pDialogue->pTargetLocation) {
                        pRetCell = pDialogue->pTargetLocation->GetParentCell();
                    }
                }
                break;
            }
            case PLT_LINKEDREF: {
                if (apActor && apActor->GetLinkedRef()) {
                    pRetCell = apActor->GetLinkedRef()->GetParentCell();
                }

                break;
            }
            default: {
                break;
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
        afResult = static_cast<float>(GetLocationRadius(apActor));
        return true;
    }
    PackageTarget* pTarget = GetPackageTarget();
    if (!pTarget) {
        return false;
    }
    float fRadius = pTarget->GetTargAcquireRadius();
    // A NaN radius is taken as found.
    if (!(fRadius <= 0.0)) {
        afResult = fRadius;
        return true;
    }
    return false;
}

// Reads the location's radius twice.
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

TESObjectREFR* TESPackage::GetEscortTargetForPackage(Actor* apActor, bool abSkipExistingFollowers) {
    if (!GetPackageTarget()) {
        return nullptr;
    }
    PACK_TARG_TYPE eType = GetPackageTarget()->GetTargType();
    TESObjectREFR* pTarget = nullptr;
    switch (eType) {
        case PTT_REFERENCE:
            pTarget = GetPackageTarget()->GetTargReference();
            if (pTarget && pTarget->GetDelete() && pTarget->GetExtra()) {
                pTarget = pTarget->GetExtra()->GetReferencePointer();
            }
            break;
        case PTT_OBJECT: {
            TESEvaluateEscortContext escortContext;
            // Starts null (a departure): set only for a bound object, and EvaluateEscortReference matches nothing
            // without one; the exe calls IsBoundObject on an unset pointer.
            escortContext.pObject = nullptr;
            escortContext.pRequester = apActor;
            escortContext.bSkipExistingFollowers = abSkipExistingFollowers;
            TESObject* pObject = nullptr;
            if (GetPackageTarget()->GetTargObject() && GetPackageTarget()->GetTargObject()->IsBoundObject()) {
                pObject = static_cast<TESObject*>(GetPackageTarget()->GetTargObject());
                escortContext.pObject = pObject;
            }
            if (pPlayer && pObject == pPlayer->GetObjectReference()) {
                pTarget = pPlayer;
            }
            else {
                // The arguments are evaluated right to left.
                float fRadius2 = fTargetSearchRadius.Float();
                const NiPoint3* pPoint2 = &apActor->GetLocationOnReference();
                float fRadius1 = fTargetSearchRadius.Float();
                const NiPoint3* pPoint1 = &apActor->GetLocationOnReference();
                TESObjectCELL* pCell = apActor->GetParentCell();
                pDataHandler->EnumReferencesCloseToPoint(
                    pCell,
                    *pPoint1,
                    fRadius1,
                    *pPoint2,
                    fRadius2,
                    &TESPackage::EvaluateEscortReference,
                    &escortContext
                );
                pTarget = pTempTarget;
                pTempTarget = nullptr;
            }
            break;
        }
        case PTT_TYPE: {
            TESEvaluateEscortContext escortContext;
            // Starts null (a departure): a type target never sets it, so EvaluateEscortReference matches nothing; the
            // exe calls IsBoundObject on an unset pointer.
            escortContext.pObject = nullptr;
            escortContext.pRequester = apActor;
            escortContext.bSkipExistingFollowers = abSkipExistingFollowers;
            float fRadius2 = fTargetSearchRadius.Float();
            const NiPoint3* pPoint2 = &apActor->GetLocationOnReference();
            float fRadius1 = fTargetSearchRadius.Float();
            const NiPoint3* pPoint1 = &apActor->GetLocationOnReference();
            TESObjectCELL* pCell = apActor->GetParentCell();
            pDataHandler->EnumReferencesCloseToPoint(
                pCell,
                *pPoint1,
                fRadius1,
                *pPoint2,
                fRadius2,
                &TESPackage::EvaluateEscortReference,
                &escortContext
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
    Actor* pAct = static_cast<Actor*>(pTarget);
    if (GetPackType() == PACKAGE_TYPE_FOLLOW || GetPackType() == PACKAGE_TYPE_ACCOMPANY) {
        // The player takes a new follower only while he has room for one, unless he commands the actor already.
        if (pTarget != pPlayer ||
            apActor->GetCurrentAIProcess()->GetCommandingActor() == pPlayer ||
            pPlayer->IsPlayerFollower(apActor) ||
            pPlayer->GetNumberOfFollowers() <= static_cast<int32_t>(iNumberActorsAllowedToFollowPlayer.UInt())) {
            pAct->AddFollower(apActor);
        }
        else {
            apActor->GetCurrentAIProcess()->AddToProcedureIndexRunning(apActor, 0xFFFFFFFF);
            Interface::ShowMessage(sNoMoreFollowers.String(), 0, nullptr, nullptr, 2.0f, false);
        }
    }
    else if (GetPackType() == PACKAGE_TYPE_ESCORT && pAct != pPlayer) {
        apActor->AddFollower(pAct);
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
    return (PackData.iPackFlags & kPackageFlag_AllowSwimming) != 0;
}

void TESPackage::SetAllowsSwim(bool abVal) {
    if (abVal) {
        PackData.iPackFlags |= kPackageFlag_AllowSwimming;
    }
    else {
        PackData.iPackFlags &= ~kPackageFlag_AllowSwimming;
    }
}

bool TESPackage::GetAllowFalls() {
    return (PackData.iPackFlags & kPackageFlag_AllowFalls) != 0;
}

void TESPackage::SetAllowFalls(bool abVal) {
    if (abVal) {
        PackData.iPackFlags |= kPackageFlag_AllowFalls;
    }
    else {
        PackData.iPackFlags &= ~kPackageFlag_AllowFalls;
    }
}

void TESPackage::SetForceUnequipWeapon(bool abVal) {
    if (abVal) {
        PackData.iPackFlags |= kPackageFlag_ForceUnequipWeapon;
    }
    else {
        PackData.iPackFlags &= ~kPackageFlag_ForceUnequipWeapon;
    }
}

void TESPackage::SetNoHeadTrack(bool abVal) {
    if (abVal) {
        PackData.iPackFlags |= kPackageFlag_NoHeadTrack;
    }
    else {
        PackData.iPackFlags &= ~kPackageFlag_NoHeadTrack;
    }
}

bool TESPackage::GetContinueDuringCombat() {
    return (PackData.iPackFlags & kPackageFlag_ContinueDuringCombat) != 0;
}

void TESPackage::SetContinueDuringCombat(bool abVal) {
    if (abVal) {
        PackData.iPackFlags |= kPackageFlag_ContinueDuringCombat;
    }
    else {
        PackData.iPackFlags &= ~kPackageFlag_ContinueDuringCombat;
    }
}

bool TESPackage::GetCombatAlert() {
    return (PackData.iPackFlags & kPackageFlag_NoCombatAlert) == 0;
}

bool TESPackage::GetGuardWarnBehavior() {
    return (PackData.iPackFlags & kPackageFlag_NoWarnAttackBehavior) == 0;
}

bool TESPackage::GetFOEnableHellosToPlayer() {
    return (PackData.iFOBehaviorFlags & kFOBehaviorFlag_EnableHellosToPlayer) != 0;
}

void TESPackage::SetFOEnableRandomConversations(bool abVal) {
    if (abVal) {
        PackData.iFOBehaviorFlags |= kFOBehaviorFlag_EnableRandomConversations;
    }
    else {
        PackData.iFOBehaviorFlags &= ~kFOBehaviorFlag_EnableRandomConversations;
    }
}

bool TESPackage::GetFOEnableObserveCombatBehavior() {
    return (PackData.iFOBehaviorFlags & kFOBehaviorFlag_EnableObserveCombatBehavior) != 0;
}

void TESPackage::SetFOEnableObserveCombatBehavior(bool abVal) {
    if (abVal) {
        PackData.iFOBehaviorFlags |= kFOBehaviorFlag_EnableObserveCombatBehavior;
    }
    else {
        PackData.iFOBehaviorFlags &= ~kFOBehaviorFlag_EnableObserveCombatBehavior;
    }
}

bool TESPackage::GetFOEnableGreetCorpseBehavior() {
    return (PackData.iFOBehaviorFlags & kFOBehaviorFlag_EnableGreetCorpseBehavior) != 0;
}

void TESPackage::SetFOEnableGreetCorpseBehavior(bool abVal) {
    if (abVal) {
        PackData.iFOBehaviorFlags |= kFOBehaviorFlag_EnableGreetCorpseBehavior;
    }
    else {
        PackData.iFOBehaviorFlags &= ~kFOBehaviorFlag_EnableGreetCorpseBehavior;
    }
}

bool TESPackage::GetFOEnableReactionToPlayerActions() {
    return (PackData.iFOBehaviorFlags & kFOBehaviorFlag_EnableReactionToPlayerActions) != 0;
}

void TESPackage::SetFOEnableReactionToPlayerActions(bool abVal) {
    if (abVal) {
        PackData.iFOBehaviorFlags |= kFOBehaviorFlag_EnableReactionToPlayerActions;
    }
    else {
        PackData.iFOBehaviorFlags &= ~kFOBehaviorFlag_EnableReactionToPlayerActions;
    }
}

bool TESPackage::GetFOEnableFriendlyFireComments() {
    return (PackData.iFOBehaviorFlags & kFOBehaviorFlag_EnableFriendlyFireComments) != 0;
}

void TESPackage::SetFOEnableFriendlyFireComments(bool abVal) {
    if (abVal) {
        PackData.iFOBehaviorFlags |= kFOBehaviorFlag_EnableFriendlyFireComments;
    }
    else {
        PackData.iFOBehaviorFlags &= ~kFOBehaviorFlag_EnableFriendlyFireComments;
    }
}

bool TESPackage::GetFOEnableAggroRadiusBehavior() {
    return (PackData.iFOBehaviorFlags & kFOBehaviorFlag_EnableAggroRadiusBehavior) != 0;
}

bool TESPackage::GetFOEnableIdleChatterBehavior() {
    return (PackData.iFOBehaviorFlags & kFOBehaviorFlag_EnableIdleChatterBehavior) != 0;
}

void TESPackage::SetFOEnableIdleChatterBehavior(bool abVal) {
    if (abVal) {
        PackData.iFOBehaviorFlags |= kFOBehaviorFlag_EnableIdleChatterBehavior;
    }
    else {
        PackData.iFOBehaviorFlags &= ~kFOBehaviorFlag_EnableIdleChatterBehavior;
    }
}

bool TESPackage::GetFOEnableAvoidRadiationBehavior() {
    return (PackData.iFOBehaviorFlags & kFOBehaviorFlag_EnableAvoidRadiationBehavior) != 0;
}

void TESPackage::SetFOEnableAvoidRadiationBehavior(bool abVal) {
    if (abVal) {
        PackData.iFOBehaviorFlags |= kFOBehaviorFlag_EnableAvoidRadiationBehavior;
    }
    else {
        PackData.iFOBehaviorFlags &= ~kFOBehaviorFlag_EnableAvoidRadiationBehavior;
    }
}

void TESPackage::SetScriptPackage(bool abVal) {
    if (abVal) {
        PackData.iPackFlags |= kPackageFlag_ScriptPackage;
    }
    else {
        PackData.iPackFlags &= ~kPackageFlag_ScriptPackage;
    }
}

void TESPackage::SetWaiting(bool abVal) {
    if (abVal) {
        PackData.iPackFlags |= kPackageFlag_Waiting;
        if (!pDataHandler->IsFormIDCreated(iFormID)) {
            AddChange(CHANGE_PACKAGE_WAITING);
        }
    }
    else {
        PackData.iPackFlags &= ~kPackageFlag_Waiting;
        if (!pDataHandler->IsFormIDCreated(iFormID)) {
            RemoveChange(CHANGE_PACKAGE_WAITING);
        }
    }
}

bool TESPackage::GetDefensive() {
    return (PackData.iPackFlags & kPackageFlag_DefensiveCombat) != 0;
}

void TESPackage::SetDefensive(bool abVal) {
    if (abVal) {
        PackData.iPackFlags |= kPackageFlag_DefensiveCombat;
    }
    else {
        PackData.iPackFlags &= ~kPackageFlag_DefensiveCombat;
    }
}

bool TESPackage::GetAlwaysRun() {
    return (PackData.iPackFlags & kPackageFlag_AlwaysRun) != 0;
}

void TESPackage::SetAlwaysRun(bool abVal) {
    if (abVal) {
        PackData.iPackFlags |= kPackageFlag_AlwaysRun;
    }
    else {
        PackData.iPackFlags &= ~kPackageFlag_AlwaysRun;
    }
}

void TESPackage::SetAlwaysSneak(bool abVal) {
    if (abVal) {
        PackData.iPackFlags |= kPackageFlag_AlwaysSneak;
    }
    else {
        PackData.iPackFlags &= ~kPackageFlag_AlwaysSneak;
    }
}

void TESPackage::SetMustReachLocation(bool abVal) {
    if (abVal) {
        PackData.iPackFlags |= kPackageFlag_MustReachLocation;
    }
    else {
        PackData.iPackFlags &= ~kPackageFlag_MustReachLocation;
    }
}

void TESPackage::SetMustComplete(bool abVal) {
    if (abVal) {
        PackData.iPackFlags |= kPackageFlag_MustComplete;
    }
    else {
        PackData.iPackFlags &= ~kPackageFlag_MustComplete;
    }
}

bool TESPackage::GetServicesEnabled() {
    return (PackData.iPackFlags & kPackageFlag_OffersServices) != 0;
}

void TESPackage::SetPretendInCombat(bool abVal) {
    if (abVal) {
        PackData.iPackFlags |= kPackageFlag_PretendInCombat;
    }
    else {
        PackData.iPackFlags &= ~kPackageFlag_PretendInCombat;
    }
}

bool TESPackage::GetWaiting() {
    return (PackData.iPackFlags & kPackageFlag_Waiting) != 0;
}

uint32_t TESPackage::DecRefCount() {
    return NiAtomicDecrement(uiRefCount);
}

bool TESPackage::SearchGetAllowBuying() {
    return (PackData.iPackageSpecificFlags & kPackSpecificFlag_SearchAllowBuying) != 0;
}

bool TESPackage::SearchGetAllowKilling() {
    return (PackData.iPackageSpecificFlags & kPackSpecificFlag_SearchAllowKilling) != 0;
}

bool TESPackage::SearchGetAllowStealing() {
    return (PackData.iPackageSpecificFlags & kPackSpecificFlag_SearchAllowStealing) != 0;
}

bool TESPackage::AmbushGetHideWhileAmbushing() {
    return (PackData.iPackageSpecificFlags & kPackSpecificFlag_AmbushHideWhileAmbushing) != 0;
}

bool TESPackage::SandboxGetAllowConversation() {
    return (PackData.iPackageSpecificFlags & kPackSpecificFlag_NoConversation) == 0;
}

bool TESPackage::GetPretendInCombat() {
    return (PackData.iPackFlags & kPackageFlag_PretendInCombat) != 0;
}

bool TESPackage::SandboxGetAllowEating() {
    return (PackData.iPackageSpecificFlags & kPackSpecificFlag_NoEating) == 0;
}

bool TESPackage::SandboxGetAllowSleeping() {
    return (PackData.iPackageSpecificFlags & kPackSpecificFlag_NoSleeping) == 0;
}

bool TESPackage::SandboxGetAllowFurniture() {
    return (PackData.iPackageSpecificFlags & kPackSpecificFlag_NoFurniture) == 0;
}

bool TESPackage::SandboxGetAllowIdleMarkers() {
    return (PackData.iPackageSpecificFlags & kPackSpecificFlag_NoIdleMarkers) == 0;
}

bool TESPackage::SandboxGetAllowWandering() {
    return (PackData.iPackageSpecificFlags & kPackSpecificFlag_NoWandering) == 0;
}

TESIdleForm* TESPackage::GetOnChangeIdle() {
    return OnChange.pIdle;
}

TESIdleForm* TESPackage::GetOnEndIdle() {
    return OnEnd.pIdle;
}

bool TESPackage::GetDialogueSayToFlag() {
    if (GetPackType() != PACKAGE_TYPE_DIALOGUE && GetPackType() != PACKAGE_TYPE_IN_GAME_DIALOGUE) {
        return false;
    }
    TESDialoguePackageData* pPackageData = static_cast<TESDialoguePackageData*>(pPackData);
    if (!pPackageData) {
        return false;
    }
    return pPackageData->bSayTo;
}

TESTopic* TESPackage::GetDialogueTopic() {
    if (GetPackType() != PACKAGE_TYPE_DIALOGUE && GetPackType() != PACKAGE_TYPE_IN_GAME_DIALOGUE) {
        return nullptr;
    }
    TESDialoguePackageData* pPackageData = static_cast<TESDialoguePackageData*>(pPackData);
    if (!pPackageData) {
        return nullptr;
    }
    return pPackageData->ptopic;
}

bool TESPackage::GetDialogueDoNotControlTarget() {
    if (GetPackType() != PACKAGE_TYPE_DIALOGUE && GetPackType() != PACKAGE_TYPE_IN_GAME_DIALOGUE) {
        return false;
    }
    TESDialoguePackageData* pPackageData = static_cast<TESDialoguePackageData*>(pPackData);
    if (!pPackageData) {
        return false;
    }
    return pPackageData->bDoNotControlTarget;
}

bool TESPackage::GetDialogueNoHeadtrack() {
    if (GetPackType() != PACKAGE_TYPE_DIALOGUE && GetPackType() != PACKAGE_TYPE_IN_GAME_DIALOGUE) {
        return false;
    }
    TESDialoguePackageData* pPackageData = static_cast<TESDialoguePackageData*>(pPackData);
    if (!pPackageData) {
        return false;
    }
    return pPackageData->bNOHeadtracking;
}

float TESPackage::GetDialogueFOV() {
    float fFov = 1.0f;
    if (GetPackType() != PACKAGE_TYPE_DIALOGUE && GetPackType() != PACKAGE_TYPE_IN_GAME_DIALOGUE) {
        return fFov;
    }
    TESDialoguePackageData* pPackageData = static_cast<TESDialoguePackageData*>(pPackData);
    if (!pPackageData) {
        return fFov;
    }
    return pPackageData->fFov;
}

uint32_t TESPackage::GetSearchLocationRadius(MobileObject* apActor) {
    PackageLocation* pSearchLocation = GetPackageSearchLocation();
    if (pSearchLocation == GetLocation()) {
        return GetLocationRadius(apActor);
    }

    return GetSecondLocationRadius();
}

void TESPackage::CopyAllFlags(TESPackage* apCopy) {
    SetSelectiveFalloutBehavior(apCopy->GetSelectiveFalloutBehavior());
    SetFOEnableHellosToPlayer(apCopy->GetFOEnableHellosToPlayer());
    SetFOEnableRandomConversations(apCopy->GetFOEnableRandomConversations());
    SetFOEnableObserveCombatBehavior(apCopy->GetFOEnableObserveCombatBehavior());
    SetFOEnableGreetCorpseBehavior(apCopy->GetFOEnableGreetCorpseBehavior());
    SetFOEnableReactionToPlayerActions(apCopy->GetFOEnableReactionToPlayerActions());
    SetFOEnableFriendlyFireComments(apCopy->GetFOEnableFriendlyFireComments());
    SetFOEnableAggroRadiusBehavior(apCopy->GetFOEnableAggroRadiusBehavior());
    SetFOEnableIdleChatterBehavior(apCopy->GetFOEnableIdleChatterBehavior());
    SetFOEnableAvoidRadiationBehavior(apCopy->GetFOEnableAvoidRadiationBehavior());
    SetDrawWeaponFlag(apCopy->GetDrawWeaponFlag());
    SetNoHeadTrack(apCopy->GetNoHeadTrack());
    SetForceUnequipWeapon(apCopy->GetForceUnequipWeapon());
    SetAlwaysRun(apCopy->GetAlwaysRun());
    SetAlwaysSneak(apCopy->GetAlwaysSneak());
    SetCombatAlert(apCopy->GetCombatAlert());
    TESPackage::SetCombatStyle(apCopy->TESPackage::GetCombatStyle());
    SetContinueDuringCombat(apCopy->GetContinueDuringCombat());
    SetDefensive(apCopy->GetDefensive());
    SetAllowFalls(apCopy->GetAllowFalls());
    SetAllowsSwim(apCopy->GetAllowsSwim());
    SetGuardWarnBehavior(apCopy->GetGuardWarnBehavior());
}

void TESPackage::SetDialogueTopic(TESTopic* apTopic) {
    if (GetPackType() == PACKAGE_TYPE_DIALOGUE || GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
        TESDialoguePackageData* pPackageData;
        if (pPackData) {
            pPackageData = static_cast<TESDialoguePackageData*>(pPackData);
        }
        else {
            pPackageData = new TESDialoguePackageData;
            pPackData = pPackageData;
        }
        pPackageData->ptopic = apTopic;
    }
}

void TESPackage::SetDialogueDoNotControlTarget(bool abControl) {
    if (GetPackType() == PACKAGE_TYPE_DIALOGUE || GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
        TESDialoguePackageData* pPackageData;
        if (pPackData) {
            pPackageData = static_cast<TESDialoguePackageData*>(pPackData);
        }
        else {
            pPackageData = new TESDialoguePackageData;
            pPackData = pPackageData;
        }
        pPackageData->bDoNotControlTarget = abControl;
    }
}

void TESPackage::SetPackageConditions(TESCondition* apPackCond) {
    if (apPackCond) {
        packConditions.Copy(apPackCond, nullptr);
    }
    else {
        TESCondition kTemp;
        packConditions.Copy(&kTemp, nullptr);
    }
}

TESTopic* TESPackage::GetOnChangeTopicInfo() {
    return OnChange.pTopic;
}

TESWorldSpace* TESPackage::GetSecondLocationWorld(MobileObject* apActor) {
    TESWorldSpace* pRetWorld = nullptr;
    PackageLocation* pLoc = GetPackageSecondLocation();
    if (!pLoc || pLoc->GetLocType() == PLT_CURRLOC) {
        if (apActor) {
            return apActor->GetPackageStartLocationWorld();
        }
    }
    else {
        switch (pLoc->GetLocType()) {
            case PLT_REFERENCE: {
                if (pLoc->GetLocReference()) {
                    pRetWorld = pLoc->GetLocReference()->GetWorldSpace();
                }
                break;
            }
            case PLT_CELL: {
                if (pLoc->GetLocCell()) {
                    pRetWorld = pLoc->GetLocCell()->GetWorldSpace();
                }
                break;
            }
            case PLT_EDITORLOC: {
                if (apActor) {
                    pRetWorld = apActor->GetEditorLocationWorld();
                }
                break;
            }
            case PLT_OBJECT:
            case PLT_TYPE: {
                if (apActor &&
                    apActor->pCurrentProcess &&
                    apActor->pCurrentProcess->GetPackageThatIsRunning() == this) {
                    TESObjectREFR* pRef = apActor->pCurrentProcess->GetSecondGenericLocation();
                    if (pRef) {
                        pRetWorld = pRef->GetWorldSpace();
                    }
                    else {
                        pRetWorld = apActor->GetWorldSpace();
                    }
                }
                else if (apActor == pPlayer && GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
                    DialoguePackage* pDialogue = static_cast<DialoguePackage*>(this);
                    if (pDialogue->pTargetLocation) {
                        pRetWorld = pDialogue->pTargetLocation->GetWorldSpace();
                    }
                }
                break;
            }
            case PLT_LINKEDREF: {
                if (apActor && apActor->GetLinkedRef()) {
                    pRetWorld = apActor->GetLinkedRef()->GetWorldSpace();
                }

                break;
            }
            default: {
                break;
            }
        }
    }
    return pRetWorld;
}

NiPoint3 TESPackage::GetSecondLocationCoord(MobileObject* apActor) {
    PackageLocation* pLoc = GetPackageSecondLocation();
    NiPoint3 locPt = NiPoint3::ZERO;
    if (!pLoc || pLoc->GetLocType() == PLT_CURRLOC) {
        if (apActor) {
            locPt = apActor->GetPackageStartLocationCoord();
        }
    }
    else {
        switch (pLoc->GetLocType()) {
            case PLT_REFERENCE: {
                if (pLoc->GetLocReference()) {
                    locPt = pLoc->GetLocReference()->GetLocationOnReference();
                }
                break;
            }
            case PLT_CELL: {
                TESObjectCELL* pCell = pLoc->GetLocCell();
                if (pCell) {
                    TESObjectREFR* pRef = pCell->GetFirstRefr();
                    if (pRef) {
                        locPt = pRef->GetLocationOnReference();
                    }
                    else if (!pCell->GetInterior()) {
                        locPt.x = static_cast<float>(pLoc->GetLocCell()->GetDataX() << 12);
                        locPt.y = static_cast<float>(pLoc->GetLocCell()->GetDataY() << 12);
                        locPt.z = 0.0f;
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
                if (apActor &&
                    apActor->pCurrentProcess &&
                    apActor->pCurrentProcess->GetPackageThatIsRunning() == this) {
                    TESObjectREFR* pRef = apActor->pCurrentProcess->GetSecondGenericLocation();
                    locPt = pRef ? pRef->GetLocationOnReference() : apActor->GetLocationOnReference();
                }
                else if (apActor == pPlayer && GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
                    DialoguePackage* pDialogue = static_cast<DialoguePackage*>(this);
                    if (pDialogue->pTargetLocation) {
                        locPt = pDialogue->pTargetLocation->GetLocationOnReference();
                    }
                }
                break;
            }
            case PLT_LINKEDREF: {
                if (apActor && apActor->GetLinkedRef()) {
                    locPt = apActor->GetLinkedRef()->GetLocationOnReference();
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
    TESObjectREFR* pRef = apActor->GetCurrentTarget();
    if (!apActor || !pRef) {
        return false;
    }
    TESWorldSpace* pRefWorld = pRef->GetWorldSpace();
    TESObjectCELL* pRefIntCell = pRef->GetParentCell();
    if (pRefIntCell && !pRefIntCell->GetInterior()) {
        pRefIntCell = nullptr;
    }

    TESWorldSpace* pLocWorld = GetLocationWorld(apActor);
    TESObjectCELL* pLocIntCell = GetLocationCell(apActor);
    if (pLocIntCell && !pLocIntCell->GetInterior()) {
        pLocIntCell = nullptr;
    }

    bool bSameSpace = false;
    if (pLocIntCell || pRefIntCell) {
        if (pLocIntCell == pRefIntCell) {
            bSameSpace = true;
        }
    }
    else {
        bSameSpace = pLocWorld == pRefWorld;
    }
    if (!bSameSpace) {
        return false;
    }
    PackageLocation* pSecondLocation = GetPackageSecondLocation();
    bool bInsideTrigger = false;
    if (pSecondLocation && pSecondLocation->IsRefAtLocationTrigger(apActor, pRef, bInsideTrigger)) {
        return bInsideTrigger;
    }
    if (aiRadius < 0.0) {
        int32_t iRadius = GetSecondLocationRadius();
        if (!iRadius) {
            if (pSecondLocation) {
                TESObjectREFR* pSecondLocationRef = GetSecondLocationReference(apActor);
                if (pSecondLocationRef) {
                    iRadius = static_cast<int32_t>(pSecondLocationRef->GetModelBoundSize());
                }
            }
        }
        if (iRadius <= 0) {
            iRadius = iAIDistanceRadiusMinLocation.UInt();
        }
        aiRadius = static_cast<float>(iRadius);
    }
    bool bGoodPt = false;
    const NiPoint3& kRefPos = pRef->GetLocationOnReference();
    NiPoint3 kOffset = GetSecondLocationCoord(apActor) - kRefPos;
    if (aiRadius >= static_cast<double>(static_cast<int32_t>(kOffset.XYLength()))) {
        return true;
    }
    return bGoodPt;
}

int32_t TESPackage::GetEscortFollowDistance() {
    if (GetPackType() == PACKAGE_TYPE_ESCORT) {
        return static_cast<TESEscortPackageData*>(pPackData)->iFollowPackRadius;
    }

    return 256;
}

void TESPackage::SetDialogueNoHeadTrack(bool abHead) {
    if (GetPackType() == PACKAGE_TYPE_DIALOGUE || GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
        TESDialoguePackageData* pPackageData;
        if (pPackData) {
            pPackageData = static_cast<TESDialoguePackageData*>(pPackData);
        }
        else {
            pPackageData = new TESDialoguePackageData;
            pPackData = pPackageData;
        }
        pPackageData->bNOHeadtracking = abHead;
    }
}

void TESPackage::SetDialogueFOV(float afFov) {
    if (GetPackType() == PACKAGE_TYPE_DIALOGUE || GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE) {
        TESDialoguePackageData* pPackageData;
        if (pPackData) {
            pPackageData = static_cast<TESDialoguePackageData*>(pPackData);
        }
        else {
            pPackageData = new TESDialoguePackageData;
            pPackData = pPackageData;
        }
        pPackageData->fFov = afFov;
    }
}

void TESPackage::SetPackageIdles(BGSIdleCollection* apIdles) {
    if (apIdles) {
        if (!pIdleCollection) {
            pIdleCollection = new BGSIdleCollection;
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
    PackageLocation* pSecondLocation = GetPackageSecondLocation();
    MobileObject* pLocRef = nullptr;
    if (!pSecondLocation || pSecondLocation->GetLocType() == PLT_CURRLOC) {
        if (apActor) {
            return apActor;
        }
    }
    else {
        switch (pSecondLocation->GetLocType()) {
            case PLT_REFERENCE: {
                if (pSecondLocation->GetLocReference()) {
                    pLocRef = static_cast<MobileObject*>(pSecondLocation->GetLocReference());
                }
                break;
            }
            case PLT_EDITORLOC: {
                if (apActor) {
                    pLocRef = apActor;
                }
                break;
            }
            case PLT_OBJECT:
            case PLT_TYPE: {
                if (apActor && apActor->pCurrentProcess) {
                    if (apActor->pCurrentProcess->GetPackageThatIsRunning() == this) {
                        TESObjectREFR* pRef = apActor->pCurrentProcess->GetSecondGenericLocation();
                        if (pRef) {
                            pLocRef = static_cast<MobileObject*>(pRef);
                        }
                        else {
                            pLocRef = apActor;
                        }
                    }
                }

                break;
            }
            case PLT_LINKEDREF: {
                if (apActor && apActor->GetLinkedRef()) {
                    pLocRef = static_cast<MobileObject*>(apActor->GetLinkedRef());
                }

                break;
            }
            default: {
                break;
            }
        }
    }
    return pLocRef;
}

const NiPoint3 TESPackage::GetSearchLocationCoord(MobileObject* apActor) {
    PackageLocation* pLoc = GetPackageSearchLocation();
    if (pLoc == pPackLoc) {
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
// "any"; 0 when all match. Where the dates are tested for "both any", this schedule's date is tested for 0, not -1.
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
    switch (aPackageType) {
        case PACKAGE_TYPE_DIALOGUE:
        case PACKAGE_TYPE_IN_GAME_DIALOGUE: pPackage = new DialoguePackage; break;
        case PACKAGE_TYPE_COMBAT: pPackage = new CombatController(nullptr, nullptr, nullptr); break;
        case PACKAGE_TYPE_ALARM: pPackage = new AlarmPackage; break;
        case PACKAGE_TYPE_FLEE: pPackage = new FleePackage(nullptr, nullptr, nullptr); break;
        case PACKAGE_TYPE_TRESPASS: pPackage = new TrespassPackage; break;
        case PACKAGE_TYPE_SPECTATOR: pPackage = new SpectatorPackage; break;
        case PACKAGE_TYPE_NONE:
        case PACKAGE_TYPE_COUNT: break;
        case PACKAGE_TYPE_FIND: aPackageType = PACKAGE_TYPE_EXPLORE; break;
        case PACKAGE_TYPE_AMBUSH: aPackageType = PACKAGE_TYPE_AMBUSH; break;
        case PACKAGE_TYPE_BACKUP: pPackage = new BackUpPackage; break;
        default: pPackage = new TESPackage; break;
    }
    if (pPackage) {
        pPackage->SetPackType(aPackageType);
    }
    return pPackage;
}

// The package object type of a form; ingredients and potions that are food count as food, weapons as melee or ranged
// where they are one.
PACKAGE_OBJECT_TYPE TESPackage::GetObjectTypeFromForm(TESForm* apForm) {
    PACKAGE_OBJECT_TYPE eType = POTYPE_NONE;
    if (apForm) {
        switch (apForm->GetFormType()) {
            case SPEL_ID: eType = POTYPE_SPELL; break;
            case ACTI_ID: eType = POTYPE_ACTI; break;
            case ARMO_ID: eType = POTYPE_ARMO; break;
            case BOOK_ID: eType = POTYPE_BOOK; break;
            case CLOT_ID: eType = POTYPE_CLOT; break;
            case CONT_ID: eType = POTYPE_CONT; break;
            case DOOR_ID: eType = POTYPE_DOOR; break;
            case INGR_ID:
                if (static_cast<EffectItemList*>(static_cast<IngredientItem*>(apForm))->IsFood()) {
                    eType = POTYPE_FOOD;
                }
                else {
                    eType = POTYPE_INGR;
                }
                break;
            case LIGH_ID: eType = POTYPE_LIGH; break;
            case MISC_ID:
            case COBJ_ID:
            case IMOD_ID: eType = POTYPE_MISC; break;
            case FLOR_ID: eType = POTYPE_FLOR; break;
            case FURN_ID: eType = POTYPE_FURN; break;
            case WEAP_ID: {
                eType = POTYPE_WEAP;
                TESObjectWEAP* pWeapon = static_cast<TESObjectWEAP*>(apForm);
                if (pWeapon) {
                    if (pWeapon->TESObjectWEAP::IsMeleeWeapon()) {
                        eType = POTYPE_WEAP_MELEE;
                    }
                    else if (pWeapon->TESObjectWEAP::IsRangedWeapon()) {
                        eType = POTYPE_WEAP_RANGED;
                    }
                }
                break;
            }
            case AMMO_ID: eType = POTYPE_AMMO; break;
            case NPC__ID: eType = POTYPE_NPC; break;
            case CREA_ID: eType = POTYPE_CREA; break;
            case KEYM_ID: eType = POTYPE_KEYM; break;
            case ALCH_ID:
                if (static_cast<EffectItemList*>(static_cast<AlchemyItem*>(apForm))->IsFood()) {
                    eType = POTYPE_FOOD;
                }
                else {
                    eType = POTYPE_ALCH;
                }
                break;
            case CHIP_ID:
            case CCRD_ID:
            case CMNY_ID: eType = POTYPE_MISC; break;
            default: break;
        }
    }
    return eType;
}

// Whether a form is of a package object type. Tests its cast spell and weapon pointers for null again before using
// them.
bool TESPackage::FormMatchesPackageObjectType(TESForm* apForm, PACKAGE_OBJECT_TYPE aeType) {
    bool bRetVal = false;
    if (apForm && aeType != POTYPE_NONE) {
        switch (apForm->GetFormType()) {
            case SPEL_ID: {
                if (aeType == POTYPE_SPELL) {
                    bRetVal = true;
                }
                else if (aeType > POTYPE_SPELL && aeType <= POTYPE_SPELL_SELF) {
                    SpellItem* pSpell = static_cast<SpellItem*>(apForm);
                    if (pSpell) {
                        switch (aeType) {
                            case POTYPE_SPELL_RANGED:
                                if (pSpell->HasTargetEffect()) {
                                    bRetVal = true;
                                }
                                break;
                            case POTYPE_SPELL_TOUCH:
                                if (pSpell->HasTouchEffect()) {
                                    bRetVal = true;
                                }
                                break;
                            case POTYPE_SPELL_SELF:
                                if (pSpell->HasSelfEffect()) {
                                    bRetVal = true;
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
                    bRetVal = true;
                }
                break;
            }
            case ARMO_ID: {
                if (aeType == POTYPE_ARMO || (aeType > POTYPE_FOOD && aeType <= POTYPE_WEAR)) {
                    bRetVal = true;
                }
                break;
            }
            case BOOK_ID: {
                if (aeType == POTYPE_BOOK) {
                    bRetVal = true;
                }
                break;
            }
            case CLOT_ID: {
                if (aeType == POTYPE_CLOT || aeType == POTYPE_WEAR) {
                    bRetVal = true;
                }
                break;
            }
            case CONT_ID: {
                if (aeType == POTYPE_CONT) {
                    bRetVal = true;
                }
                break;
            }
            case DOOR_ID: {
                if (aeType == POTYPE_DOOR) {
                    bRetVal = true;
                }
                break;
            }
            case INGR_ID: {
                if (aeType == POTYPE_INGR) {
                    bRetVal = true;
                }
                else if (
                    aeType == POTYPE_FOOD &&
                    static_cast<EffectItemList*>(static_cast<IngredientItem*>(apForm))->IsFood()
                ) {
                    bRetVal = true;
                }

                break;
            }
            case LIGH_ID: {
                if (aeType == POTYPE_LIGH) {
                    bRetVal = true;
                }
                break;
            }
            case MISC_ID:
            case COBJ_ID:
            case IMOD_ID: {
                if (aeType == POTYPE_MISC) {
                    bRetVal = true;
                }
                break;
            }
            case FLOR_ID: {
                if (aeType == POTYPE_FLOR) {
                    bRetVal = true;
                }
                break;
            }
            case FURN_ID: {
                if (aeType == POTYPE_FURN) {
                    bRetVal = true;
                }
                break;
            }
            case WEAP_ID: {
                switch (aeType) {
                    case POTYPE_WEAP:
                    case POTYPE_COMBATWEAR:
                    case POTYPE_WEAR: bRetVal = true; break;
                    case POTYPE_WEAP_MELEE:
                    case POTYPE_WEAP_RANGED: {
                        TESObjectWEAP* pWeapon = static_cast<TESObjectWEAP*>(apForm);
                        if (pWeapon &&
                            ((pWeapon->TESObjectWEAP::IsMeleeWeapon() && aeType == POTYPE_WEAP_MELEE) ||
                             (pWeapon->TESObjectWEAP::IsRangedWeapon() && aeType == POTYPE_WEAP_RANGED))) {
                            bRetVal = true;
                        }

                        break;
                    }
                    default: break;
                }
                break;
            }
            case AMMO_ID: {
                if (aeType == POTYPE_AMMO || (aeType > POTYPE_FOOD && aeType <= POTYPE_WEAR)) {
                    bRetVal = true;
                }
                break;
            }
            case NPC__ID: {
                if (aeType == POTYPE_NPC || aeType == POTYPE_ACTOR) {
                    bRetVal = true;
                }
                break;
            }
            case CREA_ID: {
                if (aeType == POTYPE_CREA || aeType == POTYPE_ACTOR) {
                    bRetVal = true;
                }
                break;
            }
            case KEYM_ID: {
                if (aeType == POTYPE_KEYM) {
                    bRetVal = true;
                }
                break;
            }
            case ALCH_ID: {
                if (aeType == POTYPE_ALCH) {
                    bRetVal = true;
                }
                else if (
                    aeType == POTYPE_FOOD && static_cast<EffectItemList*>(static_cast<AlchemyItem*>(apForm))->IsFood()
                ) {
                    bRetVal = true;
                }

                break;
            }
            case CHIP_ID:
            case CCRD_ID:
            case CMNY_ID: {
                if (aeType == POTYPE_MISC) {
                    bRetVal = true;
                }
                break;
            }
            default: break;
        }
    }
    return bRetVal;
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
    // A null object matches nothing (a departure; see GetEscortTargetForPackage).
    if (!pObject || !pObject->IsBoundObject()) {
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
            TESPackage* pPack = static_cast<MobileObject*>(apRef)->MobileObject::GetCurrentPackage();
            if (pPack && pPack->GetPackType() == PACKAGE_TYPE_FOLLOW) {
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
