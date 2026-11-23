#include "Fallout/AI/MiddleLowProcess.h"

#include "BSCore/BSPerformanceTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/AiFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/PackageTarget.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESGlobal.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/LowProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/ModifierList.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/Procedure.h"
#include "Fallout/AI/ProcessArray.h"
#include "Fallout/Misc/Calendar.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "SDK/MSVC/rtti.h"

BSPerformanceTimer AIProcessing_MiddleLowUpdate("AIProcessing", "MiddleLowUpdate", true, true);

MiddleLowProcess::MiddleLowProcess() : LowProcess(), pTemporaryModifiers() {
    eLevel = PROCESS_TYPE_MIDDLE_LOW;
}

MiddleLowProcess::~MiddleLowProcess() {}

void MiddleLowProcess::Update(MobileObject* apRef, float fTimeDelta) {
    LowProcess::Update(apRef, fTimeDelta);
}

void MiddleLowProcess::Update(MobileObject* apRef) {
    AIProcessing_MiddleLowUpdate.Start();
    if (apRef) {
        Actor* pActor = apRef->IsActor() ? static_cast<Actor*>(apRef) : nullptr;
        if (pActor) {
            if (CheckforNewPackage(pActor, false)) {
                ClearCurrentDataforProcess(pActor);
                pActor->AlertAccompanyToPackageChange();
                SetLocationForPackage(pActor, false);
            }
            TESPackage* pPack = GetCurrentPackage();
            if (GetCurrentPackage()) {
                if (GetCurrentPackage()->ePROCEDURE_TYPE != PACKAGE_PROCEDURE_NONE && !pPack->GetWaiting()) {
                    char acBuffer[304];
                    UpdateTarget();
                    PackageLocation* pLoc = GetCurrentPackage()->pPackLoc;
                    TESObjectREFR* pRef = nullptr;
                    uint32_t uiProcedureType = GetCurrentPackage()->ePROCEDURE_TYPE;
                    switch (Procedure::Action[uiProcedureType][GetProcedureIndexRunning()]) {
                        case PROCEDURE_TRAVEL: {
                            if (pLoc) {
                                pRef = pLoc->GetLocReference();
                            }
                            if (!pRef) {
                                BaseProcess* pProc = pActor->pCurrentProcess;
                                pRef = pProc ? pProc->GetGenericLocation() : nullptr;
                            }
                            if (!GetCurrentPackage()->IsActorAtLocation(pActor, false, -1.0f, false) &&
                                pActor->IsPathingComplete()) {
                                TESGlobal* pTimeScale = static_cast<TESGlobal*>(TESForm::GetFormByNumericID(58u));
                                float fhour = appCalendar.GetHour();
                                if (static_cast<double>(fhour) < fHourLastProcessed) {
                                    fhour = fhour + 24.0;
                                }
                                float fTimeDelta = fhour - fHourLastProcessed;
                                fTimeDelta = 3600.0 / pTimeScale->fValue * fTimeDelta;
                                float fRadius = GetCurrentPackage()->GetPackageRadiusActorToLocation(pActor, false);
                                if (GetCurrentPackage()->GetPackType() == PACKAGE_TYPE_WANDER) {
                                    fRadius = 0.0f;
                                }
                                TESWorldSpace* pLocationWorld = GetCurrentPackage()->GetLocationWorld(pActor);
                                TESObjectCELL* pLocationCell = GetCurrentPackage()->GetLocationCell(pActor);
                                pActor->SetPathfindingGoal(
                                    GetCurrentPackage()->GetLocationCoord(pActor),
                                    pLocationCell,
                                    pLocationWorld,
                                    fRadius,
                                    nullptr
                                );
                            }
                            if (pActor->pCurrentProcess->eLevel == PROCESS_TYPE_MIDDLE_LOW) {
                                if (GetCurrentPackage()->IsActorAtLocation(pActor, false, -1.0f, false)) {
                                    if (!GetLockedLocation()) {
                                        if (GetCurrentPackage()->GetLockDoorsAtLocation()) {
                                            const NiPoint3& kPoint2 = pActor->GetLocationOnReference();
                                            const NiPoint3& kPoint1 = pActor->GetLocationOnReference();
                                            TESObjectCELL* pParentCell = pActor->GetParentCell();
                                            pDataHandler->EnumReferencesCloseToPoint(
                                                pParentCell,
                                                kPoint1,
                                                5000.0f,
                                                kPoint2,
                                                5000.0f,
                                                &LowProcess::LockDoorsatLocation,
                                                pActor
                                            );
                                        }
                                        SetLockedLocation(true);
                                    }
                                    if (GetCurrentPackage()->GetUnlockDoorsAtLocation()) {
                                        const NiPoint3& kPoint2 = pActor->GetLocationOnReference();
                                        const NiPoint3& kPoint1 = pActor->GetLocationOnReference();
                                        TESObjectCELL* pParentCell = pActor->GetParentCell();
                                        pDataHandler->EnumReferencesCloseToPoint(
                                            pParentCell,
                                            kPoint1,
                                            5000.0f,
                                            kPoint2,
                                            5000.0f,
                                            &LowProcess::UnLockDoorsatLocation,
                                            pActor
                                        );
                                    }
                                    if ((pRef && pRef->GetObjectReference() == pXMarkerHeading) ||
                                        (GetCurrentPackage()->pPackLoc &&
                                         GetCurrentPackage()->pPackLoc->GetLocType() == PLT_EDITORLOC)) {
                                        float fTargetAngle = 0.0f;
                                        if (pRef) {
                                            fTargetAngle = pRef->GetAngleOnReference().z;
                                        }
                                        else {
                                            fTargetAngle = pActor->GetStartingAngle().z;
                                        }
                                        pActor->SetHeading(fTargetAngle);
                                    }
                                    AddToProcedureIndexRunning(pActor, 1);
                                }
                            }
                            break;
                        }
                        case PROCEDURE_WANDER: {
                            if (GetCurrentPackage()->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_EXPLORE_ACQUIRE_GENERIC) {
                                AddToProcedureIndexRunning(pActor, -1);
                            }
                            break;
                        }
                        case PROCEDURE_ACTIVATE: {
                            ProcessActivate(pActor, true);
                            break;
                        }
                        case PROCEDURE_ACQUIRE: {
                            ProcessAcquire(pActor);
                            break;
                        }
                        case PROCEDURE_SLEEP: {
                            if (Interface::GetConsoleSelection() == pActor) {
                                PACKAGE_TYPE ePackType = GetCurrentPackage()->GetPackType();
                                const void* pPackTypeName = packageTypeToString[ePackType];
                                std::sprintf(acBuffer, "%s is sleeping with %s", pActor->GetFullName(), pPackTypeName);
                                Interface::PrintLine(static_cast<const char*>(acBuffer));
                            }
                            break;
                        }
                        case PROCEDURE_EAT: {
                            if (Interface::GetConsoleSelection() == pActor) {
                                PACKAGE_TYPE ePackType = GetCurrentPackage()->GetPackType();
                                const void* pPackTypeName = packageTypeToString[ePackType];
                                std::sprintf(acBuffer, "%s is eating with %s", pActor->GetFullName(), pPackTypeName);
                                Interface::PrintLine(static_cast<const char*>(acBuffer));
                            }
                            break;
                        }
                        case PROCEDURE_FOLLOW: {
                            ProcessFollow(pActor, true, 257, false);
                            break;
                        }
                        case PROCEDURE_ESCORT: {
                            ProcessEscort(pActor);
                            break;
                        }
                        case PROCEDURE_ALARM: {
                            ProcessAlarm(pActor);
                            break;
                        }
                        case PROCEDURE_FLEE: {
                            ProcessFlee(pActor);
                            break;
                        }
                        case PROCEDURE_DIALOGUE: {
                            pActor->EndInterruptPackage(false);
                            break;
                        }
                        case PROCEDURE_WAIT: {
                            if (!pTarget) {
                                SetTargetForPackage(pActor);
                            }
                            if (!pTarget || pTarget->GetDelete()) {
                                AddToProcedureIndexRunning(pActor, 1);
                            }
                            else if (pPack->GetWaiting()) {
                                if (!pPlayer->IsPlayerFollower(pActor)) {
                                    int32_t iNumberOfFollowers = pPlayer->GetNumberOfFollowers();
                                    if (iNumberOfFollowers < iNumberActorsAllowedToFollowPlayer.Int()) {
                                        pPack->SetWaiting(false);
                                    }
                                }
                            }
                            else if (pPack->GetPackType() != PACKAGE_TYPE_IN_GAME_DIALOGUE && !pPack->GetWaiting()) {
                                if (pPack->GetPackType() == PACKAGE_TYPE_FOLLOW) {
                                    float fWaitDistance = static_cast<float>(pPack->GetPackageTarget()->iValue);
                                    if (static_cast<double>(fWaitDistance) >=
                                        pActor->GetDistanceFromReference(pTarget, false, false)) {
                                        AddToProcedureIndexRunning(pActor, 1);
                                    }
                                }
                                else {
                                    float fWaitDistance = 0.0f;
                                    int32_t iDistPack = GetCurrentPackage()->GetPackageTarget()->iValue;
                                    if (iDistPack <= 0) {
                                        iDistPack = 200;
                                    }
                                    if (fWaitDistance == 0.0 &&
                                        pActor->GetParentCell() &&
                                        pActor->GetParentCell()->GetInterior()) {
                                        fWaitDistance = fAIEscortWaitDistanceInterior.Float();
                                    }
                                    else if (fWaitDistance == 0.0) {
                                        fWaitDistance = fAIEscortWaitDistanceExterior.Float();
                                    }
                                    if (fWaitDistance / 2.0 >=
                                        pActor->GetDistanceFromReference(pTarget, false, false)) {
                                        AddToProcedureIndexRunning(pActor, 1);
                                    }
                                }
                            }
                            else {
                                AddToProcedureIndexRunning(pActor, 1);
                            }
                            break;
                        }
                        case PROCEDURE_TRAVEL_TARGET: {
                            ProcessActivate(pActor, false);
                            break;
                        }
                        case PROCEDURE_PURSUE: {
                            ProcessPursue(pActor, false, true, 257);
                            break;
                        }
                        case PROCEDURE_CREATE_FOLLOW: {
                            if (!pTarget) {
                                SetTargetForPackage(pActor);
                            }
                            if (pTarget && pTarget != pPlayer) {
                                CreateFollowforEscort(pActor, pPack, true);
                            }
                            AddToProcedureIndexRunning(pActor, 1);
                            break;
                        }
                        case PROCEDURE_NOTIFY: {
                            ProcessNotify(pActor);
                            break;
                        }
                        case PROCEDURE_ACCOMPANY: {
                            ProcessAccompany(pActor);
                            break;
                        }
                        case PROCEDURE_USE_ITEM_AT: {
                            ProcessUseItemAt(pActor);
                            break;
                        }
                        case PROCEDURE_AMBUSH_WAIT: {
                            ProcessAmbushWait(pActor);
                            break;
                        }
                        case PROCEDURE_CHOOSE_CAST: {
                            ProcessChooseSpell(pActor);
                            break;
                        }
                        case PROCEDURE_FLEE_NON_COMBAT: {
                            ProcessFleeNonCombat(pActor);
                            break;
                        }
                        case PROCEDURE_SUMMON_CREATURE_DEFEND: {
                            ProcessSummonCreatureDefend(pActor);
                            break;
                        }
                        case PROCEDURE_AVOID_RADIATION: {
                            ProcessAvoidArea(pActor);
                            break;
                        }
                        case PROCEDURE_TAKE_BACK_ITEM: {
                            AddToProcedureIndexRunning(pActor, 2);
                            break;
                        }
                        case PROCEDURE_SANDBOX: {
                            ProcessSandBox(pActor);
                            break;
                        }
                        case PROCEDURE_PATROL: {
                            ProcessPatrol(pActor);
                            break;
                        }
                        case PROCEDURE_GUARD: {
                            ProcessGuard(pActor);
                            break;
                        }
                        case PROCEDURE_DIALOGUE_ACTIVATE: {
                            Actor* pAct = nullptr;
                            if (pTarget && pTarget->IsActor()) {
                                pAct = static_cast<Actor*>(pTarget);
                            }

                            if (pAct) {
                                if (!GetCurrentPackage()->GetPackageSecondLocation() ||
                                    GetCurrentPackage()->IsActorAtSecondLocation(pAct, pActor, false, -1.0f, true)) {
                                    ProcessActivate(pActor, true);
                                }
                            }
                            break;
                        }
                        case PROCEDURE_USE_WEAPON: {
                            ProcessUseWeapon(pActor);
                            break;
                        }
                        case PROCEDURE_MOVEMENT_BLOCKED: {
                            ProcessMovementBlocked(pActor);
                            break;
                        }
                        default: {
                            break;
                        }
                    }
                    if (pActor->GetCurrentProcessType() == PROCESS_TYPE_MIDDLE_LOW && GetCurrentPackage()) {
                        uiProcedureType = GetCurrentPackage()->ePROCEDURE_TYPE;
                        if (Procedure::Action[uiProcedureType][GetProcedureIndexRunning()] == PROCEDURE_DONE) {
                            if (!IsPackageDoneOnce()) {
                                Script::SetActionFlag(
                                    GetCurrentPackage(),
                                    &pActor->m_Extra,
                                    SCRIPT_ACTION_ONPACKAGEEND
                                );
                                TESPackage* pDonePack = GetCurrentPackage();
                                PackageDone(pActor, pDonePack);
                                SetPackageDoneOnce(true);
                            }
                            SetIsAggressor(false);
                            if (GetCurrentPackage()) {
                                if (!GetCurrentPackage()->ePROCEDURE_TYPE) {
                                    if (!GetCurrentPackage()->IsActorAtLocation(pActor, false, -1.0f, false)) {
                                        AddToProcedureIndexRunning(pActor, -1);
                                        return;
                                    }
                                }
                            }
                            if (GetCurrentPackage()) {
                                if (GetCurrentPackage()->GetOncePerDay()) {
                                    // Takes the day before the package.
                                    char cDay = appCalendar.GetDay();
                                    TESPackage* pOncePack = GetCurrentPackage();
                                    pActor->SetRunOnceExtraPackage(pOncePack, cDay);
                                }
                            }
                            if (GetCurrentPackage()) {
                                if (GetCurrentPackage()->GetIsCreated()) {
                                    pActor->EndInterruptPackage(false);
                                }
                            }
                            if (Interface::GetConsoleSelection() == pActor) {
                                PACKAGE_TYPE ePackType = GetCurrentPackage()->GetPackType();
                                const void* pPackTypeName = packageTypeToString[ePackType];
                                std::sprintf(acBuffer, "%s is done with %s", pActor->GetFullName(), pPackTypeName);
                                Interface::PrintLine(static_cast<const char*>(acBuffer));
                            }
                            ObjectstoAcquire* pAcquire = pAcquireObject;
                            delete pAcquire;
                            pAcquireObject = nullptr;
                            while (!ObjectList.IsEmpty()) {
                                ObjectstoAcquire* pObj = ObjectList.m_item;
                                delete pObj;
                                ObjectList.Remove(pObj);
                            }
                            if (GetCurrentPackage()) {
                                if (!GetCurrentPackage()->GetPackageSchedule()->PSData.iDuration) {
                                    TESPackage* pTemp = GetCurrentPackage();
                                    CheckforNewPackage(pActor, true);
                                    pPack = GetCurrentPackage();
                                    if (pTemp != GetCurrentPackage()) {
                                        SetProcedureIndexRunning(0);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (!pActor->IsWaitingOnPath()) {
                apRef->pCurrentProcess->ComputeLastTimeProcessed();
            }
        }
        else {
            apRef->pCurrentProcess->ComputeLastTimeProcessed();
        }
    }
}

bool MiddleLowProcess::CheckforNewPackage(Actor* apActor, bool abFlag) {
    float fHour = appCalendar.GetHour();
    uint8_t ucNewPackage = 0;
    if (abFlag || !GetCurrentPackage() || iHourPackageEvaluated != static_cast<uint32_t>(static_cast<int32_t>(fHour))) {
        ucNewPackage = LowProcess::CheckforNewPackage(apActor, abFlag);
        iHourPackageEvaluated = static_cast<uint32_t>(static_cast<int32_t>(fHour));
    }
    return ucNewPackage;
}

void MiddleLowProcess::SetPackageEvaluateHour(int32_t iHour) {
    iHourPackageEvaluated = iHour;
}

int32_t MiddleLowProcess::GetPackageEvaluateHour() {
    return static_cast<int32_t>(iHourPackageEvaluated);
}

int32_t MiddleLowProcess::GetActorValue(TESActorBase* apActorBase, ActorValue::Index aiWhich, Actor* apActor) {
    if (ActorValue::IsFlagSet(aiWhich, ActorValue::ENUMERATION)) {
        return LowProcess::GetActorValue(apActorBase, aiWhich, apActor);
    }
    int32_t iActorValue = LowProcess::GetActorValue(apActorBase, aiWhich, apActor);
    return iActorValue + static_cast<int32_t>(pTemporaryModifiers.GetModifier(static_cast<uint8_t>(aiWhich)));
}

float MiddleLowProcess::GetActorFloatValue(TESActorBase* apActorBase, ActorValue::Index aiWhich, Actor* apActor) {
    // enumerated actor values bypass the modifiers
    if (ActorValue::IsFlagSet(aiWhich, ActorValue::ENUMERATION)) {
        return this->LowProcess::GetActorFloatValue(apActorBase, aiWhich, apActor);
    }

    // the modifier lookup truncates the actor value index to a byte
    const double dBase = this->LowProcess::GetActorFloatValue(apActorBase, aiWhich, apActor);
    const double dModifier = pTemporaryModifiers.GetModifier(static_cast<uint8_t>(aiWhich));
    return static_cast<float>(dBase + dModifier);
}

void MiddleLowProcess::TempModActorValue(Actor* apActor, ActorValue::Index aiWhich, float afValue) {
    if (!ActorValue::IsFlagSet(aiWhich, ActorValue::ENUMERATION)) {
        pTemporaryModifiers.AdjustModifier(static_cast<uint8_t>(aiWhich), afValue, ModifierList::NO_CLAMP);
    }
}

void MiddleLowProcess::TempModActorValue(Actor* apActor, ActorValue::Index aiWhich, int32_t aiValue) {
    if (!ActorValue::IsFlagSet(aiWhich, ActorValue::ENUMERATION)) {
        float fValue = static_cast<float>(aiValue);
        pTemporaryModifiers.AdjustModifier(static_cast<uint8_t>(aiWhich), fValue, ModifierList::NO_CLAMP);
    }
}

void MiddleLowProcess::DamageModActorValue(Actor* apActor, ActorValue::Index aiWhich, float afValue) {
    if (!ActorValue::IsFlagSet(aiWhich, ActorValue::ENUMERATION)) {
        double dCheckedDamage;
        if (apActor) {
            dCheckedDamage = Actor::CheckClampDamageModifier(apActor, aiWhich, afValue);
        }
        else {
            dCheckedDamage = Actor::CheckClampDamageModifier(nullptr, aiWhich, afValue);
        }
        float fClampedDamage = static_cast<float>(dCheckedDamage);
        ModifierList::ClampStyle eClampStyle = static_cast<ModifierList::ClampStyle>(
            ActorValue::IsFlagSet(aiWhich, ActorValue::DAMAGE_IS_POSITIVE)
        );
        pDamageModifiers.AdjustModifier(static_cast<uint8_t>(aiWhich), fClampedDamage, eClampStyle);
    }
}

uint16_t MiddleLowProcess::GetSaveSize(uint32_t aiFlags, MobileObject* pmob) {
    uint16_t usSaveSize = LowProcess::GetSaveSize(aiFlags, pmob);
    uint16_t usBaseSize = usSaveSize;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        usSaveSize += 6;
    }
    uint16_t usSize = usSaveSize + 4;
    if ((aiFlags & CHANGE_ACTOR_TEMP_MODIFIERS) != 0) {
        usSize += pTemporaryModifiers.GetSaveSize();
    }
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                usSize - usBaseSize,
                pHeader->iFormID,
                pForm->GetFormEditorID(),
                pHeader->iFlags,
                611,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleLowProcess.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s",
                usSize - usBaseSize,
                611,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleLowProcess.cpp"
            );
        }
    }
    return usSize;
}

// The low process's data and the package hour, then the temporary modifiers when the save changed them.
void MiddleLowProcess::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    LowProcess::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iHourPackageEvaluated), sizeof(iHourPackageEvaluated), 0);
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_ACTOR_TEMP_MODIFIERS)) {
        pTemporaryModifiers.SaveGame(apSaveGameBuffer);
    }
}

void MiddleLowProcess::SaveGame(uint32_t aiFlags, MobileObject* pmob) {
    LowProcess::SaveGame(aiFlags, pmob);
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
    pSaveLoadGameOLD->SaveGameDataOLD(&iHourPackageEvaluated, sizeof(iHourPackageEvaluated));
    if ((aiFlags & CHANGE_ACTOR_TEMP_MODIFIERS) != 0) {
        pTemporaryModifiers.SaveGame();
    }
    if (bDebugSaveBuffer.Bool()) {
        uint8_t* pBufferPointer = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* pHeader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            uint32_t uiFlags = pHeader->iFlags;
            const char* pEditorID = pForm->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                static_cast<int32_t>(pBufferPointer - pTempStartPosition),
                pHeader->iFormID,
                pEditorID,
                uiFlags,
                634,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleLowProcess.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s",
                static_cast<int32_t>(pBufferPointer - pTempStartPosition),
                634,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleLowProcess.cpp"
            );
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        // The block's size: the bytes written since the size slot, as an unsigned compare of the end against the slot
        // plus 0xFFFF, then stored as a 16-bit value. The save buffer is raw bytes, so the slot is read as a byte
        // address.
        uint8_t* pEnd = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        uint8_t* pStart = reinterpret_cast<uint8_t*>(pBlockSize);
        if (pEnd > pStart + 0xFFFF) {
            Warning(
                "SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleLowProcess.cpp",
                634
            );
        }
        *pBlockSize = static_cast<uint16_t>(pEnd - pStart);
    }
}

void MiddleLowProcess::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    LowProcess::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iHourPackageEvaluated), sizeof(iHourPackageEvaluated));
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_ACTOR_TEMP_MODIFIERS)) {
        pTemporaryModifiers.LoadGame(apLoadGameBuffer);
    }
}

// Loads the low process's old-format data, the block header and the block size, the package hour, the temporary
// modifiers, then checks for a buffer overrun/underrun against the block size. The warnings come from a 'BLOK' header
// check that never fires, kept as the game has it.
void MiddleLowProcess::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags, MobileObject* pmob) {
    LowProcess::LoadGame(aiFlags, aiCurrentFlags, pmob);
    uint16_t usBlockSize = 0;
    char* pBlockStart = nullptr;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint32_t uiBlockHeader = 0;
        pSaveLoadGameOLD->LoadGameDataOLD(&uiBlockHeader, sizeof(uiBlockHeader));
        if (uiBlockHeader != 0x424C4F4B) {
            LoadFormHeader* pHeader = pSaveLoadGameOLD->GetCurrentlyLoadingFormHeader();
            if (pHeader) {
                TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
                const char* pEditorID = pForm->GetFormEditorID();
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  "
                    "Currently loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleLowProcess.cpp",
                    647,
                    pHeader->iFormID,
                    pEditorID,
                    pHeader->cVersion,
                    pHeader->iFlags
                );
            }
            else {
                uint8_t ucVersion = pSaveLoadGameOLD->GetCurrentVersion();
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  "
                    "Current version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleLowProcess.cpp",
                    647,
                    ucVersion
                );
            }
        }
        pBlockStart = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
        pSaveLoadGameOLD->LoadGameDataOLD(&usBlockSize, sizeof(usBlockSize));
    }
    pSaveLoadGameOLD->LoadGameDataOLD(&iHourPackageEvaluated, sizeof(iHourPackageEvaluated));
    if (aiFlags & CHANGE_ACTOR_TEMP_MODIFIERS) {
        pTemporaryModifiers.LoadGameTES();
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        char* pNow = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
        char* pBlockEnd = pBlockStart + usBlockSize;
        LoadFormHeader* pHeader = pSaveLoadGameOLD->GetCurrentlyLoadingFormHeader();
        if (pHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            if (pNow > pBlockEnd) {
                const char* pEditorID = pForm->GetFormEditorID();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading "
                    "form is %08X %s with version %i and flags %08X",
                    static_cast<int32_t>(pNow - pBlockEnd),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleLowProcess.cpp",
                    656,
                    pHeader->iFormID,
                    pEditorID,
                    pHeader->cVersion,
                    pHeader->iFlags
                );
            }
            else if (pNow < pBlockEnd) {
                const char* pEditorID = pForm->GetFormEditorID();
                Warning(
                    "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading "
                    "form is %08X %s with version %i and flags %08X",
                    static_cast<int32_t>(pBlockEnd - pNow),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleLowProcess.cpp",
                    656,
                    pHeader->iFormID,
                    pEditorID,
                    pHeader->cVersion,
                    pHeader->iFlags
                );
            }
        }
        else if (pNow > pBlockEnd) {
            uint8_t ucVersion = pSaveLoadGameOLD->GetCurrentVersion();
            Warning(
                "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                static_cast<int32_t>(pNow - pBlockEnd),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleLowProcess.cpp",
                656,
                ucVersion
            );
        }
        else if (pNow < pBlockEnd) {
            uint8_t ucVersion = pSaveLoadGameOLD->GetCurrentVersion();
            Warning(
                "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is %i",
                static_cast<int32_t>(pBlockEnd - pNow),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleLowProcess.cpp",
                656,
                ucVersion
            );
        }
    }
}

// The low process's revert, then the temporary modifiers when they had changed before the load.
void MiddleLowProcess::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
    LowProcess::Revert(apLoadGameBuffer);
    if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_ACTOR_TEMP_MODIFIERS)) {
        pTemporaryModifiers.RevertBGS(apLoadGameBuffer);
    }
}

void MiddleLowProcess::Revert(uint32_t aiFlags, MobileObject* pmob) {
    LowProcess::Revert(aiFlags, pmob);
    if (aiFlags & CHANGE_ACTOR_TEMP_MODIFIERS) {
        pTemporaryModifiers.Revert();
    }
}

// ModifierList keys are bytes: only the index's low byte is passed on.
float MiddleLowProcess::GetTemporaryModifier(ActorValue::Index aiWhich) {
    return pTemporaryModifiers.GetModifier(static_cast<char>(aiWhich));
}

void MiddleLowProcess::ProcessCombat(Actor* apActor) {
    int32_t iCombatRound2 = static_cast<int32_t>(fCombatRoundAmount.Float() + fCombatRoundAmount.Float());
    Actor* pTarget = dynamic_cast<Actor*>(this->pTarget);
    bool bRoundDone = false;
    if (!this->pTarget || pTarget->GetDisabled() || pTarget->GetDelete() || pTarget->IsDead(false)) {
        apActor->StopCombat(nullptr);
        apActor->EndInterruptPackage(false);
    }
    else if (
        pTarget &&
        pTarget->GetCurrentAIProcess() &&
        pTarget->GetCurrentAIProcess()->GetProcessLevel() == PROCESS_TYPE_MIDDLE_LOW
    ) {
        while (iCombatRound2 && !apActor->IsDead(false)) {
            apActor->PerformOneRoundofCombat();
            pTarget = dynamic_cast<Actor*>(this->pTarget);
            if (pTarget && !pTarget->IsDead(false)) {
                if (pTarget->GetCurrentAIProcess()->GetProcessLevel() == PROCESS_TYPE_MIDDLE_LOW) {
                    pTarget->PerformOneRoundofCombat();
                    bRoundDone = true;
                }
            }

            --iCombatRound2;
        }
        if (bRoundDone && pTarget) {
            BaseProcess* pProcess = pTarget->GetCurrentAIProcess();
            pProcess->ComputeLastTimeProcessed();
        }
    }
    else {
        ProcessFollow(apActor, true, 257, false);
    }
}

bool MiddleLowProcess::ProcessCombatOneHour(Actor* apActor, float afTimeDelta) {
    int32_t iCombatRound4 = static_cast<int32_t>(fCombatRoundAmount.Float() * 4.0);
    Actor* pTarget = nullptr;
    while (iCombatRound4 && !apActor->IsDead(false)) {
        apActor->PerformOneRoundofCombat();
        pTarget = dynamic_cast<Actor*>(this->pTarget);
        if (pTarget && !pTarget->IsDead(false)) {
            pTarget->PerformOneRoundofCombat();
        }

        --iCombatRound4;
    }
    if (pTarget && !pTarget->IsDead(false)) {
        pTarget->GetCurrentAIProcess()->ComputeLastTimeProcessed();
    }

    return false;
}

void MiddleLowProcess::MiddleLowCopy(MiddleLowProcess* apProcess) {
    iHourPackageEvaluated = apProcess->iHourPackageEvaluated;
    pTemporaryModifiers = apProcess->pTemporaryModifiers;
}
