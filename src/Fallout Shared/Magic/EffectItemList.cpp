#include "Fallout Shared/Magic/EffectItemList.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/BGSEntryPoint.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/Magic/EffectSettingCollection.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TESHealthForm.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Menus/StatsMenu.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "SDK/MSVC/stdio.h"

#include <cstdarg>
#include <locale>

static SettingT<GameSettingCollection> sMagicCostliestEffectRequires("sMagicCostliestEffectRequires", "Requires ");
static SettingT<GameSettingCollection> sMagicCostliestEffectSkillOf("sMagicCostliestEffectSkillOf", " Skill of ");
static SettingT<GameSettingCollection> sMagicSchool("sMagicSchool", "Magic School");

int32_t BSvsprintf(char* apString1, const char* apFormat, va_list aList) {
    return std::vsprintf(apString1, apFormat, aList);
}

EffectItemList::EffectItemList() : BSSimpleList<EffectItem*>(), iHostileCount(0) {}

// The summed cost of the enabled effects, floored; 0 for an empty list or a negative sum.
float EffectItemList::GetCost(Actor* apActor) {
    EffectItemList* plist = this;
    if (IsEmpty()) {
        return 0.0f;
    }

    float fcost = 0.0f;
    while (plist) {
        EffectItem* pitem = plist->m_item;
        if (!pitem->IsDisabled()) {
            fcost = pitem->GetCost(apActor) + fcost;
        }
        BSSimpleList<EffectItem*>* pNext = plist->m_pkNext;
        plist = static_cast<EffectItemList*>(pNext);
    }
    if (!(fcost < 0.0)) {
        return FloatFloor(fcost);
    }

    return 0.0f;
}

void EffectItemList::GetEffectsString(char* apBuffer, uint32_t auiSize) {
    bool bDispositionFailure = false;
    StatsMenu::StatusDataList kStatusList;
    bool bMedicine = IsMedicine();
    bool bFood = IsFood();
    char bAppended = 0;
    char cEffect[128];
    *apBuffer = 0;
    cEffect[0] = 0;
    BSSimpleList<EffectItem*>* pEffectNode = static_cast<BSSimpleList<EffectItem*>*>(this);
    while (pEffectNode && pEffectNode->m_item) {
        if (!pEffectNode->m_item->IsUsageMonitorEffect() &&
            pEffectNode->m_item->Conditions.IsTrueFull(pPlayer, nullptr, &bDispositionFailure, false)) {
            if (pEffectNode->m_item->pEffectSetting->IsFlagSet(EffectSetting::DISPLAY_EFFECT_NAME)) {
                BSstrcpy(cEffect, 128, pEffectNode->m_item->GetEffectName().pString);
                if (bAppended) {
                    BSstrcat(apBuffer, static_cast<int32_t>(auiSize), "\n");
                    BSstrcat(apBuffer, static_cast<int32_t>(auiSize), cEffect);
                }
                else {
                    BSstrcat(apBuffer, static_cast<int32_t>(auiSize), cEffect);
                    bAppended = 1;
                }
            }
            else {
                kStatusList.Register(pEffectNode->m_item);
            }
        }

        pEffectNode = pEffectNode->m_pkNext;
    }
    BSSimpleList<EffectItemData*>* pDataNode = static_cast<BSSimpleList<EffectItemData*>*>(&kStatusList);
    while (pDataNode && pDataNode->m_item) {
        if (pDataNode->m_item->iMagnitude && pDataNode->m_item->iActorValue != ActorValue::VALUE_NONE) {
            ActorValue::Index eActorValue = pDataNode->m_item->iActorValue;
            int32_t iMagnitude = pDataNode->m_item->iMagnitude;
            float fDuration = static_cast<float>(pDataNode->m_item->iDuration);
            char cUnit = 's';
            if (!(fDuration >= 3600.0)) {
                if (fDuration >= 60.0) {
                    fDuration = fDuration / 60.0;
                    cUnit = 'm';
                }
            }
            else {
                fDuration = fDuration / 3600.0;
                cUnit = 'h';
            }
            if (ActorValue::IsFlagSet(eActorValue, ActorValue::DAMAGE_IS_POSITIVE)) {
                iMagnitude = -iMagnitude;
            }
            if (bMedicine) {
                double dMagnitude = static_cast<double>(iMagnitude);
                float fMedicine = pPlayer->GetMedicineEffectivenessMultiplier() * dMagnitude;
                if (eActorValue == ActorValue::HEALTH) {
                    BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::MODIFY_RECOVERED_HEALTH, pPlayer, &fMedicine);
                }
                iMagnitude = static_cast<int32_t>(fMedicine);
            }
            if (bFood && eActorValue != ActorValue::RADIATION_RADS) {
                double dMagnitude = static_cast<double>(iMagnitude);
                float fSurvival = pPlayer->GetSurvivalEffectivenessMultiplier() * dMagnitude;
                if (eActorValue == ActorValue::HEALTH) {
                    BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::MODIFY_RECOVERED_HEALTH, pPlayer, &fSurvival);
                }
                iMagnitude = static_cast<int32_t>(fSurvival);
            }
            if (eActorValue == ActorValue::RADIATION_RADS && iMagnitude > 0) {
                double dMagnitude = static_cast<double>(iMagnitude);
                float fRads = pPlayer->GetRadiationResistanceMult() * dMagnitude;
                iMagnitude = static_cast<int32_t>(std::floor(fRads));
            }
            if (bFood && eActorValue == ActorValue::RADIATION_RADS) {
                float fBonus = 1.0f;
                BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::RADIATION_INGESTIBLE_BONUS, pPlayer, &fBonus);
                iMagnitude = static_cast<int32_t>(static_cast<double>(iMagnitude) * fBonus);
            }
            if (!iMagnitude) {
                iMagnitude = 1;
            }
            if (*apBuffer) {
                BSstrcat(apBuffer, 128, ", ");
            }
            if (pDataNode->m_item->iDuration) {
                float fFloor = std::floor(fDuration);
                // Only an ordered equal prints the whole number.
                if (fDuration == fFloor) {
                    int32_t iUnit = cUnit;
                    int32_t iDuration = FloatToInt(fDuration);
                    BSsprintf(
                        cEffect,
                        128,
                        "%s %+i(%d%c)",
                        ActorValue::GetActorValueAbbreviation(eActorValue),
                        iMagnitude,
                        iDuration,
                        iUnit
                    );
                }
                else {
                    BSsprintf(
                        cEffect,
                        128,
                        "%s %+i(%.1f%c)",
                        ActorValue::GetActorValueAbbreviation(eActorValue),
                        iMagnitude,
                        static_cast<double>(fDuration),
                        static_cast<int32_t>(cUnit)
                    );
                }
            }
            else {
                BSsprintf(cEffect, 128, "%s %+i", ActorValue::GetActorValueAbbreviation(eActorValue), iMagnitude);
            }
            BSstrcat(apBuffer, static_cast<int32_t>(auiSize), cEffect);
        }

        pDataNode = pDataNode->m_pkNext;
    }
}

// Whether an enabled effect's magic effect has the flag aeFlag.
bool EffectItemList::HasEffect(EffectSetting::EffectFlag aeFlag) {
    EffectItemList* pIter = this;
    if (IsEmpty()) {
        return false;
    }
    bool bFound = false;
    while (pIter && !bFound) {
        EffectItem* pItem = pIter->m_item;
        if (pItem && pItem->pEffectSetting->IsFlagSet(aeFlag)) {
            bFound = !pItem->IsDisabled();
        }

        BSSimpleList<EffectItem*>* pNext = pIter->m_pkNext;
        pIter = pNext ? static_cast<EffectItemList*>(pNext) : nullptr;
    }
    return bFound;
}

// Whether an enabled effect has the self range.
bool EffectItemList::HasSelfEffect() {
    EffectItemList* pIter = this;
    if (IsEmpty()) {
        return false;
    }
    bool bFound = false;
    while (pIter && !bFound) {
        EffectItem* pItem = pIter->m_item;
        if (pItem && !pItem->data.iRange) {
            bFound = !pItem->IsDisabled();
        }

        BSSimpleList<EffectItem*>* pNext = pIter->m_pkNext;
        pIter = pNext ? static_cast<EffectItemList*>(pNext) : nullptr;
    }
    return bFound;
}

bool EffectItemList::HasTouchEffect() {
    EffectItemList* pIter = this;
    if (static_cast<BSSimpleList<EffectItem*>*>(this)->IsEmpty()) {
        return false;
    }
    uint8_t bEnabled = 0;
    while (pIter && !bEnabled) {
        EffectItem* pItem = static_cast<BSSimpleList<EffectItem*>*>(pIter)->m_item;
        if (pItem && pItem->EffectItem::GetRange() == MagicSystem::TOUCH) {
            bEnabled = !pItem->IsDisabled();
        }

        BSSimpleList<EffectItem*>* pNext = static_cast<BSSimpleList<EffectItem*>*>(pIter)->m_pkNext;
        pIter = static_cast<EffectItemList*>(pNext);
    }
    return bEnabled;
}

// Saves each effect: its magic effect's form ID as an EFID chunk, then the effect itself.
void EffectItemList::Save() {
    EffectItemList* pIter = this;
    while (pIter) {
        EffectItem* pItem = pIter->m_item;
        if (pItem) {
            TESForm::AddChunk(EFID_ID, pItem->pEffectSetting->iFormID);
            pItem->Save();
        }
        BSSimpleList<EffectItem*>* pNext = pIter->m_pkNext;
        pIter = pNext ? static_cast<EffectItemList*>(pNext) : nullptr;
    }
}

// The effect ID is set to 0 before the EFID chunk is read: GetChunkData writes at most the chunk's bytes, so a
// zero-size chunk would otherwise leave it unwritten.
void EffectItemList::Load(TESFile* apFile, const char* apSpellName) {
    CHUNK_ID eChunk = apFile->GetTESChunk();
    if (eChunk == CIDA_ID) {
        if (pLastEffect) {
            pLastEffect->Load(apFile, apSpellName);
            if (pLastEffect->IsHostile()) {
                ++iHostileCount;
            }
        }
        else {
            const char* pName = apSpellName ? apSpellName : "{unknown}";
            Warning("MASTERFILE: Trying to load condition data with no effect item in Actor Effect '%s'.", pName);
        }
    }
    else if (eChunk == EFID_ID) {
        uint32_t iEffectID = 0;
        apFile->GetChunkData<uint32_t>(iEffectID);
        TESForm::AddCompileIndex(iEffectID, apFile);
        TESForm* pForm = TESForm::GetFormByNumericID(iEffectID);
        EffectSetting* pEffectSetting = dynamic_cast<EffectSetting*>(pForm);
        if (pEffectSetting) {
            EffectItem* pEffect;
            pEffect = new EffectItem(pEffectSetting);
            apFile->NextChunk();
            if (pEffect->Load(apFile, apSpellName)) {
                AddEffect(pEffect);
            }
            else {
                const char* pName = apSpellName ? apSpellName : "{unknown}";
                Warning(
                    "MASTERFILE: Unable to load EffectItem '%s' in Actor Effect '%s'",
                    pEffectSetting->GetEffectName().String(),
                    pName
                );
                if (pEffect) {
                    delete pEffect;
                }
                pEffect = nullptr;
            }
            pLastEffect = pEffect;
        }
        else {
            const char* pName = apSpellName ? apSpellName : "{unknown}";
            Warning("MASTERFILE: Unable to find EffectSetting %d in Actor Effect '%s'.", iEffectID, pName);
        }
    }
}

bool EffectItemList::IsHostile() {
    return static_cast<int32_t>(iHostileCount) > 0;
}

bool EffectItemList::CanBePoison() {
    EffectItemList* pIter = this;
    if (static_cast<BSSimpleList<EffectItem*>*>(this)->IsEmpty()) {
        return false;
    }
    while (pIter) {
        EffectItem* pItem = static_cast<BSSimpleList<EffectItem*>*>(pIter)->m_item;
        if (pItem && !pItem->IsDisabled() && !pItem->IsHostile()) {
            return false;
        }
        BSSimpleList<EffectItem*>* pNext = static_cast<BSSimpleList<EffectItem*>*>(pIter)->m_pkNext;
        pIter = static_cast<EffectItemList*>(pNext);
    }
    return true;
}

void EffectItemList::RemoveAllEffects() {
    EffectItemList* pThis = this;
    while (pThis) {
        EffectItem* pitem = pThis->m_item;
        if (pitem) {
            delete pitem;
            pThis->RemoveHead();
        }
        else {
            BSSimpleList<EffectItem*>* pNext = pThis->m_pkNext;
            pThis = static_cast<EffectItemList*>(pNext);
        }
    }
}

// The first effect whose magic effect is the usage monitor effect, or null.
EffectItem* EffectItemList::GetUsageMonitorEffect() {
    EffectItemList* pIter = this;
    EffectItem* pResult = nullptr;
    while (pIter && !pIter->IsEmpty() && !pResult) {
        EffectItem* pItem = pIter->m_item;
        if (pItem->pEffectSetting == EffectSettingCollection::GetUsageMonitorEffectSetting()) {
            pResult = pItem;
        }
        else {
            BSSimpleList<EffectItem*>* pNext = pIter->m_pkNext;
            pIter = pNext ? static_cast<EffectItemList*>(pNext) : nullptr;
        }
    }
    return pResult;
}

EffectItem* EffectItemList::GetCostliestEffect(MagicSystem::Range aeRange, bool abAreaOnly) {
    EffectItem* pResult = nullptr;
    EffectItemList* pIter = this;
    if (static_cast<BSSimpleList<EffectItem*>*>(this)->IsEmpty()) {
        return nullptr;
    }
    int32_t iMaxCost = -1;
    while (pIter) {
        EffectItem* pEffectItem = static_cast<BSSimpleList<EffectItem*>*>(pIter)->m_item;
        int32_t iCost = static_cast<int32_t>(pEffectItem->GetCost(nullptr));
        if (!pEffectItem->IsDisabled() &&
            iCost > iMaxCost &&
            (!abAreaOnly || pEffectItem->GetArea() > 0) &&
            (aeRange == MagicSystem::RANGE_COUNT || pEffectItem->EffectItem::GetRange() == aeRange)) {
            pResult = pEffectItem;
            iMaxCost = iCost;
        }
        BSSimpleList<EffectItem*>* pNext = static_cast<BSSimpleList<EffectItem*>*>(pIter)->m_pkNext;
        pIter = static_cast<EffectItemList*>(pNext);
    }
    return pResult;
}

bool EffectItemList::HasTargetEffect() {
    EffectItemList* pIter = this;
    if (static_cast<BSSimpleList<EffectItem*>*>(this)->IsEmpty()) {
        return false;
    }
    uint8_t bEnabled = 0;
    while (pIter && !bEnabled) {
        EffectItem* pItem = static_cast<BSSimpleList<EffectItem*>*>(pIter)->m_item;
        if (pItem && pItem->EffectItem::GetRange() == MagicSystem::TARGET) {
            bEnabled = !pItem->IsDisabled();
        }

        BSSimpleList<EffectItem*>* pNext = static_cast<BSSimpleList<EffectItem*>*>(pIter)->m_pkNext;
        pIter = static_cast<EffectItemList*>(pNext);
    }
    return bEnabled;
}

ActorValue::Index EffectItemList::GetAssociatedSkill() {
    EffectItem* pCostliestEffect = GetCostliestEffect(MagicSystem::RANGE_COUNT, false);
    if (pCostliestEffect) {
        return pCostliestEffect->GetMagicSchool();
    }
    return static_cast<ActorValue::Index>(-1);
}

// The summed score of the effects of archetype aeArchetypeID, range aeRange (RANGE_COUNT: any) and actor
// value aiActorValue (ACTOR_VALUE_COUNT: any): duration (at least 1) times magnitude, negative for detrimental ones.
float EffectItemList::GetEffectScore(
    EffectArchetypes::ArchetypeID aeArchetypeID,
    MagicSystem::Range aeRange,
    ActorValue::Index aiActorValue
) {
    EffectItemList* pIter = this;
    float fScore = 0.0f;
    while (pIter) {
        EffectItem* pItem = pIter->m_item;
        if (pItem) {
            EffectSetting* pSetting = pItem->pEffectSetting;
            if (pSetting->GetEffectArchetype() == aeArchetypeID &&
                (aiActorValue == ActorValue::ACTOR_VALUE_COUNT ||
                 (!pSetting->IsAssociatedActorValueUsed() || pSetting->GetAssociatedActorValue() == aiActorValue)) &&
                (aeRange == MagicSystem::RANGE_COUNT || pItem->data.iRange == aeRange)) {
                int32_t iDuration;
                if (pItem->GetDuration() <= 0) {
                    iDuration = 1;
                }
                else {
                    iDuration = pItem->GetDuration();
                }
                float fItemScore = static_cast<float>(iDuration * pItem->GetMagnitude());
                if (pSetting->IsFlagSet(EffectSetting::DETRIMENTAL)) {
                    fScore = fScore - fItemScore;
                }
                else {
                    fScore = fScore + fItemScore;
                }
            }
        }
        BSSimpleList<EffectItem*>* pNext = pIter->m_pkNext;
        pIter = pNext ? static_cast<EffectItemList*>(pNext) : nullptr;
    }
    return fScore;
}

void EffectItemList::AddEffect(EffectItem* apEffect) {
    if (!apEffect->IsUsageMonitorEffect() || !GetUsageMonitorEffect()) {
        static_cast<BSSimpleList<EffectItem*>*>(this)->AddTail(apEffect);
        if (apEffect->IsHostile()) {
            ++iHostileCount;
        }
    }
}

void EffectItemList::Copy(EffectItemList* apCopy) {
    RemoveAllEffects();
    while (apCopy) {
        EffectItem* pItem = static_cast<BSSimpleList<EffectItem*>*>(apCopy)->m_item;
        if (pItem) {
            EffectItem* pNew;
            pNew = new EffectItem(pItem);
            AddEffect(pNew);
        }
        BSSimpleList<EffectItem*>* pNext = static_cast<BSSimpleList<EffectItem*>*>(apCopy)->m_pkNext;
        apCopy = static_cast<EffectItemList*>(pNext);
    }
}

EffectItem* EffectItemList::GetEffectItemByIndex(uint8_t aiIndex) {
    BSSimpleList<EffectItem*>* pList;
    uint8_t cIndex = 0;
    if (this) {
        pList = static_cast<BSSimpleList<EffectItem*>*>(this);
    }
    else {
        pList = nullptr;
    }
    for (BSSimpleList<EffectItem*>* pNode = pList; pNode; pNode = pNode->m_pkNext) {
        if (cIndex == aiIndex) {
            return pNode->m_item;
        }
        ++cIndex;
    }
    Warning(
        "GAME EFFECTS: When trying to get an effect item by its index, the index was larger than the size of the "
        "list."
    );
    return nullptr;
}

bool EffectItemList::IsDisabled() {
    uint8_t bIsDisabled = 0;
    EffectItemList* pIter = this;
    if (static_cast<BSSimpleList<EffectItem*>*>(this)->IsEmpty()) {
        return false;
    }
    while (pIter && !bIsDisabled) {
        EffectItem* pItem = static_cast<BSSimpleList<EffectItem*>*>(pIter)->m_item;
        if (pItem) {
            bIsDisabled = pItem->IsDisabled();
        }
        BSSimpleList<EffectItem*>* pNext = static_cast<BSSimpleList<EffectItem*>*>(pIter)->m_pkNext;
        pIter = static_cast<EffectItemList*>(pNext);
    }
    return bIsDisabled;
}

// True when the lists differ: another count, or a pair of effects at the same place that do not match.
bool EffectItemList::Compare(EffectItemList* apCompare) {
    int32_t iCount = apCompare->ItemsInList();
    if (iCount != ItemsInList()) {
        return true;
    }
    if (!IsEmpty()) {
        EffectItemList* pThisList = this;
        EffectItemList* pOtherList = apCompare;
        while (pThisList) {
            EffectItem* pThisItem = pThisList->m_item;
            EffectItem* pOtherItem = pOtherList->m_item;
            if (pThisItem && pOtherItem && pThisItem->MatchesEffect(pOtherItem)) {
                return true;
            }
            BSSimpleList<EffectItem*>* pNext = pThisList->m_pkNext;
            pThisList = pNext ? static_cast<EffectItemList*>(pNext) : nullptr;
            BSSimpleList<EffectItem*>* pOtherNext = pOtherList->m_pkNext;
            pOtherList = pOtherNext ? static_cast<EffectItemList*>(pOtherNext) : nullptr;
        }
    }
    return false;
}

uint8_t EffectItemList::GetEffectItemIndex(EffectItem* apEffect) {
    BSSimpleList<EffectItem*>* pList;
    uint8_t cIndex = 0;
    if (this) {
        pList = static_cast<BSSimpleList<EffectItem*>*>(this);
    }
    else {
        pList = nullptr;
    }
    for (EffectItem*& rpItem : *pList) {
        if (apEffect == rpItem) {
            return cIndex;
        }
        ++cIndex;
    }
    Warning(
        "GAME EFFECTS: When trying to get an effect item index, the effect item was not found in the effect item "
        "list."
    );
    return 0;
}

// Whether an enabled effect runs apScript.
bool EffectItemList::HasScript(Script* apScript) {
    EffectItemList* pIter = this;
    if (IsEmpty()) {
        return false;
    }
    bool bFound = false;
    while (pIter && !bFound) {
        EffectItem* pItem = pIter->m_item;
        if (pItem && pItem->GetScript() == apScript) {
            bFound = !pItem->IsDisabled();
        }

        BSSimpleList<EffectItem*>* pNext = pIter->m_pkNext;
        pIter = pNext ? static_cast<EffectItemList*>(pNext) : nullptr;
    }
    return bFound;
}

// The format's arguments are passed on as a va_list; the formatter's count is returned unchanged.
int32_t BSsprintf(char* apString1, int32_t aiSize, const char* apFormat, ...) {
    va_list vList;
    va_start(vList, apFormat);
    int32_t iResult = vsprintf_s(apString1, aiSize, apFormat, vList);
    va_end(vList);
    return iResult;
}

// strcpy_s's result (0 or an errno) is returned unchanged.
int32_t BSstrcpy(char* apString1, int32_t aiSize, const char* apString2) {
    return strcpy_s(apString1, aiSize, apString2);
}

int32_t BSstrcat(char* apString1, int32_t aiSize, const char* apString2) {
    return strcat_s(apString1, aiSize, apString2);
}

// Rounds in the current floating-point rounding mode, not by truncation.
int32_t FloatToInt(float someFloat) {
    return NiFastFloatToInt(someFloat);
}

bool EffectSetting::IsDisabled() {
    return IsFlagSet(EffectSetting::DISABLED);
}

StatsMenu::StatusDataList::StatusDataList() : BSSimpleList<EffectItemData*>() {}
