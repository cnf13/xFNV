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
#include "BSMain/SettingCollection.h"
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
    if ((MapMenu::pMe) && (MapMenu::pMe->pFastTravelMarker)) {
        PLAYER_TARGET_LOC* v0;
        PLAYER_TARGET_LOC* apMemory = static_cast<PLAYER_TARGET_LOC*>(ourMalloc(sizeof(PLAYER_TARGET_LOC)));
        if (apMemory) {
            v0 = ::new (apMemory) PLAYER_TARGET_LOC;
        }
        else {
            v0 = nullptr;
        }
        v0->pFastTravelMarker = MapMenu::pMe->pFastTravelMarker;
        pPlayer->RequestPositionPlayer(v0);
        MapMenu::pMe->pFastTravelMarker = nullptr;
    }
}

MapMenu::MapMenu()
  : iCurrentPage(36), pMapTile(nullptr), fWaveformAnimationStartTime(0.0f), bShowingAllNotes(true),
    pSelectedNote(nullptr), uiNoteEnterStartTime(0), pCurrentSound(nullptr), bSoundsLoading(false),
    fOldPipboyRadioVolume(static_cast<BSWin32Audio*>(&BSAudio::QInstance())->GetRadioVolume()),
    pDisplayedQuest(nullptr), pDisplayedQuestTarget(nullptr), pHighlightedMapMarker(nullptr),
    PlayerMarkerSpace(nullptr), PlayerMarkerPos(NiPoint3_ZERO), pMapSpace(nullptr), pRootMapSpace(nullptr),
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
    if (((uint32_t)aiID) <= 21) {
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
    NiPoint3 v70;
    NiPoint3 dst;
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
                            double v57 = pMapTile->pParent->GetFloat(4016) / 2.0;
                            float v56 = v57 - pMapTile->GetFloat(4002);
                            double v55 = pMapTile->pParent->GetFloat(4017) / 2.0;
                            float v54 = v55 - pMapTile->GetFloat(4001);
                            PlayerMarkerPos = *::new (&dst) NiPoint3(v54, v56, 0.0f);
                        }
                        else {
                            InterfaceManager* v12 = InterfaceManager::Instance();
                            float ya = v12->GetPipboyMouseY();
                            InterfaceManager* v13 = InterfaceManager::Instance();
                            float xa = v13->GetPipboyMouseX();
                            PlayerMarkerPos = *::new (&v70) NiPoint3(xa, ya, 0.0f);
                            PlayerMarkerPos.x = PlayerMarkerPos.x - pMapTile->GetRealX();
                            PlayerMarkerPos.y = PlayerMarkerPos.y - pMapTile->GetRealY();
                        }
                        if (iCurrentPage == 32) {
                            TESWorldSpace* apWorldspace;
                            PlayerMarkerSpace = pMapSpace;
                            if (PlayerMarkerSpace->GetFormType() == WRLD_ID) {
                                apWorldspace = static_cast<TESWorldSpace*>(PlayerMarkerSpace);
                            }
                            else {
                                apWorldspace = nullptr;
                            }
                            float yb = pMapTile->GetFloat(4017);
                            NiPoint3 afMenuWidthOverride = PlayerMarkerPos / yb;
                            PlayerMarkerPos = MapToWorldCoord(afMenuWidthOverride, apWorldspace);
                            PlayerMarkerPos.x = PlayerMarkerPos.x;
                            PlayerMarkerPos.y = PlayerMarkerPos.y;
                            PlayerMarkerPos.z = PlayerMarkerPos.z;
                        }
                        else {
                            float yc = pMapTile->GetFloat(4017);
                            NiPoint3 afMenuWidthOverridea = PlayerMarkerPos / yc;
                            PlayerMarkerPos = MapToWorldCoord(afMenuWidthOverridea, nullptr);
                            PlayerMarkerSpace = (TESForm*)CalculatePlayerMarkerSpace(PlayerMarkerPos);
                            if (PlayerMarkerSpace) {
                                static_cast<TESWorldSpace*>(PlayerMarkerSpace)
                                    ->TESWorldSpace::AdjustMapMarkerCoord(PlayerMarkerPos, true);
                            }
                        }
                        if (pAdjustNorthRotationCell) {
                            pAdjustNorthRotationCell->AdjustCoordForNorthRotation(
                                PlayerMarkerPos, PlayerMarkerPos, false
                            );
                        }
                        pTiles[6]->Set(4003, 0);
                        BSAudio* v14 = &BSAudio::QInstance();
                        v14->GetSoundHandleByName("UIPopUpMapMarkerAdded ", 289u).Play(false);
                        if (pPlayer->QPlayerMapMarker()) {
                            const char* yd = sLeaveMarker.String();
                            const char* xb = sRemoveMarker.String();
                            const char* afMenuWidthOverrideb = sMoveMarker.String();
                            Interface::CreateMessageMenu(
                                sMoveMarkerQuestion.String(), static_cast<const char*>(nullptr),
                                static_cast<const char*>(nullptr), &MapMenu::SetPlayerMarkerCallback, 0, 23, 0.0f, 0.0f,
                                afMenuWidthOverrideb, xb, yd, nullptr
                            );
                        }
                        else {
                            const char* ye = sNo.String();
                            const char* xc = sYes.String();
                            Interface::CreateMessageMenu(
                                sSetMarkerQuestion.String(), static_cast<const char*>(nullptr),
                                static_cast<const char*>(nullptr), &MapMenu::SetPlayerMarkerCallback, 3, 23, 0.0f, 0.0f,
                                xc, ye, nullptr
                            );
                        }
                    }
                    break;
                }
                case 34: {
                    if (pDisplayedQuestTarget) {
                        DoClick(33, nullptr);
                        TeleportPath* y_4b = pDisplayedQuestTarget->QTargetPath();
                        TESObjectREFR* Reference = pDisplayedQuestTarget->GetReference(true);
                        TESObjectREFR* ptargetRef = GetWorldMapTrackingRef(Reference, y_4b);
                        if ((ptargetRef) && (pMapTile)) {
                            TESWorldSpace* y_4c = ptargetRef->GetWorldSpace();
                            NiPoint3 afMenuWidthOverridec = ptargetRef->GetLocationOnReference();
                            NiPoint3 centerPoint = WorldToMapCoord(afMenuWidthOverridec, y_4c);
                            double v51 = pMapTile->pParent->GetFloat(4017) / 2.0;
                            double Float = pMapTile->GetFloat(4017);
                            float v50 = v51 - Float * centerPoint.x;
                            pMapTile->Set(4001, v50, false);
                            double v49 = pMapTile->pParent->GetFloat(4016) / 2.0;
                            double v17 = pMapTile->GetFloat(4016);
                            float v48 = v49 - v17 * centerPoint.y;
                            pMapTile->Set(4002, v48, false);
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
                        const char* v11 = g_challengeStringsArray[MapMenu::cChallengeTypeDisplayed]->String();
                        pChallengeType->Set(4036, v11, true);
                        pTiles[20]->Set(4003, 0);
                        pTiles[13]->Set(4003, 0);
                    }
                    else {
                        bShowingAllNotes = !bShowingAllNotes;
                        lbNotesList.Filter(&MapMenu::NotesFilterFunc);
                        pTiles[13]->Set(4003, 0);
                        int32_t y_4a = bShowingAllNotes;
                        int32_t v10 = Tile::TextToTrait("_ShowingAllNotes");
                        pTiles[12]->Set(v10, y_4a);
                    }
                    break;
                }
                default: {
                    return;
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
            if ((lbQuestsList.GetHighlightedData()) && (!lbQuestsList.GetHighlightedData()->GetCompleted())) {
                TESQuest* y_4 = lbQuestsList.GetHighlightedData();
                pPlayer->SetActiveQuest(y_4);
                Interface::PlayMenuSound(3);
                UpdateCurrentPage();
            }

            break;
        }
        case 24: {
            if (!MapMenu::bNotesChallengeMode) {
                BSSoundHandle hsndHandle;
                BGSNote* pnote = lbNotesList.GetDataFor(apTarget);
                bool v63 = pnote->GetNoteType() == NT_SOUND || pnote->GetNoteType() == NT_VOICE;
                if ((pnote) && (!InterfaceManager::Has360Controller() || v63)) {
                    bool bnoteMatch = pnote == pSelectedNote;
                    bool bsoundStopped = false;
                    Tile* poldSelectedTile = lbNotesList.GetTileFor(pSelectedNote);
                    if (poldSelectedTile) {
                        poldSelectedTile->Set(MapMenu::iUT_Selected, 0);
                    }
                    if (bnoteMatch && (!v63 || pCurrentSound)) {
                        pSelectedNote = nullptr;
                        pTiles[13]->Set(Tile::kTileValue_User0, 0);
                    }
                    else {
                        LoadNote(pnote);
                        apTarget->Set(MapMenu::iUT_Selected, 1);
                        pSelectedNote = pnote;
                        pTiles[13]->Set(Tile::kTileValue_User0, 1);
                    }
                    if (pCurrentSound) {
                        StopNoteSound();
                        bsoundStopped = true;
                    }
                    if (v63 && !pnote->QHasBeenRead()) {
                        apTarget->Set(MapMenu::iUT_TextAlpha, 128.0f, true);
                        pnote->SetHasBeenRead(true);
                    }
                    if (v63 && (!bnoteMatch || !bsoundStopped)) {
                        if (pnote->GetNoteType() || !pnote->GetNoteSound()) {
                            if ((pnote->GetNoteType() == NT_VOICE) && (pnote->GetNoteTopic()) &&
                                (pnote->GetNoteSpeaker())) {
                                Conversation* pVoiceNoteConvo_2;
                                Character* pVoiceNoteNPC;
                                void* apMemory = ourMalloc(sizeof(Character));
                                if (apMemory) {
                                    pVoiceNoteNPC = ::new (apMemory) Character(false);
                                }
                                else {
                                    pVoiceNoteNPC = nullptr;
                                }
                                pVoiceNoteNPC->SetTemporary();
                                DialogueItem* v91 = nullptr;
                                TESActorBase* NoteSpeaker = pnote->GetNoteSpeaker();
                                pVoiceNoteNPC->SetObjectReference(NoteSpeaker);
                                Conversation* v78 = static_cast<Conversation*>(ourMalloc(sizeof(Conversation)));
                                if (v78) {
                                    TESTopic* NoteTopic = pnote->GetNoteTopic();
                                    pVoiceNoteConvo_2 = ::new (v78) Conversation(pVoiceNoteNPC, pPlayer, NoteTopic);
                                }
                                else {
                                    pVoiceNoteConvo_2 = nullptr;
                                }
                                pVoiceNoteConvo_2->FirstItem();
                                if ((pVoiceNoteConvo_2->GetCurrentItem()) &&
                                    (pVoiceNoteConvo_2->GetCurrentItem()->FirstResponse())) {
                                    do {
                                        SubtitlesToShow.Add(
                                            pVoiceNoteConvo_2->GetCurrentItem()->GetCurrentResponse()->cText
                                        );
                                        pVoiceNoteConvo_2->GetCurrentItem()->pInfo->RunResult(
                                            TIRS_BEGIN, pVoiceNoteNPC
                                        );
                                        char* x = pVoiceNoteConvo_2->GetCurrentItem()
                                                      ->GetCurrentResponse()
                                                      ->GetVoice()
                                                      ->pString;
                                        BSAudio* v7 = &BSAudio::QInstance();
                                        hsndHandle = v7->GetSoundHandleByFilename(x, 261u, nullptr);
                                        hsndHandle.SetVolume(0.9f);
                                        SoundsToPlay.AddTail(hsndHandle);
                                        pVoiceNoteConvo_2->GetCurrentItem()->pInfo->RunResult(TIRS_END, pVoiceNoteNPC);
                                        bSoundsLoading = true;
                                    }
                                    while (pVoiceNoteConvo_2->GetCurrentItem()->NextResponse());
                                }

                                if (pVoiceNoteConvo_2) {
                                    delete pVoiceNoteConvo_2;
                                }
                                if (pVoiceNoteNPC) {
                                    delete pVoiceNoteNPC;
                                }
                            }
                        }
                        else {
                            TESSound* NoteSound = pnote->GetNoteSound();
                            uint32_t y = NoteSound->iFormID;
                            BSAudio* v4 = &BSAudio::QInstance();
                            BSSoundHandle apHandle = v4->GetSoundHandleByNumericID(y, 257u);
                            SoundsToPlay.AddTail(apHandle);
                            bSoundsLoading = true;
                        }
                        if (bSoundsLoading) {
                            BSAudio* v8 = &BSAudio::QInstance();
                            v8->GetSoundHandleByName("UIPipBoyHolotapeStart", 289u).Play(false);
                            FalloutAudio::MuteRadio();
                            BSAudio* v9 = &BSAudio::QInstance();
                            v9->EnterDialogue();
                        }
                    }
                }
            }
            break;
        }
        case 25: {
            // Called through a ListBoxBase pointer so the call stays indirect (virtual).
            ListBoxBase* pRadioBase = &lbRadioStationsList;
            Tile* pcurrTile = static_cast<Tile*>(pRadioBase->GetHighlightedItem());
            if (pcurrTile->GetFloat(MapMenu::iUT_TextAlpha) == 255.0) {
                bool bunselected = !pcurrTile->IsTrue(MapMenu::iUT_Selected);
                ResetWaveform();
                lbRadioStationsList.Traverse(&MapMenu::UnselectFunc, 0, 0x7FFFFFFF);
                if (FalloutRadio::IsPipboyRadioEnabled()) {
                    FalloutRadio::PipboyRadioEnable(false);
                }
                if (bunselected) {
                    FalloutRadio::PipboyRadioEnable(true);
                    FalloutRadio::TunePipboyToStation(lbRadioStationsList.GetHighlightedData(), false);
                    pcurrTile->Set(MapMenu::iUT_Selected, 1);
                }
                UpdateCurrentPage();
            }
            break;
        }
        case 26: {
            if (iCurrentPage == 33 &&
                ((InterfaceManager::Has360Controller() ||
                  (pMapTile &&
                   (WorldMapDragStart.x == pMapTile->GetFloat(4001) &&
                    WorldMapDragStart.y == pMapTile->GetFloat(4002)))) &&
                 (pTiles[11]->IsTrue(4015) && pPlayer->CanFastTravel()))) {
                int32_t imarkerCt = 0;
                for (TESObjectREFR*& rpItem : WorldMapMarkerList.ItemsUntilNull()) {
                    int32_t v18 = Tile::AddUserTrait("_MarkerIndex", -1);
                    if (!(apTarget->GetFloat(v18) >= static_cast<double>(imarkerCt))) {
                        break;
                    }
                    pFastTravelMarker = rpItem;
                    ++imarkerCt;
                }
                MapMarkerData* MapMarkerData_ = pFastTravelMarker->GetMapMarkerData();
                if (MapMarkerData_->GetTravelLoc()) {
                    char apDest[260];
                    DoLeave(aiID, apTarget);
                    MapMarkerData* v20 = pFastTravelMarker->GetMapMarkerData();
                    const char* y_4d = v20->GetLocationName();
                    BSsprintf(apDest, 256, "%s %s?", sTravelQuestion.String(), y_4d);
                    pTiles[6]->Set(4003, 0);
                    const char* yf = sNo.String();
                    Interface::CreateMessageMenu(
                        static_cast<const char*>(apDest), static_cast<const char*>(nullptr),
                        static_cast<const char*>(nullptr), &MapMenu::FastTravelCallback, 0, 23, 0.0f, 0.0f,
                        sYes.String(), yf, nullptr
                    );
                }
                else {
                    pFastTravelMarker = nullptr;
                }
            }
            else {
                if (iCurrentPage == 33 && !pTiles[11]->IsTrue(4015)) {
                    int32_t v21 = Tile::AddUserTrait("_LocationName", -1);
                    const char* String = apTarget->GetString(v21);
                    if (_stricmp(String, "Companion")) {
                        const char* v23 = sNoFastTravelUndiscovered.String();
                        Interface::ShowMessage(v23, 2, nullptr, nullptr, 2.0f, false);
                    }
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
            float yg = fMiniBurstIntensity.Float();
            float xd = fMiniBurstDuration.Float();
            InterfaceManager* v24 = InterfaceManager::Instance();
            FOPipboyManager* Pipboy = v24->GetPipboy();
            Pipboy->TriggerBurst(xd, yg, 0.5f);
            if (!Interface::Has360Controller() && aiID != 32 && aiID != 33) {
                InterfaceManager* v26 = InterfaceManager::Instance();
                v26->pCursor->Set(4009, 255.0f, true);
            }
            pTiles[13]->Set(4003, 0);
            pTiles[12]->Set(4015, 1);
            MapMenu::UpdateMenu();
            InterfaceManager* v27 = InterfaceManager::Instance();
            FOPipboyManager* v28 = v27->GetPipboy();
            if (v28->GetInitialized()) {
                MapMenu::SetKnobPosition(true);
            }
            break;
        }
        default: {
            return;
        }
    }
}

void MapMenu::DoEnter(int32_t aiID, Tile* apTarget) {
    static int32_t iprevHighlightIndex;
    int32_t inewHighlightIndex = 0;
    switch (aiID) {
        case -1: {
            if (apTarget) {
                Tile* Parent = apTarget->pParent;
                if (Parent == lbQuestObjectivesList.pScrollBar) {
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
                inewHighlightIndex = static_cast<int32_t>(apTarget->GetFloat(Tile::kTileValue_ListIndex));
                int32_t v3 = Tile::TextToTrait("_ItemType");
                pTiles[13]->Set(v3, 4);
                pTiles[13]->Set(Tile::kTileValue_Visible, 1);
                pDisplayedQuestTarget = nullptr;
                lbQuestObjectivesList.Traverse(&MapMenu::FindQuestObjectiveTargetFunc, 0, 0x7FFFFFFF);
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
                TESChallenge* pcurrChallenge = lbChallengeList.GetDataFor(apTarget);
                if (InterfaceManager::Has360Controller() || pcurrChallenge) {
                    LoadChallenge(pcurrChallenge);
                }
                pTiles[13]->Set(Tile::kTileValue_Visible, 1);
                Interface::PlayMenuSound(4);
            }
            else {
                int32_t inewHighlightIndex_2;
                BGSNote* pcurrNote = lbNotesList.GetDataFor(apTarget);
                ListBoxBase* pNotesBase = &lbNotesList;
                if (pNotesBase->GetHighlightedItem()) {
                    Tile* v4 = static_cast<Tile*>(pNotesBase->GetHighlightedItem());
                    inewHighlightIndex_2 = static_cast<int32_t>(v4->GetFloat(Tile::kTileValue_ListIndex));
                }
                else {
                    inewHighlightIndex_2 = 0;
                }
                inewHighlightIndex = inewHighlightIndex_2;
                if (InterfaceManager::Has360Controller() || (pcurrNote && !pSelectedNote)) {
                    LoadNote(pcurrNote);
                }
                if (pcurrNote->GetNoteType() && pcurrNote->GetNoteType() != NT_VOICE) {
                    if (!pcurrNote->QHasBeenRead()) {
                        uiNoteEnterStartTime = appTimer.iLastTime;
                    }
                }
                else {
                    if (pcurrNote == pSelectedNote && pCurrentSound) {
                        pTiles[13]->Set(Tile::kTileValue_User0, 1);
                    }
                    else {
                        pTiles[13]->Set(Tile::kTileValue_User0, 0);
                    }
                }
            }
            break;
        }
        case 25: {
            int32_t inewHighlightIndex_3;
            ListBoxBase* pRadioBase = &lbRadioStationsList;
            if (pRadioBase->GetHighlightedItem()) {
                Tile* v5 = static_cast<Tile*>(pRadioBase->GetHighlightedItem());
                inewHighlightIndex_3 = static_cast<int32_t>(v5->GetFloat(Tile::kTileValue_ListIndex));
            }
            else {
                inewHighlightIndex_3 = 0;
            }
            inewHighlightIndex = inewHighlightIndex_3;
            break;
        }
        default: {
            break;
        }
    }
    if (iprevHighlightIndex != inewHighlightIndex && (aiID == 23 || (aiID == 24 || aiID == 25))) {
        float afAmount;
        if (iprevHighlightIndex >= inewHighlightIndex) {
            afAmount = -fScrollKnobIncrement.Float();
        }
        else {
            afAmount = fScrollKnobIncrement.Float();
        }
        float afRate = fScrollKnobRate.Float();
        InterfaceManager* v7 = InterfaceManager::Instance();
        FOPipboyManager* Pipboy = v7->GetPipboy();
        Pipboy->MoveKnob(FOPipboyManager::KNOB_SCROLL, afAmount, afRate, true);
        iprevHighlightIndex = inewHighlightIndex;
    }
}

void MapMenu::DoLeave(int32_t aiID, Tile* apTarget) {
    switch (aiID) {
        case -1: {
            if (apTarget) {
                Tile* Parent = apTarget->pParent;
                if (Parent == lbQuestObjectivesList.pScrollBar) {
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
            return;
        }
    }
}

void MapMenu::DoWheelMove(int32_t aiID, Tile* apTarget) {
    InterfaceManager* singleton;
    uint8_t v4;
    if ((iCurrentPage == 33) || (iCurrentPage == 32)) {
        singleton = InterfaceManager::Instance();
        v4 = (uint8_t)(((double)(singleton->GetMouseWheel() / 120)) > 0.0);
        ZoomMap(v4, 1.100000023841858f);
    }
}

// The mouse position is a float that is widened to double before the parent's position is subtracted, so the
// subtraction is done in double.
void MapMenu::DoIdle() {
    using enum Tile::eTileValue;

    if (Interface::IsTopMenuID(Interface::eStartMenu) && this->pCurrentSound &&
        this->pCurrentSound->GetItem().IsPlaying()) {
        this->pCurrentSound->GetItem().Pause();
        wasHolotapePlayingWhenGamePaused = true;
        iSoundPlayPauseTime = appTimer.GetTime();
    }
    else if (
        !Interface::IsTopMenuID(Interface::eStartMenu) && !Interface::IsTopMenuID(Interface::eMessageMenu) &&
        this->pCurrentSound && wasHolotapePlayingWhenGamePaused
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
                    const double fMouseX = InterfaceManager::Instance()->GetPipboyMouseX();
                    float fhighlightX = fMouseX - this->pTiles[TI_MAP_HIGHLIGHT]->GetParent()->GetRealX();
                    const double fMouseY = InterfaceManager::Instance()->GetPipboyMouseY();
                    float fhighlightY = fMouseY - this->pTiles[TI_MAP_HIGHLIGHT]->GetParent()->GetRealY();
                    if (fhighlightX < 0.0 || fhighlightY < 0.0 || fhighlightX > 850.0 || fhighlightY > 500.0) {
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
                    fhighlightX = fhighlightX - this->pTiles[TI_MAP_HIGHLIGHT]->GetFloat(kTileValue_Width) / 2.0;
                    fhighlightY = fhighlightY - this->pTiles[TI_MAP_HIGHLIGHT]->GetFloat(kTileValue_Height) / 2.0;
                    this->pTiles[TI_MAP_HIGHLIGHT]->Set(kTileValue_X, fhighlightX, true);
                    this->pTiles[TI_MAP_HIGHLIGHT]->Set(kTileValue_Y, fhighlightY, true);
                    this->HighlightCenterMarker();
                }
                break;
            case TI_TAB_NOTES:
                // A note shown for fNoteGreyOutTime seconds is marked read and its entry greyed out.
                if (this->uiNoteEnterStartTime) {
                    const double fElapsed = appTimer.GetTime() - this->uiNoteEnterStartTime;
                    if (fNoteGreyOutTime.Float() * 1000.0 <= fElapsed) {
                        BGSNote* pcurrNote = this->pSelectedNote;
                        Tile* pcurrTile = nullptr;
                        if (InterfaceManager::Has360Controller() || !this->pSelectedNote) {
                            pcurrNote = this->lbNotesList.GetHighlightedData();
                        }
                        pcurrTile = this->lbNotesList.GetTileFor(pcurrNote);
                        if (pcurrNote && pcurrTile) {
                            pcurrTile->Set(MapMenu::iUT_TextAlpha, 128.0f, true);
                            pcurrNote->SetHasBeenRead(true);
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
    FOPipboyManager* Pipboy = Interface::GetPipboy();
    if (!Pipboy->GetInitialized()) {
        return false;
    }
    bool bresult = false;
    if (Interface::IsTopMenuID(Interface::ePipBoyData)) {
        if (Interface::Has360Controller()) {
            Controls* Controls_ = appMain->GetControls();
            int32_t v30 = 32767;
            int32_t v31 = 0;
            int32_t ControllerValue = 0;
            if (this->iCurrentPage == 32 || this->iCurrentPage == 33) {
                float v26 = 0.0f;
                Controls* v5 = appMain->GetControls();
                ControllerValue = v5->GetControllerValue(0, 8);
                if (NiAbs(static_cast<float>(ControllerValue)) <= 7849.0) {
                    ControllerValue = 0;
                }
                if (ControllerValue) {
                    float v20 = static_cast<double>(ControllerValue) / 32767.0;
                    float fyscale = NiAbs(v20);
                    float v19 = fyscale * 0.07500000298023224 + 1.0;
                    ZoomMap(ControllerValue > 0, v19);
                    bresult = true;
                }
            }
            Controls* v6 = appMain->GetControls();
            v31 = v6->GetControllerValue(0, 9);
            Controls* v7 = appMain->GetControls();
            ControllerValue = v7->GetControllerValue(0, 10);
            if (NiAbs(static_cast<float>(v31)) <= 8689.0) {
                v31 = 0;
            }
            if (NiAbs(static_cast<float>(ControllerValue)) <= 8689.0) {
                ControllerValue = 0;
            }
            switch (this->iCurrentPage) {
                case 32:
                case 33: {
                    if (pMapTile) {
                        if (v31 || ControllerValue) {
                            pMapTile->Set(4074, 1.0f, true);
                            pMapTile->Set(4075, 1.0f, true);
                            float v18 = static_cast<double>(v31) * -20.0 / 32767.0;
                            pMapTile->Poke(4078, v18);
                            float v17 = static_cast<double>(ControllerValue) * 20.0 / 32767.0;
                            pMapTile->Poke(4079, v17);
                            bresult = true;
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
                    Tile* pscrollbar;
                    if (this->iCurrentPage == 34) {
                        pscrollbar = lbQuestObjectivesList.pScrollBar;
                    }
                    else {
                        pscrollbar = pTiles[13]->GetChildByName("MM_TextScrollbar");
                    }
                    if ((pscrollbar) && (pscrollbar->GetFloat(4003) != 0.0)) {
                        int32_t v24 = 0;
                        if (ControllerValue) {
                            v24 = 2 * (ControllerValue < 0) - 1;
                        }
                        if (v24) {
                            double v15;
                            if (!g_lastMapMenuScrollTime ||
                                (v15 = static_cast<double>(appTimer.iLastTime - g_lastMapMenuScrollTime),
                                 fKeyRepeatInterval.Float() < v15)) {
                                int32_t icurrentValueTrait = Tile::TextToTrait("_current_value");
                                // Truncates the scroll bar's value to an int.
                                int32_t icurrentValue = static_cast<int32_t>(pscrollbar->GetFloat(icurrentValueTrait));
                                pscrollbar->Set(icurrentValueTrait, static_cast<float>(icurrentValue + v24), false);
                                if (static_cast<double>(icurrentValue) != pscrollbar->GetFloat(icurrentValueTrait)) {
                                    int32_t v8 = Tile::TextToTrait("_SetInCode");
                                    pscrollbar->Poke(v8, 1.0f);
                                }
                                g_lastMapMenuScrollTime = appTimer.iLastTime;
                                bresult = true;
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
                int32_t X_4;
                int32_t iCurrentPage;
                this->iCurrentPage += 2 * (aiButton != 4) - 1;
                if (this->iCurrentPage <= 36) {
                    iCurrentPage = this->iCurrentPage;
                }
                else {
                    iCurrentPage = 32;
                }
                this->iCurrentPage = iCurrentPage;
                if (this->iCurrentPage >= 32) {
                    X_4 = this->iCurrentPage;
                }
                else {
                    X_4 = 36;
                }
                this->iCurrentPage = X_4;
                DoClick(this->iCurrentPage, nullptr);
                bresult = true;
                break;
            }
            case 9: {
                if (this->iCurrentPage == 33) {
                    NiTListItem<Tile*>* pos = pTiles[4]->xChildren.m_pkHead;
                    while (pos) {
                        Tile* pcurrTile_2 = pTiles[4]->xChildren.GetNext(pos);
                        if (pcurrTile_2->IsTrue(MapMenu::iUT_Selected)) {
                            DoClick(26, pcurrTile_2);
                            bresult = true;
                            pos = nullptr;
                        }
                    }
                }
                break;
            }
            case 13: {
                InterfaceManager::PlayMenuSound(MENU_SOUND_MODE);
                Interface::SetInventoryMenuVisible(true, false, true);
                bresult = true;
                break;
            }
            case 14: {
                InterfaceManager::PlayMenuSound(MENU_SOUND_MODE);
                Interface::SetStatsMenuVisible(true, false);
                bresult = true;
                break;
            }
            case 15:
            case 16: {
                if (this->iCurrentPage == 32 || this->iCurrentPage == 33) {
                    ZoomMap(aiButton == 16, 1.2f);
                    bresult = true;
                }
                break;
            }
            default: {
                return bresult;
            }
        }
    }
    return bresult;
}

void MapMenu::DoControllerSwitch(bool abHas360Controller) {
    if (abHas360Controller) {
        pTiles[6]->Set(4001, 348.0f, true);
        pTiles[6]->Set(4002, 170.0f, true);
    }
}

void MapMenu::ZoomMap(bool abZoomIn, float afZoomDelta) {
    if ((pMapTile) && (iCurrentPage != 32 || !MapMenu::ShouldRenderLocalMap())) {
        double v56;
        bool bmagnificationChange = true;
        if (abZoomIn) {
            v56 = afZoomDelta;
        }
        else {
            v56 = 1.0 / afZoomDelta;
        }
        float fnewMagnification = pMapTile->GetFloat(MapMenu::iUT_Magnification) * v56;
        double v55 = pMapTile->pParent->GetFloat(4016) / 2.0;
        float v54 = v55 - pMapTile->GetFloat(4002);
        double v53 = pMapTile->pParent->GetFloat(4017) / 2.0;
        float v52 = v53 - pMapTile->GetFloat(4001);
        NiPoint2 MapCenter(v52, v54);
        if (iCurrentPage == 32) {
            float fmagnificationRatio = 0.0f;
            float a2 = fLocalMapMinZoom.Float();
            float* v3 = &fLocalMapMaxZoom.Float();
            float a1 = BSMIN(fnewMagnification, *v3);
            fnewMagnification = BSMAX(a1, a2);
            float* v4 = &fLocalMapMinZoom.Float();
            double v51 = fnewMagnification - *v4;
            float* v5 = &fLocalMapMaxZoom.Float();
            fmagnificationRatio = v51 / (*v5 - fLocalMapMinZoom.Float());
            double v50 = fLocalMapMarkerMinSize.Float();
            double v49 = fLocalMapMarkerMaxSize.Float();
            float v48 = (v49 - fLocalMapMarkerMinSize.Float()) * fmagnificationRatio + v50;
            pMapTile->Set(Tile::kTileValue_User0, v48, true);
            double v47 = fLocalQuestMarkerMinSize.Float();
            double v46 = fLocalQuestMarkerMaxSize.Float();
            float v45 = (v46 - fLocalQuestMarkerMinSize.Float()) * fmagnificationRatio + v47;
            pMapTile->Set(Tile::kTileValue_User1, v45, true);
            double v44 = gs_fLocalfPlayerCursorMinSize.Float();
            double v43 = fLocalPlayerCursorMaxSize.Float();
            float v42 = (v43 - gs_fLocalfPlayerCursorMinSize.Float()) * fmagnificationRatio + v44;
            pMapTile->Set(Tile::kTileValue_User2, v42, true);
        }
        else {
            float fmagnificationRatio_2 = 0.0f;
            float a2a = fWorldMapMinZoom.Float();
            float* v6 = &fWorldMapMaxZoom.Float();
            float a1a = BSMIN(fnewMagnification, *v6);
            fnewMagnification = BSMAX(a1a, a2a);
            float* v7 = &fWorldMapMinZoom.Float();
            double v41 = fnewMagnification - *v7;
            float* v8 = &fWorldMapMaxZoom.Float();
            fmagnificationRatio_2 = v41 / (*v8 - fWorldMapMinZoom.Float());
            double v40 = fWorldMapMarkerMinSize.Float();
            double v39 = fWorldMapMarkerMaxSize.Float();
            float v38 = (v39 - fWorldMapMarkerMinSize.Float()) * fmagnificationRatio_2 + v40;
            pMapTile->Set(Tile::kTileValue_User0, v38, true);
            double v37 = fWorldQuestMarkerMinSize.Float();
            double v36 = fWorldQuestMarkerMaxSize.Float();
            float v35 = (v36 - fWorldQuestMarkerMinSize.Float()) * fmagnificationRatio_2 + v37;
            pMapTile->Set(Tile::kTileValue_User1, v35, true);
            double v34 = gs_fWorldfPlayerCursorMinSize.Float();
            double v33 = fWorldPlayerCursorMaxSize.Float();
            float v32 = (v33 - gs_fWorldfPlayerCursorMinSize.Float()) * fmagnificationRatio_2 + v34;
            pMapTile->Set(Tile::kTileValue_User2, v32, true);
        }
        double Float = pMapTile->GetFloat(MapMenu::iUT_Magnification);
        bmagnificationChange = fnewMagnification != Float;
        double v10 = pMapTile->GetFloat(4045);
        double v31 = v10 * fnewMagnification;
        float v30 = v31 / pMapTile->GetFloat(4017);
        MapCenter *= v30;
        pMapTile->Set(MapMenu::iUT_Magnification, fnewMagnification, true);
        if (iCurrentPage == 32) {
            NiNode* pmapModel = pMapTile->GetModel();
            uint32_t uimapside = uGridsToLoad.UInt();
            for (int32_t isubmapCt = 0; pmapModel && isubmapCt < uimapside * uimapside; ++isubmapCt) {
                NiTriShape* ptri = static_cast<NiTriShape*>(pmapModel->GetAt(isubmapCt));
                if (ptri) {
                    int32_t v29 = TES::pInstance->pInteriorCell != nullptr ? 2048 : 1024;
                    float fnewTrisize = static_cast<double>(v29) * fnewMagnification;
                    float fnewTilesize = fnewTrisize / 16.0;
                    ptri->SetScale(fnewMagnification);
                    float v28 = static_cast<double>(isubmapCt / uimapside) * fnewTrisize + fnewTrisize / 2.0 -
                        static_cast<double>(uimapside) * fnewTrisize;
                    float v27 = static_cast<double>(isubmapCt % uimapside) * fnewTrisize + fnewTrisize / 2.0;
                    ptri->SetTranslate(v27, 0.0f, v28);
                    NiUpdateData upd(0.0f, false, false);
                    ptri->Update(upd);
                    ptri->GetModelData()->SetConsistency(NiGeometryData::STATIC);
                }
            }
        }
        double v11 = pMapTile->pParent->GetFloat(4017);
        double v26 = v11 / 2.0 - MapCenter.x;
        float v25 = v26 - pMapTile->GetFloat(4078);
        pMapTile->Set(4001, v25, false);
        double v12 = pMapTile->pParent->GetFloat(4016);
        double v24 = v12 / 2.0 - MapCenter.y;
        float v23 = v24 - pMapTile->GetFloat(4079);
        pMapTile->Set(4002, v23, false);
        if (bmagnificationChange) {
            double v22;
            float fzoomRateRatio = (afZoomDelta - 1.0) / 0.14300000667572021;
            if (abZoomIn) {
                v22 = -fScrollKnobIncrement.Float() * fzoomRateRatio;
            }
            else {
                v22 = fScrollKnobIncrement.Float() * fzoomRateRatio;
            }
            float* v13 = &fScrollKnobRate.Float();
            float v21 = *v13 + *v13;
            float v20 = v22;
            InterfaceManager* v14 = InterfaceManager::Instance();
            FOPipboyManager* Pipboy = v14->GetPipboy();
            Pipboy->MoveKnob(FOPipboyManager::KNOB_SCROLL, v20, v21, true);
        }
        Zoom_RefreshCompanions();
    }
}

void MapMenu::SetPageNumber(int32_t aiTab) {
    // the tab tile id is the tab index plus 0x20
    this->DoClick(aiTab + 0x20, nullptr);
}

bool MapMenu::AreAllTilesNonNull() {
    for (int32_t i = 0; i < 21; ++i) {
        if (!this->pTiles[i]) {
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
        double Float = pTiles[6]->GetFloat(4002);
        float v20 = pTiles[6]->GetFloat(4016) / 2.0 + Float;
        double v19 = pTiles[6]->GetFloat(4001);
        float v18 = pTiles[6]->GetFloat(4017) / 2.0 + v19;
        NiPoint2 highlightCenter(v18, v20);
        NiTListItem<Tile*>* pos = pMapTile->xChildren.m_pkHead;
        float fshortestDist = 3.4028235e+38f;
        Tile* pclosestTile = nullptr;
        double v1 = pMapTile->GetFloat(4001);
        highlightCenter.x = highlightCenter.x - v1;
        double v2 = pMapTile->GetFloat(4002);
        highlightCenter.y = highlightCenter.y - v2;
        while (pos) {
            Tile* pcurrTile = pMapTile->xChildren.GetNext(pos);
            if ((pcurrTile->GetFloat(4010) == 26.0) && (!std::strstr(pcurrTile->GetName(), " Marker"))) {
                float y = pcurrTile->GetFloat(MapMenu::iUT_Y);
                float x = pcurrTile->GetFloat(MapMenu::iUT_X);
                NiPoint2 currPoint(x, y);
                float fcurrDist = 0.0f;
                currPoint *= pMapTile->GetFloat(4017);
                fcurrDist = (highlightCenter - currPoint).Length();
                if (fshortestDist > static_cast<double>(fcurrDist)) {
                    double v5 = pTiles[6]->GetFloat(4016);
                    if (fcurrDist < v5 / 2.0) {
                        fshortestDist = fcurrDist;
                        pclosestTile = pcurrTile;
                    }
                }
                pcurrTile->Set(MapMenu::iUT_Selected, 0);
            }
        }
        if (pclosestTile) {
            pclosestTile->Set(MapMenu::iUT_Selected, 1);
            if (pclosestTile != pHighlightedMapMarker) {
                BSAudio* v6 = &BSAudio::QInstance();
                v6->GetSoundHandleByName("UIPipBoyHighlight", 289u).Play(false);
                int32_t v7 = Tile::AddUserTrait("_LocationName", -1);
                const char* String = pclosestTile->GetString(v7);
                if (_stricmp(String, "Companion")) {
                    int32_t v9 = Tile::AddUserTrait("_LocationName", -1);
                    const char* xa = pclosestTile->GetString(v9);
                    int32_t v10 = Tile::AddUserTrait("_Title", -1);
                    pTiles[6]->Set(v10, xa, true);
                }
                pHighlightedMapMarker = pclosestTile;
                bool IsTrue = pclosestTile->IsTrue(4100);
                pTiles[11]->Set(4015, static_cast<int32_t>(IsTrue));
            }
        }
        else {
            int32_t v12 = Tile::AddUserTrait("_Title", -1);
            pTiles[6]->Set(v12, "", true);
            pHighlightedMapMarker = nullptr;
            pTiles[11]->Set(4015, 0);
        }
    }
}

void MapMenu::UpdatePlayingSounds() {
    static int32_t ispeechEndTime;
    char Src[68];
    if (bSoundsLoading) {
        bool ballSoundsLoaded = true;
        bool ballSoundsInvalid = true;
        int32_t itotalDuration = 0;
        int32_t inumSounds = 0;
        for (BSSimpleList<BSSoundHandle>* psound = &this->SoundsToPlay; psound; psound = psound->m_pkNext) {
            if (psound->m_item.IsValid()) {
                ballSoundsInvalid = false;
                if (!this->pCurrentSound) {
                    this->pCurrentSound = psound;
                }
                if (psound->m_item.GetDuration()) {
                    itotalDuration += psound->m_item.GetDuration();
                    ++inumSounds;
                }
                else {
                    ballSoundsLoaded = false;
                }
            }
        }
        if (ballSoundsInvalid) {
            this->StopNoteSound();
            BSAudio::QInstance().SetRadioVolume(this->fOldPipboyRadioVolume);
        }
        else if (ballSoundsLoaded) {
            this->bSoundsLoading = false;
            this->fSoundDurationMillis = static_cast<float>(itotalDuration + 500 * (inumSounds - 1));
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
        float felapsedMillis = static_cast<float>(static_cast<int64_t>(appTimer.iLastTime - this->iSoundPlayStartTime));
        felapsedMillis = BSMIN(felapsedMillis, this->fSoundDurationMillis);
        int32_t iremainingMillis = static_cast<int32_t>(this->fSoundDurationMillis - felapsedMillis);
        BSsprintf(
            Src, 64, "%02d:%02d:%01d %s", iremainingMillis / 60000, iremainingMillis % 60000 / 1000,
            iremainingMillis % 1000 / 100, sRemaining.String()
        );
        this->pTiles[13]->Set(Tile::TextToTrait("_ItemStringInfo"), Src, true);
        float fcompletion = felapsedMillis / this->fSoundDurationMillis;
        this->pTiles[13]->Set(Tile::TextToTrait("_AudioFileCompletion"), fcompletion, true);
    }
    if ((this->pCurrentSound) && (!this->pCurrentSound->m_item.IsValid())) {
        if (!ispeechEndTime) {
            ispeechEndTime = appTimer.iLastTime;
        }
        if (this->pCurrentSound->m_pkNext && appTimer.iLastTime - ispeechEndTime > 500) {
            this->pCurrentSound = this->pCurrentSound->m_pkNext;
            this->pCurrentSound->m_item.Play(false);
            ispeechEndTime = 0;
            // Copies the location, then builds the by-value handle with operator=, then reads the text.
            NiPoint3 kSpeakerLoc = pPlayer->GetLocationOnReference();
            BSSoundHandle kHandle;
            kHandle = this->pCurrentSound->m_item;
            Interface::ShowSubtitle(this->SubtitlesToShow.GetAt(0).pString, kHandle, kSpeakerLoc, pPlayer, true);
            this->SubtitlesToShow.Remove(0, true);
        }
        else if (!this->pCurrentSound->m_pkNext) {
            BSsprintf(Src, 64, "00:00:0 %s", sRemaining.String());
            this->pTiles[13]->Set(Tile::TextToTrait("_ItemStringInfo"), Src, true);
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
                if ((pSelectedNote) && (!MapMenu::bNotesChallengeMode)) {
                    Tile* pselectedTile = lbNotesList.GetTileFor(pSelectedNote);
                    LoadNote(pSelectedNote);
                    if (pselectedTile) {
                        pselectedTile->Set(MapMenu::iUT_Selected, 1);
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
            // The list box's virtuals are called through a ListBoxBase pointer so they stay indirect, as the game does.
            ListBoxBase* pRadioBase = &lbRadioStationsList;
            // The highlighted tile's index is kept as a float and truncated before the refresh.
            float fhighlightIndex;
            if (pRadioBase->GetHighlightedItem()) {
                Tile* v1 = static_cast<Tile*>(pRadioBase->GetHighlightedItem());
                fhighlightIndex = v1->GetFloat(4012);
            }
            else {
                fhighlightIndex = -1.0f;
            }
            int32_t ihighlightIndex = static_cast<int32_t>(fhighlightIndex);
            MapMenu::pMe->UpdateRadioList();
            Tile* pnewHighlight = pRadioBase->GetTileAt(ihighlightIndex, false);
            if (pnewHighlight) {
                InterfaceManager* v2 = InterfaceManager::Instance();
                v2->SetCurrentFocusTarget(pnewHighlight, 4035, true);
            }
            pRadioBase->ScrollToHighlight();
            break;
        }
        default: {
            return;
        }
    }
}

void MapMenu::UpdateWaveform() {
    static float fcurrValue;
    TileShaderProperty* ptsp;
    NiAVObject* pavObj;
    int32_t cianimLength = 750;
    int32_t cianimUpdateDelta = 10;
    if (MapMenu::pMe->pTiles[16]->GetModel()) {
        pavObj = MapMenu::pMe->pTiles[16]->GetModel()->GetAtChecked(0);
    }
    else {
        pavObj = nullptr;
    }
    if (pavObj) {
        int32_t Type = NiShadeProperty::GetType();
        ptsp = static_cast<TileShaderProperty*>(pavObj->GetProperty(Type));
    }
    else {
        ptsp = nullptr;
    }
    if (ptsp) {
        char Dst[68];
        if (fWaveformAnimationStartTime == 0.0) {
            NiPointer<NiTexture> spNewTexture(nullptr);
            const char* afScrollAmountY = pTiles[16]->GetString(4050);
            BSsprintf(Dst, 64u, sRadioWaveformFileFormat.String(), afScrollAmountY);
            MapMenu::pMe->pTiles[16]->Set(4044, Dst, true);
            fWaveformAnimationStartTime = static_cast<float>(BSGetTickCount());
            pTiles[16]->Set(4003, 1);
        }
        if (BSGetTickCount() - time_11DA418 >= 10) {
            ptsp->SetTexScroll(fcurrValue, 0.0f);
            fcurrValue = fcurrValue + 0.10000000149011612;
            if (fcurrValue > 1.0) {
                fcurrValue = fcurrValue - 1.0;
            }
            time_11DA418 = BSGetTickCount();
        }
        if (static_cast<double>(BSGetTickCount()) - fWaveformAnimationStartTime >= 750.0) {
            int32_t irand = Random() % 3;
            if (irand) {
                if (irand == 1) {
                    BSstrcpy(Dst, 64, "Backward");
                }
                else {
                    BSstrcpy(Dst, 64, "Left");
                }
            }
            else {
                BSstrcpy(Dst, 64, "Forward");
            }
            pTiles[16]->Set(4050, Dst, true);
            fWaveformAnimationStartTime = 0.0f;
        }
    }
}

NiPoint3 MapMenu::WorldToMapCoord(NiPoint3 aWorldCoord, TESWorldSpace* apWorldspace) {
    // result is built with the empty constructor, which stores nothing.
    NiPoint3 result;
    // 33 is the world map page (TI_TAB_WORLD); only there does the marker coordinate need adjusting.
    if (apWorldspace && iCurrentPage == 33) {
        apWorldspace->AdjustMapMarkerCoord(aWorldCoord, false);
    }
    result.x = (aWorldCoord.x - MapNWCorner.x) / (MapSECorner.x - MapNWCorner.x);
    result.y = (aWorldCoord.y - MapNWCorner.y) / (MapSECorner.y - MapNWCorner.y);
    result.z = 0.0f;
    if (iCurrentPage == 33) {
        // Scales with the double constants 0.796875 and 0.1015625 and rounds back to float.
        result.x = result.x * 0.796875 + 0.1015625;
        result.y = result.y * 0.796875 + 0.1015625;
    }
    return result;
}

// The waypoint tiles are collected in a list on the stack. Each waypoint's position comes from the reference's
// GetLocationOnReference, read after its GetWorldSpace as the game does. The squared lengths go through the CRT's
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
    char v6 = 0;
    for (int32_t ispaceCt = apPathToRef->Spaces.iSize - 1; !v6 && ispaceCt >= 0; --ispaceCt) {
        if ((&apPathToRef->Spaces.GetAt(ispaceCt))->bWorldSpace && ispaceCt < apPathToRef->Doors.iSize) {
            pDoor = (&apPathToRef->Doors.GetAt(ispaceCt))->pDoor;
            v6 = 1;
        }
    }
    if (!v6) {
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
         bShowQuestMarkers.Bool() && pIter && pIter->GetItem(); pIter = pIter->GetNext()) {
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
            (pReference) &&
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
    NiPoint2 v32;
    NiPoint2 v33;
    NiPoint2 v34;
    NiPoint2 v35;
    TESWorldSpace* pworld = pPlayer->GetWorldSpace();
    TESObjectCELL* pplayerCell = pPlayer->GetParentCell();
    NiPoint3 playerLoc = pPlayer->GetLocationOnReference();
    pAdjustNorthRotationCell = pPlayer->GetInterior() ? pplayerCell : nullptr;
    if (pAdjustNorthRotationCell) {
        NiPoint3 playerIntLoc = playerLoc;
        pAdjustNorthRotationCell->AdjustCoordForNorthRotation(playerLoc, playerIntLoc, true);
        int32_t v1 = FloatToInt(playerIntLoc.x);
        int32_t icellx = ((v1 - 2048) >> 12) - (uGridsToLoad.Int() >> 1);
        int32_t v2 = FloatToInt(playerIntLoc.y);
        int32_t icelly = ((v2 - 2048) >> 12) - (uGridsToLoad.Int() >> 1);
        float y = static_cast<float>(((icelly + uGridsToLoad.Int() - 1) << 12) + 2048);
        NiPoint3 NWCellCoord(static_cast<float>((icellx << 12) + 2048), y, 0.0f);
        float ya = static_cast<float>((icelly << 12) + 2048);
        float xa = static_cast<float>(((icellx + uGridsToLoad.Int() - 1) << 12) + 2048);
        NiPoint3 SECellCoord(xa, ya, 0.0f);
        float v30 = NWCellCoord.y + 4096.0;
        NiPoint2* v3 = ::new (&v35) NiPoint2(NWCellCoord.x, v30);
        float v4 = v3->x;
        float v5 = v3->y;
        MapNWCorner.x = v4;
        MapNWCorner.y = v5;
        float v29 = SECellCoord.x + 4096.0;
        NiPoint2* v6 = ::new (&v34) NiPoint2(v29, SECellCoord.y);
        float v7 = v6->x;
        float v8 = v6->y;
        MapSECorner.x = v7;
        MapSECorner.y = v8;
    }
    else {
        int32_t v9 = FloatToInt(playerLoc.x);
        int32_t icellx_2 = (v9 >> 12) - (uGridsToLoad.Int() >> 1);
        int32_t v10 = FloatToInt(playerLoc.y);
        int32_t icelly_2 = (v10 >> 12) - (uGridsToLoad.Int() >> 1);
        float yb = static_cast<float>((icelly_2 + uGridsToLoad.Int() - 1) << 12);
        NiPoint3 NWCellCoord_2(static_cast<float>(icellx_2 << 12), yb, 0.0f);
        float yc = static_cast<float>(icelly_2 << 12);
        float xc = static_cast<float>((icellx_2 + uGridsToLoad.Int() - 1) << 12);
        NiPoint3 SECellCoord_2(xc, yc, 0.0f);
        float v28 = NWCellCoord_2.y + 4096.0;
        NiPoint2* v11 = ::new (&v33) NiPoint2(NWCellCoord_2.x, v28);
        float v12 = v11->y;
        MapNWCorner.x = v11->x;
        MapNWCorner.y = v12;
        float v27 = SECellCoord_2.x + 4096.0;
        NiPoint2* v13 = ::new (&v32) NiPoint2(v27, SECellCoord_2.y);
        float v14 = v13->y;
        MapSECorner.x = v13->x;
        MapSECorner.y = v14;
    }
    ZoomMap(true, 1.0f);
    if (pworld) {
        pMapSpace = pworld;
    }
    else {
        pMapSpace = pplayerCell;
    }
    if (pMapSpace) {
        if (pTiles[2]->xChildren.IsEmpty()) {
            for (TESObjectREFR* pcurrMarker : LocalMapMarkerList.ItemsUntilNull()) {
                TESObjectDOOR* pcurrDoor_2;
                if (pcurrMarker->GetObjectReference()->GetFormType() == DOOR_ID) {
                    pcurrDoor_2 = static_cast<TESObjectDOOR*>(pcurrMarker->GetObjectReference());
                }
                else {
                    pcurrDoor_2 = nullptr;
                }
                TESObjectDOOR* pcurrDoor_3 = pcurrDoor_2;
                NiPoint3 markerPos = pcurrMarker->GetLocationOnReference();
                if ((!pcurrMarker->GetDestroyed() && !pcurrMarker->GetIsHiddenDoor()) && (pcurrDoor_3) &&
                    (!pcurrDoor_3->IsHidden())) {
                    if (pAdjustNorthRotationCell) {
                        pAdjustNorthRotationCell->AdjustCoordForNorthRotation(markerPos, markerPos, true);
                    }
                    float falpha = 255.0f;
                    NiPoint3 markerTilePos = WorldToMapCoord(markerPos, nullptr);
                    if (bFogOfWarEnabled) {
                        int32_t SeenValue = TESObjectCELL::GetSeenValue(markerPos);
                        falpha = static_cast<double>(SeenValue) / 4.0 * falpha;
                    }
                    if (falpha > 0.0 && markerTilePos.x >= 0.0 && markerTilePos.x <= 1.0 && markerTilePos.y >= 0.0 &&
                        markerTilePos.y <= 1.0) {
                        Tile* pmarkerTile = MapMenu::pMe->RenderTemplate(pTiles[2], "MapMarkerTemplate", nullptr);
                        DoorTeleportData* pteleportData = pcurrMarker->GetTeleport();
                        pmarkerTile->Set(4010, 26);
                        pmarkerTile->Set(MapMenu::iUT_X, markerTilePos.x, true);
                        pmarkerTile->Set(MapMenu::iUT_Y, markerTilePos.y, true);
                        pmarkerTile->Set(4009, falpha, true);
                        pmarkerTile->Set(4044, "Interface\\Icons\\Local Map\\icon_local_door.dds", true);
                        if (pteleportData) {
                            BSStringT<char> strDoorDestinationName;
                            pteleportData->GetTeleportCellMapName(strDoorDestinationName);
                            char* yd = strDoorDestinationName.pString;
                            int32_t v15 = Tile::AddUserTrait("_LocationName", -1);
                            pmarkerTile->Set(v15, yd, true);
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
    Tile* v16;
    NiPoint3 a2;
    if (iCurrentPage == 33) {
        v16 = pTiles[5];
    }
    else {
        v16 = pTiles[3];
    }
    NiPoint3 dst(0.5f, 0.5f, 0.0f);
    if (iCurrentPage == 33 && pPlayer->GetInterior()) {
        pMapPlayerRef = pPlayer->pLastExtDoorActivated;
        if (!pMapPlayerRef) {
            TeleportPath apRetPath;
            ActorLockData apMemory(pPlayer);
            bool v21;
            // Constructs the destination, then the origin, and destroys the origin and the destination right after the
            // search: the scope ends there.
            {
                PathingLocation v20;
                PathingLocation apLocation(pPlayer);
                v21 = Pathing::BuildTeleportDoorPath(
                    apLocation, v20, &apRetPath, Pathing::TELEPORT_DOOR_PATH_FIND_FIRST_WORLDSPACE, 0.0f, &apMemory,
                    Pathing::DOOR_LOCK_DISPOSITION_LOCKS_ARE_MINIMAL_USE
                );
            }
            if (v21) {
                if (apRetPath.Doors.iSize) {
                    TeleportLink* Last = &apRetPath.Doors.GetLast();
                    DoorTeleportData* Teleport = Last->pDoor->GetTeleport();
                    pMapPlayerRef = Teleport->pLinkedDoor;
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
        float NorthRotation;
        a2 = pMapPlayerRef->GetLocationOnReference();
        if (pAdjustNorthRotationCell) {
            pAdjustNorthRotationCell->AdjustCoordForNorthRotation(a2, a2, true);
        }
        TESWorldSpace* WorldSpace = pMapPlayerRef->GetWorldSpace();
        dst = WorldToMapCoord(a2, WorldSpace);
        v16->Set(4003, 1);
        v16->Set(MapMenu::iUT_X, dst.x, true);
        v16->Set(MapMenu::iUT_Y, dst.y, true);
        float v22 = -pPlayer->GetHeading(false) - 0.30000001192092896;
        if (pPlayer->GetInterior()) {
            TESObjectCELL* ParentCell = pPlayer->GetParentCell();
            NorthRotation = ParentCell->GetNorthRotation();
        }
        else {
            NorthRotation = 0.0f;
        }
        v22 = v22 - NorthRotation;
        v16->Set(4090, v22, true);
    }
    else {
        v16->Set(4003, 0);
    }
    if (!v16->HasFadeControl(4085)) {
        v16->AddFadeControl(4085, 128.0f, 255.0f, 1.0f, Tile::TFCT_NONLINEAR_LONG_DARK_REPEATING);
    }
    Tile* ProcessLevel = pMapTile->pParent;
    double v12 = ProcessLevel->GetFloat(4017) / 2.0;
    double Float = pMapTile->GetFloat(4017);
    float v11 = v12 - Float * dst.x;
    pMapTile->Set(4001, v11, false);
    Tile* v7 = pMapTile->pParent;
    double v10 = v7->GetFloat(4016) / 2.0;
    double v8 = pMapTile->GetFloat(4016);
    float v9 = v10 - v8 * dst.y;
    pMapTile->Set(4002, v9, false);
    AddQuestMarkers();
    AddCompanionMarkers();
    if (pPlayer->QPlayerMapMarker()) {
        DeletePlayerMarker();
        CreatePlayerMarker();
    }
}

void MapMenu::AddCompanionMarkers() {
    Tile* element;
    for (element = pMapTile->GetChildByID(37); element; element = pMapTile->GetChildByID(37)) {
        pMapTile->xChildren.Remove(element);
        element->Release();
        Tile* v20 = element;
        if (element) {
            delete v20;
        }
    }
    while (true) {
        element = pMapTile->GetChildByID(38);
        if (!element) {
            break;
        }
        pMapTile->xChildren.Remove(element);
        element->Release();
        Tile* v18 = element;
        if (element) {
            delete v18;
        }
    }
    BSSimpleList<Actor*>* pcurrTeammate = pPlayer->GetTeammateList();
    while (pcurrTeammate) {
        // The marker's reference and its map position.
        TESObjectREFR* pTarget;
        NiPoint3 kMapPos;
        Actor* a1 = pcurrTeammate->m_item;
        TeleportPath a2;
        if (iCurrentPage == 33) {
            pPlayer->BuildPathToTarget(a1, &a2, true);
        }
        if (iCurrentPage == 33) {
            pTarget = GetWorldMapTrackingRef(a1, &a2);
        }
        else {
            pTarget = a1;
        }
        if ((pTarget) && (!pTarget->IsDead(false))) {
            TESObjectCELL* v1;
            TESWorldSpace* v2;
            if (iCurrentPage != 32 ||
                ((v1 = a1->GetParentCell(), v1 == pPlayer->GetParentCell()) ||
                 (v2 = a1->GetWorldSpace(), v2 == pPlayer->GetWorldSpace()))) {
                float DistanceFromReference = a1->GetDistanceFromReference(pPlayer, false, false);
                if (!(DistanceFromReference < 2048.0) || iCurrentPage != 33) {
                    if (a1) {
                        TESObjectCELL* ParentCell;
                        if ((iCurrentPage == 33 && !a1->GetInterior()) ||
                            (iCurrentPage == 32 &&
                             (ParentCell = a1->GetParentCell(), ParentCell == pPlayer->GetParentCell()))) {
                            pTarget = a1;
                        }
                    }
                    if (pTarget) {
                        double v4;
                        Tile* v12;
                        char apName[64];
                        NiPoint3 out = pTarget->GetLocationOnReference();
                        NiPoint3 v31 = out;
                        if (iCurrentPage == 33) {
                            v4 = v31.y - 1024.0;
                        }
                        else {
                            v4 = v31.y - 128.0;
                        }
                        v31.y = v4;
                        if (pAdjustNorthRotationCell) {
                            pAdjustNorthRotationCell->AdjustCoordForNorthRotation(out, out, true);
                        }
                        TESWorldSpace* WorldSpace = pTarget->GetWorldSpace();
                        kMapPos = WorldToMapCoord(out, WorldSpace);
                        TESWorldSpace* v6 = pTarget->GetWorldSpace();
                        NiPoint3 in = WorldToMapCoord(v31, v6);
                        if (iCurrentPage == 33) {
                            v12 = pTiles[5];
                        }
                        else {
                            v12 = pTiles[3];
                        }
                        Tile* apSourceTile = v12;
                        Tile* v25 = nullptr;
                        Tile* v29 = nullptr;
                        if (iCurrentPage == 33) {
                            v25 = MapMenu::pMe->RenderTemplate(pMapTile, "WorldMapQuestMarkerTemplate", nullptr);
                        }
                        else {
                            v25 = MapMenu::pMe->RenderTemplate(pMapTile, "LocalMapQuestMarkerTemplate", nullptr);
                        }
                        v25->Set(MapMenu::iUT_X, kMapPos.x, true);
                        v25->Set(MapMenu::iUT_Y, kMapPos.y, true);
                        const char* FullName = pTarget->GetFullName();
                        std::sprintf(apName, "%s Marker", FullName);
                        v25->SetName(static_cast<const char*>(apName));
                        v25->Set(Tile::kTileValue_Filename, "glow_hud_compass_landmark.dds", true);
                        v25->Set(Tile::kTileValue_ID, 26);
                        v25->Set(Tile::kTileValue_Depth, 5.0f, true);
                        int32_t v8 = Tile::AddUserTrait("_LocationName", -1);
                        v25->Set(v8, "Companion", true);
                        Tile::Value* ValueQ = v25->GetValueQ(Tile::kTileValue_Brightness);
                        ValueQ->ClearActions();
                        ValueQ->AddAction(Tile::kTileValue_Copy, apSourceTile, Tile::kTileValue_Brightness);
                        ValueQ->CalculateValue(false);
                        float Float = pMapTile->GetFloat(Tile::kTileValue_Width);
                        float v23 = pMapTile->GetFloat(Tile::kTileValue_Height);
                        Float = Float * in.x;
                        v23 = v23 * in.y;
                        v29 = Tile::BuildTile(903);
                        const char* v9 = a1->GetFullName();
                        v29->Init(pMapTile, v9, nullptr);
                        v29->Set(MapMenu::iUT_X, in.x, true);
                        v29->Set(MapMenu::iUT_Y, in.y, true);
                        const char* v10 = pTarget->GetFullName();
                        v29->SetName(v10);
                        v29->Set(Tile::kTileValue_X, Float, true);
                        v29->Set(Tile::kTileValue_Y, v23, true);
                        const char* v11 = a1->GetFullName();
                        v29->Set(Tile::kTileValue_String, v11, true);
                        v29->Set(Tile::kTileValue_WrapWidth, 200.0f, true);
                        v29->Set(Tile::kTileValue_ID, 38);
                        v29->Set(Tile::kTileValue_Depth, 2.0f, true);
                        v29->Set(Tile::kTileValue_Visible, 1);
                        v29->Set(Tile::kTileValue_Justify, 2);
                        v29->Set(Tile::kTileValue_Font, 2);
                        v29->Set(Tile::kTileValue_Height, 60.0f, true);
                        v29->AddNeedsUpdate(Tile::CHANGE_CREATE);
                        Tile::Value* v27 = v29->GetValueQ(Tile::kTileValue_X);
                        Tile::Value* v26 = v29->GetValueQ(Tile::kTileValue_Y);
                        v27->ClearActions();
                        v27->AddAction(Tile::kTileValue_Copy, v25, Tile::kTileValue_X);
                        v27->CalculateValue(false);
                        v26->ClearActions();
                        v26->AddAction(Tile::kTileValue_Copy, v25, Tile::kTileValue_Y);
                        v26->AddAction(Tile::kTileValue_Add, 60.0f);
                        v26->CalculateValue(false);
                        Tile::Value* v22 = v29->GetValueQ(Tile::kTileValue_Brightness);
                        v22->ClearActions();
                        v22->AddAction(Tile::kTileValue_Copy, apSourceTile, Tile::kTileValue_Brightness);
                        v22->CalculateValue(false);
                    }
                }
            }
        }

        pcurrTeammate = pcurrTeammate->GetNext();
    }
}

void MapMenu::CreatePlayerMarker() {
    TESObjectREFR* ptarget;
    NiPoint3 markerWorldPos;
    if (iCurrentPage == 33) {
        TeleportPath* PlacedMarkerTarget = pPlayer->QPlayerMarkerPath();
        TESObjectREFR* PlayerMapMarker = pPlayer->QPlayerMapMarker();
        ptarget = GetWorldMapTrackingRef(PlayerMapMarker, PlacedMarkerTarget);
    }
    else {
        ptarget = pPlayer->QPlayerMarkerTrackingRef();
    }
    TESObjectREFR* ptarget_2 = ptarget;
    if (ptarget) {
        if (iCurrentPage == 33) {
            TeleportPath* v2 = pPlayer->QPlayerMarkerPath();
            CreateWaypointPath(pMapPlayerRef, ptarget_2, v2, 30);
        }
    }
    else {
        if (pPlayer->QPlayerMapMarker()) {
            TESObjectREFR* v3;
            TESObjectCELL* ParentCell;
            if ((iCurrentPage == 33 && (v3 = pPlayer->QPlayerMapMarker(), !v3->GetInterior())) ||
                (iCurrentPage == 32 &&
                 (ParentCell = pPlayer->QPlayerMapMarker()->GetParentCell(), ParentCell == pPlayer->GetParentCell()))) {
                ptarget_2 = pPlayer->QPlayerMapMarker();
            }
        }
    }
    if (ptarget_2) {
        Tile* parrowTile;
        markerWorldPos = ptarget_2->GetLocationOnReference();
        if (pAdjustNorthRotationCell) {
            pAdjustNorthRotationCell->AdjustCoordForNorthRotation(markerWorldPos, markerWorldPos, true);
        }
        TESWorldSpace* WorldSpace = ptarget_2->GetWorldSpace();
        NiPoint3 markerTilePos = WorldToMapCoord(markerWorldPos, WorldSpace);
        if (iCurrentPage == 33) {
            parrowTile = pTiles[5];
        }
        else {
            parrowTile = pTiles[3];
        }
        Tile* psubMarkerTile = nullptr;
        if (iCurrentPage == 33) {
            psubMarkerTile = MapMenu::pMe->RenderTemplate(pMapTile, "WorldMapQuestMarkerTemplate", nullptr);
        }
        else {
            psubMarkerTile = MapMenu::pMe->RenderTemplate(pMapTile, "LocalMapQuestMarkerTemplate", nullptr);
        }
        psubMarkerTile->Set(MapMenu::iUT_X, markerTilePos.x, true);
        psubMarkerTile->Set(MapMenu::iUT_Y, markerTilePos.y, true);
        psubMarkerTile->Set(4044, "glow_hud_compass_pc_marker.dds", true);
        psubMarkerTile->Set(4010, 28);
        psubMarkerTile->Set(4013, 5.0f, true);
        Tile::Value* pbrightness = psubMarkerTile->GetValueQ(4085);
        pbrightness->ClearActions();
        pbrightness->AddAction(2000, parrowTile, 4085);
        pbrightness->CalculateValue(false);
    }
}

void MapMenu::DeletePlayerMarker() {
    Tile* element = pMapTile->GetChildByID(28);
    if (element) {
        pMapTile->xChildren.Remove(element);
        element->Release();
        if (element) {
        LABEL_3:;
            delete element;
        }
    }
    while (true) {
        element = pMapTile->GetChildByID(30);
        if (!element) {
            break;
        }
        pMapTile->xChildren.Remove(element);
        element->Release();
        if (element) {
            goto LABEL_3;
        }
    }
}

TESWorldSpace* MapMenu::CalculatePlayerMarkerSpace(NiPoint3 aWorldPos) {
    NiPoint3 adjustedCoord;
    TESWorldSpace* presult = nullptr;
    float fshortestDist = 3.4028235e+38f;
    for (BSSimpleList<TESWorldSpace*>* apworldObj = pDataHandler->GetWorldSpaceList(); apworldObj && apworldObj->m_item;
         apworldObj = apworldObj->m_pkNext) {
        TESWorldSpace* pcurrWorld = static_cast<TESWorldSpace*>(apworldObj->m_item);
        const char* strName = pcurrWorld->GetFullName();
        adjustedCoord = aWorldPos;
        PathingLocation destination;
        if ((strName) && (*strName)) {
            const char* v2 = BSstristr(strName, "test");
            if (v2 != strName && !pcurrWorld->CantTravelFrom()) {
                pcurrWorld->AdjustMapMarkerCoord(adjustedCoord, true);
                destination = PathingLocation(&adjustedCoord, pcurrWorld);
                if ((destination.ResolveNavMeshInfoOnly()) && (pRootMapSpace) && (pcurrWorld)) {
                    const char* WorldMapTextureName = pRootMapSpace->GetWorldMapTextureName();
                    if (BSstrlen(WorldMapTextureName) >= 31) {
                        const char* v4 = pcurrWorld->GetWorldMapTextureName();
                        if (BSstrlen(v4) >= 31) {
                            const char* v7 = pcurrWorld->GetWorldMapTextureName() + 29;
                            const char* v5 = pRootMapSpace->GetWorldMapTextureName();
                            if (!BSstrnicmp(v5 + 29, v7, 3u)) {
                                presult = pcurrWorld;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    if (!presult) {
        return static_cast<TESWorldSpace*>(pDataHandler->GetWorldSpaceList()->m_item);
    }
    return presult;
}

void MapMenu::StopNoteSound() {
    if (pCurrentSound) {
        BSSoundHandle* v1 = &pCurrentSound->m_item;
        if (v1->IsPlaying()) {
            BSSoundHandle* v2 = &pCurrentSound->m_item;
            v2->Stop();
        }
    }
    SoundsToPlay.RemoveAll();
    SubtitlesToShow.Clear(true);
    pCurrentSound = nullptr;
    fSoundDurationMillis = 0.0f;
    iSoundPlayStartTime = 0;
    bSoundsLoading = false;
    BSAudio* v3 = &BSAudio::QInstance();
    v3->GetSoundHandleByName("UIPipBoyHolotapeStop", 289u).Play(false);
    BSAudio* v4 = &BSAudio::QInstance();
    v4->ExitDialogue();
    FalloutAudio::UnMuteRadio();
}

void MapMenu::LoadChallenge(TESChallenge* apChallenge) {
    char sChallengeText[512];
    Tile* pchallengeText = pTiles[13]->GetChildByName("MM_DataText");
    Tile* pscrollbar = pTiles[13]->GetChildByName("MM_TextScrollbar");
    int32_t v2 = Tile::TextToTrait("_ScrollDelta");
    float fscrollDelta = pscrollbar->GetFloat(v2);
    float fheightDifference = 0.0f;
    int32_t v3 = Tile::TextToTrait("_ItemType");
    pTiles[13]->Set(v3, 1);
    const char* a2 = apChallenge->GetDescription(nullptr, DESC_ID);
    int32_t a1 = apChallenge->data.iThreshold;
    int32_t NumberOfItemsActivated = apChallenge->data.iThreshold;
    // The unsigned progress count is compared with BSMIN as a signed int.
    int32_t v5 = BSMIN(static_cast<int32_t>(apChallenge->progress.iProgress), NumberOfItemsActivated);
    BSsprintf(sChallengeText, 512, "%s\n%d\\%d\n\n%s", apChallenge->GetFullName(), v5, a1, a2);
    pchallengeText->Set(4036, sChallengeText, true);
    FontManager* Singleton = FontManager::Instance();
    float ftextHeight = Singleton->CalculateStringDimensions(sChallengeText, 1, 400.0f, 0).y;
    double Float = pchallengeText->pParent->GetFloat(4016);
    fheightDifference = ftextHeight - Float;
    fheightDifference = BSMAX(0.0f, fheightDifference);
    float v15 = fheightDifference / fscrollDelta;
    float v14 = std::ceil(v15) + 1.0;
    int32_t v8 = Tile::TextToTrait("_number_of_items");
    pscrollbar->Set(v8, v14, true);
    int32_t v9 = Tile::TextToTrait("_current_value");
    pscrollbar->Set(v9, 0.0f, false);
    int32_t v10 = Tile::TextToTrait("_SetInCode");
    pscrollbar->Poke(v10, 1.0);
}

void MapMenu::LoadNote(BGSNote* apNote) {
    switch (apNote->GetNoteType()) {
        case NT_SOUND:
        case NT_VOICE: {
            int32_t v12 = Tile::TextToTrait("_ItemType");
            pTiles[13]->Set(v12, 3);
            if (!pCurrentSound) {
                char Src[64];
                BSsprintf(Src, 64, "--:--:- %s", sRemaining.String());
                int32_t v13 = Tile::TextToTrait("_ItemStringInfo");
                pTiles[13]->Set(v13, Src, true);
                int32_t v14 = Tile::TextToTrait("_AudioFileCompletion");
                pTiles[13]->Set(v14, 0.0f, true);
            }
            break;
        }
        case NT_TEXT: {
            Tile* pnoteText = pTiles[13]->GetChildByName("MM_DataText");
            Tile* pscrollbar = pTiles[13]->GetChildByName("MM_TextScrollbar");
            int32_t v2 = Tile::TextToTrait("_ScrollDelta");
            float fscrollDelta = pscrollbar->GetFloat(v2);
            float fheightDifference = 0.0f;
            int32_t v3 = Tile::TextToTrait("_ItemType");
            pTiles[13]->Set(v3, 1);
            TESDescription* NoteText = apNote->GetNoteText();
            const char* v4 = NoteText->GetDescription(apNote, TNAM_ID);
            pnoteText->Set(4036, v4, true);
            TESDescription* v21 = apNote->GetNoteText();
            const char* v15 = v21->GetDescription(apNote, TNAM_ID);
            FontManager* Singleton = FontManager::Instance();
            float ftextHeight = Singleton->CalculateStringDimensions(v15, 1, 400.0f, 0).y;
            double Float = pnoteText->pParent->GetFloat(4016);
            fheightDifference = ftextHeight - Float;
            fheightDifference = BSMAX(0.0f, fheightDifference);
            float v20 = fheightDifference / fscrollDelta;
            float v19 = std::ceil(v20) + 1.0;
            int32_t v7 = Tile::TextToTrait("_number_of_items");
            pscrollbar->Set(v7, v19, true);
            int32_t v8 = Tile::TextToTrait("_current_value");
            pscrollbar->Set(v8, 0.0f, false);
            int32_t v9 = Tile::TextToTrait("_SetInCode");
            pscrollbar->Poke(v9, 1.0);
            break;
        }
        case NT_IMAGE: {
            const char* a1a = apNote->GetNoteImage()->GetTextureName();
            Tile* ChildByName = pTiles[13]->GetChildByName("MM_DataImage");
            ChildByName->Set(4044, a1a, true);
            int32_t v11 = Tile::TextToTrait("_ItemType");
            pTiles[13]->Set(v11, 2);
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
        for (int32_t inoteCt = 0; inoteCt < pPlayer->GetNumberofNotes(); ++inoteCt) {
            BGSNote* pcurrNote = pPlayer->GetNotebyIndex(inoteCt);
            Tile* pnewTile = lbNotesList.AddItem(pcurrNote, pcurrNote->GetFullName(), nullptr, nullptr);
            pnewTile->Set(4010, 24);
            if (pcurrNote->QHasBeenRead()) {
                pnewTile->Set(MapMenu::iUT_TextAlpha, 128.0f, true);
            }
            else {
                pnewTile->Set(MapMenu::iUT_TextAlpha, 255.0f, true);
            }
        }
        lbNotesList.SetEnabled(true);
        lbNotesList.Filter(&MapMenu::NotesFilterFunc);
        int32_t bShowingAllNotes = this->bShowingAllNotes;
        int32_t v1 = Tile::TextToTrait("_ShowingAllNotes");
        pTiles[12]->Set(v1, bShowingAllNotes);
        TESQuest* ActiveQuest = pPlayer->GetActiveQuest();
        pTiles[12]->Set(4015, static_cast<int32_t>(ActiveQuest != nullptr));
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
        Tile* pchallengeText = pTiles[13]->GetChildByName("MM_DataText");
        pchallengeText->Set(4036, "", true);
    }
    if (MapMenu::bNotesChallengeMode) {
        const char* v2 = sNotesToggle.String();
        MapMenu::pMe->pTiles[18]->Set(4036, v2, true);
        MapMenu::pMe->pChallengeType->Set(4003, 1);
        const char* v3 = g_challengeStringsArray[MapMenu::cChallengeTypeDisplayed]->String();
        pChallengeType->Set(4036, v3, true);
    }
    else {
        const char* v4 = sChallengeToggle.String();
        MapMenu::pMe->pTiles[18]->Set(4036, v4, true);
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
        if ((pItem) && (!pItem->GetStartsLocked() || pItem->GetUnlocked())) {
            Tile* pnewTile = lbChallengeList.AddItem(pItem, pItem->GetFullName(), nullptr, nullptr);
            pnewTile->Set(4010, 24);
            if (pItem->GetCompleted()) {
                pnewTile->Set(MapMenu::iUT_TextAlpha, 128.0f, true);
            }
            else {
                pnewTile->Set(MapMenu::iUT_TextAlpha, 255.0f, true);
            }
        }
    }
    lbChallengeList.Filter(&MapMenu::ChallengeFilterFunc);
    static_cast<ListBox<TESChallenge*>*>(&lbChallengeList)->Sort(&MapMenu::ChallengeSortFunc);
}

void MapMenu::UpdateQuests() {
    TESQuest* pcurrQuest_2;
    TESQuest* pcurrQuest;
    // RemoveAll, Sort and ScrollToHighlight are called through the lists' vtables; by-value member calls would be
    // devirtualised.
    static_cast<ListBox<TESQuest*>*>(&lbQuestsList)->RemoveAll();
    static_cast<ListBox<BGSQuestObjective*>*>(&lbQuestObjectivesList)->RemoveAll();
    for (BSSimpleList<BGSQuestObjective*>* pobjectiveList = pPlayer->GetQuestObjectiveList();
         pobjectiveList && !pobjectiveList->IsEmpty(); pobjectiveList = pobjectiveList->m_pkNext) {
        pcurrQuest = pobjectiveList->m_item->pOwnerQuest;
        if (pobjectiveList->m_item->GetDisplayed()) {
            const char* DisplayText = pobjectiveList->m_item->GetDisplayText();
            Tile* pnewTile = lbQuestObjectivesList.AddItem(pobjectiveList->m_item, DisplayText, nullptr, nullptr);
            pnewTile->xChildren.Get(pnewTile->xChildren.m_pkTail)->Set(4026, 320.0f, true);
            int32_t v2 = Tile::TextToTrait("_ShowEmptyMarker");
            pnewTile->Set(v2, 1);
            int32_t v3 = Tile::TextToTrait("_enabled");
            pnewTile->Set(v3, 0);
            if (pobjectiveList->m_item->GetCompleted()) {
                pnewTile->Set(MapMenu::iUT_TextAlpha, 128.0f, true);
                pnewTile->Set(MapMenu::iUT_Selected, 1);
            }
            else {
                if (pcurrQuest->GetCompleted()) {
                    pnewTile->Set(MapMenu::iUT_TextAlpha, 128.0f, true);
                }
                else {
                    pnewTile->Set(MapMenu::iUT_TextAlpha, 255.0f, true);
                }
            }
            if (!lbQuestsList.GetTileFor(pcurrQuest)) {
                const char* FullName = pcurrQuest->GetFullName();
                Tile* pnewTile_2 = lbQuestsList.AddItem(pcurrQuest, FullName, nullptr, nullptr);
                pnewTile_2->Set(4010, 23);
                if (pcurrQuest->GetCompleted()) {
                    pnewTile_2->Set(MapMenu::iUT_TextAlpha, 128.0f, true);
                }
                else {
                    pnewTile_2->Set(MapMenu::iUT_TextAlpha, 255.0f, true);
                }
                TESQuest* ActiveQuest = pPlayer->GetActiveQuest();
                if (pcurrQuest == ActiveQuest) {
                    pnewTile_2->Set(MapMenu::iUT_Selected, 1);
                }
            }
        }
    }
    static_cast<ListBox<BGSQuestObjective*>*>(&lbQuestObjectivesList)->Sort(&MapMenu::QuestObjectiveSortFunc);
    static_cast<ListBox<TESQuest*>*>(&lbQuestsList)->Sort(&MapMenu::QuestSortFunc);
    lbQuestsList.SetEnabled(true);
    if (pPlayer->GetActiveQuest()) {
        pcurrQuest_2 = pPlayer->GetActiveQuest();
        Tile* pactiveQuestTile = lbQuestsList.GetTileFor(pcurrQuest_2);
        DoEnter(23, pactiveQuestTile);
        if (InterfaceManager::Has360Controller()) {
            InterfaceManager* v5 = InterfaceManager::Instance();
            v5->SetCurrentFocusTarget(pactiveQuestTile, 4035, true);
            static_cast<ListBoxBase*>(&lbQuestsList)->ScrollToHighlight();
        }
    }
    else {
        lbQuestObjectivesList.Filter(&MapMenu::IsQuestMapMenuSelectedQuest);
    }
}

void MapMenu::UpdateRadioList() {
    BSSimpleList<TESObjectREFR*> inRangeStationList;
    BGSTalkingActivator::GetRadioStationsInRangeOfRef(pPlayer, &inRangeStationList, nullptr);
    // RemoveAll and Sort are called through a pointer to the list so that they dispatch virtually; by-value member
    // calls would be devirtualised.
    static_cast<ListBox<TESObjectREFR*>*>(&lbRadioStationsList)->RemoveAll();
    for (BSSimpleList<TESObjectREFR*>* pcurrStation = FalloutRadio::QDiscoveredStationsList();
         pcurrStation && pcurrStation->m_item; pcurrStation = pcurrStation->m_pkNext) {
        const char* FullName = pcurrStation->m_item->GetFullName();
        Tile* pstationTile = lbRadioStationsList.AddItem(pcurrStation->m_item, FullName, nullptr, nullptr);
        pstationTile->Set(4010, 25);
        const char* v2 = pcurrStation->m_item->GetFullName();
        pstationTile->SetName(v2);
        int32_t a3 = false;
        if (FalloutRadio::GetCurrentStation()) {
            BSSimpleList<TESObjectREFR*>* v3 = pcurrStation;
            if (v3->m_item == FalloutRadio::GetCurrentStation()->pStation) {
                a3 = true;
            }
        }
        pstationTile->Set(MapMenu::iUT_Selected, a3);
        if (inRangeStationList.IsInList(pcurrStation->m_item)) {
            pstationTile->Set(MapMenu::iUT_TextAlpha, 255.0f, true);
        }
        else {
            pstationTile->Set(MapMenu::iUT_TextAlpha, 128.0f, true);
        }
    }
    lbRadioStationsList.Filter(&MapMenu::RadioFilterFunc);
    static_cast<ListBox<TESObjectREFR*>*>(&lbRadioStationsList)->Sort(&MapMenu::RadioSortFunc);
    inRangeStationList.RemoveAll();
}

NiPoint3 MapMenu::MapToWorldCoord(NiPoint3 aMapCoord, TESWorldSpace* apWorldspace) {
    NiPoint3 ecx0;
    float x = aMapCoord.x;
    float y = aMapCoord.y;
    if (iCurrentPage == 33) {
        x = aMapCoord.x * 1.2549020051956177 - 0.12745098769664764;
        y = aMapCoord.y * 1.2549020051956177 - 0.12745098769664764;
    }
    ecx0.x = (MapSECorner.x - MapNWCorner.x) * x + MapNWCorner.x;
    ecx0.y = (MapSECorner.y - MapNWCorner.y) * y + MapNWCorner.y;
    ecx0.z = 0.0;
    if ((apWorldspace) && (iCurrentPage == 33)) {
        apWorldspace->AdjustMapMarkerCoord(ecx0, true);
    }

    return ecx0;
}

void MapMenu::UpdateWorldMap() {
    WorldSpaceMapData* prootWorldData;
    NiPoint2 v31;
    NiPoint2 v32;
    if (pRootMapSpace) {
        prootWorldData = pRootMapSpace->GetWorldMapData();
    }
    else {
        prootWorldData = nullptr;
    }
    WorldSpaceMapData* prootWorldData_2 = prootWorldData;
    if (!pRootMapSpace ||
        (!prootWorldData_2 || (!prootWorldData_2->iUsableWidth || !prootWorldData_2->iUsableHeight))) {
        WorldSpaceMapData* prootWorldData_3;
        TESForm* FormByNumericID = TESForm::GetFormByNumericID(60u);
        pRootMapSpace = dynamic_cast<TESWorldSpace*>(FormByNumericID);
        if (pRootMapSpace) {
            prootWorldData_3 = pRootMapSpace->GetWorldMapData();
        }
        else {
            prootWorldData_3 = nullptr;
        }
        prootWorldData_2 = prootWorldData_3;
        bRootSpaceChanged = true;
    }
    if (!pRootMapSpace ||
        (!prootWorldData_2 || (!prootWorldData_2->iUsableWidth || !prootWorldData_2->iUsableHeight))) {
        WorldSpaceMapData* prootWorldData_4;
        pRootMapSpace = static_cast<TESWorldSpace*>(pDataHandler->GetWorldSpaceList()->m_item);
        if (pRootMapSpace) {
            prootWorldData_4 = pRootMapSpace->GetWorldMapData();
        }
        else {
            prootWorldData_4 = nullptr;
        }
        prootWorldData_2 = prootWorldData_4;
        bRootSpaceChanged = true;
    }
    pMapSpace = (TESForm*)pRootMapSpace;
    pAdjustNorthRotationCell = nullptr;
    if ((pRootMapSpace) && (prootWorldData_2)) {
        if (MapMenu::bWorldMapTextureCleared || bRootSpaceChanged) {
            const char* WorldMapTextureName = pRootMapSpace->GetWorldMapTextureName();
            pTiles[4]->Set(4044, WorldMapTextureName, true);
            pTiles[4]->Set(4045, prootWorldData_2->iUsableWidth);
            pTiles[4]->Set(4046, prootWorldData_2->iUsableHeight);
            MapMenu::bWorldMapTextureCleared = false;
            bRootSpaceChanged = false;
        }
        ZoomMap(true, 1.0f);
        NiPoint2* v3 = ::new (&v32) NiPoint2(
            static_cast<float>(prootWorldData_2->sNorthWestCellX << 12),
            static_cast<float>((prootWorldData_2->sNorthWestCellY << 12) + 4096)
        );
        float y = v3->y;
        MapNWCorner.x = v3->x;
        MapNWCorner.y = y;
        NiPoint2* v5 = ::new (&v31) NiPoint2(
            static_cast<float>((prootWorldData_2->sSouthEastCellX << 12) + 4096),
            static_cast<float>(prootWorldData_2->sSouthEastCellY << 12)
        );
        float x = v5->x;
        float v7 = v5->y;
        MapSECorner.x = x;
        MapSECorner.y = v7;
        if (!pTiles[4]->GetChildByID(26)) {
            BSSimpleList<TESObjectREFR*>* pfullMarkerList = pRootMapSpace->BuildMapMarkerList(false);
            mapMarkerIndex = 0;
            for (BSSimpleList<TESObjectREFR*>* pcurrMarkerObj = pfullMarkerList;
                 pcurrMarkerObj && pcurrMarkerObj->m_item; pcurrMarkerObj = pcurrMarkerObj->m_pkNext) {
                TESWorldSpace* pmarkerRootWorldspace;
                TESObjectREFR* pcurrMarker = pcurrMarkerObj->m_item;
                NiPoint3 markerPos = pcurrMarker->GetLocationOnReference();
                for (pmarkerRootWorldspace = pcurrMarker->GetWorldSpace();
                     pmarkerRootWorldspace && pmarkerRootWorldspace->GetParentWorldSimple();
                     pmarkerRootWorldspace = pmarkerRootWorldspace->GetParentWorldSimple()) {
                    ;
                }
                MapMarkerData* MapMarkerData_ = pcurrMarker->GetMapMarkerData();
                if (MapMarkerData_->GetVisible() && !pcurrMarker->GetDisabled() &&
                    pmarkerRootWorldspace == pRootMapSpace) {
                    Tile* pmarkerTile = MapMenu::pMe->RenderTemplate(pTiles[4], "MapMarkerTemplate", nullptr);
                    if (pmarkerTile) {
                        TESWorldSpace* pmarkerRootWorldspace_2 = pcurrMarker->GetWorldSpace();
                        NiPoint3 markerTilePos = WorldToMapCoord(markerPos, pmarkerRootWorldspace_2);
                        TESReputation* pRep = pcurrMarker->GetMapMarkerData()->pReputation;
                        if (pRep) {
                            char Src[256];
                            const char* a3b = pRep->GetReputationTitle();
                            MapMarkerData* v10 = pcurrMarker->GetMapMarkerData();
                            BSsprintf(Src, 256, "%s\n(%s)", v10->GetLocationName(), a3b);
                            int32_t v11 = Tile::AddUserTrait("_LocationName", -1);
                            pmarkerTile->Set(v11, Src, true);
                        }
                        else {
                            MapMarkerData* v12 = pcurrMarker->GetMapMarkerData();
                            const char* afValueb = v12->GetLocationName();
                            int32_t v13 = Tile::AddUserTrait("_LocationName", -1);
                            pmarkerTile->Set(v13, afValueb, true);
                        }
                        MapMarkerData* v14 = pcurrMarker->GetMapMarkerData();
                        const char* LocationName = v14->GetLocationName();
                        pmarkerTile->SetName(LocationName);
                        pmarkerTile->Set(4010, 26);
                        pmarkerTile->Set(MapMenu::iUT_X, markerTilePos.x, true);
                        pmarkerTile->Set(MapMenu::iUT_Y, markerTilePos.y, true);
                        MapMarkerData* v16 = pcurrMarker->GetMapMarkerData();
                        if (v16->GetTravelLoc()) {
                            MapMarkerData* v17 = pcurrMarker->GetMapMarkerData();
                            int32_t Type = v17->GetType();
                            pmarkerTile->Set(4044, pcMarkerIconFilenamesA[Type - 1], true);
                        }
                        else {
                            pmarkerTile->Set(4044, "Interface\\Icons\\World Map\\icon_map_undiscovered.dds", true);
                            pmarkerTile->Set(4100, 0);
                        }
                        int32_t a3c = mapMarkerIndex;
                        int32_t v19 = Tile::AddUserTrait("_MarkerIndex", -1);
                        pmarkerTile->Set(v19, a3c);
                        WorldMapMarkerList.AddTail(pcurrMarker);
                        ++mapMarkerIndex;
                    }
                }
            }
            pfullMarkerList->RemoveAll();
            if (pfullMarkerList) {
                delete pfullMarkerList;
            }
        }
        AddMapIcons();
    }
}

void MapMenu::Zoom_RefreshCompanions() {
    BSSimpleList<Actor*>* pcurrTeammate = pPlayer->GetTeammateList();
    while (pcurrTeammate) {
        Tile* ChildByName;
        TESObjectREFR* WorldMapTrackingRef;
        Actor* a1 = pcurrTeammate->m_item;
        TeleportPath apMarkerTarget;
        if (iCurrentPage == 33) {
            pPlayer->BuildPathToTarget(a1, &apMarkerTarget, true);
        }
        if (iCurrentPage == 33) {
            WorldMapTrackingRef = GetWorldMapTrackingRef(a1, &apMarkerTarget);
        }
        else {
            WorldMapTrackingRef = a1;
        }
        TESObjectREFR* v25 = WorldMapTrackingRef;
        if (WorldMapTrackingRef) {
            const char* FullName = v25->GetFullName();
            ChildByName = pMapTile->GetChildByName(FullName);
        }
        else {
            ChildByName = nullptr;
        }
        Tile* v28 = ChildByName;
        if ((ChildByName) && (v28->GetString(4036))) {
            const char* afActionVal = v28->GetString(4036);
            const char* v2 = a1->GetFullName();
            if (!BSstricmp(v2, afActionVal)) {
                double v3;
                Tile* v8;
                const char* v9;
                char apName[68];
                if (v25) {
                    v9 = v25->GetFullName();
                }
                else {
                    v9 = nullptr;
                }
                std::sprintf(apName, "%s Marker", v9);
                if (v25) {
                    v8 = pMapTile->GetChildByName(apName);
                }
                else {
                    v8 = nullptr;
                }
                Tile* apSourceTile = v8;
                NiPoint3 a2 = v25->GetLocationOnReference();
                NiPoint3 v17 = a2;
                if (iCurrentPage == 33) {
                    v3 = v17.y - 1024.0;
                }
                else {
                    v3 = v17.y - 128.0;
                }
                v17.y = v3;
                if (pAdjustNorthRotationCell) {
                    pAdjustNorthRotationCell->AdjustCoordForNorthRotation(a2, a2, true);
                }
                TESWorldSpace* WorldSpace = v25->GetWorldSpace();
                NiPlane v23;
                v23.m_kNormal = WorldToMapCoord(a2, WorldSpace);
                TESWorldSpace* v5 = v25->GetWorldSpace();
                NiPlane afValue;
                afValue.m_kNormal = WorldToMapCoord(v17, v5);
                float Float = pMapTile->GetFloat(4017);
                afValue.m_fConstant = pMapTile->GetFloat(4016);
                Float = Float * afValue.m_kNormal.x;
                afValue.m_fConstant = afValue.m_fConstant * afValue.m_kNormal.y;
                v23.m_fConstant = Float;
                float m_fConstant = afValue.m_fConstant;
                v28->Set(MapMenu::iUT_X, afValue.m_kNormal.x, true);
                v28->Set(MapMenu::iUT_Y, afValue.m_kNormal.y, true);
                v28->Set(4001, v23.m_fConstant, true);
                v28->Set(4002, m_fConstant, true);
                const char* v6 = a1->GetFullName();
                v28->Set(4036, v6, true);
                v28->Set(4026, 200.0f, true);
                v28->Set(4010, 38);
                v28->Set(4013, 2.0f, true);
                v28->Set(4003, 1);
                v28->Set(4014, 1);
                v28->Set(4023, 2);
                v28->Set(4025, 2);
                v28->Set(4016, 60.0f, true);
                if (apSourceTile) {
                    Tile::Value* ValueQ = v28->GetValueQ(4001);
                    Tile::Value* v15 = v28->GetValueQ(4002);
                    ValueQ->ClearActions();
                    ValueQ->AddAction(2000, apSourceTile, 4001);
                    ValueQ->CalculateValue(false);
                    v15->ClearActions();
                    v15->AddAction(2000, apSourceTile, 4002);
                    v15->AddAction(2001, 60.0f);
                    v15->CalculateValue(false);
                }
                v28->AddNeedsUpdate(1023);
            }
        }

        pcurrTeammate = pcurrTeammate->m_pkNext;
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
    uint8_t v2 = 0;
    if (MapMenu::pMe) {
        const int32_t& v0 = uGridsToLoad.UInt();
        if (MapMenu::pMe->uiNextLocalMapGrid < (uGridsToLoad.UInt() * v0)) {
            return true;
        }
    }
    return (bool)v2;
}

void MapMenu::RenderLocalMapGrid(bool abSaveToDisk) {
    NiPoint2 v73;
    NiPoint3 v74;
    NiPoint3 v75;
    NiPoint3 v88;
    NiPoint3 dst;
    if ((MapMenu::pMe) && (MapMenu::pMe->pTiles[2]->GetModel()) && (MapMenu::ShouldRenderLocalMap())) {
        NiPoint3 playerLoc;
        const NiPoint3* v1 = &pPlayer->GetLocationOnReference();
        playerLoc.x = v1->x;
        playerLoc.y = v1->y;
        playerLoc.z = v1->z + 10.0;
        bool bisInInterior = pPlayer->GetInterior();
        int32_t igridSize = bisInInterior ? 2048 : 1024;
        int32_t inumSubgridsPerSide = 16;
        int32_t inumSubgrids = 256;
        int32_t isubgridSize = igridSize / 16;
        int32_t ivertsPerSide = 17;
        uint32_t inumVerts = 289;
        int32_t itilesPerSide = uGridsToLoad.Int();
        int32_t inumTiles = itilesPerSide * itilesPerSide;
        int32_t imapSize = itilesPerSide * igridSize;
        float fpossize = SeenData::GetSeenPosSize();
        NiColorA oldSunColor = WaterShader::pSunColor;
        WaterShader::pSunColor = NiColorA_BLACK;
        bool boldReflection = WaterShader::bNoReflections;
        WaterShader::bNoReflections = true;
        NiColorA tintColor(0.0f, 0.0f, 0.0f, 0.0f);
        SystemColorManager* Instance = SystemColorManager::GetInstance();
        Instance->GetColor(4, tintColor);
        bool bwaterCulled = true;
        if (pPlayer->GetWorldSpace()) {
            TESWorldSpace* WorldSpace = pPlayer->GetWorldSpace();
            if (WorldSpace->GetTerrainManager()->GetWaterLODRoot()) {
                TESWorldSpace* v4 = pPlayer->GetWorldSpace();
                BGSTerrainManager* TerrainManager = v4->GetTerrainManager();
                bwaterCulled = TerrainManager->GetWaterLODRoot()->GetAppCulled();
                TESWorldSpace* v6 = pPlayer->GetWorldSpace();
                BGSTerrainManager* v7 = v6->GetTerrainManager();
                v7->GetWaterLODRoot()->SetAppCulled(true);
            }
        }
        NiNode* plandlod_2 = Main::LandLODNode();
        bool blandCulled = plandlod_2->GetAppCulled();
        if (!blandCulled) {
            plandlod_2->SetAppCulled(true);
        }
        int32_t* v8 = &uGridsToLoad.Int();
        int32_t isectx = MapMenu::pMe->uiNextLocalMapGrid % *v8;
        int32_t* v9 = &uGridsToLoad.Int();
        int32_t isecty = MapMenu::pMe->uiNextLocalMapGrid / *v9;
        NiPointer<NiTexture> spmaptex(static_cast<NiTexture*>(nullptr));
        TESObjectCELL* pseencell = nullptr;
        NiPoint3 sectoffset = NiPoint3_ZERO;
        if (pPlayer->GetInterior()) {
            pseencell = pPlayer->GetParentCell();
            if (pseencell) {
                NiPoint3 playerIntLoc = playerLoc;
                pseencell->AdjustCoordForNorthRotation(playerLoc, playerIntLoc, true);
                int32_t iseenx = (FloatToInt(playerIntLoc.x) - 2048) >> 12;
                int32_t iseeny = (FloatToInt(playerIntLoc.y) - 2048) >> 12;
                int32_t iintsectionx = isectx + iseenx - (itilesPerSide >> 1);
                int32_t iintsectiony = isecty + iseeny - (itilesPerSide >> 1);
                sectoffset = *::new (&dst) NiPoint3(
                    static_cast<float>((iintsectionx << 12) + 2048), static_cast<float>((iintsectiony << 12) + 2048),
                    0.0f
                );
                pseencell->GetIntSeenSection(iintsectionx, iintsectiony, false);
                NiDX9Renderer* Renderer_ = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
                Renderer_->LockRenderer();
                pseencell->GetInteriorLocalMapTexture(iintsectionx, iintsectiony, spmaptex);
                NiDX9Renderer* v11 = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
                v11->UnlockRenderer();
                if (!isectx && !isecty) {
                    pseencell->BuildLocalMapDoorList(&MapMenu::pMe->LocalMapMarkerList);
                }
            }
        }
        else {
            TESWorldSpace* pworld = pPlayer->GetWorldSpace();
            int32_t icellx = FloatToInt(playerLoc.x) >> 12;
            int32_t icelly = FloatToInt(playerLoc.y) >> 12;
            int32_t icellsectionx = isectx + icellx - (itilesPerSide >> 1);
            int32_t icellsectiony = isecty + icelly - (itilesPerSide >> 1);
            sectoffset =
                *::new (&v88)
                    NiPoint3(static_cast<float>(icellsectionx << 12), static_cast<float>(icellsectiony << 12), 0.0f);
            if (pworld) {
                pseencell = pworld->GetCellFromCellCoord(icellsectionx, icellsectiony);
            }
            if (pseencell) {
                pseencell->GetSeenData();
                NiDX9Renderer* v12 = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
                v12->LockRenderer();
                pseencell->GetExteriorLocalMapTexture(spmaptex);
                NiDX9Renderer* v13 = static_cast<NiDX9Renderer*>(NiRenderer::GetRenderer());
                v13->UnlockRenderer();
                pseencell->BuildLocalMapDoorList(&MapMenu::pMe->LocalMapMarkerList);
            }
        }
        if (abSaveToDisk) {
            char apDest[268];
            char PathName[264];
            std::error_code kError;
            std::filesystem::create_directory(platform::NativePath("(LocalMaps)"), kError);
            std::filesystem::create_directory(platform::NativePath("(LocalMaps)\\Interiors"), kError);
            std::filesystem::create_directory(platform::NativePath("(LocalMaps)\\Exteriors"), kError);
            if (TES::pInstance->pInteriorCell) {
                TESObjectCELL* Interior = TES::pInstance->pInteriorCell;
                BSsprintf(PathName, 260, "(LocalMaps)\\Interiors\\%s", Interior->GetFormEditorID());
                std::filesystem::create_directory(platform::NativePath(PathName), kError);
                TESObjectCELL* v67 = TES::pInstance->pInteriorCell;
                TESObjectCELL* v66 = TES::pInstance->pInteriorCell;
                const char* xb = v67->GetFormEditorID();
                BSsprintf(
                    apDest, 260, "(LocalMaps)\\Interiors\\%s\\%s.%i.%i.png", v66->GetFormEditorID(), xb, isectx, isecty
                );
            }
            else {
                TESWorldSpace* v65 = TES::pInstance->pWorldSpace;
                BSsprintf(PathName, 260, "(LocalMaps)\\Exteriors\\%s", v65->GetFormEditorID());
                std::filesystem::create_directory(platform::NativePath(PathName), kError);
                TESWorldSpace* v64 = TES::pInstance->pWorldSpace;
                TESWorldSpace* v63 = TES::pInstance->pWorldSpace;
                int32_t v14 = FloatToInt(playerLoc.y);
                int32_t za = isecty + (v14 >> 12);
                int32_t v15 = FloatToInt(playerLoc.x);
                int32_t yc = isectx + (v15 >> 12);
                const char* xc = v64->GetFormEditorID();
                BSsprintf(apDest, 260, "(LocalMaps)\\Exteriors\\%s\\%s.%i.%i.png", v63->GetFormEditorID(), xc, yc, za);
            }
            // Casts the renderer data to NiObject's RTTI and calls GetD3DTexture.
            NiObject* v95 = NiObject::DynamicCastStatic(&NiObject::ms_RTTI, spmaptex.m_pObject->m_pkRendererData);
            IDirect3DBaseTexture9* v16 = static_cast<NiDX9TextureData*>(v95)->GetD3DTexture();
            // 3 is D3DXIFF_PNG.
            D3DXSaveTextureToFileA_Thunk(apDest, 3u, v16, nullptr);
            ++MapMenu::pMe->uiNextLocalMapGrid;
        }
        else {
            NiAlphaProperty* palpha;
            TileShaderProperty* ptsp;
            BSScissorTriShape* pnewGrid;
            NiColorA* pcolor;
            NiPoint2* ptex;
            NiPoint3* pnorm;
            NiPoint3* pverts;
            int32_t x_2;
            int32_t y_2;
            int32_t inumVerts_2 = inumVerts;
            void* ptr = ourMalloc(inumVerts <= SIZE_MAX / sizeof(NiPoint3) ? sizeof(NiPoint3) * inumVerts : SIZE_MAX);
            if (ptr) {
                rt::eh_vector_constructor_iterator(ptr, sizeof(NiPoint3), inumVerts_2, &emptyPointConstructor);
                pverts = static_cast<NiPoint3*>(ptr);
            }
            else {
                pverts = nullptr;
            }
            int32_t inumVerts_3 = inumVerts;
            void* v82 = ourMalloc(inumVerts <= SIZE_MAX / sizeof(NiPoint3) ? sizeof(NiPoint3) * inumVerts : SIZE_MAX);
            if (v82) {
                rt::eh_vector_constructor_iterator(v82, sizeof(NiPoint3), inumVerts_3, &emptyPointConstructor);
                pnorm = static_cast<NiPoint3*>(v82);
            }
            else {
                pnorm = nullptr;
            }
            int32_t inumVerts_4 = inumVerts;
            void* v79 = ourMalloc(inumVerts <= SIZE_MAX / sizeof(NiPoint2) ? sizeof(NiPoint2) * inumVerts : SIZE_MAX);
            if (v79) {
                rt::eh_vector_constructor_iterator(v79, sizeof(NiPoint2), inumVerts_4, &emptyPointConstructor);
                ptex = static_cast<NiPoint2*>(v79);
            }
            else {
                ptex = nullptr;
            }
            int32_t inumVerts_5 = inumVerts;
            void* v76 = ourMalloc(inumVerts <= SIZE_MAX / sizeof(NiColorA) ? sizeof(NiColorA) * inumVerts : SIZE_MAX);
            if (v76) {
                pcolor = ::new (v76) NiColorA[inumVerts_5];
            }
            else {
                pcolor = nullptr;
            }
            uint16_t* ptrilist = static_cast<uint16_t*>(_NiMalloc(6 * sizeof(uint16_t) * inumSubgrids));
            int32_t index = 0;
            for (y_2 = 0; y_2 < ivertsPerSide; ++y_2) {
                for (x_2 = 0; x_2 < ivertsPerSide; ++x_2) {
                    float v58 = static_cast<double>(isubgridSize * y_2) - static_cast<double>(igridSize) / 2.0;
                    float v57 = static_cast<double>(isubgridSize * x_2) - static_cast<double>(igridSize) / 2.0;
                    NiPoint3* v17 = ::new (&v75) NiPoint3(v57, 0.0f, v58);
                    pverts[index++] = *v17;
                }
            }
            index = 0;
            for (y_2 = 0; y_2 < ivertsPerSide; ++y_2) {
                for (x_2 = 0; x_2 < ivertsPerSide; ++x_2) {
                    NiPoint3* v18 = ::new (&v74) NiPoint3(0.0f, 1.0f, 0.0f);
                    pnorm[index] = *v18;
                    float v56 = 1.0 - static_cast<double>(y_2) / static_cast<double>(ivertsPerSide);
                    float v55 = static_cast<double>(x_2) / static_cast<double>(ivertsPerSide);
                    NiPoint2* v19 = ::new (&v73) NiPoint2(v55, v56);
                    float v20 = v19->x;
                    float v21 = v19->y;
                    int32_t v22 = index;
                    ptex[index].x = v20;
                    ptex[v22].y = v21;
                    float v54 = static_cast<double>(y_2) * fpossize;
                    float v53 = static_cast<double>(x_2) * fpossize;
                    NiPoint3 worldcoord(v53, v54, 0.0f);
                    worldcoord += sectoffset;
                    float fseenval = 4.0f;
                    if (MapMenu::pMe->bFogOfWarEnabled) {
                        fseenval = static_cast<float>(TESObjectCELL::GetSeenValue(worldcoord));
                    }
                    float v52 = fseenval / 4.0;
                    NiColorA kSeenColor(1.0f, 1.0f, 1.0f, v52);
                    pcolor[index] = kSeenColor;
                    ++index;
                }
            }
            index = 0;
            for (y_2 = 0; y_2 < inumSubgridsPerSide; ++y_2) {
                for (x_2 = 0; x_2 < inumSubgridsPerSide; ++x_2) {
                    uint16_t v26;
                    if (x_2 % 2 != y_2 % 2) {
                        ptrilist[index++] = x_2 + ivertsPerSide * (y_2 + 1);
                        ptrilist[index++] = x_2 + ivertsPerSide * y_2;
                        ptrilist[index++] = ivertsPerSide * y_2 + x_2 + 1;
                        ptrilist[index++] = ivertsPerSide * y_2 + x_2 + 1;
                        ptrilist[index++] = ivertsPerSide * (y_2 + 1) + x_2 + 1;
                        uint16_t ival = x_2 + ivertsPerSide * (y_2 + 1);
                        v26 = ival;
                    }
                    else {
                        ptrilist[index++] = ivertsPerSide * (y_2 + 1) + x_2 + 1;
                        ptrilist[index++] = x_2 + ivertsPerSide * (y_2 + 1);
                        ptrilist[index++] = x_2 + ivertsPerSide * y_2;
                        ptrilist[index++] = x_2 + ivertsPerSide * y_2;
                        ptrilist[index++] = ivertsPerSide * y_2 + x_2 + 1;
                        uint16_t ival_2 = ivertsPerSide * (y_2 + 1) + x_2 + 1;
                        v26 = ival_2;
                    }
                    ptrilist[index++] = v26;
                }
            }
            BSScissorTriShape* apMemory =
                static_cast<BSScissorTriShape*>(NiMemObject::operator new(sizeof(BSScissorTriShape)));
            if (apMemory) {
                int32_t zb = static_cast<int32_t>(InterfaceManager::GetScreenHeight());
                double ScreenWidth = InterfaceManager::GetScreenWidth();
                pnewGrid = ::new (apMemory) BSScissorTriShape(
                    inumVerts, pverts, pnorm, pcolor, ptex, 1, NiGeometryData::NBT_METHOD_NONE, 2 * inumSubgrids,
                    ptrilist, 0, 0, static_cast<int32_t>(ScreenWidth), zb
                );
            }
            else {
                pnewGrid = nullptr;
            }
            ptsp = new TileShaderProperty(false);
            NiTexture* zc = spmaptex.m_pObject;
            ptsp->SetTileTexture(zc);
            ptsp->TexClampMode = NiTexturingProperty::CLAMP_S_CLAMP_T;
            ptsp->SetTintColor(tintColor);
            ptsp->SetUseVertexColors(true);
            spmaptex = static_cast<NiTexture*>(nullptr);
            pnewGrid->AttachProperty(ptsp);
            palpha = new NiAlphaProperty;
            palpha->SetSrcBlendMode(NiAlphaProperty::ALPHA_SRCALPHA);
            palpha->SetDestBlendMode(NiAlphaProperty::ALPHA_ONE);
            palpha->SetAlphaBlending(true);
            pnewGrid->AttachProperty(palpha);
            BSShaderManager::PrepareObject(pnewGrid, 0, 0);
            float v48 = static_cast<double>(isecty * igridSize) + static_cast<double>(igridSize) / 2.0 -
                static_cast<double>(imapSize);
            float v47 = static_cast<double>(isectx * igridSize) + static_cast<double>(igridSize) / 2.0;
            pnewGrid->SetTranslate(v47, 0.0f, v48);
            pnewGrid->UpdateProperties();
            NiNode* Model_ = MapMenu::pMe->pTiles[2]->GetModel();
            Model_->AttachChild(pnewGrid, 1);
            MapMenu::pMe->pTiles[2]->ClearNeedsUpdate(Tile::CHANGE_TEXTURE);
            WaterShader::pSunColor = oldSunColor;
            WaterShader::bNoReflections = boldReflection;
            if ((!bwaterCulled) && (pPlayer->GetWorldSpace())) {
                TESWorldSpace* v28 = pPlayer->GetWorldSpace();
                BGSTerrainManager* v29 = v28->GetTerrainManager();
                v29->GetWaterLODRoot()->SetAppCulled(bwaterCulled);
            }

            if (!blandCulled) {
                plandlod_2->SetAppCulled(false);
            }
            MapMenu::pMe->pTiles[2]->AddNeedsUpdate(Tile::CHANGE_SCISSOR | Tile::CHANGE_COLOR);
            MapMenu::pMe->pTiles[2]->Set(Tile::kTileValue_FileWidth, itilesPerSide * igridSize);
            MapMenu::pMe->pTiles[2]->Set(Tile::kTileValue_FileHeight, itilesPerSide * igridSize);
            float v45 = tintColor.r * 255.0;
            MapMenu::pMe->pTiles[2]->Set(Tile::kTileValue_Red, v45, true);
            float v44 = tintColor.g * 255.0;
            MapMenu::pMe->pTiles[2]->Set(Tile::kTileValue_Green, v44, true);
            float v43 = tintColor.b * 255.0;
            MapMenu::pMe->pTiles[2]->Set(Tile::kTileValue_Blue, v43, true);
            NiUpdateData UpdateData(0.0f, false, false);
            MapMenu::pMe->pTiles[2]->GetModel()->Update(UpdateData);
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
    TileShaderProperty* pProperty =
        pChild ? static_cast<TileShaderProperty*>(pChild->GetProperty(NiShadeProperty::GetType())) : nullptr;
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
    TileShaderProperty* pProperty =
        pChild ? static_cast<TileShaderProperty*>(pChild->GetProperty(NiShadeProperty::GetType())) : nullptr;
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
        MapMenu::pMe->pTiles[TI_TABLINE], TI_TAB_LOCALMAP, pLocalMapText, pWorldMapText, pQuestsText, pMiscText,
        pCommsText, static_cast<const char*>(nullptr)
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
    TileShaderProperty* pProperty =
        pChild ? static_cast<TileShaderProperty*>(pChild->GetProperty(NiShadeProperty::GetType())) : nullptr;
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

// The strings are destroyed with Set(nullptr, 0). Each label's setting text and trait are fetched before pMe is read,
// as the game does.
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

// pMe is reread at each step, as the game does. bRootSpaceChanged records whether the root worldspace differs from the
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
    InterfaceManager* singleton;
    FOPipboyManager* pipboy;
    uint32_t v3;
    if (MapMenu::pMe) {
        v3 = (uint32_t)(MapMenu::pMe->iCurrentPage - 32);
        singleton = InterfaceManager::Instance();
        pipboy = singleton->GetPipboy();
        pipboy->SetTabKnobPosition(v3, abPlaySounds);
    }
}

void MapMenu::UnselectFunc(Tile* apTile, TESObjectREFR* apStation) {
    apTile->Set(MapMenu::iUT_Selected, 0);
}

bool MapMenu::RadioFilterFunc(TESObjectREFR* pcurrStation) {
    return !pcurrStation || pcurrStation->GetDisabled() ||
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
    if ((MapMenu::pMe) && (pPlayer->QPlayerMapMarker())) {
        TESObjectREFR* PlayerMapMarker = pPlayer->QPlayerMapMarker();
        MapMenu::pMe->PlayerMarkerPos = PlayerMapMarker->GetLocationOnReference();
        TESObjectREFR* v0 = pPlayer->QPlayerMapMarker();
        MapMenu::pMe->PlayerMarkerSpace = v0->GetWorldSpace();
        if (!MapMenu::pMe->PlayerMarkerSpace) {
            TESObjectREFR* v1 = pPlayer->QPlayerMapMarker();
            MapMenu::pMe->PlayerMarkerSpace = v1->GetParentCell();
        }
    }
}

int32_t MapMenu::RadioSortFunc(ListBoxItem<TESObjectREFR*>* apItem1, ListBoxItem<TESObjectREFR*>* apItem2) {
    if ((apItem1->pTile->GetFloat(MapMenu::iUT_TextAlpha) == 255.0) &&
        (apItem1->pTile->GetFloat(MapMenu::iUT_TextAlpha) != 255.0)) {
        return (-1);
    }
    if ((apItem1->pTile->GetFloat(MapMenu::iUT_TextAlpha) != 255.0) &&
        (apItem1->pTile->GetFloat(MapMenu::iUT_TextAlpha) == 255.0)) {
        return 1;
    }
    const char* fullName = apItem2->data->GetFullName();
    const char* v2 = apItem1->data->GetFullName();
    return BSstrcmp(v2, fullName);
}

bool MapMenu::NotesFilterFunc(BGSNote* pcurrNote) {
    BSSimpleList<TESQuest*>* v1;
    BSSimpleList<TESQuest*>* ecx0;
    int8_t v4 = 1;
    if ((!MapMenu::pMe) || MapMenu::pMe->bShowingAllNotes) {
        if ((MapMenu::pMe) && (MapMenu::pMe->bShowingAllNotes)) {
            return false;
        }
    }
    else {
        for (ecx0 = pcurrNote->GetQuestList(); ecx0 && ecx0->m_item; ecx0 = ecx0->m_pkNext) {
            v1 = ecx0;
            if (v1->m_item == pPlayer->GetActiveQuest()) {
                v4 = 0;
            }
        }
    }
    return (bool)v4;
}

void MapMenu::ResetCurrentNoteIfEqualTo(BGSNote* a1) {
    if ((MapMenu::pMe) && (MapMenu::pMe->pSelectedNote == a1)) {
        MapMenu::pMe->pSelectedNote = nullptr;
    }
}

void MapMenu::FastTravelCallback() {
    if (Interface::GetMessageMenuResult()) {
        MapMenu::pMe->pFastTravelMarker = nullptr;
    }
    else {
        InterfaceManager* v0 = InterfaceManager::Instance();
        v0->ClosePipboy(&MapMenu_RequestPlayerPositionAsSelectedMarker);
    }
    if (!InterfaceManager::Has360Controller()) {
        InterfaceManager* v1 = InterfaceManager::Instance();
        double PipboyMouseX = v1->GetPipboyMouseX();
        double v7 = PipboyMouseX - MapMenu::pMe->pTiles[6]->pParent->GetRealX();
        float afValue = v7 - MapMenu::pMe->pTiles[6]->GetFloat(4017) / 2.0;
        MapMenu::pMe->pTiles[6]->Set(4001, afValue, true);
        InterfaceManager* v2 = InterfaceManager::Instance();
        double PipboyMouseY = v2->GetPipboyMouseY();
        double v4 = PipboyMouseY - MapMenu::pMe->pTiles[6]->pParent->GetRealY();
        float v3 = v4 - MapMenu::pMe->pTiles[6]->GetFloat(4016) / 2.0;
        MapMenu::pMe->pTiles[6]->Set(4002, v3, true);
    }
    MapMenu::pMe->pTiles[6]->Set(4003, 1);
}

void MapMenu::SetPlayerMarkerCallback() {
    int8_t MessageMenuResult = Interface::GetMessageMenuResult();
    if (MessageMenuResult) {
        if (MessageMenuResult == 1) {
            pPlayer->RemovePlayerMapMarker();
            MapMenu::pMe->DeletePlayerMarker();
            goto LABEL_8;
        }
        if (MessageMenuResult != 3) {
            goto LABEL_8;
        }
    }
    pPlayer->SetPlayerMapMarker(MapMenu::pMe->PlayerMarkerPos, MapMenu::pMe->PlayerMarkerSpace);
    if (!MessageMenuResult) {
        MapMenu::pMe->DeletePlayerMarker();
    }
    MapMenu::pMe->CreatePlayerMarker();
LABEL_8:;
    MapMenu::pMe->PlayerMarkerPos = NiPoint3_ZERO;
    MapMenu::pMe->PlayerMarkerSpace = nullptr;
    if (!InterfaceManager::Has360Controller()) {
        InterfaceManager* v0 = InterfaceManager::Instance();
        double PipboyMouseX = v0->GetPipboyMouseX();
        Tile* ProcessLevel = MapMenu::pMe->pTiles[6]->GetParent();
        double v8 = PipboyMouseX - ProcessLevel->GetRealX();
        float afValue = v8 - MapMenu::pMe->pTiles[6]->GetFloat(4017) / 2.0;
        MapMenu::pMe->pTiles[6]->Set(4001, afValue, true);
        InterfaceManager* v2 = InterfaceManager::Instance();
        double PipboyMouseY = v2->GetPipboyMouseY();
        Tile* v3 = MapMenu::pMe->pTiles[6]->GetParent();
        double v5 = PipboyMouseY - v3->GetRealY();
        float v4 = v5 - MapMenu::pMe->pTiles[6]->GetFloat(4016) / 2.0;
        MapMenu::pMe->pTiles[6]->Set(4002, v4, true);
    }
    MapMenu::pMe->pTiles[6]->Set(4003, 1);
}

void MapMenu::ClearAudioLogSubtites() {
    if (MapMenu::pMe) {
        if (MapMenu::pMe->pCurrentSound) {
            BSSoundHandle* v0 = &MapMenu::pMe->pCurrentSound->m_item;
            if (v0->IsPlaying()) {
                BSSoundHandle* v1 = &MapMenu::pMe->pCurrentSound->m_item;
                v1->Stop();
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
    if ((!MapMenu::pMe->pDisplayedQuestTarget) && (apObjective) && (!apObjective->GetCompleted())) {
        for (BSSimpleList<TESQuestTarget*>* ptargetObj = &apObjective->ListTargets;
             !MapMenu::pMe->pDisplayedQuestTarget && (ptargetObj && ptargetObj->m_item);
             ptargetObj = ptargetObj->m_pkNext) {
            TESQuestTarget* ptarget = ptargetObj->m_item;
            TESObjectREFR* Reference = ptarget->GetReference(false);
            if ((&ptarget->objConditions)->IsTrue(Reference, nullptr)) {
                MapMenu::pMe->pDisplayedQuestTarget = ptarget;
            }
        }
    }
}

bool MapMenu::ChallengeFilterFunc(TESChallenge* pcurrChallenge) {
    if (MapMenu::pMe) {
        pcurrChallenge->GetUnlocked();
    }
    uint32_t ChallengeType = pcurrChallenge->GetChallengeType();
    if (pcurrChallenge->GetProgressValue() <= 0 &&
        (!pcurrChallenge->GetShowZero() && !pcurrChallenge->GetCompletedRecurring())) {
        return true;
    }
    if (!MapMenu::cChallengeTypeDisplayed && ChallengeType <= 2) {
        return false;
    }
    if (MapMenu::cChallengeTypeDisplayed == 1 && (ChallengeType == 3 || ChallengeType == 8)) {
        return false;
    }
    return MapMenu::cChallengeTypeDisplayed != 2 ||
        (ChallengeType != 11 &&
         (ChallengeType != 10 &&
          (ChallengeType != 9 &&
           (ChallengeType != 6 &&
            (ChallengeType != 5 && (ChallengeType != 4 && (ChallengeType != 12 && ChallengeType != 13)))))));
}

int32_t MapMenu::ChallengeSortFunc(ListBoxItem<TESChallenge*>* apItem1, ListBoxItem<TESChallenge*>* apItem2) {
    float falpha1 = apItem1->pTile->GetFloat(MapMenu::iUT_TextAlpha);
    float falpha2 = apItem2->pTile->GetFloat(MapMenu::iUT_TextAlpha);
    if (falpha1 == 128.0 && falpha2 == 255.0) {
        return 1;
    }
    if (falpha1 == 255.0 && falpha2 == 128.0) {
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

void Tile::ClearNeedsUpdate(int32_t aiFlag) {
    if (uiFlags & aiFlag) {
        uiFlags ^= aiFlag;
    }
}

void TileShaderProperty::SetTintColor(const NiColorA& aTintColor) {
    TintColor = aTintColor;
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
