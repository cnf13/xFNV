#include "Fallout Shared/TESCombatStyle.h"

#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"
#include "xfnv/Compiler.h"

// Swaps the floats read from a big-endian file in the game's order (fWeaponMaxRangeMult before fClusterDistance); the
// weapon restrictions byte is left alone.
void SimpleCombatStyleData::Endian() {
    BSEndian(fCoverSearchRadius, false);
    BSEndian(fCoverChance, false);
    BSEndian(fCoverWaitTimerMin, false);
    BSEndian(fCoverWaitTimerMax, false);
    BSEndian(fCoverWaitToFireTimerMin, false);
    BSEndian(fCoverWaitToFireTimerMax, false);
    BSEndian(fCoverFireTimerMin, false);
    BSEndian(fCoverFireTimerMax, false);
    BSEndian(fWeaponMinRangeMult, false);
    BSEndian(fWeaponMaxRangeMult, false);
    BSEndian(fClusterDistance, false);
    BSEndian(fTargetingFOV, false);
    BSEndian(fCombatRadius, false);
    BSEndian(fSemiAutoFiringDelayMinMult, false);
    BSEndian(fSemiAutoFiringDelayMaxMult, false);
}

// Swaps the 21 floats read from a big-endian file, in the game's order (fAttackDuringBlockMult before
// fAttackNoAttackMult).
void AdvancedCombatStyleData::Endian() {
    BSEndian(fDodgeFatigueMult, false);
    BSEndian(fDodgeFatigueBase, false);
    BSEndian(fDodgeSpeedBase, false);
    BSEndian(fDodgeSpeedMult, false);
    BSEndian(fDodgeDuringAttackMult, false);
    BSEndian(fDodgeNoAttackMult, false);
    BSEndian(fDodgeBackDuringAttackMult, false);
    BSEndian(fDodgeBackNoAttackMult, false);
    BSEndian(fDodgeForwardWhileAttackingMult, false);
    BSEndian(fDodgeForwardNotAttackingMult, false);
    BSEndian(fBlockSkillMult, false);
    BSEndian(fBlockSkillBase, false);
    BSEndian(fBlockDuringAttackMult, false);
    BSEndian(fBlockNoAttackMult, false);
    BSEndian(fAttackSkillMult, false);
    BSEndian(fAttackSkillBase, false);
    BSEndian(fAttackDuringAttackMult, false);
    BSEndian(fAttackDuringBlockMult, false);
    BSEndian(fAttackNoAttackMult, false);
    BSEndian(fPowerAttackFatigueBase, false);
    BSEndian(fPowerAttackFatigueMult, false);
}

// Swaps the floats and the flags read from a big-endian file in the game's order (fRushingAttackDistanceMult before
// iFlags); the chance bytes are left alone.
void CombatStyleData::Endian() {
    BSEndian(fDodgeLeftRightMinTime, false);
    BSEndian(fDodgeLeftRightMaxTime, false);
    BSEndian(fDodgeForwardMinTime, false);
    BSEndian(fDodgeForwardMaxTime, false);
    BSEndian(fDodgeBackwardMinTime, false);
    BSEndian(fDodgeBackwardMaxTime, false);
    BSEndian(fIdleMinTime, false);
    BSEndian(fIdleMaxTime, false);
    BSEndian(fAttackDuringRecoilStaggerBonus, false);
    BSEndian(fAttackDuringUnconsciousBonus, false);
    BSEndian(fAttackHandBonus, false);
    BSEndian(fPowerAttackRecoilStaggerBonus, false);
    BSEndian(fPowerAttackUnconsciousBonus, false);
    BSEndian(fHoldMinTime, false);
    BSEndian(fHoldMaxTime, false);
    BSEndian(fRushingAttackDistanceMult, false);
    BSEndian(iFlags, false);
}

TESCombatStyle::TESCombatStyle() {
    SetFormType(CSTY_ID);
    TESCombatStyle::InitializeData();
}

TESCombatStyle::~TESCombatStyle() {}

// Copies the default combat style's three data blocks into this form, or zeroes them when there is no default.
void TESCombatStyle::InitializeData() {
    TESCombatStyle* pDefault = GetDefaultCombatStyle();
    if (pDefault) {
        ourMemcpy(&Data, &pDefault->GetData(), sizeof(Data));
        ourMemcpy(&AdvancedData, &pDefault->GetAdvancedData(), sizeof(AdvancedData));
        ourMemcpy(&SimpleData, &pDefault->GetSimpleData(), sizeof(SimpleData));
    }
    else {
        ourMemset(&Data, 0, sizeof(Data));
        ourMemset(&AdvancedData, 0, sizeof(AdvancedData));
        ourMemset(&SimpleData, 0, sizeof(SimpleData));
    }
}

// An empty EDID chunk (malformed data) leaves the buffer unwritten, so one extra byte is reserved and the buffer is
// terminated.
bool TESCombatStyle::Load(TESFile* apFile) {
    if (apFile->GetTESForm() != 74) {
        return false;
    }
    ClearData();
    LoadForm(apFile);
    SetInitialized(false);
    do {
        uint32_t uiChunk = apFile->GetTESChunk();
        if (uiChunk == NO_CHUNK) {
            break;
        }
        switch (uiChunk) {
            case EDID_ID: {
                // A 16-byte aligned stack buffer of the chunk's size, read up to 0x200.
                uint32_t uiChunkSize = apFile->GetChunkSize();
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiChunkSize ? uiChunkSize : 1));
                // An empty chunk leaves the buffer unwritten.
                pBuffer[0] = '\0';
                apFile->GetChunkData(pBuffer, 0x200);
                SetFormEditorID(pBuffer);
                break;
            }
            case OBND_ID: LoadObjectBound(apFile); break;
            case CSTD_ID:
                apFile->GetChunkData(&Data, 0x5C);
                if (apFile->QEndian()) {
                    Data.Endian();
                }
                break;
            case CSAD_ID:
                apFile->GetChunkData(&AdvancedData, 0x54);
                if (apFile->QEndian()) {
                    AdvancedData.Endian();
                }
                break;
            case CSSD_ID: {
                apFile->GetChunkData(&SimpleData, 0x40);
                if (apFile->QEndian()) {
                    uint32_t uiChunkSize = apFile->GetChunkSize();
                    // A shorter CSSD leaves the bytes it did not read as they were: they are saved before the swap and
                    // put back after it.
                    SimpleCombatStyleData savedData;
                    if (uiChunkSize < 0x40) {
                        ourMemcpy(&savedData, &SimpleData, sizeof(savedData));
                    }
                    SimpleData.Endian();
                    if (uiChunkSize < 0x40) {
                        ourMemcpy(
                            reinterpret_cast<char*>(&SimpleData) + uiChunkSize,
                            reinterpret_cast<char*>(&savedData) + uiChunkSize,
                            0x40 - uiChunkSize
                        );
                    }
                }
                break;
            }
            default: break;
        }
    }
    while (apFile->NextChunk());
    if (apFile->GetFormVersion() < 0xC) {
        SimpleData.fWeaponMinRangeMult = 1.0f;
        SimpleData.fWeaponMaxRangeMult = 1.0f;
    }
    return true;
}

void TESCombatStyle::Save() {
    TESForm::StartForm();
    if (TESForm::QEndianSwapOnSave()) {
        Data.Endian();
    }
    TESForm::__AddChunkData(CSTD_ID, &Data, 0x5Cu);
    if (TESForm::QEndianSwapOnSave()) {
        Data.Endian();
    }
    if (TESForm::QEndianSwapOnSave()) {
        AdvancedData.Endian();
    }
    TESForm::__AddChunkData(CSAD_ID, &AdvancedData, 0x54u);
    if (TESForm::QEndianSwapOnSave()) {
        AdvancedData.Endian();
    }
    if (TESForm::QEndianSwapOnSave()) {
        SimpleData.Endian();
    }
    TESForm::__AddChunkData(CSSD_ID, &SimpleData, 0x40u);
    if (TESForm::QEndianSwapOnSave()) {
        SimpleData.Endian();
    }
    TESForm::CloseForm();
}

void TESCombatStyle::InitItem() {
    if (!TESForm::GetInitialized()) {
        TESForm::SetInitialized(true);
    }
}

void TESCombatStyle::Copy(TESForm* apCopy) {
    TESCombatStyle* pcopy = dynamic_cast<TESCombatStyle*>(apCopy);
    if (pcopy) {
        CopyAllComponents(pcopy);
        ourMemcpy(&Data, &pcopy->Data, sizeof(Data));
        ourMemcpy(&GetAdvancedData(), &pcopy->GetAdvancedData(), sizeof(AdvancedCombatStyleData));
        ourMemcpy(&SimpleData, &pcopy->SimpleData, sizeof(SimpleData));
    }
}

bool TESCombatStyle::Compare(TESForm* apCompare) {
    TESCombatStyle* pcomp = dynamic_cast<TESCombatStyle*>(apCompare);
    if (!pcomp) {
        return true;
    }
    if (CompareAllComponents(pcomp)) {
        return true;
    }
    if (std::memcmp(&Data, &pcomp->Data, sizeof(Data))) {
        return true;
    }
    if (!std::memcmp(&GetAdvancedData(), &pcomp->GetAdvancedData(), sizeof(AdvancedCombatStyleData))) {
        return std::memcmp(&SimpleData, &pcomp->SimpleData, sizeof(SimpleData)) != 0;
    }
    return true;
}

float TESCombatStyle::GetAttackSkillModBase() {
    return AdvancedData.fAttackSkillBase;
}

float TESCombatStyle::GetDodgeDuringAttackMult() {
    return AdvancedData.fDodgeDuringAttackMult;
}

char TESCombatStyle::GetDodgeChance() {
    return Data.cDodgeChance;
}

float TESCombatStyle::GetEncumSpeedModMult() {
    return AdvancedData.fDodgeSpeedMult;
}

float TESCombatStyle::GetWeaponMaxRangeMult() {
    return SimpleData.fWeaponMaxRangeMult;
}

bool TESCombatStyle::GetExplosiveWeaponsIgnoreSelf() {
    return IsFlagSet(TESCombatStyle::EXPLOSIVE_WEAPONS_IGNORE_SELF);
}

bool TESCombatStyle::GetExplosiveWeaponsIgnoreGroup() {
    return IsFlagSet(TESCombatStyle::EXPLOSIVE_WEAPONS_IGNORE_GROUP);
}

bool TESCombatStyle::GetExplosiveWeaponsIgnoreSpectators() {
    return IsFlagSet(TESCombatStyle::EXPLOSIVE_WEAPONS_IGNORE_SPECTATORS);
}

bool TESCombatStyle::GetIgnoreThreats() {
    return IsFlagSet(TESCombatStyle::IGNORE_THREATS);
}

char TESCombatStyle::GetRushingAttackPercentChance() {
    return static_cast<int8_t>(Data.cRushingAttackPercentChance);
}

float TESCombatStyle::GetSemiAutoFiringDelayMinMult() {
    return SimpleData.fSemiAutoFiringDelayMinMult;
}

int8_t TESCombatStyle::GetPowerAttackF() {
    return Data.cPowerAttackForwardChance;
}

int8_t TESCombatStyle::GetPowerAttackB() {
    return Data.cPowerAttackBackwardChance;
}

int8_t TESCombatStyle::QPowerAttackChance() {
    return Data.cPowerAttackChance;
}

char TESCombatStyle::GetLRChance() {
    return static_cast<int8_t>(Data.cDodgeLeftRightChance);
}

int8_t TESCombatStyle::GetPowerAttackL() {
    return Data.cPowerAttackLeftChance;
}

int8_t TESCombatStyle::GetPowerAttackN() {
    return Data.cPowerAttackNormalChance;
}

bool TESCombatStyle::IsFleeBasedOnPersonalSurvival() {
    return IsFlagSet(TESCombatStyle::PERSONAL_SURVIVAL_FLEE);
}

double TESCombatStyle::GetRandomWaitTime() {
    return RandomFloat(SimpleData.fCoverWaitTimerMin, SimpleData.fCoverWaitTimerMax);
}

double TESCombatStyle::GetRandomFireTimer() {
    return RandomFloat(SimpleData.fCoverFireTimerMin, SimpleData.fCoverFireTimerMax);
}

uint8_t TESCombatStyle::GetPowerAttackChance(ANIM_GROUP_ENUM aePowerAttack) {
    uint8_t result;
    switch (aePowerAttack) {
        case ANIM_GROUP_ATTACK_NORMAL_POWER: {
            result = GetPowerAttackN();
            break;
        }
        case ANIM_GROUP_ATTACK_FORWARD_POWER: {
            result = GetPowerAttackF();
            break;
        }
        case ANIM_GROUP_ATTACK_BACK_POWER: {
            result = GetPowerAttackB();
            break;
        }
        case ANIM_GROUP_ATTACK_LEFT_POWER: {
            result = GetPowerAttackL();
            break;
        }
        case ANIM_GROUP_ATTACK_RIGHT_POWER: {
            result = Data.cPowerAttackRightChance;
            break;
        }
        default: {
            result = 0;
            break;
        }
    }
    return result;
}

TESCombatStyle* TESCombatStyle::GetDefaultCombatStyle() {
    return static_cast<TESCombatStyle*>(TESForm::GetFormByNumericID(0x3Du));
}
