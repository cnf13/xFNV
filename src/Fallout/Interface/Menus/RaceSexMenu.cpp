#include "Fallout/Interface/Menus/RaceSexMenu.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSAudioManager.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTStaticFreeList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSDevices/Controls.h"
#include "BSMain/IOManager.h"
#include "BSMain/QueuedFiles.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSMain/SettingCollectionMap.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileImage.h"
#include "BSMenu/TileMenu.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "BSShader/Lighting30ShaderProperty.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSHeadPart.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Fallout Shared/FaceGen/BSFaceGenModelExtraData.h"
#include "Fallout Shared/FaceGen/BSFaceGenNiNode.h"
#include "Fallout Shared/FaceGen/FR2MatrixVTC.h"
#include "Fallout Shared/FaceGen/FaceGenDatas.h"
#include "Fallout Shared/FaceGenNpcData.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESEyes.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESHair.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESRace.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Character.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/FOPipboyManager.h"
#include "Fallout/Interface/FORenderedMenu.h"
#include "Fallout/Interface/FORenderedMenuRaceSex.h"
#include "Fallout/Interface/FORenderedTerminal.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/DialogMenu.h"
#include "Fallout/Interface/Menus/StatsMenu.h"
#include "Fallout/Misc/Main.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDefaultAVObjectPalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeom.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "SDK/MSVC/tchar.h"

#include <cstring>

static SettingT<INISettingCollection> gs_fRSMStartingZoom_Interface("fRSMStartingZoom:Interface", 0.7f);

// The game settings this file defines, with their IDs and values, in initialization order.
BSStringT<char> RaceSexMenu::strNextCaps;
BSStringT<char> RaceSexMenu::strDoneCaps;
BSStringT<char> RaceSexMenu::strBackCaps;
SettingT<INISettingCollection> fRSMFaceSliderDefaultMin("fRSMFaceSliderDefaultMin:Interface", -5.0f);
SettingT<INISettingCollection> fRSMFaceSliderDefaultMax("fRSMFaceSliderDefaultMax:Interface", 5.0f);

RaceSexMenu::Entry::Entry() : tile(nullptr), data(nullptr) {}

int32_t RaceSexMenu::Entry::GetValue() {
    return static_cast<int32_t>(tile->GetFloat(Tile::AddUserTrait("_position", -1)));
}

void RaceSexMenu::Entry::SetEnabled(bool abEnabled) {
    if (tile) {
        tile->Set(Tile::AddUserTrait("_enabled", -1), static_cast<int32_t>(abEnabled));
    }
}

void RaceSexMenu::Entry::SetDisplay(const char* Src) {
    if (tile && Src) {
        tile->Set(Tile::kTileValue_User5, Src, true);
    }
}

void RaceSexMenu::Entry::SetSelected2() {
    if (tile) {
        int32_t iSelected = Tile::AddUserTrait("_selected", -1);
        tile->Set(iSelected, 2.0f, true);
    }
}

void RaceSexMenu::Entry::SetSelected1() {
    if (tile) {
        int32_t iSelected = Tile::AddUserTrait("_selected", -1);
        tile->Set(iSelected, 1.0f, true);
    }
}

bool RaceSexMenu::Entry::HasSelectedValue2() {
    if (tile) {
        int32_t iSelected = Tile::AddUserTrait("_selected", -1);
        if (tile->GetFloat(iSelected) == 2.0) {
            return true;
        }
    }
    return false;
}

int32_t RaceSexMenu::Entry::GetSliderValue() {
    return static_cast<int32_t>(tile->GetFloat(Tile::kTileValue_User1));
}

bool RaceSexMenu::Entry::SetValue(int32_t aiVal) {
    int32_t iLow;
    int32_t iHigh;
    if (!tile) {
        return false;
    }
    int32_t iCur = GetSliderValue();
    GetRange(iLow, iHigh);
    if (aiVal < iLow) {
        aiVal = iLow;
    }
    if (aiVal > iHigh) {
        aiVal = iHigh;
    }
    if (iCur == aiVal) {
        return false;
    }
    tile->Set(Tile::kTileValue_User1, aiVal);
    Tile::Value* pDisplay = tile->GetValueQ(Tile::kTileValue_User5);
    if (!pDisplay->strValue) {
        pDisplay->SetFloat(static_cast<float>(aiVal), true);
    }
    return true;
}

void RaceSexMenu::Entry::GetRange(int32_t& aiLow, int32_t& aiHigh) {
    if (tile) {
        aiLow = static_cast<int32_t>(tile->GetFloat(Tile::kTileValue_User2));
        aiHigh = static_cast<int32_t>(tile->GetFloat(Tile::kTileValue_User3));
    }
}

void RaceSexMenu::Entry::SetRange(int32_t aiLow, int32_t aiHigh) {
    if (tile) {
        tile->Set(Tile::kTileValue_User2, aiLow);
        tile->Set(Tile::kTileValue_User3, aiHigh);
        if (GetSliderValue() < aiLow) {
            SetValue(aiLow);
        }
        if (GetSliderValue() > aiHigh) {
            SetValue(aiHigh);
        }
    }
}

uint32_t RaceSexMenu::Entry::GetIncrement() {
    if (!tile) {
        return 0;
    }
    float fIncrement = tile->GetFloat(Tile::kTileValue_User6);
    if (!(fIncrement <= 1.0)) {
        return static_cast<int64_t>(fIncrement);
    }

    return 1;
}

RaceSexMenu::List::List(int32_t aiID, const char* apName)
  : parentID(20), childID(20),
    listItemTemplate(
        RaceSexMenu::pMe->RenderTemplate(RaceSexMenu::pMe->rsmBackground, "RSM_list_item_template", nullptr)
    ),
    height(0.0f), IsItemSelected(nullptr), selectedEntry(nullptr) {
    listItemTemplate->Set(Tile::kTileValue_User0, aiID);
    listItemTemplate->Set(4036, apName, true);
    listItemTemplate->Set(4015, 0);
    listItemTemplate->Set(Tile::AddUserTrait("_is_header", -1), 1);
    listItemTemplate->UpdateTile(false);
}

// The list owns its header tile and its options (Flush frees them); the node list itself is items' own.
RaceSexMenu::List::~List() {
    if (listItemTemplate) {
        delete listItemTemplate;
    }
    listItemTemplate = nullptr;
    Flush();
}

char RaceSexMenu::List::Focus(char acFocus) {
    if (acFocus) {
        if (selectedEntry && selectedEntry->tile) {
            Tile* pTile = selectedEntry->tile;
            InterfaceManager::Instance()->SetCurrentFocusTarget(pTile, 4035, true);
            return 1;
        }
    }
    else {
        InterfaceManager::Instance()->SetCurrentFocusTarget(nullptr, 4035, true);
        selectedEntry = nullptr;
        return 1;
    }
    return 0;
}

void RaceSexMenu::List::Flush() {
    NiTListItem<RaceSexMenu::Entry*>* pos = items.m_pkHead;
    while (pos) {
        RaceSexMenu::Entry* pEntry = items.GetNext(pos);
        delete pEntry->tile;
        if (pEntry) {
            delete pEntry;
        }
    }
    items.RemoveAll();
    selectedEntry = nullptr;
}

RaceSexMenu::Entry* RaceSexMenu::List::AddBasicOption(const char* apName, int32_t aiID) {
    return AddMenuOption(apName, "RSM_list_item_template", aiID, nullptr);
}

RaceSexMenu::Entry* RaceSexMenu::List::AddMenuOption(
    const char* apName,
    const char* apTemplate,
    int32_t aiID,
    NiTListItem<RaceSexMenu::Entry*>* apIter
) {
    if (!apName || !RaceSexMenu::pMe || !RaceSexMenu::pMe->rsmBackground || !listItemTemplate) {
        return nullptr;
    }
    RaceSexMenu::Entry* pOption = new RaceSexMenu::Entry;
    int32_t iListID = static_cast<int32_t>(listItemTemplate->GetFloat(Tile::kTileValue_User0));
    pOption->id = aiID;
    pOption->tile = RaceSexMenu::pMe->RenderTemplate(RaceSexMenu::pMe->rsmBackground, apTemplate, nullptr);
    pOption->tile->Set(Tile::kTileValue_String, apName, true);
    pOption->tile->Set(Tile::kTileValue_User0, iListID);
    pOption->tile->Set(Tile::kTileValue_Target, 1);
    if (apIter) {
        items.InsertBefore(apIter, pOption);
    }
    else {
        items.AddTail(pOption);
    }
    apIter = items.FindPos(pOption, nullptr);
    while (apIter) {
        double dTop;
        Tile** ppPrevTile;
        NiTListItem<RaceSexMenu::Entry*>* pPrevPos = items.GetPrevPos(apIter);
        if (pPrevPos) {
            ppPrevTile = &items.Get(pPrevPos)->tile;
        }
        else {
            ppPrevTile = nullptr;
        }
        RaceSexMenu::Entry* pCurr = items.GetNext(apIter);
        if (ppPrevTile) {
            double dPrevY = (*ppPrevTile)->GetFloat(Tile::kTileValue_Y);
            dTop = (*ppPrevTile)->GetFloat(Tile::kTileValue_Height) + dPrevY;
        }
        else {
            int32_t iTopBound = Tile::AddUserTrait("_top_bound", -1);
            dTop = pCurr->tile->pParent->GetFloat(iTopBound);
        }
        float fTileY = dTop;
        pCurr->tile->Set(Tile::kTileValue_Y, fTileY, true);
        pCurr->tile->UpdateTile(false);
    }
    float fHeight = pOption->tile->GetFloat(Tile::kTileValue_Height);
    height = BSMAX(height, fHeight);
    if (iListID < 4 && pOption->id < 4) {
        pOption->tile->Set(Tile::kTileValue_ClickSound, "UIPipBoyMode", true);
    }
    if (!selectedEntry) {
        selectedEntry = pOption;
        pOption->tile->Set(Tile::kTileValue_xdefault, 2.0f, true);
    }
    return pOption;
}

// The format "%s %c" with '>' marks the option as opening a submenu.
RaceSexMenu::Entry* RaceSexMenu::List::AddSubmenuOption(const char* apName, List* apSubmenu) {
    BSStringT<char> kText;
    kText.SPrintF("%s %c", apName, '>');
    apSubmenu->parentID = static_cast<int32_t>(listItemTemplate->GetFloat(Tile::kTileValue_User0));
    int32_t iID = static_cast<int32_t>(apSubmenu->listItemTemplate->GetFloat(Tile::kTileValue_User0));
    Entry* pOption = AddMenuOption(kText.String(), "RSM_list_item_template", iID, nullptr);
    return pOption;
}

void RaceSexMenu::List::LinkForwardTo(List* apList) {
    childID = static_cast<int32_t>(apList->listItemTemplate->GetFloat(Tile::kTileValue_User0));
    apList->parentID = static_cast<int32_t>(listItemTemplate->GetFloat(Tile::kTileValue_User0));
}

void RaceSexMenu::List::UpdateCurrentSelection() {
    if (IsItemSelected) {
        NiTListItem<RaceSexMenu::Entry*>* pos = items.m_pkHead;
        while (pos) {
            RaceSexMenu::Entry* pEntry = items.GetNext(pos);
            if (IsItemSelected(pEntry)) {
                pEntry->SetSelected2();
            }
            else {
                pEntry->SetSelected1();
            }
        }
    }
}

RaceSexMenu::Entry* RaceSexMenu::List::AddSliderOption(
    const char* apDisplayText,
    int32_t aeID,
    int32_t aiLowVal,
    int32_t aiHighVal,
    int32_t aiStartVal,
    float afJumpPct
) {
    if (afJumpPct < 0.0) {
        afJumpPct = 0.0f;
    }
    if (afJumpPct > 1.0) {
        afJumpPct = 1.0f;
    }
    RaceSexMenu::Entry* pNew = AddMenuOption(apDisplayText, "RSM_slider_option_template", aeID, nullptr);
    pNew->SetRange(aiLowVal, aiHighVal);
    pNew->SetValue(aiStartVal);
    float fVal = static_cast<double>(aiHighVal - aiLowVal + 1) * afJumpPct;
    float fNearest = BSRoundToNearest(fVal, 1.0f);
    pNew->tile->Set(Tile::kTileValue_User4, fNearest, true);
    return pNew;
}

RaceSexMenu::Entry*
RaceSexMenu::List::InsertMenuOption(uint32_t aiPos, const char* apDisplayText, const char* apTemplate, int32_t aiID) {
    if (!apDisplayText || !RaceSexMenu::pMe || !RaceSexMenu::pMe->rsmBackground || !listItemTemplate) {
        return nullptr;
    }
    NiTListItem<RaceSexMenu::Entry*>* pIter = items.m_pkHead;
    while (pIter && aiPos) {
        pIter = items.GetNextPos(pIter);
        --aiPos;
    }
    return AddMenuOption(apDisplayText, apTemplate, aiID, pIter);
}

void RaceSexMenu::List::RemoveByID(uint32_t auiID) {
    NiTListItem<RaceSexMenu::Entry*>* pIter = items.m_pkHead;
    while (pIter) {
        RaceSexMenu::Entry* pEntry = items.GetNext(pIter);
        if (pEntry->id == auiID) {
            if (selectedEntry == pEntry) {
                RaceSexMenu::Entry* pSelected;
                if (pIter) {
                    pSelected = items.Get(pIter);
                }
                else {
                    pSelected = items.GetTail();
                }
                selectedEntry = pSelected;
                Interface::SetCurrentFocusTarget(selectedEntry->tile);
            }
            delete pEntry->tile;
            if (pEntry) {
                delete pEntry;
            }
            items.Remove(pEntry);
            break;
        }
    }
    // Moves each entry after the removed one to the bottom of the one before it.
    while (pIter) {
        Tile** ppPrevTile;
        NiTListItem<RaceSexMenu::Entry*>* pPrevPos = items.GetPrevPos(pIter);
        if (pPrevPos) {
            ppPrevTile = &items.Get(pPrevPos)->tile;
        }
        else {
            ppPrevTile = nullptr;
        }
        RaceSexMenu::Entry* pCurr = items.GetNext(pIter);
        double dTop;
        if (ppPrevTile) {
            double dPrevY = (*ppPrevTile)->GetFloat(Tile::kTileValue_Y);
            dTop = (*ppPrevTile)->GetFloat(Tile::kTileValue_Height) + dPrevY;
        }
        else {
            int32_t iTopBound = Tile::AddUserTrait("_top_bound", -1);
            dTop = pCurr->tile->pParent->GetFloat(iTopBound);
        }
        float fTileY = dTop;
        pCurr->tile->Set(Tile::kTileValue_Y, fTileY, true);
        pCurr->tile->UpdateTile(false);
    }
}

RaceSexMenu::Entry* RaceSexMenu::List::GetSliderWithID(int32_t aiID) {
    NiTListItem<RaceSexMenu::Entry*>* pIter = items.m_pkHead;
    while (pIter) {
        RaceSexMenu::Entry* pEntry = items.GetNext(pIter);
        if (pEntry->id == aiID) {
            return pEntry;
        }
    }
    return nullptr;
}

bool RaceSexMenu::List::HandleKey(int32_t aiKey) {
    char cResult;
    bool bPrevious = aiKey == 1 || aiKey == 15;
    char cHandled = 1;
    switch (aiKey) {
        case 1:
        case 2: {
            NiTListItem<RaceSexMenu::Entry*>* pNewPos;
            NiTListItem<RaceSexMenu::Entry*>* pPos = items.FindPos(selectedEntry, nullptr);
            if (pPos) {
                pNewPos = bPrevious ? items.GetPrevPos(pPos) : items.GetNextPos(pPos);
            }
            else {
                pNewPos = bPrevious ? items.m_pkTail : items.m_pkHead;
            }
            if (pNewPos) {
                selectedEntry = items.Get(pNewPos);
            }
            else {
                cHandled = 0;
            }
            break;
        }
        case 15: {
            selectedEntry = items.GetHead();
            break;
        }
        case 16: {
            selectedEntry = items.GetTail();
            break;
        }
        default: {
            cResult = 0;
            return cResult;
        }
    }
    if (selectedEntry && selectedEntry->tile) {
        int32_t iTries = 100;
        while (iTries-- && !selectedEntry->tile->IsVisible()) {
            RaceSexMenu::pMe->DoClick(3 - bPrevious, nullptr);
        }
        Interface::SetCurrentFocusTarget(selectedEntry->tile);
    }

    cResult = cHandled;
    return cResult;
}

void RaceSexMenu::List::SetActive() {
    if (InterfaceManager::Has360Controller()) {
        if (!selectedEntry && !items.IsEmpty()) {
            selectedEntry = items.GetHead();
        }
        if (selectedEntry) {
            BSStringT<char> kString(selectedEntry->tile->GetString(4072));
            selectedEntry->tile->Set(4072, -1);
            Interface::SetCurrentFocusTarget(selectedEntry->tile);
            selectedEntry->tile->Set(4072, kString.pString, true);
            while (true) {
                int32_t iValue = selectedEntry->GetValue();
                if (!iValue) {
                    break;
                }
                ScrollBy(-iValue);
            }
        }
    }
}

void RaceSexMenu::List::ScrollBy(int32_t aiDelta) {
    if (!(height <= 0.0)) {
        NiTListItem<RaceSexMenu::Entry*>* pos = items.m_pkHead;
        while (pos) {
            Tile* pTile = items.GetNext(pos)->tile;
            float fY = pTile->GetFloat(4002);
            float fValue = static_cast<double>(aiDelta) * height + fY;
            pTile->Set(4002, fValue, true);
        }
    }
}

RaceSexMenu::Entry* RaceSexMenu::List::Find(Tile* apTile) {
    NiTListItem<RaceSexMenu::Entry*>* pIter = items.m_pkHead;
    while (pIter) {
        RaceSexMenu::Entry* pOption = items.GetNext(pIter);
        if (pOption->tile == apTile) {
            return pOption;
        }
    }
    return nullptr;
}

RaceSexMenu::RaceSexMenu()
  : pOptionListsA{}, pTiles{}, fDragStartX(0.0f), iDragStartSliderVal(0), pSliderElem(nullptr), pSliderOpt(nullptr),
    fZoomTarget(0.0f), fZoomVelocity(0.0f), fZoomPct(0.0f), uiFlags(3), eLastList(RaceSexMenu::RSM_LIST_COUNT) {
    ptZoomBound[1] = NiPoint3::ZERO;
    ptZoomBound[0] = ptZoomBound[1];
    BSAudioManager& kManager = BSAudioManager::QInstance();
    const char* apSoundNames[4] = {"OBJComputerTerminal01LP", "QSTBirthRoomLP", "UIFaceGenZoomLP", "UIFaceGenRotateLP"};
    for (int32_t i = 0; i < 4; ++i) {
        Sounds[i] = kManager.GetSoundHandleByName(apSoundNames[i], 49u);
        Sounds[i].bAssumeSuccess = true;
    }
}

RaceSexMenu::~RaceSexMenu() {
    InterfaceManager::Instance()->EnterRenderedMenu(nullptr);
    pPlayer->GetRace()->KillEGTData();
    for (int32_t i = 0; i < 20; ++i) {
        if (pOptionListsA[i]) {
            delete pOptionListsA[i];
        }
    }
    for (int32_t j = 0; j < 4; ++j) {
        Sounds[j].Stop();
        Sounds[j].Release();
    }
}

void RaceSexMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
    if (aiID >= 0 && aiID < 6) {
        pTiles[aiID] = apTile;
    }
}

void RaceSexMenu::DoDownClick(int32_t aiID, Tile* apTarget) {
    if (aiID != 1) {
        if (aiID != 105) {
            return;
        }
        pSliderElem = apTarget;
        pSliderOpt = pOptionListsA[GetActiveList()]->Find(apTarget->pParent);
        iDragStartSliderVal = pSliderOpt->GetSliderValue();
    }
    fDragStartX = InterfaceManager::Instance()->GetPipboyMouseX();
}

void RaceSexMenu::DoClick(int32_t aiID, Tile* apTarget) {
    if (!apTarget && aiID < 6) {
        apTarget = pTiles[aiID];
    }
    RaceSexMenu::ListID eActiveList = GetActiveList();
    RaceSexMenu::Entry* pOption = pOptionListsA[eActiveList]->Find(apTarget);
    if (pOption) {
        bool bUpdateSelection = false;
        bool bUpdateLocomotion = false;
        switch (pOption->id) {
            case RSM_FEMALE: {
                pPlayerBase->GetHeadParts()->RemoveAll();
                [[fallthrough]];
            }
            case RSM_MALE:
            case RSM_RACE_OPTION: {
                if (!pOption->HasSelectedValue2()) {
                    if (pOption->id == static_cast<uint32_t>(RSM_RACE_OPTION)) {
                        pPlayerBase->GetRace()->KillEGTData();
                        pPlayer->DispellSpellsForRace();
                        pPlayer->RemoveEffects(false);
                        pPlayer->DispelPermanentMagic();
                        pPlayerBase->SwitchRace(static_cast<TESRace*>(pOption->data), nullptr);
                    }
                    else {
                        pPlayerBase->GetRace()->KillEGTData();
                        pPlayerBase->SetFlagBit(
                            TESActorBaseData::BASE_FEMALE,
                            pOption->id == static_cast<uint32_t>(RSM_FEMALE),
                            true
                        );
                        bUpdateLocomotion = true;
                    }
                    InitPresetsListAndPickFirstPreset();
                    UpdatePlayerBody();
                    ValidateHairAndEyes();
                    bUpdateSelection = true;
                    if (bUpdateLocomotion) {
                        pPlayer->QueueReplacementLocomotions(true);
                    }
                }
                break;
            }
            case RSM_RANDOMIZE_FACE: {
                if (IsFlagged(RaceSexMenu::RSM_FLAG_RANDOM_HEAD)) {
                    RaceSexMenu::RandomizeCallback();
                }
                else {
                    const char* pYesText = sYes.String();
                    const char* pNoText = sNo.String();
                    Interface::CreateMessageMenu(
                        sRSMConfirmRandomize.String(),
                        static_cast<const char*>(nullptr),
                        static_cast<const char*>(nullptr),
                        &RaceSexMenu::RandomizeCallback,
                        0,
                        18,
                        0.0f,
                        0.0f,
                        pNoText,
                        pYesText,
                        nullptr
                    );
                }
                break;
            }
            case RSM_HAIR_STYLE_OPTION: {
                if (!pOption->HasSelectedValue2()) {
                    pPlayerBase->SetHair(static_cast<TESHair*>(pOption->data));
                    UpdatePlayerHair(true);
                    bUpdateSelection = true;
                }
                break;
            }
            case RSM_FACIAL_HAIR_OPTION: {
                pPlayerBase->GetHeadParts()->RemoveAll();
                if (!pOption->HasSelectedValue2()) {
                    pPlayerBase->GetHeadParts()->AddHead(pOption->data);
                }
                UpdatePlayerHead(true, true, false);
                bUpdateSelection = true;
                break;
            }
            case RSM_EYE_COLOR_OPTION: {
                if (!pOption->HasSelectedValue2()) {
                    pPlayerBase->SetEyeColor(static_cast<TESEyes*>(pOption->data));
                    UpdatePlayerEyes();
                    bUpdateSelection = true;
                }
                break;
            }
            default: {
                if (pOption->id < RSM_LIST_COUNT) {
                    SetActiveList(static_cast<RaceSexMenu::ListID>(pOption->id));
                }
                break;
            }
        }
        if (bUpdateSelection) {
            pOptionListsA[eActiveList]->UpdateCurrentSelection();
            SetFlag(RaceSexMenu::RSM_FLAG_RANDOM_HEAD, false);
        }
    }
    else if (aiID == 4) {
        SetActiveList(static_cast<RaceSexMenu::ListID>(pOptionListsA[eActiveList]->parentID));
    }
    else if (aiID == 5) {
        if (eActiveList == eLastList) {
            const char* pYesText = sYes.String();
            const char* pNoText = sNo.String();
            Interface::CreateMessageMenu(
                static_cast<const char*>(strConfirmationMsg.pString),
                static_cast<const char*>(nullptr),
                static_cast<const char*>(nullptr),
                &RaceSexMenu::Close,
                1,
                18,
                0.0f,
                0.0f,
                pNoText,
                pYesText,
                nullptr
            );
        }
        else {
            SetActiveList(static_cast<RaceSexMenu::ListID>(pOptionListsA[eActiveList]->childID));
        }
    }
    else {
        pOption = pOptionListsA[eActiveList]->Find(apTarget->pParent);
        if (pOption) {
            pSliderElem = apTarget;
            pSliderOpt = pOption;
            SliderUpdate();
        }
        else if (aiID == 2 && apTarget->IsTrue(Tile::kTileValue_Visible)) {
            pOptionListsA[eActiveList]->ScrollBy(1);
            UpdateScrollArrows();
        }
        else if (aiID == 3 && apTarget->IsTrue(Tile::kTileValue_Visible)) {
            pOptionListsA[eActiveList]->ScrollBy(-1);
            UpdateScrollArrows();
        }
    }
}

void RaceSexMenu::DoStillDown(int32_t aiID, Tile* apTarget) {
    float fDeltaX = InterfaceManager::Instance()->GetPipboyMouseX() - fDragStartX;
    if (fDeltaX != 0.0) {
        if (aiID == 105 && apTarget) {
            RaceSexMenu::ListID eActiveList = GetActiveList();
            RaceSexMenu::Entry* pOption = pOptionListsA[eActiveList]->Find(apTarget->pParent);
            if (pOption) {
                int32_t iLow;
                int32_t iHigh;
                pOption->GetRange(iLow, iHigh);
                double dWidth = apTarget->pParent->GetChildByID(101)->GetFloat(4017);
                float fDeltaPct = fDeltaX / dWidth;
                float fVal = static_cast<double>(iDragStartSliderVal) + static_cast<double>(iHigh - iLow) * fDeltaPct;
                double dNearest = BSRoundToNearest(fVal, 1.0f);
                if (pOption->SetValue(static_cast<int32_t>(dNearest))) {
                    pSliderOpt = pOption;
                    pSliderElem = apTarget;
                    SliderRelease();
                }
            }
        }
        else if (aiID == 1) {
            static float ffullRotX = rsmFaceGrab->GetFloat(4017);
            float fRadians = fDeltaX / ffullRotX * 6.2831854820251465;
            bool bRotated = RotatePlayer(fRadians, false);
            SetFlag(RaceSexMenu::RSM_FLAG_ROTATING, bRotated);
            fDragStartX = InterfaceManager::Instance()->GetPipboyMouseX();
        }
    }
}

void RaceSexMenu::DoIdle() {
    if (Interface::IsTopMenuID(static_cast<Interface::enumMenus>(GetClass()))) {
        for (uint32_t i = 0; i < 4; ++i) {
            if (Sounds[i].eState == 3) {
                Sounds[i].Play(false);
            }
        }
        static int32_t iLastTime = BSGetTickCount();
        uint32_t uiThisTime = BSGetTickCount();
        if (InterfaceManager::Has360Controller()) {
            int32_t iMouseX = appMain->GetControls()->GetControllerValue(0, 9);
            int32_t iMouseY = appMain->GetControls()->GetControllerValue(0, 10);
            int32_t iAbsX = BSAbs(iMouseX);
            int32_t iAbsY = BSAbs(iMouseY);
            SetFlag(RSM_FLAG_ROTATING, false);
            SetFlag(RSM_FLAG_ZOOMING, false);
            if (iAbsY <= 0x21F1 || iAbsY <= iAbsX) {
                if (iAbsX > 0x21F1 && iAbsX > iAbsY) {
                    double dStickDeflection;
                    if (iMouseX <= 0) {
                        dStickDeflection = static_cast<double>(-iMouseX - 0x21F1) /
                            static_cast<double>(iRSMStickAxisMin + 0x21F1);
                    }
                    else {
                        dStickDeflection = static_cast<double>(iMouseX - 0x21F1) /
                            static_cast<double>(iRSMStickAxisMax - 0x21F1);
                    }
                    float fRotate = dStickDeflection;
                    float fRadians = fRotate * 2.5132741928100586 * static_cast<double>(uiThisTime - iLastTime) /
                        1000.0;
                    bool bRotated = RotatePlayer(fRadians, false);
                    SetFlag(RSM_FLAG_ROTATING, bRotated);
                }
                fZoomVelocity = 0.0f;
            }
            else {
                int32_t iDirection;
                int32_t iRange = iRSMStickAxisMax - 0x21F1;
                fZoomVelocity = static_cast<double>(iRange - (iRSMStickAxisMax - BSMIN(iAbsY, iRSMStickAxisMax))) *
                        3.12499992105586e-06 / static_cast<double>(iRange) *
                        static_cast<double>(uiThisTime - iLastTime) +
                    fZoomVelocity;
                if (!(static_cast<double>(iMouseY) > 0.0)) {
                    iDirection = 1;
                }
                else {
                    iDirection = -1;
                }
                float fZoomDelta = static_cast<double>(iDirection) * fZoomVelocity *
                    static_cast<double>(uiThisTime - iLastTime);
                bool bZoomed = ZoomPlayer(fZoomDelta, false);
                SetFlag(RSM_FLAG_ZOOMING, bZoomed);
            }
            if (!IsFlagged(RSM_FLAG_ROTATING) || Sounds[3].IsPlaying()) {
                if (!IsFlagged(RSM_FLAG_ROTATING) && Sounds[3].IsPlaying()) {
                    Sounds[3].Stop();
                }
            }
            else {
                Sounds[3].Play(false);
            }
            if (!IsFlagged(RSM_FLAG_ZOOMING) || Sounds[2].IsPlaying()) {
                if (!IsFlagged(RSM_FLAG_ZOOMING) && Sounds[2].IsPlaying()) {
                    Sounds[2].Stop();
                }
            }
            else {
                Sounds[2].Play(false);
            }
            if (pSliderOpt && IsFlagged(RSM_FLAG_STICK_RELEASED)) {
                SliderRelease();
                pSliderOpt = nullptr;
                pSliderElem = nullptr;
            }
        }
        else {
            SetFlag(RSM_FLAG_ZOOMING, false);
            if (InterfaceManager::Instance()->GetMouseWheel() && InterfaceManager::Instance()->pOverTileTarget) {
                bool bWheelUp = static_cast<double>(InterfaceManager::Instance()->GetMouseWheel()) > 0.0;
                if (InterfaceManager::Instance()->pOverTileTarget == rsmFaceGrab) {
                    float fStep = bWheelUp ? -0.125f : 0.125f;
                    fZoomTarget = fZoomTarget + fStep;
                    float fClamped = BSMIN(fZoomTarget, 1.0f);
                    fZoomTarget = BSMAX(fClamped, 0.0f);
                }
                else if (bWheelUp) {
                    DoClick(2, nullptr);
                }
                else {
                    DoClick(3, nullptr);
                }
            }

            if (!(fZoomTarget > static_cast<double>(fZoomPct))) {
                if (!(fZoomTarget < static_cast<double>(fZoomPct))) {
                    fZoomVelocity = 0.0f;
                }
                else {
                    fZoomVelocity = fZoomVelocity - static_cast<double>(uiThisTime - iLastTime) * 3.12499992105586e-06;
                    float fZoomDelta = static_cast<double>(uiThisTime - iLastTime) * fZoomVelocity;
                    if (fZoomTarget > fZoomPct + fZoomDelta) {
                        fZoomDelta = fZoomTarget - fZoomPct;
                    }
                    bool bZoomed = ZoomPlayer(fZoomDelta, false);
                    SetFlag(RSM_FLAG_ZOOMING, bZoomed);
                }
            }
            else {
                fZoomVelocity = static_cast<double>(uiThisTime - iLastTime) * 3.12499992105586e-06 + fZoomVelocity;
                float fZoomDelta = static_cast<double>(uiThisTime - iLastTime) * fZoomVelocity;
                if (fZoomTarget < fZoomPct + fZoomDelta) {
                    fZoomDelta = fZoomTarget - fZoomPct;
                }
                bool bZoomed = ZoomPlayer(fZoomDelta, false);
                SetFlag(RSM_FLAG_ZOOMING, bZoomed);
            }
            if (!IsFlagged(RSM_FLAG_ZOOMING) || Sounds[2].IsPlaying()) {
                if (!IsFlagged(RSM_FLAG_ZOOMING) && Sounds[2].IsPlaying()) {
                    Sounds[2].Stop();
                }
            }
            else {
                Sounds[2].Play(false);
            }
            if (!IsFlagged(RSM_FLAG_ROTATING) || Sounds[3].IsPlaying()) {
                if (!IsFlagged(RSM_FLAG_ROTATING) && Sounds[3].IsPlaying()) {
                    Sounds[3].Stop();
                }
            }
            else {
                Sounds[3].Play(false);
            }
            SetFlag(RSM_FLAG_ROTATING, false);
            if (pSliderOpt && appMain->GetControls()) {
                if (appMain->GetControls()->GetMouseButton(0, 2)) {
                    SliderUpdate();
                }
            }
        }
        iLastTime = uiThisTime;
    }
    else {
        for (uint32_t j = 0; j < 4; ++j) {
            if (Sounds[j].eState == 1) {
                Sounds[j].Pause();
            }
        }
    }
}

bool RaceSexMenu::DoGamepad(int32_t aiButton, float afValue) {
    bool bResult;
    if (!Interface::IsTopMenuID(static_cast<Interface::enumMenus>(GetClass()))) {
        return false;
    }
    if (aiButton >= 0) {
        int32_t iMouseX = appMain->GetControls()->GetControllerValue(0, 7);
        if (aiButton == 4 || aiButton == 3 || BSAbs(iMouseX) >= 0x1EA9) {
            SetFlag(RaceSexMenu::RSM_FLAG_STICK_RELEASED, false);
        }
        else {
            SetFlag(RaceSexMenu::RSM_FLAG_STICK_RELEASED, true);
        }
    }
    switch (aiButton) {
        case 2: {
            if (InterfaceManager::Instance()->pMouseOverTarget == rsmBackButton ||
                InterfaceManager::Instance()->pMouseOverTarget == rsmNextButton ||
                pOptionListsA[GetActiveList()]->HandleKey(aiButton)) {
                return true;
            }
            if (rsmBackButton->IsVisible()) {
                pOptionListsA[GetActiveList()]->Focus(0);
                Interface::SetCurrentFocusTarget(rsmBackButton);
                return true;
            }
            if (!rsmNextButton->IsVisible()) {
                return false;
            }
            pOptionListsA[GetActiveList()]->Focus(0);
            Interface::SetCurrentFocusTarget(rsmNextButton);
            return true;
        }
        case 3:
        case 4:
        case 15:
        case 16: {
            int32_t iID;
            Tile* pChild;
            Tile* pCFT;
            pCFT = InterfaceManager::Instance()->pMouseOverTarget;
            if (!pCFT && !InterfaceManager::Has360Controller()) {
                for (pCFT = InterfaceManager::Instance()->pOverTileTarget; pCFT && pCFT->pParent != rsmBackground;
                     pCFT = pCFT->pParent) {
                    ;
                }
            }
            if (pCFT == rsmBackButton && aiButton == 3 && rsmNextButton->IsVisible()) {
                Interface::SetCurrentFocusTarget(rsmNextButton);
                return true;
            }
            if (pCFT == rsmNextButton && aiButton == 4 && rsmBackButton->IsVisible()) {
                Interface::SetCurrentFocusTarget(rsmBackButton);
                return true;
            }
            if (pCFT) {
                switch (aiButton) {
                    case 15: {
                        iID = 102;
                        break;
                    }
                    case 16: {
                        iID = 103;
                        break;
                    }
                    case 3: {
                        iID = 104;
                        break;
                    }
                    case 4: {
                        iID = 100;
                        break;
                    }
                    default: {
                        return false;
                    }
                }
                pChild = pCFT->GetChildByID(iID);
                if (pChild) {
                    DoClick(iID, pChild);
                    bResult = true;
                    break;
                }
            }
            [[fallthrough]];
        }
        case 1: {
            bResult = pOptionListsA[GetActiveList()]->HandleKey(aiButton);
            break;
        }
        case 10: {
            if (SetActiveList(static_cast<RaceSexMenu::ListID>(pOptionListsA[GetActiveList()]->parentID))) {
                BSAudio::QInstance().GetSoundHandleByName("UIMenuOK", 289u).Play(false);
            }
            return true;
        }
        case 13:
        case 14: {
            RaceSexMenu::ListID eList;
            RaceSexMenu::ListID eTop;
            for (eTop = GetActiveList(); eTop >= RaceSexMenu::RSM_CUSTOM;
                 eTop = static_cast<RaceSexMenu::ListID>(pOptionListsA[eTop]->parentID)) {
                ;
            }
            if (aiButton == 13) {
                eList = static_cast<RaceSexMenu::ListID>(pOptionListsA[eTop]->parentID);
            }
            else {
                eList = static_cast<RaceSexMenu::ListID>(pOptionListsA[eTop]->childID);
            }
            if (SetActiveList(eList)) {
                BSAudio::QInstance().GetSoundHandleByName("UIPipBoyMode", 289u).Play(false);
            }
            else if (aiButton == 14) {
                const char* pYesText = sYes.String();
                const char* pNoText = sNo.String();
                Interface::CreateMessageMenu(
                    static_cast<const char*>(strConfirmationMsg.pString),
                    static_cast<const char*>(nullptr),
                    static_cast<const char*>(nullptr),
                    &RaceSexMenu::Close,
                    1,
                    18,
                    0.0f,
                    0.0f,
                    pNoText,
                    pYesText,
                    nullptr
                );
            }
            return true;
        }
        default: {
            return false;
        }
    }
    return bResult;
}

void RaceSexMenu::_Create(RaceSexMenuContext aeContext) {
    // Compares the float trait with double constants.
    if (pMenu->GetFloat(Tile::kTileValue_StackingType) == 6006.0 ||
        pMenu->GetFloat(Tile::kTileValue_StackingType) == 102.0) {
        pMenu->Set(Tile::kTileValue_Depth, Tile::GetMaximumDepth(), true);
    }
    int32_t iMenuID = GetClassStatic();
    InterfaceManager::Instance()->xTutorialManager.ShowMessage(Interface::TUT_CHARGEN_RACE, iMenuID, 0x200);
    GetMenuTile()->Set(Tile::kTileValue_User1, "<", true);
    GetMenuTile()->Set(Tile::kTileValue_User2, ">", true);
    TESBoundObject* pBase = pPlayer->GetObjectReference();
    pPlayerBase = dynamic_cast<TESNPC*>(pBase);
    rsmBackButton->Set(Tile::kTileValue_String, strBackCaps.String(), true);

    // The four title strings.
    BSStringT<char> akTitles[4];
    if (aeContext < 2 || aeContext > 3) {
        akTitles[0].SPrintF("1. %s", sRSMSex.String());
        akTitles[1].SPrintF("2. %s", sRSMRace.String());
        akTitles[2].SPrintF("3. %s", sRSMFace.String());
        akTitles[3].SPrintF("4. %s", sRSMHair.String());
    }
    else {
        akTitles[0].Set(sRSMSex.String(), 0);
        akTitles[1].Set(sRSMRace.String(), 0);
        akTitles[2].Set(sRSMFace.String(), 0);
        akTitles[3].Set(sRSMHair.String(), 0);
    }
    const char* apListNames[20];
    apListNames[0] = akTitles[0].String();
    apListNames[1] = akTitles[1].String();
    apListNames[2] = akTitles[2].String();
    apListNames[3] = akTitles[3].String();
    apListNames[4] = sRSMCustomize.String();
    apListNames[5] = sRSMHairStyle.String();
    apListNames[6] = sRSMHairColor.String();
    apListNames[7] = sRSMFacialHair.String();
    apListNames[8] = sRSMEyeColor.String();
    apListNames[9] = sShape.String();
    apListNames[10] = sRSMGeneral.String();
    apListNames[11] = sForehead.String();
    apListNames[12] = sBrow.String();
    apListNames[13] = sEyes.String();
    apListNames[14] = sNose.String();
    apListNames[15] = sMouth.String();
    apListNames[16] = sCheeks.String();
    apListNames[17] = sJaw.String();
    apListNames[18] = sChin.String();
    apListNames[19] = sTone.String();
    for (int32_t i = 0; i < 20; ++i) {
        pOptionListsA[i] = new List(i, apListNames[i]);
    }
    pOptionListsA[0]->SetIsItemSelected(&RaceSexMenu::IsPlayerSex);
    pOptionListsA[1]->SetIsItemSelected(&RaceSexMenu::IsPlayerRace);
    pOptionListsA[5]->SetIsItemSelected(&RaceSexMenu::IsPlayerHair);
    pOptionListsA[8]->SetIsItemSelected(&RaceSexMenu::IsPlayerEyes);
    pOptionListsA[7]->SetIsItemSelected(&RaceSexMenu::IsPlayerHeadPart);
    pOptionListsA[0]->AddBasicOption(static_cast<const char*>(sMale), 21);
    pOptionListsA[0]->AddBasicOption(static_cast<const char*>(sFemale), 22);
    for (BSSimpleList<TESRace*>* pIter = pDataHandler->GetRaceList(); pIter && pIter->GetItem();
         pIter = pIter->GetNext()) {
        if (pIter->GetItem()->GetPlayable()) {
            void* pRace = pIter->GetItem();
            TESRace* pNameRace = pIter->GetItem();
            const char* pName = static_cast<TESFullName&>(*pNameRace).GetFullName();
            pOptionListsA[1]->AddBasicOption(pName, 23)->SetData(pRace);
        }
    }
    pOptionListsA[2]->AddSliderOption(static_cast<const char*>(sRSMPreset), 24, 1, 20, -2147483647, 0.25f);
    pOptionListsA[2]->AddSubmenuOption(static_cast<const char*>(sRSMCustomize), pOptionListsA[4]);
    pOptionListsA[2]->AddBasicOption(static_cast<const char*>(sRSMRandomize), 25);
    Entry* pAge = pOptionListsA[2]->AddSliderOption(static_cast<const char*>(sRSMAge), 26, 1, 10, -2147483647, 0.25f);
    pAge->SetDisplay(" ");
    pOptionListsA[3]->AddSubmenuOption(static_cast<const char*>(sRSMHairStyle), pOptionListsA[5]);
    pOptionListsA[3]->AddSubmenuOption(static_cast<const char*>(sRSMHairColor), pOptionListsA[6]);
    if (pPlayerBase->GetSex() == 0) {
        pOptionListsA[3]->AddSubmenuOption(static_cast<const char*>(sRSMFacialHair), pOptionListsA[7]);
    }
    else {
        pOptionListsA[7]->SetParentID(3);
    }
    pOptionListsA[4]->AddSubmenuOption(static_cast<const char*>(sShape), pOptionListsA[9]);
    pOptionListsA[4]->AddSubmenuOption(static_cast<const char*>(sTone), pOptionListsA[19]);
    pOptionListsA[4]->AddSubmenuOption(static_cast<const char*>(sRSMEyeColor), pOptionListsA[8]);
    uint32_t uiHairColor = pPlayerBase->GetHairColor();
    const char* pCustom = sRSMCustom.String();
    pOptionListsA[6]
        ->AddSliderOption(static_cast<const char*>(sRSMPreset), 29, 0, 15, -2147483647, 0.25f)
        ->SetDisplay(pCustom);
    pOptionListsA[6]->AddSliderOption(
        static_cast<const char*>(sRSMRedAbbrev),
        30,
        0,
        255,
        static_cast<uint8_t>(uiHairColor),
        0.125f
    );
    pOptionListsA[6]->AddSliderOption(
        static_cast<const char*>(sRSMGreenAbbrev),
        31,
        0,
        255,
        static_cast<uint8_t>(static_cast<uint16_t>(uiHairColor) >> 8),
        0.125f
    );
    pOptionListsA[6]->AddSliderOption(
        static_cast<const char*>(sRSMBlueAbbrev),
        32,
        0,
        255,
        static_cast<uint8_t>(uiHairColor >> 16),
        0.125f
    );
    for (BSSimpleList<BGSHeadPart*>* pIter = pDataHandler->GetHeadPartList(); pIter && pIter->GetItem();
         pIter = pIter->GetNext()) {
        if (pIter->GetItem()->IsPlayable()) {
            void* pPart = pIter->GetItem();
            BGSHeadPart* pNamePart = pIter->GetItem();
            const char* pName = static_cast<TESFullName&>(*pNamePart).GetFullName();
            pOptionListsA[7]->AddBasicOption(pName, 28)->SetData(pPart);
        }
    }
    for (int32_t k = 10; k < 19; ++k) {
        pOptionListsA[9]->AddSubmenuOption(apListNames[k], pOptionListsA[k]);
    }
    AddFaceSliders();

    BSStringT<char> kTitle;
    ListID eActiveList;
    if (aeContext == 2) {
        strProjectorFile.Set("Terminals\\BarberInterface01.NIF", 0);
        kTitle.Set(sRSMHair.String(), 0);
        eActiveList = RSM_HAIR;
        eLastList = eActiveList;
        ptZoomBound[0].x = -7.4f;
        ptZoomBound[0].y = 80.0f;
        ptZoomBound[0].z = -0.84f;
        ptZoomBound[1].x = -12.4f;
        ptZoomBound[1].y = 135.0f;
        ptZoomBound[1].z = -0.8f;
    }
    else if (aeContext == 3) {
        strProjectorFile.Set("Terminals\\PlasticSurgeryInterface01.NIF", 0);
        kTitle.Set(sRSMFace.String(), 0);
        eActiveList = RSM_FACE;
        eLastList = eActiveList;
        ptZoomBound[0].x = -5.2f;
        ptZoomBound[0].y = 55.0f;
        ptZoomBound[0].z = -0.84f;
        ptZoomBound[1].x = -12.4f;
        ptZoomBound[1].y = 135.0f;
        ptZoomBound[1].z = -0.77f;
        MakePresetHeadList();
    }
    else {
        strProjectorFile.Set("Terminals\\NV_reflectron_UI.NIF", 0);
        kTitle.Set(sRSMCharacter.String(), 0);
        pOptionListsA[0]->LinkForwardTo(pOptionListsA[1]);
        pOptionListsA[1]->LinkForwardTo(pOptionListsA[2]);
        pOptionListsA[2]->LinkForwardTo(pOptionListsA[3]);
        eLastList = RSM_HAIR;
        eActiveList = RSM_SEX;
        ptZoomBound[0].x = -4.7f;
        ptZoomBound[0].y = 55.0f;
        ptZoomBound[0].z = -0.84f;
        ptZoomBound[1].x = -11.4f;
        ptZoomBound[1].y = 135.0f;
        ptZoomBound[1].z = -0.8f;
        MakePresetHeadList();
    }
    // 1 is BSSysInfoSystemUtility::LANGUAGE_ENGLISH.
    if (GetCurrentTexLanguage() != 1) {
        kTitle.MakeLower();
    }
    const char* pTitle = kTitle.String();
    const char* pConfirm = sRSMConfirmDone.String();
    strConfirmationMsg.SPrintF("%s %s?", pConfirm, pTitle);
    float fZoom = BSMIN(1.0f, gs_fRSMStartingZoom_Interface.Float());
    fZoomPct = BSMAX(0.0f, fZoom);
    fZoomTarget = fZoomPct;
    pOptionListsA[0]->UpdateCurrentSelection();
    pOptionListsA[1]->UpdateCurrentSelection();
    pOptionListsA[7]->UpdateCurrentSelection();
    Sounds[0].Play(false);
    Sounds[1].Play(false);
    SetActiveList(eActiveList);
    PrepForVisibility(false);
}

bool RaceSexMenu::SetActiveList(RaceSexMenu::ListID aeList) {
    if (aeList >= RaceSexMenu::RSM_LIST_COUNT) {
        return false;
    }
    pMenu->Set(Tile::kTileValue_User0, aeList);
    pOptionListsA[aeList]->SetActive();
    if (aeList == RaceSexMenu::RSM_HAIR_STYLE && IsFlagged(RaceSexMenu::RSM_FLAG_MAKE_NEW_HAIRLIST)) {
        InitHairStyles();
    }
    else if (aeList == RaceSexMenu::RSM_EYE_COLOR && IsFlagged(RaceSexMenu::RSM_FLAG_MAKE_NEW_EYESLIST)) {
        MakeEyeColorList();
    }
    else if (aeList == RaceSexMenu::RSM_HAIR) {
        ValidateHairOptionList();
    }
    UpdateScrollArrows();
    if (InterfaceManager::Instance()->GetCurrentRenderedMenu() && aeList < RaceSexMenu::RSM_CUSTOM) {
        FORenderedMenuRaceSex* pRenderedMenu = static_cast<FORenderedMenuRaceSex*>(
            InterfaceManager::Instance()->GetCurrentRenderedMenu()
        );
        pRenderedMenu->LightButton(aeList);
    }

    // The parent and child list IDs are compared signed.
    int32_t iParentID = pOptionListsA[aeList]->parentID;
    rsmBackButton->Set(Tile::kTileValue_Visible, static_cast<int32_t>(iParentID < 20));
    int32_t iChildID = pOptionListsA[aeList]->childID;
    int32_t iNextVisible = iChildID < 20 || aeList == eLastList;
    rsmNextButton->Set(Tile::kTileValue_Visible, iNextVisible);
    BSStringT<char>* pCaption = aeList == eLastList ? &RaceSexMenu::strDoneCaps : &RaceSexMenu::strNextCaps;
    rsmNextButton->Set(4036, pCaption->pString, true);
    return true;
}

RaceSexMenu::ListID RaceSexMenu::GetActiveList() {
    return static_cast<RaceSexMenu::ListID>(pMenu->GetFloat(Tile::kTileValue_User0));
}

// strProjectorFile's text, the model the rendered menu loads.
const char* RaceSexMenu::GetProjectorFile() {
    return strProjectorFile.pString;
}

void RaceSexMenu::MakePresetHeadList() {
    xPresetHeadsA.Clear(false);
    TESActorBase::SEX eSex = pPlayerBase->GetSex();
    TESRace* pRace = pPlayerBase->GetRace();
    TESNPC::FillCharGenPresetList(pRace, eSex, &xPresetHeadsA);
    RaceSexMenu::Entry* pPresetSlider = pOptionListsA[2]->GetSliderWithID(24);
    if (xPresetHeadsA.IsEmpty()) {
        pPresetSlider->SetEnabled(false);
    }
    else {
        pPresetSlider->SetEnabled(true);
        pPresetSlider->SetRange(1, xPresetHeadsA.iSize);
    }
}

void RaceSexMenu::AddFaceSliders() {
    // Entry type of ShapeEntries and ToneEntries. eID is the list the control's slider goes in, RSM_LIST_COUNT for a
    // control that gets no slider.
    struct FaceSliderEntry {
        RaceSexMenu::ListID eID;
        const char* pDisplay;
        float fMin;
        float fMax;
    };
    // The symmetric geometry controls, by FaceGen control index.
    FaceSliderEntry ShapeEntries[55] = {
        {RaceSexMenu::RSM_SHAPE_BROW, sRSMShapeOption01.String(), fRSMFaceSliderDefaultMin, fRSMFaceSliderDefaultMax},
        {RaceSexMenu::RSM_SHAPE_BROW, sRSMShapeOption02.String(), fRSMFaceSliderDefaultMin, fRSMFaceSliderDefaultMax},
        {RaceSexMenu::RSM_SHAPE_BROW, sRSMShapeOption03.String(), fRSMFaceSliderDefaultMin, fRSMFaceSliderDefaultMax},
        {RaceSexMenu::RSM_SHAPE_CHEEKS, sRSMShapeOption04.String(), fRSMFaceSliderDefaultMin, fRSMFaceSliderDefaultMax},
        {RaceSexMenu::RSM_SHAPE_CHEEKS, sRSMShapeOption05.String(), fRSMFaceSliderDefaultMin, fRSMFaceSliderDefaultMax},
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
        {RaceSexMenu::RSM_SHAPE_CHEEKS, sRSMShapeOption07.String(), fRSMFaceSliderDefaultMin, fRSMFaceSliderDefaultMax},
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
        {RaceSexMenu::RSM_SHAPE_CHIN, sRSMShapeOption09.String(), fRSMFaceSliderDefaultMin, fRSMFaceSliderDefaultMax},
        {RaceSexMenu::RSM_SHAPE_CHIN, sRSMShapeOption10.String(), fRSMFaceSliderDefaultMin, fRSMFaceSliderDefaultMax},
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
        {RaceSexMenu::RSM_SHAPE_CHIN, sRSMShapeOption12.String(), fRSMFaceSliderDefaultMin, fRSMFaceSliderDefaultMax},
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
        {RaceSexMenu::RSM_SHAPE_CHIN, sRSMShapeOption14.String(), fRSMFaceSliderDefaultMin, fRSMFaceSliderDefaultMax},
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
        {RaceSexMenu::RSM_SHAPE_EYES, sRSMShapeOption16.String(), fRSMFaceSliderDefaultMin, fRSMFaceSliderDefaultMax},
        {RaceSexMenu::RSM_SHAPE_EYES, sRSMShapeOption13.String(), fRSMFaceSliderDefaultMin, fRSMFaceSliderDefaultMax},
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
        {RaceSexMenu::RSM_SHAPE_EYES, sRSMShapeOption19.String(), fRSMFaceSliderDefaultMin, fRSMFaceSliderDefaultMax},
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
        {RaceSexMenu::RSM_SHAPE_GENERAL, sRSMShapeOption22.String(), -2.0f, 2.0f},
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
        {RaceSexMenu::RSM_SHAPE_GENERAL, sRSMShapeOption06.String(), -2.0f, 2.0f},
        {RaceSexMenu::RSM_SHAPE_FOREHEAD,
         sRSMShapeOption13.String(),
         fRSMFaceSliderDefaultMin,
         fRSMFaceSliderDefaultMax},
        {RaceSexMenu::RSM_SHAPE_FOREHEAD,
         sRSMShapeOption14.String(),
         fRSMFaceSliderDefaultMin,
         fRSMFaceSliderDefaultMax},
        {RaceSexMenu::RSM_SHAPE_FOREHEAD,
         sRSMShapeOption27.String(),
         fRSMFaceSliderDefaultMin,
         fRSMFaceSliderDefaultMax},
        {RaceSexMenu::RSM_SHAPE_JAW, sRSMShapeOption11.String(), fRSMFaceSliderDefaultMin, fRSMFaceSliderDefaultMax},
        {RaceSexMenu::RSM_SHAPE_JAW, sRSMShapeOption15.String(), fRSMFaceSliderDefaultMin, fRSMFaceSliderDefaultMax},
        {RaceSexMenu::RSM_SHAPE_JAW, sRSMShapeOption30.String(), fRSMFaceSliderDefaultMin, fRSMFaceSliderDefaultMax},
        {RaceSexMenu::RSM_SHAPE_JAW, sRSMShapeOption07.String(), fRSMFaceSliderDefaultMin, fRSMFaceSliderDefaultMax},
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
        {RaceSexMenu::RSM_SHAPE_MOUTH, sRSMShapeOption36.String(), fRSMFaceSliderDefaultMin, fRSMFaceSliderDefaultMax},
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
        {RaceSexMenu::RSM_SHAPE_MOUTH, sRSMShapeOption40.String(), fRSMFaceSliderDefaultMin, fRSMFaceSliderDefaultMax},
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
        {RaceSexMenu::RSM_SHAPE_NOSE, sRSMShapeOption42.String(), fRSMFaceSliderDefaultMin, fRSMFaceSliderDefaultMax},
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
        {RaceSexMenu::RSM_SHAPE_NOSE, sRSMShapeOption16.String(), fRSMFaceSliderDefaultMin, fRSMFaceSliderDefaultMax},
        {RaceSexMenu::RSM_SHAPE_NOSE, sRSMShapeOption45.String(), fRSMFaceSliderDefaultMin, fRSMFaceSliderDefaultMax},
        {RaceSexMenu::RSM_SHAPE_NOSE, sRSMShapeOption46.String(), fRSMFaceSliderDefaultMin, fRSMFaceSliderDefaultMax},
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
        {RaceSexMenu::RSM_SHAPE_NOSE, sRSMShapeOption48.String(), fRSMFaceSliderDefaultMin, fRSMFaceSliderDefaultMax},
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
        {RaceSexMenu::RSM_SHAPE_NOSE, sRSMShapeOption50.String(), fRSMFaceSliderDefaultMin, 3.0f},
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
        {RaceSexMenu::RSM_SHAPE_NOSE, sRSMShapeOption54.String(), fRSMFaceSliderDefaultMin, 3.0f},
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
    };
    // The symmetric texture controls, by FaceGen control index.
    FaceSliderEntry ToneEntries[32] = {
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
        {RaceSexMenu::RSM_TONE, sBeard.String(), fRSMFaceSliderDefaultMin, fRSMFaceSliderDefaultMax},
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
        {RaceSexMenu::RSM_TONE, sRSMToneOption06.String(), fRSMFaceSliderDefaultMin, fRSMFaceSliderDefaultMax},
        {RaceSexMenu::RSM_TONE, sCheeks.String(), fRSMFaceSliderDefaultMin, fRSMFaceSliderDefaultMax},
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
        {RaceSexMenu::RSM_TONE, sRSMEyeSockets.String(), fRSMFaceSliderDefaultMin, fRSMFaceSliderDefaultMax},
        {RaceSexMenu::RSM_TONE, sEyebrows.String(), fRSMFaceSliderDefaultMin, fRSMFaceSliderDefaultMax},
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
        {RaceSexMenu::RSM_TONE, sEyeliner.String(), fRSMFaceSliderDefaultMin, fRSMFaceSliderDefaultMax},
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
        {RaceSexMenu::RSM_TONE, sLips.String(), fRSMFaceSliderDefaultMin, fRSMFaceSliderDefaultMax},
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
        {RaceSexMenu::RSM_LIST_COUNT, nullptr, 0.0f, 0.0f},
        {RaceSexMenu::RSM_TONE, sNose.String(), fRSMFaceSliderDefaultMin, fRSMFaceSliderDefaultMax},
        {RaceSexMenu::RSM_TONE, sRSMToneOption01.String(), fRSMFaceSliderDefaultMin, fRSMFaceSliderDefaultMax},
        {RaceSexMenu::RSM_TONE, sRSMToneOption24.String(), fRSMFaceSliderDefaultMin, fRSMFaceSliderDefaultMax},
        {RaceSexMenu::RSM_TONE, sRSMToneOption25.String(), fRSMFaceSliderDefaultMin, fRSMFaceSliderDefaultMax},
        {RaceSexMenu::RSM_TONE, sRSMToneOption26.String(), fRSMFaceSliderDefaultMin, fRSMFaceSliderDefaultMax},
    };
    FR2MatrixVTC<float> tempcoord[2][2];
    FR2MatrixVTC<float>(*pOffsetFaceCoord)[2][2] = pPlayerBase->GetOffsetFaceCoord();
    TESActorBase::SEX eSex = pPlayerBase->GetSex();
    FR2MatrixVTC<float>(*pMeanFaceCoord)[2][2] = pPlayerBase->pFormRace->GetMeanFaceCoord(eSex);
    BSFaceGenManager::MergeFaceGenCoord(pMeanFaceCoord, pOffsetFaceCoord, &tempcoord, false, 0.0f);
    for (int32_t i = 0;; ++i) {
        BSFaceGenManager::GetInstance();
        if (i >= BSFaceGenManager::GetMaxControlID(0, 0)) {
            break;
        }
        if (ShapeEntries[i].eID < RaceSexMenu::RSM_LIST_COUNT && ShapeEntries[i].pDisplay) {
            float fVal = BSFaceGenManager::GetFaceCoordValue(
                &tempcoord,
                FANTYPE_GEO,
                FANSYMM_SYMM,
                static_cast<uint32_t>(i)
            );
            fVal = BSMAX(BSMIN(fVal, ShapeEntries[i].fMax), ShapeEntries[i].fMin);
            RaceSexMenu::Entry* pOption = pOptionListsA[ShapeEntries[i].eID]->AddSliderOption(
                ShapeEntries[i].pDisplay,
                RSM_SHAPE_SLIDER,
                static_cast<int32_t>(ShapeEntries[i].fMin * 10.0),
                static_cast<int32_t>(ShapeEntries[i].fMax * 10.0),
                static_cast<int32_t>(fVal * 10.0),
                0.25f
            );
            pOption->SetDisplay(" ");
            pOption->SetData(i);
        }
    }
    // The texture sliders, one by one in the game's order, with no test of the entry.
    {
        float fVal = BSFaceGenManager::GetFaceCoordValue(&tempcoord, FANTYPE_TEX, FANSYMM_SYMM, 29u);
        fVal = BSMAX(BSMIN(fVal, ToneEntries[29].fMax), ToneEntries[29].fMin);
        RaceSexMenu::Entry* pOption = pOptionListsA[ToneEntries[29].eID]->AddSliderOption(
            ToneEntries[29].pDisplay,
            RSM_TONE_SLIDER,
            static_cast<int32_t>(ToneEntries[29].fMin * 10.0),
            static_cast<int32_t>(ToneEntries[29].fMax * 10.0),
            static_cast<int32_t>(fVal * 10.0),
            0.25f
        );
        pOption->SetDisplay(" ");
        pOption->SetData(29);
    }
    {
        float fVal = BSFaceGenManager::GetFaceCoordValue(&tempcoord, FANTYPE_TEX, FANSYMM_SYMM, 28u);
        fVal = BSMAX(BSMIN(fVal, ToneEntries[28].fMax), ToneEntries[28].fMin);
        RaceSexMenu::Entry* pOption = pOptionListsA[ToneEntries[28].eID]->AddSliderOption(
            ToneEntries[28].pDisplay,
            RSM_TONE_SLIDER,
            static_cast<int32_t>(ToneEntries[28].fMin * 10.0),
            static_cast<int32_t>(ToneEntries[28].fMax * 10.0),
            static_cast<int32_t>(fVal * 10.0),
            0.25f
        );
        pOption->SetDisplay(" ");
        pOption->SetData(28);
    }
    {
        float fVal = BSFaceGenManager::GetFaceCoordValue(&tempcoord, FANTYPE_TEX, FANSYMM_SYMM, 30u);
        fVal = BSMAX(BSMIN(fVal, ToneEntries[30].fMax), ToneEntries[30].fMin);
        RaceSexMenu::Entry* pOption = pOptionListsA[ToneEntries[30].eID]->AddSliderOption(
            ToneEntries[30].pDisplay,
            RSM_TONE_SLIDER,
            static_cast<int32_t>(ToneEntries[30].fMin * 10.0),
            static_cast<int32_t>(ToneEntries[30].fMax * 10.0),
            static_cast<int32_t>(fVal * 10.0),
            0.25f
        );
        pOption->SetDisplay(" ");
        pOption->SetData(30);
    }
    {
        float fVal = BSFaceGenManager::GetFaceCoordValue(&tempcoord, FANTYPE_TEX, FANSYMM_SYMM, 31u);
        fVal = BSMAX(BSMIN(fVal, ToneEntries[31].fMax), ToneEntries[31].fMin);
        RaceSexMenu::Entry* pOption = pOptionListsA[ToneEntries[31].eID]->AddSliderOption(
            ToneEntries[31].pDisplay,
            RSM_TONE_SLIDER,
            static_cast<int32_t>(ToneEntries[31].fMin * 10.0),
            static_cast<int32_t>(ToneEntries[31].fMax * 10.0),
            static_cast<int32_t>(fVal * 10.0),
            0.25f
        );
        pOption->SetDisplay(" ");
        pOption->SetData(31);
    }
    {
        float fVal = BSFaceGenManager::GetFaceCoordValue(&tempcoord, FANTYPE_TEX, FANSYMM_SYMM, 8u);
        fVal = BSMAX(BSMIN(fVal, ToneEntries[8].fMax), ToneEntries[8].fMin);
        RaceSexMenu::Entry* pOption = pOptionListsA[ToneEntries[8].eID]->AddSliderOption(
            ToneEntries[8].pDisplay,
            RSM_TONE_SLIDER,
            static_cast<int32_t>(ToneEntries[8].fMin * 10.0),
            static_cast<int32_t>(ToneEntries[8].fMax * 10.0),
            static_cast<int32_t>(fVal * 10.0),
            0.25f
        );
        pOption->SetDisplay(" ");
        pOption->SetData(8);
    }
    {
        float fVal = BSFaceGenManager::GetFaceCoordValue(&tempcoord, FANTYPE_TEX, FANSYMM_SYMM, 9u);
        fVal = BSMAX(BSMIN(fVal, ToneEntries[9].fMax), ToneEntries[9].fMin);
        RaceSexMenu::Entry* pOption = pOptionListsA[ToneEntries[9].eID]->AddSliderOption(
            ToneEntries[9].pDisplay,
            RSM_TONE_SLIDER,
            static_cast<int32_t>(ToneEntries[9].fMin * 10.0),
            static_cast<int32_t>(ToneEntries[9].fMax * 10.0),
            static_cast<int32_t>(fVal * 10.0),
            0.25f
        );
        pOption->SetDisplay(" ");
        pOption->SetData(9);
    }
    {
        float fVal = BSFaceGenManager::GetFaceCoordValue(&tempcoord, FANTYPE_TEX, FANSYMM_SYMM, 18u);
        fVal = BSMAX(BSMIN(fVal, ToneEntries[18].fMax), ToneEntries[18].fMin);
        RaceSexMenu::Entry* pOption = pOptionListsA[ToneEntries[18].eID]->AddSliderOption(
            ToneEntries[18].pDisplay,
            RSM_TONE_SLIDER,
            static_cast<int32_t>(ToneEntries[18].fMin * 10.0),
            static_cast<int32_t>(ToneEntries[18].fMax * 10.0),
            static_cast<int32_t>(fVal * 10.0),
            0.25f
        );
        pOption->SetDisplay(" ");
        pOption->SetData(18);
    }
    {
        float fVal = BSFaceGenManager::GetFaceCoordValue(&tempcoord, FANTYPE_TEX, FANSYMM_SYMM, 27u);
        fVal = BSMAX(BSMIN(fVal, ToneEntries[27].fMax), ToneEntries[27].fMin);
        RaceSexMenu::Entry* pOption = pOptionListsA[ToneEntries[27].eID]->AddSliderOption(
            ToneEntries[27].pDisplay,
            RSM_TONE_SLIDER,
            static_cast<int32_t>(ToneEntries[27].fMin * 10.0),
            static_cast<int32_t>(ToneEntries[27].fMax * 10.0),
            static_cast<int32_t>(fVal * 10.0),
            0.25f
        );
        pOption->SetDisplay(" ");
        pOption->SetData(27);
    }
    {
        float fVal = BSFaceGenManager::GetFaceCoordValue(&tempcoord, FANTYPE_TEX, FANSYMM_SYMM, 22u);
        fVal = BSMAX(BSMIN(fVal, ToneEntries[22].fMax), ToneEntries[22].fMin);
        RaceSexMenu::Entry* pOption = pOptionListsA[ToneEntries[22].eID]->AddSliderOption(
            ToneEntries[22].pDisplay,
            RSM_TONE_SLIDER,
            static_cast<int32_t>(ToneEntries[22].fMin * 10.0),
            static_cast<int32_t>(ToneEntries[22].fMax * 10.0),
            static_cast<int32_t>(fVal * 10.0),
            0.25f
        );
        pOption->SetDisplay(" ");
        pOption->SetData(22);
    }
    {
        float fVal = BSFaceGenManager::GetFaceCoordValue(&tempcoord, FANTYPE_TEX, FANSYMM_SYMM, 5u);
        fVal = BSMAX(BSMIN(fVal, ToneEntries[5].fMax), ToneEntries[5].fMin);
        RaceSexMenu::Entry* pOption = pOptionListsA[ToneEntries[5].eID]->AddSliderOption(
            ToneEntries[5].pDisplay,
            RSM_TONE_SLIDER,
            static_cast<int32_t>(ToneEntries[5].fMin * 10.0),
            static_cast<int32_t>(ToneEntries[5].fMax * 10.0),
            static_cast<int32_t>(fVal * 10.0),
            0.25f
        );
        pOption->SetDisplay(" ");
        pOption->SetData(5);
    }
    {
        float fVal = BSFaceGenManager::GetFaceCoordValue(&tempcoord, FANTYPE_TEX, FANSYMM_SYMM, 6u);
        fVal = BSMAX(BSMIN(fVal, ToneEntries[6].fMax), ToneEntries[6].fMin);
        RaceSexMenu::Entry* pOption = pOptionListsA[ToneEntries[6].eID]->AddSliderOption(
            ToneEntries[6].pDisplay,
            RSM_TONE_SLIDER,
            static_cast<int32_t>(ToneEntries[6].fMin * 10.0),
            static_cast<int32_t>(ToneEntries[6].fMax * 10.0),
            static_cast<int32_t>(fVal * 10.0),
            0.25f
        );
        pOption->SetDisplay(" ");
        pOption->SetData(6);
    }
    {
        float fVal = BSFaceGenManager::GetFaceCoordValue(&tempcoord, FANTYPE_TEX, FANSYMM_SYMM, 3u);
        fVal = BSMAX(BSMIN(fVal, ToneEntries[3].fMax), ToneEntries[3].fMin);
        RaceSexMenu::Entry* pOption = pOptionListsA[ToneEntries[3].eID]->AddSliderOption(
            ToneEntries[3].pDisplay,
            RSM_TONE_SLIDER,
            static_cast<int32_t>(ToneEntries[3].fMin * 10.0),
            static_cast<int32_t>(ToneEntries[3].fMax * 10.0),
            static_cast<int32_t>(fVal * 10.0),
            0.25f
        );
        pOption->SetDisplay(" ");
        pOption->SetData(3);
    }
    SynchronizeFaceSliders(RaceSexMenu::RSM_SHAPE_SLIDER);
    SynchronizeFaceSliders(RaceSexMenu::RSM_TONE_SLIDER);
    if (IsFlagged(RaceSexMenu::RSM_FLAG_UPDATE_HEAD)) {
        UpdatePlayerHead(true, true, false);
        SetFlag(RaceSexMenu::RSM_FLAG_UPDATE_HEAD, false);
    }
    if (IsFlagged(RaceSexMenu::RSM_FLAG_UPDATE_BODY)) {
        UpdatePlayerBody();
        SetFlag(RaceSexMenu::RSM_FLAG_UPDATE_BODY, false);
    }
}

bool RaceSexMenu::IsFlagged(Flag aeFlg) {
    return (uiFlags & (1 << aeFlg)) != 0;
}

void RaceSexMenu::SetFlag(Flag aeFlg, bool abOn) {
    uiFlags = abOn ? uiFlags | (1 << aeFlg) : uiFlags & ~(1 << aeFlg);
}

void RaceSexMenu::SynchronizeFaceSliders(RaceSexMenu::OptionID aeSliderType) {
    uint32_t uiPass = 0;
    if (aeSliderType == RaceSexMenu::RSM_SHAPE_SLIDER) {
        uint32_t uiPrevPass;
        do {
            if (!UpdateEntriesBetween(10, 19, FANTYPE_GEO)) {
                break;
            }
            uiPrevPass = uiPass++;
        }
        while (uiPrevPass < 3);
    }
    else if (aeSliderType == RaceSexMenu::RSM_TONE_SLIDER) {
        uint32_t uiPrevPass;
        do {
            if (!UpdateEntriesBetween(19, 20, FANTYPE_TEX)) {
                break;
            }
            uiPrevPass = uiPass++;
        }
        while (uiPrevPass < 3);
    }
}

void RaceSexMenu::UpdatePlayerHead(bool abChangeRace, bool abShape, bool abTone) {
    if (pPlayer->GetFaceNodeBiped(nullptr) && pPlayer->GetFaceNodeSkinned(nullptr)) {
        pPlayerBase->AddChange(CHANGE_NPC_FACE);
        if (abChangeRace) {
            BSFaceGenNiNode* pBiped;
            NiAVObject* pObj = pPlayer->Get3D(false);
            NiNode* pRoot = pObj ? pObj->IsNode() : nullptr;
            if (pRoot) {
                NiAVObjectPalette* pTmpPal = nullptr;
                if (pPlayer->GetAnimation(false)) {
                    if (pPlayer->GetAnimation(false)->GetControllerManager()) {
                        pTmpPal = pPlayer->GetAnimation(false)->GetControllerManager()->GetObjectPalette();
                    }
                }
                NiDefaultAVObjectPalette* pPal = static_cast<NiDefaultAVObjectPalette*>(pTmpPal);
                NiAVObject* pRemove = pPlayer->GetFaceNodeSkinned(nullptr);
                if (pRemove && pRemove->m_pkParent) {
                    RecurseAndRemoveObjectsFromPalette(pRemove, pPal);
                    pPlayer->pCurrentProcess->SetFaceSkinnedNode(nullptr);
                    pRemove->m_pkParent->DetachChild(pRemove);
                }

                pRemove = pPlayer->GetFaceNodeBiped(nullptr);
                if (pRemove && pRemove->m_pkParent) {
                    RecurseAndRemoveObjectsFromPalette(pRemove, pPal);
                    pPlayer->pCurrentProcess->SetFaceNode(nullptr);
                    pPlayer->pCurrentProcess->SetHeadAnims(nullptr);
                    pRemove->m_pkParent->DetachChild(pRemove);
                }
            }
            if (pPlayer->pCurrentProcess) {
                pPlayer->pCurrentProcess->ClearFaceAnimationData();
            }
            pPlayerBase->ClearHead();
            BSFaceGenNiNode* pSkin = nullptr;
            pBiped = nullptr;
            pPlayerBase->InitHead(pBiped, pSkin);
            BSFaceGenNiNode* pNewSkin = pSkin;
            BSFaceGenNiNode* pNewBiped = pBiped;
            BipedAnim* pBipedAnim = pPlayer->GetBiped(false);
            pPlayerBase->AttachHead(pPlayer, pBipedAnim, pNewBiped, pNewSkin);
        }
        else {
            FR2MatrixVTC<float> kCurrCord[2][2];
            FaceGenDatas kDiffCoord;
            pPlayerBase->GetFaceCoord(kCurrCord);
            if (abShape) {
                BSFaceGenManager::OffsetFaceGenCoord(&LastFaceCoord, &kCurrCord, &kDiffCoord.kFaceGens, false);
            }
            BSFaceGenNiNode* pFaceNode = pPlayer->GetFaceNodeBiped(nullptr);
            for (int32_t i = 0; i < 2; ++i) {
                if (abShape) {
                    pFaceNode->SetForceMorphUpdates(true);
                }
                uint32_t uiSizeArray = pFaceNode->GetArrayCount();
                for (uint32_t uiChild = 0; uiChild < uiSizeArray; ++uiChild) {
                    NiAVObject* pObj = pFaceNode->GetAt(uiChild);
                    NiTriBasedGeom* pTri = pObj ? pObj->IsTriBasedGeom() : nullptr;
                    if (pTri) {
                        BSFaceGenModelExtraData* pModelExtraData = BSFaceGenManager::GetFaceGenModelExtraData(pTri);
                        BSFaceGenModel* pFGModel = pModelExtraData ? pModelExtraData->GetModel() : nullptr;
                        if (pFGModel) {
                            if (abShape) {
                                pFGModel->ApplyCoordinateToExistingMesh(&kDiffCoord.kFaceGens, pTri, false);
                            }
                            if (abTone) {
                                uint32_t uiLength = std::strlen("FaceGenFace");
                                const char* pName = pTri->m_kName.GetHandleString();
                                if (!BSstrnicmp(pName, "FaceGenFace", uiLength)) {
                                    int32_t iPPLighting;
                                    NiShadeProperty* pShaderProp = static_cast<NiShadeProperty*>(
                                        pObj->GetProperty(NiProperty::SHADE)
                                    );
                                    if (pShaderProp) {
                                        iPPLighting = pShaderProp->iShaderPropertyType >= 8 &&
                                            pShaderProp->iShaderPropertyType <= 12;
                                    }
                                    else {
                                        iPPLighting = 0;
                                    }
                                    BSShaderPPLightingProperty* pShaderPropLighting = nullptr;
                                    if (iPPLighting) {
                                        pShaderPropLighting = static_cast<BSShaderPPLightingProperty*>(pShaderProp);
                                    }
                                    if (pShaderPropLighting) {
                                        NiPointer<NiTexture> spTexBaseMod(static_cast<NiTexture*>(nullptr));
                                        if (!pFGModel->HasEGTData()) {
                                            pFGModel->ForceLoadModelTexture();
                                        }
                                        pFGModel->ApplyCoordinateTexturingToMesh(
                                            &kCurrCord,
                                            spTexBaseMod,
                                            BSFaceGenManager::GetEGTClampSize()
                                        );
                                        pShaderPropLighting->SetDiffuseTexture(1u, spTexBaseMod.m_pObject);
                                    }
                                }
                            }
                        }
                    }
                }
                pFaceNode = pPlayer->GetFaceNodeSkinned(nullptr);
            }
        }
        NiUpdatePlayer3D();
    }
}

void RaceSexMenu::UpdatePlayerBody() {
    NiMatrix3 kRotIn;
    BSFaceGenManager::ClearBodyTexturesFromPalette(static_cast<TESNPC*>(pPlayer->GetObjectReference()));
    bBipedAnimUpdateSkins = true;
    pPlayer->pCurrentProcess->ClearFaceAnimationData();
    pPlayer->pCurrentProcess->Set3DUpdateFlag(static_cast<RESET_3D_FLAGS>(RESET_MODEL | RESET_SKIN));
    pPlayer->pCurrentProcess->Update3dModel(pPlayer);
    if (!pPlayer->GetFaceNodeSkinned(nullptr) && !pPlayer->GetFaceNodeBiped(nullptr)) {
        QueuedHead kQueuedHead(pPlayerBase, IO_TASK_PRIORITY_CRITICAL);
        kQueuedHead.QueuedHead::Run();
        kQueuedHead.QueuedFile::Finish();
        BipedAnim* pBiped = pPlayer->GetBiped(false);
        kQueuedHead.Attach(pPlayer, pBiped);
    }
    bBipedAnimUpdateSkins = false;
    NiNode* p3D = static_cast<NiNode*>(pPlayer->Get3D(true));
    NiAVObject* pChild = p3D->GetAt(0);
    kRotIn = NiMatrix3::IDENTITY;
    pChild->SetRotate(pPlayer->MultipleMatrixByRace(kRotIn));
    pIOManager->LoadAllQueuedPriorities();
    NiUpdatePlayer3D();
    Interface::GetPipboy()->Shutdown();
    Interface::GetPipboy()->Initialize();
}

void RaceSexMenu::NiUpdatePlayer3D() {
    NiNode* p3D = static_cast<NiNode*>(pPlayer->Get3D(false));
    BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_MENU_PLAYER_3D)->UpdateObjectLighting(p3D, false);
    NiUpdateData kUpdateData(0.0, false, false);
    BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_MENU_PLAYER_3D)->NiAVObject::Update(kUpdateData);
    if (InterfaceManager::Instance()->GetCurrentRenderedMenu()) {
        FORenderedMenuRaceSex* pRenderedMenu = static_cast<FORenderedMenuRaceSex*>(
            InterfaceManager::Instance()->GetCurrentRenderedMenu()
        );
        pRenderedMenu->SetupLights(1.0);
    }
}

// A null NiPointer that is never used is also constructed and destroyed; both pointers are built and
// released on raw stack storage. The five empty strings are unused too, but are built and released.
// The FaceGen data and the colours are constructed in raw stack storage.
bool RaceSexMenu::UpdatePlayerHair(bool abSwapHairType) {
    alignas(NiPointer<NiTriBasedGeom>) uint8_t aUnusedStorage[sizeof(NiPointer<NiTriBasedGeom>)];
    NiPointer<NiTriBasedGeom>* pspUnused = ::new (aUnusedStorage) NiPointer<NiTriBasedGeom>(nullptr);
    alignas(NiPointer<NiTriBasedGeom>) uint8_t aGeomStorage[sizeof(NiPointer<NiTriBasedGeom>)];
    NiPointer<NiTriBasedGeom>& spGeom = *::new (aGeomStorage) NiPointer<NiTriBasedGeom>(nullptr);
    alignas(BSStringT<char>) uint8_t aString1[sizeof(BSStringT<char>)];
    alignas(BSStringT<char>) uint8_t aString2[sizeof(BSStringT<char>)];
    alignas(BSStringT<char>) uint8_t aString3[sizeof(BSStringT<char>)];
    alignas(BSStringT<char>) uint8_t aString4[sizeof(BSStringT<char>)];
    alignas(BSStringT<char>) uint8_t aString5[sizeof(BSStringT<char>)];
    BSStringT<char>& kString1 = *::new (aString1) BSStringT<char>;
    BSStringT<char>& kString2 = *::new (aString2) BSStringT<char>;
    BSStringT<char>& kString3 = *::new (aString3) BSStringT<char>;
    BSStringT<char>& kString4 = *::new (aString4) BSStringT<char>;
    BSStringT<char>& kString5 = *::new (aString5) BSStringT<char>;
    if (!pPlayer->GetFaceNodeBiped(nullptr)) {
        kString5.~BSStringT<char>();
        kString4.~BSStringT<char>();
        kString3.~BSStringT<char>();
        kString2.~BSStringT<char>();
        kString1.~BSStringT<char>();
        spGeom.~NiPointer<NiTriBasedGeom>();
        pspUnused->~NiPointer<NiTriBasedGeom>();
        return false;
    }
    pPlayerBase->AddChange(CHANGE_NPC_FACE);
    char acHairName[32];
    if (abSwapHairType) {
        alignas(FaceGenNpcData) char acNpcData[sizeof(FaceGenNpcData)];
        FaceGenNpcData* pNpcData = ::new (acNpcData) FaceGenNpcData;
        int32_t iSex = pPlayer->GetSex();
        TESBoundObject* pBase = pPlayer->GetObjectReference();
        pPlayer->GetRace()
            ->GetFaceGenData(static_cast<TESNPC*>(pBase), *pNpcData, static_cast<TESActorBase::SEX>(iSex), false);
        BSFaceGenNiNode* pBiped = pPlayer->GetFaceNodeBiped(nullptr);
        BSFaceGenNiNode* pSkinned = pPlayer->GetFaceNodeSkinned(nullptr);
        // Detach the hair models the head has now.
        for (int32_t i = HAIR_PART_NORMAL; i < HAIR_PART_COUNT; ++i) {
            NiFixedString kName(BSFaceGenManager::GetHairName(acHairName, static_cast<HAIR_PART>(i)));
            NiAVObject* pHair = pBiped->GetObjectByName(kName);
            if (pHair) {
                pBiped->DetachChild(pHair);
            }
        }
        BSFaceGenManager::AttachHairToHead(pBiped, pSkinned, *pNpcData, false);
        NiAVObjectPalette* pPalette = pPlayer->GetAnimation(false)->GetControllerManager()->GetObjectPalette();
        if (pPalette) {
            auto* pDefault = static_cast<NiDefaultAVObjectPalette*>(
                NiObject::DynamicCastStatic(&NiDefaultAVObjectPalette::ms_RTTI, pPalette)
            );
            if (pDefault) {
                pDefault->ResetAndFillFromScenegraph();
            }
        }
        static_cast<TESNPC*>(pPlayer->GetObjectReference())->FixDisplayedHeadParts(pPlayer, nullptr);
        // Prepare and tint the new hair models.
        for (int32_t i = HAIR_PART_NORMAL; i < HAIR_PART_COUNT; ++i) {
            NiFixedString kName(BSFaceGenManager::GetHairName(acHairName, static_cast<HAIR_PART>(i)));
            NiAVObject* pHair = pBiped->GetObjectByName(kName);
            if (!pHair) {
                continue;
            }
            BSShaderManager::PrepareObject(pHair, 0, 0);
            NiProperty* pProperty = pHair->GetProperty(NiProperty::SHADE);
            bool bPPLighting = pProperty &&
                static_cast<NiShadeProperty*>(pProperty)->GetShaderPropertyType() ==
                    BSShaderManager::BSSM_PROPERTYTYPE_PPLIGHTING;
            auto* pPPLighting = static_cast<BSShaderPPLightingProperty*>(bPPLighting ? pProperty : nullptr);
            // Reads the hair colour before it knows whether a property takes it.
            uint32_t uiColor = pPlayerBase->GetHairColor();
            if (pPPLighting) {
                // Each channel is divided as a double and stored as a float.
                float fB = static_cast<float>(static_cast<int32_t>((uiColor >> 16) & 0xFF) / 255.0);
                float fG = static_cast<float>(
                    static_cast<int32_t>((static_cast<uint16_t>(uiColor) >> 8) & 0xFF) / 255.0
                );
                float fR = static_cast<float>(static_cast<int32_t>(uiColor & 0xFF) / 255.0);
                alignas(NiColorA) char acColor[sizeof(NiColorA)];
                pPPLighting->pHairTintColor = *::new (acColor) NiColorA(fR, fG, fB, 1.0f);
            }
            else {
                bool bLighting30 = pProperty &&
                    static_cast<NiShadeProperty*>(pProperty)->GetShaderPropertyType() ==
                        BSShaderManager::BSSM_PROPERTYTYPE_LIGHTING30;
                auto* pLighting30 = static_cast<Lighting30ShaderProperty*>(bLighting30 ? pProperty : nullptr);
                if (pLighting30) {
                    float fB = static_cast<float>(static_cast<int32_t>((uiColor >> 16) & 0xFF) / 255.0);
                    float fG = static_cast<float>(
                        static_cast<int32_t>((static_cast<uint16_t>(uiColor) >> 8) & 0xFF) / 255.0
                    );
                    float fR = static_cast<float>(static_cast<int32_t>(uiColor & 0xFF) / 255.0);
                    alignas(NiColorA) char acColor[sizeof(NiColorA)];
                    pLighting30->pHairTintColor = *::new (acColor) NiColorA(fR, fG, fB, 1.0f);
                }
            }
        }
        pBiped->UpdateProperties();
        pNpcData->~FaceGenNpcData();
    }
    else {
        // Re-morph and re-tint the hair and accessory geometry under the head.
        uint32_t uiCount = pPlayer->GetFaceNodeBiped(nullptr)->GetArrayCount();
        for (uint32_t i = 0; i < uiCount; ++i) {
            NiAVObject* pChild = pPlayer->GetFaceNodeBiped(nullptr)->GetAt(i);
            uint8_t bHair = 0;
            if (pChild) {
                uint32_t uiLength = BSstrlen("FaceGenHair");
                if (!BSstrnicmp(pChild->GetName().GetHandleString(), "FaceGenHair", uiLength)) {
                    bHair = 1;
                }
            }
            uint8_t bAccessory = 0;
            if (!bHair && pChild) {
                uint32_t uiLength = BSstrlen("FaceGenAccessory");
                if (!BSstrnicmp(pChild->GetName().GetHandleString(), "FaceGenAccessory", uiLength)) {
                    bAccessory = 1;
                }
            }
            if (!bHair && !bAccessory) {
                continue;
            }
            spGeom = pChild->IsTriBasedGeom();
            NiTriBasedGeom* pGeom;
            pGeom = spGeom;
            if (!pGeom) {
                continue;
            }
            if (bHair) {
                float fLength = pPlayerBase->GetHairLength();
                pGeom = spGeom;
                BSFaceGenManager::ApplyHairMorph(pGeom, fLength);
            }
            pGeom = spGeom;
            NiProperty* pProperty = pGeom->GetProperty(NiProperty::SHADE);
            bool bPPLighting = pProperty &&
                static_cast<NiShadeProperty*>(pProperty)->GetShaderPropertyType() ==
                    BSShaderManager::BSSM_PROPERTYTYPE_PPLIGHTING;
            auto* pPPLighting = static_cast<BSShaderPPLightingProperty*>(bPPLighting ? pProperty : nullptr);
            if (pPPLighting) {
                uint32_t uiColor = pPlayerBase->GetHairColor();
                float fB = static_cast<float>(static_cast<int32_t>((uiColor >> 16) & 0xFF) / 255.0);
                float fG = static_cast<float>(
                    static_cast<int32_t>((static_cast<uint16_t>(uiColor) >> 8) & 0xFF) / 255.0
                );
                float fR = static_cast<float>(static_cast<int32_t>(uiColor & 0xFF) / 255.0);
                alignas(NiColorA) char acColor[sizeof(NiColorA)];
                pPPLighting->pHairTintColor = *::new (acColor) NiColorA(fR, fG, fB, 1.0f);
            }
            else {
                bool bLighting30 = pProperty &&
                    static_cast<NiShadeProperty*>(pProperty)->GetShaderPropertyType() ==
                        BSShaderManager::BSSM_PROPERTYTYPE_LIGHTING30;
                auto* pLighting30 = static_cast<Lighting30ShaderProperty*>(bLighting30 ? pProperty : nullptr);
                if (pLighting30) {
                    uint32_t uiColor = pPlayerBase->GetHairColor();
                    float fB = static_cast<float>(static_cast<int32_t>((uiColor >> 16) & 0xFF) / 255.0);
                    float fG = static_cast<float>(
                        static_cast<int32_t>((static_cast<uint16_t>(uiColor) >> 8) & 0xFF) / 255.0
                    );
                    float fR = static_cast<float>(static_cast<int32_t>(uiColor & 0xFF) / 255.0);
                    alignas(NiColorA) char acColor[sizeof(NiColorA)];
                    pLighting30->pHairTintColor = *::new (acColor) NiColorA(fR, fG, fB, 1.0f);
                }
            }
        }
    }
    NiUpdateData kData(0.0f, false, false);
    pPlayer->GetFaceNodeBiped(nullptr)->Update(kData);
    NiUpdatePlayer3D();
    kString5.~BSStringT<char>();
    kString4.~BSStringT<char>();
    kString3.~BSStringT<char>();
    kString2.~BSStringT<char>();
    kString1.~BSStringT<char>();
    spGeom.~NiPointer<NiTriBasedGeom>();
    pspUnused->~NiPointer<NiTriBasedGeom>();
    return true;
}

void RaceSexMenu::MakeEyeColorList() {
    pOptionListsA[8]->Flush();
    for (BSSimpleList<TESEyes*>* pIter = pDataHandler->GetEyesList(); pIter && pIter->GetItem();
         pIter = pIter->GetNext()) {
        if (pIter->GetItem()->IsPlayable()) {
            TESNPC* pNPC = pPlayerBase;
            if (pIter->GetItem()->CanBeEyesForNPC(pNPC)) {
                TESEyes* pEyes = pIter->GetItem();
                TESEyes* pNameEyes = pIter->GetItem();
                const char* pName = static_cast<TESFullName&>(*pNameEyes).GetFullName();
                pOptionListsA[8]->AddBasicOption(pName, 33)->SetData(pEyes);
            }
        }
    }
    pOptionListsA[8]->UpdateCurrentSelection();
    SetFlag(RSM_FLAG_MAKE_NEW_EYESLIST, false);
}

bool RaceSexMenu::RotatePlayer(float afRadians, bool abForce) {
    NiMatrix3 kProduct;
    NiMatrix3 kScale;
    float fXAngle;
    float fZAngle;
    float fYAngle;
    NiMatrix3 kRot;
    std::memcpy(&kRot, &spPlayerNode->GetLocalTransform(), sizeof(kRot));
    kRot.ToEulerAnglesXYZ(fXAngle, fYAngle, fZAngle);
    float fNewZAngle = fZAngle - afRadians;
    if (!(fNewZAngle < 0.0)) {
        if (fNewZAngle > 6.2831854820251465) {
            fNewZAngle = static_cast<float>(fNewZAngle - 6.2831854820251465);
        }
    }
    else {
        fNewZAngle = static_cast<float>(fNewZAngle + 6.2831854820251465);
    }
    if (fNewZAngle == fZAngle && !abForce) {
        return false;
    }
    kRot.MakeZRotation(fNewZAngle);
    std::memcpy(&kScale, &NiMatrix3::ZERO, sizeof(kScale));
    float fWeight = pPlayerBase->GetWeight();
    kScale.SetEntry(0, 0, fWeight);
    fWeight = pPlayerBase->GetWeight();
    kScale.SetEntry(1u, 1u, fWeight);
    float fHeight = pPlayerBase->GetHeight();
    kScale.SetEntry(2u, 2u, fHeight);
    std::memcpy(&kRot, &(kProduct = kRot * kScale), sizeof(kRot));
    spPlayerNode->SetRotate(kRot);
    NiUpdatePlayer3D();
    return true;
}

bool RaceSexMenu::ZoomPlayer(float afDelta, bool abForce) {
    float fPrevZoomPct = this->fZoomPct;
    float fNewZoomPct = fPrevZoomPct + afDelta;
    float fClamped = BSMIN(static_cast<float>(1.0), fNewZoomPct);
    this->fZoomPct = BSMAX(static_cast<float>(0.0), fClamped);
    if (this->fZoomPct == fPrevZoomPct && !abForce) {
        return false;
    }
    float fZ = pPlayer->GetHeight() * ((ptZoomBound[1].z - ptZoomBound[0].z) * this->fZoomPct + ptZoomBound[0].z);
    float fY = (ptZoomBound[1].y - ptZoomBound[0].y) * this->fZoomPct + ptZoomBound[0].y;
    float fX = (ptZoomBound[1].x - ptZoomBound[0].x) * this->fZoomPct + ptZoomBound[0].x;
    spPlayerNode->SetTranslate(fX, fY, fZ);
    NiUpdatePlayer3D();
    return true;
}

void RaceSexMenu::InitHairStyles() {
    pOptionListsA[5]->Flush();
    RaceSexMenu::Entry* pPlayerHair = nullptr;
    for (TESHair*& rpItem : pDataHandler->listHair.ItemsUntilNull()) {
        if (rpItem->IsPlayable() && rpItem->CanBeHairForNPC(pPlayerBase)) {
            RaceSexMenu::Entry* pOption = pOptionListsA[5]->AddBasicOption(rpItem->GetFullName(), 27);
            pOption->data = rpItem;
            if (!pPlayerHair && RaceSexMenu::IsPlayerHair(pOption)) {
                pPlayerHair = pOption;
            }
        }
    }
    UpdateScrollArrows();
    while (pPlayerHair && pPlayerHair != pOptionListsA[5]->selectedEntry && pOptionListsA[5]->HandleKey(2)) {
        ;
    }
    pOptionListsA[5]->UpdateCurrentSelection();
    SetFlag(RaceSexMenu::RSM_FLAG_MAKE_NEW_HAIRLIST, false);
}

void RaceSexMenu::ValidateHairOptionList() {
    bool bHasFacialHair = pOptionListsA[3]->GetSliderWithID(7) != nullptr;
    TESActorBase::SEX eSex = pPlayerBase->GetSex();
    bool bMale = eSex == TESActorBase::SEX_MALE;
    if (eSex || bHasFacialHair) {
        if (bHasFacialHair && !bMale) {
            pOptionListsA[3]->RemoveByID(7u);
        }
    }
    else {
        BSStringT<char> strtmp;
        strtmp.SPrintF("%s %c", sRSMFacialHair.String(), 62);
        pOptionListsA[3]->InsertMenuOption(2u, strtmp.pString, "RSM_list_item_template", 7);
    }
}

void RaceSexMenu::UpdateScrollArrows() {
    List* pList = pOptionListsA[GetActiveList()];
    NiTListItem<RaceSexMenu::Entry*>* pos = pList->items.GetHeadPos();
    bool bUpVis = false;
    bool bDownVis = false;
    Tile::UpdateAll(false);
    while (pos && (!bUpVis || !bDownVis)) {
        RaceSexMenu::Entry* pOption = pList->items.GetNext(pos);
        int32_t iValue = pOption->GetValue();
        if (iValue == -1) {
            bUpVis = true;
        }
        else if (iValue == 1) {
            bDownVis = true;
        }
    }
    rsmScrollUpTarget->Set(Tile::kTileValue_Visible, static_cast<int32_t>(bUpVis));
    rsmScrollDownTarget->Set(Tile::kTileValue_Visible, static_cast<int32_t>(bDownVis));
}

void RaceSexMenu::OnFaceSliderChange(RaceSexMenu::Entry* apChangedSlider) {
    FR2MatrixVTC<float> akFaceCoord[2][2];
    pPlayerBase->GetFaceCoord(akFaceCoord);
    float fFaceValue = static_cast<double>(apChangedSlider->GetSliderValue()) / 10.0;
    // A face slider's data pointer holds its FaceGen control ID, cast here to uint32_t.
    uint32_t uiCtrlID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(apChangedSlider->data));
    uint32_t uiID = apChangedSlider->id;
    BSFaceGenManager::SetFaceCoordValue(
        &akFaceCoord,
        static_cast<FanTypeE>(uiID != static_cast<uint32_t>(RSM_SHAPE_SLIDER)),
        FANSYMM_SYMM,
        uiCtrlID,
        fFaceValue
    );
    pPlayerBase->GetFaceCoord(LastFaceCoord);
    FR2MatrixVTC<float>(*pOffsetFaceCoord)[2][2] = pPlayerBase->GetOffsetFaceCoord();
    TESActorBase::SEX eSex = pPlayerBase->GetSex();
    FR2MatrixVTC<float>(*pMeanFaceCoord)[2][2] = pPlayerBase->pFormRace->GetMeanFaceCoord(eSex);
    BSFaceGenManager::OffsetFaceGenCoord(pMeanFaceCoord, &akFaceCoord, pOffsetFaceCoord, false);
    SetFlag(RaceSexMenu::RSM_FLAG_UPDATE_HEAD, true);
    if (apChangedSlider->id == static_cast<uint32_t>(RSM_TONE_SLIDER)) {
        SetFlag(RaceSexMenu::RSM_FLAG_UPDATE_BODY, true);
    }
}

void RaceSexMenu::OnHairColorSliderChange(RaceSexMenu::Entry* apChangedSlider) {
    Setting* pLabel;
    Setting* pColor;
    if (apChangedSlider) {
        if (apChangedSlider->id == 29) {
            pLabel = nullptr;
            pColor = nullptr;
            BSStringT<char> strGameSetting;
            strGameSetting.SPrintF("iHairColor%02d", apChangedSlider->GetSliderValue());
            char* pName = strGameSetting.pString;
            SettingT<GameSettingCollection>::GetCollection()->GetSetting(pName, &pColor);
            strGameSetting.SPrintF("sHairColor%d", apChangedSlider->GetSliderValue());
            pName = strGameSetting.pString;
            SettingT<GameSettingCollection>::GetCollection()->GetSetting(pName, &pLabel);
            apChangedSlider->SetDisplay(pLabel->String());
            // The setting's int is the packed colour SetHairColorSliders takes, passed unchanged.
            SetHairColorSliders(static_cast<uint32_t>(pColor->Int()));
        }
        else {
            const char* pCustom = sRSMCustom.String();
            RaceSexMenu::Entry* pPresetSlider = pOptionListsA[6]->GetSliderWithID(29);
            pPresetSlider->SetDisplay(pCustom);
        }
        ChangeHairColor();
    }
}

void RaceSexMenu::SetHairColorSliders(uint32_t aiColor) {
    NiTListItem<RaceSexMenu::Entry*>* pIter = pOptionListsA[6]->items.m_pkHead;
    while (pIter) {
        RaceSexMenu::Entry* pOption = pOptionListsA[6]->items.GetNext(pIter);
        switch (pOption->id) {
            case 30u: {
                pOption->SetValue(static_cast<uint8_t>(aiColor));
                break;
            }
            case 31u: {
                pOption->SetValue(static_cast<uint8_t>(aiColor >> 8));
                break;
            }
            case 32u: {
                pOption->SetValue(static_cast<uint8_t>(aiColor >> 16));
                break;
            }
        }
    }
}

char RaceSexMenu::UpdateEntriesBetween(int32_t aiStart, int32_t aiEnd, FanTypeE aeType) {
    char bChanged = 0;
    for (int32_t i = aiStart; i < aiEnd; ++i) {
        NiTListItem<RaceSexMenu::Entry*>* pIter = pOptionListsA[i]->items.m_pkHead;
        while (pIter) {
            RaceSexMenu::Entry* pOption = pOptionListsA[i]->items.GetNext(pIter);
            if (pOption && (pOption->id == 34 || pOption->id == 35)) {
                int32_t iLow;
                int32_t iHigh;
                FR2MatrixVTC<float> akCoord[2][2];
                FR2MatrixVTC<float>(*pOffsetFaceCoord)[2][2] = pPlayerBase->GetOffsetFaceCoord();
                TESActorBase::SEX eSex = pPlayerBase->GetSex();
                FR2MatrixVTC<float>(*pMeanFaceCoord)[2][2] = pPlayerBase->pFormRace->GetMeanFaceCoord(eSex);
                BSFaceGenManager::MergeFaceGenCoord(pMeanFaceCoord, pOffsetFaceCoord, &akCoord, false, 0.0f);
                // The entry's data is the FaceGen control index.
                int32_t iVal = static_cast<int32_t>(
                    BSFaceGenManager::GetFaceCoordValue(
                        &akCoord,
                        aeType,
                        FANSYMM_SYMM,
                        static_cast<uint32_t>(pOption->iData)
                    ) *
                    10.0
                );
                pOption->GetRange(iLow, iHigh);
                if (iVal >= iLow) {
                    if (iVal <= iHigh) {
                        if (pOption != pSliderOpt) {
                            pOption->SetValue(iVal);
                        }
                    }
                    else {
                        pOption->SetValue(iHigh);
                        bChanged = 1;
                    }
                }
                else {
                    pOption->SetValue(iLow);
                    bChanged = 1;
                }
            }
        }
    }
    return bChanged;
}

void RaceSexMenu::ValidateHairAndEyes() {
    TESNPC* pPlayerBase = this->pPlayerBase;
    TESHair* pHair = pPlayerBase->GetHair();
    if (!pHair->CanBeHairForNPC(pPlayerBase)) {
        TESActorBase::SEX eSex = this->pPlayerBase->GetSex();
        TESRace* pRace = this->pPlayerBase->GetRace();
        TESHair* pDefaultHair = pRace->GetDefaultHair(eSex);
        for (BSSimpleList<TESHair*>* pNode = &this->pPlayerBase->GetRace()->HairList;
             !pDefaultHair && pNode && pNode->m_item;
             pNode = pNode->m_pkNext) {
            if (pNode->m_item->IsPlayable()) {
                if ((this->pPlayerBase->GetSex() == TESActorBase::SEX_MALE && pNode->m_item->CanBeMale()) ||
                    (this->pPlayerBase->GetSex() == TESActorBase::SEX_FEMALE && pNode->m_item->CanBeFemale())) {
                    pDefaultHair = pNode->m_item;
                }
            }
        }
        this->pPlayerBase->SetHair(pDefaultHair);
        UpdatePlayerHair(true);
    }
    if (!this->pPlayerBase->GetEyeColor()->CanBeEyesForNPC(this->pPlayerBase)) {
        TESEyes* pDefaultEyes = this->pPlayerBase->GetRace()->GetDefaultEyeColor();
        this->pPlayerBase->SetEyeColor(pDefaultEyes);
        UpdatePlayerEyes();
    }
    SetFlag(RaceSexMenu::RSM_FLAG_MAKE_NEW_HAIRLIST, true);
    SetFlag(RaceSexMenu::RSM_FLAG_MAKE_NEW_EYESLIST, true);
}

bool RaceSexMenu::UpdatePlayerEyes() {
    NiPointer<NiTriBasedGeom> spLeftEye(static_cast<NiTriBasedGeom*>(nullptr));
    NiPointer<NiTriBasedGeom> spRightEye(static_cast<NiTriBasedGeom*>(nullptr));
    if (pPlayer->GetFaceNodeBiped(nullptr)) {
        pPlayerBase->AddChange(CHANGE_NPC_FACE);
        uint32_t uiSizeArray = pPlayer->GetFaceNodeBiped(nullptr)->GetArrayCount();
        for (uint32_t uiChild = 0; uiChild < uiSizeArray; ++uiChild) {
            NiAVObject* pObj = pPlayer->GetFaceNodeBiped(nullptr)->GetAt(uiChild);
            if (!BSstrcmp(pObj->m_kName.GetHandleString(), "FaceGenEyeLeft")) {
                spLeftEye = pObj->IsTriBasedGeom();
            }
            if (!BSstrcmp(pObj->m_kName.GetHandleString(), "FaceGenEyeRight")) {
                spRightEye = pObj->IsTriBasedGeom();
            }
        }
        if (spLeftEye.m_pObject && spRightEye.m_pObject) {
            BSStringT<char> strFileName;
            if (pPlayerBase->GetEyeColor()) {
                strFileName.SPrintF("Textures\\%s", pPlayerBase->GetEyeColor()->GetTextureName());
            }
            else {
                strFileName.SPrintF("Textures\\Characters\\Eyes\\EyeDefault.dds");
            }
            NiPointer<NiSourceTexture> spTexBase(static_cast<NiSourceTexture*>(nullptr));
            TES::pInstance->CreateTextureImage(strFileName.pString, spTexBase, false, false);
            if (spTexBase.m_pObject) {
                int32_t iPPLighting;
                NiShadeProperty* pProp = static_cast<NiShadeProperty*>(spLeftEye->GetProperty(NiProperty::SHADE));
                if (pProp) {
                    iPPLighting = pProp->iShaderPropertyType >= 8 && pProp->iShaderPropertyType <= 12;
                }
                else {
                    iPPLighting = 0;
                }
                BSShaderPPLightingProperty* pLighting = nullptr;
                if (iPPLighting) {
                    pLighting = static_cast<BSShaderPPLightingProperty*>(pProp);
                }
                if (pLighting) {
                    pLighting->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, spTexBase.m_pObject);
                }
                pProp = static_cast<NiShadeProperty*>(spRightEye->GetProperty(NiProperty::SHADE));
                if (pProp) {
                    iPPLighting = pProp->iShaderPropertyType >= 8 && pProp->iShaderPropertyType <= 12;
                }
                else {
                    iPPLighting = 0;
                }
                pLighting = iPPLighting ? static_cast<BSShaderPPLightingProperty*>(pProp) : nullptr;
                if (pLighting) {
                    pLighting->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, spTexBase.m_pObject);
                }
            }
        }

        NiUpdatePlayer3D();
        return true;
    }

    return false;
}

void RaceSexMenu::ChangeHairColor() {
    // The three channels are held as full ints (GetValue's whole return value) and packed through their low bytes.
    int32_t iRed = -1;
    int32_t iGreen = -1;
    int32_t iBlue = -1;
    NiTListItem<RaceSexMenu::Entry*>* pIter = pOptionListsA[6]->items.m_pkHead;
    while (pIter) {
        RaceSexMenu::Entry* pOption = pOptionListsA[6]->items.GetNext(pIter);
        switch (pOption->id) {
            case 30u: {
                iRed = pOption->GetSliderValue();
                break;
            }
            case 31u: {
                iGreen = pOption->GetSliderValue();
                break;
            }
            case 32u: {
                iBlue = pOption->GetSliderValue();
                break;
            }
        }
    }
    pPlayerBase->SetHairColor(
        static_cast<uint32_t>(static_cast<uint8_t>(iRed)) | static_cast<uint32_t>(static_cast<uint8_t>(iGreen)) << 8 |
        static_cast<uint32_t>(static_cast<uint8_t>(iBlue)) << 16
    );
    UpdatePlayerHair(false);
}

void RaceSexMenu::SliderUpdate() {
    if (pSliderElem && pSliderOpt) {
        int32_t iID = static_cast<int32_t>(pSliderElem->GetFloat(4010));
        bool bValueChanged = false;
        bool bCustom = false;
        if (pSliderOpt->id == 29 && (iID == 100 || iID == 104)) {
            const char* pCustom = sRSMCustom.String();
            const char* pDisplay = pSliderElem->pParent->GetString(Tile::kTileValue_User5);
            bCustom = !_tcscmp(pDisplay, pCustom);
        }
        if (bCustom) {
            bValueChanged = true;
        }
        else {
            switch (iID) {
                case 100: {
                    int32_t iValue = pSliderOpt->GetSliderValue();
                    uint32_t uiIncrement = pSliderOpt->GetIncrement();
                    bValueChanged = pSliderOpt->SetValue(iValue - uiIncrement);
                    break;
                }
                case 104: {
                    int32_t iValue = pSliderOpt->GetSliderValue();
                    uint32_t uiIncrement = pSliderOpt->GetIncrement();
                    bValueChanged = pSliderOpt->SetValue(uiIncrement + iValue);
                    break;
                }
                case 102: {
                    int32_t iValue = pSliderOpt->GetSliderValue();
                    int32_t iJump = static_cast<int32_t>(pSliderElem->pParent->GetFloat(Tile::kTileValue_User4));
                    bValueChanged = pSliderOpt->SetValue(iValue - iJump);
                    break;
                }
                case 103: {
                    int32_t iValue = pSliderOpt->GetSliderValue();
                    int32_t iJump = static_cast<int32_t>(pSliderElem->pParent->GetFloat(Tile::kTileValue_User4));
                    bValueChanged = pSliderOpt->SetValue(iJump + iValue);
                    break;
                }
                case 105: {
                    bValueChanged = IsFlagged(RaceSexMenu::RSM_FLAG_SLIDER_UPDATE);
                    break;
                }
            }
        }
        if (bValueChanged) {
            SliderRelease();
            pSliderElem->PlayTileSound(4043);
        }
        SetFlag(RaceSexMenu::RSM_FLAG_SLIDER_UPDATE, false);
        pSliderElem = nullptr;
        pSliderOpt = nullptr;
    }
}

void RaceSexMenu::SliderRelease() {
    if (GetActiveList() == RaceSexMenu::RSM_HAIR_COLOR) {
        OnHairColorSliderChange(pSliderOpt);
    }
    else if (pSliderOpt->id == RaceSexMenu::RSM_SHAPE_SLIDER || pSliderOpt->id == RaceSexMenu::RSM_TONE_SLIDER) {
        OnFaceSliderChange(pSliderOpt);
        RaceSexMenu::OptionID eSlider = static_cast<RaceSexMenu::OptionID>(pSliderOpt->id);
        SynchronizeFaceSliders(eSlider);
        if (IsFlagged(RaceSexMenu::RSM_FLAG_UPDATE_HEAD)) {
            if (pSliderOpt->id == RaceSexMenu::RSM_SHAPE_SLIDER) {
                UpdatePlayerHead(false, true, false);
            }
            else {
                UpdatePlayerHead(false, false, true);
            }
            SetFlag(RaceSexMenu::RSM_FLAG_UPDATE_HEAD, false);
        }
    }
    else if (pSliderOpt->id == RaceSexMenu::RSM_PRESET_FACE) {
        int32_t iPreset = pSliderOpt->GetSliderValue();
        pPlayerBase->CopyHeadData(xPresetHeadsA.GetAt(iPreset - 1));
        for (int32_t i = 0; i < 20; ++i) {
            pOptionListsA[i]->UpdateCurrentSelection();
        }
        SynchronizeFaceSliders(RaceSexMenu::RSM_SHAPE_SLIDER);
        SynchronizeFaceSliders(RaceSexMenu::RSM_TONE_SLIDER);
        UpdatePlayerHead(true, true, false);
        UpdatePlayerBody();
        UpdatePlayerHair(true);
        UpdatePlayerEyes();
        SetFlag(RaceSexMenu::RSM_FLAG_UPDATE_HEAD, false);
        SetFlag(RaceSexMenu::RSM_FLAG_UPDATE_BODY, false);
        SetHairColorSliders(pPlayerBase->GetHairColor());
        RaceSexMenu::Entry* pOption = pOptionListsA[6]->GetSliderWithID(RaceSexMenu::RSM_HAIR_COLOR_PRESET);
        if (pOption) {
            int32_t iLow;
            int32_t iHigh;
            pOption->SetDisplay(sRSMCustom.String());
            pOption->GetRange(iLow, iHigh);
            pOption->SetValue(iLow);
        }
        SetFlag(RaceSexMenu::RSM_FLAG_MAKE_NEW_HAIRLIST, true);
    }
    else if (pSliderOpt->id == RaceSexMenu::RSM_AGE_FACE) {
        float fValNewTex;
        float fTexLow;
        float fGeoAge;
        float fGeoLow;
        // iAge holds the age, then the clamped geometry age.
        int32_t iAge = static_cast<int32_t>(
            static_cast<double>(pSliderOpt->GetSliderValue()) * 5.550000190734863 + 9.449999809265137
        );
        FR2MatrixVTC<float> aCoord[2][2];
        FR2MatrixVTC<float> aOffsetCoord[2][2];
        BSFaceGenManager::InitFaceGenCoord(&aOffsetCoord);
        pPlayerBase->GetFaceCoord(aCoord);
        pPlayerBase->GetFaceCoord(LastFaceCoord);
        float fValOldGeo = BSFaceGenManager::GetFaceCoordAttribute(aCoord, FANATT_AGE, FANTYPE_GEO);
        float fValOldTex = BSFaceGenManager::GetFaceCoordAttribute(aCoord, FANATT_AGE, FANTYPE_TEX);
        float fTexAge = fValOldTex - (fValOldGeo - static_cast<double>(iAge));
        if (!(static_cast<double>(iAge) > 15.0)) {
            fGeoLow = 15.0f;
        }
        else {
            fGeoLow = static_cast<float>(iAge);
        }
        if (!(fGeoLow < 65.0)) {
            fGeoAge = 65.0f;
        }
        else if (!(static_cast<double>(iAge) > 15.0)) {
            fGeoAge = 15.0f;
        }
        else {
            fGeoAge = static_cast<float>(iAge);
        }
        int32_t iValNewGeo = static_cast<int32_t>(fGeoAge);
        if (!(fTexAge > 15.0)) {
            fTexLow = 15.0f;
        }
        else {
            fTexLow = fTexAge;
        }
        if (!(fTexLow < 65.0)) {
            fValNewTex = 65.0f;
        }
        else if (!(fTexAge > 15.0)) {
            fValNewTex = 15.0f;
        }
        else {
            fValNewTex = fTexAge;
        }
        BSFaceGenManager::SetFaceCoordAttribute(aCoord, FANATT_AGE, FANTYPE_GEO, static_cast<float>(iValNewGeo));
        BSFaceGenManager::SetFaceCoordAttribute(aCoord, FANATT_AGE, FANTYPE_TEX, fValNewTex);
        TESActorBase::SEX eSex = pPlayerBase->GetSex();
        FR2MatrixVTC<float>(*pMeanFaceCoord)[2][2] = pPlayerBase->pFormRace->GetMeanFaceCoord(eSex);
        BSFaceGenManager::OffsetFaceGenCoord(pMeanFaceCoord, &aCoord, &aOffsetCoord, false);
        if (BSFaceGenManager::CompareFaceGenCoord(&aOffsetCoord, pPlayerBase->GetOffsetFaceCoord())) {
            BSFaceGenManager::CopyFaceGenCoord(&aOffsetCoord, pPlayerBase->GetOffsetFaceCoord(), false, false);
        }
        RaceSexMenu::pMe->SetFlag(RaceSexMenu::RSM_FLAG_RANDOM_HEAD, true);
        RaceSexMenu::pMe->SynchronizeFaceSliders(RaceSexMenu::RSM_SHAPE_SLIDER);
        RaceSexMenu::pMe->SynchronizeFaceSliders(RaceSexMenu::RSM_TONE_SLIDER);
        RaceSexMenu::pMe->UpdatePlayerHead(true, true, false);
        RaceSexMenu::pMe->SetFlag(RaceSexMenu::RSM_FLAG_UPDATE_HEAD, false);
        RaceSexMenu::pMe->SetFlag(RaceSexMenu::RSM_FLAG_UPDATE_BODY, true);
    }
    else {
        return;
    }
    SetFlag(RaceSexMenu::RSM_FLAG_RANDOM_HEAD, false);
}

void RaceSexMenu::InitPresetsListAndPickFirstPreset() {
    int32_t iLow;
    int32_t iHigh;
    MakePresetHeadList();
    pSliderOpt = pOptionListsA[2]->GetSliderWithID(24);
    pSliderOpt->GetRange(iLow, iHigh);
    pSliderOpt->SetValue(iLow);
    SliderRelease();
    pSliderOpt = nullptr;
}

Tile* RaceSexMenu::Create(RaceSexMenuContext aeContext) {
    Tile* pOldMenuTile = Menu::GetMenuByClass(RaceSexMenu::GetClassStatic());
    if (pOldMenuTile) {
        return pOldMenuTile;
    }
    Tile* pTile = InterfaceManager::Instance()->GetMenusRoot()->ReadFile("Data\\Menus\\CharGen\\race_sex_menu.xml");
    if (pTile) {
        RaceSexMenu::pMe = static_cast<RaceSexMenu*>(pTile->GetMenu());
        RaceSexMenu::pMe->SetMenuTile(static_cast<TileMenu*>(pTile), false);
        if (!RaceSexMenu::strDoneCaps.pString) {
            RaceSexMenu::strDoneCaps = sDone.String();
            RaceSexMenu::strNextCaps = sNext.String();
            RaceSexMenu::strBackCaps = sBack.String();
            if (GetCurrentTexLanguage() != LANGUAGE_SPANISH) {
                RaceSexMenu::strDoneCaps.MakeUpper();
                RaceSexMenu::strNextCaps.MakeUpper();
                RaceSexMenu::strBackCaps.MakeUpper();
            }
        }
        RaceSexMenu::bWantedWeaponOut = pPlayer->GetWantWeaponDrawn();
        pPlayer->SetWantWeaponDrawn(false);
        InventoryChanges* pInv = InventoryChanges::GetInventoryChanges(pPlayer);
        // The body parts the menu takes off the player, by BIPED_OBJECT index.
        for (uint32_t i = 0; i < 20; ++i) {
            switch (i) {
                case 0u:
                case 1u:
                case 5u:
                case 7u:
                case 8u:
                case 9u:
                case 10u:
                case 11u:
                case 14u:
                case 15u: {
                    RaceSexMenu::pEquippedInventoryListA[i] = pInv->GetWornItem(static_cast<BIPED_OBJECT>(i), false);
                    if (RaceSexMenu::pEquippedInventoryListA[i]) {
                        TESBoundObject* pObject = RaceSexMenu::pEquippedInventoryListA[i]->pContainerObj;
                        pPlayer->UnEquipObject(pObject, 1, nullptr, false, false, true);
                    }
                    break;
                }
                default: {
                    continue;
                }
            }
        }
        RaceSexMenu::pMe->_Create(aeContext);
        if (bUseRenderedTerminals.Bool()) {
            Tile::UpdateAll(false);
            FORenderedMenuRaceSex* pRenderedMenu = new FORenderedMenuRaceSex;
            pRenderedMenu->m_Context = aeContext;
            pRenderedMenu->pMenuTile = pTile;
            pRenderedMenu->SetMenuRoot(pTile->GetModel());
            InterfaceManager::Instance()->EnterRenderedMenu(pRenderedMenu);
        }
    }
    return pTile;
}

int32_t RaceSexMenu::GetClassStatic() {
    return 1036;
}

bool RaceSexMenu::IsPlayerRace(Entry* apOpt) {
    if (apOpt && apOpt->GetPointerData() && RaceSexMenu::pMe && RaceSexMenu::pMe->pPlayerBase) {
        TESRace* pRace = RaceSexMenu::pMe->pPlayerBase->GetRace();
        if (pRace == apOpt->GetPointerData()) {
            return true;
        }
    }
    return false;
}

bool RaceSexMenu::IsPlayerHair(Entry* apOpt) {
    if (apOpt && apOpt->GetPointerData() && RaceSexMenu::pMe && RaceSexMenu::pMe->pPlayerBase) {
        TESHair* pHair = RaceSexMenu::pMe->pPlayerBase->GetHair();
        if (pHair == apOpt->GetPointerData()) {
            return true;
        }
    }
    return false;
}

bool RaceSexMenu::IsPlayerEyes(RaceSexMenu::Entry* apOpt) {
    if (apOpt && apOpt->data && RaceSexMenu::pMe && RaceSexMenu::pMe->pPlayerBase) {
        TESEyes* pEyes = RaceSexMenu::pMe->pPlayerBase->GetEyeColor();
        if (pEyes == apOpt->data) {
            return true;
        }
    }

    return false;
}

void RaceSexMenu::RandomizeCallback() {
    if (RaceSexMenu::pMe &&
        (Interface::GetMessageMenuResult() == 1 || RaceSexMenu::pMe->IsFlagged(RSM_FLAG_RANDOM_HEAD))) {
        RaceSexMenu::pMe->pPlayerBase->RandomizeHead(true, false, true);
        RaceSexMenu::pMe->SetFlag(RSM_FLAG_RANDOM_HEAD, true);
        RaceSexMenu::pMe->SynchronizeFaceSliders(RSM_SHAPE_SLIDER);
        RaceSexMenu::pMe->SynchronizeFaceSliders(RSM_TONE_SLIDER);
        RaceSexMenu::pMe->UpdatePlayerHead(true, true, false);
        RaceSexMenu::pMe->UpdatePlayerBody();
        RaceSexMenu::pMe->SetFlag(RSM_FLAG_UPDATE_HEAD, false);
        RaceSexMenu::pMe->SetFlag(RSM_FLAG_UPDATE_BODY, false);
    }
}

void RaceSexMenu::Close() {
    if (RaceSexMenu::pMe) {
        if (RaceSexMenu::pMe->IsFlagged(RaceSexMenu::RSM_FLAG_UPDATE_BODY)) {
            RaceSexMenu::pMe->UpdatePlayerBody();
            RaceSexMenu::pMe->SetFlag(RaceSexMenu::RSM_FLAG_UPDATE_BODY, false);
        }
        if (Interface::GetMessageMenuResult() == 1) {
            if (InterfaceManager::Has360Controller()) {
                RaceSexMenu::ListID eActiveList = RaceSexMenu::pMe->GetActiveList();
                if (!RaceSexMenu::pMe->pOptionListsA[eActiveList]->Focus(1)) {
                    if (RaceSexMenu::pMe->rsmNextButton->IsVisible()) {
                        TileImage* pNextButton = RaceSexMenu::pMe->rsmNextButton;
                        InterfaceManager::Instance()->SetCurrentFocusTarget(pNextButton, 4035, true);
                    }
                    else {
                        eActiveList = RaceSexMenu::pMe->GetActiveList();
                        RaceSexMenu::pMe->pOptionListsA[eActiveList]->SetActive();
                        RaceSexMenu::pMe->UpdateScrollArrows();
                    }
                }
            }
        }
        else {
            StatsMenu::UpdatePlayerName();
            RaceSexMenu::pMe->pMenu->Set(Tile::kTileValue_DeleteOnFade, 1);
            RaceSexMenu::pMe->InstantFadeOut();
            RaceSexMenu::pMe = nullptr;
            if (DialogMenu::IsDialogActive()) {
                DialogMenu::ServiceEnd(TESTopic::GetTopic(DT_SERVICE, 4));
            }
            for (ItemChange*& pItem : RaceSexMenu::pEquippedInventoryListA) {
                if (pItem) {
                    TESBoundObject* pObject = pItem->pContainerObj;
                    pPlayer->EquipObject(pObject, 1, nullptr, true, false, true);
                    ItemChange* pChange = pItem;
                    if (pChange) {
                        delete pChange;
                    }
                    pItem = nullptr;
                }
            }
            pPlayer->SetWantWeaponDrawn(RaceSexMenu::bWantedWeaponOut);
            pPlayer->SetAllowEGMCacheClear(true);
        }
    }
}

bool RaceSexMenu::IsPlayerSex(Entry* apOpt) {
    return apOpt &&
        RaceSexMenu::pMe &&
        RaceSexMenu::pMe->pPlayerBase &&
        ((RaceSexMenu::pMe->pPlayerBase->GetSex() == TESActorBase::SEX_MALE && apOpt->id == 21) ||
         (RaceSexMenu::pMe->pPlayerBase->GetSex() == TESActorBase::SEX_FEMALE && apOpt->id == 22));
}

bool RaceSexMenu::IsPlayerHeadPart(Entry* apOpt) {
    if (apOpt &&
        RaceSexMenu::pMe &&
        RaceSexMenu::pMe->pPlayerBase &&
        !RaceSexMenu::pMe->pPlayerBase->GetHeadParts()->IsEmpty()) {
        if (RaceSexMenu::pMe->pPlayerBase->GetHeadParts()->m_item == apOpt->data) {
            return true;
        }
    }

    return false;
}

FORenderedMenuRaceSex::FORenderedMenuRaceSex()
  : FORenderedMenu(), spButtons(), spBtnGlow(), spGeometry(nullptr), spPlayer3D(nullptr), spPlayerParent(nullptr),
    spClonedPlayerCamera(nullptr), spPlayerLights(nullptr), spPlayer3DParent(nullptr) {}
