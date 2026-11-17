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

BaseProcess::BaseProcess() : pCachedValues(nullptr) {}

BaseProcess::~BaseProcess() {}

void BaseProcess::Copy(BaseProcess* apProcess) {
    HighProcess* a2;
    void* v4;
    HighProcess* v5;
    MiddleHighProcess* apProcess_2;
    MiddleHighProcess* v7;
    void* arg0;
    void* v9;
    void* v10;
    if (apProcess) {
        a2 = dynamic_cast<HighProcess*>(apProcess);
        apProcess_2 = dynamic_cast<MiddleHighProcess*>(apProcess);
        v4 = dynamic_cast<MiddleLowProcess*>(apProcess);
        arg0 = dynamic_cast<LowProcess*>(apProcess);
        v5 = dynamic_cast<HighProcess*>(this);
        v7 = dynamic_cast<MiddleHighProcess*>(this);
        v9 = dynamic_cast<MiddleLowProcess*>(this);
        v10 = dynamic_cast<LowProcess*>(this);
        if (a2) {
            if (a2->GetAwarePlayerTimer() > 0.0) {
                a2->ClearRunOncePackage();
                a2->SetTarget(nullptr);
            }
        }
        if (v5) {
            if (v5->GetAwarePlayerTimer() > 0.0) {
                v5->ClearRunOncePackage();
                v5->SetTarget(nullptr);
            }
        }
        if (v5) {
            if (a2) {
                v5->HighCopy(a2);
            }
        }
        if (v7) {
            if (apProcess_2) {
                v7->MiddleHighCopy(apProcess_2);
            }
        }
        if (v9) {
            if (v4) {
                static_cast<MiddleLowProcess*>(v9)->MiddleLowCopy(static_cast<MiddleLowProcess*>(v4));
            }
        }
        if (v10) {
            if (arg0) {
                static_cast<LowProcess*>(v10)->LowCopy(static_cast<LowProcess*>(arg0));
            }
        }
        if (pCachedValues) {
            if (apProcess->pCachedValues) {
                pCachedValues->Copy(apProcess->pCachedValues);
            }
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
    return NiPoint3_ZERO;
}

float BaseProcess::GetLastTimeDetected(Actor*, int32_t) {
    return -3.402823466e+38f;
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
    return &NiPoint3_ZERO;
}

// Returns a handle that plays nothing.
BSSoundHandle BaseProcess::GetSoundHandle(ACTOR_SOUND_HANDLE aeType) {
    return BSSoundHandle();
}

void BaseProcess::SetSoundHandle(ACTOR_SOUND_HANDLE aeType, BSSoundHandle aSound) {}

uint16_t BaseProcess::GetSaveSize(uint32_t aiFlags, MobileObject* pmob) {
    int16_t v7 = 0;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        v7 = 6;
    }
    uint16_t isize = CurrentPackage.GetSaveSize(aiFlags) + v7 + 4;
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(pheader->iFormID);
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s", isize,
                pheader->iFormID, FormByNumericID->GetFormEditorID(), pheader->iFlags, 232,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\BaseProcess.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s", isize, 232,
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
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockHeader, 4u);
        pblockSize = pSaveLoadGameOLD->m_pBuffer;
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockSize, 2u);
    }
    CurrentPackage.SaveGame(aiFlags);
    pSaveLoadGameOLD->SaveGameDataOLD(&fSystemTimeClock, 4u);
    if (bDebugSaveBuffer.Bool()) {
        uint8_t* BufferPointer = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            uint32_t iFlags = pheader->iFlags;
            const char* v3 = pform->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                static_cast<int32_t>(BufferPointer - itempStartPosition), pheader->iFormID, v3, iFlags, 249,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\BaseProcess.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s",
                static_cast<int32_t>(BufferPointer - itempStartPosition), 249,
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
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\BaseProcess.cpp", 249
            );
        }
        *static_cast<uint16_t*>(pblockSize) = static_cast<uint16_t>(pcurrent - pblockStart);
    }
}

// The clock the process last updated at, the hour and the date it last ran, then the package.
void BaseProcess::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fSystemTimeClock), 4, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fHourLastProcessed), 4, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iDateLastProcessed), 4, 0);
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
        pSaveLoadGameOLD->LoadGameDataOLD(&iblockHeader, 4u);
        if (iblockHeader != 0x424C4F4B) {
            LoadFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
            if (pheader) {
                TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Currently "
                    "loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\BaseProcess.cpp", 260, pheader->iFormID,
                    pform->GetFormEditorID(), pheader->cVersion, pheader->iFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Current "
                    "version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\BaseProcess.cpp", 260,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        istartBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->LoadGameDataOLD(&iblockSize, 2u);
    }
    CurrentPackage.LoadGameTES(aiFlags, aiCurrentFlags);
    pSaveLoadGameOLD->LoadGameDataOLD(&fSystemTimeClock, 4u);
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* icurrentBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        LoadFormHeader* pheader_2 = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
        if (pheader_2) {
            TESForm* pform_2 = TESForm::GetFormByNumericID(pheader_2->iFormID);
            if (icurrentBufferPosition <= &istartBufferPosition[iblockSize]) {
                if (icurrentBufferPosition < &istartBufferPosition[iblockSize]) {
                    uint32_t iFlags = pheader_2->iFlags;
                    int32_t cVersion = pheader_2->cVersion;
                    const char* v5 = pform_2->GetFormEditorID();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading "
                        "form is %08X %s with version %i and flags %08X",
                        static_cast<int32_t>(&istartBufferPosition[iblockSize] - icurrentBufferPosition),
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\BaseProcess.cpp", 265, pheader_2->iFormID,
                        v5, cVersion, iFlags
                    );
                }
            }
            else {
                uint32_t v8 = pheader_2->iFlags;
                int32_t v6 = pheader_2->cVersion;
                const char* v4 = pform_2->GetFormEditorID();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading form "
                    "is %08X %s with version %i and flags %08X",
                    static_cast<int32_t>(icurrentBufferPosition - &istartBufferPosition[iblockSize]),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\BaseProcess.cpp", 265, pheader_2->iFormID, v4,
                    v6, v8
                );
            }
        }
        else {
            if (icurrentBufferPosition <= &istartBufferPosition[iblockSize]) {
                if (icurrentBufferPosition < &istartBufferPosition[iblockSize]) {
                    int32_t CurrentVersion = pSaveLoadGameOLD->GetCurrentVersion();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is "
                        "%i",
                        static_cast<int32_t>(&istartBufferPosition[iblockSize] - icurrentBufferPosition),
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\BaseProcess.cpp", 265, CurrentVersion
                    );
                }
            }
            else {
                int32_t v10 = pSaveLoadGameOLD->GetCurrentVersion();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                    static_cast<int32_t>(icurrentBufferPosition - &istartBufferPosition[iblockSize]),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\BaseProcess.cpp", 265, v10
                );
            }
        }
    }
}

// The clock, then from save version 6 the hour and the date, then the package.
void BaseProcess::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fSystemTimeClock), 4);
    if (apLoadGameBuffer->GetVersion() >= 6) {
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fHourLastProcessed), 4);
        apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iDateLastProcessed), 4);
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
    PlayerCharacter* v5 = static_cast<PlayerCharacter*>(dynamic_cast<Actor*>(pmob));
    if (v5) {
        if (((v5 != pPlayer) && (!GetCurrentPackage())) && (((uint32_t)GetProcedureIndexRunning()) == (-1))) {
            SetProcedureIndexRunning(0);
            v5->EvaluatePackage(true, true);
        }
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
    else {
        return pCachedValues->eCachedActorAggression;
    }
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
        pCachedValues->SetUpToDate(static_cast<CachedValues::Flags>(2048));
    }
}

void BaseProcess::SetCachedConditionPreventsRun(bool abCanRun) {
    if (pCachedValues) {
        pCachedValues->bConditionPreventsRun = (bool)((uint8_t)abCanRun);
        pCachedValues->SetUpToDate(CachedValues::CVF_CONDITION_PREVENTS_RUN);
    }
}

double BaseProcess::GetWalkSpeed(CachedValuesOwner* a1) {
    if (pCachedValues) {
        if (pCachedValues->IsDirty(CachedValues::CVF_WALK_SPEED)) {
            return a1->CalculateCachedWalkSpeed();
        }
        else {
            return pCachedValues->fCachedWalkSpeed;
        }
    }
    else {
        return 0.0f;
    }
}

double BaseProcess::GetCachedRunSpeed(CachedValuesOwner* a1) {
    if (pCachedValues) {
        if (pCachedValues->IsDirty(CachedValues::CVF_RUN_SPEED)) {
            return a1->CalculateCachedRunSpeed();
        }
        else {
            return pCachedValues->fCachedRunSpeed;
        }
    }
    else {
        return 0.0f;
    }
}

void BaseProcess::sub_83B7D0() {
    BaseProcess::ComputeLastTimeProcessed();
}

double BaseProcess::GetDamagePerSecond(CachedValuesOwner* apOwner) {
    if (pCachedValues) {
        if (pCachedValues->IsDirty(CachedValues::CVF_WEAPON_DPS)) {
            return apOwner->CalculateCachedWeaponDPS();
        }
        else {
            return pCachedValues->fCachedWeaponDPS;
        }
    }
    else {
        return 0.0f;
    }
}

double BaseProcess::GetHealRate(CachedValuesOwner* apOwner) {
    if (pCachedValues) {
        if (pCachedValues->IsDirty(CachedValues::CVF_HEALING_RATE)) {
            return apOwner->CalculateCachedHealingRate();
        }
        else {
            return pCachedValues->fCachedHealingRate;
        }
    }
    else {
        return 0.0f;
    }
}

int32_t BaseProcess::GetAssistance(CachedValuesOwner* apOwner) {
    if (!pCachedValues) {
        return 0;
    }
    if (pCachedValues->IsDirty(CachedValues::CVF_ACTOR_ASSISTANCE)) {
        return apOwner->CalculateCachedActorAssistance();
    }
    else {
        return pCachedValues->eCachedActorAssistance;
    }
}

double BaseProcess::GetMedicineSkillMult(CachedValuesOwner* a2) {
    if (pCachedValues) {
        if (pCachedValues->IsDirty(CachedValues::CVF_MEDICINE_EFFECTIVENESS_MULT)) {
            return a2->CalculateCachedMedicineEffectivenessMult();
        }
        else {
            return pCachedValues->fCachedMedicineEffectivenessMult;
        }
    }
    else {
        return 0.0f;
    }
}

double BaseProcess::GetSurvivalSkillMult(CachedValuesOwner* a2) {
    if (pCachedValues) {
        if (pCachedValues->IsDirty(CachedValues::kCached_SurvivalSkillMult)) {
            return a2->CalculateCachedSurvivalEffectivenessMult();
        }
        else {
            return pCachedValues->fCachedSurvivalEffectivenessMult;
        }
    }
    else {
        return 0.0f;
    }
}

void ActorPackage::AddToCurrentProcedureIndex(int32_t aiOffset) {
    iCurrentProcedureIndex += aiOffset;
}

// Marks every cached value stale.
void CachedValues::Reset() {
    iFlags = 0;
}
