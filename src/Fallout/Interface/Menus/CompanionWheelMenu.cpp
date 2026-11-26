#include "Fallout/Interface/Menus/CompanionWheelMenu.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/BSFurnitureMarker.h"
#include "BSMain/Setting.h"
#include "BSMenu/Menu.h"
#include "BSMenu/RadialTile.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileMenu.h"
#include "BSShader/BSFadeNode.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSDefaultObjectManager.h"
#include "Fallout Shared/BGSOpenCloseForm.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/FOScriptFunctions.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/BGSZoneTargetListener.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Magic/AlchemyItem.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout Shared/PackageSchedule.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESConditionFunctions.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESLight.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectBOOK.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESQuest.h"
#include "Fallout Shared/TESQuestStage.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESScriptFunctions.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TESTopicInfo.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/Combat/VATS.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/DialoguePackage.h"
#include "Fallout/AI/FollowerBarks.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/LowProcess.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Dialogue/DialogueItem.h"
#include "Fallout/Interface/FOPipboyManager.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/LevelUpMenu.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdio.h"
#include "SDK/MSVC/string.h"

#include <cstdint>

int32_t return_1075() {
    return 1075;
}

CompanionWheelMenu::CompanionWheelMenu()
  : Menu(), m_CurrentlySelectedTile(TI_NO_TILE), bTalkToOnClose(false), m_hCurrentReactivityBark(0u) {}

CompanionWheelMenu::~CompanionWheelMenu() {
    if (bTalkToOnClose) {
        InterfaceManager::Instance()->InternalSetMenuMode(InterfaceManager::eMenuModeOff);
        TransitionFromCloseToTalkTo();
    }
    CompanionWheelMenu::pMe = nullptr;
}

void CompanionWheelMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
    if (aiID >= 0 && aiID < 16) {
        pTiles[aiID] = apTile;
    }
}

void CompanionWheelMenu::DoClick(int32_t aiID, Tile* apTarget) {
    if (!apTarget && aiID >= 0 && aiID < 16) {
        apTarget = pTiles[aiID];
    }
    if (Interface::IsTopMenuID(Interface::eCompanionWheelMenu)) {
        switch (aiID) {
            case 0: {
                Do_ToggleAggressivePassive();
                HandleMouseoverForTile(aiID, 1);
                InterfaceManager::PlayMenuSound(MENU_SOUND_OK);
                break;
            }
            case 1: {
                Do_UseStim();
                HandleMouseoverForTile(aiID, 1);
                InterfaceManager::PlayMenuSound(MENU_SOUND_OK);
                break;
            }
            case 2: {
                Do_ToggleStayFollow();
                HandleMouseoverForTile(aiID, 1);
                InterfaceManager::PlayMenuSound(MENU_SOUND_OK);
                break;
            }
            case 3: {
                Do_Talk();
                InterfaceManager::PlayMenuSound(MENU_SOUND_OK);
                break;
            }
            case 4: {
                Do_Backup();
                InterfaceManager::PlayMenuSound(MENU_SOUND_OK);
                break;
            }
            case 5: {
                Do_ToggleNearFar();
                HandleMouseoverForTile(aiID, 1);
                InterfaceManager::PlayMenuSound(MENU_SOUND_OK);
                break;
            }
            case 6: {
                OpenInventory();
                InterfaceManager::PlayMenuSound(MENU_SOUND_OK);
                break;
            }
            case 7: {
                Do_ToggleRangedMelee();
                HandleMouseoverForTile(aiID, 1);
                InterfaceManager::PlayMenuSound(MENU_SOUND_OK);
                break;
            }
            case 11: {
                InterfaceManager::PlayMenuSound(MENU_SOUND_OK);
                CompanionWheelMenu::Close(true);
                break;
            }
            default: {
                return;
            }
        }
    }
}

void CompanionWheelMenu::DoEnter(int32_t aiID, Tile* apTarget) {
    // A null apTarget is filled from the tile table, though nothing reads it afterwards.
    if (!apTarget && aiID >= 0 && aiID < 16) {
        apTarget = pTiles[aiID];
    }
    if (Interface::IsTopMenuID(Interface::eCompanionWheelMenu) && aiID >= 0 && (aiID <= 7 || aiID == 11)) {
        HandleMouseoverForTile(aiID, 0);
    }
}

void CompanionWheelMenu::DoIdle() {
    char bStickInput = 0;
    float fThumbLX = 0.0f;
    float fThumbLY = 0.0f;
    if (m_nTimeToClearSubtitle && BSGetTickCount() > m_nTimeToClearSubtitle) {
        m_nTimeToClearSubtitle = 0;
        if (pTiles[12]) {
            pTiles[12]->Set(4036, "", true);
        }
    }

    if (Interface::IsTopMenuID(Interface::eCompanionWheelMenu)) {
        InterfaceManager::Instance();
        if (InterfaceManager::Has360Controller()) {
            if (!InterfaceManager::Instance()->GetMouseVisible()) {
                InterfaceManager::Instance();
                if (InterfaceManager::Has360Controller()) {
                    if (!InterfaceManager::Instance()->GetMouseVisible() &&
                        (std::abs(lastControllerState.Gamepad.sThumbLY) > 0x1EA9 ||
                         std::abs(pState.Gamepad.sThumbLX) > 0x1EA9)) {
                        bStickInput = 1;
                        fThumbLX = static_cast<float>(lastControllerState.Gamepad.sThumbLX);
                        fThumbLY = static_cast<float>(lastControllerState.Gamepad.sThumbLY);
                    }
                }
            }
        }
        if (bStickInput) {
            float fInputAngle;
            float fNegThumbLY = -fThumbLY;
            if (fNegThumbLY == 0.0) {
                if (fThumbLX > 0.0) {
                    fInputAngle = 1.57075f;
                }
                else {
                    fInputAngle = 4.71238f;
                }
            }
            else {
                float fRatio = -fThumbLX / fNegThumbLY;
                fInputAngle = NiATan(fRatio);
            }
            if (fNegThumbLY > 0.0) {
                fInputAngle = fInputAngle + 3.141590118408203;
            }
            if (fThumbLX < 0.0 && fNegThumbLY < 0.0) {
                fInputAngle = fInputAngle + 6.283180236816406;
            }
            SetButtonAsHighlighted(fInputAngle);
        }
    }
}

bool CompanionWheelMenu::DoGamepad(int32_t aiButton, float afValue) {
    if (!Interface::IsTopMenuID(Interface::eCompanionWheelMenu)) {
        return false;
    }
    if (aiButton == 9) {
        if (m_CurrentlySelectedTile == -1) {
            DoClick(TI_EXIT_BUTTON, pTiles[TI_EXIT_BUTTON]);
        }
        else {
            DoClick(m_CurrentlySelectedTile, pTiles[m_CurrentlySelectedTile]);
        }
        return true;
    }

    if (aiButton == 10) {
        DoClick(11, pTiles[11]);
        return true;
    }

    return false;
}

bool CompanionWheelMenu::IsWellFormedMenu() {
    for (int32_t i = 0; i < 16; ++i) {
        if (!pTiles[i]) {
            return false;
        }
    }
    return true;
}

void CompanionWheelMenu::Do_Backup() {
    if (m_pRelatedCompanion) {
        m_pRelatedCompanion->InitiateBackUpPackage();
    }
    Close(true);
}

void CompanionWheelMenu::Do_Talk() {
    bTalkToOnClose = true;
    Close(true);
}

void CompanionWheelMenu::RunTopicScript(TESTopic* apTopic) {
    TESTopicInfo* pInfo = nullptr;
    if (apTopic) {
        pInfo = apTopic->GetMatchingInfo(m_pRelatedCompanion, pPlayer);
    }
    if (pInfo) {
        Script* pScript = pInfo->GetResultScript(TIRS_BEGIN);
        if (pScript && pScript->GetHeader()->dataSize) {
            ScriptLocals* pLocals = m_pRelatedCompanion->TESObjectREFR::GetExtra()->GetScriptLocals();
            pScript->Run(m_pRelatedCompanion, pLocals, nullptr, true);
        }

        pScript = pInfo->GetResultScript(TIRS_END);
        if (pScript && pScript->GetHeader()->dataSize) {
            ScriptLocals* pLocals = m_pRelatedCompanion->TESObjectREFR::GetExtra()->GetScriptLocals();
            pScript->Run(m_pRelatedCompanion, pLocals, nullptr, true);
        }
    }
}

void CompanionWheelMenu::SetButtonPreviewText(const char* asText) {
    if (pTiles[9]) {
        pTiles[9]->Set(4036, asText, true);
    }
}

void CompanionWheelMenu::SetButtonContextText(int32_t aiID) {
    char acText[260];
    acText[0] = '\0';
    switch (aiID) {
        case 1:
            if (m_pRelatedCompanion) {
                int32_t iHealth = static_cast<int32_t>(
                    static_cast<ActorValueOwner*>(m_pRelatedCompanion)->GetActorFloatValue(ActorValue::HEALTH)
                );
                int32_t iBaseHealth = static_cast<ActorValueOwner*>(m_pRelatedCompanion)
                                          ->GetBaseActorValue(ActorValue::HEALTH);
                int32_t iStimpaks = 0;
                TESBoundObject* pStimpak = dynamic_cast<TESBoundObject*>(
                    BGSDefaultObjectManager::GetDefaultObject(DO_STIMPAK)
                );
                if (pStimpak) {
                    ItemChange* pItem = pPlayer->GetInventoryItem(pStimpak, 0);
                    if (pItem) {
                        iStimpaks = pItem->GetCount();
                        delete pItem;
                    }
                }
                const char* pStimpakText = sStimpak.String();
                const char* pHealthText = sHitPointsShort.String();
                BSsprintf(
                    acText,
                    0x104,
                    "%s  %d/%d\n%s  %d",
                    pHealthText,
                    iHealth,
                    iBaseHealth,
                    pStimpakText,
                    iStimpaks
                );
            }
            break;
        case 6:
            if (m_pRelatedCompanion) {
                ActorValueOwner* pOwner = m_pRelatedCompanion;
                int32_t iWeight = static_cast<int32_t>(Actor::GetInventoryWeight(pOwner, ActorValue::CARRY_WEIGHT));
                int32_t iMaxWeight = static_cast<int32_t>(m_pRelatedCompanion->GetMaxCarryWeight());
                BSsprintf(acText, 0x104, "%s  %d/%d", sInventoryWeight.String(), iWeight, iMaxWeight);
            }
            break;
        case 7:
            if (m_pRelatedCompanion) {
                ItemChange* pWeapon = m_pRelatedCompanion->GetBestWeapon(COMBAT_WEAPON_TYPE_COUNT);
                if (pWeapon) {
                    BSsprintf(acText, 0x104, "%s", pWeapon->GetFullName());
                }
            }
            break;
    }
    if (pTiles[10]) {
        pTiles[10]->Set(Tile::kTileValue_String, acText, true);
    }
}

void CompanionWheelMenu::SpeakTopic(TESTopic* apTopic) {
    TESTopicInfo* pTopicInfo = nullptr;
    TESObjectREFR* pTarget = pPlayer;
    Actor* pSpeaker = m_pRelatedCompanion;
    if (apTopic) {
        pTopicInfo = apTopic->GetMatchingInfo(pSpeaker, pTarget);
    }
    if (pTopicInfo) {
        DialogueItem kItem(pTopicInfo->pOwnerQuest, apTopic, pTopicInfo, pSpeaker);
        kItem.FirstResponse();
        DialogueResponse* pResponse = kItem.GetCurrentResponse();
        if (pResponse) {
            if (pResponse->pVoiceSound) {
                int32_t iSoundID = pResponse->pVoiceSound->iFormID;
                this->m_hCurrentReactivityBark = pSpeaker->PlaySound(iSoundID, false, 258u, true);
            }
            else {
                PlayerCharacter* pListener = pPlayer;
                TESIdleForm* pListenIdle = pResponse->pListenIdle;
                TESIdleForm* pSpeakerIdle = pResponse->pSpeakerIdle;
                uint32_t uiLength = pResponse->cText.GetLength();
                uint32_t uiEmotion = pResponse->uiEmotion;
                DIALOGUE_EMOTION eEmotion = pResponse->eEmotion;
                char* pVoice = pResponse->GetVoice()->pString;
                pSpeaker->SpeakSoundFunction(
                    pVoice,
                    this->m_hCurrentReactivityBark,
                    eEmotion,
                    uiEmotion,
                    uiLength,
                    pSpeakerIdle,
                    pListenIdle,
                    pListener,
                    true,
                    true,
                    false,
                    false,
                    true
                );
            }
            this->DisplayCompanionWheelSubtitle(pResponse->cText, this->m_hCurrentReactivityBark);
        }
    }
}

void CompanionWheelMenu::OpenInventory() {
    CompanionWheelMenu::Close(true);
    TESForm* pForm = TESForm::GetFormByEditorID("FollowersTrade");
    TESTopic* pTopic = dynamic_cast<TESTopic*>(pForm);
    if (m_pRelatedCompanion) {
        SpeakTopic(pTopic);
        RunTopicScript(pTopic);
    }
}

void CompanionWheelMenu::HandleMouseoverForTile(int32_t aiID, uint8_t abForce) {
    if (abForce || m_CurrentlySelectedTile != aiID) {
        BSAudio::QInstance().GetSoundHandleByName("UIPipBoyScroll", 289u).Play(false);
        if (m_CurrentlySelectedTile != CompanionWheelMenu::TI_NO_TILE && pTiles[m_CurrentlySelectedTile]) {
            pTiles[m_CurrentlySelectedTile]->Set(Tile::kTileValue_User10, 0);
        }

        switch (m_CurrentlySelectedTile) {
            case CompanionWheelMenu::TI_AGGRESSIVE_PASSIVE_BUTTON: {
                if (pTiles[m_CurrentlySelectedTile]) {
                    pTiles[m_CurrentlySelectedTile]->Set(
                        Tile::kTileValue_User11,
                        static_cast<int32_t>(m_bAggressiveStance)
                    );
                }
                break;
            }
            case CompanionWheelMenu::TI_STAY_FOLLOW_BUTTON: {
                if (pTiles[m_CurrentlySelectedTile]) {
                    pTiles[m_CurrentlySelectedTile]->Set(Tile::kTileValue_User11, static_cast<int32_t>(!m_bFollowing));
                }
                break;
            }
            case CompanionWheelMenu::TI_NEAR_FAR_BUTTON: {
                if (pTiles[m_CurrentlySelectedTile]) {
                    pTiles[m_CurrentlySelectedTile]->Set(
                        Tile::kTileValue_User11,
                        static_cast<int32_t>(m_bFollowingLong)
                    );
                }
                break;
            }
            case CompanionWheelMenu::TI_RANGED_MELEE_BUTTON: {
                if (pTiles[m_CurrentlySelectedTile]) {
                    pTiles[m_CurrentlySelectedTile]->Set(
                        Tile::kTileValue_User11,
                        static_cast<int32_t>(m_bRangedCombat)
                    );
                }
                break;
            }
            default: {
                break;
            }
        }
        m_CurrentlySelectedTile = static_cast<CompanionWheelMenu::TILE_INDEX>(aiID);
        if (m_CurrentlySelectedTile != CompanionWheelMenu::TI_NO_TILE && pTiles[m_CurrentlySelectedTile]) {
            pTiles[m_CurrentlySelectedTile]->Set(Tile::kTileValue_User10, 1);
        }

        switch (aiID) {
            case 0: {
                const char* pText = m_bAggressiveStance ? sCWheelBePassive.String() : sCWheelBeAggressive.String();
                SetButtonPreviewText(pText);
                if (pTiles[aiID]) {
                    pTiles[aiID]->Set(Tile::kTileValue_User11, static_cast<int32_t>(!m_bAggressiveStance));
                }
                break;
            }
            case 1: {
                SetButtonPreviewText(sCWheelUseStimPack.String());
                break;
            }
            case 2: {
                const char* pText = m_bFollowing ? sCWheelWaitHere.String() : sCWheelFollowMe.String();
                SetButtonPreviewText(pText);
                if (pTiles[aiID]) {
                    pTiles[aiID]->Set(Tile::kTileValue_User11, static_cast<int32_t>(m_bFollowing));
                }
                break;
            }
            case 3: {
                SetButtonPreviewText(sCWheelTalkTo.String());
                break;
            }
            case 4: {
                SetButtonPreviewText(sCWheelBackUp.String());
                break;
            }
            case 5: {
                const char* pText = m_bFollowingLong ? sCWheelStayClose.String() : sCWheelKeepDistance.String();
                SetButtonPreviewText(pText);
                if (pTiles[aiID]) {
                    pTiles[aiID]->Set(Tile::kTileValue_User11, static_cast<int32_t>(!m_bFollowingLong));
                }
                break;
            }
            case 6: {
                SetButtonPreviewText(sCWheelOpenInventory.String());
                break;
            }
            case 7: {
                const char* pText = m_bRangedCombat ? sCWheelUseMelee.String() : sCWheelUseRanged.String();
                SetButtonPreviewText(pText);
                if (pTiles[aiID]) {
                    pTiles[aiID]->Set(Tile::kTileValue_User11, static_cast<int32_t>(!m_bRangedCombat));
                }
                break;
            }
            case 11: {
                SetButtonPreviewText(sExit.String());
                break;
            }
            default: {
                SetButtonPreviewText("");
                break;
            }
        }
        SetButtonContextText(m_CurrentlySelectedTile);
    }
}

void CompanionWheelMenu::SetButtonAsHighlighted(float afAngle) {
    if (afAngle >= 0.0 && afAngle < 0.5235980153083801) {
        afAngle = 0.55f;
    }
    else if (afAngle >= 2.617990016937256 && afAngle < 3.141590118408203) {
        afAngle = 2.6f;
    }
    else if (afAngle >= 3.14159 && afAngle < 3.665186) {
        afAngle = 3.7f;
    }
    else if (afAngle >= 5.759578) {
        afAngle = 5.7f;
    }
    for (int32_t iter = 0; iter < 16; ++iter) {
        RadialTile* pRadialTileTest = dynamic_cast<RadialTile*>(pTiles[iter]);
        if (pRadialTileTest) {
            float fMinAngle = pRadialTileTest->GetFloat(Tile::kTileValue_User2);
            float fMaxAngle = pRadialTileTest->GetFloat(Tile::kTileValue_User3);
            if (fMinAngle <= static_cast<double>(afAngle) && fMaxAngle > static_cast<double>(afAngle)) {
                HandleMouseoverForTile(iter, 0);
                return;
            }
        }
    }
}

void CompanionWheelMenu::Do_ToggleNearFar() {
    m_bFollowingLong = !m_bFollowingLong;
    if (m_bFollowingLong) {
        TESForm* pForm = TESForm::GetFormByEditorID("FollowersTacticsDistanceLong");
        TESTopic* pTopic = dynamic_cast<TESTopic*>(pForm);
        SpeakTopic(pTopic);
        RunTopicScript(pTopic);
    }
    else {
        TESForm* pForm = TESForm::GetFormByEditorID("FollowersTacticsDistanceDefault");
        TESTopic* pTopic = dynamic_cast<TESTopic*>(pForm);
        SpeakTopic(pTopic);
        RunTopicScript(pTopic);
    }
    if (pTiles[5]) {
        pTiles[5]->Set(Tile::kTileValue_User11, m_bFollowingLong);
    }
}

void CompanionWheelMenu::Do_ToggleAggressivePassive() {
    m_bAggressiveStance = !m_bAggressiveStance;
    if (m_bAggressiveStance) {
        TESForm* pForm = TESForm::GetFormByEditorID("FollowersTacticsCombatAggressive");
        TESTopic* pTopic = dynamic_cast<TESTopic*>(pForm);
        SpeakTopic(pTopic);
        RunTopicScript(pTopic);
    }
    else {
        TESForm* pForm = TESForm::GetFormByEditorID("FollowersTacticsCombatPassive");
        TESTopic* pTopic = dynamic_cast<TESTopic*>(pForm);
        SpeakTopic(pTopic);
        RunTopicScript(pTopic);
    }
    float fAggressive = m_bAggressiveStance ? 1.0f : 0.0f;
    CompanionWheelMenu::pMe->m_pRelatedCompanion->SetLocalVariableOnActor("FollowerSwitchAggressive", fAggressive);
    if (pTiles[0]) {
        pTiles[0]->Set(Tile::kTileValue_User11, static_cast<int32_t>(m_bAggressiveStance));
    }
}

void CompanionWheelMenu::Do_ToggleRangedMelee() {
    if (m_pRelatedCompanion) {
        m_bRangedCombat = !m_bRangedCombat;
        if (m_bRangedCombat) {
            TESForm* pForm = TESForm::GetFormByEditorID("FollowersTacticsCombatRanged");
            TESTopic* pTopic = dynamic_cast<TESTopic*>(pForm);
            SpeakTopic(pTopic);
            RunTopicScript(pTopic);
        }
        else {
            TESForm* pForm = TESForm::GetFormByEditorID("FollowersTacticsCombatMelee");
            TESTopic* pTopic = dynamic_cast<TESTopic*>(pForm);
            SpeakTopic(pTopic);
            RunTopicScript(pTopic);
        }
        if (pTiles[7]) {
            pTiles[7]->Set(Tile::kTileValue_User11, static_cast<int32_t>(CompanionWheelMenu::pMe->m_bRangedCombat));
        }
    }
}

void CompanionWheelMenu::Do_ToggleStayFollow() {
    m_bFollowing = !m_bFollowing;
    if (m_bFollowing) {
        TESForm* pForm = TESForm::GetFormByEditorID("FollowersLetsGo");
        TESTopic* pTopic = dynamic_cast<TESTopic*>(pForm);
        SpeakTopic(pTopic);
        RunTopicScript(pTopic);
    }
    else {
        TESForm* pForm = TESForm::GetFormByEditorID("FollowersWait");
        TESTopic* pTopic = dynamic_cast<TESTopic*>(pForm);
        SpeakTopic(pTopic);
        RunTopicScript(pTopic);
    }
    if (pTiles[2]) {
        pTiles[2]->Set(Tile::kTileValue_User11, static_cast<int32_t>(!m_bFollowing));
    }
}

void CompanionWheelMenu::Do_UseStim() {
    if (this->m_pRelatedCompanion && pPlayer) {
        TESForm* pForm = BGSDefaultObjectManager::GetDefaultObject(DO_STIMPAK);
        TESBoundObject* pStimpak = dynamic_cast<TESBoundObject*>(pForm);
        if (pStimpak) {
            ItemChange* pItem = pPlayer->GetInventoryItem(pStimpak, 0);
            if (pItem) {
                float fHealthPercentage = this->m_pRelatedCompanion->GetHealthPercentage();
                int32_t iCrippledCount = 0;
                double dBodyPartCondition = CombatFormulas::GetBodyPartCondition(
                    this->m_pRelatedCompanion,
                    ActorValue::LEFT_ATTACK_CONDITION,
                    false
                );
                float fCondition = dBodyPartCondition;
                if (fCondition == 0.0) {
                    iCrippledCount = 1;
                }
                dBodyPartCondition = CombatFormulas::GetBodyPartCondition(
                    this->m_pRelatedCompanion,
                    ActorValue::LEFT_MOBILITY_CONDITION,
                    false
                );
                fCondition = dBodyPartCondition;
                if (fCondition == 0.0) {
                    ++iCrippledCount;
                }
                dBodyPartCondition = CombatFormulas::GetBodyPartCondition(
                    this->m_pRelatedCompanion,
                    ActorValue::RIGHT_MOBILITY_CONDITION,
                    false
                );
                fCondition = dBodyPartCondition;
                if (fCondition == 0.0) {
                    ++iCrippledCount;
                }
                dBodyPartCondition = CombatFormulas::GetBodyPartCondition(
                    this->m_pRelatedCompanion,
                    ActorValue::RIGHT_ATTACK_CONDITION,
                    false
                );
                fCondition = dBodyPartCondition;
                if (fCondition == 0.0) {
                    ++iCrippledCount;
                }
                dBodyPartCondition = CombatFormulas::GetBodyPartCondition(
                    this->m_pRelatedCompanion,
                    ActorValue::PERCEPTION_CONDITION,
                    false
                );
                fCondition = dBodyPartCondition;
                if (fCondition == 0.0) {
                    ++iCrippledCount;
                }
                dBodyPartCondition = CombatFormulas::GetBodyPartCondition(
                    this->m_pRelatedCompanion,
                    ActorValue::ENDURANCE_CONDITION,
                    false
                );
                fCondition = dBodyPartCondition;
                if (fCondition == 0.0) {
                    ++iCrippledCount;
                }
                if (fHealthPercentage < 1.0 || iCrippledCount > 0) {
                    if (fHealthPercentage < 1.0) {
                        ExtraDataList* pExtraList = pItem->pExtraObjectList ? pItem->pExtraObjectList->m_item : nullptr;
                        AlchemyItem* pPotion = dynamic_cast<AlchemyItem*>(pItem->pContainerObj);
                        this->m_pRelatedCompanion->DrinkPotion(pPotion, pExtraList, true);
                    }
                    ExtraDataList* pExtraList = pItem->pExtraObjectList ? pItem->pExtraObjectList->m_item : nullptr;
                    pPlayer->RemoveItem(
                        pItem->pContainerObj,
                        pExtraList,
                        1,
                        false,
                        false,
                        nullptr,
                        nullptr,
                        nullptr,
                        true,
                        false
                    );
                    this->m_pRelatedCompanion->UpdateMagic(false, true);
                    this->m_pRelatedCompanion->RestoreActorValue(ActorValue::BRAIN_CONDITION, 1000.0f);
                    this->m_pRelatedCompanion->RestoreActorValue(ActorValue::LEFT_MOBILITY_CONDITION, 1000.0f);
                    this->m_pRelatedCompanion->RestoreActorValue(ActorValue::RIGHT_MOBILITY_CONDITION, 1000.0f);
                    this->m_pRelatedCompanion->RestoreActorValue(ActorValue::LEFT_ATTACK_CONDITION, 1000.0f);
                    this->m_pRelatedCompanion->RestoreActorValue(ActorValue::RIGHT_ATTACK_CONDITION, 1000.0f);
                    this->m_pRelatedCompanion->RestoreActorValue(ActorValue::ENDURANCE_CONDITION, 1000.0f);
                    this->m_pRelatedCompanion->RestoreActorValue(ActorValue::PERCEPTION_CONDITION, 1000.0f);
                    SpeakTopic(FollowerBarks::GetBarkTopicByType(FollowerBarks::BARK_REGENERATING));
                }
                delete pItem;
            }
        }
    }
}

void CompanionWheelMenu::TransitionFromCloseToTalkTo() {
    Actor* pActionActor = pPlayer;
    Actor* pactor = m_pRelatedCompanion;
    if (pactor && !pactor->IsDying()) {
        BaseProcess* pprocess = pactor->pCurrentProcess;
        if (pprocess && (!pprocess->GetCommandingActor() || pactor->IsDead(false)) && !pprocess->GetFadeState()) {
            BaseProcess* pPlayerProc = pActionActor->pCurrentProcess;
            TESPackage* pactionpackage = pPlayerProc->GetPackageThatIsRunning();
            TESPackage* potherpackage = pprocess->GetPackageThatIsRunning();
            if (pActionActor) {
                if (pactor->IsInCombat() && pActionActor == pPlayer && pactor->IsPlayerTeammate()) {
                    pVATS.QuitVATSPlayback(false, false);
                    Interface::QueueMenuCreate(
                        Interface::OPEN_DIALOG,
                        pactor,
                        nullptr,
                        nullptr,
                        Interface::CMM_STANDARD,
                        false
                    );
                    return;
                }
                if ((!pactor->IsInCombat() || pactor->IsKnockedOut()) &&
                    (pActionActor != pPlayer ||
                     pactor->GetSitSleepState() == SIT_SLEEP_NORMAL ||
                     pactor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING ||
                     pactor->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
                     (pactor->pCurrentProcess->GetFurnitureMarker() &&
                      pactor->pCurrentProcess->GetFurnitureMarker()->cNumber > 19u))) {
                    if (!pactor->IsReanimated() &&
                        !pactor->IsDead(false) &&
                        (pActionActor != pPlayer || !pactor->IsKnockedOut() || !pactor->GetEssential())) {
                        if ((pActionActor == pPlayer ||
                             (pactor == pPlayer &&
                              (pActionActor->GetCurrentPackage()->IsInterruptPackage() ||
                               pActionActor->GetCurrentPackage()->GetPackType() == PACKAGE_TYPE_EXPLORE ||
                               pActionActor->GetCurrentPackage()->GetPackType() == PACKAGE_TYPE_AMBUSH))) &&
                            (pActionActor != pPlayer ||
                             (!pactor->IsKnockedOut() && (!pPlayer->IsSneaking() || pactor->IsPlayerTeammate())) ||
                             pactor->GetGhost())) {
                            TESTopic* pgreetTopic = nullptr;
                            if (pActionActor == pPlayer || pActionActor->pCurrentProcess->GetNeedTalkPlayer()) {
                                pgreetTopic = TESTopic::GetTopic(DT_TOPIC, 0);
                            }
                            DialogueItem* pgreetDialogue = nullptr;
                            if (pgreetTopic) {
                                if (pActionActor == pPlayer) {
                                    pgreetDialogue = pgreetTopic
                                                         ->CreateDialogueItem(pactor, pPlayer, nullptr, nullptr, false);
                                }
                                else if (pActionActor->pCurrentProcess->GetNeedTalkPlayer()) {
                                    pgreetDialogue = pgreetTopic->CreateDialogueItem(
                                        pActionActor,
                                        pPlayer,
                                        nullptr,
                                        nullptr,
                                        false
                                    );
                                }
                            }
                            if (pgreetDialogue &&
                                pgreetDialogue->pInfo->IsGoodbye() &&
                                pgreetDialogue->FirstResponse() &&
                                !pgreetDialogue->NextResponse()) {
                                if (pactor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
                                    pactor->pCurrentProcess->SetNeedTalkPlayer(true);
                                    pactor->InitiateGetUpPackage();
                                    return;
                                }
                                if (pactor == pPlayer) {
                                    pActionActor->SayForceGoodbyeGreetingToPlayer(pgreetTopic, false, false);
                                }
                                else {
                                    pactor->SayForceGoodbyeGreetingToPlayer(pgreetTopic, false, false);
                                }
                            }
                            else {
                                if (pactor->pCurrentProcess->GetFurnitureMarker()) {
                                    if (pactor->pCurrentProcess->GetFurnitureMarker()->cNumber > 19u) {
                                        pactor->pCurrentProcess->AddPostAnimationAction(
                                            BaseProcess::POSTANIM_ACTION_GETOUT_OF_FURNITURE
                                        );
                                    }
                                }
                                if (pactor->GetSitSleepState() == SIT_SLEEP_NORMAL) {
                                    NiPoint3 kActionActorPos = pActionActor->GetLocationOnReference();
                                    pactor->RequestRotate(kActionActorPos, false);
                                }
                                TESTopic* ptopic = nullptr;
                                if (pActionActor == pPlayer) {
                                    if (!pactor->IsPathingComplete() && !pactor->IsRotating()) {
                                        pactor->pCurrentProcess->EndMoveMessage(pactor);
                                    }
                                    pActionActor->StopAttack();
                                    pactor->StopAttack();
                                    TESPackage* pPackage = pActionActor->GetCurrentPackage();
                                    DialoguePackage* ppack = dynamic_cast<DialoguePackage*>(pPackage);
                                    if (ppack) {
                                        ptopic = ppack->ptopic;
                                    }
                                    if (pactor->GetCurrentTarget() == pPlayer &&
                                        potherpackage &&
                                        potherpackage->GetPackType() == PACKAGE_TYPE_EXPLORE) {
                                        if (!potherpackage->GetPackageSchedule() ||
                                            potherpackage->GetPackageSchedule()->PSData.iDuration) {
                                            pprocess->AddToProcedureIndexRunning(pactor, 1);
                                        }
                                        else {
                                            pprocess->AddToProcedureIndexRunning(pActionActor, 2);
                                        }
                                    }

                                    BSAudio::QInstance().EndAllSoundsOfType(4u);
                                    pactor->InterruptCast();
                                    if (pactor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
                                        pactor->pCurrentProcess->SetNeedTalkPlayer(true);
                                        pactor->InitiateGetUpPackage();
                                    }
                                    else {
                                        Actor* pact = pActionActor;
                                        if (pActionActor == pPlayer) {
                                            pact = pactor;
                                        }
                                        pVATS.QuitVATSPlayback(false, false);
                                        Interface::QueueMenuCreate(
                                            Interface::OPEN_DIALOG,
                                            pact,
                                            ptopic,
                                            nullptr,
                                            Interface::CMM_STANDARD,
                                            false
                                        );
                                    }
                                }
                                else if (Interface::GetMenuModeType() != Interface::eDialogMenu) {
                                    if (!pPlayer->pCurrentProcess->CanForceGreet(pActionActor)) {
                                        return;
                                    }
                                    BSAudio::QInstance().EndAllSoundsOfType(4u);
                                    pActionActor->StopAttack();
                                    pactor->StopAttack();
                                    if (!pActionActor->IsInCombat()) {
                                        TESPackage* pPackage = pActionActor->GetCurrentPackage();
                                        DialoguePackage* ppack = dynamic_cast<DialoguePackage*>(pPackage);
                                        if (ppack) {
                                            ptopic = ppack->ptopic;
                                        }
                                    }
                                    if (!pactionpackage->GetPackageSchedule() ||
                                        pactionpackage->GetPackageSchedule()->PSData.iDuration) {
                                        pActionActor->pCurrentProcess->AddToProcedureIndexRunning(pActionActor, 1);
                                    }
                                    else {
                                        pActionActor->pCurrentProcess->AddToProcedureIndexRunning(pActionActor, 2);
                                    }
                                    pactor->InterruptCast();
                                    if (pactor == pPlayer) {
                                        pactor->ForceIdleAnimation();
                                    }
                                    else {
                                        pactor->pCurrentProcess->AddPostAnimationAction(
                                            BaseProcess::POSTANIM_FORCEIDLE_ANIMATION
                                        );
                                    }
                                    if (pActionActor == pPlayer) {
                                        pActionActor->ForceIdleAnimation();
                                    }
                                    else {
                                        pActionActor->pCurrentProcess->AddPostAnimationAction(
                                            BaseProcess::POSTANIM_FORCEIDLE_ANIMATION
                                        );
                                    }
                                    pVATS.QuitVATSPlayback(false, false);
                                    Actor* pact = pActionActor;
                                    if (pActionActor == pPlayer) {
                                        pact = pactor;
                                    }
                                    Interface::QueueMenuCreate(
                                        Interface::OPEN_DIALOG,
                                        pact,
                                        ptopic,
                                        nullptr,
                                        Interface::CMM_STANDARD,
                                        false
                                    );
                                }
                            }
                            if (pgreetDialogue) {
                                delete pgreetDialogue;
                            }
                        }
                    }
                }
                else if (
                    pactor->GetCombatController() &&
                    pActionActor == pPlayer &&
                    pactor->GetCombatController()->QSwimAllowed()
                ) {
                    char msg[200];
                    const char* pCalmedText = sActivateNPCCalmed.String();
                    std::sprintf(msg, "%s %s", pactor->GetFullName(), pCalmedText);
                    Interface::ShowMessage(msg, 0, nullptr, nullptr, 2.0f, false);
                }
            }
        }
    }
}

void CompanionWheelMenu::DisplayCompanionWheelSubtitle(BSStringT<char> asText, BSSoundHandle ahSound) {
    if (asText.GetLength() &&
        (bGeneralSubtitles.Bool() ||
         (m_pRelatedCompanion && static_cast<uint8_t>(m_pRelatedCompanion->GetShowSubtitles())))) {
        if (pTiles[12]) {
            pTiles[12]->Set(4036, asText.pString, true);
        }
        float fDuration = static_cast<float>(ahSound.GetDuration());
        if (fDuration == 0.0) {
            double dTextLength = static_cast<double>(std::strlen(asText.pString));
            fDuration = fNoticeTextTimePerCharacter.Float() * dTextLength * 1000.0;
        }
        m_nTimeToClearSubtitle = static_cast<int64_t>(static_cast<double>(BSGetTickCount()) + fDuration);
    }
}

bool CompanionWheelMenu::CreateCompanionWheelMenu(Actor* apRelatedCompanion) {
    CompanionWheelMenu* pmenu = nullptr;
    Tile* ptile = Menu::GetMenuByClass(1075);
    if (ptile) {
        pmenu = dynamic_cast<CompanionWheelMenu*>(ptile->GetMenu());
    }
    if (pmenu) {
        delete pmenu;
    }
    return CompanionWheelMenu::Create(apRelatedCompanion) != nullptr;
}

void CompanionWheelMenu::Close(bool abClearOutSpeech) {
    if (CompanionWheelMenu::pMe) {
        if (abClearOutSpeech) {
            CompanionWheelMenu::pMe->m_hCurrentReactivityBark.FadeOutAndRelease(100u);
        }
        if (!pSaveLoadGame->GetSaveGameLoading()) {
            ReferenceProcessLists.Update3DAfterResting();
        }
        CompanionWheelMenu::pMe->pMenu->Set(Tile::kTileValue_DeleteOnFade, 1);
        CompanionWheelMenu::pMe->StartFadeOut();
    }
}

Tile* CompanionWheelMenu::Create(Actor* apRelatedCompanion) {
    InterfaceManager* pin = InterfaceManager::Instance();
    Tile* pold_menu_tile = Menu::GetMenuByClass(return_1075());
    if (pold_menu_tile) {
        delete pold_menu_tile;
    }
    Tile* ptile = pin->GetMenusRoot()->ReadFile("Data\\Menus\\companion_wheel_menu.xml");
    CompanionWheelMenu::pMe = static_cast<CompanionWheelMenu*>(ptile->GetMenu());
    if (CompanionWheelMenu::pMe) {
        int32_t iMenuClass = CompanionWheelMenu::pMe->GetClass();
        if (iMenuClass == return_1075()) {
            CompanionWheelMenu::pMe->m_pRelatedCompanion = apRelatedCompanion;
            if (CompanionWheelMenu::pMe->m_pRelatedCompanion) {
                CompanionWheelMenu::pMe->m_bAggressiveStance = CompanionWheelMenu::pMe->m_pRelatedCompanion
                                                                   ->GetLocalVariableOnActor(
                                                                       "FollowerSwitchAggressive"
                                                                   ) == 1.0;
                CompanionWheelMenu::pMe->m_bFollowingLong = CompanionWheelMenu::pMe->m_pRelatedCompanion
                                                                ->GetLocalVariableOnActor("IsFollowingLong") == 1.0;
                CompanionWheelMenu::pMe->m_bRangedCombat = CompanionWheelMenu::pMe->m_pRelatedCompanion
                                                               ->GetLocalVariableOnActor("CombatStyleRanged") == 1.0;
                CompanionWheelMenu::pMe->m_bFollowing = CompanionWheelMenu::pMe->m_pRelatedCompanion
                                                            ->GetLocalVariableOnActor("Waiting") == 0.0;
            }
            if (CompanionWheelMenu::pMe->pTiles[0]) {
                CompanionWheelMenu::pMe->pTiles[0]->Set(
                    Tile::kTileValue_User11,
                    static_cast<int32_t>(CompanionWheelMenu::pMe->m_bAggressiveStance)
                );
            }
            if (CompanionWheelMenu::pMe->pTiles[2]) {
                CompanionWheelMenu::pMe->pTiles[2]->Set(
                    Tile::kTileValue_User11,
                    static_cast<int32_t>(!CompanionWheelMenu::pMe->m_bFollowing)
                );
            }
            if (CompanionWheelMenu::pMe->pTiles[5]) {
                CompanionWheelMenu::pMe->pTiles[5]->Set(
                    Tile::kTileValue_User11,
                    static_cast<int32_t>(CompanionWheelMenu::pMe->m_bFollowingLong)
                );
            }
            if (CompanionWheelMenu::pMe->pTiles[7]) {
                CompanionWheelMenu::pMe->pTiles[7]->Set(
                    Tile::kTileValue_User11,
                    static_cast<int32_t>(CompanionWheelMenu::pMe->m_bRangedCombat)
                );
            }
            if (CompanionWheelMenu::pMe->pTiles[12]) {
                CompanionWheelMenu::pMe->pTiles[12]->Set(4036, "", true);
            }
            if (CompanionWheelMenu::pMe->pTiles[8]) {
                const char* pTitleText = sCWheelTitle.String();
                CompanionWheelMenu::pMe->pTiles[8]->Set(4036, pTitleText, true);
            }
            if (CompanionWheelMenu::pMe->pTiles[13]) {
                const char* pExitText = sExit.String();
                CompanionWheelMenu::pMe->pTiles[13]->Set(4036, pExitText, true);
            }
            if (CompanionWheelMenu::pMe->pTiles[14]) {
                const char* pSelectText = sSelect.String();
                CompanionWheelMenu::pMe->pTiles[14]->Set(4036, pSelectText, true);
            }
            if (CompanionWheelMenu::pMe->pTiles[15]) {
                const char* pNavigateText = sNavigate.String();
                CompanionWheelMenu::pMe->pTiles[15]->Set(4036, pNavigateText, true);
            }
            CompanionWheelMenu::pMe->SetMenuTile(static_cast<TileMenu*>(ptile), false);
            if (CompanionWheelMenu::pMe->IsWellFormedMenu()) {
                BSAudio::QInstance().GetSoundHandleByName("UIPopUpMessageGeneral", 289u).Play(false);
                if (ptile->GetFloat(4007) == 6006.0 || ptile->GetFloat(4007) == 102.0) {
                    float fDepth = Tile::GetMaximumDepth();
                    ptile->Set(4013, fDepth, true);
                }
                CompanionWheelMenu::pMe->PrepForVisibility(false);
                return ptile;
            }

            Warning("MENUS: Companion Wheel Menu Creation Failed... Are your menu and art resources up to date?");
            return nullptr;
        }
    }

    if (CompanionWheelMenu::pMe && CompanionWheelMenu::pMe->pMenu && CompanionWheelMenu::pMe) {
        delete CompanionWheelMenu::pMe;
    }

    return nullptr;
}

void InterfaceManager::InternalSetMenuMode(eMenuModeType aiMode) {
    this->cMenuMode = aiMode;
}
