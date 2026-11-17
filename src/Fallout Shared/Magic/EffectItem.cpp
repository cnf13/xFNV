#include "Fallout Shared/Magic/EffectItem.h"

#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSCore/TLSData.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSMain/SettingCollectionMap.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/AudioMarkerData.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/Magic/EffectSettingCollection.h"
#include "Fallout Shared/Magic/MagicFormulas.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/Interface/Menus/CharGenMenu.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTStringMap.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"

// A unit an effect's duration is written in, with its length in seconds.
struct TimeUnits {
	const char* pcName;
	int32_t iMultiplier;
};

// The duration units, seconds to days; no PC code reads them.
using EffectItemUnitArray = TimeUnits[4];
static EffectItemUnitArray pEffectItemUnits{{"s", 1}, {"m", 60}, {"h", 3600}, {"d", 86400}};

// The game settings this file defines, with their IDs and default values, in initialization order.
static SettingT<GameSettingCollection> sMagicEffectItemFor("sMagicEffectItemFor", "for");
static SettingT<GameSettingCollection> sMagicEffectItemOn("sMagicEffectItemOn", "on");
static SettingT<GameSettingCollection> sMagicEffectItemIn("sMagicEffectItemIn", "in");
static SettingT<GameSettingCollection> sMagicEffectItemSecondsSingular("sMagicEffectItemSecondsSingular", "sec");
static SettingT<GameSettingCollection> sMagicEffectItemPercent("sMagicEffectItemPercent", "%");
static SettingT<GameSettingCollection> sMagicEffectItemPointsSingular("sMagicEffectItemPointsSingular", "pt");
static SettingT<GameSettingCollection> sMagicEffectItemSecondsPlural("sMagicEffectItemSecondsPlural", "secs");
SettingT<GameSettingCollection> sMagicEffectItemPointsPlural("sMagicEffectItemPointsPlural", "pts");
static SettingT<GameSettingCollection> sMagicEffectItemFeet("sMagicEffectItemFeet", "ft");
static SettingT<GameSettingCollection> sMagicEffectItemLock("sMagicEffectItemLock", "Lock");
static SettingT<GameSettingCollection> sMagicEffectItemCreateLock("sMagicEffectItemCreateLock", "Create");
static SettingT<GameSettingCollection> sMagicEffectItemUpToLevel("sMagicEffectItemUpToLevel", "up to level");
static SettingT<GameSettingCollection> sMagicEffectItemFortify("sMagicEffectItemFortify", "Fortify");
static SettingT<GameSettingCollection> sMagicEffectItemRestore("sMagicEffectItemRestore", "Restore");
static SettingT<GameSettingCollection> sMagicEffectItemDrain("sMagicEffectItemDrain", "Drain");
static SettingT<GameSettingCollection> sMagicEffectItemDamage("sMagicEffectItemDamage", "Damage");
static SettingT<GameSettingCollection> sMagicEffectItemAbsorb("sMagicEffectItemAbsorb", "Absorb");
static SettingT<GameSettingCollection> sMagicRangeStrike("sMagicRangeStrike", "Strike");

double fFloor(float a1) {
	return (double)((int32_t)a1);
}

float FloatFloor(float aIn) {
	float v5 = fFloor(aIn);
	if (!(aIn - v5 >= 0.0)) {
		return v5 - 1.0;
	} else {
		return v5;
	}
}

MEM_CONTEXT QMemContext() {
	return static_cast<MEM_CONTEXT>(GetThreadMemContext());
}

EffectItemData::EffectItemData()
	: iMagnitude(0), iArea(0), iDuration(0), iRange(MagicSystem::SELF), iActorValue(ActorValue::VALUE_NONE) {}

// Swaps the five dwords read from a big-endian file, in field order.
void EffectItemData::Endian() {
	ENDIAN(iMagnitude, false);
	ENDIAN(iArea, false);
	ENDIAN(iDuration, false);
	ENDIAN(iRange, false);
	ENDIAN(iActorValue, false);
}

EffectItem::EffectItem(EffectSetting* apEffectSetting) : data(), Conditions() {
	Initialize(apEffectSetting);
}

EffectItem::EffectItem(EffectItem* a1) : data(), Conditions() {
	Initialize(a1->GetEffectSetting());
	pEffectSetting = nullptr;
	Copy(a1);
}

int32_t EffectItem::GetMagnitude() {
	if (pEffectSetting->IsFlagSet(EffectSetting::NO_MAGNITUDE)) {
		return 0;
	} else {
		return data.iMagnitude;
	}
}

int32_t EffectItem::GetDuration() {
	if (pEffectSetting->IsFlagSet(EffectSetting::NO_DURATION)) {
		return (uint32_t)0;
	} else {
		return (uint32_t)data.iDuration;
	}
}

int32_t EffectItem::GetArea() {
	if (pEffectSetting->IsFlagSet(EffectSetting::NO_AREA) || (!EffectItem::GetRange())) {
		return (int32_t)0;
	} else {
		return (int32_t)data.iArea;
	}
}

MagicSystem::Range EffectItem::GetRange() {
	return data.iRange;
}

bool EffectItem::SetMagnitude(int32_t aiMagnitude) {
	if (!pEffectSetting->IsFlagSet(EffectSetting::NO_MAGNITUDE) && aiMagnitude >= 0) {
		data.iMagnitude = aiMagnitude;
		fRawCost = -1.0f;
		return true;
	}
	return false;
}

// An effect with no area or a self range keeps its area.
bool EffectItem::SetArea(int32_t aiArea) {
	if (!pEffectSetting->IsFlagSet(EffectSetting::NO_AREA) && GetRange() && aiArea >= 0) {
		data.iArea = aiArea;
		fRawCost = -1.0f;
		return true;
	}
	return false;
}

bool EffectItem::SetDuration(int32_t aiDuration) {
	if (!pEffectSetting->IsFlagSet(EffectSetting::NO_DURATION) && aiDuration >= 0) {
		data.iDuration = aiDuration;
		fRawCost = -1.0f;
		return true;
	}
	return false;
}

// A range is allowed when the magic effect has the matching range flag.
bool EffectItem::SetRange(MagicSystem::Range aiRange) {
	if ((aiRange == MagicSystem::TOUCH && pEffectSetting->IsFlagSet(EffectSetting::RANGE_TOUCH)) ||
		(aiRange == MagicSystem::TARGET && pEffectSetting->IsFlagSet(EffectSetting::RANGE_TARGET)) ||
		(aiRange == MagicSystem::SELF && pEffectSetting->IsFlagSet(EffectSetting::RANGE_SELF))) {
		data.iRange = aiRange;
		fRawCost = -1.0f;
		return true;
	}
	return false;
}

bool EffectItem::IsUsageMonitorEffect() {
	return GetEffectSetting() == EffectSettingCollection::GetUsageMonitorEffectSetting();
}

bool EffectItem::IsActorValueUsed() {
	return pEffectSetting->IsAssociatedActorValueUsed();
}

ActorValue::Index EffectItem::GetActorValueIndex() {
	if (IsActorValueUsed()) {
		return static_cast<ActorValue::Index>(data.iActorValue);
	} else {
		return static_cast<ActorValue::Index>(-1);
	}
}

bool EffectItem::IsHostile() {
	EffectSetting* time = pEffectSetting;
	return (bool)((uint8_t)time->IsFlagSet(EffectSetting::HOSTILE));
}

float EffectItem::GetRawCost() {
	if (fRawCost < 0.0) {
		bool v8 = GetRange() == MagicSystem::TARGET;
		int32_t Magnitude = GetMagnitude();
		int32_t Duration = GetDuration();
		int32_t Area = GetArea();
		EffectSetting* Time = GetEffectSetting();
		float afBaseEffectCost = Time->GetBaseCost();
		fRawCost = MagicFormulas::GetMagicEffectCost(afBaseEffectCost, Area, Duration, Magnitude, v8);
	}
	return fRawCost;
}

bool EffectItem::IsDisabled() {
	return (bool)((int32_t)pEffectSetting->IsDisabled());
}

ActorValue::Index EffectItem::GetMagicSchool() {
	return pEffectSetting->data.iAssociatedSkill;
}

// The luck is deliberately read before the school, as the game does.
float EffectItem::GetCost(Actor* apActor) {
	float fCost = static_cast<float>(GetRawCost());
	if (apActor) {
		int32_t iLuck = static_cast<ActorValueOwner*>(apActor)->GetClampedActorValue(ActorValue::LUCK);
		int32_t iSkill = static_cast<ActorValueOwner*>(apActor)->GetClampedActorValue(GetMagicSchool());
		fCost = static_cast<float>(MagicFormulas::AdjustMagicSpellCost(fCost, iSkill, iLuck));
	}
	fCost = FloatFloor(fCost);
	return BSMAX(fCost, 1.0f);
}

bool EffectItem::IsScriptEffect() {
	return (bool)((int32_t)pEffectSetting->IsScriptEffect());
}

// The script of a script-archetype effect: its magic effect's associated item when that is a script, else null.
Script* EffectItem::GetScript() {
	Script* pscript = nullptr;
	EffectSetting* psetting = GetEffectSetting();
	if (EffectArchetypes::CheckFlag(psetting->GetEffectArchetype(), 64)) {
		if (psetting->GetAssociatedItem()) {
			if (psetting->GetAssociatedItem()->GetFormType() == SCPT_ID) {
				pscript = static_cast<Script*>(GetEffectSetting()->GetAssociatedItem());
			}
		}
	}
	return pscript;
}

BSStringT<char>* EffectItem::GetEffectName(BSStringT<char>* apString) {
	pEffectSetting->GetEffectName(apString);
	return apString;
}

void EffectItem::Initialize(EffectSetting* apEffectSetting) {
	int32_t combatState;
	pEffectSetting = apEffectSetting;
	data.iArea = 0;
	data.iDuration = 0;
	data.iMagnitude = 0;
	if (pEffectSetting->IsAssociatedActorValueUsed()) {
		combatState = (int32_t)pEffectSetting->GetAssociatedActorValue();
	} else {
		combatState = (-1);
	}
	data.iActorValue = static_cast<ActorValue::Index>(combatState);
	fRawCost = (-0x1p+0f);
}

void EffectItem::InitItem(TESForm* a2) {
	if (!a2->GetInitialized()) {
		// Tests the address of Conditions, which is never null, as the game does.
		Conditions.InitItem(a2);
	}
}

// Saves the effect's numbers as an EFIT chunk, swapped to the file's byte order around the write, then its
// conditions.
void EffectItem::Save() {
	if (TESForm::QEndianSwapOnSave()) {
		data.Endian();
	}
	TESForm::__AddChunkData(EFIT_ID, &data, sizeof(EffectItemData));
	if (TESForm::QEndianSwapOnSave()) {
		data.Endian();
	}
	// Tests the address of Conditions, which is never null, as the game does.
	Conditions.Save();
}

bool EffectItem::Load(TESFile* apFile, const char* apSpellName) {
	CHUNK_ID TESChunk = apFile->GetTESChunk();
	if (TESChunk == CIDA_ID) {
		Conditions.Load(apFile);
	} else {
		if (TESChunk != EFIT_ID) {
			return 0;
		}
		ourMemset(this, 0, 20u);
		apFile->GetChunkData(this, 20u);
		if (apFile->QEndian()) {
			data.Endian();
		}
		if (pEffectSetting) {
			if (pEffectSetting->IsAssociatedActorValueUsed() && pEffectSetting->IsAssociatedActorValueFixed()) {
				data.iActorValue = pEffectSetting->data.eAssociatedActorValue;
			}
		} else {
			const char* v4;
			if (apSpellName) {
				v4 = apSpellName;
			} else {
				v4 = "{unknown}";
			}
			Warning("GAME EFFECTS: Unknown EffectSetting encountered when loading EffectItem in actor effect '%s'", v4);
		}
	}
	return 1;
}

void EffectItem::Copy(EffectItem* apCopy) {
	pEffectSetting = apCopy->pEffectSetting;
	MagicSystem::Range CastingType = apCopy->data.iRange;
	SetRange(CastingType);
	int32_t Magnitude = apCopy->GetMagnitude();
	SetMagnitude(Magnitude);
	int32_t Area = apCopy->GetArea();
	SetArea(Area);
	int32_t Duration = apCopy->GetDuration();
	SetDuration(Duration);
	SetActorValueIndex(apCopy->GetActorValueIndex());
	SetConditions(&apCopy->Conditions);
	fRawCost = -1.0f;
}

bool EffectItem::MatchesEffect(EffectItem* apCompare) {
	EffectSetting* EffectSetting_ = apCompare->pEffectSetting;
	bool bresult = true;
	if (EffectSetting_ == pEffectSetting) {
		int32_t Magnitude = apCompare->GetMagnitude();
		if (Magnitude == GetMagnitude()) {
			int32_t Area = apCompare->GetArea();
			if (Area == GetArea()) {
				int32_t Duration = apCompare->GetDuration();
				if (Duration == GetDuration()) {
					int32_t CastingType = apCompare->data.iRange;
					if (CastingType == data.iRange) {
						ActorValue::Index ActorValueIndex = apCompare->GetActorValueIndex();
						if (ActorValueIndex == GetActorValueIndex()) {
							bresult = false;
						}
					}
				}
			}
		}
	}
	// Tests the address of Conditions, which is never null, as the game does.
	if (Conditions.Compare(&apCompare->Conditions)) {
		return 1;
	}
	return bresult;
}

void EffectItem::SetConditions(TESCondition* apCondition) {
	if (apCondition) {
		Conditions.Copy(apCondition, 0);
	} else {
		TESCondition v3;
		Conditions.Copy(&v3, 0);
	}
}

bool EffectItem::SharesAssociatedItem(EffectItem* apEffect1, EffectItem* apEffect2) {
	if (!apEffect1 || !apEffect2) {
		return 0;
	}
	if (apEffect1->pEffectSetting->IsFlagSet(EffectSetting::USE_ASSOCIATED_ARMOR) &&
		apEffect2->pEffectSetting->IsFlagSet(EffectSetting::USE_ASSOCIATED_ARMOR)) {
		TESForm* AssociatedItem = apEffect1->pEffectSetting->GetAssociatedItem();
		TESForm* v5 = apEffect2->pEffectSetting->GetAssociatedItem();
		TESBipedModelForm* v6 = dynamic_cast<TESBipedModelForm*>(AssociatedItem);
		TESBipedModelForm* apOtherBiped = dynamic_cast<TESBipedModelForm*>(v5);
		if (v6) {
			if (apOtherBiped) {
				return v6->FillsMatchingBipedSlots(apOtherBiped);
			}
		}
	}
	return apEffect1->pEffectSetting->IsFlagSet(EffectSetting::USE_ASSOCIATED_WEAPON) &&
		apEffect2->pEffectSetting->IsFlagSet(EffectSetting::USE_ASSOCIATED_WEAPON);
}

void EffectItem::operator delete(void* apMemory) {
	ourFree(apMemory);
}

char* ourStrcpy(char* apStrDest, const char* apStrSource) {
	return std::strcpy(apStrDest, apStrSource);
}

void* ourMemset(void* apDest, int32_t aiValue, uint32_t aCount) {
	return std::memset(apDest, aiValue, aCount);
}

void SetMemContext(MEM_CONTEXT aeContext) {
	GetThreadTLSData()->eMemContext = aeContext;
}

SettingCollection::SettingCollection() : pHandle(nullptr) {
	pSettingFile[0] = 0;
}

bool SettingCollection::WriteSettings() {
	return pHandle != nullptr;
}

bool SettingCollection::ReadSettings() {
	return pHandle != 0;
}

// The settings' viewer strings, one per setting in the collection's map; returns how many were added.
uint32_t SettingCollectionMap::GetViewerStrings(NiTPrimitiveArray<char*>* apStringsA) {
	AutoMemContext kContext(MEM_CONTEXT_GAMESETTING, 1, "..\\BSMain\\Setting.h", 569);
	uint32_t uiCount = 0;
	const char* pKey;
	Setting* pSetting;
	NiTMapItem<const char*, Setting*>* pkPos = Settings.GetFirstPos();
	while (pkPos) {
		Settings.GetNext(pkPos, pKey, pSetting);
		if (pSetting) {
			pSetting->GetViewerStrings(apStringsA);
			++uiCount;
		}
	}
	return uiCount;
}

void SettingCollectionMap::Add(Setting* apSetting) {
	AutoMemContext kContext(MEM_CONTEXT_GAMESETTING, 1, "..\\BSMain\\Setting.h", 539);
	if (apSetting) {
		if (apSetting->pKey) {
			Setting* pExisting;
			if (Settings.GetAt(apSetting->pKey, pExisting)) {
				SettingWarning("DEFAULT: Setting key '%s' already used in map.\nSetting keys must be unique.\n",
					apSetting->pKey);
			} else {
				Settings.SetAt(apSetting->pKey, apSetting);
			}
		}
	}
}

void SettingCollectionMap::Remove(Setting* apSetting) {
	if (apSetting) {
		if (apSetting->pKey) {
			Settings.RemoveAt(apSetting->pKey);
		}
	}
}

// Writes every setting of the map through the virtual WriteSetting; true when the store is open and every write
// succeeded.
bool SettingCollectionMap::WriteSettings() {
	bool bResult = SettingCollection::WriteSettings();
	if (bResult) {
		const char* pKey;
		Setting* pSetting;
		NiTMapItem<const char*, Setting*>* pkPos = Settings.GetFirstPos();
		while (pkPos) {
			Settings.GetNext(pkPos, pKey, pSetting);
			if (pSetting) {
				bResult = bResult & WriteSetting(*pSetting);
			}
		}
	}
	return bResult;
}

// Reads every setting of the map through the virtual ReadSetting, under the game setting memory context; true when the
// store is open and every read succeeded.
bool SettingCollectionMap::ReadSettings() {
	AutoMemContext kContext(MEM_CONTEXT_GAMESETTING, 1, "..\\BSMain\\Setting.h", 632);
	bool bResult = SettingCollection::ReadSettings();
	if (bResult) {
		const char* pKey;
		Setting* pSetting;
		NiTMapItem<const char*, Setting*>* pkPos = Settings.GetFirstPos();
		while (pkPos) {
			Settings.GetNext(pkPos, pKey, pSetting);
			if (pSetting) {
				bResult = bResult & ReadSetting(*pSetting);
			}
		}
	}
	return bResult;
}

float EffectSetting::GetBaseCost() {
	return data.fBaseCost;
}

BSStringT<char>* EffectSetting::GetEffectName(BSStringT<char>* apResult) {
	::new (apResult) BSStringT<char>(*&cFullName);
	return apResult;
}

ActorValue::Index EffectSetting::GetAssociatedActorValue() {
	return data.eAssociatedActorValue;
}

char* NiStrcpy(char* pcDest, size_t stDestSize, const char* pcSrc) {
	strcpy_s(pcDest, stDestSize, pcSrc);
	return pcDest;
}
