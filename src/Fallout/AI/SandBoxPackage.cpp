#include "Fallout/AI/SandBoxPackage.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSTimer.h"
#include "BSCore/NewOverloads.h"
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

#include <limits>

SandBoxChoiceListItem::SandBoxChoiceListItem()
  : pTarget(nullptr), eProcedure(SANDBOX_PROCEDURE_NONE), iBaseScore(0), iFullScore(0) {}

SandBoxChoiceListItem::SandBoxChoiceListItem(TESForm* apTarget, int32_t aeProcessIndex, uint32_t auiBaseScore)
  : pTarget(static_cast<TESObjectREFR*>(apTarget)), eProcedure(static_cast<SANDBOX_PROCEDURE>(aeProcessIndex)),
    iBaseScore(static_cast<int32_t>(auiBaseScore)), iFullScore(0) {
    if (pTarget) {
        pTarget->SetTargeted(true);
    }
}

void SandBoxChoiceListItem::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&eProcedure), sizeof(eProcedure), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iBaseScore), sizeof(iBaseScore), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iFullScore), sizeof(iFullScore), 0);
    apSaveGameBuffer->SaveFormID(pTarget, 0);
}

void SandBoxChoiceListItem::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&eProcedure), sizeof(eProcedure));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iBaseScore), sizeof(iBaseScore));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iFullScore), sizeof(iFullScore));
    apLoadGameBuffer->LoadFormID(pTarget);
}

TESObjectREFR* SandBoxChoiceListItem::GetTarget() const {
    return pTarget;
}

void SandBoxChoiceListItem::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESObjectREFR* pRef = nullptr;
    if (pTarget) {
        TESForm* pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTarget)));
        pRef = dynamic_cast<TESObjectREFR*>(pForm);
    }
    pTarget = pRef;
    if (pTarget) {
        pTarget->SetTargeted(true);
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
    for (uint8_t& cWeight : cProcedureWeight) {
        cWeight = 128;
    }
}

SandBoxActorPackageData::~SandBoxActorPackageData() {
    ReleaseTargetObject();
    PotentialTargets.Clear(true);
}

void SandBoxActorPackageData::HandleDeletedReference(TESObjectREFR* pDeletedObj) {
    if (pDeletedObj) {
        bool breset = false;
        if (pLocation == pDeletedObj) {
            pLocation = nullptr;
        }
        if (pPrevTargetObject == pDeletedObj) {
            pPrevTargetObject = nullptr;
        }
        if (pCurrentTargetObject == pDeletedObj) {
            breset = true;
        }
        int32_t count = PotentialTargets.iSize;
        int32_t i = 0;
        while (i < count) {
            if (PotentialTargets[i].pTarget == pDeletedObj) {
                breset = true;
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
            pCurrentTargetObject = nullptr;
            pPrevTargetObject = nullptr;
            cCurrentTargetMarker = 127;
            bShouldPickNewProcedure = true;
            iNextTimeToScanWorld = 0;
        }
    }
}

PACKAGE_TYPE SandBoxActorPackageData::GetPackageType() {
    return PACKAGE_TYPE_SANDBOX;
}

void SandBoxActorPackageData::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    uint32_t isaveGameCount;
    uint32_t uiPrevTargetDelta;
    apSaveGameBuffer->SaveDataEndian(&eState, sizeof(eState), 0);
    apSaveGameBuffer->SaveDataEndian(&eProcedure, sizeof(eProcedure), 0);
    apSaveGameBuffer->SaveDataEndian(&fProcedureDuration, sizeof(fProcedureDuration), 0);
    apSaveGameBuffer->SaveDataEndian(&fLastTimeChanged, sizeof(fLastTimeChanged), 0);
    apSaveGameBuffer->SaveDataEndian(&fTotalTime, sizeof(fTotalTime), 0);
    apSaveGameBuffer->SaveDataEndian(&iNextTimeToChangeLocations, sizeof(iNextTimeToChangeLocations), 0);
    apSaveGameBuffer->SaveDataEndian(&fProcedureDuration, sizeof(fProcedureDuration), 0);
    apSaveGameBuffer->SaveDataEndian(&iNextTimeToScanWorld, sizeof(iNextTimeToScanWorld), 0);
    apSaveGameBuffer->SaveDataEndian(&bShouldPickNewProcedure, sizeof(bShouldPickNewProcedure), 0);
    apSaveGameBuffer->SaveDataEndian(&bFood, sizeof(bFood), 0);
    apSaveGameBuffer->SaveDataEndian(&bChair, sizeof(bChair), 0);
    apSaveGameBuffer->SaveDataEndian(&bBed, sizeof(bBed), 0);
    apSaveGameBuffer->SaveDataEndian(&iIdleMarkers, sizeof(iIdleMarkers), 0);
    apSaveGameBuffer->SaveDataEndian(&cCurrentTargetMarker, sizeof(cCurrentTargetMarker), 0);
    uiPrevTargetDelta = appTimer.iLastTime - iPrevTargetChangedTimeStamp;
    apSaveGameBuffer->SaveDataEndian(&uiPrevTargetDelta, sizeof(uiPrevTargetDelta), 0);
    apSaveGameBuffer->SaveDataEndian(&iActivityStartDay, sizeof(iActivityStartDay), 0);
    apSaveGameBuffer->SaveDataEndian(&fActivityDuration, sizeof(fActivityDuration), 0);
    apSaveGameBuffer->SaveDataEndian(&fActivityStartHour, sizeof(fActivityStartHour), 0);
    apSaveGameBuffer->SaveDataEndian(&eActivity, sizeof(eActivity), 0);
    apSaveGameBuffer->SaveDataEndian(cProcedureWeight, sizeof(cProcedureWeight), 0);
    apSaveGameBuffer->SaveFormID(pLocation, 0);
    apSaveGameBuffer->SaveFormID(pCurrentTargetObject, 0);
    apSaveGameBuffer->SaveFormID(pPrevTargetObject, 0);
    isaveGameCount = PotentialTargets.iSize;
    apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount);
    for (uint32_t i = 0; i < isaveGameCount; ++i) {
        SandBoxChoiceListItem* potentialTarget = &PotentialTargets[i];
        potentialTarget->SaveGame(apSaveGameBuffer);
    }
}

void SandBoxActorPackageData::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    int32_t iprevTargetDelta;
    apLoadGameBuffer->LoadData(&eState, sizeof(eState));
    apLoadGameBuffer->LoadData(&eProcedure, sizeof(eProcedure));
    apLoadGameBuffer->LoadData(&fProcedureDuration, sizeof(fProcedureDuration));
    apLoadGameBuffer->LoadData(&fLastTimeChanged, sizeof(fLastTimeChanged));
    apLoadGameBuffer->LoadData(&fTotalTime, sizeof(fTotalTime));
    apLoadGameBuffer->LoadData(&iNextTimeToChangeLocations, sizeof(iNextTimeToChangeLocations));
    apLoadGameBuffer->LoadData(&fProcedureDuration, sizeof(fProcedureDuration));
    apLoadGameBuffer->LoadData(&iNextTimeToScanWorld, sizeof(iNextTimeToScanWorld));
    apLoadGameBuffer->LoadData(&bShouldPickNewProcedure, sizeof(bShouldPickNewProcedure));
    apLoadGameBuffer->LoadData(&bFood, sizeof(bFood));
    apLoadGameBuffer->LoadData(&bChair, sizeof(bChair));
    apLoadGameBuffer->LoadData(&bBed, sizeof(bBed));
    apLoadGameBuffer->LoadData(&iIdleMarkers, sizeof(iIdleMarkers));
    apLoadGameBuffer->LoadData(&cCurrentTargetMarker, sizeof(cCurrentTargetMarker));
    iprevTargetDelta = 0;
    apLoadGameBuffer->LoadData(&iprevTargetDelta, sizeof(iprevTargetDelta));
    iPrevTargetChangedTimeStamp = appTimer.iLastTime - iprevTargetDelta;
    apLoadGameBuffer->LoadData(&iActivityStartDay, sizeof(iActivityStartDay));
    apLoadGameBuffer->LoadData(&fActivityDuration, sizeof(fActivityDuration));
    apLoadGameBuffer->LoadData(&fActivityStartHour, sizeof(fActivityStartHour));
    apLoadGameBuffer->LoadData(&eActivity, sizeof(eActivity));
    apLoadGameBuffer->LoadData(cProcedureWeight, sizeof(cProcedureWeight));
    apLoadGameBuffer->LoadFormID(pLocation);
    apLoadGameBuffer->LoadFormID(pCurrentTargetObject);
    apLoadGameBuffer->LoadFormID(pPrevTargetObject);
    uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    PotentialTargets.SetSize(isaveGameCount, true);
    for (uint32_t i = 0; i < isaveGameCount; ++i) {
        SandBoxChoiceListItem* potentialTarget = &PotentialTargets[i];
        potentialTarget->LoadGame(apLoadGameBuffer);
    }
}

void SandBoxActorPackageData::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESObjectREFR* pRef = nullptr;
    if (pLocation) {
        TESForm* pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pLocation)));
        pRef = dynamic_cast<TESObjectREFR*>(pForm);
    }
    pLocation = pRef;
    pRef = nullptr;
    if (pCurrentTargetObject) {
        TESForm* pForm = TESForm::GetFormByNumericID(
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pCurrentTargetObject))
        );
        pRef = dynamic_cast<TESObjectREFR*>(pForm);
    }
    pCurrentTargetObject = pRef;
    if (pCurrentTargetObject) {
        pCurrentTargetObject->SetTargeted(true);
    }
    pRef = nullptr;
    if (pPrevTargetObject) {
        TESForm* pForm = TESForm::GetFormByNumericID(
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pPrevTargetObject))
        );
        pRef = dynamic_cast<TESObjectREFR*>(pForm);
    }
    pPrevTargetObject = pRef;
    if (pPrevTargetObject) {
        pPrevTargetObject->SetTargeted(true);
    }
    uint32_t isaveGameCount = PotentialTargets.iSize;
    for (uint32_t i = 0; i < isaveGameCount; ++i) {
        SandBoxChoiceListItem* potentialTarget = &PotentialTargets[i];
        potentialTarget->InitLoadGame(apLoadGameBuffer);
    }
    int32_t count = PotentialTargets.iSize;
    int32_t i = 0;
    bool breset = false;
    while (i < count) {
        SANDBOX_PROCEDURE eItemProcedure;
        SandBoxChoiceListItem* item = &PotentialTargets[i];
        if (item->pTarget ||
            (eItemProcedure = item->eProcedure, !SandBoxActorPackageData::IsTargetNeededForProcedure(eItemProcedure))) {
            ++i;
        }
        else {
            PotentialTargets.RemoveFast(i, 1u);
            --count;
            breset = true;
        }
    }
    if (pCurrentTargetObject) {
        if (pCurrentTargetObject->HasFreeMarker(false)) {
            pCurrentTargetObject->SetMarkerReserved(cCurrentTargetMarker, true);
        }
        else {
            breset = true;
        }
    }
    if (breset || !pCurrentTargetObject) {
        eProcedure = SANDBOX_PROCEDURE_NONE;
        fProcedureDuration = 0.0f;
        pCurrentTargetObject = nullptr;
        cCurrentTargetMarker = 127;
        bShouldPickNewProcedure = true;
        iNextTimeToScanWorld = 0;
    }
    if (pLocation && !iNextTimeToChangeLocations) {
        iNextTimeToChangeLocations = -1;
        if (pLocation->m_Extra.GetLinkedRef()) {
            iNextTimeToChangeLocations = 0;
            PatrolRefData* prefData = pLocation->m_Extra.GetPatrolRefData();
            if (prefData) {
                iNextTimeToChangeLocations = static_cast<int64_t>(prefData->fTimeAtRef * 1000.0) + appTimer.iLastTime;
            }
        }
    }
}

// Each part is truncated into a signed char; afTimeB's minutes are taken from afTimeB less afTimeA's
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
    uint32_t uiDay = appCalendar.iMidnightsPassed;
    float fHour = appCalendar.GetHour();
    bool bJustEndedToday = false;
    if (eActivity == -1) {
        return true;
    }
    uint32_t iActivityStartDay = this->iActivityStartDay;
    float fEndHour = fActivityDuration + fActivityStartHour;
    while (fEndHour >= 24.0) {
        fEndHour = fEndHour - 24.0;
        ++iActivityStartDay;
    }
    if (uiDay == iActivityStartDay) {
        if (fEndHour > static_cast<double>(fHour)) {
            return false;
        }
        bJustEndedToday = true;
    }
    else if (uiDay < iActivityStartDay) {
        return false;
    }
    if (abJustEndedTodayOut) {
        *abJustEndedTodayOut = bJustEndedToday;
    }
    return true;
}

bool SandBoxActorPackageData::IsActivityActive() {
    uint32_t uiDay = appCalendar.iMidnightsPassed;
    float fHour = appCalendar.GetHour();
    if (eActivity == -1) {
        return true;
    }
    uint32_t iActivityStartDay = this->iActivityStartDay;
    float fEndHour = fActivityDuration + fActivityStartHour;
    while (fEndHour >= 24.0) {
        fEndHour = fEndHour - 24.0;
        ++iActivityStartDay;
    }
    if (uiDay < this->iActivityStartDay || uiDay > iActivityStartDay) {
        return false;
    }
    if (uiDay == this->iActivityStartDay && fActivityDuration > static_cast<double>(fHour)) {
        return false;
    }
    return uiDay != iActivityStartDay || !(fEndHour <= static_cast<double>(fHour));
}

bool SandBoxActorPackageData::IsDoneWithIdleAnims(Actor* apActor, bool bfreeIfDone) {
    if (!eProcedure || eProcedure == 4) {
        ANIMATION_ACTION eAnimAction = apActor->GetAnimAction();
        if (eAnimAction >= ANIM_ACTION_WAIT_FOR_LOWER_BODY_ANIM && eAnimAction <= ANIM_ACTION_WAIT_FOR_SPECIAL_IDLE) {
            return false;
        }
        Animation* pAnimation = apActor->GetAnimation();
        if (pAnimation) {
            if (!pAnimation->SpecialIdleDonePlaying()) {
                return false;
            }
            if (bfreeIfDone) {
                BSAnimGroupSequence* pSequence = pAnimation->SpecialIdleGetSequence();
                if (pSequence && pSequence->GetState() != NiControllerSequence::kEaseOut) {
                    apActor->pCurrentProcess->FreeUpSpecialIdle(apActor);
                }
            }
        }
    }
    return true;
}

void SandBoxActorPackageData::ReleaseTargetObject() {
    if ((!pDataHandler || !pDataHandler->GetClearingData()) && pCurrentTargetObject) {
        pCurrentTargetObject->SetMarkerReserved(cCurrentTargetMarker, false);
    }

    pCurrentTargetObject = nullptr;
    cCurrentTargetMarker = 127;
}

bool SandBoxActorPackageData::MoveToNextLocation() {
    if (!pLocation) {
        return false;
    }
    if (static_cast<uint32_t>(iNextTimeToChangeLocations) == -1) {
        return false;
    }
    if (appTimer.BSTimer::GetTime() <= static_cast<uint32_t>(iNextTimeToChangeLocations)) {
        return false;
    }
    ExtraDataList* extraDataList = pLocation->TESObjectREFR::GetExtra();
    TESObjectREFR* extraLinkedRef = extraDataList->GetLinkedRef();
    if (!extraLinkedRef || extraLinkedRef == pLocation) {
        return false;
    }
    pLocation = extraLinkedRef;
    iNextTimeToChangeLocations = -1;
    return true;
}

bool SandBoxActorPackageData::CanPerformActivity() {
    uint8_t ucCanPerform;
    switch (static_cast<int32_t>(eActivity)) {
        case -1: {
            ucCanPerform = 0;
            break;
        }
        case 0: {
            ucCanPerform = bBed;
            break;
        }
        case 1:
        case 2:
        case 3: {
            ucCanPerform = bFood;
            break;
        }
        default: {
            ucCanPerform = 1;
            break;
        }
    }
    return ucCanPerform;
}

void SandBoxActorPackageData::HandleSubprocessFinished() {
    ReleaseTargetObject();
    bShouldPickNewProcedure = true;
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
    bShouldPickNewProcedure = false;
    SetUpNextActivity();
    if (fTotalTime == 0.0) {
        if (apOriginalPackage && apOriginalPackage->GetPackageSchedule()) {
            fTotalTime = static_cast<float>(apOriginalPackage->GetPackageSchedule()->PSData.iDuration);
        }

        if (fTotalTime == 0.0) {
            fTotalTime = 24.0f;
        }
    }
    if (pCurrentTargetObject && pPrevTargetObject != pCurrentTargetObject) {
        uint32_t uiTime = appTimer.iLastTime;
        iPrevTargetChangedTimeStamp = 1000 * iSandBoxPreventRepeatedActionTime.Int() + uiTime;
        pPrevTargetObject = pCurrentTargetObject;
    }

    if (pPrevTargetObject && appTimer.iLastTime > iPrevTargetChangedTimeStamp) {
        pPrevTargetObject = nullptr;
    }

    if (pCurrentTargetObject) {
        pCurrentTargetObject->SetMarkerReserved(cCurrentTargetMarker, false);
    }
    pCurrentTargetObject = nullptr;
    cCurrentTargetMarker = 127;
    eProcedure = SANDBOX_PROCEDURE_NONE;
    fProcedureDuration = 0.0f;
    if (appTimer.iLastTime >= iNextTimeToScanWorld) {
        ReleaseTargetObject();
        ScanWorld(apActor);
        uint32_t uiTime = appTimer.iLastTime;
        iNextTimeToScanWorld = RandomInt(1000 * iMinSandboxRescanSeconds.Int(), 1000 * iMaxSandboxRescanSeconds.Int()) +
            uiTime;
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
            BaseProcess* pProcess = apActor->pCurrentProcess;
            if (pProcess->GetSocialTalkTimer() > 0.0) {
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
    ppackageWorldSpace = ppackageCell ? ppackageCell->GetWorldSpace() : apActor->GetWorldSpace();
    float fsearchDialogueDistSqr = fsearchDist + fSandBoxExtraDialogueRange.Float();
    float fsearchDistSqr = fsearchDist * fsearchDist;
    fsearchDialogueDistSqr = fsearchDialogueDistSqr * fsearchDialogueDistSqr;
    // A choice that fails a test gets a full score of 0; one out of the package's cell or worldspace keeps its old
    // full score.
    for (i = 0; i < inumTargets; ++i) {
        SandBoxChoiceListItem* item = &PotentialTargets[static_cast<uint32_t>(i)];
        int32_t score = isandboxProceduresProbabilities[item->eProcedure] * item->iBaseScore;
        score = BSMAX(score, 0);
        TESObjectREFR* ptargObject = item->pTarget;
        if (ptargObject) {
            if (ptargObject == pPrevTargetObject) {
                item->iFullScore = 0;
                continue;
            }
            bool binterior = false;
            TESObjectCELL* ptargetCell = ptargObject->GetParentCell();
            if (ppackageCell && ptargetCell && (ppackageCell->GetInterior() || ptargetCell->GetInterior())) {
                binterior = true;
                if (ppackageCell != ptargetCell) {
                    continue;
                }
            }
            if (!binterior) {
                TESWorldSpace* ptargetWorldSpace = ptargetCell ? ptargetCell->GetWorldSpace()
                                                               : ptargObject->GetWorldSpace();
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
            if (!ptargObject->HasFreeMarker(false)) {
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
                if (apActor->GetShouldAttackActor(ptargetActor, false, ereaction, false) ||
                    ptargetActor->GetShouldAttackActor(apActor, false, ereaction, false)) {
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
        pCurrentTargetObject = nullptr;
        cCurrentTargetMarker = 127;
    }
    else {
        int32_t irandom = rt::rand() % itotalProb;
        for (i = 0; i < inumTargets; ++i) {
            SandBoxChoiceListItem* item = &PotentialTargets[static_cast<uint32_t>(i)];
            irandom -= item->iFullScore;
            if (irandom < 0) {
                break;
            }
        }
        SandBoxChoiceListItem ritem = PotentialTargets[static_cast<uint32_t>(i)];
        eProcedure = ritem.eProcedure;
        pCurrentTargetObject = ritem.pTarget;
        int32_t iIndex = pCurrentTargetObject ? pCurrentTargetObject->GetFirstFreeMarkerIndex(false) : -1;
        uint8_t cMarker = iIndex >= 0 ? iIndex : 0;
        cCurrentTargetMarker = cMarker;
        int32_t iactorEnergy = apActor->GetActorBaseEnergyLevel();
        fProcedureDuration = AiFormulas::ComputeRandomSandboxSubProcessDuration(iactorEnergy, eProcedure);
        for (i = 0; i < NUM_SANDBOX_PROCEDURES; ++i) {
            uint8_t cweight = cProcedureWeight[i];
            cweight = cweight == 255 ? cweight : cweight + 1;
            cProcedureWeight[i] = cweight;
        }
        cProcedureWeight[eProcedure] = 1;
        if (pCurrentTargetObject) {
            pCurrentTargetObject->SetMarkerReserved(cCurrentTargetMarker, true);
        }
    }
    fLastTimeChanged = appCalendar.GetHour();
}

void SandBoxActorPackageData::InitLocation(Actor* apActor, PackageLocation* apPackloc) {
    ExtraDataList* extra;
    uint32_t time;
    PatrolRefData* patrolRefData;
    BaseProcess* pProcess;
    switch (static_cast<int32_t>(apPackloc->GetLocType())) {
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
            extra = pLocation->TESObjectREFR::GetExtra();
            patrolRefData = extra->GetPatrolRefData();
            if (patrolRefData) {
                time = appTimer.BSTimer::GetTime();
                iNextTimeToChangeLocations = static_cast<int32_t>(
                    static_cast<int64_t>(patrolRefData->GetTimeAtRef() * 1000.0) + time
                );
            }
        }
    }
}

void SandBoxActorPackageData::SetUpNextActivity() {
    bool bjustEndedToday = false;
    if (IsActivityFinished(&bjustEndedToday)) {
        uint32_t ithisDay = appCalendar.iMidnightsPassed;
        float fthisHour = appCalendar.GetHour();
        float fbestHoursFromNow = std::numeric_limits<float>::max();
        float fbestStart = std::numeric_limits<float>::max();
        float fbestDuration = std::numeric_limits<float>::max();
        float fstart = std::numeric_limits<float>::max();
        float fduration = std::numeric_limits<float>::max();
        int32_t ibestActivity = -2;
        uint32_t ibestDay = 0;
        bool bresultFound = false;
        for (int32_t iactivity = 0; iactivity < 4; ++iactivity) {
            AiFormulas::ComputeRandomSandboxActivityStartAndDuration(iactivity, fstart, fduration);
            if (!bjustEndedToday || fthisHour >= fstart + fduration || iactivity != eActivity) {
                float fnextStart = fthisHour < fstart + fduration ? fstart : fstart + 24.0;
                float fhoursFromNow = fnextStart - fthisHour;
                if (fhoursFromNow <= 0.0 || fbestHoursFromNow > static_cast<double>(fhoursFromNow)) {
                    fbestHoursFromNow = fnextStart - fthisHour;
                    ibestActivity = iactivity;
                    fbestStart = fstart;
                    ibestDay = fthisHour < fstart + fduration ? ithisDay : ithisDay + 1;
                    fbestDuration = fduration;
                    bresultFound = true;
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
    bool bflag = false;
    TESPackage* ppack = apActor->GetCurrentPackage();
    if (ppack) {
        if (bBed && !ppack->SandboxGetAllowSleeping()) {
            bBed = false;
            bflag = true;
        }
        if (bFood && !ppack->SandboxGetAllowEating()) {
            bFood = false;
            bflag = true;
        }
        if (bChair && !ppack->SandboxGetAllowFurniture()) {
            bChair = false;
            bflag = true;
        }
    }
    if (bflag) {
        iNextTimeToScanWorld = appTimer.iLastTime;
        bresult = true;
    }
    else {
        if (!pLocation ||
            eProcedure == SANDBOX_PROCEDURE_USE_IDLE_MARKER ||
            iNextTimeToChangeLocations == -1 ||
            appTimer.iLastTime <= iNextTimeToChangeLocations) {
            if (IsDoneWithIdleAnims(apActor, true)) {
                if (bShouldPickNewProcedure) {
                    bresult = true;
                }
                else if (eProcedure == SANDBOX_PROCEDURE_NONE || fProcedureDuration <= 0.0) {
                    bresult = true;
                }
                else if (fTotalTime == 0.0) {
                    bresult = true;
                }
                else if (
                    eState != SandBoxActorPackageData::SANDBOX_STATE_DOINGSTUFF &&
                    pCurrentTargetObject &&
                    pCurrentTargetObject->GetMarkerUsed(cCurrentTargetMarker, true)
                ) {
                    bresult = true;
                }
                else if (IsActivityFinished(nullptr)) {
                    bresult = true;
                }
                else {
                    if (IsActivityActive() && CanPerformActivity()) {
                        return eProcedure != SandBoxActorPackageData::ProceduresForActivities[eActivity];
                    }

                    float fHour = appCalendar.GetHour();
                    float ftime = GetTimeDifference(fLastTimeChanged, fHour);
                    bresult = fProcedureDuration < static_cast<double>(ftime);
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

    return SandBoxActorPackageData::kProcedures[eProcedure];
}

void SandBoxActorPackageData::ScanWorld(Actor* apActor) {
    PotentialTargets.Clear(true);
    bBed = false;
    bChair = false;
    bFood = false;
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
        pcell,
        packageLoc,
        fsearchDist,
        packageLoc,
        fsearchDist,
        &SandBoxActorPackageData::EvaluateReferenceforUse,
        &pfilter
    );
    if (ppack->SandboxGetAllowEating()) {
        ItemChange* pfood = apActor->GetBestFoodItem();
        if (pfood && pfood->pContainerObj) {
            PotentialTargets.Add(SandBoxChoiceListItem(nullptr, 2, 1u));
            bFood = true;
        }
    }
    if (ppack->SandboxGetAllowWandering()) {
        PotentialTargets.Add(SandBoxChoiceListItem(nullptr, 3, 1u));
    }
}

bool SandBoxActorPackageData::UpdateIsActorAtLocation(MobileObject* apMobileObject, float afRadius) {
    if (!apMobileObject) {
        return false;
    }
    Actor* pActor = nullptr;
    if (apMobileObject->IsActor()) {
        pActor = static_cast<Actor*>(apMobileObject);
    }
    if (!pLocation) {
        TESPackage* ppack;
        return pActor &&
            (ppack = pActor->GetCurrentPackage()) != nullptr &&
            ppack->IsActorAtLocation(pActor, false, -1.0f, false);
    }
    TESWorldSpace* plocworld = pLocation->GetWorldSpace();
    TESObjectCELL* plocintcell = pLocation->GetParentCell();
    if (plocintcell && !plocintcell->GetInterior()) {
        plocintcell = nullptr;
    }

    TESWorldSpace* pactorworld = apMobileObject->GetWorldSpace();
    TESObjectCELL* pactorintcell = apMobileObject->GetParentCell();
    if (pactorintcell && !pactorintcell->GetInterior()) {
        pactorintcell = nullptr;
    }

    bool bsamespace = false;
    if (pactorintcell || plocintcell) {
        if (pactorintcell == plocintcell) {
            bsamespace = true;
        }
    }
    else {
        bsamespace = pactorworld == plocworld;
    }
    if (bsamespace) {
        float fradius = afRadius;
        bool bexactpos = false;
        const NiPoint3* loccoord = &pLocation->GetLocationOnReference();
        if (afRadius == 0.0) {
            if (pLocation->GetObjectReference() == pXMarkerHeading || pLocation->GetObjectReference() == pXMarker) {
                fradius = 20.0f;
            }
            else if (pLocation->IsActor()) {
                if (pLocation->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
                    fradius = 90.0f;
                    bexactpos = true;
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
        if (fradius == 0.0) {
            fradius = static_cast<float>(iAIDistanceRadiusMinLocation.Int());
        }
        if (apMobileObject->IsAtPoint(*loccoord, fradius, !bexactpos, true)) {
            if (iNextTimeToChangeLocations == -1 && pLocation->m_Extra.GetLinkedRef()) {
                iNextTimeToChangeLocations = 0;
                PatrolRefData* prefData = pLocation->m_Extra.GetPatrolRefData();
                if (prefData) {
                    iNextTimeToChangeLocations = static_cast<int64_t>(prefData->fTimeAtRef * 1000.0) +
                        appTimer.iLastTime;
                }
                // Tests both bounds: an unset procedure (-1) gets no new duration.
                if (eProcedure > SANDBOX_PROCEDURE_NONE && eProcedure < NUM_SANDBOX_PROCEDURES) {
                    int32_t iactorEnergy = pActor->GetActorBaseEnergyLevel();
                    fProcedureDuration = AiFormulas::ComputeRandomSandboxSubProcessDuration(iactorEnergy, eProcedure);
                }
            }

            return true;
        }

        return false;
    }

    return false;
}

void SandBoxActorPackageData::operator delete(void* apMemory) {
    ourFree(apMemory);
}

bool SandBoxActorPackageData::EvaluateReferenceforUse(TESObjectREFR* apRef, void* aiFilterData) {
    SandBoxRefEvalFilter* pfilter = static_cast<SandBoxRefEvalFilter*>(aiFilterData);
    SandBoxActorPackageData* ppackData = pfilter->pPackData;
    Actor* pactor = pfilter->pActor;
    if (!ppackData) {
        return false;
    }
    if (!pactor || !pactor->GetObjectReference()) {
        return false;
    }
    if (!apRef ||
        apRef->GetDelete() ||
        apRef->GetTemporary() ||
        apRef->GetDisabled() ||
        (pactor->IsChild(false) && !pactor->CanChildUseObject(apRef))) {
        return false;
    }
    TESPackage* ppack = pactor->GetCurrentPackage();
    if (!ppack) {
        return false;
    }
    if (apRef->GetIgnoredBySandbox()) {
        return false;
    }
    switch (apRef->GetObjectReference()->GetFormType()) {
        case INGR_ID:
        case ALCH_ID: {
            if (ppack->SandboxGetAllowEating() && (!apRef->GetOwner() || apRef->IsAnOwner(pactor, true))) {
                ppackData->PotentialTargets.Add(SandBoxChoiceListItem(apRef, 2, 1u));
                ppackData->bFood = true;
            }
            break;
        }
        case FURN_ID: {
            NiAVObject* p3D = apRef->Get3D();
            if (!p3D) {
                break;
            }
            BSFurnitureMarker* pmarkdata = BSFurnitureMarker::Find(p3D);
            if (!pmarkdata || !pmarkdata->QNumMarks()) {
                break;
            }
            TESFurniture* pfurn = static_cast<TESFurniture*>(apRef->GetObjectReference());
            if (ppack->SandboxGetAllowSleeping() &&
                pfurn->CanSleepOn() &&
                (!apRef->GetOwner() || apRef->IsAnOwner(pactor, true))) {
                ppackData->PotentialTargets.Add(SandBoxChoiceListItem(apRef, 1, 1u));
                ppackData->bBed = true;
            }
            else if (
                ppack->SandboxGetAllowFurniture() &&
                pfurn->CanSitOn() &&
                (!apRef->GetOwner() || apRef->IsAnOwner(pactor, true))
            ) {
                ppackData->PotentialTargets.Add(SandBoxChoiceListItem(apRef, 0, 1u));
                ppackData->bChair = true;
            }
            else if (
                ppack->SandboxGetAllowFurniture() &&
                !pfurn->CanSitOn() &&
                !pfurn->CanSleepOn() &&
                (!apRef->GetOwner() || apRef->IsAnOwner(pactor, true))
            ) {
                ppackData->PotentialTargets.Add(SandBoxChoiceListItem(apRef, 0, 1u));
                ppackData->bChair = true;
            }
            break;
        }
        case NPC__ID:
        case CREA_ID: {
            if (ppack->SandboxGetAllowConversation() && apRef->IsActor() && apRef != pPlayer && apRef != pactor) {
                ppackData->PotentialTargets.Add(SandBoxChoiceListItem(apRef, 5, 1u));
            }
            break;
        }
        case IDLM_ID: {
            if (ppack->SandboxGetAllowIdleMarkers() && (!apRef->GetOwner() || apRef->IsAnOwner(pactor, true))) {
                BGSIdleMarker* pIdleMark = static_cast<BGSIdleMarker*>(apRef->GetObjectReference());
                if (pIdleMark->HasIdleForActor(pactor, apRef)) {
                    ppackData->PotentialTargets.Add(SandBoxChoiceListItem(apRef, 4, 1u));
                }
            }
            break;
        }
        default: {
            break;
        }
    }
    return false;
}
