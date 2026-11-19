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

static SettingT<INISettingCollection> gs_fRSMStartingZoom_Interface("fRSMStartingZoom:Interface", 0.7f);

// The game settings this file defines, with their IDs and values, in initialization order.
BSStringT<char> RaceSexMenu::strNextCaps;
BSStringT<char> RaceSexMenu::strDoneCaps;
BSStringT<char> RaceSexMenu::strBackCaps;
SettingT<INISettingCollection> fRSMFaceSliderDefaultMin("fRSMFaceSliderDefaultMin:Interface", -5.0f);
SettingT<INISettingCollection> fRSMFaceSliderDefaultMax("fRSMFaceSliderDefaultMax:Interface", 5.0f);

RaceSexMenu::Entry::Entry() : tile(nullptr), data(nullptr) {}

int32_t RaceSexMenu::Entry::GetValue() {
    return (int32_t)tile->GetFloat(Tile::AddUserTrait("_position", -1));
}

void RaceSexMenu::Entry::SetEnabled(bool abEnabled) {
    if (tile) {
        tile->Set(Tile::AddUserTrait("_enabled", -1), (int32_t)abEnabled);
    }
}

void RaceSexMenu::Entry::SetDisplay(const char* Src) {
    if ((tile) && (Src)) {
        tile->Set(Tile::kTileValue_User5, Src, true);
    }
}

void RaceSexMenu::Entry::SetSelected2() {
    int32_t v1;
    if (tile) {
        v1 = Tile::AddUserTrait("_selected", (-1));
        tile->Set(v1, 0x1p+1f, true);
    }
}

void RaceSexMenu::Entry::SetSelected1() {
    int32_t v1;
    if (tile) {
        v1 = Tile::AddUserTrait("_selected", (-1));
        tile->Set(v1, (float)1.0, true);
    }
}

bool RaceSexMenu::Entry::HasSelectedValue2() {
    int32_t v1;
    uint8_t v3 = 0;
    if (tile) {
        v1 = Tile::AddUserTrait("_selected", (-1));
        if (tile->GetFloat(v1) == 2.0) {
            return true;
        }
    }
    return (bool)v3;
}

int32_t RaceSexMenu::Entry::GetValue_007B3740() {
    return static_cast<int32_t>(tile->GetFloat(Tile::kTileValue_User1));
}

bool RaceSexMenu::Entry::SetValue(int32_t aiVal) {
    int32_t ilow;
    int32_t ihigh;
    if (!tile) {
        return false;
    }
    int32_t icur = GetValue_007B3740();
    GetRange(ilow, ihigh);
    if (aiVal < ilow) {
        aiVal = ilow;
    }
    if (aiVal > ihigh) {
        aiVal = ihigh;
    }
    if (icur == aiVal) {
        return false;
    }
    tile->Set(Tile::kTileValue_User1, aiVal);
    Tile::Value* pdisplay = tile->GetValueQ(Tile::kTileValue_User5);
    if (!pdisplay->strValue) {
        pdisplay->SetFloat(static_cast<float>(aiVal), true);
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
        if (GetValue_007B3740() < aiLow) {
            SetValue(aiLow);
        }
        if (GetValue_007B3740() > aiHigh) {
            SetValue(aiHigh);
        }
    }
}

uint32_t RaceSexMenu::Entry::GetIncrement() {
    if (!tile) {
        return 0;
    }
    float Float = tile->GetFloat(Tile::kTileValue_User6);
    if (!(Float <= 1.0)) {
        return static_cast<int64_t>(Float);
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

char RaceSexMenu::List::Focus(char a2) {
    InterfaceManager* singleton;
    InterfaceManager* v3;
    Tile* v5;
    int8_t v7 = 0;
    if (a2) {
        if ((selectedEntry) && (selectedEntry->tile)) {
            v5 = selectedEntry->tile;
            singleton = InterfaceManager::Instance();
            singleton->SetCurrentFocusTarget(v5, 4035, true);
            return 1;
        }
    }
    else {
        v3 = InterfaceManager::Instance();
        v3->SetCurrentFocusTarget(nullptr, 4035, true);
        selectedEntry = nullptr;
        return 1;
    }
    return v7;
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
    RaceSexMenu::Entry* popt;
    RaceSexMenu::Entry* popt_2;
    if (!apName || (!RaceSexMenu::pMe || (!RaceSexMenu::pMe->rsmBackground || !listItemTemplate))) {
        return nullptr;
    }
    RaceSexMenu::Entry* apMemory = static_cast<RaceSexMenu::Entry*>(ourMalloc(sizeof(RaceSexMenu::Entry)));
    if (apMemory) {
        popt = ::new (apMemory) RaceSexMenu::Entry;
    }
    else {
        popt = nullptr;
    }
    popt_2 = popt;
    int32_t ilistID = static_cast<int32_t>(listItemTemplate->GetFloat(Tile::kTileValue_User0));
    popt->id = aiID;
    popt_2->tile = RaceSexMenu::pMe->RenderTemplate(RaceSexMenu::pMe->rsmBackground, apTemplate, nullptr);
    popt_2->tile->Set(Tile::kTileValue_String, apName, true);
    popt_2->tile->Set(Tile::kTileValue_User0, ilistID);
    popt_2->tile->Set(Tile::kTileValue_Target, 1);
    if (apIter) {
        items.InsertBefore(apIter, popt_2);
    }
    else {
        items.AddTail(popt_2);
    }
    apIter = items.FindPos(popt_2, nullptr);
    while (apIter) {
        double v9;
        Tile** pprev;
        NiTListItem<RaceSexMenu::Entry*>* pprevIter = items.GetPrevPos(apIter);
        if (pprevIter) {
            pprev = &items.Get(pprevIter)->tile;
        }
        else {
            pprev = nullptr;
        }
        RaceSexMenu::Entry* pcurr = items.GetNext(apIter);
        if (pprev) {
            double Float = (*pprev)->GetFloat(Tile::kTileValue_Y);
            v9 = (*pprev)->GetFloat(Tile::kTileValue_Height) + Float;
        }
        else {
            int32_t v6 = Tile::AddUserTrait("_top_bound", -1);
            v9 = pcurr->tile->pParent->GetFloat(v6);
        }
        float v8 = v9;
        pcurr->tile->Set(Tile::kTileValue_Y, v8, true);
        pcurr->tile->UpdateTile(false);
    }
    float a2 = popt_2->tile->GetFloat(Tile::kTileValue_Height);
    height = BSMAX(height, a2);
    if (ilistID < 4 && popt_2->id < 4) {
        popt_2->tile->Set(Tile::kTileValue_ClickSound, "UIPipBoyMode", true);
    }
    if (!selectedEntry) {
        selectedEntry = popt_2;
        popt_2->tile->Set(Tile::kTileValue_xdefault, 2.0f, true);
    }
    return popt_2;
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
            RaceSexMenu::Entry* v2 = items.GetNext(pos);
            if (IsItemSelected(v2)) {
                v2->SetSelected2();
            }
            else {
                v2->SetSelected1();
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
    RaceSexMenu::Entry* pnew = AddMenuOption(apDisplayText, "RSM_slider_option_template", aeID, nullptr);
    pnew->SetRange(aiLowVal, aiHighVal);
    pnew->SetValue(aiStartVal);
    float afVal = static_cast<double>(aiHighVal - aiLowVal + 1) * afJumpPct;
    float afNearest = BSRoundToNearest(afVal, 1.0f);
    pnew->tile->Set(Tile::kTileValue_User4, afNearest, true);
    return pnew;
}

RaceSexMenu::Entry*
RaceSexMenu::List::InsertMenuOption(uint32_t aiPos, const char* apDisplayText, const char* apTemplate, int32_t aiID) {
    if (!apDisplayText || (!RaceSexMenu::pMe || (!RaceSexMenu::pMe->rsmBackground || !listItemTemplate))) {
        return nullptr;
    }
    NiTListItem<RaceSexMenu::Entry*>* piter = items.m_pkHead;
    while (piter && aiPos) {
        piter = items.GetNextPos(piter);
        --aiPos;
    }
    return AddMenuOption(apDisplayText, apTemplate, aiID, piter);
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
        float fTop = dTop;
        pCurr->tile->Set(Tile::kTileValue_Y, fTop, true);
        pCurr->tile->UpdateTile(false);
    }
}

RaceSexMenu::Entry* RaceSexMenu::List::GetSliderWithID(int32_t a1) {
    NiTListItem<RaceSexMenu::Entry*>* piter = items.m_pkHead;
    while (piter) {
        RaceSexMenu::Entry* v3 = items.GetNext(piter);
        if (v3->id == a1) {
            return v3;
        }
    }
    return nullptr;
}

bool RaceSexMenu::List::HandleKey(int32_t a1) {
    char result;
    bool v6 = a1 == 1 || a1 == 15;
    char v9 = 1;
    switch (a1) {
        case 1:
        case 2: {
            NiTListItem<RaceSexMenu::Entry*>* a1b;
            NiTListItem<RaceSexMenu::Entry*>* a1a = items.FindPos(selectedEntry, nullptr);
            if (a1a) {
                NiTListItem<RaceSexMenu::Entry*>* PrevPos;
                if (v6) {
                    PrevPos = items.GetPrevPos(a1a);
                }
                else {
                    PrevPos = items.GetNextPos(a1a);
                }
                a1b = PrevPos;
            }
            else {
                NiTListItem<RaceSexMenu::Entry*>* TailPos;
                if (v6) {
                    TailPos = items.m_pkTail;
                }
                else {
                    TailPos = items.m_pkHead;
                }
                a1b = TailPos;
            }
            if (a1b) {
                selectedEntry = items.Get(a1b);
            }
            else {
                v9 = 0;
            }
            goto LABEL_21;
        }
        case 15: {
            selectedEntry = items.GetHead();
            goto LABEL_21;
        }
        case 16: {
            selectedEntry = items.GetTail();
        LABEL_21:;
            if ((selectedEntry) && (selectedEntry->tile)) {
                int32_t v8 = 100;
                while (true) {
                    int32_t v4 = v8--;
                    if (!v4 || selectedEntry->tile->IsVisible()) {
                        break;
                    }
                    RaceSexMenu::pMe->DoClick(3 - v6, nullptr);
                }
                Interface::SetCurrentFocusTarget(selectedEntry->tile);
            }

            result = v9;
            break;
        }
        default: {
            result = 0;
            break;
        }
    }
    return result;
}

void RaceSexMenu::List::SetActive() {
    if (InterfaceManager::Has360Controller()) {
        if (!selectedEntry && !items.IsEmpty()) {
            selectedEntry = items.GetHead();
        }
        if (selectedEntry) {
            const char* String = selectedEntry->tile->GetString(4072);
            BSStringT<char> apString(String);
            selectedEntry->tile->Set(4072, -1);
            Interface::SetCurrentFocusTarget(selectedEntry->tile);
            selectedEntry->tile->Set(4072, apString.pString, true);
            while (true) {
                int32_t Value = selectedEntry->GetValue();
                if (!Value) {
                    break;
                }
                ScrollBy(-Value);
            }
        }
    }
}

void RaceSexMenu::List::ScrollBy(int32_t aiDelta) {
    if (!(height <= 0.0)) {
        NiTListItem<RaceSexMenu::Entry*>* pos = items.m_pkHead;
        while (pos) {
            Tile* th_s = items.GetNext(pos)->tile;
            float Float = th_s->GetFloat(4002);
            float afValue = static_cast<double>(aiDelta) * height + Float;
            th_s->Set(4002, afValue, true);
        }
    }
}

RaceSexMenu::Entry* RaceSexMenu::List::Find(Tile* apTile) {
    NiTListItem<RaceSexMenu::Entry*>* piter = items.m_pkHead;
    while (piter) {
        RaceSexMenu::Entry* popt_2 = items.GetNext(piter);
        if (popt_2->tile == apTile) {
            return popt_2;
        }
    }
    return nullptr;
}

RaceSexMenu::RaceSexMenu()
  : pOptionListsA{}, pTiles{}, fDragStartX(0.0f), iDragStartSliderVal(0), pSliderElem(nullptr), pSliderOpt(nullptr),
    unkC8(0.0f), fZoomVelocity(0.0f), fZoomPct(0.0f), uiFlags(3), eLastList(RaceSexMenu::RSM_LIST_COUNT) {
    ptZoomBound[1] = NiPoint3_ZERO;
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
        iDragStartSliderVal = pSliderOpt->GetValue_007B3740();
    }
    InterfaceManager* v3 = InterfaceManager::Instance();
    fDragStartX = v3->GetPipboyMouseX();
}

void RaceSexMenu::DoClick(int32_t aiID, Tile* apTarget) {
    if (!apTarget && aiID < 6) {
        apTarget = pTiles[aiID];
    }
    RaceSexMenu::ListID eactiveList = GetActiveList();
    RaceSexMenu::Entry* popt = pOptionListsA[eactiveList]->Find(apTarget);
    if (popt) {
        bool bupdateSelection = false;
        bool bupdateLocomotion = false;
        switch (popt->id) {
            case RSM_MALE:
            case RSM_RACE_OPTION: {
                goto LABEL_7;
            }
            case RSM_FEMALE: {
                pPlayerBase->GetHeadParts()->RemoveAll();
            LABEL_7:;
                if (!popt->HasSelectedValue2()) {
                    if (popt->id == static_cast<uint32_t>(RSM_RACE_OPTION)) {
                        TESRace* Race = pPlayerBase->GetRace();
                        Race->KillEGTData();
                        pPlayer->DispellSpellsForRace();
                        pPlayer->RemoveEffects(false);
                        pPlayer->DispelPermanentMagic();
                        pPlayerBase->SwitchRace(static_cast<TESRace*>(popt->data), nullptr);
                    }
                    else {
                        TESRace* v4 = pPlayerBase->GetRace();
                        v4->KillEGTData();
                        pPlayerBase->SetFlagBit(1, popt->id == static_cast<uint32_t>(RSM_FEMALE), true);
                        bupdateLocomotion = true;
                    }
                    InitPresetsListAndPickFirstPreset();
                    UpdatePlayerBody();
                    ValidateHairAndEyes();
                    bupdateSelection = true;
                    if (bupdateLocomotion) {
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
                    const char* v9 = sYes.String();
                    const char* v8 = sNo.String();
                    Interface::CreateMessageMenu(
                        sRSMConfirmRandomize.String(), static_cast<const char*>(nullptr),
                        static_cast<const char*>(nullptr), &RaceSexMenu::RandomizeCallback, 0, 18, 0.0f, 0.0f, v8, v9,
                        nullptr
                    );
                }
                break;
            }
            case RSM_HAIR_STYLE_OPTION: {
                if (!popt->HasSelectedValue2()) {
                    pPlayerBase->SetHair(static_cast<TESHair*>(popt->data));
                    UpdatePlayerHair(true);
                    bupdateSelection = true;
                }
                break;
            }
            case RSM_FACIAL_HAIR_OPTION: {
                pPlayerBase->GetHeadParts()->RemoveAll();
                if (!popt->HasSelectedValue2()) {
                    void* a2 = popt->data;
                    pPlayerBase->GetHeadParts()->AddHead(a2);
                }
                UpdatePlayerHead(true, true, false);
                bupdateSelection = true;
                break;
            }
            case RSM_EYE_COLOR_OPTION: {
                if (!popt->HasSelectedValue2()) {
                    pPlayerBase->SetEyeColor(static_cast<TESEyes*>(popt->data));
                    UpdatePlayerEyes();
                    bupdateSelection = true;
                }
                break;
            }
            default: {
                if (popt->id < RSM_LIST_COUNT) {
                    SetActiveList(static_cast<RaceSexMenu::ListID>(popt->id));
                }
                break;
            }
        }
        if (bupdateSelection) {
            pOptionListsA[eactiveList]->UpdateCurrentSelection();
            SetFlag(RaceSexMenu::RSM_FLAG_RANDOM_HEAD, false);
        }
    }
    else {
        if (aiID == 4) {
            RaceSexMenu::ListID CastingType = static_cast<RaceSexMenu::ListID>(pOptionListsA[eactiveList]->parentID);
            SetActiveList(CastingType);
        }
        else {
            if (aiID == 5) {
                if (eactiveList == eLastList) {
                    const char* v10 = sYes.String();
                    const char* v6 = sNo.String();
                    Interface::CreateMessageMenu(
                        static_cast<const char*>(strConfirmationMsg.pString), static_cast<const char*>(nullptr),
                        static_cast<const char*>(nullptr), &RaceSexMenu::Close, 1, 18, 0.0f, 0.0f, v6, v10, nullptr
                    );
                }
                else {
                    int32_t v7 = pOptionListsA[eactiveList]->childID;
                    SetActiveList(static_cast<RaceSexMenu::ListID>(v7));
                }
            }
            else {
                popt = pOptionListsA[eactiveList]->Find(apTarget->pParent);
                if (popt) {
                    pSliderElem = apTarget;
                    pSliderOpt = popt;
                    SliderUpdate();
                }
                else {
                    if (aiID == 2 && apTarget->IsTrue(Tile::kTileValue_Visible)) {
                        pOptionListsA[eactiveList]->ScrollBy(1);
                        UpdateScrollArrows();
                    }
                    else {
                        if (aiID == 3 && apTarget->IsTrue(Tile::kTileValue_Visible)) {
                            pOptionListsA[eactiveList]->ScrollBy(-1);
                            UpdateScrollArrows();
                        }
                    }
                }
            }
        }
    }
}

void RaceSexMenu::DoStillDown(int32_t aiID, Tile* apTarget) {
    InterfaceManager* v3 = InterfaceManager::Instance();
    float fdeltaX = v3->GetPipboyMouseX() - fDragStartX;
    if (fdeltaX != 0.0) {
        if (aiID == 105 && apTarget) {
            RaceSexMenu::ListID eactiveList = GetActiveList();
            RaceSexMenu::Entry* popt = pOptionListsA[eactiveList]->Find(apTarget->pParent);
            if (popt) {
                int32_t ilow;
                int32_t ihigh;
                popt->GetRange(ilow, ihigh);
                Tile* ChildByID = apTarget->pParent->GetChildByID(101);
                double Float = ChildByID->GetFloat(4017);
                float fdeltaPct = fdeltaX / Float;
                float afVal = static_cast<double>(iDragStartSliderVal) + static_cast<double>(ihigh - ilow) * fdeltaPct;
                double v6 = BSRoundToNearest(afVal, 1.0f);
                if (popt->SetValue(static_cast<int32_t>(v6))) {
                    pSliderOpt = popt;
                    pSliderElem = apTarget;
                    SliderRelease();
                }
            }
        }
        else {
            if (aiID == 1) {
                static float ffullRotX = rsmFaceGrab->GetFloat(4017);
                float v9 = fdeltaX / ffullRotX * 6.2831854820251465;
                bool v7 = RotatePlayer(v9, false);
                SetFlag(RaceSexMenu::RSM_FLAG_ROTATING, v7);
                InterfaceManager* v8 = InterfaceManager::Instance();
                fDragStartX = v8->GetPipboyMouseX();
            }
        }
    }
}

void RaceSexMenu::DoIdle() {
    Interface::enumMenus v1 = static_cast<Interface::enumMenus>(GetClass());
    if (Interface::IsTopMenuID(v1)) {
        for (uint32_t i = 0; i < 4; ++i) {
            if (Sounds[i].eState == 3) {
                Sounds[i].Play(false);
            }
        }
        static int32_t time_11DA740 = BSGetTickCount();
        uint32_t ithistime = BSGetTickCount();
        if (InterfaceManager::Has360Controller()) {
            Controls* Controls_ = appMain->GetControls();
            int32_t imouseX = Controls_->GetControllerValue(0, 9);
            Controls* v3 = appMain->GetControls();
            int32_t imouseY = v3->GetControllerValue(0, 10);
            int32_t iabsX = BSAbs(imouseX);
            int32_t iabsY = BSAbs(imouseY);
            SetFlag(RSM_FLAG_ROTATING, false);
            SetFlag(RSM_FLAG_ZOOMING, false);
            if (iabsY <= 0x21F1 || iabsY <= iabsX) {
                if (iabsX > 0x21F1 && iabsX > iabsY) {
                    double v5;
                    if (imouseX <= 0) {
                        v5 = static_cast<double>(-imouseX - 0x21F1) / static_cast<double>(minus0x7FFF + 0x21F1);
                    }
                    else {
                        v5 = static_cast<double>(imouseX - 0x21F1) / static_cast<double>(_0x7FFF - 0x21F1);
                    }
                    float v22 = v5;
                    float v14 = v22 * 2.5132741928100586 * static_cast<double>(ithistime - time_11DA740) / 1000.0;
                    bool v6 = RotatePlayer(v14, false);
                    SetFlag(RSM_FLAG_ROTATING, v6);
                }
                fZoomVelocity = 0.0f;
            }
            else {
                int32_t v16;
                int32_t irange = _0x7FFF - 0x21F1;
                fZoomVelocity = static_cast<double>(irange - (_0x7FFF - BSMIN(iabsY, _0x7FFF))) * 3.12499992105586e-06 /
                        static_cast<double>(irange) * static_cast<double>(ithistime - time_11DA740) +
                    fZoomVelocity;
                if (!(static_cast<double>(imouseY) > 0.0)) {
                    v16 = 1;
                }
                else {
                    v16 = -1;
                }
                float v15 = static_cast<double>(v16) * fZoomVelocity * static_cast<double>(ithistime - time_11DA740);
                bool v4 = ZoomPlayer(v15, false);
                SetFlag(RSM_FLAG_ZOOMING, v4);
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
            if ((pSliderOpt) && (IsFlagged(RSM_FLAG_STICK_RELEASED))) {
                SliderRelease();
                pSliderOpt = nullptr;
                pSliderElem = nullptr;
            }
        }
        else {
            SetFlag(RSM_FLAG_ZOOMING, false);
            InterfaceManager* v7 = InterfaceManager::Instance();
            if ((v7->GetMouseWheel()) && (InterfaceManager::Instance()->pOverTileTarget)) {
                InterfaceManager* v8 = InterfaceManager::Instance();
                bool v21 = static_cast<double>(v8->GetMouseWheel()) > 0.0;
                if (InterfaceManager::Instance()->pOverTileTarget == rsmFaceGrab) {
                    float v13;
                    if (v21) {
                        v13 = -0.125f;
                    }
                    else {
                        v13 = 0.125f;
                    }
                    unkC8 = unkC8 + v13;
                    float a1 = BSMIN(unkC8, 1.0f);
                    unkC8 = BSMAX(a1, 0.0f);
                }
                else {
                    if (v21) {
                        DoClick(2, nullptr);
                    }
                    else {
                        DoClick(3, nullptr);
                    }
                }
            }

            if (!(unkC8 > static_cast<double>(fZoomPct))) {
                if (!(unkC8 < static_cast<double>(fZoomPct))) {
                    fZoomVelocity = 0.0f;
                }
                else {
                    fZoomVelocity =
                        fZoomVelocity - static_cast<double>(ithistime - time_11DA740) * 3.12499992105586e-06;
                    float v19 = static_cast<double>(ithistime - time_11DA740) * fZoomVelocity;
                    if (unkC8 > fZoomPct + v19) {
                        v19 = unkC8 - fZoomPct;
                    }
                    bool v10 = ZoomPlayer(v19, false);
                    SetFlag(RSM_FLAG_ZOOMING, v10);
                }
            }
            else {
                fZoomVelocity = static_cast<double>(ithistime - time_11DA740) * 3.12499992105586e-06 + fZoomVelocity;
                float v20 = static_cast<double>(ithistime - time_11DA740) * fZoomVelocity;
                if (unkC8 < fZoomPct + v20) {
                    v20 = unkC8 - fZoomPct;
                }
                bool v9 = ZoomPlayer(v20, false);
                SetFlag(RSM_FLAG_ZOOMING, v9);
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
            if ((pSliderOpt) && (appMain->GetControls())) {
                Controls* v11 = appMain->GetControls();
                if (v11->GetMouseButton(0, 2)) {
                    SliderUpdate();
                }
            }
        }
        time_11DA740 = ithistime;
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
    bool result;
    Interface::enumMenus v3 = static_cast<Interface::enumMenus>(GetClass());
    if (!Interface::IsTopMenuID(v3)) {
        return false;
    }
    if (aiButton >= 0) {
        Controls* Controls_ = appMain->GetControls();
        int32_t imouseX = Controls_->GetControllerValue(0, 7);
        if (aiButton == 4 || (aiButton == 3 || BSAbs(imouseX) >= 0x1EA9)) {
            SetFlag(RaceSexMenu::RSM_FLAG_STICK_RELEASED, false);
        }
        else {
            SetFlag(RaceSexMenu::RSM_FLAG_STICK_RELEASED, true);
        }
    }
    switch (aiButton) {
        case 1: {
            goto LABEL_49;
        }
        case 2: {
            if (InterfaceManager::Instance()->pMouseOverTarget == rsmBackButton ||
                (InterfaceManager::Instance()->pMouseOverTarget == rsmNextButton ||
                 pOptionListsA[GetActiveList()]->HandleKey(aiButton))) {
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
            int32_t eid;
            Tile* pchild;
            Tile* pcft;
            pcft = InterfaceManager::Instance()->pMouseOverTarget;
            if (!pcft && !InterfaceManager::Has360Controller()) {
                for (pcft = InterfaceManager::Instance()->pOverTileTarget; pcft && pcft->pParent != rsmBackground;
                     pcft = pcft->pParent) {
                    ;
                }
            }
            if (pcft == rsmBackButton && (aiButton == 3 && rsmNextButton->IsVisible())) {
                Interface::SetCurrentFocusTarget(rsmNextButton);
                return true;
            }
            if (pcft == rsmNextButton && (aiButton == 4 && rsmBackButton->IsVisible())) {
                Interface::SetCurrentFocusTarget(rsmBackButton);
                return true;
            }
            if (!pcft) {
                goto LABEL_49;
            }
            switch (aiButton) {
                case 15: {
                    eid = 102;
                    break;
                }
                case 16: {
                    eid = 103;
                    break;
                }
                case 3: {
                    eid = 104;
                    break;
                }
                case 4: {
                    eid = 100;
                    break;
                }
                default: {
                    return false;
                }
            }
            pchild = pcft->GetChildByID(eid);
            if (pchild) {
                DoClick(eid, pchild);
                result = true;
            }
            else {
            LABEL_49:;
                result = pOptionListsA[GetActiveList()]->HandleKey(aiButton);
            }
            break;
        }
        case 10: {
            RaceSexMenu::ListID CastingType =
                static_cast<RaceSexMenu::ListID>(pOptionListsA[GetActiveList()]->parentID);
            if (SetActiveList(CastingType)) {
                BSAudio* v6 = &BSAudio::QInstance();
                v6->GetSoundHandleByName("UIMenuOK", 289u).Play(false);
            }
            return true;
        }
        case 13:
        case 14: {
            RaceSexMenu::ListID v8;
            RaceSexMenu::ListID etop;
            for (etop = GetActiveList(); etop >= RaceSexMenu::RSM_CUSTOM;
                 etop = static_cast<RaceSexMenu::ListID>(pOptionListsA[etop]->parentID)) {
                ;
            }
            if (aiButton == 13) {
                v8 = static_cast<RaceSexMenu::ListID>(pOptionListsA[etop]->parentID);
            }
            else {
                v8 = static_cast<RaceSexMenu::ListID>(pOptionListsA[etop]->childID);
            }
            if (SetActiveList(v8)) {
                BSAudio* v9 = &BSAudio::QInstance();
                v9->GetSoundHandleByName("UIPipBoyMode", 289u).Play(false);
            }
            else {
                if (aiButton == 14) {
                    const char* v11 = sYes.String();
                    const char* v10 = sNo.String();
                    Interface::CreateMessageMenu(
                        static_cast<const char*>(strConfirmationMsg.pString), static_cast<const char*>(nullptr),
                        static_cast<const char*>(nullptr), &RaceSexMenu::Close, 1, 18, 0.0f, 0.0f, v10, v11, nullptr
                    );
                }
            }
            return true;
        }
        default: {
            return false;
        }
    }
    return result;
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
        void* pMemory = ourMalloc(sizeof(List));
        List* pList;
        if (pMemory) {
            pList = ::new (pMemory) List(i, apListNames[i]);
        }
        else {
            pList = nullptr;
        }
        pOptionListsA[i] = pList;
    }
    pOptionListsA[0]->SetIsItemSelected(&RaceSexMenu_IsPlayerSex);
    pOptionListsA[1]->SetIsItemSelected(&RaceSexMenu::IsPlayerRace);
    pOptionListsA[5]->SetIsItemSelected(&RaceSexMenu::IsPlayerHair);
    pOptionListsA[8]->SetIsItemSelected(&RaceSexMenu::IsPlayerEyes);
    pOptionListsA[7]->SetIsItemSelected(&RaceSexMenu_IsPlayerHeadPart);
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
        static_cast<const char*>(sRSMRedAbbrev), 30, 0, 255, static_cast<uint8_t>(uiHairColor), 0.125f
    );
    pOptionListsA[6]->AddSliderOption(
        static_cast<const char*>(sRSMGreenAbbrev), 31, 0, 255,
        static_cast<uint8_t>(static_cast<uint16_t>(uiHairColor) >> 8), 0.125f
    );
    pOptionListsA[6]->AddSliderOption(
        static_cast<const char*>(sRSMBlueAbbrev), 32, 0, 255, static_cast<uint8_t>(uiHairColor >> 16), 0.125f
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
    unkC8 = fZoomPct;
    pOptionListsA[0]->UpdateCurrentSelection();
    pOptionListsA[1]->UpdateCurrentSelection();
    pOptionListsA[7]->UpdateCurrentSelection();
    Sounds[0].Play(false);
    Sounds[1].Play(false);
    SetActiveList(eActiveList);
    PrepForVisibility(false);
}

bool RaceSexMenu::SetActiveList(RaceSexMenu::ListID aeList) {
    BSStringT<char>* v7;
    if (aeList >= RaceSexMenu::RSM_LIST_COUNT) {
        return false;
    }
    pMenu->Set(Tile::kTileValue_User0, aeList);
    pOptionListsA[aeList]->SetActive();
    if (aeList == RaceSexMenu::RSM_HAIR_STYLE && IsFlagged(RaceSexMenu::RSM_FLAG_MAKE_NEW_HAIRLIST)) {
        InitHairStyles();
    }
    else {
        if (aeList == RaceSexMenu::RSM_EYE_COLOR && IsFlagged(RaceSexMenu::RSM_FLAG_MAKE_NEW_EYESLIST)) {
            MakeEyeColorList();
        }
        else {
            if (aeList == RaceSexMenu::RSM_HAIR) {
                ValidateHairOptionList();
            }
        }
    }
    UpdateScrollArrows();
    InterfaceManager* v2 = InterfaceManager::Instance();
    if ((v2->GetCurrentRenderedMenu()) && (aeList < RaceSexMenu::RSM_CUSTOM)) {
        InterfaceManager* v3 = InterfaceManager::Instance();
        FORenderedMenuRaceSex* CurrentRenderedMenu = static_cast<FORenderedMenuRaceSex*>(v3->GetCurrentRenderedMenu());
        CurrentRenderedMenu->LightButton(aeList);
    }

    // The parent and child list IDs are compared signed.
    int32_t CastingType = pOptionListsA[aeList]->parentID;
    rsmBackButton->Set(Tile::kTileValue_Visible, static_cast<int32_t>(CastingType < 20));
    int32_t iChildID = pOptionListsA[aeList]->childID;
    int32_t aiValue = iChildID < 20 || aeList == eLastList;
    rsmNextButton->Set(Tile::kTileValue_Visible, aiValue);
    if (aeList == eLastList) {
        v7 = &RaceSexMenu::strDoneCaps;
    }
    else {
        v7 = &RaceSexMenu::strNextCaps;
    }
    rsmNextButton->Set(4036, v7->pString, true);
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
    BSSimpleArray<TESNPC*, 1024>* p_xPresetHeadsA = &xPresetHeadsA;
    TESActorBase::SEX Sex = pPlayerBase->GetSex();
    TESRace* Race = pPlayerBase->GetRace();
    TESNPC::FillCharGenPresetList(Race, Sex, p_xPresetHeadsA);
    RaceSexMenu::Entry* SliderWithID = pOptionListsA[2]->GetSliderWithID(24);
    if (xPresetHeadsA.IsEmpty()) {
        SliderWithID->SetEnabled(false);
    }
    else {
        SliderWithID->SetEnabled(true);
        uint32_t FormFlags = xPresetHeadsA.iSize;
        SliderWithID->SetRange(1, FormFlags);
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
        {RaceSexMenu::RSM_SHAPE_FOREHEAD, sRSMShapeOption13.String(), fRSMFaceSliderDefaultMin,
         fRSMFaceSliderDefaultMax},
        {RaceSexMenu::RSM_SHAPE_FOREHEAD, sRSMShapeOption14.String(), fRSMFaceSliderDefaultMin,
         fRSMFaceSliderDefaultMax},
        {RaceSexMenu::RSM_SHAPE_FOREHEAD, sRSMShapeOption27.String(), fRSMFaceSliderDefaultMin,
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
            float fval =
                BSFaceGenManager::GetFaceCoordValue(&tempcoord, FANTYPE_GEO, FANSYMM_SYMM, static_cast<uint32_t>(i));
            fval = BSMAX(BSMIN(fval, ShapeEntries[i].fMax), ShapeEntries[i].fMin);
            RaceSexMenu::Entry* popt = pOptionListsA[ShapeEntries[i].eID]->AddSliderOption(
                ShapeEntries[i].pDisplay, RSM_SHAPE_SLIDER, static_cast<int32_t>(ShapeEntries[i].fMin * 10.0),
                static_cast<int32_t>(ShapeEntries[i].fMax * 10.0), static_cast<int32_t>(fval * 10.0), 0.25f
            );
            popt->SetDisplay(" ");
            popt->SetData(i);
        }
    }
    // The texture sliders, one by one in the game's order, with no test of the entry.
    {
        float fval = BSFaceGenManager::GetFaceCoordValue(&tempcoord, FANTYPE_TEX, FANSYMM_SYMM, 29u);
        fval = BSMAX(BSMIN(fval, ToneEntries[29].fMax), ToneEntries[29].fMin);
        RaceSexMenu::Entry* popt = pOptionListsA[ToneEntries[29].eID]->AddSliderOption(
            ToneEntries[29].pDisplay, RSM_TONE_SLIDER, static_cast<int32_t>(ToneEntries[29].fMin * 10.0),
            static_cast<int32_t>(ToneEntries[29].fMax * 10.0), static_cast<int32_t>(fval * 10.0), 0.25f
        );
        popt->SetDisplay(" ");
        popt->SetData(29);
    }
    {
        float fval = BSFaceGenManager::GetFaceCoordValue(&tempcoord, FANTYPE_TEX, FANSYMM_SYMM, 28u);
        fval = BSMAX(BSMIN(fval, ToneEntries[28].fMax), ToneEntries[28].fMin);
        RaceSexMenu::Entry* popt = pOptionListsA[ToneEntries[28].eID]->AddSliderOption(
            ToneEntries[28].pDisplay, RSM_TONE_SLIDER, static_cast<int32_t>(ToneEntries[28].fMin * 10.0),
            static_cast<int32_t>(ToneEntries[28].fMax * 10.0), static_cast<int32_t>(fval * 10.0), 0.25f
        );
        popt->SetDisplay(" ");
        popt->SetData(28);
    }
    {
        float fval = BSFaceGenManager::GetFaceCoordValue(&tempcoord, FANTYPE_TEX, FANSYMM_SYMM, 30u);
        fval = BSMAX(BSMIN(fval, ToneEntries[30].fMax), ToneEntries[30].fMin);
        RaceSexMenu::Entry* popt = pOptionListsA[ToneEntries[30].eID]->AddSliderOption(
            ToneEntries[30].pDisplay, RSM_TONE_SLIDER, static_cast<int32_t>(ToneEntries[30].fMin * 10.0),
            static_cast<int32_t>(ToneEntries[30].fMax * 10.0), static_cast<int32_t>(fval * 10.0), 0.25f
        );
        popt->SetDisplay(" ");
        popt->SetData(30);
    }
    {
        float fval = BSFaceGenManager::GetFaceCoordValue(&tempcoord, FANTYPE_TEX, FANSYMM_SYMM, 31u);
        fval = BSMAX(BSMIN(fval, ToneEntries[31].fMax), ToneEntries[31].fMin);
        RaceSexMenu::Entry* popt = pOptionListsA[ToneEntries[31].eID]->AddSliderOption(
            ToneEntries[31].pDisplay, RSM_TONE_SLIDER, static_cast<int32_t>(ToneEntries[31].fMin * 10.0),
            static_cast<int32_t>(ToneEntries[31].fMax * 10.0), static_cast<int32_t>(fval * 10.0), 0.25f
        );
        popt->SetDisplay(" ");
        popt->SetData(31);
    }
    {
        float fval = BSFaceGenManager::GetFaceCoordValue(&tempcoord, FANTYPE_TEX, FANSYMM_SYMM, 8u);
        fval = BSMAX(BSMIN(fval, ToneEntries[8].fMax), ToneEntries[8].fMin);
        RaceSexMenu::Entry* popt = pOptionListsA[ToneEntries[8].eID]->AddSliderOption(
            ToneEntries[8].pDisplay, RSM_TONE_SLIDER, static_cast<int32_t>(ToneEntries[8].fMin * 10.0),
            static_cast<int32_t>(ToneEntries[8].fMax * 10.0), static_cast<int32_t>(fval * 10.0), 0.25f
        );
        popt->SetDisplay(" ");
        popt->SetData(8);
    }
    {
        float fval = BSFaceGenManager::GetFaceCoordValue(&tempcoord, FANTYPE_TEX, FANSYMM_SYMM, 9u);
        fval = BSMAX(BSMIN(fval, ToneEntries[9].fMax), ToneEntries[9].fMin);
        RaceSexMenu::Entry* popt = pOptionListsA[ToneEntries[9].eID]->AddSliderOption(
            ToneEntries[9].pDisplay, RSM_TONE_SLIDER, static_cast<int32_t>(ToneEntries[9].fMin * 10.0),
            static_cast<int32_t>(ToneEntries[9].fMax * 10.0), static_cast<int32_t>(fval * 10.0), 0.25f
        );
        popt->SetDisplay(" ");
        popt->SetData(9);
    }
    {
        float fval = BSFaceGenManager::GetFaceCoordValue(&tempcoord, FANTYPE_TEX, FANSYMM_SYMM, 18u);
        fval = BSMAX(BSMIN(fval, ToneEntries[18].fMax), ToneEntries[18].fMin);
        RaceSexMenu::Entry* popt = pOptionListsA[ToneEntries[18].eID]->AddSliderOption(
            ToneEntries[18].pDisplay, RSM_TONE_SLIDER, static_cast<int32_t>(ToneEntries[18].fMin * 10.0),
            static_cast<int32_t>(ToneEntries[18].fMax * 10.0), static_cast<int32_t>(fval * 10.0), 0.25f
        );
        popt->SetDisplay(" ");
        popt->SetData(18);
    }
    {
        float fval = BSFaceGenManager::GetFaceCoordValue(&tempcoord, FANTYPE_TEX, FANSYMM_SYMM, 27u);
        fval = BSMAX(BSMIN(fval, ToneEntries[27].fMax), ToneEntries[27].fMin);
        RaceSexMenu::Entry* popt = pOptionListsA[ToneEntries[27].eID]->AddSliderOption(
            ToneEntries[27].pDisplay, RSM_TONE_SLIDER, static_cast<int32_t>(ToneEntries[27].fMin * 10.0),
            static_cast<int32_t>(ToneEntries[27].fMax * 10.0), static_cast<int32_t>(fval * 10.0), 0.25f
        );
        popt->SetDisplay(" ");
        popt->SetData(27);
    }
    {
        float fval = BSFaceGenManager::GetFaceCoordValue(&tempcoord, FANTYPE_TEX, FANSYMM_SYMM, 22u);
        fval = BSMAX(BSMIN(fval, ToneEntries[22].fMax), ToneEntries[22].fMin);
        RaceSexMenu::Entry* popt = pOptionListsA[ToneEntries[22].eID]->AddSliderOption(
            ToneEntries[22].pDisplay, RSM_TONE_SLIDER, static_cast<int32_t>(ToneEntries[22].fMin * 10.0),
            static_cast<int32_t>(ToneEntries[22].fMax * 10.0), static_cast<int32_t>(fval * 10.0), 0.25f
        );
        popt->SetDisplay(" ");
        popt->SetData(22);
    }
    {
        float fval = BSFaceGenManager::GetFaceCoordValue(&tempcoord, FANTYPE_TEX, FANSYMM_SYMM, 5u);
        fval = BSMAX(BSMIN(fval, ToneEntries[5].fMax), ToneEntries[5].fMin);
        RaceSexMenu::Entry* popt = pOptionListsA[ToneEntries[5].eID]->AddSliderOption(
            ToneEntries[5].pDisplay, RSM_TONE_SLIDER, static_cast<int32_t>(ToneEntries[5].fMin * 10.0),
            static_cast<int32_t>(ToneEntries[5].fMax * 10.0), static_cast<int32_t>(fval * 10.0), 0.25f
        );
        popt->SetDisplay(" ");
        popt->SetData(5);
    }
    {
        float fval = BSFaceGenManager::GetFaceCoordValue(&tempcoord, FANTYPE_TEX, FANSYMM_SYMM, 6u);
        fval = BSMAX(BSMIN(fval, ToneEntries[6].fMax), ToneEntries[6].fMin);
        RaceSexMenu::Entry* popt = pOptionListsA[ToneEntries[6].eID]->AddSliderOption(
            ToneEntries[6].pDisplay, RSM_TONE_SLIDER, static_cast<int32_t>(ToneEntries[6].fMin * 10.0),
            static_cast<int32_t>(ToneEntries[6].fMax * 10.0), static_cast<int32_t>(fval * 10.0), 0.25f
        );
        popt->SetDisplay(" ");
        popt->SetData(6);
    }
    {
        float fval = BSFaceGenManager::GetFaceCoordValue(&tempcoord, FANTYPE_TEX, FANSYMM_SYMM, 3u);
        fval = BSMAX(BSMIN(fval, ToneEntries[3].fMax), ToneEntries[3].fMin);
        RaceSexMenu::Entry* popt = pOptionListsA[ToneEntries[3].eID]->AddSliderOption(
            ToneEntries[3].pDisplay, RSM_TONE_SLIDER, static_cast<int32_t>(ToneEntries[3].fMin * 10.0),
            static_cast<int32_t>(ToneEntries[3].fMax * 10.0), static_cast<int32_t>(fval * 10.0), 0.25f
        );
        popt->SetDisplay(" ");
        popt->SetData(3);
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
    return ((uiFlags & (1 << ((int8_t)aeFlg))) != 0);
}

void RaceSexMenu::SetFlag(Flag aeFlg, bool abOn) {
    uint32_t v3;
    if (abOn) {
        v3 = uiFlags | (1 << ((int8_t)aeFlg));
    }
    else {
        v3 = uiFlags & (~(1 << ((int8_t)aeFlg)));
    }
    uiFlags = v3;
}

void RaceSexMenu::SynchronizeFaceSliders(RaceSexMenu::OptionID aeSliderType) {
    uint32_t v5 = 0;
    if (aeSliderType == RaceSexMenu::RSM_SHAPE_SLIDER) {
        uint32_t v2;
        do {
            if (!UpdateEntriesBetween(10, 19, static_cast<FanTypeE>(0))) {
                break;
            }
            v2 = v5++;
        }
        while (v2 < 3);
    }
    else {
        if (aeSliderType == RaceSexMenu::RSM_TONE_SLIDER) {
            uint32_t v3;
            do {
                if (!UpdateEntriesBetween(19, 20, static_cast<FanTypeE>(1))) {
                    break;
                }
                v3 = v5++;
            }
            while (v3 < 3);
        }
    }
}

void RaceSexMenu::UpdatePlayerHead(bool abChangeRace, bool abShape, bool abTone) {
    if ((pPlayer->GetFaceNodeBiped(nullptr)) && (pPlayer->GetFaceNodeSkinned(nullptr))) {
        pPlayerBase->AddChange(CHANGE_NPC_FACE);
        if (abChangeRace) {
            NiNode* proot;
            BSFaceGenNiNode* pbiped;
            NiAVObject* pobj = pPlayer->Get3D(false);
            if (pobj) {
                proot = pobj->IsNode();
            }
            else {
                proot = nullptr;
            }
            if (proot) {
                NiAVObjectPalette* ptmpPal = nullptr;
                if (pPlayer->GetAnimation(false)) {
                    Animation* Animation_0 = pPlayer->GetAnimation(false);
                    if (Animation_0->GetControllerManager()) {
                        Animation* v5 = pPlayer->GetAnimation(false);
                        NiControllerManager* ControllerManager = v5->GetControllerManager();
                        ptmpPal = ControllerManager->GetObjectPalette();
                    }
                }
                NiDefaultAVObjectPalette* ppal = static_cast<NiDefaultAVObjectPalette*>(ptmpPal);
                NiAVObject* premove = (NiAVObject*)pPlayer->GetFaceNodeSkinned(nullptr);
                if ((premove) && (premove->m_pkParent)) {
                    RecurseAndRemoveObjectsFromPalette(premove, ppal);
                    BaseProcess* CurrentAIProcess = pPlayer->pCurrentProcess;
                    CurrentAIProcess->SetFaceSkinnedNode(nullptr);
                    NiNode* Parent = premove->m_pkParent;
                    Parent->DetachChild(premove);
                }

                premove = (NiAVObject*)pPlayer->GetFaceNodeBiped(nullptr);
                if ((premove) && (premove->m_pkParent)) {
                    RecurseAndRemoveObjectsFromPalette(premove, ppal);
                    BaseProcess* v21 = pPlayer->pCurrentProcess;
                    v21->SetFaceNode(nullptr);
                    BaseProcess* v20 = pPlayer->pCurrentProcess;
                    v20->SetHeadAnims(nullptr);
                    NiNode* v19 = premove->m_pkParent;
                    v19->DetachChild(premove);
                }
            }
            if (pPlayer->pCurrentProcess) {
                BaseProcess* v18 = pPlayer->pCurrentProcess;
                v18->ClearFaceAnimationData();
            }
            pPlayerBase->ClearHead();
            BSFaceGenNiNode* pskin = nullptr;
            pbiped = nullptr;
            pPlayerBase->InitHead(pbiped, pskin);
            BSFaceGenNiNode* v11 = pskin;
            BSFaceGenNiNode* v10 = pbiped;
            BipedAnim* Biped_0 = pPlayer->GetBiped(false);
            pPlayerBase->AttachHead(pPlayer, Biped_0, v10, v11);
        }
        else {
            FR2MatrixVTC<float> currcord[2][2];
            FaceGenDatas diffcoord;
            pPlayerBase->GetFaceCoord(currcord);
            if (abShape) {
                BSFaceGenManager::OffsetFaceGenCoord(&LastFaceCoord, &currcord, &diffcoord.kFaceGens, false);
            }
            BSFaceGenNiNode* pfacenode = pPlayer->GetFaceNodeBiped(nullptr);
            for (int32_t a = 0; a < 2; ++a) {
                if (abShape) {
                    pfacenode->SetForceMorphUpdates(true);
                }
                uint32_t isizeArray = pfacenode->GetArrayCount();
                for (uint32_t ichild = 0; ichild < isizeArray; ++ichild) {
                    NiTriBasedGeom* ptri;
                    NiAVObject* pobj_2 = pfacenode->GetAt(ichild);
                    if (pobj_2) {
                        ptri = pobj_2->IsTriBasedGeom();
                    }
                    else {
                        ptri = nullptr;
                    }
                    NiTriBasedGeom* ptri_2 = ptri;
                    if (ptri) {
                        BSFaceGenModel* pfgmodel;
                        BSFaceGenModelExtraData* pmodelExtraData = BSFaceGenManager::GetFaceGenModelExtraData(ptri_2);
                        if (pmodelExtraData) {
                            pfgmodel = pmodelExtraData->GetModel();
                        }
                        else {
                            pfgmodel = nullptr;
                        }
                        BSFaceGenModel* pfgmodel_2 = pfgmodel;
                        if (pfgmodel) {
                            if (abShape) {
                                pfgmodel_2->ApplyCoordinateToExistingMesh(&(&diffcoord)->kFaceGens, ptri_2, false);
                            }
                            if (abTone) {
                                uint32_t v12 = std::strlen("FaceGenFace");
                                const char* HandleString = (&ptri_2->m_kName)->GetHandleString();
                                if (!BSstrnicmp(HandleString, "FaceGenFace", v12)) {
                                    int32_t v15;
                                    NiShadeProperty* pshaderprop =
                                        static_cast<NiShadeProperty*>(pobj_2->GetProperty(NiProperty::SHADE));
                                    if (pshaderprop) {
                                        int32_t v14 = pshaderprop->iShaderPropertyType >= 8 &&
                                            pshaderprop->iShaderPropertyType <= 12;
                                        v15 = v14;
                                    }
                                    else {
                                        v15 = 0;
                                    }
                                    BSShaderPPLightingProperty* pshaderpropLighting =
                                        v15 ? static_cast<BSShaderPPLightingProperty*>(pshaderprop) : nullptr;
                                    if (pshaderpropLighting) {
                                        NiPointer<NiTexture> sptexBaseMod(static_cast<NiTexture*>(nullptr));
                                        if (!pfgmodel_2->HasEGTData()) {
                                            pfgmodel_2->ForceLoadModelTexture();
                                        }
                                        uint32_t EGTClampSize = BSFaceGenManager::GetEGTClampSize();
                                        pfgmodel_2->ApplyCoordinateTexturingToMesh(
                                            &currcord, sptexBaseMod, EGTClampSize
                                        );
                                        NiTexture* v13 = sptexBaseMod.m_pObject;
                                        pshaderpropLighting->SetDiffuseTexture(1u, v13);
                                    }
                                }
                            }
                        }
                    }
                }
                pfacenode = pPlayer->GetFaceNodeSkinned(nullptr);
            }
        }
        NiUpdatePlayer3D();
    }
}

void RaceSexMenu::UpdatePlayerBody() {
    NiMatrix3 apRotIn;
    BSFaceGenManager::ClearBodyTexturesFromPalette(static_cast<TESNPC*>(pPlayer->GetObjectReference()));
    bBipedAnimUpdateSkins = true;
    BaseProcess* CurrentAIProcess = pPlayer->pCurrentProcess;
    CurrentAIProcess->ClearFaceAnimationData();
    BaseProcess* v6 = pPlayer->pCurrentProcess;
    v6->Set3DUpdateFlag(static_cast<RESET_3D_FLAGS>(RESET_MODEL | RESET_SKIN));
    BaseProcess* v5 = pPlayer->pCurrentProcess;
    v5->Update3dModel(pPlayer);
    if (!pPlayer->GetFaceNodeSkinned(nullptr) && !pPlayer->GetFaceNodeBiped(nullptr)) {
        QueuedHead apMemory(pPlayerBase, IO_TASK_PRIORITY_CRITICAL);
        apMemory.QueuedHead::Run();
        apMemory.QueuedFile::Finish();
        BipedAnim* Biped_0 = pPlayer->GetBiped(false);
        apMemory.Attach(pPlayer, Biped_0);
    }
    bBipedAnimUpdateSkins = false;
    NiNode* v11 = static_cast<NiNode*>(pPlayer->Get3D(true));
    NiAVObject* v13 = v11->GetAt(0);
    apRotIn = NiMatrix3_IDENTITY;
    v13->SetRotate(pPlayer->MultipleMatrixByRace(apRotIn));
    pIOManager->LoadAllQueuedPriorities();
    NiUpdatePlayer3D();
    FOPipboyManager* Pipboy = Interface::GetPipboy();
    Pipboy->Shutdown();
    FOPipboyManager* v3 = Interface::GetPipboy();
    v3->Initialize();
}

void RaceSexMenu::NiUpdatePlayer3D() {
    InterfaceManager* v3;
    FORenderedMenuRaceSex* currentRenderedMenu;
    NiUpdateData a1;
    NiNode* v5 = static_cast<NiNode*>(pPlayer->Get3D(false));
    ShadowSceneNode* shadowSceneNode = BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_MENU_PLAYER_3D);
    shadowSceneNode->UpdateObjectLighting(v5, false);
    a1.~NiUpdateData();
    ::new (&a1) NiUpdateData(0.0, false, false);
    ShadowSceneNode* v1 = BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_MENU_PLAYER_3D);
    v1->NiAVObject::Update(a1);
    InterfaceManager* singleton = InterfaceManager::Instance();
    if (singleton->GetCurrentRenderedMenu()) {
        v3 = InterfaceManager::Instance();
        currentRenderedMenu = static_cast<FORenderedMenuRaceSex*>(v3->GetCurrentRenderedMenu());
        currentRenderedMenu->SetupLights(1.0);
    }
}

// A null NiPointer that is never used is also constructed and destroyed, as the game does; both pointers are built and
// released on raw stack storage. The five empty strings are unused too, but are built and released as the game does.
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
        pPlayer->GetRace()->GetFaceGenData(
            static_cast<TESNPC*>(pBase), *pNpcData, static_cast<TESActorBase::SEX>(iSex), false
        );
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
                float fG =
                    static_cast<float>(static_cast<int32_t>((static_cast<uint16_t>(uiColor) >> 8) & 0xFF) / 255.0);
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
                    float fG =
                        static_cast<float>(static_cast<int32_t>((static_cast<uint16_t>(uiColor) >> 8) & 0xFF) / 255.0);
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
            pGeom = spGeom.operator->();
            if (!pGeom) {
                continue;
            }
            if (bHair) {
                float fLength = pPlayerBase->GetHairLength();
                pGeom = spGeom.operator->();
                BSFaceGenManager::ApplyHairMorph(pGeom, fLength);
            }
            pGeom = spGeom.operator->();
            NiProperty* pProperty = pGeom->GetProperty(NiProperty::SHADE);
            bool bPPLighting = pProperty &&
                static_cast<NiShadeProperty*>(pProperty)->GetShaderPropertyType() ==
                    BSShaderManager::BSSM_PROPERTYTYPE_PPLIGHTING;
            auto* pPPLighting = static_cast<BSShaderPPLightingProperty*>(bPPLighting ? pProperty : nullptr);
            if (pPPLighting) {
                uint32_t uiColor = pPlayerBase->GetHairColor();
                float fB = static_cast<float>(static_cast<int32_t>((uiColor >> 16) & 0xFF) / 255.0);
                float fG =
                    static_cast<float>(static_cast<int32_t>((static_cast<uint16_t>(uiColor) >> 8) & 0xFF) / 255.0);
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
                    float fG =
                        static_cast<float>(static_cast<int32_t>((static_cast<uint16_t>(uiColor) >> 8) & 0xFF) / 255.0);
                    float fR = static_cast<float>(static_cast<int32_t>(uiColor & 0xFF) / 255.0);
                    alignas(NiColorA) char acColor[sizeof(NiColorA)];
                    pLighting30->pHairTintColor = *::new (acColor) NiColorA(fR, fG, fB, 1.0f);
                }
            }
        }
    }
    NiUpdateData kData;
    kData.~NiUpdateData();
    ::new (&kData) NiUpdateData(0.0f, false, false);
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
    NiMatrix3 result;
    NiMatrix3 mat;
    float rfXAngle;
    float rfZAngle;
    float rfYAngle;
    NiMatrix3 akRot;
    NiAVObject* v3 = (&spPlayerNode)->m_pObject;
    __builtin_memcpy(&akRot, &v3->GetLocalTransform(), sizeof(akRot));
    (&akRot)->ToEulerAnglesXYZ(rfXAngle, rfYAngle, rfZAngle);
    float v15 = rfZAngle - afRadians;
    if (!(v15 < 0.0)) {
        if (v15 > 6.2831854820251465) {
            v15 = (float)(v15 - 6.2831854820251465);
        }
    }
    else {
        v15 = (float)(v15 + 6.2831854820251465);
    }
    if ((v15 == rfZAngle) && (!abForce)) {
        return false;
    }
    (&akRot)->MakeZRotation(v15);
    __builtin_memcpy(&mat, &NiMatrix3::ZERO, sizeof(mat));
    float a4 = pPlayerBase->GetWeight();
    (&mat)->SetEntry(0, 0, a4);
    float a4a = pPlayerBase->GetWeight();
    (&mat)->SetEntry(1u, 1u, a4a);
    float a4b = pPlayerBase->GetHeight();
    (&mat)->SetEntry(2u, 2u, a4b);
    __builtin_memcpy(&akRot, &(result = akRot * mat), sizeof(akRot));
    NiAVObject* v5 = (&spPlayerNode)->m_pObject;
    v5->SetRotate(akRot);
    NiUpdatePlayer3D();
    return true;
}

bool RaceSexMenu::ZoomPlayer(float afDelta, bool abForce) {
    float fZoomPct = this->fZoomPct;
    float v9 = fZoomPct + afDelta;
    float a2 = BSMIN(static_cast<float>(1.0), v9);
    this->fZoomPct = BSMAX(static_cast<float>(0.0), a2);
    if (this->fZoomPct == fZoomPct && !abForce) {
        return false;
    }
    float v8 = pPlayer->GetHeight() * ((ptZoomBound[1].z - ptZoomBound[0].z) * this->fZoomPct + ptZoomBound[0].z);
    float v7 = (ptZoomBound[1].y - ptZoomBound[0].y) * this->fZoomPct + ptZoomBound[0].y;
    float v6 = (ptZoomBound[1].x - ptZoomBound[0].x) * this->fZoomPct + ptZoomBound[0].x;
    NiAVObject* v4 = spPlayerNode.m_pObject;
    v4->SetTranslate(v6, v7, v8);
    NiUpdatePlayer3D();
    return true;
}

void RaceSexMenu::InitHairStyles() {
    pOptionListsA[5]->Flush();
    RaceSexMenu::Entry* v4 = nullptr;
    for (TESHair*& rpItem : (&pDataHandler->listHair)->ItemsUntilNull()) {
        if (rpItem->IsPlayable() && rpItem->CanBeHairForNPC(pPlayerBase)) {
            RaceSexMenu::Entry* v2 = pOptionListsA[5]->AddBasicOption(rpItem->GetFullName(), 27);
            v2->data = rpItem;
            if (!v4 && RaceSexMenu::IsPlayerHair(v2)) {
                v4 = v2;
            }
        }
    }
    UpdateScrollArrows();
    while (v4 && (v4 != pOptionListsA[5]->selectedEntry && pOptionListsA[5]->HandleKey(2))) {
        ;
    }
    pOptionListsA[5]->UpdateCurrentSelection();
    SetFlag(RaceSexMenu::RSM_FLAG_MAKE_NEW_HAIRLIST, false);
}

void RaceSexMenu::ValidateHairOptionList() {
    bool v4 = pOptionListsA[3]->GetSliderWithID(7) != nullptr;
    TESActorBase::SEX Sex = pPlayerBase->GetSex();
    bool v5 = Sex == TESActorBase::SEX_MALE;
    if (Sex || v4) {
        if (v4 && !v5) {
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
    List* plist = pOptionListsA[GetActiveList()];
    NiTListItem<RaceSexMenu::Entry*>* pos = plist->items.GetHeadPos();
    bool bupvis = false;
    bool bdownvis = false;
    Tile::UpdateAll(false);
    while (pos && (!bupvis || !bdownvis)) {
        RaceSexMenu::Entry* popt = plist->items.GetNext(pos);
        int32_t Value = popt->GetValue();
        if (Value == -1) {
            bupvis = true;
        }
        else {
            if (Value == 1) {
                bdownvis = true;
            }
        }
    }
    rsmScrollUpTarget->Set(Tile::kTileValue_Visible, static_cast<int32_t>(bupvis));
    rsmScrollDownTarget->Set(Tile::kTileValue_Visible, static_cast<int32_t>(bdownvis));
}

void RaceSexMenu::OnFaceSliderChange(RaceSexMenu::Entry* apChangedSlider) {
    FR2MatrixVTC<float> addr[2][2];
    pPlayerBase->GetFaceCoord(addr);
    float ffacevalue = static_cast<double>(apChangedSlider->GetValue_007B3740()) / 10.0;
    // A face slider's data pointer holds its FaceGen control ID, cast here to uint32_t.
    uint32_t uiCtrlID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(apChangedSlider->data));
    uint32_t uiID = apChangedSlider->id;
    BSFaceGenManager::SetFaceCoordValue(
        &addr, static_cast<FanTypeE>(uiID != static_cast<uint32_t>(RSM_SHAPE_SLIDER)), FANSYMM_SYMM, uiCtrlID,
        ffacevalue
    );
    pPlayerBase->GetFaceCoord(LastFaceCoord);
    FR2MatrixVTC<float>(*OffsetFaceCoord)[2][2] = pPlayerBase->GetOffsetFaceCoord();
    TESActorBase::SEX Sex = pPlayerBase->GetSex();
    FR2MatrixVTC<float>(*MeanFaceCoord)[2][2] = pPlayerBase->pFormRace->GetMeanFaceCoord(Sex);
    BSFaceGenManager::OffsetFaceGenCoord(MeanFaceCoord, &addr, OffsetFaceCoord, false);
    SetFlag(RaceSexMenu::RSM_FLAG_UPDATE_HEAD, true);
    if (apChangedSlider->id == static_cast<uint32_t>(RSM_TONE_SLIDER)) {
        SetFlag(RaceSexMenu::RSM_FLAG_UPDATE_BODY, true);
    }
}

void RaceSexMenu::OnHairColorSliderChange(RaceSexMenu::Entry* apChangedSlider) {
    Setting* pclabel;
    Setting* picolor;
    if (apChangedSlider) {
        if (apChangedSlider->id == 29) {
            pclabel = nullptr;
            picolor = nullptr;
            BSStringT<char> strGameSetting;
            strGameSetting.SPrintF("iHairColor%02d", apChangedSlider->GetValue_007B3740());
            char* v6 = strGameSetting.pString;
            GameSettingCollection* Collection = SettingT<GameSettingCollection>::GetCollection();
            bool bgoodgs = Collection->GetSetting(v6, &picolor);
            strGameSetting.SPrintF("sHairColor%d", apChangedSlider->GetValue_007B3740());
            char* v7 = strGameSetting.pString;
            GameSettingCollection* v3 = SettingT<GameSettingCollection>::GetCollection();
            bgoodgs = v3->GetSetting(v7, &pclabel);
            const char* v4 = pclabel->String();
            apChangedSlider->SetDisplay(v4);
            // The setting's int is the packed colour SetHairColorSliders takes, passed unchanged.
            SetHairColorSliders(static_cast<uint32_t>(picolor->Int()));
        }
        else {
            const char* v8 = sRSMCustom.String();
            RaceSexMenu::Entry* SliderWithID = pOptionListsA[6]->GetSliderWithID(29);
            SliderWithID->SetDisplay(v8);
        }
        ChangeHairColor();
    }
}

void RaceSexMenu::SetHairColorSliders(uint32_t aiColor) {
    NiTListItem<RaceSexMenu::Entry*>* piter = pOptionListsA[6]->items.m_pkHead;
    while (piter) {
        RaceSexMenu::Entry* popt = pOptionListsA[6]->items.GetNext(piter);
        uint32_t FormHealth_0 = popt->id;
        switch (FormHealth_0) {
            case 30u: {
                popt->SetValue(static_cast<uint8_t>(aiColor));
                break;
            }
            case 31u: {
                popt->SetValue(static_cast<uint8_t>(aiColor >> 8));
                break;
            }
            case 32u: {
                popt->SetValue(static_cast<uint8_t>(aiColor >> 16));
                break;
            }
        }
    }
}

char RaceSexMenu::UpdateEntriesBetween(int32_t aiStart, int32_t aiEnd, FanTypeE a3) {
    char bChanged = 0;
    for (int32_t i = aiStart; i < aiEnd; ++i) {
        NiTListItem<RaceSexMenu::Entry*>* piter = pOptionListsA[i]->items.m_pkHead;
        while (piter) {
            RaceSexMenu::Entry* popt = pOptionListsA[i]->items.GetNext(piter);
            if ((popt) && (popt->id == 34 || popt->id == 35)) {
                int32_t aiLow;
                int32_t aiHigh;
                FR2MatrixVTC<float> a1[2][2];
                FR2MatrixVTC<float>(*OffsetFaceCoord)[2][2] = pPlayerBase->GetOffsetFaceCoord();
                TESActorBase::SEX Sex = pPlayerBase->GetSex();
                FR2MatrixVTC<float>(*MeanFaceCoord)[2][2] = pPlayerBase->pFormRace->GetMeanFaceCoord(Sex);
                BSFaceGenManager::MergeFaceGenCoord(MeanFaceCoord, OffsetFaceCoord, &a1, false, 0.0f);
                // The entry's data is the FaceGen control index.
                int32_t aiVal = static_cast<int32_t>(
                    BSFaceGenManager::GetFaceCoordValue(&a1, a3, FANSYMM_SYMM, static_cast<uint32_t>(popt->iData)) *
                    10.0
                );
                popt->GetRange(aiLow, aiHigh);
                if (aiVal >= aiLow) {
                    if (aiVal <= aiHigh) {
                        if (popt != pSliderOpt) {
                            popt->SetValue(aiVal);
                        }
                    }
                    else {
                        popt->SetValue(aiHigh);
                        bChanged = 1;
                    }
                }
                else {
                    popt->SetValue(aiLow);
                    bChanged = 1;
                }
            }
        }
    }
    return bChanged;
}

void RaceSexMenu::ValidateHairAndEyes() {
    TESNPC* pPlayerBase = this->pPlayerBase;
    TESHair* Hair = pPlayerBase->GetHair();
    if (!Hair->CanBeHairForNPC(pPlayerBase)) {
        TESActorBase::SEX Sex = this->pPlayerBase->GetSex();
        TESRace* Race = this->pPlayerBase->GetRace();
        TESHair* pdefaultHair = Race->GetDefaultHair(Sex);
        for (BSSimpleList<TESHair*>* pnode = &this->pPlayerBase->GetRace()->HairList;
             !pdefaultHair && (pnode && pnode->m_item); pnode = pnode->m_pkNext) {
            TESHair** v3 = &pnode->m_item;
            if ((*v3)->IsPlayable()) {
                TESHair** v4;
                TESHair** v5;
                if ((this->pPlayerBase->GetSex() == TESActorBase::SEX_MALE &&
                     (v4 = &pnode->m_item, (*v4)->CanBeMale())) ||
                    (this->pPlayerBase->GetSex() == TESActorBase::SEX_FEMALE &&
                     (v5 = &pnode->m_item, (*v5)->CanBeFemale()))) {
                    pdefaultHair = pnode->m_item;
                }
            }
        }
        this->pPlayerBase->SetHair(pdefaultHair);
        UpdatePlayerHair(true);
    }
    if (!this->pPlayerBase->GetEyeColor()->CanBeEyesForNPC(this->pPlayerBase)) {
        TESRace* v6 = this->pPlayerBase->GetRace();
        TESEyes* DefaultEyeColor = v6->GetDefaultEyeColor();
        this->pPlayerBase->SetEyeColor(DefaultEyeColor);
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
        BSFaceGenNiNode* v2 = pPlayer->GetFaceNodeBiped(nullptr);
        uint32_t isizeArray = v2->GetArrayCount();
        for (uint32_t ichild = 0; ichild < isizeArray; ++ichild) {
            BSFaceGenNiNode* v3 = pPlayer->GetFaceNodeBiped(nullptr);
            NiAVObject* pobj = v3->GetAt(ichild);
            const char* HandleString = (&pobj->m_kName)->GetHandleString();
            if (!BSstrcmp(HandleString, "FaceGenEyeLeft")) {
                spLeftEye = pobj->IsTriBasedGeom();
            }
            const char* v5 = (&pobj->m_kName)->GetHandleString();
            if (!BSstrcmp(v5, "FaceGenEyeRight")) {
                spRightEye = pobj->IsTriBasedGeom();
            }
        }
        if ((spLeftEye.m_pObject) && (spRightEye.m_pObject)) {
            BSStringT<char> filename;
            if (pPlayerBase->GetEyeColor()) {
                filename.SPrintF("Textures\\%s", pPlayerBase->GetEyeColor()->GetTextureName());
            }
            else {
                filename.SPrintF("Textures\\Characters\\Eyes\\EyeDefault.dds");
            }
            NiPointer<NiSourceTexture> sptexBase(static_cast<NiSourceTexture*>(nullptr));
            char* v6 = filename.pString;
            TES::pInstance->CreateTextureImage(v6, sptexBase, false, false);
            if (sptexBase.m_pObject) {
                int32_t v8;
                int32_t v10;
                NiShadeProperty* pprop =
                    static_cast<NiShadeProperty*>(spLeftEye.m_pObject->GetProperty(NiProperty::SHADE));
                if (pprop) {
                    int32_t v9 = pprop->iShaderPropertyType >= 8 && pprop->iShaderPropertyType <= 12;
                    v10 = v9;
                }
                else {
                    v10 = 0;
                }
                BSShaderPPLightingProperty* pLighting = v10 ? static_cast<BSShaderPPLightingProperty*>(pprop) : nullptr;
                if (pLighting) {
                    pLighting->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, sptexBase.m_pObject);
                }
                NiShadeProperty* pprop_2 =
                    static_cast<NiShadeProperty*>(spRightEye.m_pObject->GetProperty(NiProperty::SHADE));
                if (pprop_2) {
                    int32_t v7 = pprop_2->iShaderPropertyType >= 8 && pprop_2->iShaderPropertyType <= 12;
                    v8 = v7;
                }
                else {
                    v8 = 0;
                }
                BSShaderPPLightingProperty* pLighting_2 =
                    v8 ? static_cast<BSShaderPPLightingProperty*>(pprop_2) : nullptr;
                if (pLighting_2) {
                    pLighting_2->SetTexture(BSShaderProperty::TEXTYPE_DIFFUSE, 0, sptexBase.m_pObject);
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
    NiTListItem<RaceSexMenu::Entry*>* piter = pOptionListsA[6]->items.m_pkHead;
    while (piter) {
        RaceSexMenu::Entry* popt = pOptionListsA[6]->items.GetNext(piter);
        uint32_t FormHealth_0 = popt->id;
        switch (FormHealth_0) {
            case 30u: {
                iRed = popt->GetValue_007B3740();
                break;
            }
            case 31u: {
                iGreen = popt->GetValue_007B3740();
                break;
            }
            case 32u: {
                iBlue = popt->GetValue_007B3740();
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
    if ((pSliderElem) && (pSliderOpt)) {
        const char* String;
        const char* v10;
        int32_t iid = static_cast<int32_t>(pSliderElem->GetFloat(4010));
        bool bvalueChanged = false;
        if (pSliderOpt->id == 29 &&
            ((iid == 100 || iid == 104) &&
             (v10 = sRSMCustom.String(),
              (String = pSliderElem->pParent->GetString(Tile::kTileValue_User5), !_tcscmp(String, v10))))) {
            bvalueChanged = true;
        }
        else {
            switch (iid) {
                case 100: {
                    int32_t Value_007B3740 = pSliderOpt->GetValue_007B3740();
                    uint32_t Increment = pSliderOpt->GetIncrement();
                    bvalueChanged = pSliderOpt->SetValue(Value_007B3740 - Increment);
                    break;
                }
                case 104: {
                    int32_t v4 = pSliderOpt->GetValue_007B3740();
                    uint32_t v5 = pSliderOpt->GetIncrement();
                    bvalueChanged = pSliderOpt->SetValue(v5 + v4);
                    break;
                }
                case 102: {
                    int32_t v6 = pSliderOpt->GetValue_007B3740();
                    int32_t Float = static_cast<int32_t>(pSliderElem->pParent->GetFloat(Tile::kTileValue_User4));
                    bvalueChanged = pSliderOpt->SetValue(v6 - Float);
                    break;
                }
                case 103: {
                    int32_t v8 = pSliderOpt->GetValue_007B3740();
                    int32_t v9 = static_cast<int32_t>(pSliderElem->pParent->GetFloat(Tile::kTileValue_User4));
                    bvalueChanged = pSliderOpt->SetValue(v9 + v8);
                    break;
                }
                case 105: {
                    bvalueChanged = IsFlagged(RaceSexMenu::RSM_FLAG_SLIDER_UPDATE);
                    break;
                }
            }
        }
        if (bvalueChanged) {
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
        int32_t iPreset = pSliderOpt->GetValue_007B3740();
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
        RaceSexMenu::Entry* popt = pOptionListsA[6]->GetSliderWithID(RaceSexMenu::RSM_HAIR_COLOR_PRESET);
        if (popt) {
            int32_t ilow;
            int32_t ihigh;
            const char* v3 = sRSMCustom.String();
            popt->SetDisplay(v3);
            popt->GetRange(ilow, ihigh);
            popt->SetValue(ilow);
        }
        SetFlag(RaceSexMenu::RSM_FLAG_MAKE_NEW_HAIRLIST, true);
    }
    else if (pSliderOpt->id == RaceSexMenu::RSM_AGE_FACE) {
        float fvalNewTex_2;
        float v11;
        float v12;
        float v14;
        // iAge holds the age, then the clamped geometry age.
        int32_t iAge = static_cast<int32_t>(
            static_cast<double>(pSliderOpt->GetValue_007B3740()) * 5.550000190734863 + 9.449999809265137
        );
        FR2MatrixVTC<float> aCoord[2][2];
        FR2MatrixVTC<float> aOffsetCoord[2][2];
        BSFaceGenManager::InitFaceGenCoord(&aOffsetCoord);
        pPlayerBase->GetFaceCoord(aCoord);
        pPlayerBase->GetFaceCoord(LastFaceCoord);
        float fvalOldGeo = BSFaceGenManager::GetFaceCoordAttribute(aCoord, FANATT_AGE, FANTYPE_GEO);
        float fvalOldTex = BSFaceGenManager::GetFaceCoordAttribute(aCoord, FANATT_AGE, FANTYPE_TEX);
        float v18 = fvalOldTex - (fvalOldGeo - static_cast<double>(iAge));
        if (!(static_cast<double>(iAge) > 15.0)) {
            v14 = 15.0f;
        }
        else {
            v14 = static_cast<float>(iAge);
        }
        if (!(v14 < 65.0)) {
            v12 = 65.0f;
        }
        else {
            float v13;
            if (!(static_cast<double>(iAge) > 15.0)) {
                v13 = 15.0f;
            }
            else {
                v13 = static_cast<float>(iAge);
            }
            v12 = v13;
        }
        int32_t fvalNewGeo = static_cast<int32_t>(v12);
        if (!(v18 > 15.0)) {
            v11 = 15.0f;
        }
        else {
            v11 = v18;
        }
        if (!(v11 < 65.0)) {
            fvalNewTex_2 = 65.0f;
        }
        else {
            float fvalNewTex;
            if (!(v18 > 15.0)) {
                fvalNewTex = 15.0f;
            }
            else {
                fvalNewTex = v18;
            }
            fvalNewTex_2 = fvalNewTex;
        }
        BSFaceGenManager::SetFaceCoordAttribute(aCoord, FANATT_AGE, FANTYPE_GEO, static_cast<float>(fvalNewGeo));
        BSFaceGenManager::SetFaceCoordAttribute(aCoord, FANATT_AGE, FANTYPE_TEX, fvalNewTex_2);
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
    int32_t aiLow;
    int32_t aiHigh;
    MakePresetHeadList();
    pSliderOpt = pOptionListsA[2]->GetSliderWithID(24);
    pSliderOpt->GetRange(aiLow, aiHigh);
    pSliderOpt->SetValue(aiLow);
    SliderRelease();
    pSliderOpt = nullptr;
}

Tile* RaceSexMenu::Create(RaceSexMenuContext aeContext) {
    int32_t ClassStatic = RaceSexMenu::GetClassStatic();
    Tile* pold_menu_tile = Menu::GetMenuByClass(ClassStatic);
    if (pold_menu_tile) {
        return pold_menu_tile;
    }
    InterfaceManager* v3 = InterfaceManager::Instance();
    Tile* ptile = v3->GetMenusRoot()->ReadFile("Data\\Menus\\CharGen\\race_sex_menu.xml");
    if (ptile) {
        RaceSexMenu::pMe = static_cast<RaceSexMenu*>(ptile->GetMenu());
        RaceSexMenu::pMe->SetMenuTile(static_cast<TileMenu*>(ptile), false);
        if (!RaceSexMenu::strDoneCaps.pString) {
            const char* v4 = sDone.String();
            RaceSexMenu::strDoneCaps = v4;
            const char* v5 = sNext.String();
            RaceSexMenu::strNextCaps = v5;
            const char* v6 = sBack.String();
            RaceSexMenu::strBackCaps = v6;
            if (GetCurrentTexLanguage() != LANGUAGE_SPANISH) {
                RaceSexMenu::strDoneCaps.MakeUpper();
                RaceSexMenu::strNextCaps.MakeUpper();
                RaceSexMenu::strBackCaps.MakeUpper();
            }
        }
        RaceSexMenu::bWantedWeaponOut = pPlayer->GetWantWeaponDrawn();
        pPlayer->SetWantWeaponDrawn(false);
        InventoryChanges* pinv = InventoryChanges::GetInventoryChanges(pPlayer);
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
                    RaceSexMenu::pEquippedInventoryListA[i] = pinv->GetWornItem(static_cast<BIPED_OBJECT>(i), false);
                    if (RaceSexMenu::pEquippedInventoryListA[i]) {
                        TESBoundObject* FormFlags = RaceSexMenu::pEquippedInventoryListA[i]->pContainerObj;
                        pPlayer->UnEquipObject(FormFlags, 1, nullptr, false, false, true);
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
            FORenderedMenuRaceSex* prm;
            Tile::UpdateAll(false);
            FORenderedMenuRaceSex* apMemory =
                static_cast<FORenderedMenuRaceSex*>(ourMalloc(sizeof(FORenderedMenuRaceSex)));
            if (apMemory) {
                prm = ::new (apMemory) FORenderedMenuRaceSex;
            }
            else {
                prm = nullptr;
            }
            prm->m_Context = aeContext;
            prm->pMenuTile = ptile;
            prm->SetMenuRoot(ptile->GetModel());
            InterfaceManager* v7 = InterfaceManager::Instance();
            v7->EnterRenderedMenu(prm);
        }
    }
    return ptile;
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
    bool v3 = false;
    if ((apOpt) && (apOpt->data) && (RaceSexMenu::pMe) && (RaceSexMenu::pMe->pPlayerBase)) {
        TESEyes* EyeColor = RaceSexMenu::pMe->pPlayerBase->GetEyeColor();
        if (EyeColor == apOpt->data) {
            return true;
        }
    }

    return v3;
}

void RaceSexMenu::RandomizeCallback() {
    if ((RaceSexMenu::pMe) &&
        ((Interface::GetMessageMenuResult() == 1) || RaceSexMenu::pMe->IsFlagged(RSM_FLAG_RANDOM_HEAD))) {
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
                RaceSexMenu::ListID ActiveList = RaceSexMenu::pMe->GetActiveList();
                if (!RaceSexMenu::pMe->pOptionListsA[ActiveList]->Focus(1)) {
                    if (RaceSexMenu::pMe->rsmNextButton->IsVisible()) {
                        TileImage* rsmNextButton = RaceSexMenu::pMe->rsmNextButton;
                        InterfaceManager* v1 = InterfaceManager::Instance();
                        v1->SetCurrentFocusTarget(rsmNextButton, 4035, true);
                    }
                    else {
                        RaceSexMenu::ListID v2 = RaceSexMenu::pMe->GetActiveList();
                        RaceSexMenu::pMe->pOptionListsA[v2]->SetActive();
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
                TESTopic* Topic = TESTopic::GetTopic(DT_SERVICE, 4);
                DialogMenu::ServiceEnd(Topic);
            }
            for (int32_t i = 0; i < 20; ++i) {
                if (RaceSexMenu::pEquippedInventoryListA[i]) {
                    TESBoundObject* FormFlags = RaceSexMenu::pEquippedInventoryListA[i]->pContainerObj;
                    pPlayer->EquipObject(FormFlags, 1, nullptr, true, false, true);
                    ItemChange* apThis = RaceSexMenu::pEquippedInventoryListA[i];
                    if (apThis) {
                        delete apThis;
                    }
                    RaceSexMenu::pEquippedInventoryListA[i] = nullptr;
                }
            }
            pPlayer->SetWantWeaponDrawn(RaceSexMenu::bWantedWeaponOut);
            pPlayer->SetAllowEGMCacheClear(true);
        }
    }
}

bool RaceSexMenu_IsPlayerSex(RaceSexMenu::Entry* a1) {
    return a1 &&
        (RaceSexMenu::pMe &&
         (RaceSexMenu::pMe->pPlayerBase &&
          ((RaceSexMenu::pMe->pPlayerBase->GetSex() == TESActorBase::SEX_MALE && a1->id == 21) ||
           (RaceSexMenu::pMe->pPlayerBase->GetSex() == TESActorBase::SEX_FEMALE && a1->id == 22))));
}

bool RaceSexMenu_IsPlayerHeadPart(RaceSexMenu::Entry* a1) {
    bool v3 = false;
    if ((a1) && (RaceSexMenu::pMe) && (RaceSexMenu::pMe->pPlayerBase) &&
        (!RaceSexMenu::pMe->pPlayerBase->GetHeadParts()->IsEmpty())) {
        void** v1 = &RaceSexMenu::pMe->pPlayerBase->GetHeadParts()->m_item;
        if (*v1 == a1->data) {
            return true;
        }
    }

    return v3;
}

FORenderedMenuRaceSex::FORenderedMenuRaceSex()
  : FORenderedMenu(), spButtons(), spBtnGlow(), spGeometry(nullptr), spPlayer3D(nullptr), spPlayerParent(nullptr),
    spClonedPlayerCamera(nullptr), spPlayerLights(nullptr), spPlayer3DParent(nullptr) {}
