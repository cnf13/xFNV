#include "Fallout/Interface/Menus/MapMenu.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSAudio/BSWin32Audio.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSDevices/Controls.h"
#include "BSMain/BSScissorTriShape.h"
#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileImage.h"
#include "BSMenu/TileMenu.h"
#include "BSMenu/TileText.h"
#include "BSShader/TileShaderProperty.h"
#include "BSShader/WaterShader.h"
#include "Fallout Shared/BGSNote.h"
#include "Fallout Shared/BGSPickupPutdownSounds.h"
#include "Fallout Shared/BGSQuestObjective.h"
#include "Fallout Shared/BGSTalkingActivator.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/GridCell.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/Pathfinding/TeleportPath.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESChallenge.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESLeveledList.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectDOOR.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESQuest.h"
#include "Fallout Shared/TESQuestTarget.h"
#include "Fallout Shared/TESReputation.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout Shared/TeleportData.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Audio/FalloutAudio.h"
#include "Fallout/Audio/FalloutRadio.h"
#include "Fallout/Dialogue/Conversation.h"
#include "Fallout/Dialogue/DialogueItem.h"
#include "Fallout/Interface/FOPipboyManager.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/ListBox.h"
#include "Fallout/Interface/Menus/SleepWaitMenu.h"
#include "Fallout/Misc/Calendar.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SeenData.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9TextureData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "platform/Paths.h"

#include <cmath>
#include <filesystem>
#include <limits>
#include <system_error>

g_challengeStringsArray_t g_challengeStringsArray{&sChallengeKill, &sChallengeDamage, &sChallengeOther};

// Set while DoIdle has paused the holotape sound for the start menu.
static bool wasHolotapePlayingWhenGamePaused;

// DoIdle's function-local statics: set once the cursor has been shown again after leaving the menu, and the time
// the holotape sound was paused.
static bool bleftMenu;
static int32_t iSoundPlayPauseTime;

// The game settings this file defines, in their initialization order.
SettingT<INISettingCollection> bRenderLocalMapContinually("bRenderLocalMapContinually:General", false);

using NoteListBox = ListBox<BGSNote*>;

void MapMenu_RequestPlayerPositionAsSelectedMarker() {
    if (MapMenu::pMe && MapMenu::pMe->pFastTravelMarker) {
        PLAYER_TARGET_LOC* pTargetLoc;
        pTargetLoc = new PLAYER_TARGET_LOC;
        pTargetLoc->pFastTravelMarker = MapMenu::pMe->pFastTravelMarker;
        pPlayer->RequestPositionPlayer(pTargetLoc);
        MapMenu::pMe->pFastTravelMarker = nullptr;
    }
}

MapMenu::MapMenu()
  : iCurrentPage(36), pMapTile(nullptr), fWaveformAnimationStartTime(0.0f), bShowingAllNotes(true),
    pSelectedNote(nullptr), uiNoteEnterStartTime(0), pCurrentSound(nullptr), bSoundsLoading(false),
    fOldPipboyRadioVolume(static_cast<BSWin32Audio*>(&BSAudio::QInstance())->GetRadioVolume()),
    pDisplayedQuest(nullptr), pDisplayedQuestTarget(nullptr), pHighlightedMapMarker(nullptr),
    PlayerMarkerSpace(nullptr), PlayerMarkerPos(NiPoint3::ZERO), pMapSpace(nullptr), pRootMapSpace(nullptr),
    bRootSpaceChanged(false), pMapPlayerRef(nullptr), pFastTravelMarker(nullptr), pAdjustNorthRotationCell(nullptr),
    uiNextLocalMapGrid(0), bFogOfWarEnabled(true) {
    MapMenu::iUT_Title = Tile::TextToTrait("_Title");
    g_mapMenuTrait_Value = Tile::TextToTrait("_Value");
    MapMenu::iUT_CurrentTab = Tile::TextToTrait("_CurrentTab");
    MapMenu::iUT_Selected = Tile::TextToTrait("_selected");
    MapMenu::iUT_TextAlpha = Tile::TextToTrait("_TextAlpha");
    MapMenu::iUT_X = Tile::TextToTrait("_x");
    MapMenu::iUT_Y = Tile::TextToTrait("_y");
    MapMenu::iUT_Magnification = Tile::TextToTrait("_Magnification");
}

MapMenu::~MapMenu() {
    MapMenu::pMe = nullptr;
}

void MapMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
    if (static_cast<uint32_t>(aiID) <= 21) {
        pTiles[aiID] = apTile;
    }
}

void MapMenu::DoDownClick(int32_t aiID, Tile* apTarget) {
    if (aiID == 4) {
        WorldMapDragStart.x = pTiles[4]->GetFloat(4001);
        WorldMapDragStart.y = pTiles[4]->GetFloat(4002);
    }
}

void MapMenu::DoClick(int32_t aiID, Tile* apTarget) {
    NiPoint3 kMousePos;
    NiPoint3 kCenterPos;
    if (!apTarget && aiID >= 0 && aiID < NUM_TILES) {
        apTarget = pTiles[aiID];
    }
    switch (aiID) {
        case 2:
        case 4: {
            if (pHighlightedMapMarker) {
                DoClick(26, pHighlightedMapMarker);
            }
            break;
        }
        case 12: {
            switch (iCurrentPage) {
                case 32:
                case 33: {
                    if (pMapTile) {
                        if (InterfaceManager::Has360Controller()) {
                            double dHalfHeight = pMapTile->pParent->GetFloat(4016) / 2.0;
                            float fY = dHalfHeight - pMapTile->GetFloat(4002);
                            double dHalfWidth = pMapTile->pParent->GetFloat(4017) / 2.0;
                            float fX = dHalfWidth - pMapTile->GetFloat(4001);
                            PlayerMarkerPos = *::new (&kCenterPos) NiPoint3(fX, fY, 0.0f);
                        }
                        else {
                            float fMouseY = InterfaceManager::Instance()->GetPipboyMouseY();
                            float fMouseX = InterfaceManager::Instance()->GetPipboyMouseX();
                            PlayerMarkerPos = *::new (&kMousePos) NiPoint3(fMouseX, fMouseY, 0.0f);
                            PlayerMarkerPos.x = PlayerMarkerPos.x - pMapTile->GetRealX();
                            PlayerMarkerPos.y = PlayerMarkerPos.y - pMapTile->GetRealY();
                        }
                        if (iCurrentPage == 32) {
                            TESWorldSpace* pWorldSpace;
                            PlayerMarkerSpace = pMapSpace;
                            if (PlayerMarkerSpace->GetFormType() == WRLD_ID) {
                                pWorldSpace = static_cast<TESWorldSpace*>(PlayerMarkerSpace);
                            }
                            else {
                                pWorldSpace = nullptr;
                            }
                            float fMapWidth = pMapTile->GetFloat(4017);
                            NiPoint3 kMapPos = PlayerMarkerPos / fMapWidth;
                            PlayerMarkerPos = MapToWorldCoord(kMapPos, pWorldSpace);
                        }
                        else {
                            float fMapWidth = pMapTile->GetFloat(4017);
                            NiPoint3 kMapPos = PlayerMarkerPos / fMapWidth;
                            PlayerMarkerPos = MapToWorldCoord(kMapPos, nullptr);
                            PlayerMarkerSpace = CalculatePlayerMarkerSpace(PlayerMarkerPos);
                            if (PlayerMarkerSpace) {
                                static_cast<TESWorldSpace*>(PlayerMarkerSpace)
                                    ->TESWorldSpace::AdjustMapMarkerCoord(PlayerMarkerPos, true);
                            }
                        }
                        if (pAdjustNorthRotationCell) {
                            pAdjustNorthRotationCell
                                ->AdjustCoordForNorthRotation(PlayerMarkerPos, PlayerMarkerPos, false);
                        }
                        pTiles[6]->Set(4003, 0);
                        BSAudio::QInstance().GetSoundHandleByName("UIPopUpMapMarkerAdded ", 289u).Play(false);
                        if (pPlayer->QPlayerMapMarker()) {
                            const char* pLeaveText = sLeaveMarker.String();
                            const char* pRemoveText = sRemoveMarker.String();
                            const char* pMoveText = sMoveMarker.String();
                            Interface::CreateMessageMenu(
                                sMoveMarkerQuestion.String(),
                                static_cast<const char*>(nullptr),
                                static_cast<const char*>(nullptr),
                                &MapMenu::SetPlayerMarkerCallback,
                                0,
                                23,
                                0.0f,
                                0.0f,
                                pMoveText,
                                pRemoveText,
                                pLeaveText,
                                nullptr
                            );
                        }
                        else {
                            const char* pNoText = sNo.String();
                            const char* pYesText = sYes.String();
                            Interface::CreateMessageMenu(
                                sSetMarkerQuestion.String(),
                                static_cast<const char*>(nullptr),
                                static_cast<const char*>(nullptr),
                                &MapMenu::SetPlayerMarkerCallback,
                                3,
                                23,
                                0.0f,
                                0.0f,
                                pYesText,
                                pNoText,
                                nullptr
                            );
                        }
                    }
                    break;
                }
                case 34: {
                    if (pDisplayedQuestTarget) {
                        DoClick(33, nullptr);
                        TeleportPath* pTargetPath = pDisplayedQuestTarget->QTargetPath();
                        TESObjectREFR* pReference = pDisplayedQuestTarget->GetReference(true);
                        TESObjectREFR* pTargetRef = GetWorldMapTrackingRef(pReference, pTargetPath);
                        if (pTargetRef && pMapTile) {
                            TESWorldSpace* pTargetSpace = pTargetRef->GetWorldSpace();
                            NiPoint3 kTargetPos = pTargetRef->GetLocationOnReference();
                            NiPoint3 kCenterPoint = WorldToMapCoord(kTargetPos, pTargetSpace);
                            double dHalfWidth = pMapTile->pParent->GetFloat(4017) / 2.0;
                            double dWidth = pMapTile->GetFloat(4017);
                            float fX = dHalfWidth - dWidth * kCenterPoint.x;
                            pMapTile->Set(4001, fX, false);
                            double dHalfHeight = pMapTile->pParent->GetFloat(4016) / 2.0;
                            double dHeight = pMapTile->GetFloat(4016);
                            float fY = dHalfHeight - dHeight * kCenterPoint.y;
                            pMapTile->Set(4002, fY, false);
                        }
                    }
                    break;
                }
                case 35: {
                    if (MapMenu::bNotesChallengeMode) {
                        if (++MapMenu::cChallengeTypeDisplayed > 2) {
                            MapMenu::cChallengeTypeDisplayed = 0;
                        }
                        lbChallengeList.Filter(&MapMenu::ChallengeFilterFunc);
                        const char* pChallengeTypeText = g_challengeStringsArray[MapMenu::cChallengeTypeDisplayed]
                                                             ->String();
                        pChallengeType->Set(4036, pChallengeTypeText, true);
                        pTiles[20]->Set(4003, 0);
                        pTiles[13]->Set(4003, 0);
                    }
                    else {
                        bShowingAllNotes = !bShowingAllNotes;
                        lbNotesList.Filter(&MapMenu::NotesFilterFunc);
                        pTiles[13]->Set(4003, 0);
                        int32_t iShowingAllNotes = bShowingAllNotes;
                        int32_t iTrait = Tile::TextToTrait("_ShowingAllNotes");
                        pTiles[12]->Set(iTrait, iShowingAllNotes);
                    }
                    break;
                }
                default: {
                    break;
                }
            }
            break;
        }
        case 18: {
            if (iCurrentPage == 35) {
                ToggleNotesChallengeMode(true);
            }
            break;
        }
        case 23: {
            if (lbQuestsList.GetHighlightedData() && !lbQuestsList.GetHighlightedData()->GetCompleted()) {
                TESQuest* pQuest = lbQuestsList.GetHighlightedData();
                pPlayer->SetActiveQuest(pQuest);
                Interface::PlayMenuSound(3);
                UpdateCurrentPage();
            }

            break;
        }
        case 24: {
            if (!MapMenu::bNotesChallengeMode) {
                BSSoundHandle hsndHandle;
                BGSNote* pNote = lbNotesList.GetDataFor(apTarget);
                bool bAudioNote = pNote->GetNoteType() == NT_SOUND || pNote->GetNoteType() == NT_VOICE;
                if (pNote && (!InterfaceManager::Has360Controller() || bAudioNote)) {
                    bool bNoteMatch = pNote == pSelectedNote;
                    bool bSoundStopped = false;
                    Tile* pOldSelectedTile = lbNotesList.GetTileFor(pSelectedNote);
                    if (pOldSelectedTile) {
                        pOldSelectedTile->Set(MapMenu::iUT_Selected, 0);
                    }
                    if (bNoteMatch && (!bAudioNote || pCurrentSound)) {
                        pSelectedNote = nullptr;
                        pTiles[13]->Set(Tile::kTileValue_User0, 0);
                    }
                    else {
                        LoadNote(pNote);
                        apTarget->Set(MapMenu::iUT_Selected, 1);
                        pSelectedNote = pNote;
                        pTiles[13]->Set(Tile::kTileValue_User0, 1);
                    }
                    if (pCurrentSound) {
                        StopNoteSound();
                        bSoundStopped = true;
                    }
                    if (bAudioNote && !pNote->QHasBeenRead()) {
                        apTarget->Set(MapMenu::iUT_TextAlpha, 128.0f, true);
                        pNote->SetHasBeenRead(true);
                    }
                    if (bAudioNote && (!bNoteMatch || !bSoundStopped)) {
                        if (pNote->GetNoteType() || !pNote->GetNoteSound()) {
                            if (pNote->GetNoteType() == NT_VOICE && pNote->GetNoteTopic() && pNote->GetNoteSpeaker()) {
                                Conversation* pVoiceNoteConvo;
                                Character* pVoiceNoteNPC;
                                pVoiceNoteNPC = new Character(false);
                                pVoiceNoteNPC->SetTemporary();
                                TESActorBase* pSpeaker = pNote->GetNoteSpeaker();
                                pVoiceNoteNPC->SetObjectReference(pSpeaker);
                                pVoiceNoteConvo = new Conversation(pVoiceNoteNPC, pPlayer, pNote->GetNoteTopic());
                                pVoiceNoteConvo->FirstItem();
                                if (pVoiceNoteConvo->GetCurrentItem() &&
                                    pVoiceNoteConvo->GetCurrentItem()->FirstResponse()) {
                                    do {
                                        SubtitlesToShow.Add(
                                            pVoiceNoteConvo->GetCurrentItem()->GetCurrentResponse()->cText
                                        );
                                        pVoiceNoteConvo->GetCurrentItem()->pInfo->RunResult(TIRS_BEGIN, pVoiceNoteNPC);
                                        char* pVoiceFile = pVoiceNoteConvo->GetCurrentItem()
                                                               ->GetCurrentResponse()
                                                               ->GetVoice()
                                                               ->pString;
                                        hsndHandle = BSAudio::QInstance()
                                                         .GetSoundHandleByFilename(pVoiceFile, 261u, nullptr);
                                        hsndHandle.SetVolume(0.9f);
                                        SoundsToPlay.AddTail(hsndHandle);
                                        pVoiceNoteConvo->GetCurrentItem()->pInfo->RunResult(TIRS_END, pVoiceNoteNPC);
                                        bSoundsLoading = true;
                                    }
                                    while (pVoiceNoteConvo->GetCurrentItem()->NextResponse());
                                }

                                if (pVoiceNoteConvo) {
                                    delete pVoiceNoteConvo;
                                }
                                if (pVoiceNoteNPC) {
                                    delete pVoiceNoteNPC;
                                }
                            }
                        }
                        else {
                            TESSound* pSound = pNote->GetNoteSound();
                            uint32_t uiSoundID = pSound->iFormID;
                            BSSoundHandle kSoundHandle = BSAudio::QInstance().GetSoundHandleByNumericID(
                                uiSoundID,
                                257u
                            );
                            SoundsToPlay.AddTail(kSoundHandle);
                            bSoundsLoading = true;
                        }
                        if (bSoundsLoading) {
                            BSAudio::QInstance().GetSoundHandleByName("UIPipBoyHolotapeStart", 289u).Play(false);
                            FalloutAudio::MuteRadio();
                            BSAudio::QInstance().EnterDialogue();
                        }
                    }
                }
            }
            break;
        }
        case 25: {
            // Called through a ListBoxBase pointer so the call stays indirect (virtual).
            ListBoxBase* pRadioBase = &lbRadioStationsList;
            Tile* pCurrTile = static_cast<Tile*>(pRadioBase->GetHighlightedItem());
            if (pCurrTile->GetFloat(MapMenu::iUT_TextAlpha) == 255.0) {
                bool bUnselected = !pCurrTile->IsTrue(MapMenu::iUT_Selected);
                ResetWaveform();
                lbRadioStationsList.Traverse(&MapMenu::UnselectFunc, 0, std::numeric_limits<int32_t>::max());
                if (FalloutRadio::IsPipboyRadioEnabled()) {
                    FalloutRadio::PipboyRadioEnable(false);
                }
                if (bUnselected) {
                    FalloutRadio::PipboyRadioEnable(true);
                    FalloutRadio::TunePipboyToStation(lbRadioStationsList.GetHighlightedData(), false);
                    pCurrTile->Set(MapMenu::iUT_Selected, 1);
                }
                UpdateCurrentPage();
            }
            break;
        }
        case 26: {
            if (iCurrentPage == 33 &&
                (InterfaceManager::Has360Controller() ||
                 (pMapTile &&
                  WorldMapDragStart.x == pMapTile->GetFloat(4001) &&
                  WorldMapDragStart.y == pMapTile->GetFloat(4002))) &&
                pTiles[11]->IsTrue(4015) &&
                pPlayer->CanFastTravel()) {
                int32_t iMarkerCt = 0;
                for (TESObjectREFR*& rpItem : WorldMapMarkerList.ItemsUntilNull()) {
                    int32_t iTrait = Tile::AddUserTrait("_MarkerIndex", -1);
                    if (!(apTarget->GetFloat(iTrait) >= static_cast<double>(iMarkerCt))) {
                        break;
                    }
                    pFastTravelMarker = rpItem;
                    ++iMarkerCt;
                }
                MapMarkerData* pMarkerData = pFastTravelMarker->GetMapMarkerData();
                if (pMarkerData->GetTravelLoc()) {
                    char acMessage[260];
                    DoLeave(aiID, apTarget);
                    pMarkerData = pFastTravelMarker->GetMapMarkerData();
                    const char* pLocationName = pMarkerData->GetLocationName();
                    BSsprintf(acMessage, 256, "%s %s?", sTravelQuestion.String(), pLocationName);
                    pTiles[6]->Set(4003, 0);
                    const char* pNoText = sNo.String();
                    Interface::CreateMessageMenu(
                        static_cast<const char*>(acMessage),
                        static_cast<const char*>(nullptr),
                        static_cast<const char*>(nullptr),
                        &MapMenu::FastTravelCallback,
                        0,
                        23,
                        0.0f,
                        0.0f,
                        sYes.String(),
                        pNoText,
                        nullptr
                    );
                }
                else {
                    pFastTravelMarker = nullptr;
                }
            }
            else if (iCurrentPage == 33 && !pTiles[11]->IsTrue(4015)) {
                int32_t iTrait = Tile::AddUserTrait("_LocationName", -1);
                const char* pLocationName = apTarget->GetString(iTrait);
                if (_stricmp(pLocationName, "Companion")) {
                    Interface::ShowMessage(sNoFastTravelUndiscovered.String(), 2, nullptr, nullptr, 2.0f, false);
                }
            }
            break;
        }
        case 32:
        case 33:
        case 34:
        case 35:
        case 36: {
            pTiles[17]->Set(MapMenu::iUT_CurrentTab, aiID - 32);
            iCurrentPage = aiID;
            float fBurstIntensity = fMiniBurstIntensity.Float();
            float fBurstDuration = fMiniBurstDuration.Float();
            FOPipboyManager* pPipboy = InterfaceManager::Instance()->GetPipboy();
            pPipboy->TriggerBurst(fBurstDuration, fBurstIntensity, 0.5f);
            if (!Interface::Has360Controller() && aiID != 32 && aiID != 33) {
                InterfaceManager::Instance()->pCursor->Set(4009, 255.0f, true);
            }
            pTiles[13]->Set(4003, 0);
            pTiles[12]->Set(4015, 1);
            MapMenu::UpdateMenu();
            pPipboy = InterfaceManager::Instance()->GetPipboy();
            if (pPipboy->GetInitialized()) {
                MapMenu::SetKnobPosition(true);
            }
            break;
        }
        default: {
            break;
        }
    }
}

void MapMenu::DoEnter(int32_t aiID, Tile* apTarget) {
    static int32_t iPrevHighlightIndex;
    int32_t iNewHighlightIndex = 0;
    switch (aiID) {
        case -1: {
            if (apTarget) {
                if (apTarget->pParent == lbQuestObjectivesList.pScrollBar) {
                    DoEnter(15, nullptr);
                }
            }
            break;
        }
        case 14: {
            lbNotesList.SetEnabled(false);
            break;
        }
        case 15: {
            lbQuestsList.SetEnabled(false);
            lbQuestObjectivesList.SetEnabled(true);
            break;
        }
        case 23: {
            if (apTarget) {
                pDisplayedQuest = lbQuestsList.GetDataFor(apTarget);
                lbQuestObjectivesList.Filter(&MapMenu::IsQuestMapMenuSelectedQuest);
                iNewHighlightIndex = static_cast<int32_t>(apTarget->GetFloat(Tile::kTileValue_ListIndex));
                int32_t iTrait = Tile::TextToTrait("_ItemType");
                pTiles[13]->Set(iTrait, 4);
                pTiles[13]->Set(Tile::kTileValue_Visible, 1);
                pDisplayedQuestTarget = nullptr;
                lbQuestObjectivesList
                    .Traverse(&MapMenu::FindQuestObjectiveTargetFunc, 0, std::numeric_limits<int32_t>::max());
                pTiles[12]->Set(Tile::kTileValue_Target, static_cast<int32_t>(pDisplayedQuestTarget != nullptr));
                if (!pDisplayedQuest || pDisplayedQuest->GetCompleted()) {
                    pTiles[11]->Set(Tile::kTileValue_Target, 0);
                }
                else {
                    pTiles[11]->Set(Tile::kTileValue_Target, 1);
                }
            }
            break;
        }
        case 24: {
            if (MapMenu::bNotesChallengeMode) {
                TESChallenge* pCurrChallenge = lbChallengeList.GetDataFor(apTarget);
                if (InterfaceManager::Has360Controller() || pCurrChallenge) {
                    LoadChallenge(pCurrChallenge);
                }
                pTiles[13]->Set(Tile::kTileValue_Visible, 1);
                Interface::PlayMenuSound(4);
            }
            else {
                BGSNote* pCurrNote = lbNotesList.GetDataFor(apTarget);
                ListBoxBase* pNotesBase = &lbNotesList;
                if (pNotesBase->GetHighlightedItem()) {
                    Tile* pHighlightedTile = static_cast<Tile*>(pNotesBase->GetHighlightedItem());
                    iNewHighlightIndex = static_cast<int32_t>(pHighlightedTile->GetFloat(Tile::kTileValue_ListIndex));
                }
                else {
                    iNewHighlightIndex = 0;
                }
                if (InterfaceManager::Has360Controller() || (pCurrNote && !pSelectedNote)) {
                    LoadNote(pCurrNote);
                }
                if (pCurrNote->GetNoteType() && pCurrNote->GetNoteType() != NT_VOICE) {
                    if (!pCurrNote->QHasBeenRead()) {
                        uiNoteEnterStartTime = appTimer.iLastTime;
                    }
                }
                else if (pCurrNote == pSelectedNote && pCurrentSound) {
                    pTiles[13]->Set(Tile::kTileValue_User0, 1);
                }
                else {
                    pTiles[13]->Set(Tile::kTileValue_User0, 0);
                }
            }
            break;
        }
        case 25: {
            ListBoxBase* pRadioBase = &lbRadioStationsList;
            if (pRadioBase->GetHighlightedItem()) {
                Tile* pHighlightedTile = static_cast<Tile*>(pRadioBase->GetHighlightedItem());
                iNewHighlightIndex = static_cast<int32_t>(pHighlightedTile->GetFloat(Tile::kTileValue_ListIndex));
            }
            else {
                iNewHighlightIndex = 0;
            }
            break;
        }
        default: {
            break;
        }
    }
    if (iPrevHighlightIndex != iNewHighlightIndex && (aiID == 23 || aiID == 24 || aiID == 25)) {
        float fAmount;
        if (iPrevHighlightIndex >= iNewHighlightIndex) {
            fAmount = -fScrollKnobIncrement.Float();
        }
        else {
            fAmount = fScrollKnobIncrement.Float();
        }
        float fRate = fScrollKnobRate.Float();
        FOPipboyManager* pPipboy = InterfaceManager::Instance()->GetPipboy();
        pPipboy->MoveKnob(FOPipboyManager::KNOB_SCROLL, fAmount, fRate, true);
        iPrevHighlightIndex = iNewHighlightIndex;
    }
}

void MapMenu::DoLeave(int32_t aiID, Tile* apTarget) {
    switch (aiID) {
        case -1: {
            if (apTarget) {
                if (apTarget->pParent == lbQuestObjectivesList.pScrollBar) {
                    DoLeave(15, nullptr);
                }
            }
            break;
        }
        case 14: {
            lbNotesList.SetEnabled(true);
            break;
        }
        case 15: {
            lbQuestsList.SetEnabled(true);
            lbQuestObjectivesList.SetEnabled(false);
            break;
        }
        case 24: {
            if (InterfaceManager::Has360Controller() || !pSelectedNote) {
                uiNoteEnterStartTime = 0;
            }
            break;
        }
        default: {
            break;
        }
    }
}

void MapMenu::DoWheelMove(int32_t aiID, Tile* apTarget) {
    if (iCurrentPage == 33 || iCurrentPage == 32) {
        uint8_t ucZoomIn = InterfaceManager::Instance()->GetMouseWheel() / 120 > 0.0;
        ZoomMap(ucZoomIn, 1.100000023841858f);
    }
}

// The mouse position is a float that is widened to double before the parent's position is subtracted, so the
// subtraction is done in double.
void MapMenu::DoIdle() {
    using enum Tile::eTileValue;

    if (Interface::IsTopMenuID(Interface::eStartMenu) &&
        this->pCurrentSound &&
        this->pCurrentSound->GetItem().IsPlaying()) {
        this->pCurrentSound->GetItem().Pause();
        wasHolotapePlayingWhenGamePaused = true;
        iSoundPlayPauseTime = appTimer.GetTime();
    }
    else if (
        !Interface::IsTopMenuID(Interface::eStartMenu) &&
        !Interface::IsTopMenuID(Interface::eMessageMenu) &&
        this->pCurrentSound &&
        wasHolotapePlayingWhenGamePaused
    ) {
        this->pCurrentSound->GetItem().Play(false);
        wasHolotapePlayingWhenGamePaused = false;
        this->iSoundPlayStartTime += appTimer.GetTime() - iSoundPlayPauseTime;
    }

    if (!Interface::GetPipboy()->GetInitialized()) {
        return;
    }
    if (Interface::IsTopMenuID(Interface::ePipBoyData)) {
        switch (MapMenu::pMe->iCurrentPage) {
            case TI_TAB_LOCALMAP:
                if (bRenderLocalMapContinually.Bool() && Interface::GetPipboy()->IsMenuRendering()) {
                    MapMenu::pMe->uiNextLocalMapGrid = 0;
                    for (uint32_t i = 0; i < 25; ++i) {
                        MapMenu::RenderLocalMapGrid(false);
                    }
                }
                [[fallthrough]];
            case TI_TAB_WORLDMAP:
                if (!InterfaceManager::Has360Controller()) {
                    // The highlight follows the mouse; the cursor hides while it is over the map.
                    const double dMouseX = InterfaceManager::Instance()->GetPipboyMouseX();
                    float fHighlightX = dMouseX - this->pTiles[TI_MAP_HIGHLIGHT]->GetParent()->GetRealX();
                    const double dMouseY = InterfaceManager::Instance()->GetPipboyMouseY();
                    float fHighlightY = dMouseY - this->pTiles[TI_MAP_HIGHLIGHT]->GetParent()->GetRealY();
                    if (fHighlightX < 0.0 || fHighlightY < 0.0 || fHighlightX > 850.0 || fHighlightY > 500.0) {
                        InterfaceManager::Instance()->GetCursorTile()->Set(kTileValue_Alpha, 255.0f, true);
                    }
                    else {
                        InterfaceManager::Instance()->GetCursorTile()->Set(kTileValue_Alpha, 0.0f, true);
                        // A right click on the map, outside menu mode's key, clicks button X.
                        if (appMain->GetControls()->GetMouseButton(1, Controls::isPressed) &&
                            !appMain->GetControls()->GetUserAction(Controls::UACTN_MENUMODE, Controls::isPressed)) {
                            this->DoClick(TI_BUTTON_X, nullptr);
                        }
                    }
                    fHighlightX = fHighlightX - this->pTiles[TI_MAP_HIGHLIGHT]->GetFloat(kTileValue_Width) / 2.0;
                    fHighlightY = fHighlightY - this->pTiles[TI_MAP_HIGHLIGHT]->GetFloat(kTileValue_Height) / 2.0;
                    this->pTiles[TI_MAP_HIGHLIGHT]->Set(kTileValue_X, fHighlightX, true);
                    this->pTiles[TI_MAP_HIGHLIGHT]->Set(kTileValue_Y, fHighlightY, true);
                    this->HighlightCenterMarker();
                }
                break;
            case TI_TAB_NOTES:
                // A note shown for fNoteGreyOutTime seconds is marked read and its entry greyed out.
                if (this->uiNoteEnterStartTime) {
                    const double dElapsed = appTimer.GetTime() - this->uiNoteEnterStartTime;
                    if (fNoteGreyOutTime.Float() * 1000.0 <= dElapsed) {
                        BGSNote* pCurrNote = this->pSelectedNote;
                        Tile* pCurrTile = nullptr;
                        if (InterfaceManager::Has360Controller() || !this->pSelectedNote) {
                            pCurrNote = this->lbNotesList.GetHighlightedData();
                        }
                        pCurrTile = this->lbNotesList.GetTileFor(pCurrNote);
                        if (pCurrNote && pCurrTile) {
                            pCurrTile->Set(MapMenu::iUT_TextAlpha, 128.0f, true);
                            pCurrNote->SetHasBeenRead(true);
                            this->uiNoteEnterStartTime = 0;
                        }
                    }
                }
                break;
            case TI_TAB_RADIO:
                if (FalloutRadio::IsPipboyRadioEnabled()) {
                    this->UpdateWaveform();
                }
                else {
                    this->ResetWaveform();
                }
                break;
            default: break;
        }
        bleftMenu = false;
    }
    else if (!bleftMenu && !InterfaceManager::Has360Controller() && this->iCurrentPage <= TI_TAB_WORLDMAP) {
        // Shows the cursor again once the map pages have been left.
        InterfaceManager::Instance()->GetCursorTile()->Set(kTileValue_Alpha, 255.0f, true);
        bleftMenu = true;
    }
    if (this->bSoundsLoading || this->pCurrentSound) {
        this->UpdatePlayingSounds();
    }
}

int32_t MapMenu::GetClass() {
    return 1023;
}

bool MapMenu::DoGamepad(int32_t aiButton, float afValue) {
    if (!Interface::IsTopMenuID(Interface::ePipBoyData)) {
        return false;
    }
    FOPipboyManager* pPipboy = Interface::GetPipboy();
    if (!pPipboy->GetInitialized()) {
        return false;
    }
    bool bResult = false;
    if (Interface::IsTopMenuID(Interface::ePipBoyData)) {
        if (Interface::Has360Controller()) {
            int32_t iStickX = 0;
            int32_t iStickY = 0;
            if (this->iCurrentPage == 32 || this->iCurrentPage == 33) {
                iStickY = appMain->GetControls()->GetControllerValue(0, 8);
                if (NiAbs(static_cast<float>(iStickY)) <= 7849.0) {
                    iStickY = 0;
                }
                if (iStickY) {
                    float fZoomStick = static_cast<double>(iStickY) / 32767.0;
                    float fYScale = NiAbs(fZoomStick);
                    float fZoomDelta = fYScale * 0.07500000298023224 + 1.0;
                    ZoomMap(iStickY > 0, fZoomDelta);
                    bResult = true;
                }
            }
            iStickX = appMain->GetControls()->GetControllerValue(0, 9);
            iStickY = appMain->GetControls()->GetControllerValue(0, 10);
            if (NiAbs(static_cast<float>(iStickX)) <= 8689.0) {
                iStickX = 0;
            }
            if (NiAbs(static_cast<float>(iStickY)) <= 8689.0) {
                iStickY = 0;
            }
            switch (this->iCurrentPage) {
                case 32:
                case 33: {
                    if (pMapTile) {
                        if (iStickX || iStickY) {
                            pMapTile->Set(4074, 1.0f, true);
                            pMapTile->Set(4075, 1.0f, true);
                            float fScrollX = static_cast<double>(iStickX) * -20.0 / 32767.0;
                            pMapTile->Poke(4078, fScrollX);
                            float fScrollY = static_cast<double>(iStickY) * 20.0 / 32767.0;
                            pMapTile->Poke(4079, fScrollY);
                            bResult = true;
                        }
                        else {
                            pMapTile->Set(4074, -1.0f, true);
                            pMapTile->Set(4075, -1.0f, true);
                        }
                        HighlightCenterMarker();
                    }
                    break;
                }
                case 34:
                case 35: {
                    Tile* pScrollbar;
                    if (this->iCurrentPage == 34) {
                        pScrollbar = lbQuestObjectivesList.pScrollBar;
                    }
                    else {
                        pScrollbar = pTiles[13]->GetChildByName("MM_TextScrollbar");
                    }
                    if (pScrollbar && pScrollbar->GetFloat(4003) != 0.0) {
                        int32_t iDirection = 0;
                        if (iStickY) {
                            iDirection = 2 * (iStickY < 0) - 1;
                        }
                        if (iDirection) {
                            double dElapsed;
                            if (!g_lastMapMenuScrollTime ||
                                (dElapsed = static_cast<double>(appTimer.iLastTime - g_lastMapMenuScrollTime),
                                 fKeyRepeatInterval.Float() < dElapsed)) {
                                int32_t iCurrentValueTrait = Tile::TextToTrait("_current_value");
                                // Truncates the scroll bar's value to an int.
                                int32_t iCurrentValue = static_cast<int32_t>(pScrollbar->GetFloat(iCurrentValueTrait));
                                pScrollbar
                                    ->Set(iCurrentValueTrait, static_cast<float>(iCurrentValue + iDirection), false);
                                if (static_cast<double>(iCurrentValue) != pScrollbar->GetFloat(iCurrentValueTrait)) {
                                    int32_t iTrait = Tile::TextToTrait("_SetInCode");
                                    pScrollbar->Poke(iTrait, 1.0f);
                                }
                                g_lastMapMenuScrollTime = appTimer.iLastTime;
                                bResult = true;
                            }
                        }
                    }

                    break;
                }
                default: {
                    break;
                }
            }
        }
        switch (aiButton) {
            case 3:
            case 4: {
                int32_t iPage;
                this->iCurrentPage += 2 * (aiButton != 4) - 1;
                if (this->iCurrentPage <= 36) {
                    iPage = this->iCurrentPage;
                }
                else {
                    iPage = 32;
                }
                this->iCurrentPage = iPage;
                if (this->iCurrentPage >= 32) {
                    iPage = this->iCurrentPage;
                }
                else {
                    iPage = 36;
                }
                this->iCurrentPage = iPage;
                DoClick(this->iCurrentPage, nullptr);
                bResult = true;
                break;
            }
            case 9: {
                if (this->iCurrentPage == 33) {
                    NiTListItem<Tile*>* pPos = pTiles[4]->xChildren.m_pkHead;
                    while (pPos) {
                        Tile* pCurrTile = pTiles[4]->xChildren.GetNext(pPos);
                        if (pCurrTile->IsTrue(MapMenu::iUT_Selected)) {
                            DoClick(26, pCurrTile);
                            bResult = true;
                            pPos = nullptr;
                        }
                    }
                }
                break;
            }
            case 13: {
                InterfaceManager::PlayMenuSound(MENU_SOUND_MODE);
                Interface::SetInventoryMenuVisible(true, false, true);
                bResult = true;
                break;
            }
            case 14: {
                InterfaceManager::PlayMenuSound(MENU_SOUND_MODE);
                Interface::SetStatsMenuVisible(true, false);
                bResult = true;
                break;
            }
            case 15:
            case 16: {
                if (this->iCurrentPage == 32 || this->iCurrentPage == 33) {
                    ZoomMap(aiButton == 16, 1.2f);
                    bResult = true;
                }
                break;
            }
            default: {
                break;
            }
        }
    }
    return bResult;
}

void MapMenu::DoControllerSwitch(bool abHas360Controller) {
    if (abHas360Controller) {
        pTiles[6]->Set(4001, 348.0f, true);
        pTiles[6]->Set(4002, 170.0f, true);
    }
}

void MapMenu::ZoomMap(bool abZoomIn, float afZoomDelta) {
    if (pMapTile && (iCurrentPage != 32 || !MapMenu::ShouldRenderLocalMap())) {
        double dZoomFactor;
        bool bMagnificationChange = true;
        if (abZoomIn) {
            dZoomFactor = afZoomDelta;
        }
        else {
            dZoomFactor = 1.0 / afZoomDelta;
        }
        float fNewMagnification = pMapTile->GetFloat(MapMenu::iUT_Magnification) * dZoomFactor;
        double dHalfHeight = pMapTile->pParent->GetFloat(4016) / 2.0;
        float fCenterY = dHalfHeight - pMapTile->GetFloat(4002);
        double dHalfWidth = pMapTile->pParent->GetFloat(4017) / 2.0;
        float fCenterX = dHalfWidth - pMapTile->GetFloat(4001);
        NiPoint2 kMapCenter(fCenterX, fCenterY);
        if (iCurrentPage == 32) {
            float fMagnificationRatio = 0.0f;
            float fMinZoom = fLocalMapMinZoom.Float();
            float fCappedMagnification = BSMIN(fNewMagnification, fLocalMapMaxZoom.Float());
            fNewMagnification = BSMAX(fCappedMagnification, fMinZoom);
            double dZoomOffset = fNewMagnification - fLocalMapMinZoom.Float();
            fMagnificationRatio = dZoomOffset / (fLocalMapMaxZoom.Float() - fLocalMapMinZoom.Float());
            double dMarkerMinSize = fLocalMapMarkerMinSize.Float();
            double dMarkerMaxSize = fLocalMapMarkerMaxSize.Float();
            float fMarkerSize = (dMarkerMaxSize - fLocalMapMarkerMinSize.Float()) * fMagnificationRatio +
                dMarkerMinSize;
            pMapTile->Set(Tile::kTileValue_User0, fMarkerSize, true);
            double dQuestMarkerMinSize = fLocalQuestMarkerMinSize.Float();
            double dQuestMarkerMaxSize = fLocalQuestMarkerMaxSize.Float();
            float fQuestMarkerSize = (dQuestMarkerMaxSize - fLocalQuestMarkerMinSize.Float()) * fMagnificationRatio +
                dQuestMarkerMinSize;
            pMapTile->Set(Tile::kTileValue_User1, fQuestMarkerSize, true);
            double dCursorMinSize = fLocalPlayerCursorMinSize.Float();
            double dCursorMaxSize = fLocalPlayerCursorMaxSize.Float();
            float fCursorSize = (dCursorMaxSize - fLocalPlayerCursorMinSize.Float()) * fMagnificationRatio +
                dCursorMinSize;
            pMapTile->Set(Tile::kTileValue_User2, fCursorSize, true);
        }
        else {
            float fMagnificationRatio = 0.0f;
            float fMinZoom = fWorldMapMinZoom.Float();
            float fCappedMagnification = BSMIN(fNewMagnification, fWorldMapMaxZoom.Float());
            fNewMagnification = BSMAX(fCappedMagnification, fMinZoom);
            double dZoomOffset = fNewMagnification - fWorldMapMinZoom.Float();
            fMagnificationRatio = dZoomOffset / (fWorldMapMaxZoom.Float() - fWorldMapMinZoom.Float());
            double dMarkerMinSize = fWorldMapMarkerMinSize.Float();
            double dMarkerMaxSize = fWorldMapMarkerMaxSize.Float();
            float fMarkerSize = (dMarkerMaxSize - fWorldMapMarkerMinSize.Float()) * fMagnificationRatio +
                dMarkerMinSize;
            pMapTile->Set(Tile::kTileValue_User0, fMarkerSize, true);
            double dQuestMarkerMinSize = fWorldQuestMarkerMinSize.Float();
            double dQuestMarkerMaxSize = fWorldQuestMarkerMaxSize.Float();
            float fQuestMarkerSize = (dQuestMarkerMaxSize - fWorldQuestMarkerMinSize.Float()) * fMagnificationRatio +
                dQuestMarkerMinSize;
            pMapTile->Set(Tile::kTileValue_User1, fQuestMarkerSize, true);
            double dCursorMinSize = fWorldPlayerCursorMinSize.Float();
            double dCursorMaxSize = fWorldPlayerCursorMaxSize.Float();
            float fCursorSize = (dCursorMaxSize - fWorldPlayerCursorMinSize.Float()) * fMagnificationRatio +
                dCursorMinSize;
            pMapTile->Set(Tile::kTileValue_User2, fCursorSize, true);
        }
        double dMagnification = pMapTile->GetFloat(MapMenu::iUT_Magnification);
        bMagnificationChange = fNewMagnification != dMagnification;
        double dFileWidth = pMapTile->GetFloat(4045);
        double dScaledWidth = dFileWidth * fNewMagnification;
        float fCenterScale = dScaledWidth / pMapTile->GetFloat(4017);
        kMapCenter *= fCenterScale;
        pMapTile->Set(MapMenu::iUT_Magnification, fNewMagnification, true);
        if (iCurrentPage == 32) {
            NiNode* pMapModel = pMapTile->GetModel();
            uint32_t uiMapSide = uGridsToLoad.UInt();
            for (int32_t iSubmapCt = 0; pMapModel && iSubmapCt < uiMapSide * uiMapSide; ++iSubmapCt) {
                NiTriShape* pTri = static_cast<NiTriShape*>(pMapModel->GetAt(iSubmapCt));
                if (pTri) {
                    int32_t iGridSize = TES::pInstance->pInteriorCell != nullptr ? 2048 : 1024;
                    float fNewTrisize = static_cast<double>(iGridSize) * fNewMagnification;
                    pTri->SetScale(fNewMagnification);
                    float fZ = static_cast<double>(iSubmapCt / uiMapSide) * fNewTrisize + fNewTrisize / 2.0 -
                        static_cast<double>(uiMapSide) * fNewTrisize;
                    float fX = static_cast<double>(iSubmapCt % uiMapSide) * fNewTrisize + fNewTrisize / 2.0;
                    pTri->SetTranslate(fX, 0.0f, fZ);
                    NiUpdateData kUpdateData(0.0f, false, false);
                    pTri->Update(kUpdateData);
                    pTri->GetModelData()->SetConsistency(NiGeometryData::STATIC);
                }
            }
        }
        double dParentWidth = pMapTile->pParent->GetFloat(4017);
        double dOffsetX = dParentWidth / 2.0 - kMapCenter.x;
        float fX = dOffsetX - pMapTile->GetFloat(4078);
        pMapTile->Set(4001, fX, false);
        double dParentHeight = pMapTile->pParent->GetFloat(4016);
        double dOffsetY = dParentHeight / 2.0 - kMapCenter.y;
        float fY = dOffsetY - pMapTile->GetFloat(4079);
        pMapTile->Set(4002, fY, false);
        if (bMagnificationChange) {
            double dKnobAmount;
            float fZoomRateRatio = (afZoomDelta - 1.0) / 0.14300000667572021;
            if (abZoomIn) {
                dKnobAmount = -fScrollKnobIncrement.Float() * fZoomRateRatio;
            }
            else {
                dKnobAmount = fScrollKnobIncrement.Float() * fZoomRateRatio;
            }
            float fRate = fScrollKnobRate.Float() + fScrollKnobRate.Float();
            float fAmount = dKnobAmount;
            FOPipboyManager* pPipboy = InterfaceManager::Instance()->GetPipboy();
            pPipboy->MoveKnob(FOPipboyManager::KNOB_SCROLL, fAmount, fRate, true);
        }
        Zoom_RefreshCompanions();
    }
}

void MapMenu::SetPageNumber(int32_t aiTab) {
    // the tab tile id is the tab index plus 0x20
    this->DoClick(aiTab + 0x20, nullptr);
}

bool MapMenu::AreAllTilesNonNull() {
    for (Tile* pTile : this->pTiles) {
        if (!pTile) {
            return false;
        }
    }
    return this->pChallengeType != nullptr;
}

void MapMenu::ResetWaveform() {
    this->fWaveformAnimationStartTime = 0.0f;
    this->pTiles[TI_WAVEFORM]->Set(Tile::kTileValue_Visible, 0);
}

void MapMenu::HighlightCenterMarker() {
    if (pMapTile) {
        double dY = pTiles[6]->GetFloat(4002);
        float fCenterY = pTiles[6]->GetFloat(4016) / 2.0 + dY;
        double dX = pTiles[6]->GetFloat(4001);
        float fCenterX = pTiles[6]->GetFloat(4017) / 2.0 + dX;
        NiPoint2 kHighlightCenter(fCenterX, fCenterY);
        NiTListItem<Tile*>* pPos = pMapTile->xChildren.m_pkHead;
        float fShortestDist = std::numeric_limits<float>::max();
        Tile* pClosestTile = nullptr;
        double dMapX = pMapTile->GetFloat(4001);
        kHighlightCenter.x = kHighlightCenter.x - dMapX;
        double dMapY = pMapTile->GetFloat(4002);
        kHighlightCenter.y = kHighlightCenter.y - dMapY;
        while (pPos) {
            Tile* pCurrTile = pMapTile->xChildren.GetNext(pPos);
            if (pCurrTile->GetFloat(4010) == 26.0 && !std::strstr(pCurrTile->GetName(), " Marker")) {
                float fTileY = pCurrTile->GetFloat(MapMenu::iUT_Y);
                float fTileX = pCurrTile->GetFloat(MapMenu::iUT_X);
                NiPoint2 kCurrPoint(fTileX, fTileY);
                float fCurrDist = 0.0f;
                kCurrPoint *= pMapTile->GetFloat(4017);
                fCurrDist = (kHighlightCenter - kCurrPoint).Length();
                if (fShortestDist > static_cast<double>(fCurrDist)) {
                    double dHeight = pTiles[6]->GetFloat(4016);
                    if (fCurrDist < dHeight / 2.0) {
                        fShortestDist = fCurrDist;
                        pClosestTile = pCurrTile;
                    }
                }
                pCurrTile->Set(MapMenu::iUT_Selected, 0);
            }
        }
        if (pClosestTile) {
            pClosestTile->Set(MapMenu::iUT_Selected, 1);
            if (pClosestTile != pHighlightedMapMarker) {
                BSAudio::QInstance().GetSoundHandleByName("UIPipBoyHighlight", 289u).Play(false);
                int32_t iTrait = Tile::AddUserTrait("_LocationName", -1);
                const char* pLocationName = pClosestTile->GetString(iTrait);
                if (_stricmp(pLocationName, "Companion")) {
                    iTrait = Tile::AddUserTrait("_LocationName", -1);
                    pLocationName = pClosestTile->GetString(iTrait);
                    int32_t iTitleTrait = Tile::AddUserTrait("_Title", -1);
                    pTiles[6]->Set(iTitleTrait, pLocationName, true);
                }
                pHighlightedMapMarker = pClosestTile;
                bool bUser0 = pClosestTile->IsTrue(4100);
                pTiles[11]->Set(4015, static_cast<int32_t>(bUser0));
            }
        }
        else {
            int32_t iTitleTrait = Tile::AddUserTrait("_Title", -1);
            pTiles[6]->Set(iTitleTrait, "", true);
            pHighlightedMapMarker = nullptr;
            pTiles[11]->Set(4015, 0);
        }
    }
}

void MapMenu::UpdatePlayingSounds() {
    static int32_t iSpeechEndTime;
    char acRemaining[68];
    if (bSoundsLoading) {
        bool bAllSoundsLoaded = true;
        bool bAllSoundsInvalid = true;
        int32_t iTotalDuration = 0;
        int32_t iNumSounds = 0;
        for (BSSimpleList<BSSoundHandle>* pSound = &this->SoundsToPlay; pSound; pSound = pSound->m_pkNext) {
            if (pSound->m_item.IsValid()) {
                bAllSoundsInvalid = false;
                if (!this->pCurrentSound) {
                    this->pCurrentSound = pSound;
                }
                if (pSound->m_item.GetDuration()) {
                    iTotalDuration += pSound->m_item.GetDuration();
                    ++iNumSounds;
                }
                else {
                    bAllSoundsLoaded = false;
                }
            }
        }
        if (bAllSoundsInvalid) {
            this->StopNoteSound();
            BSAudio::QInstance().SetRadioVolume(this->fOldPipboyRadioVolume);
        }
        else if (bAllSoundsLoaded) {
            this->bSoundsLoading = false;
            this->fSoundDurationMillis = static_cast<float>(iTotalDuration + 500 * (iNumSounds - 1));
            this->pCurrentSound->m_item.Play(false);
            this->iSoundPlayStartTime = appTimer.iLastTime;
            // Copies the location, then builds the by-value handle with operator=, then reads the text.
            NiPoint3 kSpeakerLoc = pPlayer->GetLocationOnReference();
            BSSoundHandle kHandle;
            kHandle = this->pCurrentSound->m_item;
            Interface::ShowSubtitle(this->SubtitlesToShow.GetAt(0).pString, kHandle, kSpeakerLoc, pPlayer, false);
            this->SubtitlesToShow.Remove(0, true);
        }
    }
    else if (this->fSoundDurationMillis > 0.0) {
        // The unsigned difference is zero-extended to 64 bits before the float conversion.
        float fElapsedMillis = static_cast<float>(static_cast<int64_t>(appTimer.iLastTime - this->iSoundPlayStartTime));
        fElapsedMillis = BSMIN(fElapsedMillis, this->fSoundDurationMillis);
        int32_t iRemainingMillis = static_cast<int32_t>(this->fSoundDurationMillis - fElapsedMillis);
        BSsprintf(
            acRemaining,
            64,
            "%02d:%02d:%01d %s",
            iRemainingMillis / 60000,
            iRemainingMillis % 60000 / 1000,
            iRemainingMillis % 1000 / 100,
            sRemaining.String()
        );
        this->pTiles[13]->Set(Tile::TextToTrait("_ItemStringInfo"), acRemaining, true);
        float fCompletion = fElapsedMillis / this->fSoundDurationMillis;
        this->pTiles[13]->Set(Tile::TextToTrait("_AudioFileCompletion"), fCompletion, true);
    }
    if (this->pCurrentSound && !this->pCurrentSound->m_item.IsValid()) {
        if (!iSpeechEndTime) {
            iSpeechEndTime = appTimer.iLastTime;
        }
        if (this->pCurrentSound->m_pkNext && appTimer.iLastTime - iSpeechEndTime > 500) {
            this->pCurrentSound = this->pCurrentSound->m_pkNext;
            this->pCurrentSound->m_item.Play(false);
            iSpeechEndTime = 0;
            // Copies the location, then builds the by-value handle with operator=, then reads the text.
            NiPoint3 kSpeakerLoc = pPlayer->GetLocationOnReference();
            BSSoundHandle kHandle;
            kHandle = this->pCurrentSound->m_item;
            Interface::ShowSubtitle(this->SubtitlesToShow.GetAt(0).pString, kHandle, kSpeakerLoc, pPlayer, true);
            this->SubtitlesToShow.Remove(0, true);
        }
        else if (!this->pCurrentSound->m_pkNext) {
            BSsprintf(acRemaining, 64, "00:00:0 %s", sRemaining.String());
            this->pTiles[13]->Set(Tile::TextToTrait("_ItemStringInfo"), acRemaining, true);
            this->pTiles[13]->Set(Tile::TextToTrait("_AudioFileCompletion"), 1.0f, true);
            this->pTiles[13]->Set(4100, 0);
            this->StopNoteSound();
        }
    }
}

void MapMenu::UpdateCurrentPage() {
    lbQuestsList.SetEnabled(false);
    lbQuestObjectivesList.SetEnabled(false);
    lbNotesList.SetEnabled(false);
    lbNotesList.SetVisible(false);
    lbChallengeList.SetEnabled(false);
    lbChallengeList.SetVisible(false);
    MapMenu::pMe->pTiles[11]->Set(4015, 1);
    MapMenu::pMe->pTiles[12]->Set(4003, 1);
    MapMenu::pMe->pTiles[18]->Set(4003, 0);
    MapMenu::pMe->pChallengeType->Set(4003, 0);
    switch (MapMenu::pMe->iCurrentPage) {
        case 32: {
            MapMenu::pMe->pMapTile = MapMenu::pMe->pTiles[2];
            if (!MapMenu::ShouldRenderLocalMap()) {
                MapMenu::pMe->UpdateLocalMap();
            }
            break;
        }
        case 33: {
            MapMenu::pMe->pMapTile = MapMenu::pMe->pTiles[4];
            MapMenu::pMe->UpdateWorldMap();
            break;
        }
        case 34: {
            MapMenu::pMe->UpdateQuests();
            break;
        }
        case 35: {
            MapMenu::pMe->pTiles[18]->Set(4003, 1);
            if (!MapMenu::bNotesChallengeMode) {
                MapMenu::pMe->lbNotesList.SaveListState();
                MapMenu::pMe->UpdateNotes();
                if (pSelectedNote && !MapMenu::bNotesChallengeMode) {
                    Tile* pSelectedTile = lbNotesList.GetTileFor(pSelectedNote);
                    LoadNote(pSelectedNote);
                    if (pSelectedTile) {
                        pSelectedTile->Set(MapMenu::iUT_Selected, 1);
                    }
                }

                MapMenu::pMe->lbNotesList.RestoreListState(true);
            }
            MapMenu::pMe->UpdateChallenges();
            ToggleNotesChallengeMode(false);
            break;
        }
        case 36: {
            MapMenu::pMe->pTiles[12]->Set(4003, 0);
            // The list box's virtuals are called through a ListBoxBase pointer so they stay indirect.
            ListBoxBase* pRadioBase = &lbRadioStationsList;
            // The highlighted tile's index is kept as a float and truncated before the refresh.
            float fHighlightIndex;
            if (pRadioBase->GetHighlightedItem()) {
                Tile* pHighlightedTile = static_cast<Tile*>(pRadioBase->GetHighlightedItem());
                fHighlightIndex = pHighlightedTile->GetFloat(4012);
            }
            else {
                fHighlightIndex = -1.0f;
            }
            int32_t iHighlightIndex = static_cast<int32_t>(fHighlightIndex);
            MapMenu::pMe->UpdateRadioList();
            Tile* pNewHighlight = pRadioBase->GetTileAt(iHighlightIndex, false);
            if (pNewHighlight) {
                InterfaceManager::Instance()->SetCurrentFocusTarget(pNewHighlight, 4035, true);
            }
            pRadioBase->ScrollToHighlight();
            break;
        }
        default: {
            break;
        }
    }
}

void MapMenu::UpdateWaveform() {
    static float fCurrValue;
    TileShaderProperty* pTSP;
    NiAVObject* pAVObj;
    if (MapMenu::pMe->pTiles[16]->GetModel()) {
        pAVObj = MapMenu::pMe->pTiles[16]->GetModel()->GetAtChecked(0);
    }
    else {
        pAVObj = nullptr;
    }
    if (pAVObj) {
        int32_t iType = NiShadeProperty::GetType();
        pTSP = static_cast<TileShaderProperty*>(pAVObj->GetProperty(iType));
    }
    else {
        pTSP = nullptr;
    }
    if (pTSP) {
        char acWaveform[68];
        if (fWaveformAnimationStartTime == 0.0) {
            NiPointer<NiTexture> spNewTexture(nullptr);
            const char* pDirection = pTiles[16]->GetString(4050);
            BSsprintf(acWaveform, 64u, sRadioWaveformFileFormat.String(), pDirection);
            MapMenu::pMe->pTiles[16]->Set(4044, acWaveform, true);
            fWaveformAnimationStartTime = static_cast<float>(BSGetTickCount());
            pTiles[16]->Set(4003, 1);
        }
        if (BSGetTickCount() - iLastWaveformScrollTime >= 10) {
            pTSP->SetTexScroll(fCurrValue, 0.0f);
            fCurrValue = fCurrValue + 0.10000000149011612;
            if (fCurrValue > 1.0) {
                fCurrValue = fCurrValue - 1.0;
            }
            iLastWaveformScrollTime = BSGetTickCount();
        }
        if (static_cast<double>(BSGetTickCount()) - fWaveformAnimationStartTime >= 750.0) {
            int32_t iRand = Random() % 3;
            if (iRand) {
                if (iRand == 1) {
                    BSstrcpy(acWaveform, 64, "Backward");
                }
                else {
                    BSstrcpy(acWaveform, 64, "Left");
                }
            }
            else {
                BSstrcpy(acWaveform, 64, "Forward");
            }
            pTiles[16]->Set(4050, acWaveform, true);
            fWaveformAnimationStartTime = 0.0f;
        }
    }
}

NiPoint3 MapMenu::WorldToMapCoord(NiPoint3 aWorldCoord, TESWorldSpace* apWorldspace) {
    // kMapCoord is built with the empty constructor, which stores nothing.
    NiPoint3 kMapCoord;
    // 33 is the world map page (TI_TAB_WORLD); only there does the marker coordinate need adjusting.
    if (apWorldspace && iCurrentPage == 33) {
        apWorldspace->AdjustMapMarkerCoord(aWorldCoord, false);
    }
    kMapCoord.x = (aWorldCoord.x - MapNWCorner.x) / (MapSECorner.x - MapNWCorner.x);
    kMapCoord.y = (aWorldCoord.y - MapNWCorner.y) / (MapSECorner.y - MapNWCorner.y);
    kMapCoord.z = 0.0f;
    if (iCurrentPage == 33) {
        // Scales with the double constants 0.796875 and 0.1015625 and rounds back to float.
        kMapCoord.x = kMapCoord.x * 0.796875 + 0.1015625;
        kMapCoord.y = kMapCoord.y * 0.796875 + 0.1015625;
    }
    return kMapCoord;
}

// The waypoint tiles are collected in a list on the stack. Each waypoint's position comes from the reference's
// GetLocationOnReference, read after its GetWorldSpace. The squared lengths go through the CRT's
// pow(float, int) and the angle through NiATan2(dy, dx), negated.
void MapMenu::CreateWaypointPath(
    TESObjectREFR* apStartRef,
    TESObjectREFR* apTargetRef,
    TeleportPath* apPathToTarget,
    int32_t aiID
) {
    using enum Tile::eTileValue;

    if (!apStartRef || !apTargetRef || !apPathToTarget) {
        return;
    }
    BSSimpleList<Tile*> kWaypoints;
    TESWorldSpace* pStartSpace = apStartRef->GetWorldSpace();
    NiPoint3 kStart = WorldToMapCoord(apStartRef->GetLocationOnReference(), pStartSpace);
    TESWorldSpace* pTargetSpace = apTargetRef->GetWorldSpace();
    NiPoint3 kTarget = WorldToMapCoord(apTargetRef->GetLocationOnReference(), pTargetSpace);

    Tile* pStartTile = MapMenu::pMe->RenderTemplate(pMapTile, "WaypointTemplate", nullptr);
    pStartTile->Set(MapMenu::iUT_X, kStart.x, true);
    pStartTile->Set(MapMenu::iUT_Y, kStart.y, true);
    pStartTile->Set(kTileValue_ID, aiID);
    pStartTile->Set(kTileValue_Alpha, 0.0f, true);
    if (apStartRef->GetObjectReference()->GetFormType() == DOOR_ID &&
        apStartRef->GetTeleport()->GetLinkedDoor()->GetInterior()) {
        pStartTile->Set(kTileValue_User0, 1);
    }
    kWaypoints.AddTail(pStartTile);

    // The doors between the start and the target: from the one after the start's door up to the one before the
    // target's.
    BSSimpleArray<TeleportLink>& rDoors = apPathToTarget->Doors;
    int32_t iFirst = 0;
    int32_t iLast = rDoors.GetSize() - 1;
    for (uint32_t i = 0; i < rDoors.GetSize(); ++i) {
        if (rDoors.GetAt(i).pDoor == apStartRef ||
            rDoors.GetAt(i).pDoor->GetTeleport()->GetLinkedDoor() == apStartRef) {
            iFirst = i + 1;
        }
        if (rDoors.GetAt(i).pDoor == apTargetRef ||
            rDoors.GetAt(i).pDoor->GetTeleport()->GetLinkedDoor() == apTargetRef) {
            iLast = i - 1;
        }
    }
    for (int32_t j = iFirst; j <= iLast; ++j) {
        TESObjectREFR* pDoor = rDoors.GetAt(j).pDoor->GetInterior() ? nullptr : rDoors.GetAt(j).pDoor;
        if (!pDoor) {
            pDoor = rDoors.GetAt(j).pDoor->GetTeleport()->GetLinkedDoor();
        }
        if (!pDoor || pDoor->GetInterior()) {
            continue;
        }
        Tile* pDoorTile = MapMenu::pMe->RenderTemplate(pMapTile, "WaypointTemplate", nullptr);
        TESWorldSpace* pDoorSpace = pDoor->GetWorldSpace();
        NiPoint3 kDoor = WorldToMapCoord(pDoor->GetLocationOnReference(), pDoorSpace);
        pDoorTile->Set(MapMenu::iUT_X, kDoor.x, true);
        pDoorTile->Set(MapMenu::iUT_Y, kDoor.y, true);
        pDoorTile->Set(kTileValue_ID, aiID);
        if (rDoors.GetAt(j).pDoor->GetTeleport()->GetLinkedDoor()->GetInterior()) {
            pDoorTile->Set(kTileValue_User0, 1);
        }
        kWaypoints.AddTail(pDoorTile);
    }

    Tile* pTargetTile = MapMenu::pMe->RenderTemplate(pMapTile, "WaypointTemplate", nullptr);
    pTargetTile->Set(MapMenu::iUT_X, kTarget.x, true);
    pTargetTile->Set(MapMenu::iUT_Y, kTarget.y, true);
    pTargetTile->Set(kTileValue_ID, aiID);
    pTargetTile->Set(kTileValue_Alpha, 0.0f, true);
    kWaypoints.AddTail(pTargetTile);

    float fWidth = pMapTile ? pMapTile->GetFloat(kTileValue_FileWidth) : 0.0f;
    float fHeight = pMapTile ? pMapTile->GetFloat(kTileValue_FileHeight) : 0.0f;
    // A path line from each waypoint to the next.
    for (BSSimpleList<Tile*>* pIter = &kWaypoints; pIter && pIter->GetItem() && pIter->GetNext();
         pIter = pIter->GetNext()) {
        Tile* pLine = MapMenu::pMe->RenderTemplate(pIter->GetItem(), "PathLineTemplate", nullptr);
        NiPoint2 kFrom;
        NiPoint2 kTo;
        float fFromY = pIter->GetItem()->GetFloat(MapMenu::iUT_Y);
        float fFromX = pIter->GetItem()->GetFloat(MapMenu::iUT_X);
        ::new (&kFrom) NiPoint2(fFromX, fFromY);
        float fToY = pIter->GetNext()->GetItem()->GetFloat(MapMenu::iUT_Y);
        float fToX = pIter->GetNext()->GetItem()->GetFloat(MapMenu::iUT_X);
        ::new (&kTo) NiPoint2(fToX, fToY);
        pLine->Set(kTileValue_ID, aiID);
        float fScaledDx = (kTo.x - kFrom.x) * fWidth;
        float fDxSquared = std::pow(fScaledDx, 2);
        float fScaledDy = (kTo.y - kFrom.y) * fHeight;
        float fLengthSquared = fDxSquared + std::pow(fScaledDy, 2);
        pLine->Set(kTileValue_User0, NiSqrt(fLengthSquared), true);
        float fDx = kTo.x - kFrom.x;
        float fDy = kTo.y - kFrom.y;
        pLine->Set(kTileValue_RotateAngle, -NiATan2(fDy, fDx), true);
        if (pLine->GetParent()->IsTrue(kTileValue_User0)) {
            pLine->Set(kTileValue_Filename, "Interface\\Worldmap\\waypoint_path_interior.dds", true);
        }
    }
}

TESObjectREFR* MapMenu::GetWorldMapTrackingRef(TESObjectREFR* apRef, TeleportPath* apPathToRef) {
    TESObjectREFR* pDoor = nullptr;
    if (!apPathToRef || (!apPathToRef->Spaces.iSize && !apPathToRef->Doors.iSize)) {
        return nullptr;
    }
    if (!apRef || !apRef->GetInterior()) {
        return apRef;
    }
    char cFound = 0;
    for (int32_t iSpaceCt = apPathToRef->Spaces.iSize - 1; !cFound && iSpaceCt >= 0; --iSpaceCt) {
        if (apPathToRef->Spaces.GetAt(iSpaceCt).bWorldSpace && iSpaceCt < apPathToRef->Doors.iSize) {
            pDoor = apPathToRef->Doors.GetAt(iSpaceCt).pDoor;
            cFound = 1;
        }
    }
    if (!cFound) {
        return pMapPlayerRef;
    }
    return pDoor;
}

// The old quest marker and waypoint tiles are taken out of the map tile and deleted through their virtual destructor.
// Then, while bShowQuestMarkers is set, each quest target gets a marker: on the world map at the reference the map
// tracks for it (with a waypoint path), on the local map at the target's path end, or at the target itself when it is
// outside (world map) or in the player's cell (local map). The item is read once per use.
void MapMenu::AddQuestMarkers() {
    using enum Tile::eTileValue;

    for (Tile* pTile = pMapTile->GetChildByID(TI_QUEST_MARKER); pTile;
         pTile = pMapTile->GetChildByID(TI_QUEST_MARKER)) {
        pMapTile->xChildren.Remove(pTile);
        pTile->Release();
        delete pTile;
    }
    for (Tile* pTile = pMapTile->GetChildByID(TI_QUEST_WAYPOINT); pTile;
         pTile = pMapTile->GetChildByID(TI_QUEST_WAYPOINT)) {
        pMapTile->xChildren.Remove(pTile);
        pTile->Release();
        delete pTile;
    }
    for (BSSimpleList<TESQuestTarget*>* pIter = pPlayer->GetCurrentTargetList();
         bShowQuestMarkers.Bool() && pIter && pIter->GetItem();
         pIter = pIter->GetNext()) {
        TESObjectREFR* pReference = pIter->GetItem()->GetReference(true);
        TESObjectREFR* pTarget;
        if (iCurrentPage == TI_TAB_WORLDMAP) {
            pTarget = GetWorldMapTrackingRef(pReference, pIter->GetItem()->QTargetPath());
        }
        else {
            pTarget = pIter->GetItem()->QTrackingRef();
        }
        if (pTarget) {
            if (iCurrentPage == TI_TAB_WORLDMAP) {
                CreateWaypointPath(pMapPlayerRef, pTarget, pIter->GetItem()->QTargetPath(), TI_QUEST_WAYPOINT);
            }
        }
        else if (
            pReference &&
            ((iCurrentPage == TI_TAB_WORLDMAP && !pReference->GetInterior()) ||
             (iCurrentPage == TI_TAB_LOCALMAP && pReference->GetParentCell() == pPlayer->GetParentCell()))
        ) {
            pTarget = pIter->GetItem()->GetReference(true);
        }

        if (!pTarget) {
            continue;
        }
        NiPoint3 kPos = pTarget->GetLocationOnReference();
        NiPoint3 kMapPos;
        if (pAdjustNorthRotationCell) {
            pAdjustNorthRotationCell->AdjustCoordForNorthRotation(kPos, kPos, true);
        }
        TESWorldSpace* pWorldSpace = pTarget->GetWorldSpace();
        kMapPos = WorldToMapCoord(kPos, pWorldSpace);
        Tile* pMarker = nullptr;
        if (iCurrentPage == TI_TAB_WORLDMAP) {
            pMarker = MapMenu::pMe->RenderTemplate(pMapTile, "WorldMapQuestMarkerTemplate", nullptr);
        }
        else {
            pMarker = MapMenu::pMe->RenderTemplate(pMapTile, "LocalMapQuestMarkerTemplate", nullptr);
        }
        pMarker->Set(MapMenu::iUT_X, kMapPos.x, true);
        pMarker->Set(MapMenu::iUT_Y, kMapPos.y, true);
        pMarker->Set(kTileValue_Filename, "glow_hud_compass_objective_marker.dds", true);
        pMarker->Set(kTileValue_ID, TI_QUEST_MARKER);
        Tile::Value* pBrightness = pMarker->GetValueQ(kTileValue_Brightness);
        pBrightness->ClearActions();
        Tile* pArrow = iCurrentPage == TI_TAB_WORLDMAP ? pTiles[TI_WORLD_MAP_ARROW] : pTiles[TI_LOCAL_MAP_ARROW];
        pBrightness->AddAction(kTileValue_Copy, pArrow, kTileValue_Brightness);
        pBrightness->CalculateValue(false);
    }
}

void MapMenu::UpdateLocalMap() {
    NiPoint2 kExtSECorner;
    NiPoint2 kExtNWCorner;
    NiPoint2 kIntSECorner;
    NiPoint2 kIntNWCorner;
    TESWorldSpace* pWorld = pPlayer->GetWorldSpace();
    TESObjectCELL* pPlayerCell = pPlayer->GetParentCell();
    NiPoint3 kPlayerLoc = pPlayer->GetLocationOnReference();
    pAdjustNorthRotationCell = pPlayer->GetInterior() ? pPlayerCell : nullptr;
    if (pAdjustNorthRotationCell) {
        NiPoint3 kPlayerIntLoc = kPlayerLoc;
        pAdjustNorthRotationCell->AdjustCoordForNorthRotation(kPlayerLoc, kPlayerIntLoc, true);
        int32_t iPlayerX = FloatToInt(kPlayerIntLoc.x);
        int32_t iCellX = ((iPlayerX - 2048) >> 12) - (uGridsToLoad.Int() >> 1);
        int32_t iPlayerY = FloatToInt(kPlayerIntLoc.y);
        int32_t iCellY = ((iPlayerY - 2048) >> 12) - (uGridsToLoad.Int() >> 1);
        float fNWCellY = static_cast<float>(((iCellY + uGridsToLoad.Int() - 1) << 12) + 2048);
        NiPoint3 kNWCellCoord(static_cast<float>((iCellX << 12) + 2048), fNWCellY, 0.0f);
        float fSECellY = static_cast<float>((iCellY << 12) + 2048);
        float fSECellX = static_cast<float>(((iCellX + uGridsToLoad.Int() - 1) << 12) + 2048);
        NiPoint3 kSECellCoord(fSECellX, fSECellY, 0.0f);
        float fNWOffsetY = kNWCellCoord.y + 4096.0;
        NiPoint2* pNWCorner = ::new (&kIntNWCorner) NiPoint2(kNWCellCoord.x, fNWOffsetY);
        float fNWCornerX = pNWCorner->x;
        float fNWCornerY = pNWCorner->y;
        MapNWCorner.x = fNWCornerX;
        MapNWCorner.y = fNWCornerY;
        float fSEOffsetX = kSECellCoord.x + 4096.0;
        NiPoint2* pSECorner = ::new (&kIntSECorner) NiPoint2(fSEOffsetX, kSECellCoord.y);
        float fSECornerX = pSECorner->x;
        float fSECornerY = pSECorner->y;
        MapSECorner.x = fSECornerX;
        MapSECorner.y = fSECornerY;
    }
    else {
        int32_t iPlayerX = FloatToInt(kPlayerLoc.x);
        int32_t iCellX = (iPlayerX >> 12) - (uGridsToLoad.Int() >> 1);
        int32_t iPlayerY = FloatToInt(kPlayerLoc.y);
        int32_t iCellY = (iPlayerY >> 12) - (uGridsToLoad.Int() >> 1);
        float fNWCellY = static_cast<float>((iCellY + uGridsToLoad.Int() - 1) << 12);
        NiPoint3 kNWCellCoord(static_cast<float>(iCellX << 12), fNWCellY, 0.0f);
        float fSECellY = static_cast<float>(iCellY << 12);
        float fSECellX = static_cast<float>((iCellX + uGridsToLoad.Int() - 1) << 12);
        NiPoint3 kSECellCoord(fSECellX, fSECellY, 0.0f);
        float fNWOffsetY = kNWCellCoord.y + 4096.0;
        NiPoint2* pNWCorner = ::new (&kExtNWCorner) NiPoint2(kNWCellCoord.x, fNWOffsetY);
        float fNWCornerY = pNWCorner->y;
        MapNWCorner.x = pNWCorner->x;
        MapNWCorner.y = fNWCornerY;
        float fSEOffsetX = kSECellCoord.x + 4096.0;
        NiPoint2* pSECorner = ::new (&kExtSECorner) NiPoint2(fSEOffsetX, kSECellCoord.y);
        float fSECornerY = pSECorner->y;
        MapSECorner.x = pSECorner->x;
        MapSECorner.y = fSECornerY;
    }
    ZoomMap(true, 1.0f);
    if (pWorld) {
        pMapSpace = pWorld;
    }
    else {
        pMapSpace = pPlayerCell;
    }
    if (pMapSpace) {
        if (pTiles[2]->xChildren.IsEmpty()) {
            for (TESObjectREFR* pCurrMarker : LocalMapMarkerList.ItemsUntilNull()) {
                TESObjectDOOR* pCurrDoor;
                if (pCurrMarker->GetObjectReference()->GetFormType() == DOOR_ID) {
                    pCurrDoor = static_cast<TESObjectDOOR*>(pCurrMarker->GetObjectReference());
                }
                else {
                    pCurrDoor = nullptr;
                }
                NiPoint3 kMarkerPos = pCurrMarker->GetLocationOnReference();
                if (!pCurrMarker->GetDestroyed() &&
                    !pCurrMarker->GetIsHiddenDoor() &&
                    pCurrDoor &&
                    !pCurrDoor->IsHidden()) {
                    if (pAdjustNorthRotationCell) {
                        pAdjustNorthRotationCell->AdjustCoordForNorthRotation(kMarkerPos, kMarkerPos, true);
                    }
                    float fAlpha = 255.0f;
                    NiPoint3 kMarkerTilePos = WorldToMapCoord(kMarkerPos, nullptr);
                    if (bFogOfWarEnabled) {
                        int32_t iSeenValue = TESObjectCELL::GetSeenValue(kMarkerPos);
                        fAlpha = static_cast<double>(iSeenValue) / 4.0 * fAlpha;
                    }
                    if (fAlpha > 0.0 &&
                        kMarkerTilePos.x >= 0.0 &&
                        kMarkerTilePos.x <= 1.0 &&
                        kMarkerTilePos.y >= 0.0 &&
                        kMarkerTilePos.y <= 1.0) {
                        Tile* pMarkerTile = MapMenu::pMe->RenderTemplate(pTiles[2], "MapMarkerTemplate", nullptr);
                        DoorTeleportData* pTeleportData = pCurrMarker->GetTeleport();
                        pMarkerTile->Set(4010, 26);
                        pMarkerTile->Set(MapMenu::iUT_X, kMarkerTilePos.x, true);
                        pMarkerTile->Set(MapMenu::iUT_Y, kMarkerTilePos.y, true);
                        pMarkerTile->Set(4009, fAlpha, true);
                        pMarkerTile->Set(4044, "Interface\\Icons\\Local Map\\icon_local_door.dds", true);
                        if (pTeleportData) {
                            BSStringT<char> strDoorDestinationName;
                            pTeleportData->GetTeleportCellMapName(strDoorDestinationName);
                            char* pDestinationName = strDoorDestinationName.pString;
                            int32_t iTrait = Tile::AddUserTrait("_LocationName", -1);
                            pMarkerTile->Set(iTrait, pDestinationName, true);
                        }
                    }
                }
            }
        }
        AddMapIcons();
    }
    pTiles[2]->Set(4003, 1);
    pTiles[3]->Set(4003, 1);
}

void MapMenu::AddMapIcons() {
    Tile* pArrow = iCurrentPage == 33 ? pTiles[5] : pTiles[3];
    NiPoint3 kPlayerPos;
    NiPoint3 kMapPos(0.5f, 0.5f, 0.0f);
    if (iCurrentPage == 33 && pPlayer->GetInterior()) {
        pMapPlayerRef = pPlayer->pLastExtDoorActivated;
        if (!pMapPlayerRef) {
            TeleportPath kRetPath;
            ActorLockData kLockData(pPlayer);
            bool bFoundPath;
            // Constructs the destination, then the origin, and destroys the origin and the destination right after the
            // search: the scope ends there.
            {
                PathingLocation kDestination;
                PathingLocation kOrigin(pPlayer);
                bFoundPath = Pathing::BuildTeleportDoorPath(
                    kOrigin,
                    kDestination,
                    &kRetPath,
                    Pathing::TELEPORT_DOOR_PATH_FIND_FIRST_WORLDSPACE,
                    0.0f,
                    &kLockData,
                    Pathing::DOOR_LOCK_DISPOSITION_LOCKS_ARE_MINIMAL_USE
                );
            }
            if (bFoundPath) {
                if (kRetPath.Doors.iSize) {
                    TeleportLink* pLastLink = &kRetPath.Doors.GetLast();
                    DoorTeleportData* pTeleport = pLastLink->pDoor->GetTeleport();
                    pMapPlayerRef = pTeleport->pLinkedDoor;
                }
                if (!pMapPlayerRef) {
                    pMapPlayerRef = pPlayer;
                }
            }
        }
    }
    else {
        pMapPlayerRef = pPlayer;
    }
    if (pMapPlayerRef) {
        float fNorthRotation;
        kPlayerPos = pMapPlayerRef->GetLocationOnReference();
        if (pAdjustNorthRotationCell) {
            pAdjustNorthRotationCell->AdjustCoordForNorthRotation(kPlayerPos, kPlayerPos, true);
        }
        TESWorldSpace* pWorldSpace = pMapPlayerRef->GetWorldSpace();
        kMapPos = WorldToMapCoord(kPlayerPos, pWorldSpace);
        pArrow->Set(4003, 1);
        pArrow->Set(MapMenu::iUT_X, kMapPos.x, true);
        pArrow->Set(MapMenu::iUT_Y, kMapPos.y, true);
        float fHeading = -pPlayer->GetHeading(false) - 0.30000001192092896;
        if (pPlayer->GetInterior()) {
            TESObjectCELL* pParentCell = pPlayer->GetParentCell();
            fNorthRotation = pParentCell->GetNorthRotation();
        }
        else {
            fNorthRotation = 0.0f;
        }
        fHeading = fHeading - fNorthRotation;
        pArrow->Set(4090, fHeading, true);
    }
    else {
        pArrow->Set(4003, 0);
    }
    if (!pArrow->HasFadeControl(4085)) {
        pArrow->AddFadeControl(4085, 128.0f, 255.0f, 1.0f, Tile::TFCT_NONLINEAR_LONG_DARK_REPEATING);
    }
    Tile* pParent = pMapTile->pParent;
    double dHalfWidth = pParent->GetFloat(4017) / 2.0;
    double dWidth = pMapTile->GetFloat(4017);
    float fX = dHalfWidth - dWidth * kMapPos.x;
    pMapTile->Set(4001, fX, false);
    pParent = pMapTile->pParent;
    double dHalfHeight = pParent->GetFloat(4016) / 2.0;
    double dHeight = pMapTile->GetFloat(4016);
    float fY = dHalfHeight - dHeight * kMapPos.y;
    pMapTile->Set(4002, fY, false);
    AddQuestMarkers();
    AddCompanionMarkers();
    if (pPlayer->QPlayerMapMarker()) {
        DeletePlayerMarker();
        CreatePlayerMarker();
    }
}

void MapMenu::AddCompanionMarkers() {
    Tile* pTile;
    for (pTile = pMapTile->GetChildByID(37); pTile; pTile = pMapTile->GetChildByID(37)) {
        pMapTile->xChildren.Remove(pTile);
        pTile->Release();
        if (pTile) {
            delete pTile;
        }
    }
    while (true) {
        pTile = pMapTile->GetChildByID(38);
        if (!pTile) {
            break;
        }
        pMapTile->xChildren.Remove(pTile);
        pTile->Release();
        if (pTile) {
            delete pTile;
        }
    }
    BSSimpleList<Actor*>* pCurrTeammate = pPlayer->GetTeammateList();
    while (pCurrTeammate) {
        // The marker's reference and its map position.
        TESObjectREFR* pTarget;
        NiPoint3 kMapPos;
        Actor* pTeammate = pCurrTeammate->m_item;
        TeleportPath kPath;
        if (iCurrentPage == 33) {
            pPlayer->BuildPathToTarget(pTeammate, &kPath, true);
        }
        if (iCurrentPage == 33) {
            pTarget = GetWorldMapTrackingRef(pTeammate, &kPath);
        }
        else {
            pTarget = pTeammate;
        }
        if (pTarget && !pTarget->IsDead(false)) {
            TESWorldSpace* pTeammateSpace;
            if (iCurrentPage != 32 ||
                (pTeammate->GetParentCell() == pPlayer->GetParentCell() ||
                 (pTeammateSpace = pTeammate->GetWorldSpace(), pTeammateSpace == pPlayer->GetWorldSpace()))) {
                float fDistance = pTeammate->GetDistanceFromReference(pPlayer, false, false);
                if (!(fDistance < 2048.0) || iCurrentPage != 33) {
                    if (pTeammate) {
                        if ((iCurrentPage == 33 && !pTeammate->GetInterior()) ||
                            (iCurrentPage == 32 && pTeammate->GetParentCell() == pPlayer->GetParentCell())) {
                            pTarget = pTeammate;
                        }
                    }
                    if (pTarget) {
                        char acName[64];
                        NiPoint3 kPos = pTarget->GetLocationOnReference();
                        NiPoint3 kLabelPos = kPos;
                        double dLabelWorldY = iCurrentPage == 33 ? kLabelPos.y - 1024.0 : kLabelPos.y - 128.0;
                        kLabelPos.y = dLabelWorldY;
                        if (pAdjustNorthRotationCell) {
                            pAdjustNorthRotationCell->AdjustCoordForNorthRotation(kPos, kPos, true);
                        }
                        TESWorldSpace* pWorldSpace = pTarget->GetWorldSpace();
                        kMapPos = WorldToMapCoord(kPos, pWorldSpace);
                        pWorldSpace = pTarget->GetWorldSpace();
                        NiPoint3 kLabelMapPos = WorldToMapCoord(kLabelPos, pWorldSpace);
                        Tile* pArrow = iCurrentPage == 33 ? pTiles[5] : pTiles[3];
                        Tile* pMarker = nullptr;
                        Tile* pLabel = nullptr;
                        if (iCurrentPage == 33) {
                            pMarker = MapMenu::pMe->RenderTemplate(pMapTile, "WorldMapQuestMarkerTemplate", nullptr);
                        }
                        else {
                            pMarker = MapMenu::pMe->RenderTemplate(pMapTile, "LocalMapQuestMarkerTemplate", nullptr);
                        }
                        pMarker->Set(MapMenu::iUT_X, kMapPos.x, true);
                        pMarker->Set(MapMenu::iUT_Y, kMapPos.y, true);
                        const char* pFullName = pTarget->GetFullName();
                        std::sprintf(acName, "%s Marker", pFullName);
                        pMarker->SetName(static_cast<const char*>(acName));
                        pMarker->Set(Tile::kTileValue_Filename, "glow_hud_compass_landmark.dds", true);
                        pMarker->Set(Tile::kTileValue_ID, 26);
                        pMarker->Set(Tile::kTileValue_Depth, 5.0f, true);
                        int32_t iTrait = Tile::AddUserTrait("_LocationName", -1);
                        pMarker->Set(iTrait, "Companion", true);
                        Tile::Value* pBrightness = pMarker->GetValueQ(Tile::kTileValue_Brightness);
                        pBrightness->ClearActions();
                        pBrightness->AddAction(Tile::kTileValue_Copy, pArrow, Tile::kTileValue_Brightness);
                        pBrightness->CalculateValue(false);
                        float fLabelX = pMapTile->GetFloat(Tile::kTileValue_Width);
                        float fLabelY = pMapTile->GetFloat(Tile::kTileValue_Height);
                        fLabelX = fLabelX * kLabelMapPos.x;
                        fLabelY = fLabelY * kLabelMapPos.y;
                        pLabel = Tile::BuildTile(903);
                        const char* pTeammateName = pTeammate->GetFullName();
                        pLabel->Init(pMapTile, pTeammateName, nullptr);
                        pLabel->Set(MapMenu::iUT_X, kLabelMapPos.x, true);
                        pLabel->Set(MapMenu::iUT_Y, kLabelMapPos.y, true);
                        pFullName = pTarget->GetFullName();
                        pLabel->SetName(pFullName);
                        pLabel->Set(Tile::kTileValue_X, fLabelX, true);
                        pLabel->Set(Tile::kTileValue_Y, fLabelY, true);
                        pTeammateName = pTeammate->GetFullName();
                        pLabel->Set(Tile::kTileValue_String, pTeammateName, true);
                        pLabel->Set(Tile::kTileValue_WrapWidth, 200.0f, true);
                        pLabel->Set(Tile::kTileValue_ID, 38);
                        pLabel->Set(Tile::kTileValue_Depth, 2.0f, true);
                        pLabel->Set(Tile::kTileValue_Visible, 1);
                        pLabel->Set(Tile::kTileValue_Justify, 2);
                        pLabel->Set(Tile::kTileValue_Font, 2);
                        pLabel->Set(Tile::kTileValue_Height, 60.0f, true);
                        pLabel->AddNeedsUpdate(Tile::CHANGE_CREATE);
                        Tile::Value* pXValue = pLabel->GetValueQ(Tile::kTileValue_X);
                        Tile::Value* pYValue = pLabel->GetValueQ(Tile::kTileValue_Y);
                        pXValue->ClearActions();
                        pXValue->AddAction(Tile::kTileValue_Copy, pMarker, Tile::kTileValue_X);
                        pXValue->CalculateValue(false);
                        pYValue->ClearActions();
                        pYValue->AddAction(Tile::kTileValue_Copy, pMarker, Tile::kTileValue_Y);
                        pYValue->AddAction(Tile::kTileValue_Add, 60.0f);
                        pYValue->CalculateValue(false);
                        pBrightness = pLabel->GetValueQ(Tile::kTileValue_Brightness);
                        pBrightness->ClearActions();
                        pBrightness->AddAction(Tile::kTileValue_Copy, pArrow, Tile::kTileValue_Brightness);
                        pBrightness->CalculateValue(false);
                    }
                }
            }
        }

        pCurrTeammate = pCurrTeammate->GetNext();
    }
}

void MapMenu::CreatePlayerMarker() {
    TESObjectREFR* pTarget;
    NiPoint3 kMarkerWorldPos;
    if (iCurrentPage == 33) {
        TeleportPath* pMarkerPath = pPlayer->QPlayerMarkerPath();
        TESObjectREFR* pPlayerMapMarker = pPlayer->QPlayerMapMarker();
        pTarget = GetWorldMapTrackingRef(pPlayerMapMarker, pMarkerPath);
    }
    else {
        pTarget = pPlayer->QPlayerMarkerTrackingRef();
    }
    if (pTarget) {
        if (iCurrentPage == 33) {
            TeleportPath* pMarkerPath = pPlayer->QPlayerMarkerPath();
            CreateWaypointPath(pMapPlayerRef, pTarget, pMarkerPath, 30);
        }
    }
    else if (pPlayer->QPlayerMapMarker()) {
        if ((iCurrentPage == 33 && !pPlayer->QPlayerMapMarker()->GetInterior()) ||
            (iCurrentPage == 32 && pPlayer->QPlayerMapMarker()->GetParentCell() == pPlayer->GetParentCell())) {
            pTarget = pPlayer->QPlayerMapMarker();
        }
    }
    if (pTarget) {
        kMarkerWorldPos = pTarget->GetLocationOnReference();
        if (pAdjustNorthRotationCell) {
            pAdjustNorthRotationCell->AdjustCoordForNorthRotation(kMarkerWorldPos, kMarkerWorldPos, true);
        }
        TESWorldSpace* pWorldSpace = pTarget->GetWorldSpace();
        NiPoint3 kMarkerTilePos = WorldToMapCoord(kMarkerWorldPos, pWorldSpace);
        Tile* pArrowTile = iCurrentPage == 33 ? pTiles[5] : pTiles[3];
        Tile* pSubMarkerTile = nullptr;
        if (iCurrentPage == 33) {
            pSubMarkerTile = MapMenu::pMe->RenderTemplate(pMapTile, "WorldMapQuestMarkerTemplate", nullptr);
        }
        else {
            pSubMarkerTile = MapMenu::pMe->RenderTemplate(pMapTile, "LocalMapQuestMarkerTemplate", nullptr);
        }
        pSubMarkerTile->Set(MapMenu::iUT_X, kMarkerTilePos.x, true);
        pSubMarkerTile->Set(MapMenu::iUT_Y, kMarkerTilePos.y, true);
        pSubMarkerTile->Set(4044, "glow_hud_compass_pc_marker.dds", true);
        pSubMarkerTile->Set(4010, 28);
        pSubMarkerTile->Set(4013, 5.0f, true);
        Tile::Value* pBrightness = pSubMarkerTile->GetValueQ(4085);
        pBrightness->ClearActions();
        pBrightness->AddAction(2000, pArrowTile, 4085);
        pBrightness->CalculateValue(false);
    }
}

void MapMenu::DeletePlayerMarker() {
    Tile* pTile = pMapTile->GetChildByID(28);
    if (pTile) {
        pMapTile->xChildren.Remove(pTile);
        pTile->Release();
        delete pTile;
    }
    while (true) {
        pTile = pMapTile->GetChildByID(30);
        if (!pTile) {
            break;
        }
        pMapTile->xChildren.Remove(pTile);
        pTile->Release();
        delete pTile;
    }
}

TESWorldSpace* MapMenu::CalculatePlayerMarkerSpace(NiPoint3 aWorldPos) {
    NiPoint3 kAdjustedCoord;
    TESWorldSpace* pFoundWorld = nullptr;
    for (BSSimpleList<TESWorldSpace*>* pWorldObj = pDataHandler->GetWorldSpaceList(); pWorldObj && pWorldObj->m_item;
         pWorldObj = pWorldObj->m_pkNext) {
        TESWorldSpace* pCurrWorld = static_cast<TESWorldSpace*>(pWorldObj->m_item);
        const char* pName = pCurrWorld->GetFullName();
        kAdjustedCoord = aWorldPos;
        PathingLocation kDestination;
        if (pName && *pName) {
            const char* pTestSubstring = BSstristr(pName, "test");
            if (pTestSubstring != pName && !pCurrWorld->CantTravelFrom()) {
                pCurrWorld->AdjustMapMarkerCoord(kAdjustedCoord, true);
                kDestination = PathingLocation(&kAdjustedCoord, pCurrWorld);
                if (kDestination.ResolveNavMeshInfoOnly() && pRootMapSpace && pCurrWorld) {
                    const char* pRootTextureName = pRootMapSpace->GetWorldMapTextureName();
                    if (BSstrlen(pRootTextureName) >= 31) {
                        const char* pTextureName = pCurrWorld->GetWorldMapTextureName();
                        if (BSstrlen(pTextureName) >= 31) {
                            const char* pTextureSuffix = pCurrWorld->GetWorldMapTextureName() + 29;
                            pRootTextureName = pRootMapSpace->GetWorldMapTextureName();
                            if (!BSstrnicmp(pRootTextureName + 29, pTextureSuffix, 3u)) {
                                pFoundWorld = pCurrWorld;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    if (!pFoundWorld) {
        return static_cast<TESWorldSpace*>(pDataHandler->GetWorldSpaceList()->m_item);
    }
    return pFoundWorld;
}

void MapMenu::StopNoteSound() {
    if (pCurrentSound) {
        if (pCurrentSound->m_item.IsPlaying()) {
            pCurrentSound->m_item.Stop();
        }
    }
    SoundsToPlay.RemoveAll();
    SubtitlesToShow.Clear(true);
    pCurrentSound = nullptr;
    fSoundDurationMillis = 0.0f;
    iSoundPlayStartTime = 0;
    bSoundsLoading = false;
    BSAudio::QInstance().GetSoundHandleByName("UIPipBoyHolotapeStop", 289u).Play(false);
    BSAudio::QInstance().ExitDialogue();
    FalloutAudio::UnMuteRadio();
}

void MapMenu::LoadChallenge(TESChallenge* apChallenge) {
    char sChallengeText[512];
    Tile* pChallengeText = pTiles[13]->GetChildByName("MM_DataText");
    Tile* pScrollbar = pTiles[13]->GetChildByName("MM_TextScrollbar");
    int32_t iTrait = Tile::TextToTrait("_ScrollDelta");
    float fScrollDelta = pScrollbar->GetFloat(iTrait);
    float fHeightDifference = 0.0f;
    iTrait = Tile::TextToTrait("_ItemType");
    pTiles[13]->Set(iTrait, 1);
    const char* pDescription = apChallenge->GetDescription(nullptr, DESC_ID);
    int32_t iThreshold = apChallenge->data.iThreshold;
    // The unsigned progress count is compared with BSMIN as a signed int.
    int32_t iProgress = BSMIN(static_cast<int32_t>(apChallenge->progress.iProgress), iThreshold);
    BSsprintf(sChallengeText, 512, "%s\n%d\\%d\n\n%s", apChallenge->GetFullName(), iProgress, iThreshold, pDescription);
    pChallengeText->Set(4036, sChallengeText, true);
    float fTextHeight = FontManager::Instance()->CalculateStringDimensions(sChallengeText, 1, 400.0f, 0).y;
    double dParentHeight = pChallengeText->pParent->GetFloat(4016);
    fHeightDifference = fTextHeight - dParentHeight;
    fHeightDifference = BSMAX(0.0f, fHeightDifference);
    float fScrollSteps = fHeightDifference / fScrollDelta;
    float fNumItems = std::ceil(fScrollSteps) + 1.0;
    iTrait = Tile::TextToTrait("_number_of_items");
    pScrollbar->Set(iTrait, fNumItems, true);
    iTrait = Tile::TextToTrait("_current_value");
    pScrollbar->Set(iTrait, 0.0f, false);
    iTrait = Tile::TextToTrait("_SetInCode");
    pScrollbar->Poke(iTrait, 1.0);
}

void MapMenu::LoadNote(BGSNote* apNote) {
    switch (apNote->GetNoteType()) {
        case NT_SOUND:
        case NT_VOICE: {
            int32_t iTrait = Tile::TextToTrait("_ItemType");
            pTiles[13]->Set(iTrait, 3);
            if (!pCurrentSound) {
                char acRemaining[64];
                BSsprintf(acRemaining, 64, "--:--:- %s", sRemaining.String());
                iTrait = Tile::TextToTrait("_ItemStringInfo");
                pTiles[13]->Set(iTrait, acRemaining, true);
                iTrait = Tile::TextToTrait("_AudioFileCompletion");
                pTiles[13]->Set(iTrait, 0.0f, true);
            }
            break;
        }
        case NT_TEXT: {
            Tile* pNoteText = pTiles[13]->GetChildByName("MM_DataText");
            Tile* pScrollbar = pTiles[13]->GetChildByName("MM_TextScrollbar");
            int32_t iTrait = Tile::TextToTrait("_ScrollDelta");
            float fScrollDelta = pScrollbar->GetFloat(iTrait);
            float fHeightDifference = 0.0f;
            iTrait = Tile::TextToTrait("_ItemType");
            pTiles[13]->Set(iTrait, 1);
            TESDescription* pNoteDescription = apNote->GetNoteText();
            const char* pText = pNoteDescription->GetDescription(apNote, TNAM_ID);
            pNoteText->Set(4036, pText, true);
            pNoteDescription = apNote->GetNoteText();
            pText = pNoteDescription->GetDescription(apNote, TNAM_ID);
            float fTextHeight = FontManager::Instance()->CalculateStringDimensions(pText, 1, 400.0f, 0).y;
            double dParentHeight = pNoteText->pParent->GetFloat(4016);
            fHeightDifference = fTextHeight - dParentHeight;
            fHeightDifference = BSMAX(0.0f, fHeightDifference);
            float fScrollSteps = fHeightDifference / fScrollDelta;
            float fNumItems = std::ceil(fScrollSteps) + 1.0;
            iTrait = Tile::TextToTrait("_number_of_items");
            pScrollbar->Set(iTrait, fNumItems, true);
            iTrait = Tile::TextToTrait("_current_value");
            pScrollbar->Set(iTrait, 0.0f, false);
            iTrait = Tile::TextToTrait("_SetInCode");
            pScrollbar->Poke(iTrait, 1.0);
            break;
        }
        case NT_IMAGE: {
            const char* pImageName = apNote->GetNoteImage()->GetTextureName();
            Tile* pDataImage = pTiles[13]->GetChildByName("MM_DataImage");
            pDataImage->Set(4044, pImageName, true);
            int32_t iTrait = Tile::TextToTrait("_ItemType");
            pTiles[13]->Set(iTrait, 2);
            break;
        }
        default: {
            break;
        }
    }
    pTiles[13]->Set(4003, 1);
}

void MapMenu::UpdateNotes() {
    if (MapMenu::pMe) {
        // RemoveAll is called through a pointer so that it dispatches virtually; a by-value member call would be
        // devirtualised.
        static_cast<ListBox<BGSNote*>*>(&lbNotesList)->RemoveAll();
        for (int32_t iNoteCt = 0; iNoteCt < pPlayer->GetNumberofNotes(); ++iNoteCt) {
            BGSNote* pCurrNote = pPlayer->GetNotebyIndex(iNoteCt);
            Tile* pNewTile = lbNotesList.AddItem(pCurrNote, pCurrNote->GetFullName(), nullptr, nullptr);
            pNewTile->Set(4010, 24);
            if (pCurrNote->QHasBeenRead()) {
                pNewTile->Set(MapMenu::iUT_TextAlpha, 128.0f, true);
            }
            else {
                pNewTile->Set(MapMenu::iUT_TextAlpha, 255.0f, true);
            }
        }
        lbNotesList.SetEnabled(true);
        lbNotesList.Filter(&MapMenu::NotesFilterFunc);
        int32_t iShowingAllNotes = this->bShowingAllNotes;
        int32_t iTrait = Tile::TextToTrait("_ShowingAllNotes");
        pTiles[12]->Set(iTrait, iShowingAllNotes);
        TESQuest* pActiveQuest = pPlayer->GetActiveQuest();
        pTiles[12]->Set(4015, static_cast<int32_t>(pActiveQuest != nullptr));
    }
}

void MapMenu::ToggleNotesChallengeMode(bool bChange) {
    if (bChange) {
        MapMenu::bNotesChallengeMode = !MapMenu::bNotesChallengeMode;
    }
    lbChallengeList.SetEnabled(MapMenu::bNotesChallengeMode);
    lbChallengeList.SetVisible(MapMenu::bNotesChallengeMode);
    lbNotesList.SetEnabled(!MapMenu::bNotesChallengeMode);
    lbNotesList.SetVisible(!MapMenu::bNotesChallengeMode);
    pTiles[13]->Set(4003, 1);
    if (bChange) {
        Tile* pChallengeText = pTiles[13]->GetChildByName("MM_DataText");
        pChallengeText->Set(4036, "", true);
    }
    if (MapMenu::bNotesChallengeMode) {
        const char* pToggleText = sNotesToggle.String();
        MapMenu::pMe->pTiles[18]->Set(4036, pToggleText, true);
        MapMenu::pMe->pChallengeType->Set(4003, 1);
        const char* pChallengeTypeText = g_challengeStringsArray[MapMenu::cChallengeTypeDisplayed]->String();
        pChallengeType->Set(4036, pChallengeTypeText, true);
    }
    else {
        const char* pToggleText = sChallengeToggle.String();
        MapMenu::pMe->pTiles[18]->Set(4036, pToggleText, true);
        MapMenu::pMe->pTiles[12]->Set(4015, 1);
        MapMenu::pMe->pChallengeType->Set(4003, 0);
    }
    if (bChange) {
        UpdateCurrentPage();
    }
}

void MapMenu::UpdateChallenges() {
    // RemoveAll and Sort are called through a pointer to the list so that they dispatch virtually; by-value member
    // calls would be devirtualised.
    static_cast<ListBox<TESChallenge*>*>(&lbChallengeList)->RemoveAll();
    for (TESChallenge* pItem : *pDataHandler->GetChallengeList()) {
        if (pItem && (!pItem->GetStartsLocked() || pItem->GetUnlocked())) {
            Tile* pNewTile = lbChallengeList.AddItem(pItem, pItem->GetFullName(), nullptr, nullptr);
            pNewTile->Set(4010, 24);
            if (pItem->GetCompleted()) {
                pNewTile->Set(MapMenu::iUT_TextAlpha, 128.0f, true);
            }
            else {
                pNewTile->Set(MapMenu::iUT_TextAlpha, 255.0f, true);
            }
        }
    }
    lbChallengeList.Filter(&MapMenu::ChallengeFilterFunc);
    static_cast<ListBox<TESChallenge*>*>(&lbChallengeList)->Sort(&MapMenu::ChallengeSortFunc);
}

void MapMenu::UpdateQuests() {
    TESQuest* pCurrQuest;
    // RemoveAll, Sort and ScrollToHighlight are called through the lists' vtables; by-value member calls would be
    // devirtualised.
    static_cast<ListBox<TESQuest*>*>(&lbQuestsList)->RemoveAll();
    static_cast<ListBox<BGSQuestObjective*>*>(&lbQuestObjectivesList)->RemoveAll();
    for (BSSimpleList<BGSQuestObjective*>* pObjectiveList = pPlayer->GetQuestObjectiveList();
         pObjectiveList && !pObjectiveList->IsEmpty();
         pObjectiveList = pObjectiveList->m_pkNext) {
        pCurrQuest = pObjectiveList->m_item->pOwnerQuest;
        if (pObjectiveList->m_item->GetDisplayed()) {
            const char* pDisplayText = pObjectiveList->m_item->GetDisplayText();
            Tile* pNewTile = lbQuestObjectivesList.AddItem(pObjectiveList->m_item, pDisplayText, nullptr, nullptr);
            pNewTile->xChildren.Get(pNewTile->xChildren.m_pkTail)->Set(4026, 320.0f, true);
            int32_t iTrait = Tile::TextToTrait("_ShowEmptyMarker");
            pNewTile->Set(iTrait, 1);
            iTrait = Tile::TextToTrait("_enabled");
            pNewTile->Set(iTrait, 0);
            if (pObjectiveList->m_item->GetCompleted()) {
                pNewTile->Set(MapMenu::iUT_TextAlpha, 128.0f, true);
                pNewTile->Set(MapMenu::iUT_Selected, 1);
            }
            else if (pCurrQuest->GetCompleted()) {
                pNewTile->Set(MapMenu::iUT_TextAlpha, 128.0f, true);
            }
            else {
                pNewTile->Set(MapMenu::iUT_TextAlpha, 255.0f, true);
            }
            if (!lbQuestsList.GetTileFor(pCurrQuest)) {
                const char* pFullName = pCurrQuest->GetFullName();
                Tile* pQuestTile = lbQuestsList.AddItem(pCurrQuest, pFullName, nullptr, nullptr);
                pQuestTile->Set(4010, 23);
                if (pCurrQuest->GetCompleted()) {
                    pQuestTile->Set(MapMenu::iUT_TextAlpha, 128.0f, true);
                }
                else {
                    pQuestTile->Set(MapMenu::iUT_TextAlpha, 255.0f, true);
                }
                TESQuest* pActiveQuest = pPlayer->GetActiveQuest();
                if (pCurrQuest == pActiveQuest) {
                    pQuestTile->Set(MapMenu::iUT_Selected, 1);
                }
            }
        }
    }
    static_cast<ListBox<BGSQuestObjective*>*>(&lbQuestObjectivesList)->Sort(&MapMenu::QuestObjectiveSortFunc);
    static_cast<ListBox<TESQuest*>*>(&lbQuestsList)->Sort(&MapMenu::QuestSortFunc);
    lbQuestsList.SetEnabled(true);
    if (pPlayer->GetActiveQuest()) {
        pCurrQuest = pPlayer->GetActiveQuest();
        Tile* pActiveQuestTile = lbQuestsList.GetTileFor(pCurrQuest);
        DoEnter(23, pActiveQuestTile);
        if (InterfaceManager::Has360Controller()) {
            InterfaceManager::Instance()->SetCurrentFocusTarget(pActiveQuestTile, 4035, true);
            static_cast<ListBoxBase*>(&lbQuestsList)->ScrollToHighlight();
        }
    }
    else {
        lbQuestObjectivesList.Filter(&MapMenu::IsQuestMapMenuSelectedQuest);
    }
}

void MapMenu::UpdateRadioList() {
    BSSimpleList<TESObjectREFR*> kInRangeStationList;
    BGSTalkingActivator::GetRadioStationsInRangeOfRef(pPlayer, &kInRangeStationList, nullptr);
    // RemoveAll and Sort are called through a pointer to the list so that they dispatch virtually; by-value member
    // calls would be devirtualised.
    static_cast<ListBox<TESObjectREFR*>*>(&lbRadioStationsList)->RemoveAll();
    for (BSSimpleList<TESObjectREFR*>* pCurrStation = FalloutRadio::QDiscoveredStationsList();
         pCurrStation && pCurrStation->m_item;
         pCurrStation = pCurrStation->m_pkNext) {
        const char* pFullName = pCurrStation->m_item->GetFullName();
        Tile* pStationTile = lbRadioStationsList.AddItem(pCurrStation->m_item, pFullName, nullptr, nullptr);
        pStationTile->Set(4010, 25);
        pFullName = pCurrStation->m_item->GetFullName();
        pStationTile->SetName(pFullName);
        int32_t iSelected = false;
        if (FalloutRadio::GetCurrentStation()) {
            if (pCurrStation->m_item == FalloutRadio::GetCurrentStation()->pStation) {
                iSelected = true;
            }
        }
        pStationTile->Set(MapMenu::iUT_Selected, iSelected);
        if (kInRangeStationList.IsInList(pCurrStation->m_item)) {
            pStationTile->Set(MapMenu::iUT_TextAlpha, 255.0f, true);
        }
        else {
            pStationTile->Set(MapMenu::iUT_TextAlpha, 128.0f, true);
        }
    }
    lbRadioStationsList.Filter(&MapMenu::RadioFilterFunc);
    static_cast<ListBox<TESObjectREFR*>*>(&lbRadioStationsList)->Sort(&MapMenu::RadioSortFunc);
    kInRangeStationList.RemoveAll();
}

NiPoint3 MapMenu::MapToWorldCoord(NiPoint3 aMapCoord, TESWorldSpace* apWorldspace) {
    NiPoint3 kWorldCoord;
    float fX = aMapCoord.x;
    float fY = aMapCoord.y;
    if (iCurrentPage == 33) {
        fX = aMapCoord.x * 1.2549020051956177 - 0.12745098769664764;
        fY = aMapCoord.y * 1.2549020051956177 - 0.12745098769664764;
    }
    kWorldCoord.x = (MapSECorner.x - MapNWCorner.x) * fX + MapNWCorner.x;
    kWorldCoord.y = (MapSECorner.y - MapNWCorner.y) * fY + MapNWCorner.y;
    kWorldCoord.z = 0.0;
    if (apWorldspace && iCurrentPage == 33) {
        apWorldspace->AdjustMapMarkerCoord(kWorldCoord, true);
    }

    return kWorldCoord;
}

void MapMenu::UpdateWorldMap() {
    WorldSpaceMapData* pRootWorldData;
    NiPoint2 kSECorner;
    NiPoint2 kNWCorner;
    if (pRootMapSpace) {
        pRootWorldData = pRootMapSpace->GetWorldMapData();
    }
    else {
        pRootWorldData = nullptr;
    }
    if (!pRootMapSpace || !pRootWorldData || !pRootWorldData->iUsableWidth || !pRootWorldData->iUsableHeight) {
        TESForm* pForm = TESForm::GetFormByNumericID(60u);
        pRootMapSpace = dynamic_cast<TESWorldSpace*>(pForm);
        if (pRootMapSpace) {
            pRootWorldData = pRootMapSpace->GetWorldMapData();
        }
        else {
            pRootWorldData = nullptr;
        }
        bRootSpaceChanged = true;
    }
    if (!pRootMapSpace || !pRootWorldData || !pRootWorldData->iUsableWidth || !pRootWorldData->iUsableHeight) {
        pRootMapSpace = static_cast<TESWorldSpace*>(pDataHandler->GetWorldSpaceList()->m_item);
        if (pRootMapSpace) {
            pRootWorldData = pRootMapSpace->GetWorldMapData();
        }
        else {
            pRootWorldData = nullptr;
        }
        bRootSpaceChanged = true;
    }
    pMapSpace = pRootMapSpace;
    pAdjustNorthRotationCell = nullptr;
    if (pRootMapSpace && pRootWorldData) {
        if (MapMenu::bWorldMapTextureCleared || bRootSpaceChanged) {
            const char* pTextureName = pRootMapSpace->GetWorldMapTextureName();
            pTiles[4]->Set(4044, pTextureName, true);
            pTiles[4]->Set(4045, pRootWorldData->iUsableWidth);
            pTiles[4]->Set(4046, pRootWorldData->iUsableHeight);
            MapMenu::bWorldMapTextureCleared = false;
            bRootSpaceChanged = false;
        }
        ZoomMap(true, 1.0f);
        NiPoint2* pNWCorner = ::new (&kNWCorner) NiPoint2(
            static_cast<float>(pRootWorldData->sNorthWestCellX << 12),
            static_cast<float>((pRootWorldData->sNorthWestCellY << 12) + 4096)
        );
        float fNWCornerY = pNWCorner->y;
        MapNWCorner.x = pNWCorner->x;
        MapNWCorner.y = fNWCornerY;
        NiPoint2* pSECorner = ::new (&kSECorner) NiPoint2(
            static_cast<float>((pRootWorldData->sSouthEastCellX << 12) + 4096),
            static_cast<float>(pRootWorldData->sSouthEastCellY << 12)
        );
        float fSECornerX = pSECorner->x;
        float fSECornerY = pSECorner->y;
        MapSECorner.x = fSECornerX;
        MapSECorner.y = fSECornerY;
        if (!pTiles[4]->GetChildByID(26)) {
            BSSimpleList<TESObjectREFR*>* pFullMarkerList = pRootMapSpace->BuildMapMarkerList(false);
            mapMarkerIndex = 0;
            for (BSSimpleList<TESObjectREFR*>* pCurrMarkerObj = pFullMarkerList;
                 pCurrMarkerObj && pCurrMarkerObj->m_item;
                 pCurrMarkerObj = pCurrMarkerObj->m_pkNext) {
                TESWorldSpace* pMarkerRootWorldspace;
                TESObjectREFR* pCurrMarker = pCurrMarkerObj->m_item;
                NiPoint3 kMarkerPos = pCurrMarker->GetLocationOnReference();
                for (pMarkerRootWorldspace = pCurrMarker->GetWorldSpace();
                     pMarkerRootWorldspace && pMarkerRootWorldspace->GetParentWorldSimple();
                     pMarkerRootWorldspace = pMarkerRootWorldspace->GetParentWorldSimple()) {
                    ;
                }
                MapMarkerData* pMarkerData = pCurrMarker->GetMapMarkerData();
                if (pMarkerData->GetVisible() &&
                    !pCurrMarker->GetDisabled() &&
                    pMarkerRootWorldspace == pRootMapSpace) {
                    Tile* pMarkerTile = MapMenu::pMe->RenderTemplate(pTiles[4], "MapMarkerTemplate", nullptr);
                    if (pMarkerTile) {
                        TESWorldSpace* pMarkerWorldspace = pCurrMarker->GetWorldSpace();
                        NiPoint3 kMarkerTilePos = WorldToMapCoord(kMarkerPos, pMarkerWorldspace);
                        TESReputation* pRep = pCurrMarker->GetMapMarkerData()->pReputation;
                        if (pRep) {
                            char acLocationName[256];
                            const char* pReputationTitle = pRep->GetReputationTitle();
                            pMarkerData = pCurrMarker->GetMapMarkerData();
                            BSsprintf(
                                acLocationName,
                                256,
                                "%s\n(%s)",
                                pMarkerData->GetLocationName(),
                                pReputationTitle
                            );
                            int32_t iTrait = Tile::AddUserTrait("_LocationName", -1);
                            pMarkerTile->Set(iTrait, acLocationName, true);
                        }
                        else {
                            pMarkerData = pCurrMarker->GetMapMarkerData();
                            const char* pLocationName = pMarkerData->GetLocationName();
                            int32_t iTrait = Tile::AddUserTrait("_LocationName", -1);
                            pMarkerTile->Set(iTrait, pLocationName, true);
                        }
                        pMarkerData = pCurrMarker->GetMapMarkerData();
                        const char* pLocationName = pMarkerData->GetLocationName();
                        pMarkerTile->SetName(pLocationName);
                        pMarkerTile->Set(4010, 26);
                        pMarkerTile->Set(MapMenu::iUT_X, kMarkerTilePos.x, true);
                        pMarkerTile->Set(MapMenu::iUT_Y, kMarkerTilePos.y, true);
                        pMarkerData = pCurrMarker->GetMapMarkerData();
                        if (pMarkerData->GetTravelLoc()) {
                            pMarkerData = pCurrMarker->GetMapMarkerData();
                            int32_t iType = pMarkerData->GetType();
                            pMarkerTile->Set(4044, pcMarkerIconFilenamesA[iType - 1], true);
                        }
                        else {
                            pMarkerTile->Set(4044, "Interface\\Icons\\World Map\\icon_map_undiscovered.dds", true);
                            pMarkerTile->Set(4100, 0);
                        }
                        int32_t iMarkerIndex = mapMarkerIndex;
                        int32_t iTrait = Tile::AddUserTrait("_MarkerIndex", -1);
                        pMarkerTile->Set(iTrait, iMarkerIndex);
                        WorldMapMarkerList.AddTail(pCurrMarker);
                        ++mapMarkerIndex;
                    }
                }
            }
            pFullMarkerList->RemoveAll();
            if (pFullMarkerList) {
                delete pFullMarkerList;
            }
        }
        AddMapIcons();
    }
}

void MapMenu::Zoom_RefreshCompanions() {
    BSSimpleList<Actor*>* pCurrTeammate = pPlayer->GetTeammateList();
    while (pCurrTeammate) {
        Tile* pLabel;
        TESObjectREFR* pTarget;
        Actor* pTeammate = pCurrTeammate->m_item;
        TeleportPath kPath;
        if (iCurrentPage == 33) {
            pPlayer->BuildPathToTarget(pTeammate, &kPath, true);
        }
        if (iCurrentPage == 33) {
            pTarget = GetWorldMapTrackingRef(pTeammate, &kPath);
        }
        else {
            pTarget = pTeammate;
        }
        if (pTarget) {
            const char* pFullName = pTarget->GetFullName();
            pLabel = pMapTile->GetChildByName(pFullName);
        }
        else {
            pLabel = nullptr;
        }
        if (pLabel && pLabel->GetString(4036)) {
            const char* pLabelText = pLabel->GetString(4036);
            const char* pTeammateName = pTeammate->GetFullName();
            if (!BSstricmp(pTeammateName, pLabelText)) {
                Tile* pSourceTile;
                const char* pTargetName;
                char acName[68];
                if (pTarget) {
                    pTargetName = pTarget->GetFullName();
                }
                else {
                    pTargetName = nullptr;
                }
                std::sprintf(acName, "%s Marker", pTargetName);
                if (pTarget) {
                    pSourceTile = pMapTile->GetChildByName(acName);
                }
                else {
                    pSourceTile = nullptr;
                }
                NiPoint3 kPos = pTarget->GetLocationOnReference();
                NiPoint3 kLabelPos = kPos;
                double dLabelY = iCurrentPage == 33 ? kLabelPos.y - 1024.0 : kLabelPos.y - 128.0;
                kLabelPos.y = dLabelY;
                if (pAdjustNorthRotationCell) {
                    pAdjustNorthRotationCell->AdjustCoordForNorthRotation(kPos, kPos, true);
                }
                TESWorldSpace* pWorldSpace = pTarget->GetWorldSpace();
                NiPlane kMarkerPlane;
                kMarkerPlane.m_kNormal = WorldToMapCoord(kPos, pWorldSpace);
                pWorldSpace = pTarget->GetWorldSpace();
                NiPlane kLabelPlane;
                kLabelPlane.m_kNormal = WorldToMapCoord(kLabelPos, pWorldSpace);
                float fX = pMapTile->GetFloat(4017);
                kLabelPlane.m_fConstant = pMapTile->GetFloat(4016);
                fX = fX * kLabelPlane.m_kNormal.x;
                kLabelPlane.m_fConstant = kLabelPlane.m_fConstant * kLabelPlane.m_kNormal.y;
                kMarkerPlane.m_fConstant = fX;
                float fY = kLabelPlane.m_fConstant;
                pLabel->Set(MapMenu::iUT_X, kLabelPlane.m_kNormal.x, true);
                pLabel->Set(MapMenu::iUT_Y, kLabelPlane.m_kNormal.y, true);
                pLabel->Set(4001, kMarkerPlane.m_fConstant, true);
                pLabel->Set(4002, fY, true);
                pTeammateName = pTeammate->GetFullName();
                pLabel->Set(4036, pTeammateName, true);
                pLabel->Set(4026, 200.0f, true);
                pLabel->Set(4010, 38);
                pLabel->Set(4013, 2.0f, true);
                pLabel->Set(4003, 1);
                pLabel->Set(4014, 1);
                pLabel->Set(4023, 2);
                pLabel->Set(4025, 2);
                pLabel->Set(4016, 60.0f, true);
                if (pSourceTile) {
                    Tile::Value* pXValue = pLabel->GetValueQ(4001);
                    Tile::Value* pYValue = pLabel->GetValueQ(4002);
                    pXValue->ClearActions();
                    pXValue->AddAction(2000, pSourceTile, 4001);
                    pXValue->CalculateValue(false);
                    pYValue->ClearActions();
                    pYValue->AddAction(2000, pSourceTile, 4002);
                    pYValue->AddAction(2001, 60.0f);
                    pYValue->CalculateValue(false);
                }
                pLabel->AddNeedsUpdate(1023);
            }
        }

        pCurrTeammate = pCurrTeammate->m_pkNext;
    }
}

void MapMenu::ClearCount128() {
    if (MapMenu::pMe) {
        MapMenu::pMe->uiNextLocalMapGrid = 0;
    }
}

void MapMenu::SetCurrentMapPage(uint8_t aucTab) {
    if (pMe) {
        pMe->SetPageNumber(aucTab);
    }
}

bool MapMenu::IsQuestMapMenuSelectedQuest(BGSQuestObjective* apObjective) {
    return apObjective && pMe->pDisplayedQuest != apObjective->GetOwner();
}

bool MapMenu::ShouldRenderLocalMap() {
    if (MapMenu::pMe) {
        const int32_t& kGridsToLoad = uGridsToLoad.UInt();
        if (MapMenu::pMe->uiNextLocalMapGrid < uGridsToLoad.UInt() * kGridsToLoad) {
            return true;
        }
    }
    return false;
}

void MapMenu::RenderLocalMapGrid(bool abSaveToDisk) {
    NiPoint2 kTexCoord;
    NiPoint3 kNormal;
    NiPoint3 kVertex;
    NiPoint3 kExtOffset;
    NiPoint3 kIntOffset;
    if (MapMenu::pMe && MapMenu::pMe->pTiles[2]->GetModel() && MapMenu::ShouldRenderLocalMap()) {
        NiPoint3 kPlayerLoc;
        const NiPoint3* pPlayerLoc = &pPlayer->GetLocationOnReference();
        kPlayerLoc.x = pPlayerLoc->x;
        kPlayerLoc.y = pPlayerLoc->y;
        kPlayerLoc.z = pPlayerLoc->z + 10.0;
        bool bIsInInterior = pPlayer->GetInterior();
        int32_t iGridSize = bIsInInterior ? 2048 : 1024;
        int32_t iNumSubgridsPerSide = 16;
        int32_t iNumSubgrids = 256;
        int32_t iSubgridSize = iGridSize / 16;
        int32_t iVertsPerSide = 17;
        uint32_t uiNumVerts = 289;
        int32_t iTilesPerSide = uGridsToLoad.Int();
        int32_t iMapSize = iTilesPerSide * iGridSize;
        float fPosSize = SeenData::GetSeenPosSize();
        NiColorA kOldSunColor = WaterShader::pSunColor;
        WaterShader::pSunColor = NiColorA::BLACK;
        bool bOldReflection = WaterShader::bNoReflections;
        WaterShader::bNoReflections = true;
        NiColorA kTintColor(0.0f, 0.0f, 0.0f, 0.0f);
        SystemColorManager::GetInstance()->GetColor(4, kTintColor);
        bool bWaterCulled = true;
        if (pPlayer->GetWorldSpace()) {
            TESWorldSpace* pWorldSpace = pPlayer->GetWorldSpace();
            if (pWorldSpace->GetTerrainManager()->GetWaterLODRoot()) {
                pWorldSpace = pPlayer->GetWorldSpace();
                BGSTerrainManager* pTerrainManager = pWorldSpace->GetTerrainManager();
                bWaterCulled = pTerrainManager->GetWaterLODRoot()->GetAppCulled();
                pWorldSpace = pPlayer->GetWorldSpace();
                pTerrainManager = pWorldSpace->GetTerrainManager();
                pTerrainManager->GetWaterLODRoot()->SetAppCulled(true);
            }
        }
        NiNode* pLandLOD = Main::LandLODNode();
        bool bLandCulled = pLandLOD->GetAppCulled();
        if (!bLandCulled) {
            pLandLOD->SetAppCulled(true);
        }
        int32_t iSectX = MapMenu::pMe->uiNextLocalMapGrid % uGridsToLoad.Int();
        int32_t iSectY = MapMenu::pMe->uiNextLocalMapGrid / uGridsToLoad.Int();
        NiPointer<NiTexture> spMapTex(static_cast<NiTexture*>(nullptr));
        TESObjectCELL* pSeenCell = nullptr;
        NiPoint3 kSectOffset = NiPoint3::ZERO;
        if (pPlayer->GetInterior()) {
            pSeenCell = pPlayer->GetParentCell();
            if (pSeenCell) {
                NiPoint3 kPlayerIntLoc = kPlayerLoc;
                pSeenCell->AdjustCoordForNorthRotation(kPlayerLoc, kPlayerIntLoc, true);
                int32_t iSeenX = (FloatToInt(kPlayerIntLoc.x) - 2048) >> 12;
                int32_t iSeenY = (FloatToInt(kPlayerIntLoc.y) - 2048) >> 12;
                int32_t iIntSectionX = iSectX + iSeenX - (iTilesPerSide >> 1);
                int32_t iIntSectionY = iSectY + iSeenY - (iTilesPerSide >> 1);
                kSectOffset = *::new (&kIntOffset) NiPoint3(
                    static_cast<float>((iIntSectionX << 12) + 2048),
                    static_cast<float>((iIntSectionY << 12) + 2048),
                    0.0f
                );
                pSeenCell->GetIntSeenSection(iIntSectionX, iIntSectionY, false);
                NiDX9Renderer* pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
                pRenderer->LockRenderer();
                pSeenCell->GetInteriorLocalMapTexture(iIntSectionX, iIntSectionY, spMapTex);
                pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
                pRenderer->UnlockRenderer();
                if (!iSectX && !iSectY) {
                    pSeenCell->BuildLocalMapDoorList(&MapMenu::pMe->LocalMapMarkerList);
                }
            }
        }
        else {
            TESWorldSpace* pWorld = pPlayer->GetWorldSpace();
            int32_t iCellX = FloatToInt(kPlayerLoc.x) >> 12;
            int32_t iCellY = FloatToInt(kPlayerLoc.y) >> 12;
            int32_t iCellSectionX = iSectX + iCellX - (iTilesPerSide >> 1);
            int32_t iCellSectionY = iSectY + iCellY - (iTilesPerSide >> 1);
            kSectOffset = *::new (
                &kExtOffset
            ) NiPoint3(static_cast<float>(iCellSectionX << 12), static_cast<float>(iCellSectionY << 12), 0.0f);
            if (pWorld) {
                pSeenCell = pWorld->GetCellFromCellCoord(iCellSectionX, iCellSectionY);
            }
            if (pSeenCell) {
                pSeenCell->GetSeenData();
                NiDX9Renderer* pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
                pRenderer->LockRenderer();
                pSeenCell->GetExteriorLocalMapTexture(spMapTex);
                pRenderer = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
                pRenderer->UnlockRenderer();
                pSeenCell->BuildLocalMapDoorList(&MapMenu::pMe->LocalMapMarkerList);
            }
        }
        if (abSaveToDisk) {
            char acFileName[268];
            char acPathName[264];
            std::error_code kError;
            std::filesystem::create_directory(platform::NativePath("(LocalMaps)"), kError);
            std::filesystem::create_directory(platform::NativePath("(LocalMaps)\\Interiors"), kError);
            std::filesystem::create_directory(platform::NativePath("(LocalMaps)\\Exteriors"), kError);
            if (TES::pInstance->pInteriorCell) {
                TESObjectCELL* pInterior = TES::pInstance->pInteriorCell;
                BSsprintf(acPathName, 260, "(LocalMaps)\\Interiors\\%s", pInterior->GetFormEditorID());
                std::filesystem::create_directory(platform::NativePath(acPathName), kError);
                TESObjectCELL* pFileCell = TES::pInstance->pInteriorCell;
                TESObjectCELL* pFolderCell = TES::pInstance->pInteriorCell;
                const char* pFileEditorID = pFileCell->GetFormEditorID();
                BSsprintf(
                    acFileName,
                    260,
                    "(LocalMaps)\\Interiors\\%s\\%s.%i.%i.png",
                    pFolderCell->GetFormEditorID(),
                    pFileEditorID,
                    iSectX,
                    iSectY
                );
            }
            else {
                TESWorldSpace* pWorldSpace = TES::pInstance->pWorldSpace;
                BSsprintf(acPathName, 260, "(LocalMaps)\\Exteriors\\%s", pWorldSpace->GetFormEditorID());
                std::filesystem::create_directory(platform::NativePath(acPathName), kError);
                TESWorldSpace* pFileSpace = TES::pInstance->pWorldSpace;
                TESWorldSpace* pFolderSpace = TES::pInstance->pWorldSpace;
                int32_t iPlayerY = FloatToInt(kPlayerLoc.y);
                int32_t iSectionY = iSectY + (iPlayerY >> 12);
                int32_t iPlayerX = FloatToInt(kPlayerLoc.x);
                int32_t iSectionX = iSectX + (iPlayerX >> 12);
                const char* pFileEditorID = pFileSpace->GetFormEditorID();
                BSsprintf(
                    acFileName,
                    260,
                    "(LocalMaps)\\Exteriors\\%s\\%s.%i.%i.png",
                    pFolderSpace->GetFormEditorID(),
                    pFileEditorID,
                    iSectionX,
                    iSectionY
                );
            }
            // Casts the renderer data to NiObject's RTTI and calls GetD3DTexture.
            NiObject* pRendererData = NiObject::DynamicCastStatic(&NiObject::ms_RTTI, spMapTex->m_pkRendererData);
            IDirect3DBaseTexture9* pD3DTexture = static_cast<NiDX9TextureData*>(pRendererData)->GetD3DTexture();
            // 3 is D3DXIFF_PNG.
            D3DXSaveTextureToFileA_Thunk(acFileName, 3u, pD3DTexture, nullptr);
            ++MapMenu::pMe->uiNextLocalMapGrid;
        }
        else {
            NiAlphaProperty* pAlpha;
            TileShaderProperty* pTSP;
            BSScissorTriShape* pNewGrid;
            int32_t iX;
            int32_t iY;
            NiPoint3* pVerts = new NiPoint3[uiNumVerts];
            NiPoint3* pNorm = new NiPoint3[uiNumVerts];
            NiPoint2* pTex = new NiPoint2[uiNumVerts];
            NiColorA* pColor = new NiColorA[uiNumVerts];
            uint16_t* pTriList = static_cast<uint16_t*>(_NiMalloc(6 * sizeof(uint16_t) * iNumSubgrids));
            int32_t iIndex = 0;
            for (iY = 0; iY < iVertsPerSide; ++iY) {
                for (iX = 0; iX < iVertsPerSide; ++iX) {
                    float fZ = static_cast<double>(iSubgridSize * iY) - static_cast<double>(iGridSize) / 2.0;
                    float fX = static_cast<double>(iSubgridSize * iX) - static_cast<double>(iGridSize) / 2.0;
                    NiPoint3* pVertex = ::new (&kVertex) NiPoint3(fX, 0.0f, fZ);
                    pVerts[iIndex++] = *pVertex;
                }
            }
            iIndex = 0;
            for (iY = 0; iY < iVertsPerSide; ++iY) {
                for (iX = 0; iX < iVertsPerSide; ++iX) {
                    NiPoint3* pNormal = ::new (&kNormal) NiPoint3(0.0f, 1.0f, 0.0f);
                    pNorm[iIndex] = *pNormal;
                    float fV = 1.0 - static_cast<double>(iY) / static_cast<double>(iVertsPerSide);
                    float fU = static_cast<double>(iX) / static_cast<double>(iVertsPerSide);
                    NiPoint2* pTexCoord = ::new (&kTexCoord) NiPoint2(fU, fV);
                    float fTexU = pTexCoord->x;
                    float fTexV = pTexCoord->y;
                    int32_t iTexIndex = iIndex;
                    pTex[iIndex].x = fTexU;
                    pTex[iTexIndex].y = fTexV;
                    float fWorldY = static_cast<double>(iY) * fPosSize;
                    float fWorldX = static_cast<double>(iX) * fPosSize;
                    NiPoint3 kWorldCoord(fWorldX, fWorldY, 0.0f);
                    kWorldCoord += kSectOffset;
                    float fSeenVal = 4.0f;
                    if (MapMenu::pMe->bFogOfWarEnabled) {
                        fSeenVal = static_cast<float>(TESObjectCELL::GetSeenValue(kWorldCoord));
                    }
                    float fAlpha = fSeenVal / 4.0;
                    NiColorA kSeenColor(1.0f, 1.0f, 1.0f, fAlpha);
                    pColor[iIndex] = kSeenColor;
                    ++iIndex;
                }
            }
            iIndex = 0;
            for (iY = 0; iY < iNumSubgridsPerSide; ++iY) {
                for (iX = 0; iX < iNumSubgridsPerSide; ++iX) {
                    uint16_t usLastIndex;
                    if (iX % 2 != iY % 2) {
                        pTriList[iIndex++] = iX + iVertsPerSide * (iY + 1);
                        pTriList[iIndex++] = iX + iVertsPerSide * iY;
                        pTriList[iIndex++] = iVertsPerSide * iY + iX + 1;
                        pTriList[iIndex++] = iVertsPerSide * iY + iX + 1;
                        pTriList[iIndex++] = iVertsPerSide * (iY + 1) + iX + 1;
                        usLastIndex = iX + iVertsPerSide * (iY + 1);
                    }
                    else {
                        pTriList[iIndex++] = iVertsPerSide * (iY + 1) + iX + 1;
                        pTriList[iIndex++] = iX + iVertsPerSide * (iY + 1);
                        pTriList[iIndex++] = iX + iVertsPerSide * iY;
                        pTriList[iIndex++] = iX + iVertsPerSide * iY;
                        pTriList[iIndex++] = iVertsPerSide * iY + iX + 1;
                        usLastIndex = iVertsPerSide * (iY + 1) + iX + 1;
                    }
                    pTriList[iIndex++] = usLastIndex;
                }
            }
            pNewGrid = new BSScissorTriShape(
                uiNumVerts,
                pVerts,
                pNorm,
                pColor,
                pTex,
                1,
                NiGeometryData::NBT_METHOD_NONE,
                2 * iNumSubgrids,
                pTriList,
                0,
                0,
                static_cast<int32_t>(InterfaceManager::GetScreenWidth()),
                static_cast<int32_t>(InterfaceManager::GetScreenHeight())
            );
            pTSP = new TileShaderProperty(false);
            NiTexture* pMapTexture = spMapTex.m_pObject;
            pTSP->SetTileTexture(pMapTexture);
            pTSP->TexClampMode = NiTexturingProperty::CLAMP_S_CLAMP_T;
            pTSP->SetTintColor(kTintColor);
            pTSP->SetUseVertexColors(true);
            spMapTex = static_cast<NiTexture*>(nullptr);
            pNewGrid->AttachProperty(pTSP);
            pAlpha = new NiAlphaProperty;
            pAlpha->SetSrcBlendMode(NiAlphaProperty::ALPHA_SRCALPHA);
            pAlpha->SetDestBlendMode(NiAlphaProperty::ALPHA_ONE);
            pAlpha->SetAlphaBlending(true);
            pNewGrid->AttachProperty(pAlpha);
            BSShaderManager::PrepareObject(pNewGrid, 0, 0);
            float fZ = static_cast<double>(iSectY * iGridSize) + static_cast<double>(iGridSize) / 2.0 -
                static_cast<double>(iMapSize);
            float fX = static_cast<double>(iSectX * iGridSize) + static_cast<double>(iGridSize) / 2.0;
            pNewGrid->SetTranslate(fX, 0.0f, fZ);
            pNewGrid->UpdateProperties();
            NiNode* pModel = MapMenu::pMe->pTiles[2]->GetModel();
            pModel->AttachChild(pNewGrid, 1);
            MapMenu::pMe->pTiles[2]->ClearNeedsUpdate(Tile::CHANGE_TEXTURE);
            WaterShader::pSunColor = kOldSunColor;
            WaterShader::bNoReflections = bOldReflection;
            if (!bWaterCulled && pPlayer->GetWorldSpace()) {
                TESWorldSpace* pWorldSpace = pPlayer->GetWorldSpace();
                BGSTerrainManager* pTerrainManager = pWorldSpace->GetTerrainManager();
                pTerrainManager->GetWaterLODRoot()->SetAppCulled(bWaterCulled);
            }

            if (!bLandCulled) {
                pLandLOD->SetAppCulled(false);
            }
            MapMenu::pMe->pTiles[2]->AddNeedsUpdate(Tile::CHANGE_SCISSOR | Tile::CHANGE_COLOR);
            MapMenu::pMe->pTiles[2]->Set(Tile::kTileValue_FileWidth, iTilesPerSide * iGridSize);
            MapMenu::pMe->pTiles[2]->Set(Tile::kTileValue_FileHeight, iTilesPerSide * iGridSize);
            float fRed = kTintColor.r * 255.0;
            MapMenu::pMe->pTiles[2]->Set(Tile::kTileValue_Red, fRed, true);
            float fGreen = kTintColor.g * 255.0;
            MapMenu::pMe->pTiles[2]->Set(Tile::kTileValue_Green, fGreen, true);
            float fBlue = kTintColor.b * 255.0;
            MapMenu::pMe->pTiles[2]->Set(Tile::kTileValue_Blue, fBlue, true);
            NiUpdateData kUpdateData(0.0f, false, false);
            MapMenu::pMe->pTiles[2]->GetModel()->Update(kUpdateData);
            ++MapMenu::pMe->uiNextLocalMapGrid;
            if (!MapMenu::ShouldRenderLocalMap() && MapMenu::pMe->pMapTile == MapMenu::pMe->pTiles[2]) {
                MapMenu::pMe->UpdateLocalMap();
            }
        }
    }
}

// The marker lists are emptied with the pointer list's RemoveAll and the debug waypoints with the pointer array's
// Clear.
void MapMenu::ClearMapMemory() {
    using enum Tile::eTileValue;

    if (!MapMenu::pMe) {
        return;
    }
    NiNode* pLocalMap = MapMenu::pMe->pTiles[TI_LOCAL_MAP]->GetModel();
    if (!pLocalMap) {
        return;
    }
    for (uint32_t i = 0; i < pLocalMap->GetArrayCount(); ++i) {
        NiAVObject* pChild = pLocalMap->GetAt(i);
        if (pChild) {
            pLocalMap->DetachChild(pChild);
        }
    }
    MapMenu::pMe->pTiles[TI_LOCAL_MAP]->DeleteChildren();
    MapMenu::pMe->pTiles[TI_LOCAL_MAP]->Set(kTileValue_Visible, 0);
    MapMenu::pMe->pTiles[TI_LOCAL_MAP_ARROW]->Set(kTileValue_Visible, 0);
    MapMenu::pMe->pTiles[TI_LOCAL_MAP_ARROW]->RemoveFadeControl(kTileValue_Brightness);
    MapMenu::pMe->LocalMapMarkerList.RemoveAll();
    MapMenu::pMe->pTiles[TI_WORLD_MAP]->DeleteChildren();
    MapMenu::pMe->pTiles[TI_WORLD_MAP_ARROW]->RemoveFadeControl(kTileValue_Brightness);
    MapMenu::pMe->WorldMapMarkerList.RemoveAll();
    MapMenu::pMe->debugWaypoints.Clear(true);
    MapMenu::pMe->uiNextLocalMapGrid = 0;
    MapMenu::pMe->pMapTile = nullptr;

    // The waveform drops its texture.
    NiAVObject* pChild = MapMenu::pMe->pTiles[TI_WAVEFORM]->GetModel()
        ? MapMenu::pMe->pTiles[TI_WAVEFORM]->GetModel()->GetAtChecked(0)
        : nullptr;
    TileShaderProperty* pProperty = pChild
        ? static_cast<TileShaderProperty*>(pChild->GetProperty(NiShadeProperty::GetType()))
        : nullptr;
    if (pProperty && pProperty->GetTileTexture()) {
        pProperty->SetTileTexture(nullptr);
        MapMenu::pMe->pTiles[TI_WAVEFORM]->Set(kTileValue_Filename, "", true);
    }
    MapMenu::pMe->ResetWaveform();
}

void MapMenu::NoteRemoved(BGSNote* apNote) {
    if (apNote) {
        MapMenu::ResetCurrentNoteIfEqualTo(apNote);
    }
}

void MapMenu::ClearWorldMapTexture() {
    using enum Tile::eTileValue;

    if (Interface::IsTopMenuID(Interface::ePipBoyData) && MapMenu::pMe->iCurrentPage == TI_TAB_WORLDMAP) {
        return;
    }
    if (!MapMenu::pMe) {
        return;
    }
    NiAVObject* pChild = MapMenu::pMe->pTiles[TI_WORLD_MAP]->GetModel()
        ? MapMenu::pMe->pTiles[TI_WORLD_MAP]->GetModel()->GetAtChecked(0)
        : nullptr;
    TileShaderProperty* pProperty = pChild
        ? static_cast<TileShaderProperty*>(pChild->GetProperty(NiShadeProperty::GetType()))
        : nullptr;
    if (pProperty && pProperty->GetTileTexture()) {
        pProperty->SetTileTexture(nullptr);
        MapMenu::pMe->pTiles[TI_WORLD_MAP]->Set(kTileValue_Filename, "", true);
        MapMenu::bWorldMapTextureCleared = true;
    }
}

Tile* MapMenu::Create() {
    using enum Tile::eTileValue;

    InterfaceManager* pInterfaceManager = InterfaceManager::Instance();
    TileMenu* pOldMenu = static_cast<TileMenu*>(Menu::GetMenuByClass(Interface::ePipBoyData));
    if (pOldMenu) {
        delete pOldMenu;
    }
    Tile* pFile = pInterfaceManager->GetMenusRoot()->ReadFile("Data\\Menus\\Main\\map_menu.xml");
    MapMenu::pMe = static_cast<MapMenu*>(pFile->GetMenu());
    Menu::RemoveFromXMLArchive("Data\\Menus\\Main\\map_menu.xml", MapMenu::pMe);
    if (!MapMenu::pMe || MapMenu::pMe->GetClass() != Interface::ePipBoyData) {
        if (MapMenu::pMe && MapMenu::pMe->GetMenuTile()) {
            delete MapMenu::pMe;
        }
        return nullptr;
    }
    MapMenu::pMe->SetRenderedMenu();
    MapMenu::pMe->SetMenuTile(static_cast<TileMenu*>(pFile), false);
    if (!MapMenu::pMe->AreAllTilesNonNull()) {
        Warning("MENUS: Map Menu Creation Failed... Are your menu and art resources up to date?");
        return nullptr;
    }
    if (pFile->GetFloat(kTileValue_StackingType) == 6006.0 || pFile->GetFloat(kTileValue_StackingType) == 102.0) {
        pFile->Set(kTileValue_Depth, Tile::GetMaximumDepth(), true);
    }
    const int32_t iMenuClass = GetClassStatic();
    InterfaceManager::Instance()->xTutorialManager.ShowMessage(Interface::TUT_PIPBOY_DATA, iMenuClass, 0x200);

    // The tabs, built from the last argument's text first.
    const char* pCommsText = sCommsTabText.String();
    const char* pMiscText = sMiscTabText.String();
    const char* pQuestsText = sQuestsTabText.String();
    const char* pWorldMapText = sWorldMapTabText.String();
    const char* pLocalMapText = sLocalMapTabText.String();
    Interface::InitializeTabline(
        MapMenu::pMe->pTiles[TI_TABLINE],
        TI_TAB_LOCALMAP,
        pLocalMapText,
        pWorldMapText,
        pQuestsText,
        pMiscText,
        pCommsText,
        static_cast<const char*>(nullptr)
    );

    MapMenu::pMe->lbQuestsList.Initialize(MapMenu::pMe->pTiles[TI_QUESTS_LIST], "MM_ListMarkerTemplate");
    MapMenu::pMe->lbQuestsList.SetAutoUpdate(false);
    MapMenu::pMe->lbQuestObjectivesList.Initialize(MapMenu::pMe->pTiles[TI_QUEST_OBJ_LIST], "MM_ListMarkerTemplate");
    MapMenu::pMe->lbQuestObjectivesList.SetAutoUpdate(false);
    MapMenu::pMe->lbNotesList.Initialize(MapMenu::pMe->pTiles[TI_NOTES_LIST], "MM_ListMarkerTemplate");
    MapMenu::pMe->lbNotesList.SetAutoUpdate(false);
    MapMenu::pMe->lbChallengeList.Initialize(MapMenu::pMe->pTiles[TI_CHALLENGE_LIST], "MM_ListMarkerTemplate");
    MapMenu::pMe->lbRadioStationsList.Initialize(MapMenu::pMe->pTiles[TI_RADIO_STATIONS_LIST], "MM_ListMarkerTemplate");

    // The waveform's texture rotates with the tile.
    NiAVObject* pChild = MapMenu::pMe->pTiles[TI_WAVEFORM]->GetModel()
        ? MapMenu::pMe->pTiles[TI_WAVEFORM]->GetModel()->GetAtChecked(0)
        : nullptr;
    TileShaderProperty* pProperty = pChild
        ? static_cast<TileShaderProperty*>(pChild->GetProperty(NiShadeProperty::GetType()))
        : nullptr;
    if (pProperty) {
        pProperty->SetUseTexScroll(true);
    }

    const int32_t iTitleTrait = Tile::AddUserTrait("_Title", -1);
    MapMenu::pMe->pTiles[TI_MAP_HIGHLIGHT]->Set(iTitleTrait, "", true);
    MapMenu::pMe->pTiles[TI_WORLD_MAP_ARROW]->SetManualTriUpdate(true);
    MapMenu::pMe->pTiles[TI_LOCAL_MAP_ARROW]->SetManualTriUpdate(true);
    MapMenu::ClearMapMemory();
    MapMenu::pMe->PrepForVisibility(true);
    return static_cast<TileMenu*>(pFile);
}

int32_t MapMenu::GetClassStatic() {
    return 1023;
}

// The strings are destroyed with Set(nullptr, 0). Each label's setting text and trait are fetched before pMe is read.
void MapMenu::UpdateMenu() {
    using enum Tile::eTileValue;

    if (!MapMenu::pMe || !Interface::GetPipboy()->GetInitialized()) {
        return;
    }

    // "<date>, <hour>:<minutes>"
    BSStringT<char> text;
    const float fHour = appCalendar.GetHour();
    const int32_t iMinutes = static_cast<int32_t>((fHour - static_cast<int32_t>(fHour)) * 60.0);
    const int32_t iHour = static_cast<int32_t>(fHour);
    text.SPrintF("%s, %d:%02d", appCalendar.GetDateString().String(), iHour, iMinutes);
    MapMenu::pMe->pTiles[TI_PLAYER_TIME]->Set(kTileValue_String, text.String(), true);
    pPlayer->GetMapNameForLocation(text);
    MapMenu::pMe->pTiles[TI_PLAYER_LOCATION]->Set(kTileValue_String, text.String(), true);

    // Outside English the button titles come from the game settings.
    if (BSstrcmp(sLanguage.String(), "ENGLISH")) {
        const char* pLabel = sTravel.String();
        int32_t iTrait = Tile::TextToTrait("_Title_1");
        MapMenu::pMe->pTiles[TI_BUTTON_A]->Set(iTrait, pLabel, true);
        pLabel = sMakeActiveQuest.String();
        iTrait = Tile::TextToTrait("_Title_2");
        MapMenu::pMe->pTiles[TI_BUTTON_A]->Set(iTrait, pLabel, true);
        pLabel = sPlayAudio.String();
        iTrait = Tile::TextToTrait("_Title_3");
        MapMenu::pMe->pTiles[TI_BUTTON_A]->Set(iTrait, pLabel, true);
        pLabel = sTune.String();
        iTrait = Tile::TextToTrait("_Title_4");
        MapMenu::pMe->pTiles[TI_BUTTON_A]->Set(iTrait, pLabel, true);
        pLabel = sStopAudio.String();
        iTrait = Tile::TextToTrait("_Title_5");
        MapMenu::pMe->pTiles[TI_BUTTON_A]->Set(iTrait, pLabel, true);
        MapMenu::pMe->pTiles[TI_BUTTON_A]->GetValueQ(MapMenu::iUT_Title)->CalculateValue(true);
        pLabel = sPlaceMarker.String();
        iTrait = Tile::TextToTrait("_Title_1");
        MapMenu::pMe->pTiles[TI_BUTTON_X]->Set(iTrait, pLabel, true);
        pLabel = sShowLocation.String();
        iTrait = Tile::TextToTrait("_Title_2");
        MapMenu::pMe->pTiles[TI_BUTTON_X]->Set(iTrait, pLabel, true);
        pLabel = sShowAllNotes.String();
        iTrait = Tile::TextToTrait("_Title_3");
        MapMenu::pMe->pTiles[TI_BUTTON_X]->Set(iTrait, pLabel, true);
        pLabel = sShowActiveNotes.String();
        iTrait = Tile::TextToTrait("_Title_4");
        MapMenu::pMe->pTiles[TI_BUTTON_X]->Set(iTrait, pLabel, true);
        pLabel = sNextChallenge.String();
        iTrait = Tile::TextToTrait("_Title_5");
        MapMenu::pMe->pTiles[TI_BUTTON_X]->Set(iTrait, pLabel, true);
        MapMenu::pMe->pTiles[TI_BUTTON_X]->GetValueQ(MapMenu::iUT_Title)->CalculateValue(true);
        pLabel = sCommsTabText.String();
        MapMenu::pMe->pTiles[TI_TABLINE]->GetChildByID(TI_TAB_RADIO)->Set(kTileValue_String, pLabel, true);
        pLabel = sData.String();
        MapMenu::pMe->GetMenuTile()->GetChildByName("MM_Headline_Title")->Set(kTileValue_String, pLabel, true);
    }
    MapMenu::pMe->UpdateCurrentPage();
    text.Set(nullptr, 0);
}

// pMe is reread at each step. bRootSpaceChanged records whether the root worldspace differs from the
// previous one.
void MapMenu::SetRootMapSpace(TESWorldSpace* apNewSpace) {
    if (!MapMenu::pMe) {
        return;
    }
    TESWorldSpace* pOldSpace = MapMenu::pMe->pRootMapSpace;
    for (MapMenu::pMe->pRootMapSpace = apNewSpace;
         MapMenu::pMe->pRootMapSpace && MapMenu::pMe->pRootMapSpace->GetParentWorldSimple();
         MapMenu::pMe->pRootMapSpace = MapMenu::pMe->pRootMapSpace->GetParentWorldSimple()) {}
    MapMenu::pMe->bRootSpaceChanged = pOldSpace != MapMenu::pMe->pRootMapSpace;
}

void MapMenu::RenderNextGridToDisk() {
    if (MapMenu::pMe) {
        MapMenu::RenderLocalMapGrid(true);
    }
}

void MapMenu::ClearSelectedNote() {
    if (MapMenu::pMe) {
        MapMenu::pMe->pSelectedNote = nullptr;
    }
}

void MapMenu::SetKnobPosition(bool abPlaySounds) {
    if (MapMenu::pMe) {
        uint32_t uiTab = static_cast<uint32_t>(MapMenu::pMe->iCurrentPage - 32);
        FOPipboyManager* pPipboy = InterfaceManager::Instance()->GetPipboy();
        pPipboy->SetTabKnobPosition(uiTab, abPlaySounds);
    }
}

void MapMenu::UnselectFunc(Tile* apTile, TESObjectREFR* apStation) {
    apTile->Set(MapMenu::iUT_Selected, 0);
}

bool MapMenu::RadioFilterFunc(TESObjectREFR* pcurrStation) {
    return !pcurrStation ||
        pcurrStation->GetDisabled() ||
        BGSTalkingActivator::IsNonPipboyRadioStation(pcurrStation->GetObjectReference(), 0);
}

// Item 1 sorts after item 2 (1) when it is dimmed (_TextAlpha 128) and item 2 is full alpha (255), before it (-1) the
// other way round; otherwise by list index.
int32_t
MapMenu::QuestObjectiveSortFunc(ListBoxItem<BGSQuestObjective*>* apItem1, ListBoxItem<BGSQuestObjective*>* apItem2) {
    int32_t iResult = 0;
    float fAlpha1 = apItem1->pTile->GetFloat(MapMenu::iUT_TextAlpha);
    float fAlpha2 = apItem2->pTile->GetFloat(MapMenu::iUT_TextAlpha);
    if (fAlpha1 == 128.0 && fAlpha2 == 255.0) {
        iResult = 1;
    }
    else if (fAlpha1 == 255.0 && fAlpha2 == 128.0) {
        iResult = -1;
    }
    else {
        float fIndex1 = apItem1->pTile->GetFloat(Tile::kTileValue_ListIndex);
        float fIndex2 = apItem2->pTile->GetFloat(Tile::kTileValue_ListIndex);
        if (fIndex2 > fIndex1) {
            iResult = -1;
        }
        else if (fIndex2 < fIndex1) {
            iResult = 1;
        }
    }
    return iResult;
}

int32_t MapMenu::QuestSortFunc(ListBoxItem<TESQuest*>* apItem1, ListBoxItem<TESQuest*>* apItem2) {
    int32_t iResult = 0;
    float fAlpha1 = apItem1->pTile->GetFloat(MapMenu::iUT_TextAlpha);
    float fAlpha2 = apItem2->pTile->GetFloat(MapMenu::iUT_TextAlpha);
    if (fAlpha1 == 128.0 && fAlpha2 == 255.0) {
        iResult = 1;
    }
    else if (fAlpha1 == 255.0 && fAlpha2 == 128.0) {
        iResult = -1;
    }
    else {
        float fIndex1 = apItem1->pTile->GetFloat(Tile::kTileValue_ListIndex);
        float fIndex2 = apItem2->pTile->GetFloat(Tile::kTileValue_ListIndex);
        if (fIndex2 > fIndex1) {
            iResult = -1;
        }
        else if (fIndex2 < fIndex1) {
            iResult = 1;
        }
    }
    return iResult;
}

void MapMenu::SetLocalMapMagnification(float afMagnification) {
    if (MapMenu::pMe) {
        MapMenu::pMe->pTiles[2]->Set(MapMenu::iUT_Magnification, afMagnification, true);
    }
}

void MapMenu::SetWorldMapMagnification(float afMagnification) {
    if (MapMenu::pMe) {
        MapMenu::pMe->pTiles[4]->Set(MapMenu::iUT_Magnification, afMagnification, true);
    }
}

void MapMenu::SetPlayerMarker() {
    if (MapMenu::pMe && pPlayer->QPlayerMapMarker()) {
        TESObjectREFR* pMarker = pPlayer->QPlayerMapMarker();
        MapMenu::pMe->PlayerMarkerPos = pMarker->GetLocationOnReference();
        pMarker = pPlayer->QPlayerMapMarker();
        MapMenu::pMe->PlayerMarkerSpace = pMarker->GetWorldSpace();
        if (!MapMenu::pMe->PlayerMarkerSpace) {
            pMarker = pPlayer->QPlayerMapMarker();
            MapMenu::pMe->PlayerMarkerSpace = pMarker->GetParentCell();
        }
    }
}

int32_t MapMenu::RadioSortFunc(ListBoxItem<TESObjectREFR*>* apItem1, ListBoxItem<TESObjectREFR*>* apItem2) {
    if (apItem1->pTile->GetFloat(MapMenu::iUT_TextAlpha) == 255.0 &&
        apItem1->pTile->GetFloat(MapMenu::iUT_TextAlpha) != 255.0) {
        return -1;
    }
    if (apItem1->pTile->GetFloat(MapMenu::iUT_TextAlpha) != 255.0 &&
        apItem1->pTile->GetFloat(MapMenu::iUT_TextAlpha) == 255.0) {
        return 1;
    }
    const char* pcFullName2 = apItem2->data->GetFullName();
    const char* pcFullName1 = apItem1->data->GetFullName();
    return BSstrcmp(pcFullName1, pcFullName2);
}

bool MapMenu::NotesFilterFunc(BGSNote* pcurrNote) {
    int8_t cShow = 1;
    if (!MapMenu::pMe || MapMenu::pMe->bShowingAllNotes) {
        if (MapMenu::pMe && MapMenu::pMe->bShowingAllNotes) {
            return false;
        }
    }
    else {
        for (BSSimpleList<TESQuest*>* pQuestNode = pcurrNote->GetQuestList(); pQuestNode && pQuestNode->m_item;
             pQuestNode = pQuestNode->m_pkNext) {
            if (pQuestNode->m_item == pPlayer->GetActiveQuest()) {
                cShow = 0;
            }
        }
    }
    return cShow;
}

void MapMenu::ResetCurrentNoteIfEqualTo(BGSNote* apNote) {
    if (MapMenu::pMe && MapMenu::pMe->pSelectedNote == apNote) {
        MapMenu::pMe->pSelectedNote = nullptr;
    }
}

void MapMenu::FastTravelCallback() {
    if (Interface::GetMessageMenuResult()) {
        MapMenu::pMe->pFastTravelMarker = nullptr;
    }
    else {
        InterfaceManager::Instance()->ClosePipboy(&MapMenu_RequestPlayerPositionAsSelectedMarker);
    }
    if (!InterfaceManager::Has360Controller()) {
        double dMouseX = InterfaceManager::Instance()->GetPipboyMouseX();
        double dOffsetX = dMouseX - MapMenu::pMe->pTiles[6]->pParent->GetRealX();
        float fX = dOffsetX - MapMenu::pMe->pTiles[6]->GetFloat(4017) / 2.0;
        MapMenu::pMe->pTiles[6]->Set(4001, fX, true);
        double dMouseY = InterfaceManager::Instance()->GetPipboyMouseY();
        double dOffsetY = dMouseY - MapMenu::pMe->pTiles[6]->pParent->GetRealY();
        float fY = dOffsetY - MapMenu::pMe->pTiles[6]->GetFloat(4016) / 2.0;
        MapMenu::pMe->pTiles[6]->Set(4002, fY, true);
    }
    MapMenu::pMe->pTiles[6]->Set(4003, 1);
}

void MapMenu::SetPlayerMarkerCallback() {
    int8_t cResult = Interface::GetMessageMenuResult();
    if (cResult == 1) {
        pPlayer->RemovePlayerMapMarker();
        MapMenu::pMe->DeletePlayerMarker();
    }
    else if (!cResult || cResult == 3) {
        pPlayer->SetPlayerMapMarker(MapMenu::pMe->PlayerMarkerPos, MapMenu::pMe->PlayerMarkerSpace);
        if (!cResult) {
            MapMenu::pMe->DeletePlayerMarker();
        }
        MapMenu::pMe->CreatePlayerMarker();
    }
    MapMenu::pMe->PlayerMarkerPos = NiPoint3::ZERO;
    MapMenu::pMe->PlayerMarkerSpace = nullptr;
    if (!InterfaceManager::Has360Controller()) {
        double dMouseX = InterfaceManager::Instance()->GetPipboyMouseX();
        Tile* pParent = MapMenu::pMe->pTiles[6]->GetParent();
        double dOffsetX = dMouseX - pParent->GetRealX();
        float fX = dOffsetX - MapMenu::pMe->pTiles[6]->GetFloat(4017) / 2.0;
        MapMenu::pMe->pTiles[6]->Set(4001, fX, true);
        double dMouseY = InterfaceManager::Instance()->GetPipboyMouseY();
        pParent = MapMenu::pMe->pTiles[6]->GetParent();
        double dOffsetY = dMouseY - pParent->GetRealY();
        float fY = dOffsetY - MapMenu::pMe->pTiles[6]->GetFloat(4016) / 2.0;
        MapMenu::pMe->pTiles[6]->Set(4002, fY, true);
    }
    MapMenu::pMe->pTiles[6]->Set(4003, 1);
}

void MapMenu::ClearAudioLogSubtites() {
    if (MapMenu::pMe) {
        if (MapMenu::pMe->pCurrentSound) {
            if (MapMenu::pMe->pCurrentSound->m_item.IsPlaying()) {
                MapMenu::pMe->pCurrentSound->m_item.Stop();
            }
        }
        MapMenu::pMe->SoundsToPlay.RemoveAll();
        MapMenu::pMe->SubtitlesToShow.Clear(true);
        MapMenu::pMe->pCurrentSound = nullptr;
        MapMenu::pMe->fSoundDurationMillis = 0.0f;
        MapMenu::pMe->iSoundPlayStartTime = 0;
        MapMenu::pMe->bSoundsLoading = false;
    }
}

void MapMenu::FindQuestObjectiveTargetFunc(Tile* apTile, BGSQuestObjective* apObjective) {
    if (!MapMenu::pMe->pDisplayedQuestTarget && apObjective && !apObjective->GetCompleted()) {
        for (BSSimpleList<TESQuestTarget*>* pTargetObj = &apObjective->ListTargets;
             !MapMenu::pMe->pDisplayedQuestTarget && pTargetObj && pTargetObj->m_item;
             pTargetObj = pTargetObj->m_pkNext) {
            TESQuestTarget* pTarget = pTargetObj->m_item;
            TESObjectREFR* pReference = pTarget->GetReference(false);
            if (pTarget->objConditions.IsTrue(pReference, nullptr)) {
                MapMenu::pMe->pDisplayedQuestTarget = pTarget;
            }
        }
    }
}

bool MapMenu::ChallengeFilterFunc(TESChallenge* pcurrChallenge) {
    if (MapMenu::pMe) {
        pcurrChallenge->GetUnlocked();
    }
    uint32_t uiChallengeType = pcurrChallenge->GetChallengeType();
    if (pcurrChallenge->GetProgressValue() <= 0 &&
        !pcurrChallenge->GetShowZero() &&
        !pcurrChallenge->GetCompletedRecurring()) {
        return true;
    }
    if (!MapMenu::cChallengeTypeDisplayed && uiChallengeType <= 2) {
        return false;
    }
    if (MapMenu::cChallengeTypeDisplayed == 1 && (uiChallengeType == 3 || uiChallengeType == 8)) {
        return false;
    }
    return MapMenu::cChallengeTypeDisplayed != 2 ||
        (uiChallengeType != 11 &&
         uiChallengeType != 10 &&
         uiChallengeType != 9 &&
         uiChallengeType != 6 &&
         uiChallengeType != 5 &&
         uiChallengeType != 4 &&
         uiChallengeType != 12 &&
         uiChallengeType != 13);
}

int32_t MapMenu::ChallengeSortFunc(ListBoxItem<TESChallenge*>* apItem1, ListBoxItem<TESChallenge*>* apItem2) {
    float fAlpha1 = apItem1->pTile->GetFloat(MapMenu::iUT_TextAlpha);
    float fAlpha2 = apItem2->pTile->GetFloat(MapMenu::iUT_TextAlpha);
    if (fAlpha1 == 128.0 && fAlpha2 == 255.0) {
        return 1;
    }
    if (fAlpha1 == 255.0 && fAlpha2 == 128.0) {
        return -1;
    }
    return BSstrcmp(apItem1->data->GetFullName(), apItem2->data->GetFullName());
}

float MapMenu::GetLocalMapMagnification() {
    if (MapMenu::pMe) {
        return MapMenu::pMe->pTiles[2]->GetFloat(MapMenu::iUT_Magnification);
    }

    return 1.0;
}

float MapMenu::GetWorldMapMagnification() {
    if (MapMenu::pMe) {
        return MapMenu::pMe->pTiles[4]->GetFloat(MapMenu::iUT_Magnification);
    }

    return 1.0;
}

bool BGSNote::QHasBeenRead() {
    return bHasBeenRead;
}

BSSimpleList<TESObjectREFR*>* FalloutRadio::QDiscoveredStationsList() {
    return &FalloutRadio::DiscoveredStationList;
}

FalloutRadio::FORadioStation* FalloutRadio::GetCurrentStation() {
    return FalloutRadio::pCurrentPipboyStation;
}
