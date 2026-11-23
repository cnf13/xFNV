#include "Fallout/AI/GuardPackage.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/ActorPackageData.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"

GuardActorPackageData::GuardActorPackageData()
  : eState(GuardActorPackageData::GUARD_STATE_INIT), pGuardWarnedActor(nullptr), iWarningCount(0), fGuardTimer(0.0f) {}

GuardActorPackageData::~GuardActorPackageData() {}

// The guard's warned actor is held as a form pointer, so its deletion arrives here: the guard forgets it.
void GuardActorPackageData::HandleDeletedReference(TESObjectREFR* pDeletedObj) {
    if (pDeletedObj == static_cast<TESObjectREFR*>(pGuardWarnedActor)) {
        pGuardWarnedActor = nullptr;
    }
}

PACKAGE_TYPE GuardActorPackageData::GetPackageType() {
    return PACKAGE_TYPE_GUARD;
}

// The state, timer and warning count, then the warned actor's form ID.
void GuardActorPackageData::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&eState), sizeof(eState), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fGuardTimer), sizeof(fGuardTimer), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iWarningCount), sizeof(iWarningCount), 0);
    apSaveGameBuffer->SaveFormID(pGuardWarnedActor, 0);
}

// The warned actor is loaded as its form ID; InitLoadGame resolves it. The loads go eState, fGuardTimer, iWarningCount,
// then pGuardWarnedActor, as SaveGame writes them.
void GuardActorPackageData::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&eState), sizeof(eState));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fGuardTimer), sizeof(fGuardTimer));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iWarningCount), sizeof(iWarningCount));
    apLoadGameBuffer->LoadFormID(pGuardWarnedActor);
}

// The warned actor field holds its form ID until the load game resolves it. With a null field the null is still stored
// back.
void GuardActorPackageData::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    Actor* pActor = nullptr;
    if (pGuardWarnedActor) {
        TESForm* pForm = TESForm::GetFormByNumericID(
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pGuardWarnedActor))
        );
        pActor = dynamic_cast<Actor*>(pForm);
    }
    pGuardWarnedActor = pActor;
    if (pGuardWarnedActor) {
        pGuardWarnedActor->TESObjectREFR::SetTargeted(true);
    }
}

void GuardActorPackageData::IncWarningCount() {
    ++iWarningCount;
}

void GuardActorPackageData::ClearWarningCount() {
    iWarningCount = 0;
}

void GuardActorPackageData::UpdateWarningTimer(float afTimeDelta) {
    fGuardTimer = fGuardTimer + afTimeDelta;
}

void GuardActorPackageData::SetTarget(Actor* apTarget) {
    pGuardWarnedActor = apTarget;
    pGuardWarnedActor->TESObjectREFR::SetTargeted(true);
}

void GuardActorPackageData::ClearWarningTimer() {
    fGuardTimer = 0.0f;
}
