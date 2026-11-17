#include "Fallout/Interface/Menus/DialogMenu.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSString.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
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

#include <stdint.h>

TESObjectREFR* GetDialogMenuParterRef() {
    InterfaceManager* v0;
    Menu* Menu_;
    DialogMenu* v3;
    Tile* MenuByClass;
    if (InterfaceManager::Instance() &&
        ((v0 = InterfaceManager::Instance(), v0->bFirstInit) &&
         ((MenuByClass = Menu::GetMenuByClass(1009)) == 0
              ? v3 = 0
              : (Menu_ = MenuByClass->GetMenu(), v3 = dynamic_cast<DialogMenu*>(Menu_)),
          v3))) {
        return v3->pSpeaker;
    }
    else {
        return 0;
    }
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
    if (((uint32_t)aiID) < 4) {
        pTiles[aiID] = apTile;
    }
}

void DialogMenu::DoClick(int32_t aiID, Tile* apTarget) {
    bool bspeechChallenge = 0;
    if (eDialogState && eDialogState != DialogMenu::eCameraZoomOut) {
        double Float;
        if (uiIgnoreInputStartTime) {
            if (appTimer.iLastTime - uiIgnoreInputStartTime < 500) {
                return;
            }
            uiIgnoreInputStartTime = 0;
        }
        if (aiID != -1 ||
            (!lbResponseList.IsListItem(apTarget) ||
             (Float = apTarget->GetFloat(4012), !pTopicManager->GoToTopic(static_cast<int32_t>(Float))))) {
            if (pMenu->IsTrue(DialogMenu::iUT_ShowingText) && !aiID) {
                eDialogState = DialogMenu::eLoadSpeech;
            }
            return;
        }
        MenuTopic* ptopic = pTopicManager->GetCurrentTopic();
        bool bNewTopic = ptopic->bTopicIsNew;
        ptopic->bTopicIsNew = 0;
        if (ptopic->pInfo->IsSpeechChallenge()) {
            bspeechChallenge = 1;
            if (pTopicManager->DoSpeechChallengeCheck(ptopic)) {
                ptopic = pTopicManager->GetCurrentTopic();
                SetGoodbyeState(ptopic);
            LABEL_32:;
                pTopicManager->RunBeginScript();
                bResponseShown = 0;
                DialogMenu::pLastClickedInfo = ptopic->pInfo;
                eDialogState = LoadNextScreen(bspeechChallenge);
                return;
            }
            if (static_cast<int32_t>(DialogMenu::pMe->pTopicManager->iLastDialogXP) > 0) {
                uint32_t Time = DialogMenu::pMe->pTopicManager->iLastDialogXP;
                pPlayer->RewardExperience(Time);
            }
        LABEL_16:;
            MiscStatManager::Increment(MiscStatManager::MS_SPEECH_SUCCESSES);
        }
        else {
            if (ptopic->pInfo->GetActorValue() || ptopic->pInfo->GetPerk()) {
                BSSimpleList<void*>* pCondItemList = &ptopic->pInfo->objConditions;
                for (; pCondItemList && pCondItemList->m_item; pCondItemList = pCondItemList->m_pkNext) {
                    TESConditionItem* pItem = static_cast<TESConditionItem*>(pCondItemList->m_item);
                    FunctionData* pfnData = &pItem->Data.kFunctionData;
                    if (pfnData && pfnData->usFunction == 14) {
                        ActorValue::Index actorValueIndex = static_cast<ActorValue::Index>(pfnData->uParam[0].uiNumber);
                        int32_t reqActorValue = static_cast<int32_t>(pItem->GetComparisonValue());
                        if (pPlayer->GetClampedActorValue(actorValueIndex) >= reqActorValue) {
                            goto LABEL_16;
                        }
                        if (bNewTopic) {
                            MiscStatManager::Increment(MiscStatManager::MS_SPEECH_FAILURES);
                        }
                        break;
                    }
                }
            }
        }
        SetGoodbyeState(ptopic);
        goto LABEL_32;
    }
}

bool DialogMenu::DoGamepad(int32_t aiButton, float afValue) {
    if (!Interface::IsTopMenuID(Interface::eDialogMenu) || aiButton != 9) {
        return 0;
    }
    if (pMenu->IsTrue(DialogMenu::iUT_ShowingText)) {
        DoClick(0, 0);
    }
    else {
        Tile* v3 = static_cast<Tile*>(lbResponseList.GetHighlightedItem());
        DoClick(-1, v3);
    }
    return 1;
}

void DialogMenu::DoIdle() {
    if (!bPaused && !Interface::IsConsoleVisible()) {
        if (eDialogState != DialogMenu::eCameraZoomOut) {
            BaseProcess* v12;
            BaseProcess* CurrentAIProcess;
            if (!pSpeaker->pCurrentProcess ||
                ((CurrentAIProcess = pSpeaker->pCurrentProcess,
                  CurrentAIProcess->GetFadeState() == BaseProcess::FADE_OUT) ||
                 (v12 = pSpeaker->pCurrentProcess, v12->GetFadeState() >= BaseProcess::FADE_TELEPORT_OUT))) {
                DialogMenu::Close();
                fElapsedSecs = 0.0f;
                eDialogState = DialogMenu::eCameraZoomOut;
            }
        }
        if (pSpeaker->pCurrentProcess) {
            if (pSpeaker->pCurrentProcess->eLevel) {
                if (eDialogState != DialogMenu::eCameraZoomOut) {
                    DialogMenu::Close();
                    fElapsedSecs = 0.0f;
                    eDialogState = DialogMenu::eCameraZoomOut;
                }
            }
        }
        switch (eDialogState) {
            case DialogMenu::eCameraZoomIn: {
                if (bShowingTutorial && fElapsedSecs == 0.0) {
                    uint32_t a2 = 1400 * static_cast<int64_t>(fDialogZoomInSeconds.Float());
                    int32_t a3 = DialogMenu::GetClassStatic();
                    InterfaceManager* v1 = InterfaceManager::Instance();
                    bShowingTutorial = v1->xTutorialManager.ShowMessage(Interface::TUT_DIALOGUE, a3, a2);
                    if (!bShowingTutorial) {
                        if (pSpeaker) {
                            pSpeaker->bSpeakingDone = 1;
                        }
                        LoadNextScreen(0);
                    }
                }
                else {
                    if (!bShowingTutorial && fPercentZoomed >= 0.1) {
                        pMenu->Set(DialogMenu::iUT_DialogVisible, 1);
                    }
                }
                fElapsedSecs = appTimer.fDelta + fElapsedSecs;
                float v11 = fElapsedSecs / fDialogZoomInSeconds.Float();
                fPercentZoomed = BSMIN(v11, 1.0);
                if (fPercentZoomed == 1.0) {
                    if (bShowingTutorial) {
                        InterfaceManager* v2 = InterfaceManager::Instance();
                        if (v2->xTutorialManager.HasBeenShown(Interface::TUT_DIALOGUE) &&
                            Interface::IsTopMenuID(Interface::eDialogMenu)) {
                            fElapsedSecs = 0.0f;
                            pMenu->Set(DialogMenu::iUT_DialogVisible, 1);
                            if (pSpeaker) {
                                pSpeaker->bSpeakingDone = 1;
                            }
                            LoadNextScreen(0);
                            eDialogState = DialogMenu::ePlaySpeech;
                        }
                    }
                    else {
                        fElapsedSecs = 0.0f;
                        eDialogState = DialogMenu::ePlaySpeech;
                    }
                }
                goto LABEL_58;
            }
            case DialogMenu::eLoadSpeech: {
                if (pSpeaker->bSpeakingDone) {
                    fElapsedSecs = 0.0f;
                    eDialogState = LoadNextScreen(0);
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
                        Actor::ChangeDialogueAnimation(pContext, 1);
                    }
                }
                goto LABEL_58;
            }
            case DialogMenu::eSpeechIdle: {
                if (bInServiceMenu && bDoServiceLoad) {
                    eDialogState = DialogMenu::eServiceFadeOut;
                }
                goto LABEL_58;
            }
            case DialogMenu::ePlaySpeech: {
                if (!(DialogMenu::fUseTimerNoVoice > 0.0)) {
                    if (pSpeaker) {
                        if (pSpeaker->bSpeakingDone) {
                            fElapsedSecs = 0.0f;
                            eDialogState = DialogMenu::eLoadSpeech;
                            uiIgnoreInputStartTime = appTimer.iLastTime;
                        }
                    }
                }
                else {
                    DialogMenu::fUseTimerNoVoice = DialogMenu::fUseTimerNoVoice - appTimer.fDelta;
                }
                goto LABEL_58;
            }
            case DialogMenu::eCameraZoomOut: {
                fElapsedSecs = appTimer.fDelta + fElapsedSecs;
                float v10 = fElapsedSecs / fDialogZoomOutSeconds.Float();
                fPercentZoomed = 1.0 - BSMIN(v10, 1.0f);
                if (fPercentZoomed != 0.0) {
                    goto LABEL_58;
                }
                bVATSForce1stPerson = 1;
                bVATSReturnPlayerTo1stPerson = !bIsIn3rdPerson;
                pPlayer->FocusOnActor(pSpeaker, 0.0f, 1);
                pPlayer->UpdateCamera(0, 0);
                pDepthOfFieldInstance->SetStrength(0.0f);
                TileMenu* MenuTile = pMenu;
                if (MenuTile) {
                    delete MenuTile;
                }
                break;
            }
            case DialogMenu::eServiceFadeOut: {
                if (bDoServiceLoad && xFadeState == Interface::FADE_HIDDEN) {
                    LoadNextScreen(0);
                    bDoServiceLoad = 0;
                    eDialogState = DialogMenu::eSpeechIdle;
                }
                goto LABEL_58;
            }
            default: {
                double v9;
            LABEL_58:;
                if (pPlayer->IsInDialoguewithPlayer()) {
                    float Seconds = appTimer.fDelta;
                    pSpeaker->UpdateInDialogue(Seconds, 0, 0);
                }
                if (DialogMenu::fPackagePercentZoom == 0.0) {
                    v9 = 1.0;
                }
                else {
                    v9 = DialogMenu::fPackagePercentZoom / 100.0;
                }
                float v8 = fPercentZoomed * v9;
                pPlayer->FocusOnActor(pSpeaker, v8, eDialogState == DialogMenu::eCameraZoomOut);
                pPlayer->UpdateCamera(0, 0);
                float v7 = fDialogFocalDepthStrength.Float() * fPercentZoomed;
                pDepthOfFieldInstance->SetStrength(v7);
                break;
            }
        }
    }
}

bool DialogMenu::IsWellFormedMenu() {
    int32_t i;
    for (i = 0; i < 4; ++i) {
        if (!pTiles[i]) {
            return (bool)0;
        }
    }
    return (bool)1;
}

void DialogMenu::SetPause(bool abPause) {
    if (!bPaused || abPause) {
        if (!bPaused && abPause) {
            hSpeechFile.Pause();
            bPaused = 1;
            ePausedState = eDialogState;
            eDialogState = eSpeechIdle;
        }
    }
    else {
        hSpeechFile.Play(0);
        bPaused = 0;
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
    int32_t iindex = 0;
    int32_t ilistHeight = 0;
    bool bmore = pTopicManager->FirstTopic(1);
    while (bmore) {
        int32_t aiValue;
        TESFullName* ptopicSkill_3;
        char Dst[516];
        MenuTopic* pcurrTopic = pTopicManager->GetCurrentTopic();
        bool bSpeechFailure = 0;
        if (pcurrTopic->pInfo->GetActorValue()) {
            TESFullName* ptopicSkill;
            ActorValueInfo* ActorValue_ = pcurrTopic->pInfo->GetActorValue();
            if (ActorValue_) {
                ptopicSkill = static_cast<TESFullName*>(ActorValue_);
            }
            else {
                ptopicSkill = 0;
            }
            ptopicSkill_3 = ptopicSkill;
        }
        else {
            TESFullName* ptopicSkill_2;
            BGSPerk* Perk = pcurrTopic->pInfo->GetPerk();
            if (Perk) {
                ptopicSkill_2 = static_cast<TESFullName*>(Perk);
            }
            else {
                ptopicSkill_2 = 0;
            }
            ptopicSkill_3 = ptopicSkill_2;
        }
        if (ptopicSkill_3) {
            BSsprintf(Dst, 512, "[%s", ptopicSkill_3->GetFullName());
            if (pcurrTopic->pInfo->IsSpeechChallenge()) {
                float fdisposition;
                if (pSpeaker->IsActor()) {
                    MobileObject* pactor = pSpeaker;
                }
                else {
                    if (pSpeaker->IsTalkingActivator()) {}
                }
                if (pSpeaker->IsActor()) {
                    fdisposition = static_cast<float>(static_cast<Actor*>(pSpeaker)->GetDisposition(pPlayer, nullptr));
                }
                else {
                    fdisposition = fSpeechChallengeDispositionBase.Float();
                }
                uint32_t SpeechChallengeDifficultyMult = pcurrTopic->pInfo->GetSpeechChallengeDifficultyMult();
                float afActorValue = pPlayer->GetClampedActorFloatValue(ActorValue::SPEECH);
                BSsprintf(
                    Dst, 512, "%s, %i", Dst,
                    GamePlayFormulas::GetSpeechChallengeChance(
                        afActorValue, fdisposition, SpeechChallengeDifficultyMult
                    )
                );
                BSstrcat(Dst, 512, "%");
            }
            else {
                BSSimpleList<void*>* pCondItemList = &pcurrTopic->pInfo->objConditions;
                for (; pCondItemList && pCondItemList->m_item; pCondItemList = pCondItemList->m_pkNext) {
                    TESConditionItem* pItem = static_cast<TESConditionItem*>(pCondItemList->m_item);
                    FunctionData* pfnData = &pItem->Data.kFunctionData;
                    if (pfnData && pfnData->usFunction == 14) {
                        ActorValue::Index actorValueIndex = static_cast<ActorValue::Index>(pfnData->uParam[0].uiNumber);
                        int32_t reqActorValue = static_cast<int32_t>(pItem->GetComparisonValue());
                        int32_t playerActorValue = pPlayer->GetClampedActorValue(actorValueIndex);
                        if (playerActorValue >= reqActorValue) {
                            BSsprintf(Dst, 512, "%s %d", Dst, reqActorValue);
                        }
                        else {
                            BSsprintf(Dst, 512, "%s %d/%d", Dst, playerActorValue, reqActorValue);
                            bSpeechFailure = 1;
                        }
                        break;
                    }
                }
            }
            BSsprintf(Dst, 512, "%s]  %s", Dst, pcurrTopic->GetTopicText());
        }
        else {
            if (pcurrTopic->GetTopicText()) {
                BSstrcpy(Dst, 512, pcurrTopic->GetTopicText());
            }
            else {
                const char* v10;
                Dst[0] = 0;
                if (pcurrTopic->pTopic) {
                    v10 = pcurrTopic->pTopic->GetFormEditorID();
                }
                else {
                    v10 = "unknown";
                }
                Warning("ERROR: DialogMenu::LoadTopicsList(): has a topic without topictext OR info prompt: %s", v10);
            }
        }
        Tile* ptile = lbResponseList.AddItem(iindex, Dst, 0, 0);
        BSsprintf(Dst, 512, "topic_%i", iindex);
        ptile->SetName(static_cast<const char*>(Dst));
        ptile->Set(4012, iindex);
        if (!pcurrTopic->bTopicIsNew || pcurrTopic->pInfo->IsAlwaysDarkened()) {
            aiValue = 128;
        }
        else {
            aiValue = 255;
        }
        int32_t v3 = Tile::TextToTrait("_line_alpha");
        ptile->Set(v3, aiValue);
        if (bSpeechFailure) {
            ptile->SetSpeechChallengeFailure(1);
        }
        if (iindex <= 2) {
            ilistHeight += static_cast<int32_t>(ptile->GetFloat(4016));
        }
        bmore = pTopicManager->NextTopic();
        ++iindex;
    }
    pTiles[3]->Set(4016, static_cast<float>(ilistHeight), 0);
    int32_t v4 = Tile::TextToTrait("_ScrollbarHeight");
    float afDispositiona = pMenu->GetFloat(v4);
    int32_t v5 = Tile::TextToTrait("_height");
    lbResponseList.pScrollBar->Set(v5, afDispositiona, 1);
    lbResponseList.pScrollBar->Set(g_dialogMenu_TraitCurrentValue, 0.0f, 0);
    lbResponseList.Synchronize();
    uiIgnoreInputStartTime = appTimer.iLastTime;
}

DialogMenu::DialogState DialogMenu::LoadNextScreen(bool abShowChallengePrefix) {
    MenuTopic* pnewTopic;
    BSSimpleList<MenuTopic*>* pnewList;
    DialogueResponse* presponse;
    bool bDispositionFailure;
    MenuTopic* ptopic = pTopicManager->GetCurrentTopic();
    if (ptopic) {
        presponse = ptopic->GetCurrentResponse();
    }
    else {
        presponse = 0;
    }
    DialogMenu::DialogState enextState = DialogMenu::eSpeechIdle;
    if (ptopic) {
        TESTopic* Topic = ptopic->pTopic;
        int32_t CastingType = ptopic->pTopic->iFormID;
        BSsprintf(strCurrTopicID, 128, "%s (%d)", Topic->GetFormEditorID(), CastingType);
    }
    else {
        BSstrcpy(strCurrTopicID, 128, "----");
    }
    if (presponse) {
        BaseProcess* CurrentAIProcess;
        bool bdonotchangeanim;
        bdonotchangeanim = 0;
        bResponseShown = 1;
        ptopic->NextResponse();
        this->pSpeaker->UpdateInDialogue(0.0f, presponse, 1);
        CurrentAIProcess = this->pSpeaker->pCurrentProcess;
        hSpeechFile = CurrentAIProcess->GetSoundHandle(ACTOR_SOUND_HANDLE_VOICE);
        if (!bInServiceMenu) {
            char strtemp[512];
            if (abShowChallengePrefix) {
                if (pTopicManager->bSpeechChallengeLoss) {
                    if (presponse->cText.pString) {
                        BSsprintf(strtemp, 512, "%s %s", sSpeechChallengeFailure.String(), presponse->cText.pString);
                    }
                    else {
                        BSsprintf(strtemp, 512, "%s", sSpeechChallengeFailure.String());
                    }
                }
                else {
                    if (presponse->cText.pString) {
                        BSsprintf(strtemp, 512, "%s %s", sSpeechChallengeSuccess.String(), presponse->cText.pString);
                    }
                    else {
                        BSsprintf(strtemp, 512, "%s", sSpeechChallengeSuccess.String());
                    }
                }
            }
            else {
                if (presponse->cText.pString) {
                    BSstrcpy(strtemp, 512, presponse->cText.pString);
                }
                else {
                    BSstrcpy(strtemp, 512, " ");
                }
            }
            pTiles[2]->Set(4036, strtemp, 1);
            pMenu->Set(DialogMenu::iUT_ShowingText, 1);
            enextState = DialogMenu::ePlaySpeech;
        }
    LABEL_66:;
        if (bPaused) {
            ePausedState = enextState;
        }
        return enextState;
    }
    if (!bResponseShown) {
        const char* FullName;
        if (ptopic) {
            FullName = ptopic->pTopic->GetFullName();
        }
        else {
            FullName = "NULL";
        }
        Warning("MENUS: (Dialog Menu) Topic %s has no responses, which shouldn't happen.", FullName);
    }
    if (!DialogMenu::pLastClickedInfo ||
        (eGoodbyeState == DialogMenu::eForceGoodbye || DialogMenu::pLastClickedInfo->IsRunImmediately())) {
    LABEL_58:;
        if (!bInServiceMenu || eDialogState == DialogMenu::eServiceFadeOut) {
            pTopicManager->Update();
            if (!pTopicManager->FirstTopic(1) && eGoodbyeState == DialogMenu::eInDialogue) {
                eGoodbyeState = DialogMenu::eInvalidChoiceList;
            }
            if (eGoodbyeState) {
                DialogMenu::Close();
                fElapsedSecs = 0.0f;
                enextState = DialogMenu::eCameraZoomOut;
                eDialogState = DialogMenu::eCameraZoomOut;
            }
            else {
                pMenu->Set(DialogMenu::iUT_ShowingText, 0);
                LoadTopicsList();
                enextState = DialogMenu::eSpeechIdle;
            }
        }
        goto LABEL_66;
    }
    DialogMenu::pLastClickedInfo->RunResult(TIRS_END, this->pSpeaker);
    PlayerCharacter* v13 = pPlayer;
    TESObjectREFR* v11 = pTopicManager->pSpeaker;
    TESTopicInfo* pInfo =
        TESTopic::GetMatchingFollowUpInfo(DialogMenu::pLastClickedInfo, bDispositionFailure, v11, v13, 0, 0);
    if (!pInfo || !ptopic) {
        DialogMenu::pLastClickedInfo = 0;
        goto LABEL_58;
    }
    BSSimpleList<MenuTopic*>* v30 = static_cast<BSSimpleList<MenuTopic*>*>(ourMalloc(sizeof(BSSimpleList<MenuTopic*>)));
    if (v30) {
        pnewList = ::new (v30) BSSimpleList<MenuTopic*>;
    }
    else {
        pnewList = 0;
    }
    MenuTopic* apMemory = static_cast<MenuTopic*>(ourMalloc(sizeof(MenuTopic)));
    if (apMemory) {
        MobileObject* pSpeaker = this->pSpeaker;
        TESTopic* v12 = ptopic->pTopic;
        pnewTopic = ::new (apMemory) MenuTopic(ptopic->pQuest, v12, pInfo, pSpeaker, 0);
    }
    else {
        pnewTopic = 0;
    }
    MenuTopic* pnewTopic_3 = pnewTopic;
    pnewTopic->FirstResponse();
    MenuTopic* pnewTopic_2 = pTopicManager->GetGreeting();
    pnewList->AddTail(pnewTopic_2);
    pnewList->AddTail(pnewTopic_3);
    pTopicManager->PushTopicList(pnewList);
    pTopicManager->FirstTopic(1);
    if (pnewTopic_3->pInfo->IsSpeechChallenge()) {
        if (pTopicManager->DoSpeechChallengeCheck(pnewTopic_3)) {
            pnewTopic_3 = pTopicManager->GetCurrentTopic();
        }
        else {
            if (static_cast<int32_t>(pTopicManager->iLastDialogXP) > 0) {
                uint32_t Time = pTopicManager->iLastDialogXP;
                pPlayer->RewardExperience(Time);
            }
        LABEL_43:;
            MiscStatManager::Increment(MiscStatManager::MS_SPEECH_SUCCESSES);
        }
    }
    else {
        if (pnewTopic_3->pInfo->GetActorValue() || pnewTopic_3->pInfo->GetPerk()) {
            BSSimpleList<void*>* pCondItemList = &pnewTopic_3->pInfo->objConditions;
            for (; pCondItemList && pCondItemList->m_item; pCondItemList = pCondItemList->m_pkNext) {
                TESConditionItem* pItem = static_cast<TESConditionItem*>(pCondItemList->m_item);
                FunctionData* pfnData = &pItem->Data.kFunctionData;
                if (pfnData && pfnData->usFunction == 14) {
                    ActorValue::Index actorValueIndex = static_cast<ActorValue::Index>(pfnData->uParam[0].uiNumber);
                    int32_t reqActorValue = static_cast<int32_t>(pItem->GetComparisonValue());
                    int32_t playerActorValue = pPlayer->GetClampedActorValue(actorValueIndex);
                    if (playerActorValue >= reqActorValue) {
                        goto LABEL_43;
                    }
                    MiscStatManager::Increment(MiscStatManager::MS_SPEECH_FAILURES);
                    break;
                }
            }
        }
    }
    SetGoodbyeState(pnewTopic_3);
    pTopicManager->RunBeginScript();
    bResponseShown = 0;
    DialogMenu::pLastClickedInfo = pnewTopic_3->pInfo;
    // Passes the parameter's byte on zero-extended.
    return LoadNextScreen(static_cast<uint8_t>(abShowChallengePrefix) != 0);
}

void DialogMenu::ServiceStart() {
    MenuTopic* currentTopic;
    DialogueResponse* currentResponse;
    if (DialogMenu::pMe) {
        if (!DialogMenu::pMe->eGoodbyeState) {
            DialogMenu::pMe->bInServiceMenu = (bool)1;
            DialogMenu::pMe->bDoServiceLoad = (bool)1;
            if (DialogMenu::pMe->pTopicManager->GetCurrentTopic()) {
                currentTopic = DialogMenu::pMe->pTopicManager->GetCurrentTopic();
                currentResponse = currentTopic->GetCurrentResponse();
                DialogMenu::pMe->pSpeaker->UpdateInDialogue((float)0.0, currentResponse, (bool)0);
                DialogMenu::pMe->pSpeaker->bSpeakingDone = (bool)1;
            }
            DialogMenu::pMe->StartFadeOut();
        }
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
    Tile* pold_menu_tile = Menu::GetMenuByClass(1009);
    if (pold_menu_tile) {
        return pold_menu_tile;
    }
    if (apSpeaker) {
        int32_t v4;
        Tile* ptile = pin->GetMenusRoot()->ReadFile("Data\\Menus\\Dialog\\dialog_menu.xml");
        DialogMenu::pMe = static_cast<DialogMenu*>(ptile->GetMenu());
        if (DialogMenu::pMe && (v4 = DialogMenu::pMe->GetClass(), v4 == DialogMenu::GetClassStatic())) {
            DialogMenu::pMe->SetMenuTile(static_cast<TileMenu*>(ptile), 0);
            if (DialogMenu::pMe->IsWellFormedMenu()) {
                InterfaceManager* v17;
                if (ptile->GetFloat(4007) == 6006.0 || ptile->GetFloat(4007) == 102.0) {
                    float afMotionBlur = Tile::GetMaximumDepth();
                    ptile->Set(4013, afMotionBlur, 1);
                }
                DialogMenu::pMe->pSpeaker = apSpeaker;
                TESImageSpaceModifier* Hit = TESImageSpaceModifier::GetGetHit();
                ImageSpaceModifierInstanceForm::Stop(Hit);
                if (apSpeaker->GetFaceNodeBiped(0)) {
                    BSFaceGenNiNode* v6 = apSpeaker->GetFaceNodeBiped(0);
                    v6->SetIAmInDialogue(1);
                }
                if (apSpeaker->GetFaceNodeSkinned(0)) {
                    BSFaceGenNiNode* v7 = apSpeaker->GetFaceNodeSkinned(0);
                    v7->SetIAmInDialogue(1);
                }
                TESPackage* ppack = apSpeaker->GetCurrentPackage();
                if (apSpeaker->IsActor()) {
                    Actor* pact = static_cast<Actor*>(apSpeaker);
                    TESPackage* peditorpack = (&apSpeaker->m_Extra)->GetPackageExtra();
                    if (peditorpack) {
                        if (peditorpack->GetPackType() == PACKAGE_TYPE_DIALOGUE) {
                            DialogMenu::fPackagePercentZoom = peditorpack->GetDialogueFOV();
                        }
                    }
                }
                if (!ppack ||
                    (ppack->GetPackType() != PACKAGE_TYPE_IN_GAME_DIALOGUE ||
                     apSpeaker->GetCurrentTarget() != pPlayer)) {
                    if (apSpeaker->IsActor() && !apSpeaker->IsInCombat(1)) {
                        static_cast<Actor*>(apSpeaker)->Actor::EndInterruptPackage(0);
                    }
                    if (apSpeaker->IsInCombat(1)) {
                        static_cast<Actor*>(apSpeaker)->Actor::StopMoving();
                    }
                    apSpeaker->InitiateDialogue(pPlayer, 0, 0, 1, 0, 1, 0, 0, 0);
                }
                MobileObject* pspeak = apSpeaker;
                if (apSpeakas) {
                    pspeak = apSpeakas;
                }
                if (DialogMenu::pMe->pTopicManager->Initialize(pspeak, apGreetTopic)) {
                    MenuTopic* Greeting = DialogMenu::pMe->pTopicManager->GetGreeting();
                    DialogMenu::pMe->SetGoodbyeState(Greeting);
                }
                if (DialogMenu::pMe->pTopicManager->GetGreeting()) {
                    DialogMenu::pLastClickedInfo = DialogMenu::pMe->pTopicManager->GetGreeting()->pInfo;
                }
                pPlayer->SetDialoguewithPlayer(1);
                if (apSpeaker->IsActor()) {
                    BaseProcess* CurrentAIProcess = apSpeaker->pCurrentProcess;
                    CurrentAIProcess->AddToProcedureIndexRunning(apSpeaker, 2);
                    apSpeaker->SetDialoguewithPlayer(1);
                }
                DialogMenu::pMe->bIsIn3rdPerson = pPlayer->Is3rdPerson();
                if (DialogMenu::pMe->eGoodbyeState != DialogMenu::eForceGoodbye) {
                    pPlayer->Show1stPerson(1);
                    pPlayer->FocusOnActor(apSpeaker, 0.0f, 1);
                }
                if (!DialogMenu::pMe->pDepthOfFieldInstance) {
                    const NiPoint3* afDuration = &pPlayer->GetLocationOnReference();
                    const NiPoint3* v9 = &apSpeaker->GetLocationOnReference();
                    float fdistance = (*v9 - *afDuration).Length();
                    float* v11 = &fDialogFocalDepthRange.Float();
                    DialogMenu::pMe->pDepthOfFieldInstance = ImageSpaceModifierInstanceDOF::Trigger(
                        fdistance, *v11, ImageSpaceModifierInstanceDOF::DOF_BACK, 0.0f, 0.0f, 0.0f
                    );
                    DialogMenu::pMe->pDepthOfFieldInstance->SetPermanent(1);
                }
                pPlayer->SetIronSights(0, 0, 0);
                pPlayer->StopIronSightsImageSpace();
                DialogMenu::pMe->lbResponseList.Initialize(DialogMenu::pMe->pTiles[3], "DM_TopicTemplate");
                Tile* ChildByName = DialogMenu::pMe->pTiles[3]->GetChildByName("lb_scrollbar");
                ChildByName->Set(4002, 40);
                Tile* v13 = DialogMenu::pMe->pTiles[3]->GetChildByName("lb_highlight_box");
                v13->Set(4001, 40);
                Tile* v14 = DialogMenu::pMe->pTiles[3]->GetChildByName("lb_highlight_box");
                v14->Set(4017, 960);
                DialogMenu::iUT_DialogVisible = Tile::TextToTrait("_DialogVisible");
                DialogMenu::iUT_ShowingText = Tile::TextToTrait("_ShowingText");
                g_dialogMenu_TraitCurrentValue = Tile::TextToTrait("_current_value");
                bool v23 = gs_bDialogueSubtitles_GamePlay.Bool() ||
                    (apSpeaker->IsActor() &&
                     static_cast<uint8_t>(static_cast<Actor*>(apSpeaker)->Actor::GetShowSubtitles()));
                int32_t v15 = Tile::TextToTrait("_ShowSubtitles");
                DialogMenu::pMe->pMenu->Set(v15, static_cast<int32_t>(v23));
                const char* FullName = apSpeaker->GetFullName();
                DialogMenu::pMe->pTiles[1]->Set(4036, FullName, 1);
                if (!ReturnTrue() ||
                    (v17 = InterfaceManager::Instance(), v17->xTutorialManager.HasBeenShown(Interface::TUT_DIALOGUE))) {
                    DialogMenu::pMe->LoadNextScreen(0);
                }
                else {
                    DialogMenu::pMe->bShowingTutorial = 1;
                }
                DialogMenu::pMe->PrepForVisibility(0);
                FalloutAudio::MuteRadio();
                BSAudio* v18 = &BSAudio::QInstance();
                v18->EnterDialogue();
                apSpeaker->SetHasSpokenFlag(1);
                Interface::SetGunScopeVisible(0);
                return ptile;
            }
            else {
                Warning("MENUS: Dialogue Menu Creation Failed... Are your menu and art resources up to date?");
                return 0;
            }
        }
        else {
            if (DialogMenu::pMe) {
                if (DialogMenu::pMe->pMenu) {
                    if (DialogMenu::pMe) {
                        delete DialogMenu::pMe;
                    }
                }
            }
            return 0;
        }
    }
    else {
        Warning(
            "MENUS: DialogMenu::Create() called with a null speaker, which shouldn't happen.  Notify your friendly "
            "Interface programmer!"
        );
        return 0;
    }
}

bool DialogMenu::IsDialogActive() {
    return (bool)((int8_t)DialogMenu::bDialogActive);
}

void DialogMenu::ServiceEnd(TESTopic* apTopic) {
    if (DialogMenu::pMe) {
        DialogMenu::pMe->bInServiceMenu = 0;
        if (apTopic) {
            MenuTopic* pnewTopic;
            TESTopicInfo* pnewTopicInfo = apTopic->GetMatchingInfo(DialogMenu::pMe->pSpeaker, pPlayer);
            MenuTopic* apMemory = static_cast<MenuTopic*>(ourMalloc(sizeof(MenuTopic)));
            if (apMemory) {
                MobileObject* pSpeaker = DialogMenu::pMe->pSpeaker;
                TESQuest* OwnerQuest = apTopic->GetOwnerQuest(pnewTopicInfo);
                pnewTopic = ::new (apMemory) MenuTopic(OwnerQuest, apTopic, pnewTopicInfo, pSpeaker, 0);
            }
            else {
                pnewTopic = 0;
            }
            pnewTopic->FirstResponse();
            DialogueResponse* CurrentResponse = pnewTopic->GetCurrentResponse();
            DialogMenu::pMe->pSpeaker->UpdateInDialogue(0.0f, CurrentResponse, 0);
            DialogMenu::pMe->pSpeaker->bSpeakingDone = 1;
            if (pnewTopic) {
                delete pnewTopic;
            }
        }
        DialogMenu::pMe->StartFadeIn();
    }
}

void DialogMenu::Close() {
    if (DialogMenu::pMe) {
        MobileObject* pSpeaker = DialogMenu::pMe->pSpeaker;
        Actor* v13 = 0;
        if (pSpeaker->IsActor()) {
            v13 = static_cast<Actor*>(pSpeaker);
        }
        TESTopicInfo* v14 = DialogMenu::pLastClickedInfo;
        bool v11 = DialogMenu::pMe->eGoodbyeState == 2 || DialogMenu::pMe->eGoodbyeState == 3;
        if (DialogMenu::pMe->eGoodbyeState == 3) {
            Warning("MENUS: (Dialog Menu) Invalid choice list encountered.  Closing menu...");
        }
        if (pSpeaker->GetFaceNodeBiped(0)) {
            BSFaceGenNiNode* v0 = pSpeaker->GetFaceNodeBiped(0);
            v0->SetIAmInDialogue(0);
        }
        if (pSpeaker->GetFaceNodeSkinned(0)) {
            BSFaceGenNiNode* v1 = pSpeaker->GetFaceNodeSkinned(0);
            v1->SetIAmInDialogue(0);
        }
        pSpeaker->StopCurrentDialogue();
        pSpeaker->SetDialoguewithPlayer(0);
        pPlayer->SetDialoguewithPlayer(0);
        pSpeaker->EndDialogue();
        DialogMenu::bDialogActive = 0;
        if (pSpeaker->pCurrentProcess) {
            BaseProcess* CurrentAIProcess = pSpeaker->pCurrentProcess;
            CurrentAIProcess->SetLastSpeakingEmotion(DE_NEUTRAL);
            if (v13) {
                if (!v13->IsInCombat()) {
                    BaseProcess* v9 = v13->pCurrentProcess;
                    v9->CheckforNewPackage(v13, 1);
                }
            }
        }
        if (v13) {
            if (v13->pCurrentProcess) {
                BaseProcess* v8 = v13->pCurrentProcess;
                v8->ClearDialogHeadTrackTarget(1);
            }
        }
        if (pSpeaker->pCurrentProcess) {
            if (v13) {
                if (!v13->IsDead(0) && !v13->IsDying()) {
                    BaseProcess* v7 = pSpeaker->pCurrentProcess;
                    float* v2 = &fAIGreetingTimer.Float();
                    v7->SetGreetingTimer(*v2);
                }
            }
        }
        if (v13) {
            v13->StopMoving();
        }
        pPlayer->ResetPlayerGreetFlag();
        BaseProcess* v6 = pPlayer->pCurrentProcess;
        v6->ClearDialogHeadTrackTarget(1);
        if (v14) {
            if (v11 && !v14->IsRunImmediately()) {
                v14->RunResult(TIRS_END, pSpeaker);
                v14->AddTopicList();
                if (v13) {
                    if (!v13->IsInCombat()) {
                        if (v13->pCurrentProcess) {
                            BaseProcess* v5 = v13->pCurrentProcess;
                            if (v5->CheckforNewPackage(v13, 1)) {
                                BaseProcess* v4 = v13->pCurrentProcess;
                                v4->ClearCurrentDataforProcess(v13);
                            }
                        }
                    }
                }
            }
        }
        DialogMenu::pMe->pMenu->Set(4003, 1);
        DialogMenu::pMe->StartFadeOut();
    }
    BSAudio* v3 = &BSAudio::QInstance();
    v3->ExitDialogue();
    FalloutAudio::UnMuteRadio();
}

void DialogMenu::DebugResetSaidOnceFlags() {
    if (DialogMenu::pMe) {
        MenuTopicManager* pTopicManager = DialogMenu::pMe->pTopicManager;
        pTopicManager->FirstTopic(1);
        while (1) {
            MenuTopic* CurrentTopic = pTopicManager->GetCurrentTopic();
            if (!CurrentTopic) {
                break;
            }
            CurrentTopic->bTopicIsNew = 1;
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
