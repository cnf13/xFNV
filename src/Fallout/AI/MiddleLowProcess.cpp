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
        Actor* apActor__pact = apRef->IsActor() ? static_cast<Actor*>(apRef) : nullptr;
        float fHour = appCalendar.GetHour();
        if (apActor__pact) {
            if (CheckforNewPackage(apActor__pact, false)) {
                ClearCurrentDataforProcess(apActor__pact);
                apActor__pact->AlertAccompanyToPackageChange();
                SetLocationForPackage(apActor__pact, false);
            }
            TESPackage* ppack = GetCurrentPackage();
            if (GetCurrentPackage()) {
                TESPackage* v2 = GetCurrentPackage();
                if (v2->ePROCEDURE_TYPE != PACKAGE_PROCEDURE_NONE && !ppack->GetWaiting()) {
                    char Format[304];
                    UpdateTarget();
                    PackageLocation* ploc = GetCurrentPackage()->pPackLoc;
                    TESObjectREFR* pRef_2 = nullptr;
                    TESPackage* v3 = GetCurrentPackage();
                    uint32_t Count = v3->ePROCEDURE_TYPE;
                    switch (Procedure_Action[Count][GetProcedureIndexRunning()]) {
                        case PROCEDURE_TRAVEL: {
                            if (ploc) {
                                pRef_2 = ploc->GetLocReference();
                            }
                            if (!pRef_2) {
                                TESObjectREFR* pRef;
                                BaseProcess* pproc = apActor__pact->pCurrentProcess;
                                if (pproc) {
                                    pRef = pproc->GetGenericLocation();
                                }
                                else {
                                    pRef = nullptr;
                                }
                                pRef_2 = pRef;
                            }
                            TESPackage* v57 = GetCurrentPackage();
                            if (!v57->IsActorAtLocation(apActor__pact, false, -1.0f, false) &&
                                apActor__pact->IsPathingComplete()) {
                                TESGlobal* ptimescale = static_cast<TESGlobal*>(TESForm::GetFormByNumericID(58u));
                                float fhour = appCalendar.GetHour();
                                if (static_cast<double>(fhour) < fHourLastProcessed) {
                                    fhour = fhour + 24.0;
                                }
                                float ftimedelta = fhour - fHourLastProcessed;
                                ftimedelta = 3600.0 / ptimescale->fValue * ftimedelta;
                                TESPackage* v5 = GetCurrentPackage();
                                float fradius = v5->GetPackageRadiusActorToLocation(apActor__pact, false);
                                TESPackage* v6 = GetCurrentPackage();
                                if (v6->GetPackType() == PACKAGE_TYPE_WANDER) {
                                    fradius = 0.0f;
                                }
                                TESPackage* v7 = GetCurrentPackage();
                                TESWorldSpace* afRadius2 = v7->GetLocationWorld(apActor__pact);
                                TESPackage* v8 = GetCurrentPackage();
                                TESObjectCELL* LocationCell = v8->GetLocationCell(apActor__pact);
                                TESPackage* v9 = GetCurrentPackage();
                                apActor__pact->SetPathfindingGoal(
                                    v9->GetLocationCoord(apActor__pact), LocationCell, afRadius2, fradius, nullptr
                                );
                            }
                            if (apActor__pact->pCurrentProcess->eLevel == PROCESS_TYPE_MIDDLE_LOW) {
                                TESPackage* v56 = GetCurrentPackage();
                                if (v56->IsActorAtLocation(apActor__pact, false, -1.0f, false)) {
                                    TESBoundObject* ObjectReference;
                                    TESPackage* v14;
                                    if (!GetLockedLocation()) {
                                        TESPackage* v11 = GetCurrentPackage();
                                        if (v11->GetLockDoorsAtLocation()) {
                                            const NiPoint3* v41 = &apActor__pact->GetLocationOnReference();
                                            const NiPoint3* v37 = &apActor__pact->GetLocationOnReference();
                                            TESObjectCELL* ParentCell = apActor__pact->GetParentCell();
                                            pDataHandler->EnumReferencesCloseToPoint(
                                                ParentCell, *v37, 5000.0f, *v41, 5000.0f,
                                                &LowProcess::LockDoorsatLocation, apActor__pact
                                            );
                                        }
                                        SetLockedLocation(true);
                                    }
                                    TESPackage* v12 = GetCurrentPackage();
                                    if (v12->GetUnlockDoorsAtLocation()) {
                                        const NiPoint3* v42 = &apActor__pact->GetLocationOnReference();
                                        const NiPoint3* v38 = &apActor__pact->GetLocationOnReference();
                                        TESObjectCELL* v36 = apActor__pact->GetParentCell();
                                        pDataHandler->EnumReferencesCloseToPoint(
                                            v36, *v38, 5000.0f, *v42, 5000.0f, &LowProcess::UnLockDoorsatLocation,
                                            apActor__pact
                                        );
                                    }
                                    if ((pRef_2 &&
                                         (ObjectReference = pRef_2->GetObjectReference(),
                                          ObjectReference == pXMarkerHeading)) ||
                                        (GetCurrentPackage()->pPackLoc &&
                                         (v14 = GetCurrentPackage(), v14->pPackLoc->GetLocType() == PLT_EDITORLOC))) {
                                        float ftargetangle = 0.0f;
                                        if (pRef_2) {
                                            ftargetangle = (&pRef_2->GetAngleOnReference())->z;
                                        }
                                        else {
                                            ftargetangle = apActor__pact->GetStartingAngle().z;
                                        }
                                        apActor__pact->SetHeading(ftargetangle);
                                    }
                                    AddToProcedureIndexRunning(apActor__pact, 1);
                                }
                            }
                            break;
                        }
                        case PROCEDURE_WANDER: {
                            TESPackage* v16 = GetCurrentPackage();
                            if (v16->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_EXPLORE_ACQUIRE_GENERIC) {
                                AddToProcedureIndexRunning(apActor__pact, -1);
                            }
                            break;
                        }
                        case PROCEDURE_ACTIVATE: {
                            ProcessActivate(apActor__pact, true);
                            break;
                        }
                        case PROCEDURE_ACQUIRE: {
                            ProcessAcquire(apActor__pact);
                            break;
                        }
                        case PROCEDURE_SLEEP: {
                            if (Interface::GetConsoleSelection() == apActor__pact) {
                                TESPackage* v17 = GetCurrentPackage();
                                PACKAGE_TYPE PackType = v17->GetPackType();
                                const void* CellOrCoords_0 = packageTypeToString[PackType];
                                std::sprintf(
                                    Format, "%s is sleeping with %s", apActor__pact->GetFullName(), CellOrCoords_0
                                );
                                Interface::PrintLine(static_cast<const char*>(Format));
                            }
                            break;
                        }
                        case PROCEDURE_EAT: {
                            if (Interface::GetConsoleSelection() == apActor__pact) {
                                TESPackage* v19 = GetCurrentPackage();
                                PACKAGE_TYPE v20 = v19->GetPackType();
                                const void* v49 = packageTypeToString[v20];
                                std::sprintf(Format, "%s is eating with %s", apActor__pact->GetFullName(), v49);
                                Interface::PrintLine(static_cast<const char*>(Format));
                            }
                            break;
                        }
                        case PROCEDURE_FOLLOW: {
                            ProcessFollow(apActor__pact, true, 257, false);
                            break;
                        }
                        case PROCEDURE_ESCORT: {
                            ProcessEscort(apActor__pact);
                            break;
                        }
                        case PROCEDURE_ALARM: {
                            ProcessAlarm(apActor__pact);
                            break;
                        }
                        case PROCEDURE_FLEE: {
                            ProcessFlee(apActor__pact);
                            break;
                        }
                        case PROCEDURE_DIALOGUE: {
                            apActor__pact->EndInterruptPackage(false);
                            break;
                        }
                        case PROCEDURE_WAIT: {
                            if (!pTarget) {
                                SetTargetForPackage(apActor__pact);
                            }
                            if (!pTarget || pTarget->GetDelete()) {
                                goto LABEL_88;
                            }
                            if (ppack->GetWaiting()) {
                                if (!pPlayer->IsPlayerFollower(apActor__pact)) {
                                    int32_t NumberOfFollowers = pPlayer->GetNumberOfFollowers();
                                    if (NumberOfFollowers < iNumberActorsAllowedToFollowPlayer.Int()) {
                                        ppack->SetWaiting(false);
                                    }
                                }
                            }
                            else {
                                if (ppack->GetPackType() == PACKAGE_TYPE_IN_GAME_DIALOGUE || ppack->GetWaiting()) {
                                    goto LABEL_88;
                                }
                                if (ppack->GetPackType() == PACKAGE_TYPE_FOLLOW) {
                                    float fwaitdistance = static_cast<float>(ppack->GetPackageTarget()->iValue);
                                    if (static_cast<double>(fwaitdistance) >=
                                        apActor__pact->GetDistanceFromReference(pTarget, false, false)) {
                                        AddToProcedureIndexRunning(apActor__pact, 1);
                                    }
                                }
                                else {
                                    float fwaitdistance_2 = 0.0f;
                                    TESPackage* v22 = GetCurrentPackage();
                                    int32_t idistpack = v22->GetPackageTarget()->iValue;
                                    if (idistpack <= 0) {
                                        idistpack = 200;
                                    }
                                    if (fwaitdistance_2 == 0.0 &&
                                        (apActor__pact->GetParentCell() &&
                                         apActor__pact->GetParentCell()->GetInterior())) {
                                        fwaitdistance_2 = fAIEscortWaitDistanceInterior.Float();
                                    }
                                    else {
                                        if (fwaitdistance_2 == 0.0) {
                                            fwaitdistance_2 = fAIEscortWaitDistanceExterior.Float();
                                        }
                                    }
                                    if (fwaitdistance_2 / 2.0 >=
                                        apActor__pact->GetDistanceFromReference(pTarget, false, false)) {
                                        AddToProcedureIndexRunning(apActor__pact, 1);
                                    }
                                }
                            }
                            break;
                        }
                        case PROCEDURE_TRAVEL_TARGET: {
                            ProcessActivate(apActor__pact, false);
                            break;
                        }
                        case PROCEDURE_PURSUE: {
                            ProcessPursue(apActor__pact, false, true, 257);
                            break;
                        }
                        case PROCEDURE_CREATE_FOLLOW: {
                            if (!pTarget) {
                                SetTargetForPackage(apActor__pact);
                            }
                            if ((pTarget) && (pTarget != pPlayer)) {
                                CreateFollowforEscort(apActor__pact, ppack, true);
                            }

                        LABEL_88:;
                            AddToProcedureIndexRunning(apActor__pact, 1);
                            break;
                        }
                        case PROCEDURE_NOTIFY: {
                            ProcessNotify(apActor__pact);
                            break;
                        }
                        case PROCEDURE_ACCOMPANY: {
                            ProcessAccompany(apActor__pact);
                            break;
                        }
                        case PROCEDURE_USE_ITEM_AT: {
                            ProcessUseItemAt(apActor__pact);
                            break;
                        }
                        case PROCEDURE_AMBUSH_WAIT: {
                            ProcessAmbushWait(apActor__pact);
                            break;
                        }
                        case PROCEDURE_CHOOSE_CAST: {
                            ProcessChooseSpell(apActor__pact);
                            break;
                        }
                        case PROCEDURE_FLEE_NON_COMBAT: {
                            ProcessFleeNonCombat(apActor__pact);
                            break;
                        }
                        case PROCEDURE_SUMMON_CREATURE_DEFEND: {
                            ProcessSummonCreatureDefend(apActor__pact);
                            break;
                        }
                        case PROCEDURE_AVOID_RADIATION: {
                            ProcessAvoidArea(apActor__pact);
                            break;
                        }
                        case PROCEDURE_TAKE_BACK_ITEM: {
                            AddToProcedureIndexRunning(apActor__pact, 2);
                            break;
                        }
                        case PROCEDURE_SANDBOX: {
                            ProcessSandBox(apActor__pact);
                            break;
                        }
                        case PROCEDURE_PATROL: {
                            ProcessPatrol(apActor__pact);
                            break;
                        }
                        case PROCEDURE_GUARD: {
                            ProcessGuard(apActor__pact);
                            break;
                        }
                        case PROCEDURE_DIALOGUE_ACTIVATE: {
                            Actor* pact = nullptr;
                            if ((pTarget) && (pTarget->IsActor())) {
                                pact = static_cast<Actor*>(pTarget);
                            }

                            if (pact) {
                                TESPackage* v55;
                                TESPackage* v15 = GetCurrentPackage();
                                if (!v15->GetPackageSecondLocation() ||
                                    (v55 = GetCurrentPackage(),
                                     v55->IsActorAtSecondLocation(pact, apActor__pact, false, -1.0f, true))) {
                                    ProcessActivate(apActor__pact, true);
                                }
                            }
                            break;
                        }
                        case PROCEDURE_USE_WEAPON: {
                            ProcessUseWeapon(apActor__pact);
                            break;
                        }
                        case PROCEDURE_MOVEMENT_BLOCKED: {
                            ProcessMovementBlocked(apActor__pact);
                            break;
                        }
                        default: {
                            break;
                        }
                    }
                    if ((apActor__pact->GetCurrentProcessType() == PROCESS_TYPE_MIDDLE_LOW) && (GetCurrentPackage())) {
                        TESPackage* v23 = GetCurrentPackage();
                        uint32_t v24 = v23->ePROCEDURE_TYPE;
                        if (Procedure_Action[v24][GetProcedureIndexRunning()] == PROCEDURE_DONE) {
                            if (!IsPackageDoneOnce()) {
                                ExtraDataList* a5 = &apActor__pact->m_Extra;
                                TESPackage* v25 = GetCurrentPackage();
                                Script::SetActionFlag(v25, a5, SCRIPT_ACTION_ONPACKAGEEND);
                                TESPackage* v26 = GetCurrentPackage();
                                PackageDone(apActor__pact, v26);
                                SetPackageDoneOnce(true);
                            }
                            SetIsAggressor(false);
                            if (GetCurrentPackage()) {
                                TESPackage* v27 = GetCurrentPackage();
                                if (!v27->ePROCEDURE_TYPE) {
                                    TESPackage* v54 = GetCurrentPackage();
                                    if (!v54->IsActorAtLocation(apActor__pact, false, -1.0f, false)) {
                                        AddToProcedureIndexRunning(apActor__pact, -1);
                                        return;
                                    }
                                }
                            }
                            if (GetCurrentPackage()) {
                                TESPackage* v28 = GetCurrentPackage();
                                if (v28->GetOncePerDay()) {
                                    // Takes the day before the package.
                                    char cDay = appCalendar.GetDay();
                                    TESPackage* v29 = GetCurrentPackage();
                                    apActor__pact->SetRunOnceExtraPackage(v29, cDay);
                                }
                            }
                            if (GetCurrentPackage()) {
                                TESPackage* v30 = GetCurrentPackage();
                                if (v30->GetIsCreated()) {
                                    apActor__pact->EndInterruptPackage(false);
                                }
                            }
                            if (Interface::GetConsoleSelection() == apActor__pact) {
                                TESPackage* v31 = GetCurrentPackage();
                                PACKAGE_TYPE v32 = v31->GetPackType();
                                const void* v50 = packageTypeToString[v32];
                                std::sprintf(Format, "%s is done with %s", apActor__pact->GetFullName(), v50);
                                Interface::PrintLine(static_cast<const char*>(Format));
                            }
                            ObjectstoAcquire* apThis = pAcquireObject;
                            if (apThis) {
                                apThis->Delete(1u);
                            }
                            pAcquireObject = nullptr;
                            while (!ObjectList.IsEmpty()) {
                                ObjectstoAcquire* pobj = (&ObjectList)->m_item;
                                if (pobj) {
                                    pobj->Delete(1u);
                                }
                                ObjectList.Remove(pobj);
                            }
                            if (GetCurrentPackage()) {
                                TESPackage* v33 = GetCurrentPackage();
                                if (!v33->GetPackageSchedule()->PSData.iDuration) {
                                    TESPackage* ptemp = GetCurrentPackage();
                                    CheckforNewPackage(apActor__pact, true);
                                    ppack = GetCurrentPackage();
                                    TESPackage* v34 = GetCurrentPackage();
                                    if (ptemp != v34) {
                                        SetProcedureIndexRunning(0);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (!apActor__pact->IsWaitingOnPath()) {
                BaseProcess* CurrentAIProcess = apRef->pCurrentProcess;
                CurrentAIProcess->ComputeLastTimeProcessed();
            }
        }
        else {
            BaseProcess* v52 = apRef->pCurrentProcess;
            v52->ComputeLastTimeProcessed();
        }
    }
}

bool MiddleLowProcess::CheckforNewPackage(Actor* apActor, bool abFlag) {
    float hour = (&appCalendar)->GetHour();
    uint8_t v6 = 0;
    if (abFlag || ((!GetCurrentPackage()) || (iHourPackageEvaluated != ((uint32_t)((int32_t)hour))))) {
        v6 = (uint8_t)LowProcess::CheckforNewPackage(apActor, abFlag);
        iHourPackageEvaluated = (uint32_t)((int32_t)hour);
    }
    return (bool)v6;
}

void MiddleLowProcess::SetPackageEvaluateHour(int32_t iHour) {
    iHourPackageEvaluated = iHour;
}

int32_t MiddleLowProcess::GetPackageEvaluateHour() {
    return static_cast<int32_t>(iHourPackageEvaluated);
}

int32_t MiddleLowProcess::GetActorValue(TESActorBase* apActorBase, ActorValue::Index aiWhich, Actor* apActor) {
    if ((uint8_t)ActorValue::IsFlagSet(aiWhich, ActorValue::ENUMERATION)) {
        return LowProcess::GetActorValue(apActorBase, aiWhich, apActor);
    }
    int32_t actorValue = LowProcess::GetActorValue(apActorBase, aiWhich, apActor);
    return actorValue + ((int32_t)(&pTemporaryModifiers)->GetModifier((uint8_t)aiWhich));
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
    if (!((uint8_t)ActorValue::IsFlagSet(aiWhich, ActorValue::ENUMERATION))) {
        (&pTemporaryModifiers)->AdjustModifier((int32_t)((uint8_t)aiWhich), afValue, ModifierList::NO_CLAMP);
    }
}

void MiddleLowProcess::TempModActorValue(Actor* apActor, ActorValue::Index aiWhich, int32_t aiValue) {
    if (!((uint8_t)ActorValue::IsFlagSet(aiWhich, ActorValue::ENUMERATION))) {
        float fValue = static_cast<float>(aiValue);
        (&pTemporaryModifiers)->AdjustModifier((int32_t)((uint8_t)aiWhich), fValue, ModifierList::NO_CLAMP);
    }
}

void MiddleLowProcess::DamageModActorValue(Actor* apActor, ActorValue::Index aiWhich, float afValue) {
    double v4;
    ModifierList::ClampStyle a4;
    float afValue_2;
    if (!((uint8_t)ActorValue::IsFlagSet(aiWhich, ActorValue::ENUMERATION))) {
        if (apActor) {
            v4 = (double)Actor::CheckClampDamageModifier(static_cast<ActorValueOwner*>(apActor), aiWhich, afValue);
        }
        else {
            v4 = (double)Actor::CheckClampDamageModifier(nullptr, aiWhich, afValue);
        }
        afValue_2 = (float)v4;
        a4 = static_cast<ModifierList::ClampStyle>(
            ((uint8_t)ActorValue::IsFlagSet(aiWhich, ActorValue::DAMAGE_IS_POSITIVE)) != 0
        );
        (&pDamageModifiers)->AdjustModifier((int32_t)((uint8_t)aiWhich), afValue_2, a4);
    }
}

uint16_t MiddleLowProcess::GetSaveSize(uint32_t aiFlags, MobileObject* pmob) {
    uint16_t SaveSize = LowProcess::GetSaveSize(aiFlags, pmob);
    uint16_t v7 = SaveSize;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        SaveSize += 6;
    }
    uint16_t isize = SaveSize + 4;
    if ((aiFlags & CHANGE_ACTOR_TEMP_MODIFIERS) != 0) {
        isize += pTemporaryModifiers.GetSaveSize();
    }
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s", isize - v7,
                pheader->iFormID, pform->GetFormEditorID(), pheader->iFlags, 611,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleLowProcess.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s", isize - v7, 611,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleLowProcess.cpp"
            );
        }
    }
    return isize;
}

// The low process's data and the package hour, then the temporary modifiers when the save changed them (change
// flag 0x100000).
void MiddleLowProcess::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    LowProcess::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iHourPackageEvaluated), 4, 0);
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(0x100000)) {
        pTemporaryModifiers.SaveGame(apSaveGameBuffer);
    }
}

void MiddleLowProcess::SaveGame(uint32_t aiFlags, MobileObject* pmob) {
    uint32_t iblockHeader;
    LowProcess::SaveGame(aiFlags, pmob);
    uint16_t iblockSize = 0;
    uint16_t* pblockSize = nullptr;
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
    pSaveLoadGameOLD->SaveGameDataOLD(&iHourPackageEvaluated, 4u);
    if ((aiFlags & 0x100000) != 0) {
        pTemporaryModifiers.SaveGame_009378D0();
    }
    if (bDebugSaveBuffer.Bool()) {
        uint8_t* BufferPointer = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            uint32_t iFlags = pheader->iFlags;
            const char* v3 = pform->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                static_cast<int32_t>(BufferPointer - itempStartPosition), pheader->iFormID, v3, iFlags, 634,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleLowProcess.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s",
                static_cast<int32_t>(BufferPointer - itempStartPosition), 634,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleLowProcess.cpp"
            );
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        // The block's size: the bytes written since the size slot, as an unsigned compare of the end against the slot
        // plus 0xFFFF, then stored as a 16-bit value. The save buffer is raw bytes, so the slot is read as a byte
        // address.
        uint8_t* pEnd = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        uint8_t* pStart = reinterpret_cast<uint8_t*>(pblockSize);
        if (pEnd > pStart + 0xFFFF) {
            Warning(
                "SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleLowProcess.cpp", 634
            );
        }
        *pblockSize = static_cast<uint16_t>(pEnd - pStart);
    }
}

void MiddleLowProcess::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    LowProcess::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iHourPackageEvaluated), 4);
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(0x100000)) {
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
        pSaveLoadGameOLD->LoadGameDataOLD(&uiBlockHeader, 4);
        if (uiBlockHeader != 0x424C4F4B) {
            LoadFormHeader* pHeader = pSaveLoadGameOLD->GetCurrentlyLoadingFormHeader();
            if (pHeader) {
                TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
                const char* pEditorID = pForm->GetFormEditorID();
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  "
                    "Currently loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleLowProcess.cpp", 647, pHeader->iFormID,
                    pEditorID, pHeader->cVersion, pHeader->iFlags
                );
            }
            else {
                uint8_t ucVersion = pSaveLoadGameOLD->GetCurrentVersion();
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  "
                    "Current version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleLowProcess.cpp", 647, ucVersion
                );
            }
        }
        pBlockStart = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
        pSaveLoadGameOLD->LoadGameDataOLD(&usBlockSize, 2);
    }
    pSaveLoadGameOLD->LoadGameDataOLD(&iHourPackageEvaluated, 4);
    if (aiFlags & 0x100000) {
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
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleLowProcess.cpp", 656, pHeader->iFormID,
                    pEditorID, pHeader->cVersion, pHeader->iFlags
                );
            }
            else if (pNow < pBlockEnd) {
                const char* pEditorID = pForm->GetFormEditorID();
                Warning(
                    "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading "
                    "form is %08X %s with version %i and flags %08X",
                    static_cast<int32_t>(pBlockEnd - pNow),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleLowProcess.cpp", 656, pHeader->iFormID,
                    pEditorID, pHeader->cVersion, pHeader->iFlags
                );
            }
        }
        else if (pNow > pBlockEnd) {
            uint8_t ucVersion = pSaveLoadGameOLD->GetCurrentVersion();
            Warning(
                "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                static_cast<int32_t>(pNow - pBlockEnd),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleLowProcess.cpp", 656, ucVersion
            );
        }
        else if (pNow < pBlockEnd) {
            uint8_t ucVersion = pSaveLoadGameOLD->GetCurrentVersion();
            Warning(
                "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is %i",
                static_cast<int32_t>(pBlockEnd - pNow),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\ai\\MiddleLowProcess.cpp", 656, ucVersion
            );
        }
    }
}

// The low process's revert, then the temporary modifiers when the change flags from before the load have 0x100000.
void MiddleLowProcess::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
    LowProcess::Revert(apLoadGameBuffer);
    if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(0x100000)) {
        pTemporaryModifiers.RevertBGS(apLoadGameBuffer);
    }
}

// Reverts the temporary modifiers too when aiFlags has 0x100000.
void MiddleLowProcess::Revert(uint32_t aiFlags, MobileObject* pmob) {
    LowProcess::Revert(aiFlags, pmob);
    if (aiFlags & 0x100000) {
        pTemporaryModifiers.Revert();
    }
}

// ModifierList keys are bytes: only the index's low byte is passed on.
float MiddleLowProcess::GetTemporaryModifier(ActorValue::Index aiWhich) {
    return pTemporaryModifiers.GetModifier(static_cast<char>(aiWhich));
}

void MiddleLowProcess::ProcessCombat(Actor* apActor) {
    float* pCombatRound = &fCombatRoundAmount.Float();
    int32_t iCombatRound2 = static_cast<int32_t>(*pCombatRound + *pCombatRound);
    Actor* pTarget = dynamic_cast<Actor*>(this->pTarget);
    bool bRoundDone = false;
    if (!this->pTarget || (pTarget->GetDisabled() || (pTarget->GetDelete() || pTarget->IsDead(false)))) {
        apActor->StopCombat(nullptr);
        apActor->EndInterruptPackage(false);
    }
    else {
        BaseProcess* v3;
        if (pTarget &&
            (pTarget->GetCurrentAIProcess() &&
             (v3 = pTarget->GetCurrentAIProcess(), v3->GetProcessLevel() == PROCESS_TYPE_MIDDLE_LOW))) {
            while (iCombatRound2 && !apActor->IsDead(false)) {
                apActor->PerformOneRoundofCombat();
                pTarget = dynamic_cast<Actor*>(this->pTarget);
                if ((pTarget) && (!pTarget->IsDead(false))) {
                    BaseProcess* CurrentAIProcess = pTarget->GetCurrentAIProcess();
                    if (CurrentAIProcess->GetProcessLevel() == PROCESS_TYPE_MIDDLE_LOW) {
                        pTarget->PerformOneRoundofCombat();
                        bRoundDone = true;
                    }
                }

                --iCombatRound2;
            }
            if ((bRoundDone) && (pTarget)) {
                BaseProcess* pProcess = pTarget->GetCurrentAIProcess();
                pProcess->ComputeLastTimeProcessed();
            }
        }
        else {
            ProcessFollow(apActor, true, 257, false);
        }
    }
}

bool MiddleLowProcess::ProcessCombatOneHour(Actor* apActor, float afTimeDelta) {
    int32_t v6 = static_cast<int32_t>(fCombatRoundAmount.Float() * 4.0);
    Actor* pTarget = nullptr;
    while (v6 && !apActor->IsDead(false)) {
        apActor->PerformOneRoundofCombat();
        pTarget = dynamic_cast<Actor*>(this->pTarget);
        if ((pTarget) && (!pTarget->IsDead(false))) {
            pTarget->PerformOneRoundofCombat();
        }

        --v6;
    }
    if ((pTarget) && (!pTarget->IsDead(false))) {
        BaseProcess* CurrentAIProcess = pTarget->GetCurrentAIProcess();
        CurrentAIProcess->ComputeLastTimeProcessed();
    }

    return false;
}

void MiddleLowProcess::MiddleLowCopy(MiddleLowProcess* apProcess) {
    iHourPackageEvaluated = (uint32_t)((int32_t)apProcess->iHourPackageEvaluated);
    (&pTemporaryModifiers)->operator=(apProcess->pTemporaryModifiers);
}
