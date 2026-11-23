#include "Fallout/AI/BaseProcess.h"

#include "BSAudio/BSSoundHandle.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESScriptFunctions.h"
#include "Fallout/AI/AITaskManager.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/ActorPackage.h"
#include "Fallout/AI/ActorPackageData.h"
#include "Fallout/AI/CachedValues.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/LowProcess.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/MiddleLowProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "SDK/MSVC/rtti.h"

#include <limits>

BaseProcess::BaseProcess() : pCachedValues(nullptr) {}

BaseProcess::~BaseProcess() {}

void BaseProcess::Copy(BaseProcess* apProcess) {
    HighProcess* phighProcess;
    void* pmiddleLowProcess;
    HighProcess* pthisHighProcess;
    MiddleHighProcess* pmiddleHighProcess;
    MiddleHighProcess* pthisMiddleHighProcess;
    void* plowProcess;
    void* pthisMiddleLowProcess;
    void* pthisLowProcess;
    if (apProcess) {
        phighProcess = dynamic_cast<HighProcess*>(apProcess);
        pmiddleHighProcess = dynamic_cast<MiddleHighProcess*>(apProcess);
        pmiddleLowProcess = dynamic_cast<MiddleLowProcess*>(apProcess);
        plowProcess = dynamic_cast<LowProcess*>(apProcess);
        pthisHighProcess = dynamic_cast<HighProcess*>(this);
        pthisMiddleHighProcess = dynamic_cast<MiddleHighProcess*>(this);
        pthisMiddleLowProcess = dynamic_cast<MiddleLowProcess*>(this);
        pthisLowProcess = dynamic_cast<LowProcess*>(this);
        if (phighProcess && phighProcess->GetAwarePlayerTimer() > 0.0) {
            phighProcess->ClearRunOncePackage();
            phighProcess->SetTarget(nullptr);
        }

        if (pthisHighProcess && pthisHighProcess->GetAwarePlayerTimer() > 0.0) {
            pthisHighProcess->ClearRunOncePackage();
            pthisHighProcess->SetTarget(nullptr);
        }

        if (pthisHighProcess && phighProcess) {
            pthisHighProcess->HighCopy(phighProcess);
        }

        if (pthisMiddleHighProcess && pmiddleHighProcess) {
            pthisMiddleHighProcess->MiddleHighCopy(pmiddleHighProcess);
        }

        if (pthisMiddleLowProcess && pmiddleLowProcess) {
            static_cast<MiddleLowProcess*>(pthisMiddleLowProcess)
                ->MiddleLowCopy(static_cast<MiddleLowProcess*>(pmiddleLowProcess));
        }

        if (pthisLowProcess && plowProcess) {
            static_cast<LowProcess*>(pthisLowProcess)->LowCopy(static_cast<LowProcess*>(plowProcess));
        }

        if (pCachedValues && apProcess->pCachedValues) {
            pCachedValues->Copy(apProcess->pCachedValues);
        }
    }
}

void BaseProcess::HandleDeletedReference(TESObjectREFR* apRef) {
    CurrentPackage.HandleDeletedReference(apRef);
    ActorPackageData* pactorPackageData = BaseProcess::GetInstanceDataThatIsRunning();
    if (pactorPackageData) {
        pactorPackageData->HandleDeletedReference(apRef);
    }
}

NiPoint3 BaseProcess::GetCurrentPackageLocation(Actor*) {
    return NiPoint3::ZERO;
}

float BaseProcess::GetLastTimeDetected(Actor*, int32_t) {
    return -std::numeric_limits<float>::max();
}

bool BaseProcess::SetupSpecialIdle(MobileObject*, TESIdleForm*, AnimIdle::PLAY_TYPE_ENUM, bool, bool, bool) {
    return false;
}

DetectionState* BaseProcess::InsertIntoDetectionList(Actor*, DETECTION_LEVEL, bool, int32_t, int32_t, bool, bool) {
    return nullptr;
}

void BaseProcess::SetActorsDetectionEvent(Actor*, NiPoint3, int32_t, int32_t, TESObjectREFR*) {}

void BaseProcess::
    AddQueuedItem(Actor*, bool, TESBoundObject*, int32_t, ExtraDataList*, bool, bool, bool, bool, bool, bool) {}

float BaseProcess::GetPotentialThreatDamage(Actor*, Actor*) {
    return 0.0f;
}

TESPackage* BaseProcess::GetCurrentPackage() {
    return CurrentPackage.pPackage;
}

void BaseProcess::SetCurrentPackage(TESPackage* apNewPackage, MobileObject* apMob) {
    TESObjectREFR* apTarget = apNewPackage->ComputeInitialTarget(apMob);
    CurrentPackage.InitFromPackage(apNewPackage, apTarget);
}

void BaseProcess::ClearCurrentPackage() {
    CurrentPackage.Clear();
}

void BaseProcess::SetCurrentProcedureIndex(int32_t aiNewIndex) {
    CurrentPackage.iCurrentProcedureIndex = aiNewIndex;
}

int32_t BaseProcess::GetCurrentProcedureIndex() {
    return CurrentPackage.iCurrentProcedureIndex;
}

void BaseProcess::AddToCurrentProcedureIndex(int32_t aiOffset) {
    CurrentPackage.AddToCurrentProcedureIndex(aiOffset);
}

void BaseProcess::AddAvoidPathingArea(Actor*, NiPoint3, float, float, TESObjectREFR*, TESObjectSTAT*) {}

ActorPackageData* BaseProcess::GetInstanceDataThatIsRunning() {
    return CurrentPackage.pData;
}

ActorPackage* BaseProcess::GetActorPackageThatIsRunning() {
    return &CurrentPackage;
}

TESPackage* BaseProcess::GetPackageThatIsRunning() {
    return GetCurrentPackage();
}

int32_t BaseProcess::GetProcedureIndexRunning() {
    return GetCurrentProcedureIndex();
}

void BaseProcess::SetProcedureIndexRunning(int32_t aiNewIndex) {
    SetCurrentProcedureIndex(aiNewIndex);
}

void BaseProcess::AddToProcedureIndexRunning(MobileObject* apActor, int32_t ainumb) {
    AddToCurrentProcedureIndex(ainumb);
}

// the base process keeps no controller; the by-value pointer's destructor releases it
void BaseProcess::SetCharController(NiPointer<bhkCharacterController>) {}

bool BaseProcess::
    EnterCombat(Actor*, Actor*, bool, bool, CombatGroup*, bool, bool, bool, char, bool, bool, bool, TESPackage*) {
    return false;
}

int32_t BaseProcess::CalculateMoveMode(Actor*, float, float, float, bool, bool) {
    return 0x101;
}

int32_t BaseProcess::GetActorValue(TESActorBase*, ActorValue::Index, Actor*) {
    return -1;
}

float BaseProcess::GetActorFloatValue(TESActorBase*, ActorValue::Index, Actor*) {
    return 0.0f;
}

const NiPoint3* BaseProcess::GetWeaponLastPos() {
    return &NiPoint3::ZERO;
}

// Returns a handle that plays nothing.
BSSoundHandle BaseProcess::GetSoundHandle(ACTOR_SOUND_HANDLE aeType) {
    return BSSoundHandle();
}

void BaseProcess::SetSoundHandle(ACTOR_SOUND_HANDLE aeType, BSSoundHandle aSound) {}

uint16_t BaseProcess::GetSaveSize(uint32_t aiFlags, MobileObject* pmob) {
    int16_t iblockHeaderSize = 0;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        iblockHeaderSize = 6;
    }
    uint16_t isize = CurrentPackage.GetSaveSize(aiFlags) + iblockHeaderSize + 4;
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                isize,
                pheader->iFormID,
                pform->GetFormEditorID(),
                pheader->iFlags,
                232,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\BaseProcess.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s",
                isize,
                232,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\BaseProcess.cpp"
            );
        }
    }
    return isize;
}

// The old-format save: with save blocks, a 'BLOK' marker and a 16-bit size slot, written back at the end; then the
// current package and the system time clock.
void BaseProcess::SaveGame(uint32_t aiFlags, MobileObject* pmob) {
    uint32_t iblockHeader;
    uint16_t iblockSize;
    iblockSize = 0;
    // The reserved size slot in the save buffer (untyped bytes there).
    void* pblockSize = nullptr;
    uint8_t* itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    if (bDebugSaveBuffer.Bool()) {
        itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        iblockHeader = 0x424C4F4B;
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockHeader, sizeof(iblockHeader));
        pblockSize = pSaveLoadGameOLD->m_pBuffer;
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockSize, sizeof(iblockSize));
    }
    CurrentPackage.SaveGame(aiFlags);
    pSaveLoadGameOLD->SaveGameDataOLD(&fSystemTimeClock, sizeof(fSystemTimeClock));
    if (bDebugSaveBuffer.Bool()) {
        uint8_t* pcurrent = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            uint32_t iFlags = pheader->iFlags;
            const char* pEditorID = pform->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                static_cast<int32_t>(pcurrent - itempStartPosition),
                pheader->iFormID,
                pEditorID,
                iFlags,
                249,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\BaseProcess.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s",
                static_cast<int32_t>(pcurrent - itempStartPosition),
                249,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\BaseProcess.cpp"
            );
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* pcurrent = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        uint8_t* pblockStart = static_cast<uint8_t*>(pblockSize);
        if (pcurrent > pblockStart + 0xFFFF) {
            Warning(
                "SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\BaseProcess.cpp",
                249
            );
        }
        *static_cast<uint16_t*>(pblockSize) = static_cast<uint16_t>(pcurrent - pblockStart);
    }
}

// The clock the process last updated at, the hour and the date it last ran, then the package.
void BaseProcess::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fSystemTimeClock), sizeof(fSystemTimeClock), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fHourLastProcessed), sizeof(fHourLastProcessed), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iDateLastProcessed), sizeof(iDateLastProcessed), 0);
    CurrentPackage.SaveGame(apSaveGameBuffer);
}

// The old-format load: with save blocks, the 'BLOK' marker is checked and the block's 16-bit size read; then the
// current package and the system time clock, and the buffer position is checked against the block's end.
void BaseProcess::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags, MobileObject* pmob) {
    uint32_t iblockHeader;
    uint16_t iblockSize;
    iblockSize = 0;
    uint8_t* istartBufferPosition = nullptr;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        pSaveLoadGameOLD->LoadGameDataOLD(&iblockHeader, sizeof(iblockHeader));
        if (iblockHeader != 0x424C4F4B) {
            LoadFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
            if (pheader) {
                TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Currently "
                    "loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\BaseProcess.cpp",
                    260,
                    pheader->iFormID,
                    pform->GetFormEditorID(),
                    pheader->cVersion,
                    pheader->iFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Current "
                    "version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\BaseProcess.cpp",
                    260,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        istartBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->LoadGameDataOLD(&iblockSize, sizeof(iblockSize));
    }
    CurrentPackage.LoadGameTES(aiFlags, aiCurrentFlags);
    pSaveLoadGameOLD->LoadGameDataOLD(&fSystemTimeClock, sizeof(fSystemTimeClock));
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* icurrentBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        LoadFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            if (icurrentBufferPosition <= &istartBufferPosition[iblockSize]) {
                if (icurrentBufferPosition < &istartBufferPosition[iblockSize]) {
                    uint32_t iFlags = pheader->iFlags;
                    int32_t cVersion = pheader->cVersion;
                    const char* pEditorID = pform->GetFormEditorID();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading "
                        "form is %08X %s with version %i and flags %08X",
                        static_cast<int32_t>(&istartBufferPosition[iblockSize] - icurrentBufferPosition),
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\BaseProcess.cpp",
                        265,
                        pheader->iFormID,
                        pEditorID,
                        cVersion,
                        iFlags
                    );
                }
            }
            else {
                uint32_t iFlags = pheader->iFlags;
                int32_t cVersion = pheader->cVersion;
                const char* pEditorID = pform->GetFormEditorID();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading form "
                    "is %08X %s with version %i and flags %08X",
                    static_cast<int32_t>(icurrentBufferPosition - &istartBufferPosition[iblockSize]),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\BaseProcess.cpp",
                    265,
                    pheader->iFormID,
                    pEditorID,
                    cVersion,
                    iFlags
                );
            }
        }
        else if (icurrentBufferPosition <= &istartBufferPosition[iblockSize]) {
            if (icurrentBufferPosition < &istartBufferPosition[iblockSize]) {
                int32_t iCurrentVersion = pSaveLoadGameOLD->GetCurrentVersion();
                Warning(
                    "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is "
                    "%i",
                    static_cast<int32_t>(&istartBufferPosition[iblockSize] - icurrentBufferPosition),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\BaseProcess.cpp",
                    265,
                    iCurrentVersion
                );
            }
        }
        else {
            int32_t iCurrentVersion = pSaveLoadGameOLD->GetCurrentVersion();
            Warning(
                "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                static_cast<int32_t>(icurrentBufferPosition - &istartBufferPosition[iblockSize]),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\BaseProcess.cpp",
                265,
                iCurrentVersion
            );
        }
    }
}

// The clock, then from save version 6 the hour and the date, then the package.
void BaseProcess::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fSystemTimeClock), sizeof(fSystemTimeClock));
    if (apLoadGameBuffer->GetVersion() >= 6) {
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fHourLastProcessed), sizeof(fHourLastProcessed));
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iDateLastProcessed), sizeof(iDateLastProcessed));
    }
    CurrentPackage.LoadGame(apLoadGameBuffer);
}

void BaseProcess::InitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags, MobileObject* pmob) {
    CurrentPackage.InitLoadGameTES(aiFlags, aiOldFlags);
}

void BaseProcess::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    CurrentPackage.InitLoadGameBGS(apLoadGameBuffer);
    if (pCachedValues) {
        pCachedValues->Reset();
    }
}

void BaseProcess::FinishInitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags, MobileObject* pmob) {
    PlayerCharacter* pactor = static_cast<PlayerCharacter*>(dynamic_cast<Actor*>(pmob));
    if (pactor && (pactor != pPlayer && !GetCurrentPackage() && GetProcedureIndexRunning() == -1)) {
        SetProcedureIndexRunning(0);
        pactor->EvaluatePackage(true, true);
    }
}

// ActorPackage::Revert ignores both the flags and the process.
void BaseProcess::Revert(uint32_t aiFlags, MobileObject* pmob) {
    CurrentPackage.RevertTES(static_cast<int32_t>(aiFlags));
}

// Forgets when the actor was last processed, then reverts the package and marks the cached values stale.
void BaseProcess::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
    fHourLastProcessed = -1.0f;
    iDateLastProcessed = 0;
    CurrentPackage.RevertBGS(apLoadGameBuffer);
    if (pCachedValues) {
        pCachedValues->Reset();
    }
}

void BaseProcess::FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    CurrentPackage.FinishLoadGame(apLoadGameBuffer);
}

PROCESS_TYPE BaseProcess::GetProcessLevel() {
    return eLevel;
}

void BaseProcess::SetLastTimeProcessed(float afTime) {
    fHourLastProcessed = afTime;
}

float BaseProcess::GetLastTimeProcessed() {
    return fHourLastProcessed;
}

uint32_t BaseProcess::GetLastDateProcessed() {
    return iDateLastProcessed;
}

void BaseProcess::SetLastDateProcessed(uint32_t aiDate) {
    iDateLastProcessed = aiDate;
}

bool BaseProcess::HasCachedValues() {
    return pCachedValues != nullptr;
}

bool BaseProcess::GetCachedFlag(uint32_t aeWhich) {
    return pCachedValues && (pCachedValues->iFlags & aeWhich) != 0;
}

void BaseProcess::SetCachedValueDirty(CachedValues::Flags aeWhich) {
    if (pCachedValues) {
        pCachedValues->SetDirty(aeWhich);
    }
}

void BaseProcess::SetCachedValueFlag(CachedValues::Flags aeWhich, bool abSet) {
    if (pCachedValues) {
        pCachedValues->SetFlag(aeWhich, abSet);
    }
}

void BaseProcess::SetCachedRadius(float afRadius) {
    if (pCachedValues) {
        pCachedValues->fCachedRadius = afRadius;
        pCachedValues->SetUpToDate(CachedValues::CVF_RADIUS);
    }
}

void BaseProcess::SetCachedParalysis(float afParalysis) {
    if (pCachedValues) {
        pCachedValues->fCachedParalysis = afParalysis;
        pCachedValues->SetUpToDate(CachedValues::CVF_PARALYSIS);
    }
}

float BaseProcess::GetRadius(CachedValuesOwner* apOwner) {
    if (!pCachedValues) {
        return 0.0f;
    }
    if (pCachedValues->IsDirty(CachedValues::CVF_RADIUS)) {
        float fValue = apOwner->CalculateCachedRadius();
        return fValue;
    }
    return pCachedValues->fCachedRadius;
}

float BaseProcess::GetWidth(CachedValuesOwner* apOwner) {
    if (!pCachedValues) {
        return 0.0f;
    }
    if (pCachedValues->IsDirty(CachedValues::CVF_WIDTH)) {
        // The computed value is rounded through a float, as in GetRadius.
        float fValue = apOwner->CalculateCachedWidth();
        return fValue;
    }
    return pCachedValues->fCachedWidth;
}

float BaseProcess::GetLength(CachedValuesOwner* apOwner) {
    if (!pCachedValues) {
        return 0.0f;
    }
    if (pCachedValues->IsDirty(CachedValues::CVF_LENGTH)) {
        // The computed value is rounded through a float, as in GetRadius.
        float fValue = apOwner->CalculateCachedLength();
        return fValue;
    }
    return pCachedValues->fCachedLength;
}

// The forward length, under the kCached_Height bit.
float BaseProcess::GetCachedForwardLength(CachedValuesOwner* apOwner) {
    if (!pCachedValues) {
        return 0.0f;
    }
    if (pCachedValues->IsDirty(CachedValues::CVF_FORWARD_LENGTH)) {
        // The computed value is rounded through a float, as in GetRadius.
        float fValue = apOwner->CalculateCachedForwardLength();
        return fValue;
    }
    return pCachedValues->fCachedForwardLength;
}

float BaseProcess::GetPerceptionCondition(CachedValuesOwner* apOwner) {
    if (!pCachedValues) {
        return 0.0f;
    }
    if (pCachedValues->IsDirty(CachedValues::CVF_PERCEPTION_CONDITION)) {
        // The computed value is rounded through a float, as in GetRadius.
        float fValue = apOwner->CalculateCachedPerceptionCondition();
        return fValue;
    }
    return pCachedValues->fCachedPerceptionCondition;
}

float BaseProcess::GetEyeLevel(CachedValuesOwner* apOwner) {
    if (!pCachedValues) {
        return 0.0f;
    }
    if (pCachedValues->IsDirty(CachedValues::CVF_EYE_LEVEL)) {
        // The computed value is rounded through a float, as in GetRadius.
        float fValue = apOwner->CalculateCachedEyeLevel();
        return fValue;
    }
    return pCachedValues->fCachedEyeLevel;
}

float BaseProcess::GetParalysis(CachedValuesOwner* apOwner) {
    if (!pCachedValues) {
        return 0.0f;
    }
    if (pCachedValues->IsDirty(CachedValues::CVF_PARALYSIS)) {
        // The computed value is rounded through a float, as in GetRadius.
        float fValue = apOwner->CalculateCachedParalysis();
        return fValue;
    }
    return pCachedValues->fCachedParalysis;
}

int32_t BaseProcess::GetCachedActorAggression(CachedValuesOwner* apOwner) {
    if (!pCachedValues) {
        return 0;
    }
    if (pCachedValues->IsDirty(CachedValues::CVF_ACTOR_AGGRESSION)) {
        return apOwner->CalculateCachedActorAggression();
    }

    return pCachedValues->eCachedActorAggression;
}

void BaseProcess::SetCachedActorAggression(int32_t aeActorAggression) {
    if (pCachedValues) {
        pCachedValues->eCachedActorAggression = aeActorAggression;
        pCachedValues->SetUpToDate(CachedValues::CVF_ACTOR_AGGRESSION);
    }
}

void BaseProcess::SetCachedEndurance(float afEndurance) {
    if (pCachedValues) {
        pCachedValues->fCachedEndurance = afEndurance;
        pCachedValues->SetUpToDate(CachedValues::CVF_ENDURANCE);
    }
}

void BaseProcess::SetCachedWidth(float afWidth) {
    if (pCachedValues) {
        pCachedValues->fCachedWidth = afWidth;
        pCachedValues->SetUpToDate(CachedValues::CVF_WIDTH);
    }
}

void BaseProcess::SetCachedForwardLength(float afLength) {
    if (pCachedValues) {
        pCachedValues->fCachedForwardLength = afLength;
        pCachedValues->SetUpToDate(CachedValues::CVF_FORWARD_LENGTH);
    }
}

void BaseProcess::SetCachedEyeLevel(float afEyeLevel) {
    if (pCachedValues) {
        pCachedValues->fCachedEyeLevel = afEyeLevel;
        pCachedValues->SetUpToDate(CachedValues::CVF_EYE_LEVEL);
    }
}

void BaseProcess::SetCachedWeaponDPS(float afWeaponDPS) {
    if (pCachedValues) {
        pCachedValues->fCachedWeaponDPS = afWeaponDPS;
        pCachedValues->SetUpToDate(CachedValues::CVF_WEAPON_DPS);
    }
}

void BaseProcess::SetCachedActorAssistance(int32_t aeActorAssistance) {
    if (pCachedValues) {
        pCachedValues->eCachedActorAssistance = aeActorAssistance;
        pCachedValues->SetUpToDate(CachedValues::CVF_ACTOR_ASSISTANCE);
    }
}

void BaseProcess::SetCachedConditionPreventsRun(bool abCanRun) {
    if (pCachedValues) {
        pCachedValues->bConditionPreventsRun = abCanRun;
        pCachedValues->SetUpToDate(CachedValues::CVF_CONDITION_PREVENTS_RUN);
    }
}

double BaseProcess::GetWalkSpeed(CachedValuesOwner* apOwner) {
    if (pCachedValues) {
        if (pCachedValues->IsDirty(CachedValues::CVF_WALK_SPEED)) {
            return apOwner->CalculateCachedWalkSpeed();
        }

        return pCachedValues->fCachedWalkSpeed;
    }

    return 0.0f;
}

double BaseProcess::GetCachedRunSpeed(CachedValuesOwner* apOwner) {
    if (pCachedValues) {
        if (pCachedValues->IsDirty(CachedValues::CVF_RUN_SPEED)) {
            return apOwner->CalculateCachedRunSpeed();
        }

        return pCachedValues->fCachedRunSpeed;
    }

    return 0.0f;
}

double BaseProcess::GetDamagePerSecond(CachedValuesOwner* apOwner) {
    if (pCachedValues) {
        if (pCachedValues->IsDirty(CachedValues::CVF_WEAPON_DPS)) {
            return apOwner->CalculateCachedWeaponDPS();
        }

        return pCachedValues->fCachedWeaponDPS;
    }

    return 0.0f;
}

double BaseProcess::GetHealRate(CachedValuesOwner* apOwner) {
    if (pCachedValues) {
        if (pCachedValues->IsDirty(CachedValues::CVF_HEALING_RATE)) {
            return apOwner->CalculateCachedHealingRate();
        }

        return pCachedValues->fCachedHealingRate;
    }

    return 0.0f;
}

int32_t BaseProcess::GetAssistance(CachedValuesOwner* apOwner) {
    if (!pCachedValues) {
        return 0;
    }
    if (pCachedValues->IsDirty(CachedValues::CVF_ACTOR_ASSISTANCE)) {
        return apOwner->CalculateCachedActorAssistance();
    }

    return pCachedValues->eCachedActorAssistance;
}

double BaseProcess::GetMedicineSkillMult(CachedValuesOwner* apOwner) {
    if (pCachedValues) {
        if (pCachedValues->IsDirty(CachedValues::CVF_MEDICINE_EFFECTIVENESS_MULT)) {
            return apOwner->CalculateCachedMedicineEffectivenessMult();
        }

        return pCachedValues->fCachedMedicineEffectivenessMult;
    }

    return 0.0f;
}

double BaseProcess::GetSurvivalSkillMult(CachedValuesOwner* apOwner) {
    if (pCachedValues) {
        if (pCachedValues->IsDirty(CachedValues::kCached_SurvivalSkillMult)) {
            return apOwner->CalculateCachedSurvivalEffectivenessMult();
        }

        return pCachedValues->fCachedSurvivalEffectivenessMult;
    }

    return 0.0f;
}

void ActorPackage::AddToCurrentProcedureIndex(int32_t aiOffset) {
    iCurrentProcedureIndex += aiOffset;
}

// Marks every cached value stale.
void CachedValues::Reset() {
    iFlags = 0;
}
