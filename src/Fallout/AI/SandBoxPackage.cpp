#include "Fallout/AI/SandBoxPackage.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/BSFurnitureMarker.h"
#include "BSMain/Setting.h"
#include "BSShader/BSShaderAccumulator.h"
#include "Fallout Shared/AiFormulas.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSIdleCollection.h"
#include "Fallout Shared/BGSIdleMarker.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout Shared/PackageSchedule.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFurniture.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/ActorPackageData.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/Procedure.h"
#include "Fallout/Misc/Calendar.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdlib.h"

SandBoxChoiceListItem::SandBoxChoiceListItem()
  : pTarget(nullptr), eProcedure(SANDBOX_PROCEDURE_NONE), iBaseScore(0), iFullScore(0) {}

SandBoxChoiceListItem::SandBoxChoiceListItem(TESForm* apTarget, int32_t aeProcessIndex, uint32_t auiBaseScore)
  : pTarget(static_cast<TESObjectREFR*>(apTarget)), eProcedure(static_cast<SANDBOX_PROCEDURE>(aeProcessIndex)),
    iBaseScore((int32_t)auiBaseScore), iFullScore(0) {
    if (pTarget) {
        pTarget->SetTargeted(1);
    }
}

void SandBoxChoiceListItem::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&eProcedure), 4, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iBaseScore), 4, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iFullScore), 4, 0);
    apSaveGameBuffer->SaveFormID(pTarget, 0);
}

void SandBoxChoiceListItem::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&eProcedure), 4);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iBaseScore), 4);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iFullScore), 4);
    apLoadGameBuffer->LoadFormID(pTarget);
}

TESObjectREFR* SandBoxChoiceListItem::GetTarget() const {
    return pTarget;
}

void SandBoxChoiceListItem::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESObjectREFR* v3;
    if (pTarget) {
        TESForm* FormByNumericID =
            TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTarget)));
        v3 = dynamic_cast<TESObjectREFR*>(FormByNumericID);
    }
    else {
        v3 = 0;
    }
    pTarget = v3;
    if (pTarget) {
        pTarget->SetTargeted(1);
    }
}

SandBoxRefEvalFilter::SandBoxRefEvalFilter(Actor* apActor, SandBoxActorPackageData* apPackData)
  : pActor(apActor), pPackData(apPackData) {}

SandBoxActorPackageData::SandBoxActorPackageData()
  : eState(SANDBOX_STATE_INIT), eProcedure(SANDBOX_PROCEDURE_NONE), fProcedureDuration(0.0f), pLocation(nullptr),
    fLastTimeChanged(0.0f), fTotalTime(0.0f), iNextTimeToChangeLocations(-1), iNextTimeToScanWorld(0),
    bShouldPickNewProcedure(true), bFood(false), bChair(false), bBed(false), pCurrentTargetObject(nullptr),
    cCurrentTargetMarker(127), pPrevTargetObject(nullptr), iActivityStartDay(0), fActivityDuration(0.0f),
    fActivityStartHour(0.0f), eActivity(SANDBOX_ACTIVITY_NONE) {
    for (int32_t i = 0; i < 6; ++i) {
        cProcedureWeight[i] = (uint8_t)(-128);
    }
}

SandBoxActorPackageData::~SandBoxActorPackageData() {
    ReleaseTargetObject();
    PotentialTargets.Clear(1);
}

void SandBoxActorPackageData::HandleDeletedReference(TESObjectREFR* pDeletedObj) {
    if (pDeletedObj) {
        bool breset = 0;
        if (pLocation == pDeletedObj) {
            pLocation = 0;
        }
        if (pPrevTargetObject == pDeletedObj) {
            pPrevTargetObject = 0;
        }
        if (pCurrentTargetObject == pDeletedObj) {
            breset = 1;
        }
        int32_t count = PotentialTargets.iSize;
        int32_t i = 0;
        while (i < count) {
            if ((&PotentialTargets.operator[](i))->pTarget == pDeletedObj) {
                breset = 1;
                PotentialTargets.RemoveFast(i, 1u);
                --count;
            }
            else {
                ++i;
            }
        }
        if (breset) {
            eProcedure = SANDBOX_PROCEDURE_NONE;
            fProcedureDuration = 0.0f;
            pCurrentTargetObject = 0;
            pPrevTargetObject = 0;
            cCurrentTargetMarker = 127;
            bShouldPickNewProcedure = 1;
            iNextTimeToScanWorld = 0;
        }
    }
}

PACKAGE_TYPE SandBoxActorPackageData::GetPackageType() {
    return PACKAGE_TYPE_SANDBOX;
}

void SandBoxActorPackageData::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    uint32_t isaveGameCount;
    uint32_t apData;
    apSaveGameBuffer->SaveDataEndian(&eState, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&eProcedure, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fProcedureDuration, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fLastTimeChanged, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fTotalTime, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&iNextTimeToChangeLocations, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fProcedureDuration, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&iNextTimeToScanWorld, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&bShouldPickNewProcedure, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&bFood, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&bChair, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&bBed, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&iIdleMarkers, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&cCurrentTargetMarker, 1u, 0);
    apData = appTimer.iLastTime - iPrevTargetChangedTimeStamp;
    apSaveGameBuffer->SaveDataEndian(&apData, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&iActivityStartDay, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fActivityDuration, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&fActivityStartHour, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&eActivity, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(cProcedureWeight, 6u, 0);
    apSaveGameBuffer->SaveFormID(pLocation, 0);
    apSaveGameBuffer->SaveFormID(pCurrentTargetObject, 0);
    apSaveGameBuffer->SaveFormID(pPrevTargetObject, 0);
    isaveGameCount = PotentialTargets.iSize;
    apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount);
    for (uint32_t i = 0; i < isaveGameCount; ++i) {
        SandBoxChoiceListItem* potentialTarget = &PotentialTargets.operator[](i);
        potentialTarget->SaveGame(apSaveGameBuffer);
    }
}

void SandBoxActorPackageData::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    int32_t iprevTargetDelta;
    apLoadGameBuffer->LoadData(&eState, 4u);
    apLoadGameBuffer->LoadData(&eProcedure, 4u);
    apLoadGameBuffer->LoadData(&fProcedureDuration, 4u);
    apLoadGameBuffer->LoadData(&fLastTimeChanged, 4u);
    apLoadGameBuffer->LoadData(&fTotalTime, 4u);
    apLoadGameBuffer->LoadData(&iNextTimeToChangeLocations, 4u);
    apLoadGameBuffer->LoadData(&fProcedureDuration, 4u);
    apLoadGameBuffer->LoadData(&iNextTimeToScanWorld, 4u);
    apLoadGameBuffer->LoadData(&bShouldPickNewProcedure, 1u);
    apLoadGameBuffer->LoadData(&bFood, 1u);
    apLoadGameBuffer->LoadData(&bChair, 1u);
    apLoadGameBuffer->LoadData(&bBed, 1u);
    apLoadGameBuffer->LoadData(&iIdleMarkers, 4u);
    apLoadGameBuffer->LoadData(&cCurrentTargetMarker, 1u);
    iprevTargetDelta = 0;
    apLoadGameBuffer->LoadData(&iprevTargetDelta, 4u);
    uint32_t Time = appTimer.iLastTime;
    iPrevTargetChangedTimeStamp = Time - iprevTargetDelta;
    apLoadGameBuffer->LoadData(&iActivityStartDay, 4u);
    apLoadGameBuffer->LoadData(&fActivityDuration, 4u);
    apLoadGameBuffer->LoadData(&fActivityStartHour, 4u);
    apLoadGameBuffer->LoadData(&eActivity, 4u);
    apLoadGameBuffer->LoadData(cProcedureWeight, 6u);
    apLoadGameBuffer->LoadFormID(pLocation);
    apLoadGameBuffer->LoadFormID(pCurrentTargetObject);
    apLoadGameBuffer->LoadFormID(pPrevTargetObject);
    uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    PotentialTargets.SetSize(isaveGameCount, 1);
    for (uint32_t i = 0; i < isaveGameCount; ++i) {
        SandBoxChoiceListItem* potentialTarget = &PotentialTargets.operator[](i);
        potentialTarget->LoadGame(apLoadGameBuffer);
    }
}

void SandBoxActorPackageData::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESObjectREFR* v7;
    TESObjectREFR* v8;
    TESObjectREFR* v9;
    if (pLocation) {
        TESForm* FormByNumericID =
            TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pLocation)));
        v9 = dynamic_cast<TESObjectREFR*>(FormByNumericID);
    }
    else {
        v9 = 0;
    }
    pLocation = v9;
    if (pCurrentTargetObject) {
        TESForm* v3 =
            TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pCurrentTargetObject)));
        v8 = dynamic_cast<TESObjectREFR*>(v3);
    }
    else {
        v8 = 0;
    }
    pCurrentTargetObject = v8;
    if (pCurrentTargetObject) {
        pCurrentTargetObject->SetTargeted(1);
    }
    if (pPrevTargetObject) {
        TESForm* v4 =
            TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pPrevTargetObject)));
        v7 = dynamic_cast<TESObjectREFR*>(v4);
    }
    else {
        v7 = 0;
    }
    pPrevTargetObject = v7;
    if (pPrevTargetObject) {
        pPrevTargetObject->SetTargeted(1);
    }
    uint32_t isaveGameCount = PotentialTargets.iSize;
    for (uint32_t i = 0; i < isaveGameCount; ++i) {
        SandBoxChoiceListItem* potentialTarget = &PotentialTargets.operator[](i);
        potentialTarget->InitLoadGame(apLoadGameBuffer);
    }
    int32_t count = PotentialTargets.iSize;
    int32_t i_2 = 0;
    bool breset = 0;
    while (i_2 < count) {
        SANDBOX_PROCEDURE FormHealth_0;
        SandBoxChoiceListItem* item = &PotentialTargets.operator[](i_2);
        if (item->pTarget ||
            (FormHealth_0 = item->eProcedure, !SandBoxActorPackageData::IsTargetNeededForProcedure(FormHealth_0))) {
            ++i_2;
        }
        else {
            PotentialTargets.RemoveFast(i_2, 1u);
            --count;
            breset = 1;
        }
    }
    if (pCurrentTargetObject) {
        if (pCurrentTargetObject->HasFreeMarker(0)) {
            pCurrentTargetObject->SetMarkerReserved(cCurrentTargetMarker, 1);
        }
        else {
            breset = 1;
        }
    }
    if (breset || !pCurrentTargetObject) {
        eProcedure = SANDBOX_PROCEDURE_NONE;
        fProcedureDuration = 0.0f;
        pCurrentTargetObject = 0;
        cCurrentTargetMarker = 127;
        bShouldPickNewProcedure = 1;
        iNextTimeToScanWorld = 0;
    }
    if (pLocation) {
        if (!iNextTimeToChangeLocations) {
            iNextTimeToChangeLocations = -1;
            if ((&pLocation->m_Extra)->GetLinkedRef()) {
                iNextTimeToChangeLocations = 0;
                PatrolRefData* prefData = (&pLocation->m_Extra)->GetPatrolRefData();
                if (prefData) {
                    uint32_t Time = appTimer.iLastTime;
                    iNextTimeToChangeLocations = static_cast<int64_t>(prefData->fTimeAtRef * 1000.0) + Time;
                }
            }
        }
    }
}

// Each part is truncated into a signed char. As the game does, afTimeB's minutes are taken from afTimeB less afTimeA's
// hour, and the minute difference is added to (or taken from) the float absolute hour difference. Both seconds are also
// truncated and never read.
float SandBoxActorPackageData::GetTimeDifference(float afTimeA, float afTimeB) {
    char cHourA = static_cast<char>(static_cast<int64_t>(afTimeA));
    char cMinuteA = static_cast<char>(static_cast<int64_t>((afTimeA - static_cast<double>(cHourA)) * 60.0));
    char cHourB = static_cast<char>(static_cast<int64_t>(afTimeB));
    char cMinuteB = static_cast<char>(static_cast<int64_t>((afTimeB - static_cast<double>(cHourA)) * 60.0));
    float fDifference = static_cast<float>(std::abs(cHourB - cHourA));
    if (cMinuteB > cMinuteA) {
        fDifference = static_cast<float>(fDifference + static_cast<double>(cMinuteB - cMinuteA));
    }
    else {
        fDifference = static_cast<float>(fDifference - static_cast<double>(cMinuteA - cMinuteB));
    }
    return fDifference;
}

bool SandBoxActorPackageData::IsActivityFinished(bool* abJustEndedTodayOut) {
    uint32_t Count = appCalendar.iMidnightsPassed;
    float Hour = appCalendar.GetHour();
    bool v5 = 0;
    if (eActivity == -1) {
        return 1;
    }
    uint32_t iActivityStartDay = this->iActivityStartDay;
    float v8 = fActivityDuration + fActivityStartHour;
    while (v8 >= 24.0) {
        v8 = v8 - 24.0;
        ++iActivityStartDay;
    }
    if (Count == iActivityStartDay) {
        if (v8 > static_cast<double>(Hour)) {
            return 0;
        }
        v5 = 1;
    }
    else {
        if (Count < iActivityStartDay) {
            return 0;
        }
    }
    if (abJustEndedTodayOut) {
        *abJustEndedTodayOut = v5;
    }
    return 1;
}

bool SandBoxActorPackageData::IsActivityActive() {
    uint32_t Count = appCalendar.iMidnightsPassed;
    float Hour = appCalendar.GetHour();
    if (eActivity == -1) {
        return 1;
    }
    uint32_t iActivityStartDay = this->iActivityStartDay;
    float v6 = fActivityDuration + fActivityStartHour;
    while (v6 >= 24.0) {
        v6 = v6 - 24.0;
        ++iActivityStartDay;
    }
    if (Count < this->iActivityStartDay || Count > iActivityStartDay) {
        return 0;
    }
    if (Count == this->iActivityStartDay && fActivityDuration > static_cast<double>(Hour)) {
        return 0;
    }
    return Count != iActivityStartDay || !(v6 <= static_cast<double>(Hour));
}

bool SandBoxActorPackageData::IsDoneWithIdleAnims(Actor* apActor, bool bfreeIfDone) {
    if (!eProcedure || eProcedure == 4) {
        ANIMATION_ACTION AnimAction = apActor->GetAnimAction();
        if (AnimAction >= ANIM_ACTION_WAIT_FOR_LOWER_BODY_ANIM && AnimAction <= ANIM_ACTION_WAIT_FOR_SPECIAL_IDLE) {
            return 0;
        }
        Animation* v7 = apActor->GetAnimation();
        if (v7) {
            if (!v7->SpecialIdleDonePlaying()) {
                return 0;
            }
            if (bfreeIfDone) {
                BSAnimGroupSequence* Sequence = v7->SpecialIdleGetSequence();
                if (Sequence) {
                    if (Sequence->GetState() != NiControllerSequence::kEaseOut) {
                        BaseProcess* CurrentAIProcess = apActor->pCurrentProcess;
                        CurrentAIProcess->FreeUpSpecialIdle(apActor);
                    }
                }
            }
        }
    }
    return 1;
}

void SandBoxActorPackageData::ReleaseTargetObject() {
    if ((!pDataHandler) || (!pDataHandler->GetClearingData())) {
        if (pCurrentTargetObject) {
            pCurrentTargetObject->SetMarkerReserved(cCurrentTargetMarker, 0);
        }
    }
    pCurrentTargetObject = nullptr;
    cCurrentTargetMarker = (uint8_t)127;
}

bool SandBoxActorPackageData::MoveToNextLocation() {
    if (!pLocation) {
        return (bool)0;
    }
    if (((uint32_t)iNextTimeToChangeLocations) == ((-1))) {
        return (bool)0;
    }
    if ((&appTimer)->BSTimer::GetTime() <= ((uint32_t)iNextTimeToChangeLocations)) {
        return (bool)0;
    }
    ExtraDataList* extraDataList = pLocation->TESObjectREFR::GetExtra();
    TESObjectREFR* extraLinkedRef = extraDataList->GetLinkedRef();
    if ((!extraLinkedRef) || (extraLinkedRef == pLocation)) {
        return (bool)0;
    }
    pLocation = extraLinkedRef;
    iNextTimeToChangeLocations = (-1);
    return (bool)1;
}

bool SandBoxActorPackageData::CanPerformActivity() {
    uint8_t result;
    switch ((int32_t)eActivity) {
        case -1: {
            result = 0;
            break;
        }
        case 0: {
            result = (uint8_t)bBed;
            break;
        }
        case 1:
        case 2:
        case 3: {
            result = (uint8_t)bFood;
            break;
        }
        default: {
            result = 1;
            break;
        }
    }
    return (bool)result;
}

void SandBoxActorPackageData::HandleSubprocessFinished() {
    ReleaseTargetObject();
    bShouldPickNewProcedure = (bool)1;
    eState = SANDBOX_STATE_GETTING_UP;
}

// The index is compared with the count signed.
void SandBoxActorPackageData::ForgetTargetAndPickProcedure(Actor* apActor, TESPackage* apOriginalPackage) {
    if (pCurrentTargetObject) {
        uint32_t uiCount = PotentialTargets.GetSize();
        for (int32_t i = 0; i < static_cast<int32_t>(uiCount); ++i) {
            SandBoxChoiceListItem* pChoice = &PotentialTargets[i];
            if (pChoice->GetTarget() == pCurrentTargetObject) {
                pChoice->iBaseScore = 0;
            }
        }
    }
    PickProcedure(apActor, apOriginalPackage);
}

void SandBoxActorPackageData::PickProcedure(Actor* apActor, TESPackage* apOriginalPackage) {
    TESWorldSpace* ppackageWorldSpace;
    int32_t isandboxProceduresProbabilities[NUM_SANDBOX_PROCEDURES];
    int32_t i;
    bShouldPickNewProcedure = 0;
    SetUpNextActivity();
    if (fTotalTime == 0.0) {
        if (apOriginalPackage) {
            if (apOriginalPackage->GetPackageSchedule()) {
                fTotalTime = static_cast<float>(apOriginalPackage->GetPackageSchedule()->PSData.iDuration);
            }
        }
        if (fTotalTime == 0.0) {
            fTotalTime = 24.0f;
        }
    }
    if (pCurrentTargetObject) {
        if (pPrevTargetObject != pCurrentTargetObject) {
            uint32_t Time = appTimer.iLastTime;
            iPrevTargetChangedTimeStamp = 1000 * iSandBoxPreventRepeatedActionTime.Int() + Time;
            pPrevTargetObject = pCurrentTargetObject;
        }
    }
    if (pPrevTargetObject) {
        if (appTimer.iLastTime > iPrevTargetChangedTimeStamp) {
            pPrevTargetObject = 0;
        }
    }
    if (pCurrentTargetObject) {
        pCurrentTargetObject->SetMarkerReserved(cCurrentTargetMarker, 0);
    }
    pCurrentTargetObject = 0;
    cCurrentTargetMarker = 127;
    eProcedure = SANDBOX_PROCEDURE_NONE;
    fProcedureDuration = 0.0f;
    if (appTimer.iLastTime >= iNextTimeToScanWorld) {
        ReleaseTargetObject();
        ScanWorld(apActor);
        uint32_t v4 = appTimer.iLastTime;
        iNextTimeToScanWorld =
            RandomInt(1000 * iMinSandboxRescanSeconds.Int(), 1000 * iMaxSandboxRescanSeconds.Int()) + v4;
    }
    bool bcanPerformActivity = IsActivityActive() && CanPerformActivity();
    if (bcanPerformActivity) {
        for (i = 0; i < NUM_SANDBOX_PROCEDURES; ++i) {
            isandboxProceduresProbabilities[i] = 0;
        }
        isandboxProceduresProbabilities[SandBoxActorPackageData::ProceduresForActivities[eActivity]] = 10;
    }
    else {
        for (i = 0; i < NUM_SANDBOX_PROCEDURES; ++i) {
            isandboxProceduresProbabilities[i] = cProcedureWeight[i];
        }
        isandboxProceduresProbabilities[SANDBOX_PROCEDURE_SLEEP] = 0;
        if (bFood) {
            isandboxProceduresProbabilities[SANDBOX_PROCEDURE_EAT] += 20;
        }
        if (!bChair) {
            isandboxProceduresProbabilities[SANDBOX_PROCEDURE_EAT] -= 15;
        }
        if (apActor->pCurrentProcess) {
            BaseProcess* process = apActor->pCurrentProcess;
            if (process->GetSocialTalkTimer() > 0.0) {
                isandboxProceduresProbabilities[SANDBOX_PROCEDURE_DIALOGUE] = 0;
            }
        }
    }
    int32_t itotalProb = 0;
    int32_t inumTargets = PotentialTargets.iSize;
    float fsearchDist = fSandBoxSearchRadius.Float();
    NiPoint3 packageLoc = apActor->GetLocationOnReference();
    TESObjectCELL* ppackageCell = apActor->GetParentCell();
    TESPackage* ppack = apActor->GetCurrentPackage();
    if (ppack) {
        packageLoc = ppack->GetLocationCoord(apActor);
        ppackageCell = ppack->GetLocationCell(apActor);
        int32_t ipackageLocRadius = ppack->GetLocationRadius(apActor);
        // Stores the chosen radius in a float temporary before fsearchDist.
        float fradius;
        if (ipackageLocRadius <= 0) {
            fradius = fsearchDist;
        }
        else {
            fradius = static_cast<float>(ipackageLocRadius);
        }
        fsearchDist = fradius;
    }
    if (pLocation) {
        packageLoc = pLocation->GetLocationOnReference();
        ppackageCell = pLocation->GetParentCell();
    }
    if (ppackageCell) {
        ppackageWorldSpace = ppackageCell->GetWorldSpace();
    }
    else {
        ppackageWorldSpace = apActor->GetWorldSpace();
    }
    float* pextraRange = &fSandBoxExtraDialogueRange.Float();
    float fsearchDialogueDistSqr = fsearchDist + *pextraRange;
    float fsearchDistSqr = fsearchDist * fsearchDist;
    fsearchDialogueDistSqr = fsearchDialogueDistSqr * fsearchDialogueDistSqr;
    // A choice that fails a test gets a full score of 0; one out of the package's cell or worldspace keeps its old
    // full score.
    for (i = 0; i < inumTargets; ++i) {
        SandBoxChoiceListItem* item = &PotentialTargets.operator[](static_cast<uint32_t>(i));
        int32_t score = isandboxProceduresProbabilities[item->eProcedure] * item->iBaseScore;
        score = BSMAX(score, 0);
        TESPackage* pcurrentPack = apActor->GetCurrentPackage();
        TESObjectREFR* ptargObject = item->pTarget;
        if (ptargObject) {
            if (ptargObject == pPrevTargetObject) {
                item->iFullScore = 0;
                continue;
            }
            bool binterior = 0;
            TESObjectCELL* ptargetCell = ptargObject->GetParentCell();
            if (ppackageCell && ptargetCell && (ppackageCell->GetInterior() || ptargetCell->GetInterior())) {
                binterior = 1;
                if (ppackageCell != ptargetCell) {
                    continue;
                }
            }
            if (!binterior) {
                TESWorldSpace* ptargetWorldSpace =
                    ptargetCell ? ptargetCell->GetWorldSpace() : ptargObject->GetWorldSpace();
                if (ppackageWorldSpace != ptargetWorldSpace) {
                    continue;
                }
            }
            bool bisDialogue = item->eProcedure == SANDBOX_PROCEDURE_DIALOGUE;
            float fmaxDistSqr = bisDialogue ? fsearchDialogueDistSqr : fsearchDistSqr;
            if (fmaxDistSqr < (ptargObject->GetLocationOnReference() - packageLoc).SqrLength()) {
                item->iFullScore = 0;
                continue;
            }
            if (!ptargObject->HasFreeMarker(0)) {
                item->iFullScore = 0;
                continue;
            }
            if (bisDialogue) {
                Actor* ptargetActor = static_cast<Actor*>(ptargObject);
                if (!apActor->pCurrentProcess ||
                    !apActor->pCurrentProcess->ComputeAllowSandboxConversation(apActor, ptargetActor)) {
                    item->iFullScore = 0;
                    continue;
                }
                FIGHT_REACTION ereaction = FIGHT_REACTION_NEUTRAL;
                if (apActor->GetShouldAttackActor(ptargetActor, 0, ereaction, 0) ||
                    ptargetActor->GetShouldAttackActor(apActor, 0, ereaction, 0)) {
                    item->iFullScore = 0;
                    continue;
                }
            }
        }
        item->iFullScore = score;
        itotalProb += score;
    }
    if (itotalProb <= 0) {
        eProcedure = SANDBOX_PROCEDURE_NONE;
        fProcedureDuration = 0.0f;
        pCurrentTargetObject = 0;
        cCurrentTargetMarker = 127;
    }
    else {
        uint8_t v13;
        int32_t iIndex;
        int32_t irandom = rt::rand() % itotalProb;
        for (i = 0; i < inumTargets; ++i) {
            SandBoxChoiceListItem* item_2 = &PotentialTargets.operator[](static_cast<uint32_t>(i));
            int32_t CastingType = item_2->iFullScore;
            irandom -= CastingType;
            if (irandom < 0) {
                break;
            }
        }
        SandBoxChoiceListItem ritem = PotentialTargets.operator[](static_cast<uint32_t>(i));
        eProcedure = ritem.eProcedure;
        pCurrentTargetObject = ritem.pTarget;
        if (pCurrentTargetObject) {
            iIndex = pCurrentTargetObject->GetFirstFreeMarkerIndex(0);
        }
        else {
            iIndex = -1;
        }
        if (iIndex >= 0) {
            v13 = iIndex;
        }
        else {
            v13 = 0;
        }
        cCurrentTargetMarker = v13;
        int32_t iactorEnergy = apActor->GetActorBaseEnergyLevel();
        fProcedureDuration = AiFormulas::ComputeRandomSandboxSubProcessDuration(iactorEnergy, eProcedure);
        for (i = 0; i < NUM_SANDBOX_PROCEDURES; ++i) {
            uint8_t cweight_2;
            uint8_t cweight = cProcedureWeight[i];
            if (cweight == 255) {
                cweight_2 = cweight;
            }
            else {
                cweight_2 = cweight + 1;
            }
            cweight = cweight_2;
            cProcedureWeight[i] = cweight_2;
        }
        cProcedureWeight[eProcedure] = 1;
        if (pCurrentTargetObject) {
            pCurrentTargetObject->SetMarkerReserved(cCurrentTargetMarker, 1);
        }
    }
    fLastTimeChanged = appCalendar.GetHour();
}

void SandBoxActorPackageData::InitLocation(Actor* apActor, PackageLocation* apPackloc) {
    ExtraDataList* extra;
    ExtraDataList* v4;
    uint32_t time;
    PatrolRefData* patrolRefData;
    BaseProcess* pProcess;
    switch ((int32_t)apPackloc->GetLocType()) {
        case 0: {
            pLocation = apPackloc->GetLocReference();
            break;
        }
        case 4:
        case 5: {
            pProcess = apActor->MobileObject::GetCurrentAIProcess();
            if (pProcess) {
                pLocation = pProcess->GetGenericLocation();
            }
            break;
        }
        case 6: {
            pLocation = apActor->GetLinkedRef();
            break;
        }
        default: {
            pLocation = nullptr;
            break;
        }
    }
    if (pLocation) {
        extra = pLocation->TESObjectREFR::GetExtra();
        if (extra->GetLinkedRef()) {
            iNextTimeToChangeLocations = 0;
            v4 = pLocation->TESObjectREFR::GetExtra();
            patrolRefData = v4->GetPatrolRefData();
            if (patrolRefData) {
                time = (&appTimer)->BSTimer::GetTime();
                iNextTimeToChangeLocations = (int32_t)(((int64_t)(patrolRefData->GetTimeAtRef() * 1000.0)) + time);
            }
        }
    }
}

void SandBoxActorPackageData::SetUpNextActivity() {
    bool bjustEndedToday;
    bjustEndedToday = 0;
    if (IsActivityFinished(&bjustEndedToday)) {
        uint32_t ithisDay = appCalendar.iMidnightsPassed;
        float fthisHour = appCalendar.GetHour();
        float fbestHoursFromNow = 3.4028235e+38f;
        float fbestStart = 3.4028235e+38f;
        float fbestDuration = 3.4028235e+38f;
        float fstart = 3.4028235e+38f;
        float fduration = 3.4028235e+38f;
        int32_t ibestActivity = -2;
        uint32_t ibestDay = 0;
        bool bresultFound = 0;
        for (int32_t iactivity = 0; iactivity < 4; ++iactivity) {
            AiFormulas::ComputeRandomSandboxActivityStartAndDuration(iactivity, fstart, fduration);
            if (!bjustEndedToday || (fthisHour >= fstart + fduration || iactivity != eActivity)) {
                double v2 = fthisHour < fstart + fduration ? fstart : fstart + 24.0;
                float v4 = v2;
                float fhoursFromNow = v4 - fthisHour;
                if (fhoursFromNow <= 0.0 || fbestHoursFromNow > static_cast<double>(fhoursFromNow)) {
                    fbestHoursFromNow = v4 - fthisHour;
                    ibestActivity = iactivity;
                    fbestStart = fstart;
                    uint32_t v1 = fthisHour < fstart + fduration ? ithisDay : ithisDay + 1;
                    ibestDay = v1;
                    fbestDuration = fduration;
                    bresultFound = 1;
                    if (fhoursFromNow <= 0.0) {
                        break;
                    }
                }
            }
        }
        if (bresultFound) {
            eActivity = static_cast<SANDBOX_ACTIVITY>(ibestActivity);
            fActivityDuration = fbestStart;
            iActivityStartDay = ibestDay;
            fActivityStartHour = fbestDuration;
        }
    }
}

bool SandBoxActorPackageData::ShouldPickNewProcedure(Actor* apActor) {
    bool bresult;
    bool bflag;
    TESPackage* ppack = apActor->GetCurrentPackage();
    if (!ppack) {
        goto LABEL_16;
    }
    bflag = 0;
    if (bBed && !ppack->SandboxGetAllowSleeping()) {
        bBed = 0;
        bflag = 1;
    }
    if (bFood && !ppack->SandboxGetAllowEating()) {
        bFood = 0;
        bflag = 1;
    }
    if (bChair && !ppack->SandboxGetAllowFurniture()) {
        bChair = 0;
        bflag = 1;
    }
    if (bflag) {
        iNextTimeToScanWorld = appTimer.iLastTime;
        bresult = true;
    }
    else {
    LABEL_16:;
        if (!pLocation ||
            (eProcedure == SANDBOX_PROCEDURE_USE_IDLE_MARKER ||
             (iNextTimeToChangeLocations == -1 || appTimer.iLastTime <= iNextTimeToChangeLocations))) {
            if (IsDoneWithIdleAnims(apActor, 1)) {
                if (bShouldPickNewProcedure) {
                    bresult = true;
                }
                else {
                    if (eProcedure == SANDBOX_PROCEDURE_NONE || fProcedureDuration <= 0.0) {
                        bresult = true;
                    }
                    else {
                        if (fTotalTime == 0.0) {
                            bresult = true;
                        }
                        else {
                            if (eState != SandBoxActorPackageData::SANDBOX_STATE_DOINGSTUFF &&
                                (pCurrentTargetObject &&
                                 pCurrentTargetObject->GetMarkerUsed(cCurrentTargetMarker, 1))) {
                                bresult = true;
                            }
                            else {
                                if (IsActivityFinished(0)) {
                                    bresult = true;
                                }
                                else {
                                    if (IsActivityActive() && CanPerformActivity()) {
                                        return eProcedure !=
                                            SandBoxActorPackageData::ProceduresForActivities[eActivity];
                                    }
                                    else {
                                        float a2 = appCalendar.GetHour();
                                        float ftime = GetTimeDifference(fLastTimeChanged, a2);
                                        bresult = fProcedureDuration < static_cast<double>(ftime);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            else {
                bresult = false;
            }
        }
        else {
            bresult = true;
        }
    }
    return bresult;
}

PROCEDURE SandBoxActorPackageData::GetProcedure() {
    if (eProcedure == -1) {
        return PROCEDURE_NONE;
    }
    else {
        return SandBoxActorPackageData::kProcedures[eProcedure];
    }
}

void SandBoxActorPackageData::ScanWorld(Actor* apActor) {
    SandBoxChoiceListItem v6;
    SandBoxChoiceListItem apMemory;
    PotentialTargets.Clear(1);
    bBed = 0;
    bChair = 0;
    bFood = 0;
    float fsearchDist = fSandBoxSearchRadius.Float();
    TESObjectCELL* pcell = apActor->GetParentCell();
    NiPoint3 packageLoc = apActor->GetLocationOnReference();
    TESPackage* ppack = apActor->GetCurrentPackage();
    if (ppack) {
        packageLoc = ppack->GetLocationCoord(apActor);
        int32_t ipackageLocRadius = ppack->GetLocationRadius(apActor);
        // Stores the chosen radius in a float temporary before fsearchDist.
        float fradius;
        if (ipackageLocRadius <= 0) {
            fradius = fsearchDist;
        }
        else {
            fradius = static_cast<float>(ipackageLocRadius);
        }
        fsearchDist = fradius;
    }
    if (pLocation) {
        packageLoc = pLocation->GetLocationOnReference();
        pcell = pLocation->GetParentCell();
    }
    SandBoxRefEvalFilter pfilter(apActor, this);
    pDataHandler->EnumReferencesCloseToPoint(
        pcell, packageLoc, fsearchDist, packageLoc, fsearchDist, &SandBoxActorPackageData::EvaluateReferenceforUse,
        &pfilter
    );
    if (ppack->SandboxGetAllowEating()) {
        ItemChange* pfood = apActor->GetBestFoodItem();
        if (pfood) {
            if (pfood->pContainerObj) {
                SandBoxChoiceListItem* v2 = FNVD_RECONSTRUCT(SandBoxChoiceListItem)(&apMemory, nullptr, 2, 1u);
                PotentialTargets.Add(*v2);
                bFood = 1;
            }
        }
    }
    if (ppack->SandboxGetAllowWandering()) {
        SandBoxChoiceListItem* v3 = FNVD_RECONSTRUCT(SandBoxChoiceListItem)(&v6, nullptr, 3, 1u);
        PotentialTargets.Add(*v3);
    }
}

bool SandBoxActorPackageData::UpdateIsActorAtLocation(MobileObject* apMobileObject, float afRadius) {
    if (!apMobileObject) {
        return 0;
    }
    Actor* apActor = 0;
    if (apMobileObject->IsActor()) {
        apActor = static_cast<Actor*>(apMobileObject);
    }
    if (!pLocation) {
        TESPackage* ppack;
        return apActor && (ppack = apActor->GetCurrentPackage()) != 0 && ppack->IsActorAtLocation(apActor, 0, -1.0f, 0);
    }
    TESWorldSpace* plocworld = pLocation->GetWorldSpace();
    TESObjectCELL* plocintcell = pLocation->GetParentCell();
    if (plocintcell) {
        if (!plocintcell->GetInterior()) {
            plocintcell = 0;
        }
    }
    TESWorldSpace* pactorworld = apMobileObject->GetWorldSpace();
    TESObjectCELL* pactorintcell = apMobileObject->GetParentCell();
    if (pactorintcell) {
        if (!pactorintcell->GetInterior()) {
            pactorintcell = 0;
        }
    }
    bool bsamespace = 0;
    if (pactorintcell || plocintcell) {
        if (pactorintcell == plocintcell) {
            bsamespace = 1;
        }
    }
    else {
        bsamespace = pactorworld == plocworld;
    }
    if (bsamespace) {
        float fradius = afRadius;
        bool bexactpos = 0;
        const NiPoint3* loccoord = &pLocation->GetLocationOnReference();
        if (afRadius == 0.0) {
            TESBoundObject* v5;
            TESBoundObject* ObjectReference = pLocation->GetObjectReference();
            if (ObjectReference == pXMarkerHeading || (v5 = pLocation->GetObjectReference(), v5 == pXMarker)) {
                fradius = 20.0f;
            }
            else {
                if (pLocation->IsActor()) {
                    if (pLocation->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
                        fradius = 90.0f;
                        bexactpos = 1;
                    }
                    else {
                        fradius = pLocation->GetModelBoundSize() + 20.0;
                    }
                }
                else {
                    float ffudge = 20.0f;
                    switch (pLocation->GetObjectReference()->GetFormType()) {
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
                    fradius = pLocation->GetModelBoundSize() + ffudge;
                    if (fradius < 0.0) {
                        fradius = 0.0f;
                    }
                }
            }
        }
        if (fradius == 0.0) {
            fradius = static_cast<float>(iAIDistanceRadiusMinLocation.Int());
        }
        if (apMobileObject->IsAtPoint(*loccoord, fradius, !bexactpos, 1)) {
            if (iNextTimeToChangeLocations == -1) {
                if ((&pLocation->m_Extra)->GetLinkedRef()) {
                    iNextTimeToChangeLocations = 0;
                    PatrolRefData* prefData = (&pLocation->m_Extra)->GetPatrolRefData();
                    if (prefData) {
                        uint32_t Time = appTimer.iLastTime;
                        iNextTimeToChangeLocations = static_cast<int64_t>(prefData->fTimeAtRef * 1000.0) + Time;
                    }
                    // Tests both bounds: an unset procedure (-1) gets no new duration.
                    if (eProcedure > SANDBOX_PROCEDURE_NONE && eProcedure < NUM_SANDBOX_PROCEDURES) {
                        int32_t iactorEnergy = apActor->GetActorBaseEnergyLevel();
                        fProcedureDuration =
                            AiFormulas::ComputeRandomSandboxSubProcessDuration(iactorEnergy, eProcedure);
                    }
                }
            }
            return 1;
        }
        else {
            return 0;
        }
    }
    else {
        return 0;
    }
}

void SandBoxActorPackageData::operator delete(void* apMemory) {
    ourFree(apMemory);
}

bool SandBoxActorPackageData::EvaluateReferenceforUse(TESObjectREFR* apRef, void* aiFilterData) {
    SandBoxChoiceListItem v9;
    SandBoxChoiceListItem v10;
    SandBoxChoiceListItem v11;
    SandBoxChoiceListItem v12;
    SandBoxChoiceListItem v13;
    SandBoxChoiceListItem apMemory;
    SandBoxRefEvalFilter* pfilter = static_cast<SandBoxRefEvalFilter*>(aiFilterData);
    SandBoxActorPackageData* ppackData = pfilter->pPackData;
    Actor* pactor = pfilter->pActor;
    if (!ppackData) {
        return 0;
    }
    if (!pactor || !pactor->GetObjectReference()) {
        return 0;
    }
    if (!apRef ||
        (apRef->GetDelete() ||
         (apRef->GetTemporary() ||
          (apRef->GetDisabled() || (pactor->IsChild(0) && !pactor->CanChildUseObject(apRef)))))) {
        return 0;
    }
    TESPackage* ppack = pactor->GetCurrentPackage();
    if (!ppack) {
        return 0;
    }
    if (apRef->GetIgnoredBySandbox()) {
        return 0;
    }
    switch (apRef->GetObjectReference()->GetFormType()) {
        case INGR_ID:
        case ALCH_ID: {
            if (ppack->SandboxGetAllowEating() && (!apRef->GetOwner() || apRef->IsAnOwner(pactor, 1))) {
                SandBoxChoiceListItem* v8 = FNVD_RECONSTRUCT(SandBoxChoiceListItem)(&v9, apRef, 2, 1u);
                ppackData->PotentialTargets.Add(*v8);
                ppackData->bFood = 1;
            }
            break;
        }
        case FURN_ID: {
            NiAVObject* p3D = (NiAVObject*)apRef->Get3D();
            if (!p3D) {
                break;
            }
            BSFurnitureMarker* pmarkdata = BSFurnitureMarker::Find(p3D);
            if (!pmarkdata || !pmarkdata->QNumMarks()) {
                break;
            }
            TESFurniture* pfurn = static_cast<TESFurniture*>(apRef->GetObjectReference());
            if (ppack->SandboxGetAllowSleeping() &&
                (pfurn->CanSleepOn() && (!apRef->GetOwner() || apRef->IsAnOwner(pactor, 1)))) {
                SandBoxChoiceListItem* v5 = FNVD_RECONSTRUCT(SandBoxChoiceListItem)(&v12, apRef, 1, 1u);
                ppackData->PotentialTargets.Add(*v5);
                ppackData->bBed = 1;
            }
            else if (
                ppack->SandboxGetAllowFurniture() &&
                (pfurn->CanSitOn() && (!apRef->GetOwner() || apRef->IsAnOwner(pactor, 1)))
            ) {
                SandBoxChoiceListItem* v6 = FNVD_RECONSTRUCT(SandBoxChoiceListItem)(&v11, apRef, 0, 1u);
                ppackData->PotentialTargets.Add(*v6);
                ppackData->bChair = 1;
            }
            else if (
                ppack->SandboxGetAllowFurniture() && !pfurn->CanSitOn() && !pfurn->CanSleepOn() &&
                (!apRef->GetOwner() || apRef->IsAnOwner(pactor, 1))
            ) {
                SandBoxChoiceListItem* v7 = FNVD_RECONSTRUCT(SandBoxChoiceListItem)(&v10, apRef, 0, 1u);
                ppackData->PotentialTargets.Add(*v7);
                ppackData->bChair = 1;
            }
            break;
        }
        case NPC__ID:
        case CREA_ID: {
            if (ppack->SandboxGetAllowConversation() && apRef->IsActor() && apRef != pPlayer && apRef != pactor) {
                SandBoxChoiceListItem* v4 = FNVD_RECONSTRUCT(SandBoxChoiceListItem)(&v13, apRef, 5, 1u);
                ppackData->PotentialTargets.Add(*v4);
            }
            break;
        }
        case IDLM_ID: {
            if (ppack->SandboxGetAllowIdleMarkers() && (!apRef->GetOwner() || apRef->IsAnOwner(pactor, 1))) {
                BGSIdleMarker* pIdleMark = static_cast<BGSIdleMarker*>(apRef->GetObjectReference());
                int32_t icount = pIdleMark->GetIdleCount();
                BGSIdleCollection::PICK_TYPE etype = pIdleMark->GetPickType();
                bool badd = 0;
                if (pIdleMark->HasIdleForActor(pactor, apRef)) {
                    SandBoxChoiceListItem* v3 = FNVD_RECONSTRUCT(SandBoxChoiceListItem)(&apMemory, apRef, 4, 1u);
                    ppackData->PotentialTargets.Add(*v3);
                }
            }
            break;
        }
        default: {
            break;
        }
    }
    return 0;
}
