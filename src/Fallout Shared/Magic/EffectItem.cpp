#include "Fallout Shared/Magic/EffectItem.h"

#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/AudioMarkerData.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/Magic/EffectSettingCollection.h"
#include "Fallout Shared/Magic/MagicFormulas.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/Settings/XGameSetting.h"
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

EffectItemData::EffectItemData()
  : iMagnitude(0), iArea(0), iDuration(0), iRange(MagicSystem::SELF), iActorValue(ActorValue::VALUE_NONE) {}

// Swaps the five dwords read from a big-endian file, in field order.
void EffectItemData::Endian() {
    BSEndian(iMagnitude, false);
    BSEndian(iArea, false);
    BSEndian(iDuration, false);
    BSEndian(iRange, false);
    BSEndian(iActorValue, false);
}

EffectItem::EffectItem(EffectSetting* apEffectSetting) : data(), Conditions() {
    Initialize(apEffectSetting);
}

EffectItem::EffectItem(EffectItem* apCopy) : data(), Conditions() {
    Initialize(apCopy->GetEffectSetting());
    pEffectSetting = nullptr;
    Copy(apCopy);
}

int32_t EffectItem::GetMagnitude() {
    if (pEffectSetting->IsFlagSet(EffectSetting::NO_MAGNITUDE)) {
        return 0;
    }

    return data.iMagnitude;
}

int32_t EffectItem::GetDuration() {
    if (pEffectSetting->IsFlagSet(EffectSetting::NO_DURATION)) {
        return 0;
    }

    return data.iDuration;
}

int32_t EffectItem::GetArea() {
    if (pEffectSetting->IsFlagSet(EffectSetting::NO_AREA) || !EffectItem::GetRange()) {
        return 0;
    }

    return data.iArea;
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
        return data.iActorValue;
    }

    return ActorValue::VALUE_NONE;
}

bool EffectItem::IsHostile() {
    return pEffectSetting->IsFlagSet(EffectSetting::HOSTILE);
}

float EffectItem::GetRawCost() {
    if (fRawCost < 0.0) {
        bool bTarget = GetRange() == MagicSystem::TARGET;
        int32_t iMagnitude = GetMagnitude();
        int32_t iDuration = GetDuration();
        int32_t iArea = GetArea();
        float fBaseCost = GetEffectSetting()->GetBaseCost();
        fRawCost = MagicFormulas::GetMagicEffectCost(fBaseCost, iArea, iDuration, iMagnitude, bTarget);
    }
    return fRawCost;
}

bool EffectItem::IsDisabled() {
    return pEffectSetting->IsDisabled();
}

ActorValue::Index EffectItem::GetMagicSchool() {
    return pEffectSetting->data.iAssociatedSkill;
}

// The luck is deliberately read before the school.
float EffectItem::GetCost(Actor* apActor) {
    float fCost = GetRawCost();
    if (apActor) {
        int32_t iLuck = static_cast<ActorValueOwner*>(apActor)->GetClampedActorValue(ActorValue::LUCK);
        int32_t iSkill = static_cast<ActorValueOwner*>(apActor)->GetClampedActorValue(GetMagicSchool());
        fCost = MagicFormulas::AdjustMagicSpellCost(fCost, iSkill, iLuck);
    }
    fCost = FloatFloor(fCost);
    return BSMAX(fCost, 1.0f);
}

bool EffectItem::IsScriptEffect() {
    return pEffectSetting->IsScriptEffect();
}

// The script of a script-archetype effect: its magic effect's associated item when that is a script, else null.
Script* EffectItem::GetScript() {
    Script* pScript = nullptr;
    EffectSetting* pSetting = GetEffectSetting();
    if (EffectArchetypes::CheckFlag(pSetting->GetEffectArchetype(), EffectArchetypes::USES_SCRIPT) &&
        pSetting->GetAssociatedItem() &&
        pSetting->GetAssociatedItem()->GetFormType() == SCPT_ID) {
        pScript = static_cast<Script*>(GetEffectSetting()->GetAssociatedItem());
    }

    return pScript;
}

BSStringT<char> EffectItem::GetEffectName() {
    return pEffectSetting->GetEffectName();
}

void EffectItem::Initialize(EffectSetting* apEffectSetting) {
    int32_t iActorValue;
    pEffectSetting = apEffectSetting;
    data.iArea = 0;
    data.iDuration = 0;
    data.iMagnitude = 0;
    if (pEffectSetting->IsAssociatedActorValueUsed()) {
        iActorValue = static_cast<int32_t>(pEffectSetting->GetAssociatedActorValue());
    }
    else {
        iActorValue = -1;
    }
    data.iActorValue = static_cast<ActorValue::Index>(iActorValue);
    fRawCost = -1.0f;
}

void EffectItem::InitItem(TESForm* apOwner) {
    if (!apOwner->GetInitialized()) {
        // Tests the address of Conditions, which is never null.
        Conditions.InitItem(apOwner);
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
    // Tests the address of Conditions, which is never null.
    Conditions.Save();
}

bool EffectItem::Load(TESFile* apFile, const char* apSpellName) {
    CHUNK_ID eChunk = apFile->GetTESChunk();
    if (eChunk == CIDA_ID) {
        Conditions.Load(apFile);
    }
    else {
        if (eChunk != EFIT_ID) {
            return false;
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
        }
        else {
            const char* pcSpellName = apSpellName ? apSpellName : "{unknown}";
            Warning(
                "GAME EFFECTS: Unknown EffectSetting encountered when loading EffectItem in actor effect '%s'",
                pcSpellName
            );
        }
    }
    return true;
}

void EffectItem::Copy(EffectItem* apCopy) {
    pEffectSetting = apCopy->pEffectSetting;
    SetRange(apCopy->data.iRange);
    SetMagnitude(apCopy->GetMagnitude());
    SetArea(apCopy->GetArea());
    SetDuration(apCopy->GetDuration());
    SetActorValueIndex(apCopy->GetActorValueIndex());
    SetConditions(&apCopy->Conditions);
    fRawCost = -1.0f;
}

bool EffectItem::MatchesEffect(EffectItem* apCompare) {
    bool bResult = true;
    if (apCompare->pEffectSetting == pEffectSetting) {
        int32_t iMagnitude = apCompare->GetMagnitude();
        if (iMagnitude == GetMagnitude()) {
            int32_t iArea = apCompare->GetArea();
            if (iArea == GetArea()) {
                int32_t iDuration = apCompare->GetDuration();
                if (iDuration == GetDuration()) {
                    int32_t iRange = apCompare->data.iRange;
                    if (iRange == data.iRange) {
                        ActorValue::Index iActorValue = apCompare->GetActorValueIndex();
                        if (iActorValue == GetActorValueIndex()) {
                            bResult = false;
                        }
                    }
                }
            }
        }
    }
    // Tests the address of Conditions, which is never null.
    if (Conditions.Compare(&apCompare->Conditions)) {
        return true;
    }
    return bResult;
}

void EffectItem::SetConditions(TESCondition* apCondition) {
    if (apCondition) {
        Conditions.Copy(apCondition, nullptr);
    }
    else {
        TESCondition kEmptyConditions;
        Conditions.Copy(&kEmptyConditions, nullptr);
    }
}

bool EffectItem::SharesAssociatedItem(EffectItem* apEffect1, EffectItem* apEffect2) {
    if (!apEffect1 || !apEffect2) {
        return false;
    }
    if (apEffect1->pEffectSetting->IsFlagSet(EffectSetting::USE_ASSOCIATED_ARMOR) &&
        apEffect2->pEffectSetting->IsFlagSet(EffectSetting::USE_ASSOCIATED_ARMOR)) {
        TESForm* pItem = apEffect1->pEffectSetting->GetAssociatedItem();
        TESForm* pOtherItem = apEffect2->pEffectSetting->GetAssociatedItem();
        TESBipedModelForm* pBiped = dynamic_cast<TESBipedModelForm*>(pItem);
        TESBipedModelForm* pOtherBiped = dynamic_cast<TESBipedModelForm*>(pOtherItem);
        if (pBiped && pOtherBiped) {
            return pBiped->FillsMatchingBipedSlots(pOtherBiped);
        }
    }
    return apEffect1->pEffectSetting->IsFlagSet(EffectSetting::USE_ASSOCIATED_WEAPON) &&
        apEffect2->pEffectSetting->IsFlagSet(EffectSetting::USE_ASSOCIATED_WEAPON);
}

void EffectItem::operator delete(void* apMemory) {
    ourFree(apMemory);
}

float EffectSetting::GetBaseCost() {
    return data.fBaseCost;
}

BSStringT<char> EffectSetting::GetEffectName() {
    return cFullName;
}

ActorValue::Index EffectSetting::GetAssociatedActorValue() {
    return data.eAssociatedActorValue;
}
