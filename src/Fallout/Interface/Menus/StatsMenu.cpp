#include "Fallout/Interface/Menus/StatsMenu.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSDevices/Controls.h"
#include "BSMain/Setting.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileMenu.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/AiFormulas.h"
#include "Fallout Shared/BGSDefaultObjectManager.h"
#include "Fallout Shared/BGSDehydration.h"
#include "Fallout Shared/BGSEquipType.h"
#include "Fallout Shared/BGSHunger.h"
#include "Fallout Shared/BGSPerk.h"
#include "Fallout Shared/BGSRadiation.h"
#include "Fallout Shared/BGSSleepDeprevation.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Magic/AlchemyItem.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/Magic/SpellItem.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESDescription.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESReputation.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/FOPipboyManager.h"
#include "Fallout/Interface/FORenderedMenu.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/ListBox.h"
#include "Fallout/Interface/MenuManager.h"
#include "Fallout/Interface/Menus/HackingMenu.h"
#include "Fallout/Interface/Menus/SleepWaitMenu.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Misc/Main.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/rtti.h"

#include <cmath>
#include <cstdint>

// Whether the player was in hardcore mode at the last DoIdle; it starts out set.
static bool bWasHardcore = true;

// ListBox<PerkRankData*>::NumUnfiltered's guarded static and its guard.
static int32_t iUT_num_filtered;
static uint32_t iUT_num_filtered_guard;

// The time (BSGetTickCount) at which DoIdle next refreshes the meters.
static uint32_t uiNextMeterUpdateTime;
static int32_t eenabledtrait;

// UpdateHealingMode's guarded statics: the user trait ids of "_xbox_button" and "_enabled", and their shared guard.
static int32_t exbbuttontrait;
static uint32_t exbbuttontrait_guard;

using StatsMenuUnusedBytes = uint8_t[0xC];
static StatsMenuUnusedBytes data_11DAD90{};

StatsMenu::HealingOption::HealingOption()
  : pAlchemyItem(nullptr), pTile(nullptr), pInventoryItem(nullptr),
    pfTargetConditionCallback(&StatsMenu::HealingOption::DefaultTargetConditionCallback) {}

StatsMenu::HealingOption::~HealingOption() {
    if (pInventoryItem) {
        delete pInventoryItem;
    }
}

bool StatsMenu::HealingOption::DefaultTargetConditionCallback(StatsMenu::HealingOption& aHO) {
    return aHO.pInventoryItem != nullptr;
}

StatsMenu::StatusDataList::~StatusDataList() {
    Flush();
}

void StatsMenu::StatusDataList::Register(ActiveEffect* apActiveEffect) {
    if (apActiveEffect) {
        EffectItem* CastingType = apActiveEffect->pEffect;
        int32_t ActorValueIndex = CastingType->GetActorValueIndex();
        EffectItemData* a2 = GetEntryByAVCode(ActorValueIndex);
        if (a2) {
            a2->iMagnitude += static_cast<int32_t>(apActiveEffect->fMagnitude);
        }
        else {
            EffectItemData* v5;
            EffectItemData* apMemory = static_cast<EffectItemData*>(ourMalloc(sizeof(EffectItemData)));
            if (apMemory) {
                v5 = ::new (apMemory) EffectItemData;
            }
            else {
                v5 = nullptr;
            }
            a2 = v5;
            EffectItem* v4 = apActiveEffect->pEffect;
            v5->iActorValue = v4->GetActorValueIndex();
            a2->iMagnitude = static_cast<int32_t>(apActiveEffect->fMagnitude);
            AddHead(a2);
        }
    }
}

// Walks the list's nodes for the entry of the actor value.
EffectItemData* StatsMenu::StatusDataList::GetEntryByAVCode(int32_t aiActorValue) {
    EffectItemData* pResult = nullptr;
    BSSimpleList<EffectItemData*>* pIter = this ? static_cast<BSSimpleList<EffectItemData*>*>(this) : nullptr;
    for (; !pResult && pIter; pIter = pIter->GetNext()) {
        if (pIter->GetItem() && pIter->GetItem()->iActorValue == aiActorValue) {
            pResult = pIter->GetItem();
        }
    }
    return pResult;
}

int32_t StatsMenu::StatusDataList::Flush() {
    int32_t result;
    while (true) {
        result = IsEmpty();
        if (static_cast<uint8_t>(result)) {
            break;
        }
        void* Block = m_item;
        ourFree(Block);
        RemoveHead();
    }
    return result;
}

void StatsMenu::StatusDataList::Register(EffectItem* arg0) {
    if (arg0) {
        int32_t ActorValueIndex = arg0->GetActorValueIndex();
        EffectItemData* a2 = GetEntryByAVCode(ActorValueIndex);
        if (a2) {
            a2->iMagnitude += arg0->GetMagnitude();
        }
        else {
            EffectItemData* v5;
            EffectItemData* apMemory = static_cast<EffectItemData*>(ourMalloc(sizeof(EffectItemData)));
            if (apMemory) {
                v5 = ::new (apMemory) EffectItemData;
            }
            else {
                v5 = nullptr;
            }
            a2 = v5;
            v5->iActorValue = arg0->GetActorValueIndex();
            a2->iMagnitude = arg0->GetMagnitude();
            EffectSetting* Time = arg0->pEffectSetting;
            if (Time->IsFlagSet(EffectSetting::DETRIMENTAL)) {
                a2->iMagnitude = -a2->iMagnitude;
            }
            EffectSetting* v4 = arg0->pEffectSetting;
            if (!v4->IsFlagSet(EffectSetting::RECOVER)) {
                a2->iDuration = arg0->GetDuration();
            }
            AddHead(a2);
        }
    }
}

StatsMenu::StatusEffectList::~StatusEffectList() {
    Flush();
}

void StatsMenu::StatusEffectList::Flush() {
    while (!IsEmpty()) {
        StatsMenu::StatusEffect* apThis = m_item;
        if (apThis) {
            delete apThis;
        }
        RemoveHead();
    }
}

void StatsMenu::StatusEffectList::Register(ActiveEffect* apActiveEffect) {
    TESForm* pform = dynamic_cast<TESForm*>(apActiveEffect->pSpell);
    uint32_t CastingType = pform->iFormID;
    StatsMenu::StatusEffect* peffect = Find(CastingType);
    if (!peffect) {
        StatsMenu::StatusEffect* peffect_2;
        StatsMenu::StatusEffect* apMemory =
            static_cast<StatsMenu::StatusEffect*>(ourMalloc(sizeof(StatsMenu::StatusEffect)));
        if (apMemory) {
            peffect_2 = ::new (apMemory) StatsMenu::StatusEffect(pform);
        }
        else {
            peffect_2 = nullptr;
        }
        peffect = peffect_2;
        Insert(peffect_2, &StatsMenu::StatusEffectSortFn);
    }
    EffectSetting* EffectSetting_ = apActiveEffect->GetEffectSetting();
    if (EffectSetting_->IsFlagSet(EffectSetting::DISPLAY_EFFECT_NAME)) {
        peffect->xNamedEffects.Register(apActiveEffect);
    }
    else {
        peffect->xStatsEffected.Register(apActiveEffect);
    }
}

StatsMenu::StatusEffect* StatsMenu::StatusEffectList::Find(uint32_t aiID) {
    BSSimpleList<StatsMenu::StatusEffect*>* pnode;
    StatsMenu::StatusEffect* peffect = nullptr;
    if (this) {
        pnode = static_cast<BSSimpleList<StatsMenu::StatusEffect*>*>(this);
    }
    else {
        pnode = nullptr;
    }
    for (BSSimpleList<StatsMenu::StatusEffect*>* pnode_2 = pnode; !peffect && (pnode_2 && pnode_2->m_item);
         pnode_2 = pnode_2->m_pkNext) {
        if (pnode_2->m_item->pEffectForm->iFormID == aiID) {
            peffect = pnode_2->m_item;
        }
    }
    return peffect;
}

StatsMenu::StatusEffect::StatusEffect(TESForm* apEffectForm)
  : pEffectForm(apEffectForm), xStatsEffected(), xNamedEffects() {}

// The lists release themselves.
StatsMenu::StatusEffect::~StatusEffect() {}

StatsMenu::BodyPartController::BodyPartController() : eCurrentBodyPart(SM_HEAD), listFlashingTiles() {
    ourMemset(pBodyPartMapA, -1, sizeof(pBodyPartMapA));
}

StatsMenu::BodyPartController::~BodyPartController() {
    for (BSSimpleList<Tile*>& rList : listFlashingTiles) {
        rList.RemoveAll();
    }
}

StatsMenu::BodyPartController::BodyPartIndex StatsMenu::BodyPartController::Find(Tile* apTile) {
    StatsMenu::BodyPartController::BodyPartIndex eindex;
    for (eindex = StatsMenu::BodyPartController::SM_HEAD; eindex < StatsMenu::BodyPartController::SM_BODYPART_COUNT;
         ++eindex) {
        if (listFlashingTiles[eindex].IsInList(apTile)) {
            return eindex;
        }
    }
    return StatsMenu::BodyPartController::SM_BODYPART_COUNT;
}

void StatsMenu::BodyPartController::AddFlash(StatsMenu::BodyPartController::BodyPartIndex aeBodyPart, Tile* apTile) {
    listFlashingTiles[aeBodyPart].AddHead(apTile);
}

void StatsMenu::BodyPartController::LinkBodyParts(BodyPartIndex aeFrom, Direction aeDir, BodyPartIndex aeTo) {
    pBodyPartMapA[(int32_t)aeFrom] &= (~(255 << (8 * ((int32_t)aeDir))));
    pBodyPartMapA[(int32_t)aeFrom] |= (((int32_t)aeTo) << (8 * ((int32_t)aeDir)));
}

// Runs FadeControlCallback with aiOn over the current body part's flashing tiles.
void StatsMenu::BodyPartController::SetFlash(uint32_t aiOn) {
    listFlashingTiles[eCurrentBodyPart].Traverse(&StatsMenu::FadeControlCallback, aiOn);
}

void StatsMenu::BodyPartController::SetCurrentBodyPart(
    StatsMenu::BodyPartController::BodyPartIndex aeBodyPart,
    bool abFlash
) {
    if (abFlash) {
        SetFlash(0);
    }
    eCurrentBodyPart = static_cast<StatsMenu::BodyPartController::BodyPartIndex>((int32_t)aeBodyPart);
    if (abFlash) {
        SetFlash(1);
    }
}

StatsMenu::BodyPartController::BodyPartIndex StatsMenu::BodyPartController::GetLink(
    StatsMenu::BodyPartController::BodyPartIndex aeFrom,
    StatsMenu::BodyPartController::Direction aeDir
) {
    return static_cast<StatsMenu::BodyPartController::BodyPartIndex>(
        (pBodyPartMapA[(int32_t)aeFrom] & (255 << (8 * ((int32_t)aeDir)))) >> (8 * ((int32_t)aeDir))
    );
}

StatsMenu::BodyPartController* StatsMenu::BodyPartController::GetInstance() {
    if (!StatsMenu::BodyPartController::pInstance) {
        StatsMenu::BodyPartController* v1;
        StatsMenu::BodyPartController* apMemory =
            static_cast<StatsMenu::BodyPartController*>(ourMalloc(sizeof(StatsMenu::BodyPartController)));
        if (apMemory) {
            v1 = ::new (apMemory) StatsMenu::BodyPartController;
        }
        else {
            v1 = nullptr;
        }
        StatsMenu::BodyPartController::pInstance = v1;
    }
    return StatsMenu::BodyPartController::pInstance;
}

ActorValue::Index StatsMenu::BodyPartController::Convert(StatsMenu::BodyPartController::BodyPartIndex aeBodyPart) {
    int32_t result;
    switch ((int32_t)aeBodyPart) {
        case 0: {
            result = 25;
            break;
        }
        case 1: {
            result = 28;
            break;
        }
        case 2: {
            result = 26;
            break;
        }
        case 3: {
            result = 27;
            break;
        }
        case 4: {
            result = 30;
            break;
        }
        case 5: {
            result = 29;
            break;
        }
        default: {
            result = (-1);
            break;
        }
    }
    return static_cast<ActorValue::Index>(result);
}

void StatsMenu::BodyPartController::operator delete(void* apMemory) {
    ourFree(apMemory);
}

StatsMenu::~StatsMenu() {
    StatsMenu::pMe = nullptr;
    if (pRadiationEffect) {
        delete pRadiationEffect;
    }
    if (pSleepEffect) {
        delete pSleepEffect;
    }
    if (pHungerEffect) {
        delete pHungerEffect;
    }
    if (pDehydrationEffect) {
        delete pDehydrationEffect;
    }
    StatsMenu::BodyPartController::DeleteInstance();
}

void StatsMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
    if (aiID >= 0 && aiID < 60) {
        this->pTiles[aiID] = apTile;
    }
}

void StatsMenu::DoClick(int32_t aiID, Tile* apTarget) {
    if (!apTarget && aiID >= 0 && aiID < NUM_TILES) {
        apTarget = pTiles[aiID];
    }
    if (!InterfaceManager::Has360Controller() || apTarget->IsTrue(4015)) {
        StatsMenu::Page epage;
        if (GetPageFor(static_cast<StatsMenu::TileIndex>(aiID), epage)) {
            SetCurrentPage(epage);
        }
        else {
            StatsMenu::StatusPage estatuspage;
            if (GetStatusPageFor(static_cast<StatsMenu::TileIndex>(aiID), estatuspage)) {
                SetStatusPage(estatuspage);
            }
            else {
                if (aiID == 9 || (bLimbMode && aiID == 5)) {
                    ToggleHealingMode();
                }
                else {
                    if (aiID == 49) {
                        ToggleGenRepMode();
                    }
                    else {
                        for (StatsMenu::HealingOptionID i = StatsMenu::SM_HO_STIMPAK; i < StatsMenu::SM_HO_COUNT;
                             i = static_cast<StatsMenu::HealingOptionID>(i + 1)) {
                            if ((apTarget == pHealingOptionsA[i].pTile) && (pHealingOptionsA[i].pInventoryItem)) {
                                ExtraDataList* extraData;
                                if (pHealingOptionsA[i].pInventoryItem->pExtraObjectList) {
                                    extraData = pHealingOptionsA[i].pInventoryItem->pExtraObjectList->m_item;
                                }
                                else {
                                    extraData = nullptr;
                                }
                                uint32_t FormHealth_0 = pHealingOptionsA[i].pInventoryItem->iNumber;
                                TESBoundObject* FormFlags = pHealingOptionsA[i].pInventoryItem->pContainerObj;
                                pPlayer->QueueEquipObject(FormFlags, FormHealth_0, extraData, false, false, true);
                                for (BSSimpleList<EffectItem*>* pnode = dynamic_cast<BSSimpleList<EffectItem*>*>(
                                         pHealingOptionsA[i].pInventoryItem->pContainerObj
                                     );
                                     pnode && pnode->m_item; pnode = pnode->m_pkNext) {
                                    ActorValue::Index ActorValueIndex = pnode->m_item->GetActorValueIndex();
                                    UpdateActorValue(ActorValueIndex);
                                }
                                UpdateHealingOptions(i);
                                bActiveEffectsUpdatedThisFrame = false;
                                UpdateActiveEffects();
                                i = StatsMenu::SM_HO_COUNT;
                            }
                        }
                        if (bLimbMode && !InterfaceManager::Has360Controller()) {
                            StatsMenu::BodyPartController* Instance = StatsMenu::BodyPartController::GetInstance();
                            if (Instance->Find(apTarget) != StatsMenu::BodyPartController::SM_BODYPART_COUNT) {
                                if (pPlayer->IsHardcore()) {
                                    DoClick(59, nullptr);
                                }
                                else {
                                    DoClick(10, nullptr);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

// A list entry the cursor enters: an apTarget of null stands for the menu's own tile aiID. On the status page a limb
// tile picks that body part (with a highlight sound) and turns on the limb mode, when a healing item is at hand and no
// 360 controller is used; on the other pages the list entry shows its icon and description.
void StatsMenu::DoEnter(int32_t aiID, Tile* apTarget) {
    if (!apTarget && aiID >= 0 && aiID < NUM_TILES) {
        apTarget = pTiles[aiID];
    }
    switch (GetCurrentPage()) {
        case SM_STATUS:
            if (apTarget->GetFloat(Tile::kTileValue_ListIndex) > 0.0) {
                MoveKnob(apTarget, aiPageListData[SM_STATUS]);
            }
            if (!InterfaceManager::Has360Controller()) {
                ItemChange* pItem = (pPlayer && pPlayer->IsHardcore()) ? pHealingOptionsA[SM_HO_DRBAG].pInventoryItem
                                                                       : pHealingOptionsA[SM_HO_STIMPAK].pInventoryItem;
                if (pItem) {
                    BodyPartController::BodyPartIndex eBodyPart = BodyPartController::GetInstance()->Find(apTarget);
                    if (eBodyPart != BodyPartController::SM_BODYPART_COUNT) {
                        BodyPartController::GetInstance()->SetCurrentBodyPart(eBodyPart, true);
                        BSAudio::QInstance()
                            .GetSoundHandleByName(
                                "UIPipBoyHighlight",
                                BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
                                    BSGameSound::SOUND_FLAG_ONE_SHOT
                            )
                            .Play(false);
                        if (!bLimbMode) {
                            ToggleHealingMode();
                        }
                    }
                }
            }
            break;
        case SM_SPECIAL:
            if (lbSpecial.IsListItem(apTarget)) {
                MoveKnob(apTarget, aiPageListData[SM_SPECIAL]);
                SetIconAndDescription(lbSpecial.GetDataFor(apTarget));
            }
            break;
        case SM_SKILLS:
            if (lbSkills.IsListItem(apTarget)) {
                MoveKnob(apTarget, aiPageListData[SM_SKILLS]);
                SetIconAndDescription(lbSkills.GetDataFor(apTarget));
            }
            break;
        case SM_PERKS:
            if (lbPerks.IsListItem(apTarget)) {
                MoveKnob(apTarget, aiPageListData[SM_PERKS]);
                SetIconAndDescription(lbPerks.GetDataFor(apTarget));
            }
            break;
        case SM_GENERAL:
            if (lbGeneral.IsListItem(apTarget)) {
                MoveKnob(apTarget, aiPageListData[SM_GENERAL]);
            }
            if (lbReputation.IsListItem(apTarget)) {
                SetReputation(lbReputation.GetDataFor(apTarget));
                MoveKnob(apTarget, aiPageListData[SM_GENERAL]);
            }
            break;
    }
}

void StatsMenu::DoLeave(int32_t aiID, Tile* apTarget) {
    if (bLimbMode && !InterfaceManager::Has360Controller()) {
        StatsMenu::BodyPartController* Instance = StatsMenu::BodyPartController::GetInstance();
        if (Instance->Find(apTarget) != StatsMenu::BodyPartController::SM_BODYPART_COUNT) {
            ToggleHealingMode();
        }
    }
    pTiles[52]->Set(4003, 0);
}

// While the pip-boy is up, the meters are refreshed at most once every user3 seconds. The hardcore meters' buttons
// follow the player's hardcore mode.
void StatsMenu::DoIdle() {
    this->bActiveEffectsUpdatedThisFrame = false;
    if (Interface::GetPipboy()->GetInitialized()) {
        if (InterfaceManager::Instance()->IsInPipboyMenu() && BSGetTickCount() >= uiNextMeterUpdateTime) {
            UpdateRadLevel();
            if (pPlayer && pPlayer->IsHardcore()) {
                UpdateH20Level();
                UpdateFODLevel();
                UpdateSLPLevel();
            }
            double now = BSGetTickCount();
            uiNextMeterUpdateTime = static_cast<uint32_t>(
                static_cast<int64_t>(GetMenuTile()->GetFloat(Tile::kTileValue_User3) * 1000.0 + now)
            );
        }
        if (this->bLimbMode && !Interface::IsTopMenuID(Interface::eStatsMenu)) {
            ToggleHealingMode();
        }
    }

    if (GetCurrentPage() == SM_STATUS) {
        int32_t statusPage = GetStatusPage();
        if (statusPage > (pPlayer->IsHardcore() ? SM_LAST_STATUS_PAGE_HARDCORE : SM_LAST_STATUS_PAGE)) {
            SetStatusPage(SM_STATUS_CONDITION);
        }
    }

    if (pPlayer && !pPlayer->IsHardcore() && bWasHardcore) {
        StatsMenu::pMe->pTiles[eH20Button]->Set(Tile::kTileValue_Target, 0);
        StatsMenu::pMe->pTiles[eH20Button]->Set(Tile::kTileValue_Visible, 0);
        StatsMenu::pMe->pTiles[eSLPButton]->Set(Tile::kTileValue_Target, 0);
        StatsMenu::pMe->pTiles[eSLPButton]->Set(Tile::kTileValue_Visible, 0);
        StatsMenu::pMe->pTiles[eFODButton]->Set(Tile::kTileValue_Target, 0);
        StatsMenu::pMe->pTiles[eFODButton]->Set(Tile::kTileValue_Visible, 0);
        bWasHardcore = false;
    }
    else if (pPlayer && pPlayer->IsHardcore() && !bWasHardcore) {
        StatsMenu::pMe->pTiles[eH20Button]->Set(Tile::kTileValue_Target, 1);
        StatsMenu::pMe->pTiles[eH20Button]->Set(Tile::kTileValue_Visible, 1);
        StatsMenu::pMe->pTiles[eSLPButton]->Set(Tile::kTileValue_Target, 1);
        StatsMenu::pMe->pTiles[eSLPButton]->Set(Tile::kTileValue_Visible, 1);
        StatsMenu::pMe->pTiles[eFODButton]->Set(Tile::kTileValue_Target, 1);
        StatsMenu::pMe->pTiles[eFODButton]->Set(Tile::kTileValue_Visible, 1);
    }
    bWasHardcore = pPlayer->IsHardcore();
}

int32_t StatsMenu::GetClass() {
    return 1003;
}

bool StatsMenu::DoGamepad(int32_t aiButton, float afValue) {
    StatsMenu::Page epage_2;
    StatsMenu::Page epage;
    if (!Interface::IsTopMenuID(Interface::eStatsMenu)) {
        return false;
    }
    FOPipboyManager* Pipboy = Interface::GetPipboy();
    if (!Pipboy->GetInitialized()) {
        return false;
    }
    if (!Interface::IsTopMenuID(Interface::eStatsMenu)) {
        return false;
    }
    if (this->bLimbMode && (aiButton == 1 || (aiButton == 2 || (aiButton == 4 || aiButton == 3)))) {
        StatsMenu::BodyPartController::Direction edir;
        int32_t bLimbMode = this->bLimbMode;
        switch (aiButton) {
            case 1: {
                edir = StatsMenu::BodyPartController::DIR_UP;
                break;
            }
            case 2: {
                edir = StatsMenu::BodyPartController::DIR_DOWN;
                break;
            }
            case 3: {
                edir = StatsMenu::BodyPartController::DIR_RIGHT;
                break;
            }
            case 4: {
                edir = StatsMenu::BodyPartController::DIR_LEFT;
                break;
            }
        }
        StatsMenu::BodyPartController::BodyPartIndex afRate =
            StatsMenu::BodyPartController::GetInstance()->eCurrentBodyPart;
        StatsMenu::BodyPartController* Instance = StatsMenu::BodyPartController::GetInstance();
        StatsMenu::BodyPartController::BodyPartIndex enewBodyPart = Instance->GetLink(afRate, edir);
        if (enewBodyPart < StatsMenu::BodyPartController::SM_BODYPART_COUNT) {
            StatsMenu::BodyPartController* v7 = StatsMenu::BodyPartController::GetInstance();
            v7->SetCurrentBodyPart(enewBodyPart, true);
            bool v8 = StatsMenu::StimpakTargetConditionCallback(*pHealingOptionsA);
            pHealingOptionsA[0].pTile->Set(4015, static_cast<int32_t>(v8));
            bool v9 = DrBagTargetConditionCallback(pHealingOptionsA[3]);
            pHealingOptionsA[3].pTile->Set(4015, static_cast<int32_t>(v9));
            BSAudio* v10 = &BSAudio::QInstance();
            v10->GetSoundHandleByName("UIPipBoyHighlight", 289u).Play(false);
        }
        return true;
    }
    if ((aiButton == 13 || aiButton == 14) && afValue >= 1.0) {
        InterfaceManager::PlayMenuSound(MENU_SOUND_MODE);
        if (aiButton == 13) {
            Interface::SetMapMenuVisible(true, false);
        }
        else {
            Interface::SetInventoryMenuVisible(true, false, true);
        }
        return true;
    }
    if (aiButton == 4) {
        epage = GetCurrentPage();
        if (epage) {
            StatsMenu::Page aePage = --epage;
            SetCurrentPage(aePage);
        }
        else {
            SetCurrentPage(StatsMenu::SM_GENERAL);
        }
        pTiles[0]->PlayTileSound(4043);
        return true;
    }
    if (aiButton == 3) {
        epage_2 = GetCurrentPage();
        if (epage_2 == StatsMenu::SM_GENERAL) {
            SetCurrentPage(StatsMenu::SM_STATUS);
        }
        else {
            StatsMenu::Page v30 = ++epage_2;
            SetCurrentPage(v30);
        }
        pTiles[0]->PlayTileSound(4043);
        return true;
    }
    if (GetCurrentPage() == StatsMenu::SM_GENERAL) {
        if (aiButton == 11) {
            DoClick(49, nullptr);
            return true;
        }
        goto LABEL_86;
    }
    if (GetCurrentPage()) {
        goto LABEL_86;
    }
    switch (aiButton) {
        case 1:
        case 2: {
            int32_t idelta;
            int32_t istatusPage;
            idelta = 2 * (aiButton != 1) - 1;
            istatusPage = idelta + GetStatusPage();
            if (SetStatusPage(istatusPage)) {
                StatsMenu::StatusPage etestPage = StatsMenu::SM_STATUS_PAGE_COUNT;
                if (GetStatusPageFor(StatsMenu::eConditionButton, etestPage) && etestPage == istatusPage) {
                    pTiles[28]->PlayTileSound(4043);
                }
                else {
                    if (GetStatusPageFor(StatsMenu::eRadsButton, etestPage) && etestPage == istatusPage) {
                        pTiles[29]->PlayTileSound(4043);
                    }
                    else {
                        if (GetStatusPageFor(StatsMenu::eEffectsButton, etestPage) && etestPage == istatusPage) {
                            pTiles[30]->PlayTileSound(4043);
                        }
                        else {
                            if (GetStatusPageFor(StatsMenu::eH20Button, etestPage) && etestPage == istatusPage) {
                                pTiles[53]->PlayTileSound(4043);
                            }
                            else {
                                if (GetStatusPageFor(StatsMenu::eFODButton, etestPage) && etestPage == istatusPage) {
                                    pTiles[55]->PlayTileSound(4043);
                                }
                                else {
                                    if (GetStatusPageFor(StatsMenu::eSLPButton, etestPage) &&
                                        etestPage == istatusPage) {
                                        pTiles[57]->PlayTileSound(4043);
                                    }
                                }
                            }
                        }
                    }
                }
                float afRatea = fScrollKnobRate.Float();
                float* v11 = &fScrollKnobIncrement.Float();
                float afAmount = static_cast<double>(idelta) * *v11;
                InterfaceManager* v12 = InterfaceManager::Instance();
                FOPipboyManager* v13 = v12->GetPipboy();
                v13->MoveKnob(FOPipboyManager::KNOB_SCROLL, afAmount, afRatea, true);
                return true;
            }
        LABEL_86:;
            if (Interface::Has360Controller() &&
                (lbStatusEffects.pScrollBar->IsVisible() || pTiles[26]->xChildren.GetTail()->IsTrue(4003))) {
                int32_t iscrollDelta = 0;
                Controls* Controls_ = appMain->GetControls();
                int32_t iyAxis = Controls_->GetControllerValue(0, 10);
                if (NiAbs(static_cast<float>(iyAxis)) <= 8689.0) {
                    iyAxis = 0;
                }
                if (iyAxis) {
                    iscrollDelta = 2 * (iyAxis < 0) - 1;
                }
                if (iscrollDelta && BSGetTickCount() > inext) {
                    Tile* Time;
                    int32_t Float;
                    if (GetCurrentPage()) {
                        Time = pTiles[26]->xChildren.GetTail();
                    }
                    else {
                        Time = lbStatusEffects.pScrollBar;
                    }
                    int32_t iUT_CurrentValue = Tile::TextToTrait("_current_value");
                    int32_t icurrScrollbarValue = static_cast<int32_t>(Time->GetFloat(iUT_CurrentValue));
                    Time->Set(iUT_CurrentValue, static_cast<float>(iscrollDelta + icurrScrollbarValue), false);
                    if (static_cast<double>(icurrScrollbarValue) != Time->GetFloat(iUT_CurrentValue)) {
                        int32_t v18 = Tile::TextToTrait("_SetInCode");
                        Time->Poke(v18, 1.0f);
                    }
                    if (inext) {
                        Float = static_cast<int32_t>(pTiles[33]->GetFloat(Tile::kTileValue_User0));
                    }
                    else {
                        Float = static_cast<int32_t>(pTiles[33]->GetFloat(Tile::kTileValue_User1));
                    }
                    inext = Float + BSGetTickCount();
                }
                else {
                    if (!iscrollDelta) {
                        inext = 0;
                    }
                }
            }
            return false;
        }
        case 9: {
            StatsMenu::StatusPage StatusPage = GetStatusPage();
            if (StatusPage) {
                if (StatusPage == StatsMenu::SM_STATUS_RADS) {
                    DoClick(11, nullptr);
                    return true;
                }

                return false;
            }

            DoClick(10, nullptr);
            return true;
        }
        case 11: {
            StatsMenu::StatusPage v27 = GetStatusPage();
            if (v27) {
                if (v27 != StatsMenu::SM_STATUS_RADS) {
                    return false;
                }
                goto LABEL_77;
            }
            if (this->bLimbMode || !pTiles[9]->IsTrue(4015)) {
                return false;
            }

            DoClick(9, nullptr);
            BSAudio* v14 = &BSAudio::QInstance();
            v14->GetSoundHandleByName("UIMenuOK", 289u).Play(false);
            return true;
        }
        case 12: {
            StatsMenu::StatusPage v25;
            v25 = GetStatusPage();
            if (v25) {
                if (v25 != StatsMenu::SM_STATUS_RADS) {
                    return false;
                }
            LABEL_77:;
                DoClick(31, nullptr);
                return true;
            }
            if (pTiles[9]->IsTrue(4015)) {
                DoClick(59, nullptr);
                BSAudio* v15 = &BSAudio::QInstance();
                v15->GetSoundHandleByName("UIMenuOK", 289u).Play(false);
                return true;
            }

            return false;
        }
        default: {
            goto LABEL_86;
        }
    }
}

bool StatsMenu::DoGamepadUpEvent(int32_t aiButton, float afValue) {
    if (!bLimbMode || (aiButton != 10 || !pTiles[9]->IsTrue(4015))) {
        return false;
    }
    DoClick(9, nullptr);
    BSAudio* v3 = &BSAudio::QInstance();
    v3->GetSoundHandleByName("UIMenuOK", 289u).Play(false);
    return true;
}

void StatsMenu::DoControllerSwitch(bool abHas360Controller) {
    UpdateHealingMode();
}

void StatsMenu::UpdateHealingMode() {
    // Guarded statics sharing one guard: exbbuttontrait (bit 0), then eenabledtrait (bit 1).
    if (!(exbbuttontrait_guard & 1)) {
        exbbuttontrait_guard |= 1;
        exbbuttontrait = Tile::AddUserTrait("_xbox_button", -1);
    }
    if (!(exbbuttontrait_guard & 2)) {
        exbbuttontrait_guard |= 2;
        eenabledtrait = Tile::AddUserTrait("_enabled", -1);
    }

    if (pTiles[eHealingMode] && InterfaceManager::Has360Controller()) {
        pTiles[eHealingMode]->Set(exbbuttontrait, bLimbMode ? 10 : 11);
        const char* pLabel = bLimbMode ? sStatsBody.String() : sStatsLimbs.String();
        pTiles[eHealingMode]->Set(Tile::kTileValue_String, pLabel, true);
        pTiles[eStatusButton]->Set(eenabledtrait, static_cast<int32_t>(!bLimbMode));
        pTiles[eSpecialButton]->Set(eenabledtrait, static_cast<int32_t>(!bLimbMode));
        pTiles[eSkillsButton]->Set(eenabledtrait, static_cast<int32_t>(!bLimbMode));
        pTiles[ePerksButton]->Set(eenabledtrait, static_cast<int32_t>(!bLimbMode));
        pTiles[eGeneralButton]->Set(eenabledtrait, static_cast<int32_t>(!bLimbMode));
        pTiles[eConditionButton]->Set(eenabledtrait, static_cast<int32_t>(!bLimbMode));
        pTiles[eRadsButton]->Set(eenabledtrait, static_cast<int32_t>(!bLimbMode));
        pTiles[eH20Button]->Set(eenabledtrait, static_cast<int32_t>(!bLimbMode));
        pTiles[eSLPButton]->Set(eenabledtrait, static_cast<int32_t>(!bLimbMode));
        pTiles[eFODButton]->Set(eenabledtrait, static_cast<int32_t>(!bLimbMode));
        pTiles[eEffectsButton]->Set(eenabledtrait, static_cast<int32_t>(!bLimbMode));
    }

    // Reads the flag before GetInstance.
    int32_t iOn = bLimbMode;
    BodyPartController::GetInstance()->SetFlash(iOn);
}

bool StatsMenu::IsWellFormedMenu() {
    for (int32_t i = 0; i < 60; ++i) {
        if (!this->pTiles[i]) {
            return false;
        }
    }
    return true;
}

StatsMenu::Page StatsMenu::GetCurrentPage() {
    return static_cast<StatsMenu::Page>(this->pMenu->GetFloat(0x1004 /* kTileValue_User0 */));
}

StatsMenu::StatusPage StatsMenu::GetStatusPage() {
    // The status tile is slot 5 of the tile table.
    return static_cast<StatsMenu::StatusPage>(this->pTiles[5]->GetFloat(0x1004 /* kTileValue_User0 */));
}

int32_t StatsMenu::GetTraitForValue(int32_t aeActorValue) {
    switch (aeActorValue) {
        case 0xC /* kAVCode_ActionPoints */: return 0x100A;
        case 0x10 /* kAVCode_Health */: return 0x1009;
        case 0x18 /* kAVCode_XP */: return 0x100B;
    }
    return INT32_MIN;
}

// The radiation knob shows the rads with a jitter of up to half the meter's maximum (user4, in thousands) either
// way; the rads shown are rounded to whole rads, and a fraction of one shows as 1.
void StatsMenu::UpdateRadLevel() {
    float maxRads = GetMenuTile()->GetFloat(Tile::kTileValue_User4) * 1000.0;
    double jitterBase = -maxRads / 2.0;
    float jitter = RandomFloat0To1() * maxRads + jitterBase;
    float rads = BSMAX(0.0f, static_cast<ActorValueOwner&>(*pPlayer).GetActorFloatValue(ActorValue::RADIATION_RADS));
    float knob = 1.5707963267948966 - (rads + jitter) / 1000.0 * 3.1415927410125732;
    if (rads > 0.0 && rads < 1.0) {
        rads = 1.0f;
    }
    else {
        rads = BSRoundToNearest(rads, 1.0f);
    }
    this->pMenu->Set(Tile::kTileValue_User1, rads, true);
    InterfaceManager::Instance()->GetPipboy()->MoveKnobAbsolute(FOPipboyManager::KNOB_RAD, knob, 0.0005f);
}

// The body-part cases chain into each other: each one fills the image and percentage slots only when an earlier case
// has not.
void StatsMenu::UpdateActorValue(ActorValue::Index aeWhich) {
    int32_t imageTile = NUM_TILES;
    int32_t valueTile = NUM_TILES;
    BSStringT<char> text;
    const char* format = "%d/%d";
    switch (aeWhich) {
        case ActorValue::HEALTH: {
            text.SPrintF("Interface\\Stats\\face_%02d.dds", GetFaceFileNumber());
            this->pTiles[eFaceImage]->Set(Tile::kTileValue_Filename, text.pString, true);
            int32_t base = static_cast<ActorValueOwner&>(*pPlayer).GetPermanentActorValue(ActorValue::HEALTH);
            int32_t current = static_cast<ActorValueOwner&>(*pPlayer).GetActorValue(ActorValue::HEALTH);
            text.SPrintF(format, current, base);
            int32_t trait = GetTraitForValue(ActorValue::HEALTH);
            GetMenuTile()->Set(trait, text.pString, true);
            UpdateHealingOptions(SM_HO_STIMPAK);
            break;
        }
        case ActorValue::ACTION_POINTS: {
            int32_t base = static_cast<ActorValueOwner&>(*pPlayer).GetPermanentActorValue(ActorValue::ACTION_POINTS);
            int32_t current = static_cast<ActorValueOwner&>(*pPlayer).GetActorValue(ActorValue::ACTION_POINTS);
            text.SPrintF(format, current, base);
            int32_t trait = GetTraitForValue(ActorValue::ACTION_POINTS);
            GetMenuTile()->Set(trait, text.pString, true);
            break;
        }
        case ActorValue::EXPERIENCE_POINTS: {
            int32_t level = pPlayer->GetLevel();
            if (level < static_cast<int32_t>(iMaxCharacterLevel.UInt())) {
                int32_t required = GamePlayFormulas::GetRequiredExperiencePoints(pPlayer->GetLevel() + 1);
                int32_t current = static_cast<ActorValueOwner&>(*pPlayer).GetActorValue(ActorValue::EXPERIENCE_POINTS);
                text.SPrintF(format, current, required);
            }
            else {
                text.SPrintF("%s", sStatsXPMax.String());
            }
            int32_t trait = GetTraitForValue(ActorValue::EXPERIENCE_POINTS);
            GetMenuTile()->Set(trait, text.pString, true);
            break;
        }
        case ActorValue::RADIATION_RADS:
            UpdateRadLevel();
            UpdateHealingOptions(SM_HO_RADAWAY);
            UpdateActiveEffects();
            break;
        case ActorValue::RADIATION_RESISTANCE: {
            int32_t resist =
                static_cast<ActorValueOwner&>(*pPlayer).GetClampedActorValue(ActorValue::RADIATION_RESISTANCE);
            int32_t maxResist = static_cast<int32_t>(fPlayerMaxResistance.Float());
            int32_t shown = BSMIN(maxResist, resist);
            text.SPrintF("%d%%", shown);
            this->pTiles[eRadResistLevel]->Set(Tile::kTileValue_String, text.pString, true);
            UpdateActiveEffects();
            break;
        }
        case ActorValue::PERCEPTION_CONDITION:
            imageTile = eHeadImage;
            valueTile = eHeadPct;
            [[fallthrough]];
        case ActorValue::ENDURANCE_CONDITION:
            if (imageTile == NUM_TILES) {
                imageTile = eTorsoImage;
                valueTile = eTorsoPct;
            }
            [[fallthrough]];
        case ActorValue::LEFT_ATTACK_CONDITION:
            if (imageTile == NUM_TILES) {
                imageTile = eLeftArmImage;
                valueTile = eLeftArmPct;
            }
            [[fallthrough]];
        case ActorValue::RIGHT_ATTACK_CONDITION:
            if (imageTile == NUM_TILES) {
                imageTile = eRightArmImage;
                valueTile = eRightArmPct;
            }
            [[fallthrough]];
        case ActorValue::LEFT_MOBILITY_CONDITION:
            if (imageTile == NUM_TILES) {
                imageTile = eLeftLegImage;
                valueTile = eLeftLegPct;
            }
            [[fallthrough]];
        case ActorValue::RIGHT_MOBILITY_CONDITION: {
            if (imageTile == NUM_TILES) {
                imageTile = eRightLegImage;
                valueTile = eRightLegPct;
            }
            float condition = static_cast<ActorValueOwner&>(*pPlayer).GetActorFloatValue(aeWhich) / 100.0;
            // Zero, negative and NaN conditions show the broken image.
            const char* imageFormat = condition > 0.0 ? "Interface\\Stats\\%s.dds" : "Interface\\Stats\\%s_broken.dds";
            text.SPrintF(imageFormat, pBodyPartImageNames[aeWhich - ActorValue::PERCEPTION_CONDITION]);
            this->pTiles[imageTile]->Set(Tile::kTileValue_Filename, text.pString, true);
            int32_t valueTrait = Tile::AddUserTrait("_Value", -1);
            this->pTiles[valueTile]->Set(valueTrait, condition, true);
            if (this->bLimbMode) {
                UpdateHealingOptions(SM_HO_STIMPAK);
            }
            break;
        }
        case ActorValue::KARMA: {
            float karma = static_cast<ActorValueOwner&>(*pPlayer).GetActorFloatValue(ActorValue::KARMA);
            switch (TESActorBaseData::GetAlignmentForKarma(karma)) {
                case TESActorBaseData::ALIGNMENT_GOOD:
                    this->pTiles[eKarmaImage]->Set(
                        Tile::kTileValue_Filename, "Interface\\Icons\\PipboyImages\\Karma icons\\karma_good.dds", true
                    );
                    this->pTiles[eKarmaAlignment]->Set(Tile::kTileValue_String, sAlignGood.String(), true);
                    break;
                case TESActorBaseData::ALIGNMENT_NEUTRAL:
                    this->pTiles[eKarmaImage]->Set(
                        Tile::kTileValue_Filename, "Interface\\Icons\\PipboyImages\\Karma icons\\karma_neutral.dds",
                        true
                    );
                    this->pTiles[eKarmaAlignment]->Set(Tile::kTileValue_String, sAlignNeutral.String(), true);
                    break;
                case TESActorBaseData::ALIGNMENT_EVIL:
                    this->pTiles[eKarmaImage]->Set(
                        Tile::kTileValue_Filename, "Interface\\Icons\\PipboyImages\\Karma icons\\karma_bad.dds", true
                    );
                    this->pTiles[eKarmaAlignment]->Set(Tile::kTileValue_String, sAlignEvil.String(), true);
                    break;
                case TESActorBaseData::ALIGNMENT_VERY_GOOD:
                    this->pTiles[eKarmaImage]->Set(
                        Tile::kTileValue_Filename, "Interface\\Icons\\PipboyImages\\Karma icons\\karma_saintly.dds",
                        true
                    );
                    this->pTiles[eKarmaAlignment]->Set(Tile::kTileValue_String, sAlignVeryGood.String(), true);
                    break;
                case TESActorBaseData::ALIGNMENT_VERY_EVIL:
                    this->pTiles[eKarmaImage]->Set(
                        Tile::kTileValue_Filename, "Interface\\Icons\\PipboyImages\\Karma icons\\karma_evil.dds", true
                    );
                    this->pTiles[eKarmaAlignment]->Set(Tile::kTileValue_String, sAlignVeryEvil.String(), true);
                    break;
                default: break;
            }
            UpdatePlayerTitle();
            break;
        }
        default: {
            SPECIALUpdateFn(this->lbSpecial.GetTileFor(aeWhich), aeWhich);
            SkillsUpdateFn(this->lbSkills.GetTileFor(aeWhich), aeWhich);
            break;
        }
    }
    text.Set(nullptr, 0);
}

void StatsMenu::UpdateAllMiscStats() {
    lbGeneral.Traverse(&StatsMenu::MiscStatsUpdateFn, 0, 0x7FFFFFFF);
}

// Each effect name is a temporary destroyed right after its text is read, as the game does; the first one only tests
// the text for null after the string is gone, deliberately.
void StatsMenu::UpdateActiveEffects() {
    if (this->bActiveEffectsUpdatedThisFrame) {
        return;
    }
    this->bActiveEffectsUpdatedThisFrame = true;
    BSStringT<char> text;
    this->xStatusEffects.Flush();
    StatusEffect* oldEffect = this->pRadiationEffect;
    if (oldEffect) {
        delete oldEffect;
    }
    this->pRadiationEffect = nullptr;
    oldEffect = this->pDehydrationEffect;
    if (oldEffect) {
        delete oldEffect;
    }
    this->pDehydrationEffect = nullptr;
    oldEffect = this->pSleepEffect;
    if (oldEffect) {
        delete oldEffect;
    }
    this->pSleepEffect = nullptr;
    oldEffect = this->pHungerEffect;
    if (oldEffect) {
        delete oldEffect;
    }
    this->pHungerEffect = nullptr;

    BGSRadiationStage* radiationStage = BGSRadiationStage::FindStageForRadLevel(
        static_cast<ActorValueOwner&>(*pPlayer).GetActorFloatValue(ActorValue::RADIATION_RADS)
    );
    BGSDehydrationStage* dehydrationStage = BGSDehydrationStage::FindStageForRadLevel(
        static_cast<ActorValueOwner&>(*pPlayer).GetActorFloatValue(ActorValue::DEHYDRATION)
    );
    BGSSleepDeprevationStage* sleepStage = BGSSleepDeprevationStage::FindStageForRadLevel(
        static_cast<ActorValueOwner&>(*pPlayer).GetActorFloatValue(ActorValue::SLEEPDEPREVATION)
    );
    BGSHungerStage* hungerStage = BGSHungerStage::FindStageForRadLevel(
        static_cast<ActorValueOwner&>(*pPlayer).GetActorFloatValue(ActorValue::HUNGER)
    );

    for (BSSimpleList<ActiveEffect*>* node = static_cast<MagicTarget&>(*pPlayer).GetActiveEffectList(); node;
         node = node->GetNext()) {
        ActiveEffect* activeEffect = node->GetItem();
        if (!activeEffect) {
            break;
        }
        if (!activeEffect->bActive) {
            continue;
        }
        if (activeEffect->pEffect->IsUsageMonitorEffect()) {
            continue;
        }
        if (activeEffect->pEffect->pEffectSetting->IsScriptEffect() &&
            !activeEffect->pEffect->pEffectSetting->IsFlagSet(EffectSetting::DISPLAY_EFFECT_NAME)) {
            continue;
        }
        SpellItem* spell = dynamic_cast<SpellItem*>(activeEffect->pSpell);
        if (spell && radiationStage && spell == radiationStage->GetSickness()) {
            if (!this->pRadiationEffect) {
                void* memory = ourMalloc(sizeof(StatusEffect));
                this->pRadiationEffect = memory ? ::new (memory) StatusEffect(spell) : nullptr;
            }
            if (this->pRadiationEffect->pEffectForm == spell) {
                this->pRadiationEffect->xStatsEffected.Register(activeEffect);
            }
        }
        if (spell && dehydrationStage && spell == dehydrationStage->GetSickness()) {
            if (!this->pDehydrationEffect) {
                void* memory = ourMalloc(sizeof(StatusEffect));
                this->pDehydrationEffect = memory ? ::new (memory) StatusEffect(spell) : nullptr;
            }
            if (this->pDehydrationEffect->pEffectForm == spell) {
                this->pDehydrationEffect->xStatsEffected.Register(activeEffect);
            }
        }
        if (spell && hungerStage && spell == hungerStage->GetSickness()) {
            if (!this->pHungerEffect) {
                void* memory = ourMalloc(sizeof(StatusEffect));
                this->pHungerEffect = memory ? ::new (memory) StatusEffect(spell) : nullptr;
            }
            if (this->pHungerEffect->pEffectForm == spell) {
                this->pHungerEffect->xStatsEffected.Register(activeEffect);
            }
        }
        if (spell && sleepStage && spell == sleepStage->GetSickness()) {
            if (!this->pSleepEffect) {
                void* memory = ourMalloc(sizeof(StatusEffect));
                this->pSleepEffect = memory ? ::new (memory) StatusEffect(spell) : nullptr;
            }
            if (this->pSleepEffect->pEffectForm == spell) {
                this->pSleepEffect->xStatsEffected.Register(activeEffect);
            }
        }
        // Timed effects, diseases, enchantments, addictions and conditional abilities that change a named actor
        // value are listed; other effects only when they show their name.
        if (activeEffect->pEffect->IsActorValueUsed() &&
            ActorValue::GetActorValueAbbreviation(activeEffect->pEffect->GetActorValueIndex()) &&
            (activeEffect->fDuration != 0.0 || activeEffect->eSpellType == MagicSystem::DISEASE ||
             activeEffect->eSpellType == MagicSystem::ENCHANTMENT ||
             activeEffect->eSpellType == MagicSystem::ADDICTION ||
             (activeEffect->eSpellType == MagicSystem::ABILITY &&
              (activeEffect->iFlags & ActiveEffect::HAS_CONDITIONS)))) {
            this->xStatusEffects.Register(activeEffect);
        }
        else if (activeEffect->pEffect->pEffectSetting->IsFlagSet(EffectSetting::DISPLAY_EFFECT_NAME)) {
            this->xStatusEffects.Register(activeEffect);
        }
    }

    if (this->pRadiationEffect) {
        this->pTiles[eRadEffects]->DeleteChildren();
        int32_t index = 0;
        for (BSSimpleList<EffectItemData*>* node = &this->pRadiationEffect->xStatsEffected; node && node->GetItem();
             node = node->GetNext()) {
            Tile* tile = RenderTemplate(this->pTiles[eRadEffects], "stats_rad_effects_template", nullptr);
            tile->Set(Tile::kTileValue_ListIndex, index);
            ++index;
            text = "stats_rad_effect_";
            text += ActorValue::GetActorValueAbbreviation(node->GetItem()->iActorValue);
            tile->SetName(text);
            text.SPrintF("%+d", node->GetItem()->iMagnitude);
            tile->Set(Tile::kTileValue_String, text.pString, true);
            tile->Set(
                Tile::kTileValue_User1, ActorValue::GetActorValueAbbreviation(node->GetItem()->iActorValue), true
            );
        }
        this->pTiles[eRadEffects]->Set(Tile::kTileValue_Visible, 1);
    }
    else {
        this->pTiles[eRadEffects]->Set(Tile::kTileValue_Visible, 0);
    }
    if (this->pDehydrationEffect) {
        this->pTiles[eH20Effects]->DeleteChildren();
        int32_t index = 0;
        for (BSSimpleList<EffectItemData*>* node = &this->pDehydrationEffect->xStatsEffected; node && node->GetItem();
             node = node->GetNext()) {
            Tile* tile = RenderTemplate(this->pTiles[eH20Effects], "stats_rad_effects_template", nullptr);
            tile->Set(Tile::kTileValue_ListIndex, index);
            ++index;
            text = "stats_h20_effect_";
            text += ActorValue::GetActorValueAbbreviation(node->GetItem()->iActorValue);
            tile->SetName(text);
            text.SPrintF("%+d", node->GetItem()->iMagnitude);
            tile->Set(Tile::kTileValue_String, text.pString, true);
            tile->Set(
                Tile::kTileValue_User1, ActorValue::GetActorValueAbbreviation(node->GetItem()->iActorValue), true
            );
        }
        this->pTiles[eH20Effects]->Set(Tile::kTileValue_Visible, 1);
    }
    else {
        this->pTiles[eH20Effects]->Set(Tile::kTileValue_Visible, 0);
    }
    if (this->pHungerEffect) {
        this->pTiles[eFODEffects]->DeleteChildren();
        int32_t index = 0;
        for (BSSimpleList<EffectItemData*>* node = &this->pHungerEffect->xStatsEffected; node && node->GetItem();
             node = node->GetNext()) {
            Tile* tile = RenderTemplate(this->pTiles[eFODEffects], "stats_rad_effects_template", nullptr);
            tile->Set(Tile::kTileValue_ListIndex, index);
            ++index;
            text = "stats_hunger_effect_";
            text += ActorValue::GetActorValueAbbreviation(node->GetItem()->iActorValue);
            tile->SetName(text);
            text.SPrintF("%+d", node->GetItem()->iMagnitude);
            tile->Set(Tile::kTileValue_String, text.pString, true);
            tile->Set(
                Tile::kTileValue_User1, ActorValue::GetActorValueAbbreviation(node->GetItem()->iActorValue), true
            );
        }
        this->pTiles[eFODEffects]->Set(Tile::kTileValue_Visible, 1);
    }
    else {
        this->pTiles[eFODEffects]->Set(Tile::kTileValue_Visible, 0);
    }
    if (this->pSleepEffect) {
        this->pTiles[eSLPEffects]->DeleteChildren();
        int32_t index = 0;
        for (BSSimpleList<EffectItemData*>* node = &this->pSleepEffect->xStatsEffected; node && node->GetItem();
             node = node->GetNext()) {
            Tile* tile = RenderTemplate(this->pTiles[eSLPEffects], "stats_rad_effects_template", nullptr);
            tile->Set(Tile::kTileValue_ListIndex, index);
            ++index;
            text = "stats_sleep_effect_";
            text += ActorValue::GetActorValueAbbreviation(node->GetItem()->iActorValue);
            tile->SetName(text);
            text.SPrintF("%+d", node->GetItem()->iMagnitude);
            tile->Set(Tile::kTileValue_String, text.pString, true);
            tile->Set(
                Tile::kTileValue_User1, ActorValue::GetActorValueAbbreviation(node->GetItem()->iActorValue), true
            );
        }
        this->pTiles[eSLPEffects]->Set(Tile::kTileValue_Visible, 1);
    }
    else {
        this->pTiles[eSLPEffects]->Set(Tile::kTileValue_Visible, 0);
    }

    Tile* lastTile = nullptr;
    static_cast<ListBox<StatusEffect*>*>(&this->lbStatusEffects)->RemoveAll();
    for (BSSimpleList<StatusEffect*>* node = &this->xStatusEffects; node && node->GetItem(); node = node->GetNext()) {
        lastTile = this->lbStatusEffects.AddItem(node->GetItem(), nullptr, nullptr, nullptr);
        AlchemyItem* alchemy = dynamic_cast<AlchemyItem*>(node->GetItem()->pEffectForm);
        if (alchemy && static_cast<BGSEquipType*>(alchemy)->eEquipType == BGSEquipType::ET_ALCOHOL) {
            lastTile->Set(Tile::kTileValue_User0, sStatsAlcohol.String(), true);
        }
        else {
            TESFullName* fullName = dynamic_cast<TESFullName*>(node->GetItem()->pEffectForm);
            if (fullName) {
                lastTile->Set(Tile::kTileValue_User0, fullName->GetFullName(), true);
            }
            else {
                lastTile->Set(Tile::kTileValue_User0, node->GetItem()->pEffectForm->GetFormEditorID(), true);
            }
        }
        BSStringT<char> effects;
        for (BSSimpleList<EffectSetting*>* named = &node->GetItem()->xNamedEffects; named && named->GetItem();
             named = named->GetNext()) {
            const char* nameText = named->GetItem()->GetEffectName().pString;
            if (nameText) {
                text.SPrintF("%s", named->GetItem()->GetEffectName().pString);
                effects += text;
                if (named->GetNext()) {
                    effects += ", ";
                }
            }
        }
        if (effects.GetLength() && node->GetItem()->xStatsEffected.ItemsInList()) {
            effects += ", ";
        }
        for (BSSimpleList<EffectItemData*>* data = &node->GetItem()->xStatsEffected; data && data->GetItem();
             data = data->GetNext()) {
            if (data->GetItem()->iMagnitude) {
                int32_t magnitude = data->GetItem()->iMagnitude;
                text.SPrintF("%s %+d", ActorValue::GetActorValueAbbreviation(data->GetItem()->iActorValue), magnitude);
                effects += text;
                if (data->GetNext()) {
                    effects += ", ";
                }
            }
        }
        lastTile->Set(Tile::kTileValue_User1, effects.pString, true);
        effects.Set(nullptr, 0);
    }
    if (lastTile) {
        uint32_t count = this->lbStatusEffects.NumItems();
        uint32_t shown = count < 8 ? count : 8;
        float height = lastTile->GetFloat(Tile::kTileValue_Height) * static_cast<double>(shown);
        this->pTiles[eStatusEffects]->Set(Tile::kTileValue_Height, height, true);
        lastTile->Set(Tile::kTileValue_User2, 0);
    }
    else {
        this->pTiles[eStatusEffects]->Set(Tile::kTileValue_Height, 0);
    }
    this->lbStatusEffects.Synchronize();
    text.Set(nullptr, 0);
}

void StatsMenu::UpdateHealingOptions(HealingOptionID aeID) {
    if (!pPlayer) {
        return;
    }
    for (int32_t i = 0; i < SM_HO_COUNT; ++i) {
        if (aeID != i && aeID != SM_HO_COUNT) {
            continue;
        }
        BSStringT<char> text;
        if (ItemChange* const previous = this->pHealingOptionsA[i].pInventoryItem) {
            delete previous;
        }
        this->pHealingOptionsA[i].pInventoryItem = pPlayer->GetInventoryItem(this->pHealingOptionsA[i].pAlchemyItem, 0);
        bool valid = this->pHealingOptionsA[i].pfTargetConditionCallback(this->pHealingOptionsA[i]);
        this->pHealingOptionsA[i].pTile->Set(Tile::kTileValue_Target, static_cast<int32_t>(valid));
        if (this->pHealingOptionsA[i].pInventoryItem) {
            const char* label = this->pHealingOptionsA[i].pTile->GetString(Tile::kTileValue_User0);
            text.SPrintF("(%d) %s", this->pHealingOptionsA[i].pInventoryItem->GetCount(), label);
            if ((i == SM_HO_STIMPAK && !pPlayer->IsHardcore()) || i == SM_HO_DRBAG) {
                this->pTiles[eHealingMode]->Set(Tile::kTileValue_Target, 1);
            }
        }
        else {
            text = this->pHealingOptionsA[i].pTile->GetString(Tile::kTileValue_User0);
        }
        this->pHealingOptionsA[i].pTile->Set(Tile::kTileValue_String, text.pString, true);
        text.Set(nullptr, 0);
    }

    if (aeID == SM_HO_STIMPAK || aeID == SM_HO_DRBAG || aeID == SM_HO_COUNT) {
        int32_t stimpaks = pPlayer->GetInventoryObjectCount(this->pHealingOptionsA[SM_HO_STIMPAK].pAlchemyItem);
        int32_t doctorsBags = pPlayer->GetInventoryObjectCount(this->pHealingOptionsA[SM_HO_DRBAG].pAlchemyItem);
        bool canHeal = true;
        if (pPlayer->IsHardcore() && doctorsBags == 0) {
            canHeal = false;
        }
        else if (!pPlayer->IsHardcore() && doctorsBags == 0 && stimpaks == 0) {
            canHeal = false;
        }
        if (!canHeal) {
            if (this->bLimbMode) {
                ToggleHealingMode();
            }
            this->pTiles[eHealingMode]->Set(Tile::kTileValue_Target, 0);
        }
    }
}

int32_t StatsMenu::GetFaceFileNumber() {
    for (ActiveEffect*& rpItem : pPlayer->GetActiveEffectList()->ItemsUntilNull()) {
        if (rpItem->eSpellType == MagicSystem::ADDICTION && rpItem->bActive) {
            return 10;
        }
    }
    float fmax = static_cast<float>(pPlayer->GetPermanentActorValue(ActorValue::HEALTH));
    float fdmg = fmax - pPlayer->GetActorFloatValue(ActorValue::HEALTH);
    int32_t iret = static_cast<int32_t>(fdmg / fmax * 5.0);
    if (iret < 0) {
        iret = 0;
    }
    if (iret > 4) {
        return 4;
    }
    return iret;
}

void StatsMenu::UpdateAllActorValues() {
    UpdateActorValue(ActorValue::HEALTH);
    UpdateActorValue(ActorValue::ACTION_POINTS);
    UpdateActorValue(ActorValue::EXPERIENCE_POINTS);
    UpdateActorValue(ActorValue::RADIATION_RADS);
    UpdateActorValue(ActorValue::RADIATION_RESISTANCE);
    for (int32_t i = ActorValue::PERCEPTION_CONDITION; i <= ActorValue::BRAIN_CONDITION; ++i) {
        UpdateActorValue(static_cast<ActorValue::Index>(i));
    }
    this->pTiles[eSpecialRect]->Set(Tile::kTileValue_User0, 0);
    this->lbSpecial.Traverse(&StatsMenu::SPECIALUpdateFn, 0, INT32_MAX);
    this->pTiles[eSkillsRect]->Set(Tile::kTileValue_User0, 0);
    this->lbSkills.Traverse(&StatsMenu::SkillsUpdateFn, 0, INT32_MAX);
    UpdateActorValue(ActorValue::KARMA);
}

void StatsMenu::SetCurrentPage(StatsMenu::Page aePage) {
    if (aePage < SM_FIRST_PAGE || aePage > SM_LAST_PAGE) {
        return;
    }
    float burstIntensity = fMiniBurstIntensity.Float();
    float burstDuration = fMiniBurstDuration.Float();
    InterfaceManager::Instance()->GetPipboy()->TriggerBurst(burstDuration, burstIntensity, 0.5f);
    this->pMenu->Set(Tile::kTileValue_User0, aePage);
    if (InterfaceManager::Instance()->GetPipboy()->GetInitialized()) {
        SetKnobPosition(true);
    }
    if (this->bLimbMode) {
        ToggleHealingMode();
    }
    if (aePage == SM_SPECIAL) {
        SetIconAndDescription(ActorValue::STRENGTH);
    }
    else if (aePage == SM_SKILLS) {
        SetIconAndDescription(ActorValue::BARTER);
    }
    else if (aePage == SM_PERKS && this->lbPerks.NumUnfiltered()) {
        SetIconAndDescription(this->lbPerks.GetDataAt(0, false));
    }
    else {
        this->pTiles[eItemIcon]->Set(Tile::kTileValue_Visible, 0);
    }
}

void StatsMenu::UpdateH20Level() {
    pMenu->GetFloat(Tile::kTileValue_User4);
    RandomFloat0To1();
    float a2 = pPlayer->GetActorFloatValue(ActorValue::DEHYDRATION);
    float fh20 = BSMAX(static_cast<float>(0.0), a2);
    if (!(fh20 > 0.0) || !(fh20 < 1.0)) {
        float fh20_2 = BSRoundToNearest(fh20, 1.0f);
        pMenu->Set(Tile::kTileValue_User11, fh20_2, true);
    }
    else {
        pMenu->Set(Tile::kTileValue_User11, static_cast<float>(1.0), true);
    }
}

void StatsMenu::UpdateFODLevel() {
    pMenu->GetFloat(Tile::kTileValue_User4);
    RandomFloat0To1();
    float a2 = pPlayer->GetActorFloatValue(ActorValue::HUNGER);
    float fh20 = BSMAX(static_cast<float>(0.0), a2);
    if (!(fh20 > 0.0) || !(fh20 < 1.0)) {
        float fh20_2 = BSRoundToNearest(fh20, 1.0f);
        pMenu->Set(Tile::kTileValue_User12, fh20_2, true);
    }
    else {
        pMenu->Set(Tile::kTileValue_User12, static_cast<float>(1.0), true);
    }
}

void StatsMenu::UpdateSLPLevel() {
    pMenu->GetFloat(Tile::kTileValue_User4);
    RandomFloat0To1();
    float a2 = pPlayer->GetActorFloatValue(ActorValue::SLEEPDEPREVATION);
    float fh20 = BSMAX(static_cast<float>(0.0), a2);
    if (!(fh20 > 0.0) || !(fh20 < 1.0)) {
        float fh20_2 = BSRoundToNearest(fh20, 1.0f);
        pMenu->Set(Tile::kTileValue_User13, fh20_2, true);
    }
    else {
        pMenu->Set(Tile::kTileValue_User13, static_cast<float>(1.0), true);
    }
}

void StatsMenu::ToggleHealingMode() {
    bLimbMode = !bLimbMode;
    bool v1 = StatsMenu::StimpakTargetConditionCallback(*pHealingOptionsA);
    pHealingOptionsA[0].pTile->Set(4015, static_cast<int32_t>(v1));
    bool v2 = DrBagTargetConditionCallback(pHealingOptionsA[3]);
    pHealingOptionsA[3].pTile->Set(4015, static_cast<int32_t>(v2));
    UpdateHealingMode();
}

bool StatsMenu::SetStatusPage(int32_t aiPage) {
    if (aiPage < 0 || aiPage > (pPlayer->IsHardcore() ? SM_LAST_STATUS_PAGE_HARDCORE : SM_LAST_STATUS_PAGE) ||
        GetStatusPage() == aiPage) {
        return false;
    }
    if (bLimbMode) {
        ToggleHealingMode();
    }
    pTiles[eStatusRect]->Set(Tile::kTileValue_User0, aiPage);
    return true;
}

void StatsMenu::SetIconAndDescription(ActorValue::Index aeIndex) {
    Tile* pDescText = pTiles[eItemDesc]->xChildren.GetHead();
    Tile* pScrollBar = pTiles[eItemDesc]->xChildren.GetTail();
    float fScrollDelta = pScrollBar->GetFloat(Tile::TextToTrait("_ScrollDelta"));
    pTiles[eItemIcon]->Set(Tile::kTileValue_Visible, 1);
    pTiles[eItemIcon]->Set(Tile::kTileValue_Filename, ActorValue::GetActorValueIcon(aeIndex), true);
    pDescText->Set(Tile::kTileValue_String, ActorValue::GetActorValueDesc(aeIndex), true);
    const char* pBadge = ActorValue::GetItemWeaponTypeBadge(aeIndex);
    if (pBadge) {
        pTiles[eSkillBadgeImage]->Set(Tile::kTileValue_Visible, 1);
        pTiles[eSkillBadgeImage]->Set(Tile::kTileValue_Filename, pBadge, true);
    }
    else {
        pTiles[eSkillBadgeImage]->Set(Tile::kTileValue_Visible, 0);
    }
    pDescText->UpdateTile(false);
    double dHeight = pDescText->GetFloat(Tile::kTileValue_Height);
    float fOverflow = static_cast<float>(dHeight - pDescText->GetParent()->GetFloat(Tile::kTileValue_Height));
    float fClamped = BSMAX(0.0f, fOverflow);
    float fSteps = fClamped / fScrollDelta;
    float fItems = static_cast<float>(std::ceil(fSteps) + 1.0);
    pScrollBar->Set(Tile::TextToTrait("_number_of_items"), fItems, true);
    pScrollBar->Set(Tile::TextToTrait("_current_value"), 0.0f, false);
    pScrollBar->Poke(Tile::TextToTrait("_SetInCode"), 1.0f);
}

void StatsMenu::SetIconAndDescription(PerkRankData* apPerkRankData) {
    Tile* pdescText = pTiles[26]->xChildren.GetHead();
    Tile* pscrollbar = pTiles[26]->xChildren.GetTail();
    int32_t v2 = Tile::TextToTrait("_ScrollDelta");
    float fscrollDelta = pscrollbar->GetFloat(v2);
    pTiles[25]->Set(4003, 1);
    pTiles[25]->Set(4044, apPerkRankData->pPerk->GetTextureName(), true);
    const char* v3 = apPerkRankData->pPerk->GetDescription(nullptr, DESC_ID);
    pdescText->Set(4036, v3, true);
    pdescText->UpdateTile(false);
    double Float = pdescText->GetFloat(4016);
    float fheightDifference = Float - pdescText->pParent->GetFloat(4016);
    float fheightDifference_2 = BSMAX(0.0f, fheightDifference);
    float v8 = fheightDifference_2 / fscrollDelta;
    float v7 = std::ceil(v8) + 1.0;
    int32_t v4 = Tile::TextToTrait("_number_of_items");
    pscrollbar->Set(v4, v7, true);
    int32_t v5 = Tile::TextToTrait("_current_value");
    pscrollbar->Set(v5, 0.0f, false);
    int32_t v6 = Tile::TextToTrait("_SetInCode");
    pscrollbar->Poke(v6, 1.0);
}

void StatsMenu::SetReputation(TESReputation* pRep) {
    if (pRep) {
        const char* ReputationMainIcon = pRep->GetReputationMainIcon();
        pTiles[47]->Set(4044, ReputationMainIcon, true);
        pTiles[47]->Set(4003, 1);
        pTiles[48]->Set(4003, 1);
        pTiles[51]->Set(4003, 1);
        pTiles[51]->Set(4036, pRep->GetFullName(), true);
        const char* ReputationTitle = pRep->GetReputationTitle();
        pTiles[48]->Set(4036, ReputationTitle, true);
    }
}

uint32_t StatsMenu::MoveKnob(Tile* apTarget, uint32_t& aiLastIndex) {
    uint32_t Float = static_cast<int64_t>(apTarget->GetFloat(4012));
    if (aiLastIndex >= Float) {
        if (aiLastIndex > Float) {
            float afRatea = fScrollKnobRate.Float();
            float afAmounta = -fScrollKnobIncrement.Float();
            InterfaceManager* v5 = InterfaceManager::Instance();
            FOPipboyManager* Pipboy = v5->GetPipboy();
            Pipboy->MoveKnob(FOPipboyManager::KNOB_SCROLL, afAmounta, afRatea, true);
        }
    }
    else {
        float afRate = fScrollKnobRate.Float();
        float afAmount = fScrollKnobIncrement.Float();
        InterfaceManager* v3 = InterfaceManager::Instance();
        FOPipboyManager* v4 = v3->GetPipboy();
        v4->MoveKnob(FOPipboyManager::KNOB_SCROLL, afAmount, afRate, true);
    }
    aiLastIndex = Float;
    return Float;
}

void StatsMenu::UpdateMiscStat(MiscStatManager::MiscStatID aeID) {
    StatsMenu::MiscStatsUpdateFn(lbGeneral.GetTileFor(aeID), aeID);
}

void StatsMenu::UpdateAll() {
    UpdateAllActorValues();
    UpdateAllMiscStats();
    UpdateReputations();
    UpdateHealingOptions(StatsMenu::SM_HO_COUNT);
    TargetMostDamagedBodyPart();
    const char* v1 = sLanguage.String();
    if (BSstrcmp(v1, "ENGLISH")) {
        const char* v2 = sStatsStatus.String();
        pTiles[0]->Set(4036, v2, true);
        const char* v3 = sStatsSpecial.String();
        pTiles[1]->Set(4036, v3, true);
        const char* v4 = sStatsSkills.String();
        pTiles[2]->Set(4036, v4, true);
        const char* v5 = sStatsPerks.String();
        pTiles[3]->Set(4036, v5, true);
        const char* v6 = sStatsGeneral.String();
        pTiles[4]->Set(4036, v6, true);
        const char* v7 = sStatsCNDAbbrev.String();
        pTiles[28]->Set(4036, v7, true);
        const char* v8 = sStatsRADAbbrev.String();
        pTiles[29]->Set(4036, v8, true);
        const char* v9 = sStatsH20Abbrev.String();
        pTiles[53]->Set(4036, v9, true);
        const char* v10 = sStatsSLPAbbrev.String();
        pTiles[57]->Set(4036, v10, true);
        const char* v11 = sStatsFODAbbrev.String();
        pTiles[55]->Set(4036, v11, true);
        const char* v12 = sStatsCrippled.String();
        pMenu->Set(Tile::kTileValue_User9, v12, true);
        const char* v13 = sStatsEFFAbbrev.String();
        pMenu->Set(Tile::kTileValue_User10, v13, true);
        const char* v25 = sStats.String();
        Tile* ChildByName = pMenu->GetChildByName("stats_title");
        ChildByName->Set(4036, v25, true);
        const char* v26 = sStatsLVLAbbrev.String();
        int32_t v21 = Tile::TextToTrait("_Title");
        Tile* v15 = pMenu->GetChildByName("stats_lvl_info");
        v15->Set(v21, v26, true);
        const char* v27 = sStatsHP.String();
        int32_t v22 = Tile::TextToTrait("_Title");
        Tile* v16 = pMenu->GetChildByName("stats_hp_info");
        v16->Set(v22, v27, true);
        const char* v28 = sStatsAP.String();
        int32_t v23 = Tile::TextToTrait("_Title");
        Tile* v17 = pMenu->GetChildByName("stats_ap_info");
        v17->Set(v23, v28, true);
        const char* v29 = sStatsXP.String();
        int32_t v24 = Tile::TextToTrait("_Title");
        Tile* v18 = pMenu->GetChildByName("stats_xp_info");
        v18->Set(v24, v29, true);
        const char* v30 = sStatsRadResist.String();
        Tile* v19 = pMenu->GetChildByName("stats_rad_resist_label");
        v19->Set(4036, v30, true);
        const char* v31 = sStatsNoEffects.String();
        Tile* v20 = pMenu->GetChildByName("stats_no_effects");
        v20->Set(4036, v31, true);
    }
}

void StatsMenu::UpdateReputations() {
    if ((StatsMenu::pMe) && (StatsMenu::pMe->pTiles[46])) {
        pTiles[47]->Set(4003, 0);
        pTiles[51]->Set(4003, 0);
        pTiles[48]->Set(4003, 0);
        static_cast<ListBox<TESReputation*>*>(&StatsMenu::pMe->lbReputation)->RemoveAll();
        for (TESReputation* pRepItem : (&pDataHandler->listReputations)->ItemsUntilNull()) {
            if ((pRepItem) &&
                (pRepItem->GetReputationValue(REPUTATION_POSITIVE) > 0.0 ||
                 pRepItem->GetReputationValue(REPUTATION_NEGATIVE) > 0.0)) {
                BSStringT<char> strtmp;
                strtmp.SPrintF("%s", pRepItem->GetFullName());
                char* v1 = strtmp.pString;
                StatsMenu::pMe->lbReputation.AddItem(pRepItem, v1, &StatsMenu::ReputationListSortFn, nullptr);
            }
        }
    }
}

void StatsMenu::TargetMostDamagedBodyPart() {
    StatsMenu::BodyPartController::BodyPartIndex ebpi;
    StatsMenu::BodyPartController::BodyPartIndex ewinner = StatsMenu::BodyPartController::SM_BODYPART_COUNT;
    float flowVal = 3.4028235e+38f;
    for (ebpi = StatsMenu::BodyPartController::SM_HEAD; ebpi < StatsMenu::BodyPartController::SM_BODYPART_COUNT;
         ++ebpi) {
        ActorValue::Index v1 = StatsMenu::BodyPartController::Convert(ebpi);
        float fval = pPlayer->GetActorFloatValue(v1);
        if (flowVal > static_cast<double>(fval)) {
            flowVal = fval;
            ewinner = ebpi;
        }
    }
    bool bLimbMode = this->bLimbMode;
    StatsMenu::BodyPartController* Instance = StatsMenu::BodyPartController::GetInstance();
    Instance->SetCurrentBodyPart(ewinner, bLimbMode);
}

bool StatsMenu::GetPageFor(StatsMenu::TileIndex aeIndex, StatsMenu::Page& aePage) {
    bool result;
    switch (aeIndex) {
        case StatsMenu::eStatusButton: {
            aePage = StatsMenu::SM_STATUS;
            result = true;
            break;
        }
        case StatsMenu::eSpecialButton: {
            aePage = StatsMenu::SM_SPECIAL;
            result = true;
            break;
        }
        case StatsMenu::eSkillsButton: {
            aePage = StatsMenu::SM_SKILLS;
            result = true;
            break;
        }
        case StatsMenu::ePerksButton: {
            aePage = StatsMenu::SM_PERKS;
            result = true;
            break;
        }
        case StatsMenu::eGeneralButton: {
            aePage = StatsMenu::SM_GENERAL;
            result = true;
            break;
        }
        default: {
            result = false;
            break;
        }
    }
    return result;
}

bool StatsMenu::GetStatusPageFor(StatsMenu::TileIndex aeIndex, StatsMenu::StatusPage& aePage) {
    bool result;
    switch (aeIndex) {
        case StatsMenu::eConditionButton: {
            aePage = StatsMenu::SM_STATUS_CONDITION;
            result = true;
            break;
        }
        case StatsMenu::eRadsButton: {
            aePage = StatsMenu::SM_STATUS_RADS;
            result = true;
            break;
        }
        case StatsMenu::eEffectsButton: {
            aePage = StatsMenu::SM_STATUS_EFFECTS;
            result = true;
            break;
        }
        case StatsMenu::eH20Button: {
            aePage = StatsMenu::SM_STATUS_H20;
            result = true;
            break;
        }
        case StatsMenu::eFODButton: {
            aePage = StatsMenu::SM_STATUS_FOD;
            result = true;
            break;
        }
        case StatsMenu::eSLPButton: {
            aePage = StatsMenu::SM_STATUS_SLP;
            result = true;
            break;
        }
        default: {
            result = false;
            break;
        }
    }
    return result;
}

void StatsMenu::ToggleGenRepMode() {
    static int32_t eenabledtrait = Tile::AddUserTrait("_enabled", -1);
    static int32_t euser0 = Tile::AddUserTrait("_user0", -1);
    g_StatsMenuIsShowGeneralNotReputation = g_StatsMenuIsShowGeneralNotReputation == 0;
    StatsMenu::pMe->pTiles[46]->Set(eenabledtrait, static_cast<int32_t>(g_StatsMenuIsShowGeneralNotReputation));
    StatsMenu::pMe->pTiles[46]->Set(4003, static_cast<int32_t>(g_StatsMenuIsShowGeneralNotReputation));
    StatsMenu::pMe->pTiles[8]->Set(eenabledtrait, static_cast<int32_t>(g_StatsMenuIsShowGeneralNotReputation == 0));
    StatsMenu::pMe->pTiles[8]->Set(4003, static_cast<int32_t>(g_StatsMenuIsShowGeneralNotReputation == 0));
    if (g_StatsMenuIsShowGeneralNotReputation) {
        const char* v1 = sStatsGeneral.String();
        StatsMenu::pMe->pTiles[49]->Set(4036, v1, true);
        StatsMenu::pMe->pTiles[50]->Set(Tile::kTileValue_User0, 1);
    }
    else {
        const char* v2 = sReputationButton.String();
        StatsMenu::pMe->pTiles[49]->Set(4036, v2, true);
        StatsMenu::pMe->pTiles[50]->Set(Tile::kTileValue_User0, 0);
    }
}

int32_t StatsMenu::GetClassStatic() {
    return 1003;
}

// Each step rereads pMe, as the game does. The tile pointer of each AddFlash is read before GetInstance. The RadX
// option gets no condition callback.
Tile* StatsMenu::Create() {
    using enum Tile::eTileValue;
    using BPC = BodyPartController;

    Tile* pOldMenu = Menu::GetMenuByClass(GetClassStatic());
    if (pOldMenu) {
        delete pOldMenu;
    }
    InterfaceManager* pInterfaceManager = InterfaceManager::Instance();
    const float fMaxDepth = Tile::GetMaximumDepth();
    Tile* pFile = pInterfaceManager->GetMenusRoot()->ReadFile("Data\\Menus\\Main\\stats_menu.xml");
    StatsMenu::pMe = static_cast<StatsMenu*>(pFile->GetMenu());
    Menu::RemoveFromXMLArchive("Data\\Menus\\Main\\stats_menu.xml", StatsMenu::pMe);
    StatsMenu::pMe->SetRenderedMenu();
    if (!StatsMenu::pMe || StatsMenu::pMe->GetClass() != GetClassStatic()) {
        if (StatsMenu::pMe && StatsMenu::pMe->GetMenuTile()) {
            delete StatsMenu::pMe;
        }
        return nullptr;
    }
    StatsMenu::pMe->SetMenuTile(static_cast<TileMenu*>(pFile), false);
    if (!StatsMenu::pMe->IsWellFormedMenu()) {
        Warning("MENUS: Stats Menu Creation Failed... Are your menu and art resources up to date?");
        return nullptr;
    }
    if (pFile->GetFloat(kTileValue_StackingType) == 6006.0 || pFile->GetFloat(kTileValue_StackingType) == 102.0) {
        pFile->Set(kTileValue_Depth, fMaxDepth, true);
    }
    const int32_t iMenuClass = GetClassStatic();
    InterfaceManager::Instance()->xTutorialManager.ShowMessage(Interface::TUT_PIPBOY_STATS, iMenuClass, 0x200);
    UpdatePlayerName();

    // The radiation meter's maximum: the rads of the highest stage, or 1000 without stages.
    BGSRadiationStage* pStage = BGSRadiationStage::FindStageForRadLevel(3.4028234663852886e+38f);
    const uint32_t uiRadMeterMax = pStage ? pStage->data.iRads : 1000u;
    pFile->Set(kTileValue_User2, uiRadMeterMax);

    BSStringT<char> text;
    const char* format = "%d/%d";
    int32_t iBase = static_cast<ActorValueOwner&>(*pPlayer).GetPermanentActorValue(ActorValue::HEALTH);
    int32_t iCurrent = static_cast<ActorValueOwner&>(*pPlayer).GetActorValue(ActorValue::HEALTH);
    text.SPrintF(format, iCurrent, iBase);
    pFile->Set(kTileValue_User5, text.pString, true);
    iBase = static_cast<ActorValueOwner&>(*pPlayer).GetPermanentActorValue(ActorValue::ACTION_POINTS);
    iCurrent = static_cast<ActorValueOwner&>(*pPlayer).GetActorValue(ActorValue::ACTION_POINTS);
    text.SPrintF(format, iCurrent, iBase);
    pFile->Set(kTileValue_User6, text.pString, true);
    const int32_t iLevel = pPlayer->GetLevel();
    if (iLevel < static_cast<int32_t>(iMaxCharacterLevel.UInt())) {
        const int32_t iRequired = GamePlayFormulas::GetRequiredExperiencePoints(pPlayer->GetLevel() + 1);
        iCurrent = static_cast<ActorValueOwner&>(*pPlayer).GetActorValue(ActorValue::EXPERIENCE_POINTS);
        text.SPrintF(format, iCurrent, iRequired);
    }
    else {
        text.SPrintF("%s", sStatsXPMax.String());
    }
    pFile->Set(kTileValue_User7, text.pString, true);
    const int32_t iShownLevel = pPlayer->GetLevel();
    pFile->Set(kTileValue_User8, iShownLevel);

    // The SPECIAL (section 0) and skill (section 2) lists, without deprecated values.
    text = "stats_list_template";
    StatsMenu::pMe->lbSpecial.Initialize(StatsMenu::pMe->pTiles[eSpecialRect], text.pString);
    for (uint32_t i = 0; i < ActorValue::GetSectionCount(ActorValue::ATTRIBUTE); ++i) {
        const int32_t iValue = ActorValue::ToActorValue(ActorValue::ATTRIBUTE, static_cast<int8_t>(i));
        if (!ActorValue::IsFlagSet(static_cast<ActorValue::Index>(iValue), ActorValue::DEPRECATED) &&
            iValue != ActorValue::BRAIN_CONDITION) {
            Tile* pTile =
                StatsMenu::pMe->lbSpecial.AddItem(static_cast<ActorValue::Index>(iValue), nullptr, nullptr, nullptr);
            pTile->Set(kTileValue_String, ActorValue::GetActorValueName(static_cast<ActorValue::Index>(iValue)), true);
            pTile->SetName(ActorValue::GetActorValueName(static_cast<ActorValue::Index>(iValue)));
        }
    }
    StatsMenu::pMe->lbSkills.Initialize(StatsMenu::pMe->pTiles[eSkillsRect], text.pString);
    for (uint32_t i = 0; i < ActorValue::GetSectionCount(ActorValue::SKILL); ++i) {
        const int32_t iValue = ActorValue::ToActorValue(ActorValue::SKILL, static_cast<int8_t>(i));
        if (!ActorValue::IsFlagSet(static_cast<ActorValue::Index>(iValue), ActorValue::DEPRECATED) &&
            iValue != ActorValue::BRAIN_CONDITION) {
            Tile* pTile =
                StatsMenu::pMe->lbSkills.AddItem(static_cast<ActorValue::Index>(iValue), nullptr, nullptr, nullptr);
            pTile->Set(kTileValue_String, ActorValue::GetActorValueName(static_cast<ActorValue::Index>(iValue)), true);
            pTile->SetName(ActorValue::GetActorValueName(static_cast<ActorValue::Index>(iValue)));
        }
    }
    static_cast<ListBox<ActorValue::Index>*>(&StatsMenu::pMe->lbSkills)->Sort(&ActorValue::CompareForSorting);
    StatsMenu::pMe->lbPerks.Initialize(StatsMenu::pMe->pTiles[ePerksRect], text.pString);
    UpdatePerks();
    StatsMenu::pMe->lbReputation.Initialize(StatsMenu::pMe->pTiles[eReputationRect], text.pString);
    StatsMenu::pMe->lbGeneral.Initialize(StatsMenu::pMe->pTiles[eGeneralRect], text.pString);
    // Every statistic before MS_EQUIPMENT_DAMAGE_REPAIRED.
    for (MiscStatManager::MiscStatID eStat = MiscStatManager::MS_QUESTS_COMPLETED;
         eStat < MiscStatManager::MS_EQUIPMENT_DAMAGE_REPAIRED; ++eStat) {
        Tile* pTile = StatsMenu::pMe->lbGeneral.AddItem(eStat, nullptr, nullptr, nullptr);
        const char* pName = MiscStatManager::GetDisplayText(eStat);
        pTile->Set(kTileValue_String, pName, true);
        pTile->Set(kTileValue_User1, MiscStatManager::GetVal(eStat));
        pTile->SetName(pName);
    }
    StatsMenu::pMe->pTiles[eReputationIcon]->Set(kTileValue_Filename, "Interface\\shared\\solid_black.dds", true);
    StatsMenu::pMe->pTiles[eReputationIcon]->Set(kTileValue_Visible, 0);

    // The healing options, in order: stimpak, doctor's bag, RadX, RadAway.
    TESForm* pItem = BGSDefaultObjectManager::GetDefaultObject(DO_STIMPAK);
    StatsMenu::pMe->pHealingOptionsA[SM_HO_STIMPAK].pAlchemyItem = dynamic_cast<AlchemyItem*>(pItem);
    StatsMenu::pMe->pHealingOptionsA[SM_HO_STIMPAK].pTile = StatsMenu::pMe->pTiles[eStimpakButton];
    TESFullName* pFullName = dynamic_cast<TESFullName*>(pItem);
    StatsMenu::pMe->pHealingOptionsA[SM_HO_STIMPAK].pTile->Set(kTileValue_User0, pFullName->GetFullName(), true);
    StatsMenu::pMe->pHealingOptionsA[SM_HO_STIMPAK].pfTargetConditionCallback =
        &StatsMenu::StimpakTargetConditionCallback;
    pItem = BGSDefaultObjectManager::GetDefaultObject(DO_DOCTOR_BAG);
    StatsMenu::pMe->pHealingOptionsA[SM_HO_DRBAG].pAlchemyItem = dynamic_cast<AlchemyItem*>(pItem);
    StatsMenu::pMe->pHealingOptionsA[SM_HO_DRBAG].pTile = StatsMenu::pMe->pTiles[eDrBagButton];
    pFullName = dynamic_cast<TESFullName*>(pItem);
    StatsMenu::pMe->pHealingOptionsA[SM_HO_DRBAG].pTile->Set(kTileValue_User0, pFullName->GetFullName(), true);
    StatsMenu::pMe->pHealingOptionsA[SM_HO_DRBAG].pfTargetConditionCallback = &DrBagTargetConditionCallback;
    pItem = BGSDefaultObjectManager::GetDefaultObject(DO_RADX);
    StatsMenu::pMe->pHealingOptionsA[SM_HO_RADX].pAlchemyItem = dynamic_cast<AlchemyItem*>(pItem);
    StatsMenu::pMe->pHealingOptionsA[SM_HO_RADX].pTile = StatsMenu::pMe->pTiles[eRadXButton];
    pFullName = dynamic_cast<TESFullName*>(pItem);
    StatsMenu::pMe->pHealingOptionsA[SM_HO_RADX].pTile->Set(kTileValue_User0, pFullName->GetFullName(), true);
    pItem = BGSDefaultObjectManager::GetDefaultObject(DO_RADAWAY);
    StatsMenu::pMe->pHealingOptionsA[SM_HO_RADAWAY].pAlchemyItem = dynamic_cast<AlchemyItem*>(pItem);
    StatsMenu::pMe->pHealingOptionsA[SM_HO_RADAWAY].pTile = StatsMenu::pMe->pTiles[eRadAwayButton];
    pFullName = dynamic_cast<TESFullName*>(pItem);
    StatsMenu::pMe->pHealingOptionsA[SM_HO_RADAWAY].pTile->Set(kTileValue_User0, pFullName->GetFullName(), true);
    StatsMenu::pMe->pHealingOptionsA[SM_HO_RADAWAY].pfTargetConditionCallback =
        &StatsMenu::RadAwayTargetConditionCallback;
    StatsMenu::pMe->lbStatusEffects.Initialize(StatsMenu::pMe->pTiles[eStatusEffects], "stats_effects_template");

    // The tiles that flash for each body part, then the limb selector's moves between body parts.
    Tile* pTile = StatsMenu::pMe->pTiles[eHeadImage];
    BPC::GetInstance()->AddFlash(BPC::SM_HEAD, pTile);
    pTile = StatsMenu::pMe->pTiles[eHeadPct];
    BPC::GetInstance()->AddFlash(BPC::SM_HEAD, pTile);
    pTile = StatsMenu::pMe->pTiles[eFaceImage];
    BPC::GetInstance()->AddFlash(BPC::SM_HEAD, pTile);
    pTile = StatsMenu::pMe->pTiles[eRightArmImage];
    BPC::GetInstance()->AddFlash(BPC::SM_RIGHTARM, pTile);
    pTile = StatsMenu::pMe->pTiles[eRightArmPct];
    BPC::GetInstance()->AddFlash(BPC::SM_RIGHTARM, pTile);
    pTile = StatsMenu::pMe->pTiles[eTorsoImage];
    BPC::GetInstance()->AddFlash(BPC::SM_TORSO, pTile);
    pTile = StatsMenu::pMe->pTiles[eTorsoPct];
    BPC::GetInstance()->AddFlash(BPC::SM_TORSO, pTile);
    pTile = StatsMenu::pMe->pTiles[eLeftArmImage];
    BPC::GetInstance()->AddFlash(BPC::SM_LEFTARM, pTile);
    pTile = StatsMenu::pMe->pTiles[eLeftArmPct];
    BPC::GetInstance()->AddFlash(BPC::SM_LEFTARM, pTile);
    pTile = StatsMenu::pMe->pTiles[eRightLegImage];
    BPC::GetInstance()->AddFlash(BPC::SM_RIGHTLEG, pTile);
    pTile = StatsMenu::pMe->pTiles[eRightLegPct];
    BPC::GetInstance()->AddFlash(BPC::SM_RIGHTLEG, pTile);
    pTile = StatsMenu::pMe->pTiles[eLeftLegImage];
    BPC::GetInstance()->AddFlash(BPC::SM_LEFTLEG, pTile);
    pTile = StatsMenu::pMe->pTiles[eLeftLegPct];
    BPC::GetInstance()->AddFlash(BPC::SM_LEFTLEG, pTile);
    BPC::GetInstance()->LinkBodyParts(BPC::SM_HEAD, BPC::DIR_DOWN, BPC::SM_TORSO);
    BPC::GetInstance()->LinkBodyParts(BPC::SM_HEAD, BPC::DIR_LEFT, BPC::SM_RIGHTARM);
    BPC::GetInstance()->LinkBodyParts(BPC::SM_HEAD, BPC::DIR_RIGHT, BPC::SM_LEFTARM);
    BPC::GetInstance()->LinkBodyParts(BPC::SM_TORSO, BPC::DIR_UP, BPC::SM_HEAD);
    BPC::GetInstance()->LinkBodyParts(BPC::SM_TORSO, BPC::DIR_DOWN, BPC::SM_RIGHTLEG);
    BPC::GetInstance()->LinkBodyParts(BPC::SM_TORSO, BPC::DIR_LEFT, BPC::SM_RIGHTARM);
    BPC::GetInstance()->LinkBodyParts(BPC::SM_TORSO, BPC::DIR_RIGHT, BPC::SM_LEFTARM);
    BPC::GetInstance()->LinkBodyParts(BPC::SM_RIGHTARM, BPC::DIR_UP, BPC::SM_HEAD);
    BPC::GetInstance()->LinkBodyParts(BPC::SM_RIGHTARM, BPC::DIR_DOWN, BPC::SM_RIGHTLEG);
    BPC::GetInstance()->LinkBodyParts(BPC::SM_RIGHTARM, BPC::DIR_RIGHT, BPC::SM_TORSO);
    BPC::GetInstance()->LinkBodyParts(BPC::SM_LEFTARM, BPC::DIR_UP, BPC::SM_HEAD);
    BPC::GetInstance()->LinkBodyParts(BPC::SM_LEFTARM, BPC::DIR_DOWN, BPC::SM_LEFTLEG);
    BPC::GetInstance()->LinkBodyParts(BPC::SM_LEFTARM, BPC::DIR_LEFT, BPC::SM_TORSO);
    BPC::GetInstance()->LinkBodyParts(BPC::SM_RIGHTLEG, BPC::DIR_UP, BPC::SM_TORSO);
    BPC::GetInstance()->LinkBodyParts(BPC::SM_RIGHTLEG, BPC::DIR_RIGHT, BPC::SM_LEFTLEG);
    BPC::GetInstance()->LinkBodyParts(BPC::SM_LEFTLEG, BPC::DIR_UP, BPC::SM_TORSO);
    BPC::GetInstance()->LinkBodyParts(BPC::SM_LEFTLEG, BPC::DIR_LEFT, BPC::SM_RIGHTLEG);
    StatsMenu::pMe->UpdateAllActorValues();
    StatsMenu::pMe->UpdateHealingMode();

    // The labels: the meters' names upper-cased on the menu tile, then the titles and buttons.
    text = sHUDRads.String();
    text.MakeUpper();
    int32_t iTrait = Tile::AddUserTrait("_rads", -1);
    pFile->Set(iTrait, text.pString, true);
    text = sStatsH20Abbrev.String();
    text.MakeUpper();
    iTrait = Tile::AddUserTrait("_h20", -1);
    pFile->Set(iTrait, text.pString, true);
    text = sStatsFODAbbrev.String();
    text.MakeUpper();
    iTrait = Tile::AddUserTrait("_hunger", -1);
    pFile->Set(iTrait, text.pString, true);
    text = sStatsSLPAbbrev.String();
    text.MakeUpper();
    iTrait = Tile::AddUserTrait("_sleep", -1);
    pFile->Set(iTrait, text.pString, true);
    StatsMenu::pMe->pTiles[eTitle]->Set(kTileValue_String, sStats.String(), true);
    const char* pLabel = sStatsLVLAbbrev.String();
    iTrait = Tile::AddUserTrait("_Title", -1);
    StatsMenu::pMe->pTiles[eLevelInfo]->Set(iTrait, pLabel, true);
    pLabel = sStatsHP.String();
    iTrait = Tile::AddUserTrait("_Title", -1);
    StatsMenu::pMe->pTiles[eHPInfo]->Set(iTrait, pLabel, true);
    pLabel = sStatsAP.String();
    iTrait = Tile::AddUserTrait("_Title", -1);
    StatsMenu::pMe->pTiles[eAPInfo]->Set(iTrait, pLabel, true);
    pLabel = sStatsXP.String();
    iTrait = Tile::AddUserTrait("_Title", -1);
    StatsMenu::pMe->pTiles[eXPInfo]->Set(iTrait, pLabel, true);
    StatsMenu::pMe->pTiles[eConditionButton]->Set(kTileValue_String, sStatsCNDAbbrev.String(), true);
    StatsMenu::pMe->pTiles[eRadsButton]->Set(kTileValue_String, sStatsRADAbbrev.String(), true);
    pFile->Set(kTileValue_User9, sStatsCrippled.String(), true);
    pFile->Set(kTileValue_User10, sStatsEFFAbbrev.String(), true);
    StatsMenu::pMe->pTiles[eRadResistLabel]->Set(kTileValue_String, sStatsRadResist.String(), true);
    StatsMenu::pMe->pTiles[eNoEffects]->Set(kTileValue_String, sStatsNoEffects.String(), true);
    StatsMenu::pMe->pTiles[eStatusButton]->Set(kTileValue_String, sStatsStatus.String(), true);
    StatsMenu::pMe->pTiles[eSpecialButton]->Set(kTileValue_String, sStatsSpecial.String(), true);
    StatsMenu::pMe->pTiles[eSkillsButton]->Set(kTileValue_String, sStatsSkills.String(), true);
    StatsMenu::pMe->pTiles[ePerksButton]->Set(kTileValue_String, sStatsPerks.String(), true);
    StatsMenu::pMe->pTiles[eGeneralButton]->Set(kTileValue_String, sStatsGeneral.String(), true);
    StatsMenu::pMe->PrepForVisibility(true);
    text.Set(nullptr, 0);
    return static_cast<TileMenu*>(pFile);
}

void StatsMenu::UpdatePerks() {
    if ((StatsMenu::pMe) && (StatsMenu::pMe->pTiles[34])) {
        static_cast<ListBox<PerkRankData*>*>(&StatsMenu::pMe->lbPerks)->RemoveAll();
        for (PerkRankData*& rpItem : pPlayer->GetPerkList(false)->ItemsUntilNull()) {
            if (!rpItem->pPerk->IsPerkHidden()) {
                BSStringT<char> strtmp;
                if (rpItem->cCurrentRank <= 1u) {
                    strtmp.SPrintF("%s", rpItem->pPerk->GetFullName());
                }
                else {
                    int32_t cCurrentRank = rpItem->cCurrentRank;
                    strtmp.SPrintF("%s (%d)", rpItem->pPerk->GetFullName(), cCurrentRank);
                }
                char* v1 = strtmp.pString;
                PerkRankData* m_item = rpItem;
                StatsMenu::pMe->lbPerks.AddItem(m_item, v1, &StatsMenu::PerkListSortFn, nullptr);
            }
        }
    }
}

void StatsMenu::SetCurrentStatsPage(uint8_t a1) {
    if (StatsMenu::pMe) {
        StatsMenu::pMe->SetCurrentPage(static_cast<StatsMenu::Page>(a1));
    }
}

void StatsMenu::UpdatePlayerTitle() {
    if (!StatsMenu::pMe || !StatsMenu::pMe->pTiles[ePlayerTitle]) {
        return;
    }
    const char* title = TESActorBaseData::GetKarmicTitle(pPlayer);
    StatsMenu::pMe->pTiles[ePlayerTitle]->Set(Tile::kTileValue_String, title, true);
}

void StatsMenu::SPECIALUpdateFn(Tile* apTile, ActorValue::Index aeIndex) {
    if (!apTile) {
        return;
    }
    int32_t current = static_cast<ActorValueOwner&>(*pPlayer).GetClampedActorValue(aeIndex);
    int32_t base = static_cast<ActorValueOwner&>(*pPlayer).GetPermanentActorValue(aeIndex);
    apTile->Set(Tile::kTileValue_User1, current);
    if (base == current) {
        apTile->Set(Tile::kTileValue_User0, 0);
        return;
    }
    apTile->Set(Tile::kTileValue_User0, 1);
    apTile->Set(Tile::kTileValue_User2, current > base ? "(+)" : "(-)", true);
    apTile->pParent->Set(Tile::kTileValue_User0, 1);
}

// A skill counts as changed when its value differs from the permanent one, the derived parts (from SPECIAL) included.
void StatsMenu::SkillsUpdateFn(Tile* apTile, ActorValue::Index aeIndex) {
    if (!apTile) {
        return;
    }
    int32_t current = static_cast<ActorValueOwner&>(*pPlayer).GetClampedActorValue(aeIndex);
    int32_t base = static_cast<ActorValueOwner&>(*pPlayer).GetPermanentActorValue(aeIndex);
    float derivedCurrent = AiFormulas::AVDeriveSkill(pPlayer, aeIndex, false);
    float derivedBase = AiFormulas::AVDeriveSkill(pPlayer, aeIndex, true);
    float derivedChange = derivedCurrent - derivedBase;
    int32_t change = current - base + FloatToInt(derivedChange);
    apTile->Set(Tile::kTileValue_User1, current);
    if (change == 0) {
        apTile->Set(Tile::kTileValue_User0, 0);
        return;
    }
    apTile->Set(Tile::kTileValue_User0, 1);
    apTile->Set(Tile::kTileValue_User2, change > 0 ? "(+)" : "(-)", true);
    apTile->pParent->Set(Tile::kTileValue_User0, 1);
}

bool StatsMenu::FadeControlCallback(Tile* apTile, uint32_t aiOn) {
    if (aiOn) {
        apTile->AddFadeControl(Tile::kTileValue_Alpha, 255.0f, 0.0f, 1.0f, Tile::TFCT_NONLINEAR_LONG_DARK_REPEATING);
    }
    else {
        apTile->RemoveFadeControl(Tile::kTileValue_Alpha);
        apTile->Set(Tile::kTileValue_Alpha, 255);
    }
    return true;
}

void StatsMenu::UpdatePlayerName() {
    if (!StatsMenu::pMe || !pPlayer) {
        return;
    }
    BSStringT<char> text;
    int32_t level = pPlayer->GetLevel();
    const char* levelLabel = sMenuDisplayLevelString.String();
    const char* name = pPlayer->GetFullName();
    text.SPrintF("%s - %s %hd", name, levelLabel, level);
    StatsMenu::pMe->pTiles[ePlayerName]->Set(Tile::kTileValue_String, text.pString, true);
    int32_t shownLevel = pPlayer->GetLevel();
    StatsMenu::pMe->GetMenuTile()->Set(Tile::kTileValue_User8, shownLevel);
    text.Set(nullptr, 0);
}

// Hardcore mode heals only the health; otherwise a stimpak heals the targeted limb when a controller targets one.
bool StatsMenu::StimpakTargetConditionCallback(HealingOption& aHO) {
    bool hardcore = pPlayer->IsHardcore();
    if (!aHO.pInventoryItem) {
        return false;
    }
    int32_t target = GetTargetedBodyPart();
    if (hardcore && target != -1) {
        return false;
    }
    if (hardcore || !InterfaceManager::Has360Controller() || target == -1) {
        target = ActorValue::HEALTH;
    }
    int32_t current = static_cast<ActorValueOwner&>(*pPlayer).GetActorValue(static_cast<ActorValue::Index>(target));
    int32_t permanent =
        static_cast<ActorValueOwner&>(*pPlayer).GetPermanentActorValue(static_cast<ActorValue::Index>(target));
    return current < permanent;
}

bool StatsMenu::RadAwayTargetConditionCallback(HealingOption& aHO) {
    return aHO.pInventoryItem &&
        static_cast<ActorValueOwner&>(*pPlayer).GetActorFloatValue(ActorValue::RADIATION_RADS) > 0.0;
}

void StatsMenu::SetKnobPosition(bool abPlaySounds) {
    InterfaceManager* singleton;
    FOPipboyManager* pipboy;
    uint32_t currentTab;
    if (StatsMenu::pMe) {
        currentTab = (uint32_t)StatsMenu::pMe->GetCurrentPage();
        singleton = InterfaceManager::Instance();
        pipboy = singleton->GetPipboy();
        pipboy->SetTabKnobPosition(currentTab, abPlaySounds);
    }
}

ActorValue::Index StatsMenu::GetTargetedBodyPart() {
    if (!StatsMenu::IsTargetingBodyPart()) {
        return ActorValue::VALUE_NONE;
    }
    StatsMenu::BodyPartController* Instance = StatsMenu::BodyPartController::GetInstance();
    return StatsMenu::BodyPartController::Convert(Instance->eCurrentBodyPart);
}

float StatsMenu::GetRadMeterMax() {
    return 1000.0f;
}

uint8_t StatsMenu::GetCurrentStatsPage() {
    if (StatsMenu::pMe) {
        return (uint8_t)StatsMenu::pMe->GetCurrentPage();
    }

    return (uint8_t)0;
}

void StatsMenu::Close() {
    if ((StatsMenu::pMe) && (StatsMenu::pMe->bLimbMode)) {
        StatsMenu::pMe->ToggleHealingMode();
    }
}

void StatsMenu::MiscStatsUpdateFn(Tile* apTile, MiscStatManager::MiscStatID aeID) {
    int32_t pCMiscStat;
    if (apTile) {
        pCMiscStat = MiscStatManager::GetVal(aeID);
        apTile->Set(4101, pCMiscStat);
    }
}

void StatsMenu::ClearPerks() {
    if ((StatsMenu::pMe) && (StatsMenu::pMe->pTiles[34])) {
        static_cast<ListBox<PerkRankData*>*>(&StatsMenu::pMe->lbPerks)->RemoveAll();
    }
}

int32_t StatsMenu::StatusEffectSortFn(StatsMenu::StatusEffect* apItem1, StatsMenu::StatusEffect* apItem2) {
    TESFullName* pFullName2 = dynamic_cast<TESFullName*>(apItem2->pEffectForm);
    const char* pName2 = pFullName2->GetFullName();
    TESFullName* pFullName1 = dynamic_cast<TESFullName*>(apItem1->pEffectForm);
    return BSstricmp(pFullName1->GetFullName(), pName2);
}

int32_t StatsMenu::PerkListSortFn(ListBoxItem<PerkRankData*>* apItem1, ListBoxItem<PerkRankData*>* apItem2) {
    return BSstricmp(apItem1->data->pPerk->GetFullName(), apItem2->data->pPerk->GetFullName());
}

int32_t StatsMenu::ReputationListSortFn(ListBoxItem<TESReputation*>* apItem1, ListBoxItem<TESReputation*>* apItem2) {
    return BSstricmp(apItem1->data->GetFullName(), apItem2->data->GetFullName());
}

StatsMenu::Page& operator++(StatsMenu::Page& arPage) {
    arPage = static_cast<StatsMenu::Page>(arPage + 1);
    return arPage;
}

StatsMenu::Page& operator--(StatsMenu::Page& aePage) {
    aePage = static_cast<StatsMenu::Page>(aePage - 1);
    return aePage;
}

StatsMenu::BodyPartController::BodyPartIndex& operator++(StatsMenu::BodyPartController::BodyPartIndex& arIndex) {
    arIndex = static_cast<StatsMenu::BodyPartController::BodyPartIndex>(arIndex + 1);
    return arIndex;
}

int32_t StatsMenu_sortingFunction(EffectSetting* a1, EffectSetting* a2) {
    BSStringT<char> apResult = a2->GetEffectName();
    char* v3 = apResult.pString;
    int32_t v8 = BSstricmp(a1->GetEffectName().pString, v3);
    return v8;
}

bool DrBagTargetConditionCallback(StatsMenu::HealingOption& aHO) {
    bool bIsHardcore = pPlayer->IsHardcore();
    if (aHO.pInventoryItem) {
        ActorValue::Index etargetAV = StatsMenu::GetTargetedBodyPart();
        if (!bIsHardcore && etargetAV != ActorValue::VALUE_NONE) {
            return false;
        }
        for (ActorValue::Index eav = ActorValue::PERCEPTION_CONDITION; eav < ActorValue::BRAIN_CONDITION;
             eav = static_cast<ActorValue::Index>(eav + 1)) {
            int32_t v2 = pPlayer->GetActorValue(eav);
            if (v2 < pPlayer->GetPermanentActorValue(eav)) {
                return true;
            }
        }
    }
    return false;
}

EffectSetting* ActiveEffect::GetEffectSetting() {
    return pEffect->GetEffectSetting();
}
