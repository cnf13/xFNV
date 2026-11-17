#include "Fallout Shared/TESCombatStyle.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"

// Swaps the floats read from a big-endian file in the exe's order (fWeaponMaxRangeMult before fClusterDistance); the
// weapon restrictions byte is left alone.
void SimpleCombatStyleData::Endian() {
	ENDIAN(fCoverSearchRadius, false);
	ENDIAN(fCoverChance, false);
	ENDIAN(fCoverWaitTimerMin, false);
	ENDIAN(fCoverWaitTimerMax, false);
	ENDIAN(fCoverWaitToFireTimerMin, false);
	ENDIAN(fCoverWaitToFireTimerMax, false);
	ENDIAN(fCoverFireTimerMin, false);
	ENDIAN(fCoverFireTimerMax, false);
	ENDIAN(fWeaponMinRangeMult, false);
	ENDIAN(fWeaponMaxRangeMult, false);
	ENDIAN(fClusterDistance, false);
	ENDIAN(fTargetingFOV, false);
	ENDIAN(fCombatRadius, false);
	ENDIAN(fSemiAutoFiringDelayMinMult, false);
	ENDIAN(fSemiAutoFiringDelayMaxMult, false);
}

// Swaps the 21 floats read from a big-endian file in the exe's order (fAttackDuringBlockMult before
// fAttackNoAttackMult).
void AdvancedCombatStyleData::Endian() {
	ENDIAN(fDodgeFatigueMult, false);
	ENDIAN(fDodgeFatigueBase, false);
	ENDIAN(fDodgeSpeedBase, false);
	ENDIAN(fDodgeSpeedMult, false);
	ENDIAN(fDodgeDuringAttackMult, false);
	ENDIAN(fDodgeNoAttackMult, false);
	ENDIAN(fDodgeBackDuringAttackMult, false);
	ENDIAN(fDodgeBackNoAttackMult, false);
	ENDIAN(fDodgeForwardWhileAttackingMult, false);
	ENDIAN(fDodgeForwardNotAttackingMult, false);
	ENDIAN(fBlockSkillMult, false);
	ENDIAN(fBlockSkillBase, false);
	ENDIAN(fBlockDuringAttackMult, false);
	ENDIAN(fBlockNoAttackMult, false);
	ENDIAN(fAttackSkillMult, false);
	ENDIAN(fAttackSkillBase, false);
	ENDIAN(fAttackDuringAttackMult, false);
	ENDIAN(fAttackDuringBlockMult, false);
	ENDIAN(fAttackNoAttackMult, false);
	ENDIAN(fPowerAttackFatigueBase, false);
	ENDIAN(fPowerAttackFatigueMult, false);
}

// Swaps the floats and the flags read from a big-endian file in the exe's order (fRushingAttackDistanceMult before
// iFlags); the chance bytes are left alone.
void CombatStyleData::Endian() {
	ENDIAN(fDodgeLeftRightMinTime, false);
	ENDIAN(fDodgeLeftRightMaxTime, false);
	ENDIAN(fDodgeForwardMinTime, false);
	ENDIAN(fDodgeForwardMaxTime, false);
	ENDIAN(fDodgeBackwardMinTime, false);
	ENDIAN(fDodgeBackwardMaxTime, false);
	ENDIAN(fIdleMinTime, false);
	ENDIAN(fIdleMaxTime, false);
	ENDIAN(fAttackDuringRecoilStaggerBonus, false);
	ENDIAN(fAttackDuringUnconsciousBonus, false);
	ENDIAN(fAttackHandBonus, false);
	ENDIAN(fPowerAttackRecoilStaggerBonus, false);
	ENDIAN(fPowerAttackUnconsciousBonus, false);
	ENDIAN(fHoldMinTime, false);
	ENDIAN(fHoldMaxTime, false);
	ENDIAN(fRushingAttackDistanceMult, false);
	ENDIAN(iFlags, false);
}

TESCombatStyle::TESCombatStyle() {
	SetFormType(CSTY_ID);
	TESCombatStyle::InitializeData();
}

TESCombatStyle::~TESCombatStyle() {}

// Copies the default combat style's three data blocks into this form, or zeroes them when there is no default. The
// exe copies 0x5C, 0x54 and 0x40 bytes from the getters, and zeroes the same blocks in the other branch.
void TESCombatStyle::InitializeData() {
	TESCombatStyle* pDefault = GetDefaultCombatStyle();
	if (pDefault) {
		ourMemcpy(&Data, &pDefault->GetData(), sizeof(Data));
		ourMemcpy(&AdvancedData, &pDefault->GetAdvancedData(), sizeof(AdvancedData));
		ourMemcpy(&SimpleData, &pDefault->GetSimpleData(), sizeof(SimpleData));
	} else {
		ourMemset(&Data, 0, sizeof(Data));
		ourMemset(&AdvancedData, 0, sizeof(AdvancedData));
		ourMemset(&SimpleData, 0, sizeof(SimpleData));
	}
}

// An empty EDID chunk leaves the alloca'd buffer unwritten (malformed data), so src reserves one byte and terminates
// it. The strict check reports a DIFF on that buffer only: the exe's sits at a different place in its frame, so the
// bytes GetChunkData (0x472890) is handed differ (fix queue).
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
			// The exe's _alloca (__alloca_probe_16, 0xEC5EC0): a 16-byte aligned buffer of the chunk's size, read up
			// to 0x200.
			uint32_t uiChunkSize = apFile->GetChunkSize();
			char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiChunkSize ? uiChunkSize : 1, 128));
			// Not in the exe: an empty chunk leaves the buffer unwritten.
			pBuffer[0] = '\0';
			apFile->GetChunkData(pBuffer, 0x200);
			SetFormEditorID(pBuffer);
			break;
		}
		case OBND_ID:
			LoadObjectBound(apFile);
			break;
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
					ourMemcpy(reinterpret_cast<char*>(&SimpleData) + uiChunkSize,
						reinterpret_cast<char*>(&savedData) + uiChunkSize, 0x40 - uiChunkSize);
				}
			}
			break;
		}
		default:
			break;
		}
	} while (apFile->NextChunk());
	if (apFile->GetFormVersion() < 0xC) {
		SimpleData.fWeaponMinRangeMult = 1.0f;
		SimpleData.fWeaponMaxRangeMult = 1.0f;
	}
	return true;
}

void TESCombatStyle::Save() {
	TESForm::StartForm();
	if (TESForm::QEndianSwapOnSave()) {
		(&Data)->Endian();
	}
	TESForm::__AddChunkData(static_cast<CHUNK_ID>(1146377027), &Data, 0x5Cu);
	if (TESForm::QEndianSwapOnSave()) {
		(&Data)->Endian();
	}
	if (TESForm::QEndianSwapOnSave()) {
		(&AdvancedData)->Endian();
	}
	TESForm::__AddChunkData(static_cast<CHUNK_ID>(1145131843), &AdvancedData, 0x54u);
	if (TESForm::QEndianSwapOnSave()) {
		(&AdvancedData)->Endian();
	}
	if (TESForm::QEndianSwapOnSave()) {
		(&SimpleData)->Endian();
	}
	TESForm::__AddChunkData(static_cast<CHUNK_ID>(1146311491), &SimpleData, 0x40u);
	if (TESForm::QEndianSwapOnSave()) {
		(&SimpleData)->Endian();
	}
	TESForm::CloseForm();
}

void TESCombatStyle::InitItem() {
	if (!TESForm::GetInitialized()) {
		TESForm::SetInitialized(1);
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
		return 1;
	}
	if (CompareAllComponents(pcomp)) {
		return 1;
	}
	if (std::memcmp(&Data, &pcomp->Data, sizeof(Data))) {
		return 1;
	}
	if (!std::memcmp(&GetAdvancedData(), &pcomp->GetAdvancedData(), sizeof(AdvancedCombatStyleData))) {
		return std::memcmp(&SimpleData, &pcomp->SimpleData, sizeof(SimpleData)) != 0;
	}
	return 1;
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
	return (bool)((uint8_t)IsFlagSet(TESCombatStyle::EXPLOSIVE_WEAPONS_IGNORE_SELF));
}

bool TESCombatStyle::GetExplosiveWeaponsIgnoreGroup() {
	return (bool)((uint8_t)IsFlagSet(TESCombatStyle::EXPLOSIVE_WEAPONS_IGNORE_GROUP));
}

bool TESCombatStyle::GetExplosiveWeaponsIgnoreSpectators() {
	return (bool)((uint8_t)IsFlagSet(TESCombatStyle::EXPLOSIVE_WEAPONS_IGNORE_SPECTATORS));
}

bool TESCombatStyle::GetIgnoreThreats() {
	return (bool)((uint8_t)IsFlagSet(TESCombatStyle::IGNORE_THREATS));
}

char TESCombatStyle::GetRushingAttackPercentChance() {
	return (int8_t)Data.cRushingAttackPercentChance;
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

// IDA name; the PDB's GetPowerAttackChance() returns char.
int8_t TESCombatStyle::QPowerAttackChance() {
	return Data.cPowerAttackChance;
}

char TESCombatStyle::GetLRChance() {
	return (int8_t)Data.cDodgeLeftRightChance;
}

int8_t TESCombatStyle::GetPowerAttackL() {
	return Data.cPowerAttackLeftChance;
}

int8_t TESCombatStyle::GetPowerAttackN() {
	return Data.cPowerAttackNormalChance;
}

bool TESCombatStyle::IsFleeBasedOnPersonalSurvival() {
	return (bool)((uint8_t)IsFlagSet(TESCombatStyle::PERSONAL_SURVIVAL_FLEE));
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
