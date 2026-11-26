#include "Fallout/Interface/Menus/ComputersMenu.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTStaticFreeList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileMenu.h"
#include "Fallout Shared/BGSNote.h"
#include "Fallout Shared/BGSTerminal.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/TESDescription.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/FORenderedMenu.h"
#include "Fallout/Interface/FORenderedTerminal.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/ListBox.h"
#include "Fallout/Interface/Menus/HackingText.h"
#include "Fallout/Interface/Menus/StatsMenu.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/rtti.h"

#include <cmath>
#include <cstdint>
#include <limits>

ComputersMenu::AnimatingTextManager::AnimatingTextManager() {}

ComputersMenu::AnimatingTextManager::~AnimatingTextManager() {
    FlushList();
}

void ComputersMenu::AnimatingTextManager::Update() {
    bool btextwraps = false;
    while (!xAnimatingText.IsEmpty() && BSGetTickCount() > g_computersNextMainMenuTextDisplayTime) {
        if (!xAnimatingText.GetHead()->bIsTextRemaining) {
            if (ComputersMenu::pMe->eMode || *xAnimatingText.GetHead()->GetFullText() != "> "[0]) {
                xAnimatingText.GetHead()->Update();
                ComputersMenu::pMe->Sounds[1].Play(false);
            }
            else {
                uint32_t ioldlen = BSstrlen(xAnimatingText.GetHead()->pCurrentlyDisplayedText);
                xAnimatingText.GetHead()->Update();
                uint32_t inewlen = BSstrlen(xAnimatingText.GetHead()->pCurrentlyDisplayedText);
                for (int32_t j = 0; j < inewlen - ioldlen; ++j) {
                    uint32_t uiDisplayRate = xAnimatingText.GetHead()->uiDisplayRate;
                    int32_t itime = j * uiDisplayRate / (inewlen - ioldlen);
                    uint32_t uiDelay = itime;
                    BSAudioManager::QInstance().GetSoundHandleByName("UIHackingCharSingle", 289u).PlayAfter(uiDelay, 0);
                }
            }
            int32_t ix = 0;
            int32_t iy = 0;
            Tile* pcurrent = xAnimatingText.GetHead()->GetParent();
            btextwraps = pcurrent->GetValueQ(4026) != nullptr;
            for (Tile* ptile = pcurrent; ptile; ptile = ptile->pParent) {
                if (ptile == ComputersMenu::pMe->pTiles[6]->pParent) {
                    break;
                }
                ix += static_cast<int32_t>(ptile->GetFloat(4001));
                iy += static_cast<int32_t>(ptile->GetFloat(4002));
            }
            pcurrent->UpdateTile(false);
            if (ComputersMenu::pMe->lbMenuItems.IsListItem(pcurrent)) {
                pcurrent->UpdateChildren(false);
                double dParentX = static_cast<double>(ix);
                int32_t iTextWidth = Tile::AddUserTrait("_text_width", -1);
                float fX = pcurrent->GetFloat(iTextWidth) + dParentX;
                ComputersMenu::pMe->pTiles[6]->Set(4001, fX, true);
                double dParentY = static_cast<double>(iy);
                int32_t iTextY = Tile::AddUserTrait("_text_y", -1);
                float fY = pcurrent->GetFloat(iTextY) + dParentY;
                ComputersMenu::pMe->pTiles[6]->Set(4002, fY, true);
            }
            else if (!btextwraps) {
                double dParentX = static_cast<double>(ix);
                float fX = pcurrent->GetFloat(4017) + dParentX;
                ComputersMenu::pMe->pTiles[6]->Set(4001, fX, true);
                ComputersMenu::pMe->pTiles[6]->Set(4002, iy);
            }
            break;
        }
        AnimatingText* pold = xAnimatingText.RemoveHead();
        if (ComputersMenu::pMe->eMode) {
            if (pold && pold->GetParent() == ComputersMenu::pMe->pTiles[7]) {
                ComputersMenu::pMe->pTiles[15]->Set(4003, 1);
            }
        }
        else {
            if (pold && *pold->GetFullText() == "> "[0]) {
                ComputersMenu::pMe->Sounds[2].Play(false);
            }

            g_computersNextMainMenuTextDisplayTime = BSGetTickCount() + 500;
        }
        if (pold) {
            delete pold;
        }
    }
    if (xAnimatingText.IsEmpty() || btextwraps) {
        float fWidth = 0.0f;
        if (ComputersMenu::pMe->pTiles[5]->IsVisible()) {
            fWidth = ComputersMenu::pMe->pTiles[5]->GetFloat(4017);
        }
        float fX = ComputersMenu::pMe->pTiles[5]->GetFloat(4001) + fWidth;
        ComputersMenu::pMe->pTiles[6]->Set(4001, fX, true);
        float fY = ComputersMenu::pMe->pTiles[5]->GetFloat(4002);
        ComputersMenu::pMe->pTiles[6]->Set(4002, fY, true);
    }
}

uint32_t ComputersMenu::AnimatingTextManager::GetCount() {
    return xAnimatingText.GetSize();
}

void ComputersMenu::AnimatingTextManager::FlushList() {
    while (!xAnimatingText.IsEmpty()) {
        AnimatingText* pText = xAnimatingText.RemoveHead();
        if (pText) {
            delete pText;
        }
    }
}

void ComputersMenu::AnimatingTextManager::AddItem(
    const char* apString,
    Tile::Value* apStringTrait,
    Tile::Value* apVisTrait,
    int32_t aiWhere
) {
    AnimatingText* pText = new HackingText(apString);
    pText->SetTraits(apStringTrait, apVisTrait);
    if (aiWhere == -1) {
        xAnimatingText.AddTail(pText);
    }
    else {
        NiTListItem<AnimatingText*>* pIter = xAnimatingText.m_pkHead;
        for (int32_t i = 0; pIter && i < aiWhere; ++i) {
            pIter = xAnimatingText.GetNextPos(pIter);
        }
        if (pIter) {
            xAnimatingText.InsertBefore(pIter, pText);
        }
        else {
            xAnimatingText.AddTail(pText);
        }
    }
    pText->SetVisibility(false);
}

void ComputersMenu::AnimatingTextManager::ForceFinish() {
    for (NiTListItem<AnimatingText*>* pPos = xAnimatingText.m_pkHead; pPos;) {
        xAnimatingText.GetNext(pPos)->ForceFinish();
    }
}

void ComputersMenu::AnimatingTextManager::DeleteInstance() {
    AnimatingTextManager* pManager = pInstance;
    if (pManager) {
        delete pManager;
    }
    pInstance = nullptr;
}

ComputersMenu::AnimatingTextManager* ComputersMenu::AnimatingTextManager::GetInstance() {
    if (!ComputersMenu::AnimatingTextManager::pInstance) {
        ComputersMenu::AnimatingTextManager::pInstance = new ComputersMenu::AnimatingTextManager;
    }
    return ComputersMenu::AnimatingTextManager::pInstance;
}

ComputersMenu::ComputersMenu()
  : Menu(), pTiles{}, uiResultDisplayTimeout(0), pCursorVis(nullptr), pTerminal(nullptr), pTerminalRef(nullptr),
    eMode(CMM_INTRO), pNoteText(nullptr), bTutorial(0), pATManager(AnimatingTextManager::GetInstance()) {
    NoteActivationFn[0] = [](ComputersMenu* apThis, BGSNote* apNote) { apThis->PlayNoteSound(apNote); };
    NoteActivationFn[1] = [](ComputersMenu* apThis, BGSNote* apNote) { apThis->DisplayNoteText(apNote); };
    NoteActivationFn[2] = [](ComputersMenu* apThis, BGSNote* apNote) { apThis->ShowImage(apNote); };
    NoteActivationFn[3] = nullptr;
    BSAudioManager* pManager = &BSAudioManager::QInstance();
    Sounds[0] = pManager->GetSoundHandleByName("UIHackingFanHumLP", 49u);
    Sounds[1] = pManager->GetSoundHandleByName("UIHackingCharScroll", 33u);
    Sounds[2] = pManager->GetSoundHandleByName("UIHackingCharEnter", 33u);
}

ComputersMenu::~ComputersMenu() {
    ComputersMenu::pMe = nullptr;
    ComputersMenu::AnimatingTextManager::DeleteInstance();
    for (BSSoundHandle& rSound : Sounds) {
        rSound.Stop();
        rSound.Release();
    }
    SystemColorManager::GetInstance()->SetColor(3, ComputersMenu::SaveColor);
}

void ComputersMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
    if (static_cast<uint32_t>(aiID) < 16) {
        pTiles[aiID] = apTile;
    }
}

void ComputersMenu::DoClick(int32_t aiID, Tile* apTarget) {
    if (!apTarget && aiID >= 0 && aiID < 16) {
        apTarget = pTiles[aiID];
    }
    bool bretainReloadTerminal = false;
    if (aiID == 2) {
        pATManager->ForceFinish();
        if (pNoteText) {
            DisplayNoteText(nullptr);
            bretainReloadTerminal = true;
        }
        else {
            pTiles[2]->Set(4003, 0);
            pTiles[1]->Set(4003, 1);
            if (g_bCurrentMenuEntryHasFlags2) {
                DisplayTerminal(0);
            }
            else {
                if (!pTiles[7]->IsVisible()) {
                    pTiles[15]->Set(4003, 0);
                    Tile::Value* pstringval = pTiles[7]->GetValueQ(4036);
                    Tile::Value* pvisval = pTiles[7]->GetValue(4003);
                    pATManager->AddItem(pstringval->strValue, pstringval, pvisval, -1);
                }
                lbMenuItems.Traverse(&ComputersMenu::Redisplay, 0, std::numeric_limits<int32_t>::max());
                bDisplayFlag = true;
            }
        }
        Sounds[2].Play(false);
    }
    else if (aiID == 13 && eMode < ComputersMenu::CMM_NORMAL) {
        FinishIntro();
    }
    else if (lbMenuItems.IsListItem(apTarget)) {
        Sounds[2].Play(false);
        TERMINAL_MENU_ITEM* pmenuitem = lbMenuItems.GetDataFor(apTarget);
        if (pmenuitem) {
            if (pmenuitem->pSubMenu) {
                pmenuitem->SetSelected(true);
                pmenuitem->RunResultScript(pTerminalRef);
                pTerminal = pmenuitem->pSubMenu;
                DisplayTerminal(1);
            }
            else {
                pmenuitem->SetSelected(true);
                pmenuitem->RunResultScript(pTerminalRef);
                if (pmenuitem->ResultText.String()) {
                    Tile::Value* pvisval = pTiles[5]->GetValue(4003);
                    Tile::Value* pstringval = pTiles[5]->GetValueQ(4036);
                    const char* pResultText = pmenuitem->ResultText.String();
                    pATManager->AddItem(pResultText, pstringval, pvisval, 0);
                    uint32_t uiNow = BSGetTickCount();
                    uiResultDisplayTimeout = 1000 * iComputersResultDisplayTimeout.Int() + uiNow;
                }
                if (pmenuitem->pDisplayNote) {
                    if (pmenuitem->IsAutoAdd() && !pPlayer->HasNote(pmenuitem->pDisplayNote)) {
                        pPlayer->AddNote(pmenuitem->pDisplayNote, false);
                        BSStringT<char> strtmp;
                        strtmp.SPrintF(sComputersAddedNote.String(), pmenuitem->pDisplayNote->GetFullName());
                        Interface::ShowMessage(
                            strtmp.pString,
                            0,
                            "Interface\\Icons\\Message Icons\\glow_message_giftbox.dds",
                            nullptr,
                            2.0f,
                            false
                        );
                    }
                    if (NoteActivationFn[pmenuitem->pDisplayNote->GetNoteType()]) {
                        BGSNote* pDisplayNote = pmenuitem->pDisplayNote;
                        NOTE_TYPE eNoteType = pDisplayNote->GetNoteType();
                        NoteActivationFn[eNoteType](this, pDisplayNote);
                    }
                    g_bCurrentMenuEntryHasFlags2 = pmenuitem->ForcesRedraw();
                    bretainReloadTerminal = true;
                }
                else if (pmenuitem->ForcesRedraw()) {
                    DisplayTerminal(0);
                }
            }
        }
        else {
            DisplayPreviousTerminal();
        }
    }
    if (!bretainReloadTerminal) {
        g_bCurrentMenuEntryHasFlags2 = 0;
    }
}

// eMode is compared signed against CMM_NORMAL.
void ComputersMenu::DoIdle() {
    if (bTutorial && !InterfaceManager::Instance()->xTutorialManager.HasBeenShown(Interface::TUT_TERMINAL)) {
        return;
    }
    if (xFadeState == 2) {
        return;
    }
    pATManager->Update();
    FlashCursor();
    if (!pATManager->GetCount()) {
        if (eMode >= CMM_NORMAL) {
            if (bDisplayFlag) {
                lbMenuItems.Traverse(&ComputersMenu::PostDisplay, 0, std::numeric_limits<int32_t>::max());
                bDisplayFlag = false;
            }
        }
        else if (eMode == CMM_INTRO) {
            g_computersSkipMainMenuTime = BSGetTickCount() + 2000;
            eMode = CMM_WAIT;
        }
        else if (eMode == CMM_WAIT && BSGetTickCount() > static_cast<uint32_t>(g_computersSkipMainMenuTime)) {
            FinishIntro();
        }
    }
    if (uiResultDisplayTimeout && BSGetTickCount() > uiResultDisplayTimeout) {
        uiResultDisplayTimeout = 0;
        pTiles[5]->Set(Tile::kTileValue_Visible, 0);
    }
}

int32_t ComputersMenu::GetClass() {
    return 1057;
}

bool ComputersMenu::DoGamepadUpEvent(int32_t aiButton, float afValue) {
    if (!Interface::IsTopMenuID(Interface::eComputersMenu) || aiButton != 10) {
        return false;
    }
    if (pTiles[2]->IsVisible()) {
        pNoteText = nullptr;
        DoClick(2, nullptr);
    }
    else {
        DisplayPreviousTerminal();
    }
    return true;
}

void ComputersMenu::DisplayPreviousTerminal() {
    TerminalStack.RemoveHead();
    pTerminal = TerminalStack.m_item;
    if (pTerminal) {
        DisplayTerminal(0);
    }
    else {
        ComputersMenu::Close();
    }
}

void ComputersMenu::FlashCursor() {
    static float g_computersCursorBlinkInterval = [&] {
        int32_t iBlinkInterval = Tile::AddUserTrait("_blink_interval", -1);
        return pTiles[6]->GetFloat(iBlinkInterval);
    }();
    if (BSGetTickCount() > g_computersNextCursorBlinkTime && pCursorVis) {
        if (pCursorVis->QFloat() == 1.0) {
            pCursorVis->SetFloat(0.0, true);
        }
        else {
            pCursorVis->SetFloat(1.0, true);
        }
        g_computersNextCursorBlinkTime = static_cast<int64_t>(
            static_cast<double>(BSGetTickCount()) + g_computersCursorBlinkInterval
        );
    }
    else if (!pCursorVis) {
        pCursorVis = pTiles[6]->GetValue(4003);
    }
}

void ComputersMenu::FinishIntro() {
    pTiles[13]->Set(4015, 0);
    pATManager->ForceFinish();
    pTiles[9]->Set(4003, 0);
    pTiles[10]->Set(4003, 0);
    pTiles[11]->Set(4003, 0);
    pTiles[12]->Set(4003, 0);
    DisplayTerminal(1);
    Tile::Value* pvisval = pTiles[3]->GetValue(4003);
    Tile::Value* pstringval = pTiles[3]->GetValueQ(4036);
    const char* pHeader = static_cast<const char*>(sComputersHeader1);
    pATManager->AddItem(pHeader, pstringval, pvisval, 0);
    double dParentWidth = pTiles[3]->pParent->GetFloat(4017);
    pHeader = sComputersHeader1.String();
    float fX = (dParentWidth - static_cast<double>(17 * BSstrlen(pHeader))) / 2.0;
    pTiles[3]->Set(4001, fX, true);
    pvisval = pTiles[4]->GetValue(4003);
    pstringval = pTiles[4]->GetValueQ(4036);
    pHeader = static_cast<const char*>(sComputersHeader2);
    pATManager->AddItem(pHeader, pstringval, pvisval, 1);
    dParentWidth = pTiles[4]->pParent->GetFloat(4017);
    pHeader = sComputersHeader2.String();
    fX = (dParentWidth - static_cast<double>(17 * BSstrlen(pHeader))) / 2.0;
    pTiles[4]->Set(4001, fX, true);
    pvisval = pTiles[0]->GetValue(4003);
    pstringval = pTiles[0]->GetValueQ(4036);
    pHeader = pTerminal->GetServerText();
    pATManager->AddItem(pHeader, pstringval, pvisval, 2);
    dParentWidth = pTiles[0]->pParent->GetFloat(4017);
    pHeader = pTerminal->GetServerText();
    fX = (dParentWidth - static_cast<double>(17 * BSstrlen(pHeader))) / 2.0;
    pTiles[0]->Set(4001, fX, true);
    pvisval = pTiles[8]->GetValue(4003);
    pstringval = pTiles[8]->GetValueQ(4036);
    pATManager->AddItem("> ", pstringval, pvisval, -1);
    eMode = ComputersMenu::CMM_NORMAL;
}

void ComputersMenu::DisplayTerminal(char acAddToStack) {
    if (pTerminal) {
        int32_t iDisplayRate;
        BSStringT<char> strtmp;
        pATManager->ForceFinish();
        // RemoveAll is called through a pointer so that it dispatches virtually; a by-value member call would be
        // devirtualised.
        static_cast<ListBox<TERMINAL_MENU_ITEM*>*>(&lbMenuItems)->RemoveAll();
        if (acAddToStack) {
            TerminalStack.AddHead(pTerminal);
        }
        if (iComputersDisplayRateMenus.Int()) {
            iDisplayRate = 1000u / iComputersDisplayRateMenus.Int();
        }
        else {
            iDisplayRate = 1000;
        }
        g_iComputersDisplayRateForCurrentMenu = iDisplayRate;
        const char* pdesc = pTerminal->GetDescription(pTerminal, DESC_ID);
        Tile::Value* pstringval = pTiles[7]->GetValueQ(4036);
        if (pdesc && *pdesc && (!pTiles[7]->IsVisible() || BSstrcmp(pdesc, pstringval->strValue))) {
            pTiles[15]->Set(4003, 0);
            Tile::Value* pvisval = pTiles[7]->GetValue(4003);
            pATManager->AddItem(pdesc, pstringval, pvisval, -1);
        }

        for (TERMINAL_MENU_ITEM*& rpItem : pTerminal->GetMenuItemList()->ItemsUntilNull()) {
            if (rpItem->Conditions.IsTrue(pTerminalRef, nullptr)) {
                Tile* pnew = lbMenuItems.AddItem(rpItem, nullptr, nullptr, nullptr);
                int32_t iEnabled = Tile::AddUserTrait("_enabled", -1);
                pnew->Set(iEnabled, 0);
                bool bSelected = rpItem->IsSelected();
                pnew->Set(4054, bSelected + 1);
                strtmp.SPrintF("%s%s", "> ", rpItem->ResponseText.String());
                Tile::Value* pvisval = pnew->GetValueQ(4003);
                Tile::Value* pstringval = pnew->GetValueQ(Tile::kTileValue_User0);
                pATManager->AddItem(strtmp.pString, pstringval, pvisval, -1);
            }
            rpItem->SetSelected(false);
        }
        if (TerminalStack.m_pkNext) {
            Tile* pback = lbMenuItems.AddItem(0, nullptr, nullptr, nullptr);
            int32_t iEnabled = Tile::AddUserTrait("_enabled", -1);
            pback->Set(iEnabled, 0);
            strtmp.SPrintF("%s%s", "> ", sComputersBack.String());
            Tile::Value* pvisval = pback->GetValueQ(4003);
            Tile::Value* pstringval = pback->GetValueQ(Tile::kTileValue_User0);
            pATManager->AddItem(strtmp.pString, pstringval, pvisval, -1);
        }
        bDisplayFlag = true;
    }
}

bool ComputersMenu::IsWellFormedMenu(int32_t& ai) {
    for (ai = 0; ai < 16; ++ai) {
        if (!pTiles[ai]) {
            return false;
        }
    }
    return true;
}

void ComputersMenu::DisplayIntro() {
    uint32_t uiInputRate;
    uint32_t uiDisplayRate;
    uint8_t aucPassword[16];
    BSStringT<char> strtmp;
    if (iComputersDisplayRateMenus.UInt()) {
        uiDisplayRate = 1000 / iComputersDisplayRateMenus.UInt();
    }
    else {
        uiDisplayRate = 1000;
    }
    if (iHackingInputRate.UInt()) {
        uiInputRate = 1000 / iHackingInputRate.UInt();
    }
    else {
        uiInputRate = 1000;
    }
    int32_t iInputRate = uiInputRate;
    g_iComputersDisplayRateForCurrentMenu = uiDisplayRate;
    Tile::Value* pvisval = pTiles[9]->GetValue(4003);
    Tile::Value* pstringval = pTiles[9]->GetValueQ(4036);
    const char* pText = static_cast<const char*>(sHackingIntro01);
    pATManager->AddItem(pText, pstringval, pvisval, -1);
    g_iComputersDisplayRateForCurrentMenu = iInputRate;
    strtmp.SPrintF("%s%s", "> ", sComputersLogon.String());
    pvisval = pTiles[10]->GetValue(4003);
    pstringval = pTiles[10]->GetValueQ(4036);
    pATManager->AddItem(strtmp.pString, pstringval, pvisval, -1);
    g_iComputersDisplayRateForCurrentMenu = uiDisplayRate;
    pvisval = pTiles[11]->GetValue(4003);
    pstringval = pTiles[11]->GetValueQ(4036);
    pText = static_cast<const char*>(sHackingHeader2);
    pATManager->AddItem(pText, pstringval, pvisval, -1);
    g_iComputersDisplayRateForCurrentMenu = iInputRate;
    ourMemset(aucPassword, 0, 16u);
    uint32_t uiPasswordLength = pTerminal->GetHackMiniGameValue(pTerminalRef);
    ourMemset(aucPassword, 42, uiPasswordLength);
    strtmp.SPrintF("%s%s", "> ", aucPassword);
    pvisval = pTiles[12]->GetValue(4003);
    pstringval = pTiles[12]->GetValueQ(4036);
    pATManager->AddItem(strtmp.pString, pstringval, pvisval, -1);
}

void ComputersMenu::PlayNoteSound(BGSNote* apNote) {
    uint32_t uiSoundID = apNote->GetNoteSound()->iFormID;
    BSAudio::QInstance().GetSoundHandleByNumericID(uiSoundID, 257u).Play(false);
}

void ComputersMenu::DisplayNoteText(BGSNote* apNote) {
    int32_t iDisplayRate;
    pATManager->ForceFinish();
    if (iComputersDisplayRateNotes.Int()) {
        iDisplayRate = 1000u / iComputersDisplayRateNotes.Int();
    }
    else {
        iDisplayRate = 1000;
    }
    g_iComputersDisplayRateForCurrentMenu = iDisplayRate;
    pTiles[1]->Set(4003, 0);
    pTiles[2]->Set(4009, 0);
    pTiles[2]->Set(4003, 1);
    if (!this->pNoteText) {
        const char* pDescription = apNote->GetNoteText()->GetDescription(apNote, TNAM_ID);
        strNoteText.Set(pDescription, 0);
        this->pNoteText = strNoteText.pString;
    }
    double dWrapLines = pTiles[14]->GetFloat(4028);
    double dHeaderHeight = pTiles[7]->GetFloat(4016);
    int32_t iHeaderFont = static_cast<int32_t>(pTiles[7]->GetFloat(4025)) - 1;
    int32_t iLines = static_cast<int32_t>(
                         dWrapLines - dHeaderHeight / FontManager::Instance()->GetFont(iHeaderFont)->fFontHeight
                     ) +
        1;
    double dWrapWidth = pTiles[14]->GetFloat(4026);
    Font::TextData kTextData(static_cast<int32_t>(dWrapWidth), 0, 0, iLines, 10);
    const char* pNoteText = this->pNoteText;
    int32_t iNoteFont = static_cast<int32_t>(pTiles[14]->GetFloat(4025)) - 1;
    FontManager::Instance()->GetFont(iNoteFont)->PrepText(pNoteText, kTextData);
    Tile::Value* pvisval = pTiles[14]->GetValue(4003);
    Tile::Value* pstringval = pTiles[14]->GetValueQ(4036);
    pATManager->AddItem(kTextData.xNewText.pString, pstringval, pvisval, -1);
    uint32_t uiLength = strNoteText.GetLength();
    // Takes the unsigned minimum of the character count and uiLength.
    this->pNoteText += BSMIN(static_cast<uint32_t>(kTextData.GetCharCount()), uiLength);
    if (!*this->pNoteText) {
        this->pNoteText = nullptr;
    }
}

void ComputersMenu::ShowImage(BGSNote* apNote) {
    pATManager->ForceFinish();
    pTiles[1]->Set(4003, 0);
    pTiles[2]->Set(4009, 255);
    pTiles[2]->Set(4003, 1);
    pTiles[14]->Set(4003, 0);
    if (pTerminal->HidesWelcomeTextWhenDisplayingImage()) {
        pTiles[7]->Set(4003, 0);
        pTiles[15]->Set(4003, 0);
    }
    const char* pTextureName = apNote->GetNoteImage()->GetTextureName();
    pTiles[2]->Set(4044, pTextureName, true);
}

Tile* ComputersMenu::Create(TESObjectREFR* apTerminalRef) {
    Tile* pold_menu_tile = Menu::GetMenuByClass(ComputersMenu::GetClassStatic());
    if (pold_menu_tile) {
        delete pold_menu_tile;
    }
    SystemColorManager::GetInstance()->GetColor(3, ComputersMenu::SaveColor);
    BGSTerminal* pterminal = static_cast<BGSTerminal*>(apTerminalRef->GetObjectReference());
    if (pterminal->UsesAltColor()) {
        NiColorA kAltColor(1.0f, 1.0f, 1.0f, 0.0f);
        SystemColorManager::GetInstance()->SetColor(3, kAltColor);
    }
    float ftop_depth = Tile::GetMaximumDepth();
    Tile* ptile = InterfaceManager::Instance()->GetMenusRoot()->ReadFile("Data\\Menus\\computers_menu.xml");
    Menu* pmenu = ptile->GetMenu();
    if (pmenu) {
        int32_t iMenuClass = pmenu->GetClass();
        if (iMenuClass == ComputersMenu::GetClassStatic()) {
            int32_t imissingIndex;
            pmenu->SetMenuTile(static_cast<TileMenu*>(ptile), false);
            ComputersMenu::pMe = static_cast<ComputersMenu*>(pmenu);
            if (static_cast<ComputersMenu*>(pmenu)->ComputersMenu::IsWellFormedMenu(imissingIndex)) {
                if (apTerminalRef) {
                    if (ptile->GetFloat(4007) == 6006.0 || ptile->GetFloat(4007) == 102.0) {
                        ptile->Set(4013, ftop_depth, true);
                    }
                    int32_t iClass = ComputersMenu::GetClassStatic();
                    ComputersMenu::pMe->bTutorial = InterfaceManager::Instance()->xTutorialManager.ShowMessage(
                        Interface::TUT_TERMINAL,
                        iClass,
                        512u
                    );
                    ComputersMenu::pMe->pTerminalRef = apTerminalRef;
                    ComputersMenu::pMe->pTerminal = pterminal;
                    ComputersMenu::pMe->lbMenuItems.Initialize(
                        ComputersMenu::pMe->pTiles[1],
                        "computers_file_template"
                    );
                    ComputersMenu::pMe->Sounds->Play(false);
                    if (ComputersMenu::pMe->pTerminal->IsUnlocked(ComputersMenu::pMe->pTerminalRef)) {
                        ComputersMenu::pMe->FinishIntro();
                    }
                    else {
                        ComputersMenu::pMe->DisplayIntro();
                    }
                    pmenu->PrepForVisibility(false);
                    return ptile;
                }

                Warning("MENUS: Computers Menu Creation Failed... Missing terminal data ");
                return nullptr;
            }

            Warning("MENUS: Computers Menu Creation Failed... Missing index = #%d", imissingIndex);
            return nullptr;
        }
    }

    if (pmenu && pmenu->pMenu && pmenu) {
        delete pmenu;
    }

    return nullptr;
}

int32_t ComputersMenu::GetClassStatic() {
    return 1057;
}

void ComputersMenu::PostDisplay(Tile* apTile, TERMINAL_MENU_ITEM* apMenuItem) {
    if (apTile) {
        apTile->Set(Tile::AddUserTrait("_enabled", -1), 1);
    }
}

void ComputersMenu::Close() {
    if (ComputersMenu::pMe) {
        PlayTerminalShutdownSound();
        ComputersMenu::pMe->pMenu->Set(4033, 0.75f, true);
        if (InterfaceManager::Instance()->GetCurrentRenderedMenu()) {
            NiNode* pMenuRoot = static_cast<NiNode*>(
                InterfaceManager::Instance()->GetCurrentRenderedMenu()->GetMenuRoot()
            );
            if (pMenuRoot == ComputersMenu::pMe->pMenu->GetModel()) {
                FORenderedMenu* pRenderedMenu = InterfaceManager::Instance()->GetCurrentRenderedMenu();
                FORenderedTerminal* pterm = dynamic_cast<FORenderedTerminal*>(pRenderedMenu);
                pterm->FadeOut();
            }
        }
        ComputersMenu::pMe->pMenu->Set(Tile::kTileValue_DeleteOnFade, 1);
        ComputersMenu::pMe->StartFadeOut();
    }
}

void ComputersMenu::Redisplay(Tile* apTile, TERMINAL_MENU_ITEM* apMenuItem) {
    if (apTile) {
        BSStringT<char> strtmp("> ");
        int32_t idefault = 1;
        bool bgo = true;
        int32_t iEnabled = Tile::AddUserTrait("_enabled", -1);
        apTile->Set(iEnabled, 0);
        if (apMenuItem && apMenuItem->Conditions.IsTrue(nullptr, nullptr)) {
            if (apMenuItem->IsSelected()) {
                idefault = 2;
                apMenuItem->SetSelected(false);
            }
            strtmp += static_cast<const char*>(apMenuItem->ResponseText.pString);
        }
        else if (apMenuItem) {
            bgo = false;
        }
        else {
            strtmp += sComputersBack.String();
        }
        apTile->Set(4054, idefault);
        if (bgo) {
            Tile::Value* pvisval = apTile->GetValueQ(4003);
            Tile::Value* pstringval = apTile->GetValueQ(Tile::kTileValue_User0);
            ComputersMenu::AnimatingTextManager::GetInstance()->AddItem(strtmp.pString, pstringval, pvisval, -1);
        }
    }
}

const char* BGSTerminal::GetServerText() {
    if (Data.cServerType >= 10) {
        return nullptr;
    }
    return s_TerminalServerTexts[Data.cServerType]->String();
}

HackingText::HackingText(const char* apString) {
    init();
    *this = apString;
}

HackingText& HackingText::operator=(const char* apString) {
    strText = apString;
    ResetBuffer();
    return *this;
}

const char* HackingText::GetFullText() {
    return strText.String();
}
