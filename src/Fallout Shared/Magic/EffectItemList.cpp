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
        EffectItemList* p_m_pkNext;
        EffectItem* pitem = plist->m_item;
        if (!pitem->IsDisabled()) {
            fcost = pitem->GetCost(apActor) + fcost;
        }
        BSSimpleList<EffectItem*>* pNext = plist->m_pkNext;
        if (pNext) {
            p_m_pkNext = static_cast<EffectItemList*>(pNext);
        }
        else {
            p_m_pkNext = nullptr;
        }
        plist = p_m_pkNext;
    }
    if (!(fcost < 0.0)) {
        return FloatFloor(fcost);
    }

    return 0.0f;
}

void EffectItemList::GetEffectsString(char* apBuffer, uint32_t auiSize) {
    FNVD_INLINED_LOCAL(BSStringT<char>, apString, std::nullopt, std::nullopt);
    bool bDispositionFailure = false;
    StatsMenu::StatusDataList kStatusList;
    bool bMedicine = IsMedicine();
    bool bFood = IsFood();
    char bAppended = 0;
    char Dst[128];
    *apBuffer = 0;
    Dst[0] = 0;
    BSSimpleList<EffectItem*>* pEffectNode = static_cast<BSSimpleList<EffectItem*>*>(this);
    while (pEffectNode && pEffectNode->m_item) {
        if ((!pEffectNode->m_item->IsUsageMonitorEffect()) &&
            (pEffectNode->m_item->Conditions.IsTrueFull(pPlayer, nullptr, &bDispositionFailure, false))) {
            if (pEffectNode->m_item->pEffectSetting->IsFlagSet(EffectSetting::DISPLAY_EFFECT_NAME)) {
                BSStringT<char>* EffectName = pEffectNode->m_item->GetEffectName(&apString);
                BSstrcpy(Dst, 128, EffectName->pString);
                apString.~BSStringT<char>();
                if (bAppended) {
                    BSstrcat(apBuffer, static_cast<int32_t>(auiSize), "\n");
                    BSstrcat(apBuffer, static_cast<int32_t>(auiSize), Dst);
                }
                else {
                    BSstrcat(apBuffer, static_cast<int32_t>(auiSize), Dst);
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
        if ((pDataNode->m_item->iMagnitude) && (pDataNode->m_item->iActorValue != ActorValue::VALUE_NONE)) {
            ActorValue::Index aiIndex = pDataNode->m_item->iActorValue;
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
            if (ActorValue::IsFlagSet(aiIndex, ActorValue::DAMAGE_IS_POSITIVE)) {
                iMagnitude = -iMagnitude;
            }
            if (bMedicine) {
                double dMagnitude = static_cast<double>(iMagnitude);
                float fMedicine = pPlayer->GetMedicineEffectivenessMultiplier() * dMagnitude;
                if (aiIndex == ActorValue::HEALTH) {
                    BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::MODIFY_RECOVERED_HEALTH, pPlayer, &fMedicine);
                }
                iMagnitude = static_cast<int32_t>(fMedicine);
            }
            if (bFood && aiIndex != ActorValue::RADIATION_RADS) {
                double dMagnitude = static_cast<double>(iMagnitude);
                float fSurvival = pPlayer->GetSurvivalEffectivenessMultiplier() * dMagnitude;
                if (aiIndex == ActorValue::HEALTH) {
                    BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::MODIFY_RECOVERED_HEALTH, pPlayer, &fSurvival);
                }
                iMagnitude = static_cast<int32_t>(fSurvival);
            }
            if (aiIndex == ActorValue::RADIATION_RADS && iMagnitude > 0) {
                double dMagnitude = static_cast<double>(iMagnitude);
                float fRads = pPlayer->GetRadiationResistanceMult() * dMagnitude;
                iMagnitude = static_cast<int32_t>(std::floor(fRads));
            }
            if (bFood && aiIndex == ActorValue::RADIATION_RADS) {
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
                        Dst, 128, "%s %+i(%d%c)", ActorValue::GetActorValueAbbreviation(aiIndex), iMagnitude, iDuration,
                        iUnit
                    );
                }
                else {
                    BSsprintf(
                        Dst, 128, "%s %+i(%.1f%c)", ActorValue::GetActorValueAbbreviation(aiIndex), iMagnitude,
                        static_cast<double>(fDuration), static_cast<int32_t>(cUnit)
                    );
                }
            }
            else {
                BSsprintf(Dst, 128, "%s %+i", ActorValue::GetActorValueAbbreviation(aiIndex), iMagnitude);
            }
            BSstrcat(apBuffer, static_cast<int32_t>(auiSize), Dst);
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
        if ((pItem) && (pItem->pEffectSetting->IsFlagSet(aeFlag))) {
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
        if ((pItem) && (!pItem->data.iRange)) {
            bFound = !pItem->IsDisabled();
        }

        BSSimpleList<EffectItem*>* pNext = pIter->m_pkNext;
        pIter = pNext ? static_cast<EffectItemList*>(pNext) : nullptr;
    }
    return bFound;
}

bool EffectItemList::HasTouchEffect() {
    EffectItemList* v2;
    BSSimpleList<EffectItem*>* v3;
    EffectItem* v4;
    EffectItemList* pIter = this;
    bool vCondition1;
    vCondition1 = static_cast<BSSimpleList<EffectItem*>*>(this)->IsEmpty();
    if (vCondition1) {
        return false;
    }
    uint8_t bEnabled = 0;
    while (pIter && (!bEnabled)) {
        v4 = static_cast<BSSimpleList<EffectItem*>*>(pIter)->m_item;
        if ((v4) && (((uint32_t)v4->EffectItem::GetRange()) == 1)) {
            bEnabled = (uint8_t)(!v4->IsDisabled());
        }

        v3 = static_cast<BSSimpleList<EffectItem*>*>(pIter)->m_pkNext;
        if (v3) {
            v2 = static_cast<EffectItemList*>(v3);
        }
        else {
            v2 = nullptr;
        }
        pIter = v2;
    }
    return (bool)bEnabled;
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
    FNVD_INLINED_LOCAL(BSStringT<char>, kEffectName, std::nullopt, std::nullopt);
    CHUNK_ID eChunk = apFile->GetTESChunk();
    if (eChunk == CIDA_ID) {
        if (pLastEffect) {
            pLastEffect->Load(apFile, apSpellName);
            if (pLastEffect->IsHostile()) {
                ++iHostileCount;
            }
        }
        else {
            const char* pName;
            if (apSpellName) {
                pName = apSpellName;
            }
            else {
                pName = "{unknown}";
            }
            Warning("MASTERFILE: Trying to load condition data with no effect item in Actor Effect '%s'.", pName);
        }
    }
    else {
        if (eChunk == EFID_ID) {
            uint32_t iEffectID = 0;
            apFile->GetChunkData<uint32_t>(iEffectID);
            TESForm::AddCompileIndex(iEffectID, apFile);
            TESForm* pForm = TESForm::GetFormByNumericID(iEffectID);
            EffectSetting* pEffectSetting = dynamic_cast<EffectSetting*>(pForm);
            if (pEffectSetting) {
                EffectItem* pNew;
                void* pMemory = ourMalloc(sizeof(EffectItem));
                if (pMemory) {
                    pNew = ::new (pMemory) EffectItem(pEffectSetting);
                }
                else {
                    pNew = nullptr;
                }
                EffectItem* pEffect = pNew;
                apFile->NextChunk();
                // Stores 0 to an otherwise unused local, as the game does (a debug breakpoint spot in the source).
                if (pEffectSetting->IsAssociatedActorValueUsed() &&
                    pEffectSetting->GetAssociatedActorValue() == ActorValue::RADIATION_RADS) {
                    int32_t i = 0;
                }
                if (pEffect->Load(apFile, apSpellName)) {
                    AddEffect(pEffect);
                }
                else {
                    const char* pName;
                    if (apSpellName) {
                        pName = apSpellName;
                    }
                    else {
                        pName = "{unknown}";
                    }
                    BSStringT<char>* pEffectName = pEffectSetting->GetEffectName(&kEffectName);
                    Warning(
                        "MASTERFILE: Unable to load EffectItem '%s' in Actor Effect '%s'", pEffectName->String(), pName
                    );
                    kEffectName.~BSStringT<char>();
                    if (pEffect) {
                        delete pEffect;
                    }
                    pEffect = nullptr;
                }
                pLastEffect = pEffect;
            }
            else {
                const char* pName;
                if (apSpellName) {
                    pName = apSpellName;
                }
                else {
                    pName = "{unknown}";
                }
                Warning("MASTERFILE: Unable to find EffectSetting %d in Actor Effect '%s'.", iEffectID, pName);
            }
        }
    }
}

bool EffectItemList::IsHostile() {
    return (bool)((uint8_t)(((int32_t)iHostileCount) > 0));
}

bool EffectItemList::CanBePoison() {
    EffectItemList* v2;
    BSSimpleList<EffectItem*>* v3;
    EffectItem* v4;
    EffectItemList* v5 = this;
    bool vCondition1;
    vCondition1 = static_cast<BSSimpleList<EffectItem*>*>(this)->IsEmpty();
    if (vCondition1) {
        return false;
    }
    while (v5) {
        v4 = static_cast<BSSimpleList<EffectItem*>*>(v5)->m_item;
        if (v4 && ((!v4->IsDisabled()) && (!v4->IsHostile()))) {
            return false;
        }
        v3 = static_cast<BSSimpleList<EffectItem*>*>(v5)->m_pkNext;
        if (v3) {
            v2 = static_cast<EffectItemList*>(v3);
        }
        else {
            v2 = nullptr;
        }
        v5 = v2;
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
            EffectItemList* p_m_pkNext;
            BSSimpleList<EffectItem*>* pNext = pThis->m_pkNext;
            if (pNext) {
                p_m_pkNext = static_cast<EffectItemList*>(pNext);
            }
            else {
                p_m_pkNext = nullptr;
            }
            pThis = p_m_pkNext;
        }
    }
}

// The first effect whose magic effect is the usage monitor effect, or null.
EffectItem* EffectItemList::GetUsageMonitorEffect() {
    EffectItemList* pIter = this;
    EffectItem* pResult = nullptr;
    while (pIter && (!pIter->IsEmpty() && !pResult)) {
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
    EffectItemList* v4;
    BSSimpleList<EffectItem*>* v5;
    EffectItem* pEffectItem;
    int32_t iCost;
    EffectItem* pResult = nullptr;
    EffectItemList* pIter = this;
    bool vCondition1;
    vCondition1 = static_cast<BSSimpleList<EffectItem*>*>(this)->IsEmpty();
    if (vCondition1) {
        return nullptr;
    }
    int32_t v11 = (-1);
    while (pIter) {
        pEffectItem = static_cast<BSSimpleList<EffectItem*>*>(pIter)->m_item;
        iCost = (int32_t)pEffectItem->GetCost(nullptr);
        if ((((!pEffectItem->IsDisabled()) && (iCost > v11)) && ((!abAreaOnly) || (pEffectItem->GetArea() > 0))) &&
            ((aeRange == 3) || (((uint32_t)pEffectItem->EffectItem::GetRange()) == aeRange))) {
            pResult = pEffectItem;
            v11 = iCost;
        }
        v5 = static_cast<BSSimpleList<EffectItem*>*>(pIter)->m_pkNext;
        if (v5) {
            v4 = static_cast<EffectItemList*>(v5);
        }
        else {
            v4 = nullptr;
        }
        pIter = v4;
    }
    return pResult;
}

bool EffectItemList::HasTargetEffect() {
    EffectItemList* v2;
    BSSimpleList<EffectItem*>* v3;
    EffectItem* v4;
    EffectItemList* pIter = this;
    bool vCondition1;
    vCondition1 = static_cast<BSSimpleList<EffectItem*>*>(this)->IsEmpty();
    if (vCondition1) {
        return false;
    }
    uint8_t v6 = 0;
    while (pIter && (!v6)) {
        v4 = static_cast<BSSimpleList<EffectItem*>*>(pIter)->m_item;
        if ((v4) && (((uint32_t)v4->EffectItem::GetRange()) == 2)) {
            v6 = (uint8_t)(!v4->IsDisabled());
        }

        v3 = static_cast<BSSimpleList<EffectItem*>*>(pIter)->m_pkNext;
        if (v3) {
            v2 = static_cast<EffectItemList*>(v3);
        }
        else {
            v2 = nullptr;
        }
        pIter = v2;
    }
    return (bool)v6;
}

ActorValue::Index EffectItemList::GetAssociatedSkill() {
    int32_t v2 = (-1);
    EffectItem* costliestEffect = GetCostliestEffect(MagicSystem::RANGE_COUNT, false);
    if (costliestEffect) {
        return static_cast<ActorValue::Index>((int32_t)costliestEffect->GetMagicSchool());
    }
    return static_cast<ActorValue::Index>(v2);
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
    if ((!apEffect->IsUsageMonitorEffect()) || (!GetUsageMonitorEffect())) {
        static_cast<BSSimpleList<EffectItem*>*>(this)->AddTail(apEffect);
        if (apEffect->IsHostile()) {
            ++iHostileCount;
        }
    }
}

void EffectItemList::Copy(EffectItemList* apCopy) {
    EffectItemList* v2;
    BSSimpleList<EffectItem*>* v3;
    EffectItem* v4;
    void* v6;
    EffectItem* v7;
    RemoveAllEffects();
    while (apCopy) {
        v7 = static_cast<BSSimpleList<EffectItem*>*>(apCopy)->m_item;
        if (v7) {
            v6 = ourMalloc(sizeof(EffectItem));
            if (v6) {
                v4 = ::new (v6) EffectItem(v7);
            }
            else {
                v4 = nullptr;
            }
            AddEffect(v4);
        }
        v3 = static_cast<BSSimpleList<EffectItem*>*>(apCopy)->m_pkNext;
        if (v3) {
            v2 = static_cast<EffectItemList*>(v3);
        }
        else {
            v2 = nullptr;
        }
        apCopy = v2;
    }
}

EffectItem* EffectItemList::GetEffectItemByIndex(uint8_t aiIndex) {
    BSSimpleList<EffectItem*>* v3;
    BSSimpleList<EffectItem*>* ecx0;
    uint8_t v5 = 0;
    if (this) {
        v3 = static_cast<BSSimpleList<EffectItem*>*>(this);
    }
    else {
        v3 = (BSSimpleList<EffectItem*>*)nullptr;
    }
    for (ecx0 = v3; ecx0; ecx0 = ecx0->m_pkNext) {
        if (v5 == aiIndex) {
            return ecx0->m_item;
        }
        ++v5;
    }
    Warning(
        "GAME EFFECTS: When trying to get an effect item by its index, the index was larger than the size of the "
        "list."
    );
    return nullptr;
}

bool EffectItemList::IsDisabled() {
    EffectItemList* v2;
    BSSimpleList<EffectItem*>* v3;
    EffectItem* v4;
    uint8_t bIsDisabled = 0;
    EffectItemList* pIter = this;
    bool vCondition1;
    vCondition1 = static_cast<BSSimpleList<EffectItem*>*>(this)->IsEmpty();
    if (vCondition1) {
        return false;
    }
    while (pIter && (!bIsDisabled)) {
        v4 = static_cast<BSSimpleList<EffectItem*>*>(pIter)->m_item;
        if (v4) {
            bIsDisabled = (uint8_t)v4->IsDisabled();
        }
        v3 = static_cast<BSSimpleList<EffectItem*>*>(pIter)->m_pkNext;
        if (v3) {
            v2 = static_cast<EffectItemList*>(v3);
        }
        else {
            v2 = nullptr;
        }
        pIter = v2;
    }
    return (bool)bIsDisabled;
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
            if (pThisItem && (pOtherItem && pThisItem->MatchesEffect(pOtherItem))) {
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
    BSSimpleList<EffectItem*>* v3;
    uint8_t v5 = 0;
    if (this) {
        v3 = static_cast<BSSimpleList<EffectItem*>*>(this);
    }
    else {
        v3 = nullptr;
    }
    for (EffectItem*& rpItem : *v3) {
        if (apEffect == rpItem) {
            return v5;
        }
        ++v5;
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
        if ((pItem) && (pItem->GetScript() == apScript)) {
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
