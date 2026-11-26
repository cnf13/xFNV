#include "Fallout Shared/PackageData.h"

#include "BSCore/RTTI.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout Shared/PackageTarget.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESTopic.h"

void PACK_DIALOGUE_DATA::Endian() {
    BSEndian(fFov, false);
    BSEndian(iTopicID, false);
    BSEndian(eTriggerType, false);
    BSEndian(fDistanceStartTalking, false);
}

void PACK_PATROL_DATA::Endian() {}

void PACK_USE_WEAPON_DATA_PKW2::Endian() {
    BSEndian(sBurstCount, false);
    BSEndian(sVolleyShotsMin, false);
    BSEndian(sVolleyShotsMax, false);
    BSEndian(fVolleyWaitMin, false);
    BSEndian(fVolleyWaitMax, false);
}

void PACK_USE_WEAPON_DATA_PKW3::Endian() {
    BSEndian(sBurstCount, false);
    BSEndian(sVolleyShotsMin, false);
    BSEndian(sVolleyShotsMax, false);
    BSEndian(fVolleyWaitMin, false);
    BSEndian(fVolleyWaitMax, false);
}

TESAmbushPackageData::TESAmbushPackageData() : pAmbushLocation(nullptr) {}

TESDialoguePackageData::TESDialoguePackageData()
  : fFov(100.0f), ptopic(nullptr), pTargetLocation(nullptr), bNOHeadtracking(false), bDoNotControlTarget(false),
    fDistanceStartTalking(0.0f), bSayTo(false), eTriggerType(PDTT_NONE) {}

// The topic's form ID is kept in the pointer until InitItem looks the form up.
TESDialoguePackageData::TESDialoguePackageData(PACK_DIALOGUE_DATA eDialData)
  : fFov(eDialData.fFov), ptopic(reinterpret_cast<TESTopic*>(static_cast<uintptr_t>(eDialData.iTopicID))),
    pTargetLocation(nullptr), bNOHeadtracking(eDialData.bNOHeadtracking),
    bDoNotControlTarget(eDialData.bDoNotControlTarget), fDistanceStartTalking(eDialData.fDistanceStartTalking),
    bSayTo(eDialData.bSayTo), eTriggerType(eDialData.eTriggerType) {}

void TESDialoguePackageData::InitItem(TESForm* apOwner) {
    // Before initialization the topic pointer field holds the unresolved numeric form ID.
    uint32_t aiID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(ptopic));
    TESFile* file = apOwner->GetFile(-1);
    TESForm::AddCompileIndex(aiID, file);
    TESForm* form = TESForm::GetFormByNumericID(aiID);
    ptopic = dynamic_cast<TESTopic*>(form);
    if (!ptopic && aiID) {
        uint32_t ownerID = apOwner->GetFormID();
        const char* editorID = apOwner->GetFormEditorID();
        Warning(
            "FORMS: Failed to find topic (%08X) in init for dialogue package data on package '%s' (%08X).",
            aiID,
            editorID,
            ownerID
        );
    }
}

TESEatPackageData::TESEatPackageData() : pSearchLocation(nullptr) {}

TESEscortPackageData::TESEscortPackageData() : pSearchLocation(nullptr), iFollowPackRadius(200) {}

TESFollowPackageData::TESFollowPackageData() : pStartLocation(nullptr), fTriggerRadius(0.0f) {}

TESFollowPackageData::TESFollowPackageData(PACK_FOLLOW_DATA eFolData)
  : pStartLocation(nullptr), fTriggerRadius(eFolData.fTriggerRadius) {}

TESPatrolPackageData::TESPatrolPackageData() : bRepeatable(true), bStartingLocationAtLinkedRef(false) {}

TESPatrolPackageData::TESPatrolPackageData(PACK_PATROL_DATA aPatrolData)
  : bRepeatable(aPatrolData.bRepeatable), bStartingLocationAtLinkedRef(aPatrolData.bStartingLocationAtLinkedRef) {}

TESUseItemPackageData::TESUseItemPackageData() : pSearchLocation(nullptr) {}

TESUseWeaponPackageData::TESUseWeaponPackageData()
  : pAttackTarget(nullptr), pTargetLocation(nullptr), bAlwaysHit(false), bDoNoDamage(false), bCrouch(true),
    bHoldFire(true), bVolleyFire(false), bRepeatFire(false), sBurstCount(1), sVolleyShotsMin(1), sVolleyShotsMax(6),
    fVolleyWaitMin(1.0f), fVolleyWaitMax(2.0f), pWeapon(nullptr) {}

// The old file data has no repeat flag: the fire repeats when the burst count is 0.
TESUseWeaponPackageData::TESUseWeaponPackageData(PACK_USE_WEAPON_DATA_PKW2 eDialData)
  : pAttackTarget(nullptr), pTargetLocation(nullptr), bAlwaysHit(eDialData.bAlwaysHit),
    bDoNoDamage(eDialData.bDoNoDamage), bCrouch(eDialData.bCrouch), bHoldFire(eDialData.bHoldFire),
    bVolleyFire(eDialData.bVolleyFire), bRepeatFire(eDialData.sBurstCount == 0), sBurstCount(eDialData.sBurstCount),
    sVolleyShotsMin(eDialData.sVolleyShotsMin), sVolleyShotsMax(eDialData.sVolleyShotsMax),
    fVolleyWaitMin(eDialData.fVolleyWaitMin), fVolleyWaitMax(eDialData.fVolleyWaitMax), pWeapon(nullptr) {}

// The weapon's form ID is kept in the pointer until the form is resolved.
TESUseWeaponPackageData::TESUseWeaponPackageData(PACK_USE_WEAPON_DATA_PKW3 eDialData)
  : pAttackTarget(nullptr), pTargetLocation(nullptr), bAlwaysHit(eDialData.bAlwaysHit),
    bDoNoDamage(eDialData.bDoNoDamage), bCrouch(eDialData.bCrouch), bHoldFire(eDialData.bHoldFire),
    bVolleyFire(eDialData.bVolleyFire), bRepeatFire(eDialData.bRepeatFire), sBurstCount(eDialData.sBurstCount),
    sVolleyShotsMin(eDialData.sVolleyShotsMin), sVolleyShotsMax(eDialData.sVolleyShotsMax),
    fVolleyWaitMin(eDialData.fVolleyWaitMin), fVolleyWaitMax(eDialData.fVolleyWaitMax),
    pWeapon(reinterpret_cast<TESObjectWEAP*>(static_cast<uintptr_t>(eDialData.iWeapon))) {}

TESUseWeaponPackageData::~TESUseWeaponPackageData() {
    delete pAttackTarget;
    pAttackTarget = nullptr;
    delete pTargetLocation;
    pTargetLocation = nullptr;
}

uint16_t TESUseWeaponPackageData::GetVolleyShotsMax() const {
    return sVolleyShotsMax;
}

bool TESUseWeaponPackageData::GetDoNoDamage() const {
    return bDoNoDamage;
}

bool TESUseWeaponPackageData::GetRepeatFire() const {
    return bRepeatFire;
}

bool TESUseWeaponPackageData::IsCrouch() const {
    return bCrouch;
}

char TESUseWeaponPackageData::IsGetHoldFire() const {
    return bHoldFire;
}

uint16_t TESUseWeaponPackageData::GetVolleyShotsMin() const {
    return sVolleyShotsMin;
}
