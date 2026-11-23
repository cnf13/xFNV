#include "Fallout/Interface/Menus/DialogMenu.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSString.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSMenu/Font.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileMenu.h"
#include "BSShader/BSFadeNode.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueInfo.h"
#include "Fallout Shared/AiFormulas.h"
#include "Fallout Shared/BGSPerk.h"
#include "Fallout Shared/FaceGen/BSFaceGenNiNode.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/ImageSpaceModifierInstance.h"
#include "Fallout Shared/ImageSpaceModifierInstanceDOF.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESConditionItem.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESImageSpaceModifier.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESQuest.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TESTopicInfo.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Audio/FalloutAudio.h"
#include "Fallout/Dialogue/DialogueResponse.h"
#include "Fallout/Dialogue/MenuTopic.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/ListBox.h"
#include "Fallout/Interface/Menus/HUDMainMenu.h"
#include "Fallout/Interface/Menus/StartMenu.h"
#include "Fallout/Interface/Menus/StatsMenu.h"
#include "Fallout/Interface/Menus/TraitMenu.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"

#include <cstdint>

TESObjectREFR* GetDialogMenuParterRef() {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->bFirstInit) {
        DialogMenu* pDialogMenu = nullptr;
        Tile* pMenuTile = Menu::GetMenuByClass(1009);
        if (pMenuTile) {
            Menu* pMenu = pMenuTile->GetMenu();
            pDialogMenu = dynamic_cast<DialogMenu*>(pMenu);
        }
        if (pDialogMenu) {
            return pDialogMenu->pSpeaker;
        }
    }

    return nullptr;
}

DialogMenu::DialogMenu()
  : Menu(), eDialogState(eCameraZoomIn), eGoodbyeState(eInDialogue), pSpeaker(nullptr), bResponseShown(false),
    fElapsedSecs(0.0f), uiIgnoreInputStartTime(0), pDepthOfFieldInstance(nullptr), bIsIn3rdPerson(false),
    bPaused(false), bInServiceMenu(false), bShowingTutorial(false), bDoServiceLoad(false) {
    DialogMenu::bDialogActive = true;
    pTopicManager = MenuTopicManager::Instance();
    strCurrTopicID[0] = 0;
    DialogMenu::fPackagePercentZoom = 100.0f;
}

DialogMenu::~DialogMenu() {
    DialogMenu::bDialogActive = false;
    if (DialogMenu::pMe->pDepthOfFieldInstance) {
        ImageSpaceModifierInstance::Stop(pDepthOfFieldInstance);
        pDepthOfFieldInstance = nullptr;
    }
    MenuTopicManager::Destroy();
    DialogMenu::pMe = nullptr;
}

void DialogMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
    if (static_cast<uint32_t>(aiID) < 4) {
        pTiles[aiID] = apTile;
    }
}

void DialogMenu::DoClick(int32_t aiID, Tile* apTarget) {
    bool bSpeechChallenge = false;
    if (eDialogState && eDialogState != DialogMenu::eCameraZoomOut) {
        if (uiIgnoreInputStartTime) {
            if (appTimer.iLastTime - uiIgnoreInputStartTime < 500) {
                return;
            }
            uiIgnoreInputStartTime = 0;
        }
        if (aiID == -1 && lbResponseList.IsListItem(apTarget)) {
            double dTopicID = apTarget->GetFloat(4012);
            if (pTopicManager->GoToTopic(static_cast<int32_t>(dTopicID))) {
                MenuTopic* pTopic = pTopicManager->GetCurrentTopic();
                bool bNewTopic = pTopic->bTopicIsNew;
                pTopic->bTopicIsNew = false;
                if (pTopic->pInfo->IsSpeechChallenge()) {
                    bSpeechChallenge = true;
                    if (pTopicManager->DoSpeechChallengeCheck(pTopic)) {
                        pTopic = pTopicManager->GetCurrentTopic();
                    }
                    else {
                        if (static_cast<int32_t>(DialogMenu::pMe->pTopicManager->iLastDialogXP) > 0) {
                            pPlayer->RewardExperience(DialogMenu::pMe->pTopicManager->iLastDialogXP);
                        }
                        MiscStatManager::Increment(MiscStatManager::MS_SPEECH_SUCCESSES);
                    }
                }
                else if (pTopic->pInfo->GetActorValue() || pTopic->pInfo->GetPerk()) {
                    BSSimpleList<void*>* pCondItemList = &pTopic->pInfo->objConditions;
                    for (; pCondItemList && pCondItemList->m_item; pCondItemList = pCondItemList->m_pkNext) {
                        TESConditionItem* pItem = static_cast<TESConditionItem*>(pCondItemList->m_item);
                        FunctionData* pfnData = &pItem->Data.kFunctionData;
                        if (pfnData && pfnData->usFunction == 14) {
                            ActorValue::Index eActorValueIndex = static_cast<ActorValue::Index>(
                                pfnData->uParam[0].uiNumber
                            );
                            int32_t iReqActorValue = static_cast<int32_t>(pItem->GetComparisonValue());
                            if (pPlayer->GetClampedActorValue(eActorValueIndex) >= iReqActorValue) {
                                MiscStatManager::Increment(MiscStatManager::MS_SPEECH_SUCCESSES);
                            }
                            else if (bNewTopic) {
                                MiscStatManager::Increment(MiscStatManager::MS_SPEECH_FAILURES);
                            }
                            break;
                        }
                    }
                }
                SetGoodbyeState(pTopic);
                pTopicManager->RunBeginScript();
                bResponseShown = false;
                DialogMenu::pLastClickedInfo = pTopic->pInfo;
                eDialogState = LoadNextScreen(bSpeechChallenge);
                return;
            }
        }
        if (pMenu->IsTrue(DialogMenu::iUT_ShowingText) && !aiID) {
            eDialogState = DialogMenu::eLoadSpeech;
        }
    }
}

bool DialogMenu::DoGamepad(int32_t aiButton, float afValue) {
    if (!Interface::IsTopMenuID(Interface::eDialogMenu) || aiButton != 9) {
        return false;
    }
    if (pMenu->IsTrue(DialogMenu::iUT_ShowingText)) {
        DoClick(0, nullptr);
    }
    else {
        Tile* pHighlighted = static_cast<Tile*>(lbResponseList.GetHighlightedItem());
        DoClick(-1, pHighlighted);
    }
    return true;
}

void DialogMenu::DoIdle() {
    if (!bPaused && !Interface::IsConsoleVisible()) {
        if (eDialogState != DialogMenu::eCameraZoomOut) {
            if (!pSpeaker->pCurrentProcess ||
                pSpeaker->pCurrentProcess->GetFadeState() == BaseProcess::FADE_OUT ||
                pSpeaker->pCurrentProcess->GetFadeState() >= BaseProcess::FADE_TELEPORT_OUT) {
                DialogMenu::Close();
                fElapsedSecs = 0.0f;
                eDialogState = DialogMenu::eCameraZoomOut;
            }
        }
        if (pSpeaker->pCurrentProcess &&
            pSpeaker->pCurrentProcess->eLevel &&
            eDialogState != DialogMenu::eCameraZoomOut) {
            DialogMenu::Close();
            fElapsedSecs = 0.0f;
            eDialogState = DialogMenu::eCameraZoomOut;
        }

        switch (eDialogState) {
            case DialogMenu::eCameraZoomIn: {
                if (bShowingTutorial && fElapsedSecs == 0.0) {
                    uint32_t uiDuration = 1400 * static_cast<int64_t>(fDialogZoomInSeconds.Float());
                    int32_t iMenuClass = DialogMenu::GetClassStatic();
                    bShowingTutorial = InterfaceManager::Instance()->xTutorialManager.ShowMessage(
                        Interface::TUT_DIALOGUE,
                        iMenuClass,
                        uiDuration
                    );
                    if (!bShowingTutorial) {
                        if (pSpeaker) {
                            pSpeaker->bSpeakingDone = true;
                        }
                        LoadNextScreen(false);
                    }
                }
                else if (!bShowingTutorial && fPercentZoomed >= 0.1) {
                    pMenu->Set(DialogMenu::iUT_DialogVisible, 1);
                }
                fElapsedSecs = appTimer.fDelta + fElapsedSecs;
                float fZoomRatio = fElapsedSecs / fDialogZoomInSeconds.Float();
                fPercentZoomed = BSMIN(fZoomRatio, 1.0);
                if (fPercentZoomed == 1.0) {
                    if (bShowingTutorial) {
                        if (InterfaceManager::Instance()->xTutorialManager.HasBeenShown(Interface::TUT_DIALOGUE) &&
                            Interface::IsTopMenuID(Interface::eDialogMenu)) {
                            fElapsedSecs = 0.0f;
                            pMenu->Set(DialogMenu::iUT_DialogVisible, 1);
                            if (pSpeaker) {
                                pSpeaker->bSpeakingDone = true;
                            }
                            LoadNextScreen(false);
                            eDialogState = DialogMenu::ePlaySpeech;
                        }
                    }
                    else {
                        fElapsedSecs = 0.0f;
                        eDialogState = DialogMenu::ePlaySpeech;
                    }
                }
                break;
            }
            case DialogMenu::eLoadSpeech: {
                if (pSpeaker->bSpeakingDone) {
                    fElapsedSecs = 0.0f;
                    eDialogState = LoadNextScreen(false);
                    iStopSpeechStartTime = 0;
                }
                else {
                    pSpeaker->StopCurrentDialogue();
                    if (!iStopSpeechStartTime) {
                        iStopSpeechStartTime = appTimer.iLastTime;
                    }
                    if (static_cast<double>(appTimer.iLastTime - iStopSpeechStartTime) >= 500.0) {
                        // The callback's context is the speaker's form ID, as Actor's speech callbacks pass it.
                        void* pContext = reinterpret_cast<void*>(static_cast<uintptr_t>(pSpeaker->GetFormID()));
                        Actor::ChangeDialogueAnimation(pContext, true);
                    }
                }
                break;
            }
            case DialogMenu::eSpeechIdle: {
                if (bInServiceMenu && bDoServiceLoad) {
                    eDialogState = DialogMenu::eServiceFadeOut;
                }
                break;
            }
            case DialogMenu::ePlaySpeech: {
                if (DialogMenu::fUseTimerNoVoice > 0.0) {
                    DialogMenu::fUseTimerNoVoice = DialogMenu::fUseTimerNoVoice - appTimer.fDelta;
                }
                else if (pSpeaker && pSpeaker->bSpeakingDone) {
                    fElapsedSecs = 0.0f;
                    eDialogState = DialogMenu::eLoadSpeech;
                    uiIgnoreInputStartTime = appTimer.iLastTime;
                }
                break;
            }
            case DialogMenu::eCameraZoomOut: {
                fElapsedSecs = appTimer.fDelta + fElapsedSecs;
                float fZoomRatio = fElapsedSecs / fDialogZoomOutSeconds.Float();
                fPercentZoomed = 1.0 - BSMIN(fZoomRatio, 1.0f);
                if (fPercentZoomed != 0.0) {
                    break;
                }
                bVATSForce1stPerson = true;
                bVATSReturnPlayerTo1stPerson = !bIsIn3rdPerson;
                pPlayer->FocusOnActor(pSpeaker, 0.0f, true);
                pPlayer->UpdateCamera(false, false);
                pDepthOfFieldInstance->SetStrength(0.0f);
                if (pMenu) {
                    delete pMenu;
                }
                return;
            }
            case DialogMenu::eServiceFadeOut: {
                if (bDoServiceLoad && xFadeState == Interface::FADE_HIDDEN) {
                    LoadNextScreen(false);
                    bDoServiceLoad = false;
                    eDialogState = DialogMenu::eSpeechIdle;
                }
                break;
            }
            default: {
                break;
            }
        }
        if (pPlayer->IsInDialoguewithPlayer()) {
            pSpeaker->UpdateInDialogue(appTimer.fDelta, nullptr, false);
        }
        double dZoomScale = DialogMenu::fPackagePercentZoom == 0.0 ? 1.0 : DialogMenu::fPackagePercentZoom / 100.0;
        float fZoom = fPercentZoomed * dZoomScale;
        pPlayer->FocusOnActor(pSpeaker, fZoom, eDialogState == DialogMenu::eCameraZoomOut);
        pPlayer->UpdateCamera(false, false);
        float fStrength = fDialogFocalDepthStrength.Float() * fPercentZoomed;
        pDepthOfFieldInstance->SetStrength(fStrength);
    }
}

bool DialogMenu::IsWellFormedMenu() {
    for (int32_t i = 0; i < 4; ++i) {
        if (!pTiles[i]) {
            return false;
        }
    }
    return true;
}

void DialogMenu::SetPause(bool abPause) {
    if (!bPaused || abPause) {
        if (!bPaused && abPause) {
            hSpeechFile.Pause();
            bPaused = true;
            ePausedState = eDialogState;
            eDialogState = eSpeechIdle;
        }
    }
    else {
        hSpeechFile.Play(false);
        bPaused = false;
        eDialogState = ePausedState;
    }
}

void DialogMenu::SetGoodbyeState(MenuTopic* apTopic) {
    if (apTopic->GetInfo()->IsGoodbye()) {
        eGoodbyeState = eForceGoodbye;
    }
    else if (apTopic->GetTopic() == TESTopic::GetTopic(DT_CONVERSATION, 2)) {
        eGoodbyeState = eNaturalGoodbye;
    }
}

void DialogMenu::LoadTopicsList() {
    // RemoveAll is called through the list's vtable; a call on the member itself would be devirtualised, so it goes
    // through a pointer.
    ListBox<int32_t>* pResponseList = &lbResponseList;
    pResponseList->RemoveAll();
    int32_t iIndex = 0;
    int32_t iListHeight = 0;
    bool bMore = pTopicManager->FirstTopic(true);
    while (bMore) {
        TESFullName* pTopicSkill;
        char acText[516];
        MenuTopic* pCurrTopic = pTopicManager->GetCurrentTopic();
        bool bSpeechFailure = false;
        if (pCurrTopic->pInfo->GetActorValue()) {
            ActorValueInfo* pActorValue = pCurrTopic->pInfo->GetActorValue();
            pTopicSkill = static_cast<TESFullName*>(pActorValue);
        }
        else {
            BGSPerk* pPerk = pCurrTopic->pInfo->GetPerk();
            pTopicSkill = static_cast<TESFullName*>(pPerk);
        }
        if (pTopicSkill) {
            BSsprintf(acText, 512, "[%s", pTopicSkill->GetFullName());
            if (pCurrTopic->pInfo->IsSpeechChallenge()) {
                float fDisposition;
                if (pSpeaker->IsActor()) {}
                else if (pSpeaker->IsTalkingActivator()) {}
                if (pSpeaker->IsActor()) {
                    fDisposition = static_cast<float>(static_cast<Actor*>(pSpeaker)->GetDisposition(pPlayer, nullptr));
                }
                else {
                    fDisposition = fSpeechChallengeDispositionBase.Float();
                }
                uint32_t uiDifficultyMult = pCurrTopic->pInfo->GetSpeechChallengeDifficultyMult();
                float fActorValue = pPlayer->GetClampedActorFloatValue(ActorValue::SPEECH);
                BSsprintf(
                    acText,
                    512,
                    "%s, %i",
                    acText,
                    GamePlayFormulas::GetSpeechChallengeChance(fActorValue, fDisposition, uiDifficultyMult)
                );
                BSstrcat(acText, 512, "%");
            }
            else {
                BSSimpleList<void*>* pCondItemList = &pCurrTopic->pInfo->objConditions;
                for (; pCondItemList && pCondItemList->m_item; pCondItemList = pCondItemList->m_pkNext) {
                    TESConditionItem* pItem = static_cast<TESConditionItem*>(pCondItemList->m_item);
                    FunctionData* pfnData = &pItem->Data.kFunctionData;
                    if (pfnData && pfnData->usFunction == 14) {
                        ActorValue::Index eActorValueIndex = static_cast<ActorValue::Index>(
                            pfnData->uParam[0].uiNumber
                        );
                        int32_t iReqActorValue = static_cast<int32_t>(pItem->GetComparisonValue());
                        int32_t iPlayerActorValue = pPlayer->GetClampedActorValue(eActorValueIndex);
                        if (iPlayerActorValue >= iReqActorValue) {
                            BSsprintf(acText, 512, "%s %d", acText, iReqActorValue);
                        }
                        else {
                            BSsprintf(acText, 512, "%s %d/%d", acText, iPlayerActorValue, iReqActorValue);
                            bSpeechFailure = true;
                        }
                        break;
                    }
                }
            }
            BSsprintf(acText, 512, "%s]  %s", acText, pCurrTopic->GetTopicText());
        }
        else if (pCurrTopic->GetTopicText()) {
            BSstrcpy(acText, 512, pCurrTopic->GetTopicText());
        }
        else {
            acText[0] = 0;
            const char* pEditorID = pCurrTopic->pTopic ? pCurrTopic->pTopic->GetFormEditorID() : "unknown";
            Warning("ERROR: DialogMenu::LoadTopicsList(): has a topic without topictext OR info prompt: %s", pEditorID);
        }
        Tile* pTile = lbResponseList.AddItem(iIndex, acText, nullptr, nullptr);
        BSsprintf(acText, 512, "topic_%i", iIndex);
        pTile->SetName(static_cast<const char*>(acText));
        pTile->Set(4012, iIndex);
        int32_t iAlpha = !pCurrTopic->bTopicIsNew || pCurrTopic->pInfo->IsAlwaysDarkened() ? 128 : 255;
        pTile->Set(Tile::TextToTrait("_line_alpha"), iAlpha);
        if (bSpeechFailure) {
            pTile->SetSpeechChallengeFailure(true);
        }
        if (iIndex <= 2) {
            iListHeight += static_cast<int32_t>(pTile->GetFloat(4016));
        }
        bMore = pTopicManager->NextTopic();
        ++iIndex;
    }
    pTiles[3]->Set(4016, static_cast<float>(iListHeight), false);
    int32_t iScrollbarHeightTrait = Tile::TextToTrait("_ScrollbarHeight");
    float fScrollbarHeight = pMenu->GetFloat(iScrollbarHeightTrait);
    int32_t iHeightTrait = Tile::TextToTrait("_height");
    lbResponseList.pScrollBar->Set(iHeightTrait, fScrollbarHeight, true);
    lbResponseList.pScrollBar->Set(g_dialogMenu_TraitCurrentValue, 0.0f, false);
    lbResponseList.Synchronize();
    uiIgnoreInputStartTime = appTimer.iLastTime;
}

DialogMenu::DialogState DialogMenu::LoadNextScreen(bool abShowChallengePrefix) {
    bool bDispositionFailure;
    MenuTopic* pTopic = pTopicManager->GetCurrentTopic();
    DialogueResponse* pResponse = pTopic ? pTopic->GetCurrentResponse() : nullptr;
    DialogMenu::DialogState eNextState = DialogMenu::eSpeechIdle;
    if (pTopic) {
        TESTopic* pDialogTopic = pTopic->pTopic;
        int32_t iFormID = pTopic->pTopic->iFormID;
        BSsprintf(strCurrTopicID, 128, "%s (%d)", pDialogTopic->GetFormEditorID(), iFormID);
    }
    else {
        BSstrcpy(strCurrTopicID, 128, "----");
    }
    if (pResponse) {
        bResponseShown = true;
        pTopic->NextResponse();
        this->pSpeaker->UpdateInDialogue(0.0f, pResponse, true);
        hSpeechFile = this->pSpeaker->pCurrentProcess->GetSoundHandle(ACTOR_SOUND_HANDLE_VOICE);
        if (!bInServiceMenu) {
            char strTemp[512];
            if (abShowChallengePrefix) {
                if (pTopicManager->bSpeechChallengeLoss) {
                    if (pResponse->cText.pString) {
                        BSsprintf(strTemp, 512, "%s %s", sSpeechChallengeFailure.String(), pResponse->cText.pString);
                    }
                    else {
                        BSsprintf(strTemp, 512, "%s", sSpeechChallengeFailure.String());
                    }
                }
                else if (pResponse->cText.pString) {
                    BSsprintf(strTemp, 512, "%s %s", sSpeechChallengeSuccess.String(), pResponse->cText.pString);
                }
                else {
                    BSsprintf(strTemp, 512, "%s", sSpeechChallengeSuccess.String());
                }
            }
            else if (pResponse->cText.pString) {
                BSstrcpy(strTemp, 512, pResponse->cText.pString);
            }
            else {
                BSstrcpy(strTemp, 512, " ");
            }
            pTiles[2]->Set(4036, strTemp, true);
            pMenu->Set(DialogMenu::iUT_ShowingText, 1);
            eNextState = DialogMenu::ePlaySpeech;
        }
    }
    else {
        if (!bResponseShown) {
            const char* pName = pTopic ? pTopic->pTopic->GetFullName() : "NULL";
            Warning("MENUS: (Dialog Menu) Topic %s has no responses, which shouldn't happen.", pName);
        }
        if (DialogMenu::pLastClickedInfo &&
            eGoodbyeState != DialogMenu::eForceGoodbye &&
            !DialogMenu::pLastClickedInfo->IsRunImmediately()) {
            DialogMenu::pLastClickedInfo->RunResult(TIRS_END, this->pSpeaker);
            TESTopicInfo* pInfo = TESTopic::GetMatchingFollowUpInfo(
                DialogMenu::pLastClickedInfo,
                bDispositionFailure,
                pTopicManager->pSpeaker,
                pPlayer,
                false,
                nullptr
            );
            if (pInfo && pTopic) {
                BSSimpleList<MenuTopic*>* pNewList = new BSSimpleList<MenuTopic*>;
                MenuTopic* pNewTopic = new MenuTopic(pTopic->pQuest, pTopic->pTopic, pInfo, this->pSpeaker, false);
                pNewTopic->FirstResponse();
                MenuTopic* pGreeting = pTopicManager->GetGreeting();
                pNewList->AddTail(pGreeting);
                pNewList->AddTail(pNewTopic);
                pTopicManager->PushTopicList(pNewList);
                pTopicManager->FirstTopic(true);
                if (pNewTopic->pInfo->IsSpeechChallenge()) {
                    if (pTopicManager->DoSpeechChallengeCheck(pNewTopic)) {
                        pNewTopic = pTopicManager->GetCurrentTopic();
                    }
                    else {
                        if (static_cast<int32_t>(pTopicManager->iLastDialogXP) > 0) {
                            pPlayer->RewardExperience(pTopicManager->iLastDialogXP);
                        }
                        MiscStatManager::Increment(MiscStatManager::MS_SPEECH_SUCCESSES);
                    }
                }
                else if (pNewTopic->pInfo->GetActorValue() || pNewTopic->pInfo->GetPerk()) {
                    BSSimpleList<void*>* pCondItemList = &pNewTopic->pInfo->objConditions;
                    for (; pCondItemList && pCondItemList->m_item; pCondItemList = pCondItemList->m_pkNext) {
                        TESConditionItem* pItem = static_cast<TESConditionItem*>(pCondItemList->m_item);
                        FunctionData* pfnData = &pItem->Data.kFunctionData;
                        if (pfnData && pfnData->usFunction == 14) {
                            ActorValue::Index eActorValueIndex = static_cast<ActorValue::Index>(
                                pfnData->uParam[0].uiNumber
                            );
                            int32_t iReqActorValue = static_cast<int32_t>(pItem->GetComparisonValue());
                            int32_t iPlayerActorValue = pPlayer->GetClampedActorValue(eActorValueIndex);
                            if (iPlayerActorValue >= iReqActorValue) {
                                MiscStatManager::Increment(MiscStatManager::MS_SPEECH_SUCCESSES);
                            }
                            else {
                                MiscStatManager::Increment(MiscStatManager::MS_SPEECH_FAILURES);
                            }
                            break;
                        }
                    }
                }
                SetGoodbyeState(pNewTopic);
                pTopicManager->RunBeginScript();
                bResponseShown = false;
                DialogMenu::pLastClickedInfo = pNewTopic->pInfo;
                // Passes the parameter's byte on zero-extended.
                return LoadNextScreen(static_cast<uint8_t>(abShowChallengePrefix) != 0);
            }
            DialogMenu::pLastClickedInfo = nullptr;
        }
        if (!bInServiceMenu || eDialogState == DialogMenu::eServiceFadeOut) {
            pTopicManager->Update();
            if (!pTopicManager->FirstTopic(true) && eGoodbyeState == DialogMenu::eInDialogue) {
                eGoodbyeState = DialogMenu::eInvalidChoiceList;
            }
            if (eGoodbyeState) {
                DialogMenu::Close();
                fElapsedSecs = 0.0f;
                eNextState = DialogMenu::eCameraZoomOut;
                eDialogState = DialogMenu::eCameraZoomOut;
            }
            else {
                pMenu->Set(DialogMenu::iUT_ShowingText, 0);
                LoadTopicsList();
                eNextState = DialogMenu::eSpeechIdle;
            }
        }
    }
    if (bPaused) {
        ePausedState = eNextState;
    }
    return eNextState;
}

void DialogMenu::ServiceStart() {
    if (DialogMenu::pMe && !DialogMenu::pMe->eGoodbyeState) {
        DialogMenu::pMe->bInServiceMenu = true;
        DialogMenu::pMe->bDoServiceLoad = true;
        if (DialogMenu::pMe->pTopicManager->GetCurrentTopic()) {
            MenuTopic* pCurrentTopic = DialogMenu::pMe->pTopicManager->GetCurrentTopic();
            DialogueResponse* pCurrentResponse = pCurrentTopic->GetCurrentResponse();
            DialogMenu::pMe->pSpeaker->UpdateInDialogue(0.0f, pCurrentResponse, false);
            DialogMenu::pMe->pSpeaker->bSpeakingDone = true;
        }
        DialogMenu::pMe->StartFadeOut();
    }
}

MobileObject* DialogMenu::GetDialogSpeaker() {
    if (pMe) {
        return pMe->GetSpeaker();
    }
    return nullptr;
}

bool DialogMenu::GetPaused() {
    return pMe && pMe->bPaused;
}

Tile* DialogMenu::Create(MobileObject* apSpeaker, TESTopic* apGreetTopic, MobileObject* apSpeakas) {
    InterfaceManager* pin = InterfaceManager::Instance();
    Tile* pOldMenuTile = Menu::GetMenuByClass(1009);
    if (pOldMenuTile) {
        return pOldMenuTile;
    }
    if (apSpeaker) {
        Tile* pTile = pin->GetMenusRoot()->ReadFile("Data\\Menus\\Dialog\\dialog_menu.xml");
        DialogMenu::pMe = static_cast<DialogMenu*>(pTile->GetMenu());
        if (DialogMenu::pMe && DialogMenu::pMe->GetClass() == DialogMenu::GetClassStatic()) {
            DialogMenu::pMe->SetMenuTile(static_cast<TileMenu*>(pTile), false);
            if (DialogMenu::pMe->IsWellFormedMenu()) {
                if (pTile->GetFloat(4007) == 6006.0 || pTile->GetFloat(4007) == 102.0) {
                    float fMaxDepth = Tile::GetMaximumDepth();
                    pTile->Set(4013, fMaxDepth, true);
                }
                DialogMenu::pMe->pSpeaker = apSpeaker;
                ImageSpaceModifierInstanceForm::Stop(TESImageSpaceModifier::GetGetHit());
                if (apSpeaker->GetFaceNodeBiped(nullptr)) {
                    apSpeaker->GetFaceNodeBiped(nullptr)->SetIAmInDialogue(true);
                }
                if (apSpeaker->GetFaceNodeSkinned(nullptr)) {
                    apSpeaker->GetFaceNodeSkinned(nullptr)->SetIAmInDialogue(true);
                }
                TESPackage* pPack = apSpeaker->GetCurrentPackage();
                if (apSpeaker->IsActor()) {
                    TESPackage* pEditorPack = apSpeaker->m_Extra.GetPackageExtra();
                    if (pEditorPack && pEditorPack->GetPackType() == PACKAGE_TYPE_DIALOGUE) {
                        DialogMenu::fPackagePercentZoom = pEditorPack->GetDialogueFOV();
                    }
                }
                if (!pPack ||
                    pPack->GetPackType() != PACKAGE_TYPE_IN_GAME_DIALOGUE ||
                    apSpeaker->GetCurrentTarget() != pPlayer) {
                    if (apSpeaker->IsActor() && !apSpeaker->IsInCombat(true)) {
                        static_cast<Actor*>(apSpeaker)->Actor::EndInterruptPackage(false);
                    }
                    if (apSpeaker->IsInCombat(true)) {
                        static_cast<Actor*>(apSpeaker)->Actor::StopMoving();
                    }
                    apSpeaker->InitiateDialogue(pPlayer, nullptr, nullptr, true, false, true, nullptr, false, false);
                }
                MobileObject* pSpeak = apSpeaker;
                if (apSpeakas) {
                    pSpeak = apSpeakas;
                }
                if (DialogMenu::pMe->pTopicManager->Initialize(pSpeak, apGreetTopic)) {
                    MenuTopic* pGreeting = DialogMenu::pMe->pTopicManager->GetGreeting();
                    DialogMenu::pMe->SetGoodbyeState(pGreeting);
                }
                if (DialogMenu::pMe->pTopicManager->GetGreeting()) {
                    DialogMenu::pLastClickedInfo = DialogMenu::pMe->pTopicManager->GetGreeting()->pInfo;
                }
                pPlayer->SetDialoguewithPlayer(true);
                if (apSpeaker->IsActor()) {
                    apSpeaker->pCurrentProcess->AddToProcedureIndexRunning(apSpeaker, 2);
                    apSpeaker->SetDialoguewithPlayer(true);
                }
                DialogMenu::pMe->bIsIn3rdPerson = pPlayer->Is3rdPerson();
                if (DialogMenu::pMe->eGoodbyeState != DialogMenu::eForceGoodbye) {
                    pPlayer->Show1stPerson(true);
                    pPlayer->FocusOnActor(apSpeaker, 0.0f, true);
                }
                if (!DialogMenu::pMe->pDepthOfFieldInstance) {
                    const NiPoint3& kPlayerPos = pPlayer->GetLocationOnReference();
                    const NiPoint3& kSpeakerPos = apSpeaker->GetLocationOnReference();
                    float fDistance = (kSpeakerPos - kPlayerPos).Length();
                    DialogMenu::pMe->pDepthOfFieldInstance = ImageSpaceModifierInstanceDOF::Trigger(
                        fDistance,
                        fDialogFocalDepthRange.Float(),
                        ImageSpaceModifierInstanceDOF::DOF_BACK,
                        0.0f,
                        0.0f,
                        0.0f
                    );
                    DialogMenu::pMe->pDepthOfFieldInstance->SetPermanent(true);
                }
                pPlayer->SetIronSights(false, false, false);
                pPlayer->StopIronSightsImageSpace();
                DialogMenu::pMe->lbResponseList.Initialize(DialogMenu::pMe->pTiles[3], "DM_TopicTemplate");
                DialogMenu::pMe->pTiles[3]->GetChildByName("lb_scrollbar")->Set(4002, 40);
                DialogMenu::pMe->pTiles[3]->GetChildByName("lb_highlight_box")->Set(4001, 40);
                DialogMenu::pMe->pTiles[3]->GetChildByName("lb_highlight_box")->Set(4017, 960);
                DialogMenu::iUT_DialogVisible = Tile::TextToTrait("_DialogVisible");
                DialogMenu::iUT_ShowingText = Tile::TextToTrait("_ShowingText");
                g_dialogMenu_TraitCurrentValue = Tile::TextToTrait("_current_value");
                bool bShowSubtitles = gs_bDialogueSubtitles_GamePlay.Bool() ||
                    (apSpeaker->IsActor() &&
                     static_cast<uint8_t>(static_cast<Actor*>(apSpeaker)->Actor::GetShowSubtitles()));
                int32_t iShowSubtitlesTrait = Tile::TextToTrait("_ShowSubtitles");
                DialogMenu::pMe->pMenu->Set(iShowSubtitlesTrait, static_cast<int32_t>(bShowSubtitles));
                const char* pFullName = apSpeaker->GetFullName();
                DialogMenu::pMe->pTiles[1]->Set(4036, pFullName, true);
                if (!ReturnTrue() ||
                    InterfaceManager::Instance()->xTutorialManager.HasBeenShown(Interface::TUT_DIALOGUE)) {
                    DialogMenu::pMe->LoadNextScreen(false);
                }
                else {
                    DialogMenu::pMe->bShowingTutorial = true;
                }
                DialogMenu::pMe->PrepForVisibility(false);
                FalloutAudio::MuteRadio();
                BSAudio::QInstance().EnterDialogue();
                apSpeaker->SetHasSpokenFlag(true);
                Interface::SetGunScopeVisible(false);
                return pTile;
            }

            Warning("MENUS: Dialogue Menu Creation Failed... Are your menu and art resources up to date?");
            return nullptr;
        }

        if (DialogMenu::pMe && DialogMenu::pMe->pMenu && DialogMenu::pMe) {
            delete DialogMenu::pMe;
        }

        return nullptr;
    }

    Warning(
        "MENUS: DialogMenu::Create() called with a null speaker, which shouldn't happen.  Notify your friendly "
        "Interface programmer!"
    );
    return nullptr;
}

bool DialogMenu::IsDialogActive() {
    return DialogMenu::bDialogActive;
}

void DialogMenu::ServiceEnd(TESTopic* apTopic) {
    if (DialogMenu::pMe) {
        DialogMenu::pMe->bInServiceMenu = false;
        if (apTopic) {
            TESTopicInfo* pNewTopicInfo = apTopic->GetMatchingInfo(DialogMenu::pMe->pSpeaker, pPlayer);
            MenuTopic* pNewTopic = new MenuTopic(
                apTopic->GetOwnerQuest(pNewTopicInfo),
                apTopic,
                pNewTopicInfo,
                DialogMenu::pMe->pSpeaker,
                false
            );
            pNewTopic->FirstResponse();
            DialogueResponse* pCurrentResponse = pNewTopic->GetCurrentResponse();
            DialogMenu::pMe->pSpeaker->UpdateInDialogue(0.0f, pCurrentResponse, false);
            DialogMenu::pMe->pSpeaker->bSpeakingDone = true;
            if (pNewTopic) {
                delete pNewTopic;
            }
        }
        DialogMenu::pMe->StartFadeIn();
    }
}

void DialogMenu::Close() {
    if (DialogMenu::pMe) {
        MobileObject* pSpeaker = DialogMenu::pMe->pSpeaker;
        Actor* pActor = nullptr;
        if (pSpeaker->IsActor()) {
            pActor = static_cast<Actor*>(pSpeaker);
        }
        TESTopicInfo* pLastInfo = DialogMenu::pLastClickedInfo;
        bool bForcedGoodbye = DialogMenu::pMe->eGoodbyeState == 2 || DialogMenu::pMe->eGoodbyeState == 3;
        if (DialogMenu::pMe->eGoodbyeState == 3) {
            Warning("MENUS: (Dialog Menu) Invalid choice list encountered.  Closing menu...");
        }
        if (pSpeaker->GetFaceNodeBiped(nullptr)) {
            pSpeaker->GetFaceNodeBiped(nullptr)->SetIAmInDialogue(false);
        }
        if (pSpeaker->GetFaceNodeSkinned(nullptr)) {
            pSpeaker->GetFaceNodeSkinned(nullptr)->SetIAmInDialogue(false);
        }
        pSpeaker->StopCurrentDialogue();
        pSpeaker->SetDialoguewithPlayer(false);
        pPlayer->SetDialoguewithPlayer(false);
        pSpeaker->EndDialogue();
        DialogMenu::bDialogActive = false;
        if (pSpeaker->pCurrentProcess) {
            pSpeaker->pCurrentProcess->SetLastSpeakingEmotion(DE_NEUTRAL);
            if (pActor && !pActor->IsInCombat()) {
                pActor->pCurrentProcess->CheckforNewPackage(pActor, true);
            }
        }
        if (pActor && pActor->pCurrentProcess) {
            pActor->pCurrentProcess->ClearDialogHeadTrackTarget(true);
        }

        if (pSpeaker->pCurrentProcess && pActor && !pActor->IsDead(false) && !pActor->IsDying()) {
            BaseProcess* pProcess = pSpeaker->pCurrentProcess;
            pProcess->SetGreetingTimer(fAIGreetingTimer.Float());
        }

        if (pActor) {
            pActor->StopMoving();
        }
        pPlayer->ResetPlayerGreetFlag();
        pPlayer->pCurrentProcess->ClearDialogHeadTrackTarget(true);
        if (pLastInfo && bForcedGoodbye && !pLastInfo->IsRunImmediately()) {
            pLastInfo->RunResult(TIRS_END, pSpeaker);
            pLastInfo->AddTopicList();
            if (pActor && !pActor->IsInCombat() && pActor->pCurrentProcess) {
                if (pActor->pCurrentProcess->CheckforNewPackage(pActor, true)) {
                    pActor->pCurrentProcess->ClearCurrentDataforProcess(pActor);
                }
            }
        }

        DialogMenu::pMe->pMenu->Set(4003, 1);
        DialogMenu::pMe->StartFadeOut();
    }
    BSAudio::QInstance().ExitDialogue();
    FalloutAudio::UnMuteRadio();
}

void DialogMenu::DebugResetSaidOnceFlags() {
    if (DialogMenu::pMe) {
        MenuTopicManager* pTopicManager = DialogMenu::pMe->pTopicManager;
        pTopicManager->FirstTopic(true);
        while (true) {
            MenuTopic* pCurrentTopic = pTopicManager->GetCurrentTopic();
            if (!pCurrentTopic) {
                break;
            }
            pCurrentTopic->bTopicIsNew = true;
            pTopicManager->NextTopic();
        }
    }
}

ActorValueInfo* TESTopicInfo::GetActorValue() {
    if (!pPerkSkillStat || pPerkSkillStat->GetFormType() == PERK_ID) {
        return nullptr;
    }
    return static_cast<ActorValueInfo*>(pPerkSkillStat);
}

BGSPerk* TESTopicInfo::GetPerk() {
    if (pPerkSkillStat && pPerkSkillStat->GetFormType() == PERK_ID) {
        return static_cast<BGSPerk*>(pPerkSkillStat);
    }
    return nullptr;
}

TESTopic* MenuTopic::GetTopic() {
    return pTopic;
}

const char* MenuTopic::GetTopicText() {
    return cTopicText.String();
}
