#include "Fallout/Interface/Menus/HackingMenu.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSAudioManager.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTStaticFreeList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkShape.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSMenu/Font.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileMenu.h"
#include "BSShader/BSFadeNode.h"
#include "BSSystem/BSFile.h"
#include "BSSystem/FileFinder.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/BGSEntryPoint.h"
#include "Fallout Shared/BGSPerk.h"
#include "Fallout Shared/BGSTerminal.h"
#include "Fallout Shared/ExperiencePoints.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/TESConditionFunctions.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESHealthForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Dialogue/MenuTopic.h"
#include "Fallout/Interface/FORenderedMenu.h"
#include "Fallout/Interface/FORenderedTerminal.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/ListBox.h"
#include "Fallout/Interface/Menus/ComputersMenu.h"
#include "Fallout/Interface/Menus/CreditsMenu.h"
#include "Fallout/Interface/Menus/DialogMenu.h"
#include "Fallout/Interface/Menus/HUDMainMenu.h"
#include "Fallout/Interface/Menus/HackingText.h"
#include "Fallout/Interface/Menus/RaceSexMenu.h"
#include "Fallout/Interface/Menus/StartMenu.h"
#include "Fallout/Interface/Menus/StatsMenu.h"
#include "Fallout/Interface/Menus/TraitMenu.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9SourceTextureData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "SDK/FaceGen/BSFaceGenBinaryFile.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"

#include <cctype>
#include <cstdint>

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> fHackingMinSkillVeryEasy("fHackingMinSkillVeryEasy:Hacking", 0.0f);
SettingT<INISettingCollection> fHackingMinSkillEasy("fHackingMinSkillEasy:Hacking", 25.0f);
SettingT<INISettingCollection> fHackingMinSkillAverage("fHackingMinSkillAverage:Hacking", 50.0f);
SettingT<INISettingCollection> fHackingMinSkillHard("fHackingMinSkillHard:Hacking", 75.0f);
SettingT<INISettingCollection> fHackingMinSkillVeryHard("fHackingMinSkillVeryHard:Hacking", 100.0f);
BSSimpleList<BSSoundHandle> HackingMenu::listSoundsToPlay;

int32_t get_num_matching_characters(const char* a1, const char* apString) {
    if (!a1 || !apString) {
        return 0;
    }
    int32_t v6 = 0;
    uint32_t v4 = BSstrlen(apString);
    uint32_t v3 = BSstrlen(a1);
    uint32_t v7 = BSMIN(v3, v4);
    for (uint32_t i = 0; i < v7; ++i) {
        if (a1[i] == apString[i]) {
            ++v6;
        }
    }
    return v6;
}

uint8_t Max(uint8_t a1, uint8_t a2) {
    if (((int32_t)a1) <= ((int32_t)a2)) {
        return a2;
    }

    return a1;
}

int32_t RandomCompare(void* a1, void* a2) {
    return 2 * RandomUnsignedInt(2u) - 1;
}

HackingMenu::TerminalIO::TerminalIO() : pTextTile(nullptr), pPromptTile(nullptr) {}

HackingMenu::TerminalIO::TerminalIO(
    const char* apString,
    Tile* apParentTile,
    const char* apTemplateName,
    int32_t aiY,
    uint8_t aucRate
)
  : Animation(apString), pTextTile(nullptr), pPromptTile(nullptr) {
    if (!HackingMenu::pMe) {
        return;
    }
    uint32_t auiRates[3];
    auiRates[0] = iHackingOutputRate.Int();
    auiRates[1] = iHackingInputRate.Int();
    auiRates[2] = iHackingDumpRate.Int();
    float fInterval;
    if (auiRates[aucRate] <= 1u) {
        fInterval = 1000.0f;
    }
    else {
        float fRaw = 1000.0 / static_cast<double>(auiRates[aucRate]);
        fInterval = BSRoundToNearest(fRaw, 1.0f);
    }
    Animation.SetDisplayInterval(static_cast<int64_t>(fInterval));
    pTextTile = HackingMenu::pMe->RenderTemplate(apParentTile, apTemplateName, nullptr);
    if (!pTextTile) {
        return;
    }
    Tile::Value* pVisibility = pTextTile->GetValue(4003);
    Tile::Value* pString = pTextTile->GetValueQ(4036);
    Animation.SetTraits(pString, pVisibility);
    if (aucRate == 1) {
        pPromptTile = HackingMenu::pMe->RenderTemplate(HackingMenu::pMe->pTiles[0], "hacking_intro_template", nullptr);
        pPromptTile->Set(4036, ">", true);
    }
    if (aiY >= 0) {
        pTextTile->Set(4002, aiY);
        int32_t iX = static_cast<int32_t>(HackingMenu::pMe->pMenu->GetFloat(Tile::kTileValue_User3));
        if (pPromptTile) {
            pPromptTile->Set(4001, iX);
            pPromptTile->Set(4002, aiY);
            pTextTile->Set(4001, iX + 17 * BSstrlen(">"));
        }
        else {
            pTextTile->Set(4001, iX);
        }
    }
}

// Animation is destroyed by its own destructor.
HackingMenu::TerminalIO::~TerminalIO() {
    delete pTextTile;
    delete pPromptTile;
}

HackingMenu::WordList::WordList() {}

// The words are heap copies the list owns; the base destructor frees the nodes.
HackingMenu::WordList::~WordList() {
    DeleteAll();
}

HackingMenu::GridRow::GridRow() : ucData(nullptr), iSize(0) {}

HackingMenu::GridRow::~GridRow() {
    ourFree(ucData);
}

void HackingMenu::GridRow::SetSize(uint32_t aiSize) {
    ourFree(ucData);
    iSize = aiSize;
    ucData = static_cast<uint8_t*>(ourMalloc(aiSize));
}

HackingMenu::GridRow& HackingMenu::GridRow::operator=(const HackingMenu::GridRow& aCopy) {
    SetSize(aCopy.iSize);
    ourMemcpy(ucData, aCopy.ucData, iSize);
    return *this;
}

uint32_t HackingMenu::GridRow::GetTally(uint32_t aiVal, uint8_t* apCols) {
    uint8_t v5 = 0;
    for (uint32_t i = 0; i < iSize; ++i) {
        if (ucData[i] == aiVal) {
            apCols[v5++] = i;
        }
    }
    return v5;
}

uint32_t HackingMenu::GridRow::GetMaxTally() {
    uint8_t apDest[32];
    uint8_t v5 = 0;
    ourMemset(apDest, 0, 32u);
    for (uint32_t i = 0; i < iSize; ++i) {
        if (ucData[i] < 32u) {
            ++apDest[ucData[i]];
        }
    }
    for (uint32_t j = 0; j < 32; ++j) {
        v5 = Max(v5, apDest[j]);
    }
    return v5;
}

HackingMenu::Grid::Grid(uint32_t aiSize) : pData(new GridRow[aiSize]), iSize(aiSize) {
    for (uint32_t i = 0; i < iSize; ++i) {
        pData[i].SetSize(iSize);
    }
}

HackingMenu::Grid::Grid(const Grid& aCopy) : pData(nullptr) {
    *this = aCopy;
}

HackingMenu::Grid::~Grid() {
    if (pData) {
        delete[] pData;
    }
}

// The old rows are freed, then pData is a new GridRow[aCopy.iSize], and each row is assigned from aCopy's.
HackingMenu::Grid& HackingMenu::Grid::operator=(const HackingMenu::Grid& aCopy) {
    this->iSize = aCopy.iSize;
    if (pData) {
        delete[] pData;
    }
    pData = new HackingMenu::GridRow[aCopy.iSize];
    for (uint32_t i = 0; i < this->iSize; ++i) {
        pData[i].operator=(aCopy.pData[i]);
    }
    return *this;
}

HackingMenu::Grid*
HackingMenu::Grid::MakeSubGrid(HackingMenu::Grid* apResult, uint8_t* apIndices, uint8_t ucSize) const {
    HackingMenu::Grid v10(ucSize);
    for (int32_t i = 0; i < ucSize; ++i) {
        for (int32_t j = 0; j < ucSize; ++j) {
            uint8_t* v4 = pData[apIndices[i]].ucData;
            int32_t v5 = apIndices[j];
            v10.pData[i].ucData[j] = v4[v5];
        }
    }
    ::new (apResult) HackingMenu::Grid(v10);
    return apResult;
}

HackingMenu::HackingMenu()
  : Menu(), IOLines{}, pTiles{}, pPassword(nullptr), iNumGuesses(0), iTotGuesses(0), iWordsHave(0), iWordLength(0),
    iBufferPos(nullptr), iYStart(0), iYInterval(30), iLastOffset(0xFFFFFFFFu), pLastTarget(nullptr), eStage(HMS_INTRO),
    pOverTile(nullptr), iCursorX(0), dwAccessTime(0), pCursorVis(nullptr), bSuppressEntry(false), bUsedReset(false),
    bTutorial(false) {
    BSAudioManager* pManager = &BSAudioManager::QInstance();
    Sounds[0] = pManager->GetSoundHandleByName("UIHackingFanHumLP", 49u);
    Sounds[1] = pManager->GetSoundHandleByName("UIHackingPassGood", 33u);
    Sounds[2] = pManager->GetSoundHandleByName("UIHackingPassBad", 33u);
    Sounds[3] = pManager->GetSoundHandleByName("UIHackingCharScroll", 33u);
    Sounds[4] = pManager->GetSoundHandleByName("UIHackingCharEnter", 33u);
}

// The members' own destructors empty the tile lists and destroy xEntryText; the words, the buffer and the IO lines
// are the menu's heap data.
HackingMenu::~HackingMenu() {
    while (!xLogTiles.IsEmpty()) {
        Tile* pTile = xLogTiles.RemoveHead();
        if (pTile) {
            delete pTile;
        }
    }
    while (!xGuessTiles.IsEmpty()) {
        Tile* pTile = xGuessTiles.RemoveHead();
        if (pTile) {
            delete pTile;
        }
    }
    HackingMenu::iNextAllowedHackTime = iHackingRetryMilliseconds.Int() + BSGetTickCount();
    wordList.DeleteAll();
    ourFree(iBufferPos);
    HackingMenu::pMe = nullptr;
    for (int32_t i = 0; i < 5; ++i) {
        Sounds[i].Stop();
        Sounds[i].Release();
    }
    for (uint32_t i = 0; i < 46; ++i) {
        if (IOLines[i]) {
            delete IOLines[i];
        }
    }
    SystemColorManager::GetInstance()->SetColor(3, HackingMenu::SaveColor);
}

void HackingMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
    if (((uint32_t)aiID) < 8) {
        pTiles[aiID] = apTile;
    }
}

// Each log branch builds its lines in a BSStringT array.
void HackingMenu::DoClick(int32_t a1, Tile* a2) {
    InterfaceManager::Instance();
    if (InterfaceManager::Has360Controller()) {
        a2 = pOverTile;
    }
    if (!a2 && a1 >= 0 && a1 < 8) {
        a2 = pTiles[a1];
    }
    if (eStage == HackingMenu::HMS_INTRO) {
        if (BSGetTickCount() > static_cast<uint32_t>(HackingMenu::iNextAllowedHackTime)) {
            SetupWordTiles();
        }
        return;
    }
    if (eStage == HackingMenu::HMS_PREGAME) {
        for (uint32_t i = 0; i < 46; ++i) {
            if (IOLines[i]) {
                IOLines[i]->Animation.ForceFinish();
            }
        }
        return;
    }
    if (eStage != HackingMenu::HMS_INGAME || !a2 || a2->pParent != pTiles[2]) {
        return;
    }
    xEntryText.ForceFinish();
    const char* pguess = xEntryText.GetFullText();
    if (!pguess || !*pguess) {
        return;
    }
    Sounds[4].Play(false);
    uint32_t inumCommon = get_num_matching_characters(pguess, pPassword);
    if (std::isalpha(*pguess) || BSstrlen(pguess) <= 1) {
        if (inumCommon == iWordLength) {
            const char* ptmp[5];
            BSStringT<char> astrLog[5];
            astrLog[0].SPrintF("%s%s", ">", pguess);
            astrLog[1].SPrintF("%s%s", ">", sHackingGranted.String());
            astrLog[2].SPrintF("%s%s", ">", sHackingAccessing1.String());
            astrLog[3].SPrintF("%s%s", ">", sHackingAccessing2.String());
            astrLog[4].SPrintF("%s%s", ">", sHackingAccessing3.String());
            ptmp[0] = astrLog[0].String();
            ptmp[1] = astrLog[1].String();
            ptmp[2] = astrLog[2].String();
            ptmp[3] = astrLog[3].String();
            ptmp[4] = astrLog[4].String();
            ScrollLog(5u, ptmp);
            Sounds[1].PlayAfter(250u, 0);
            dwAccessTime = BSGetTickCount() + 3000;
            eStage = HackingMenu::HMS_FINISHED;
            if (pTerminal) {
                TESObjectREFR::SetTerminalUnlocked(pTerminalRef, 1);
                LOCK_LEVEL eLockLevel = pTerminal->GetHackDifficultyLockLevel(pTerminalRef);
                int32_t iXP =
                    ExperiencePoints::GetExperiencePoints(ExperiencePoints::ACTIVITY_HACK_COMPUTER, eLockLevel);
                pPlayer->RewardExperience(iXP);
                TESForm* pform = pTerminalRef->GetOwner();
                if (pform) {
                    pPlayer->TrespassAlarm(pTerminalRef, pform, -1);
                }
            }
            MiscStatManager::Increment(MiscStatManager::MS_COMPUTERS_HACKED);
        }
        else {
            BSStringT<char> astrLog[4];
            astrLog[0].SPrintF("%s%s", ">", pguess);
            astrLog[1].SPrintF("%s%s", ">", sHackingDenied.String());
            if (--iNumGuesses) {
                const char* ptmp[3];
                const char* pCorrect = sHackingCorrect.String();
                astrLog[2].SPrintF("%s%d/%d %s.", ">", inumCommon, iWordLength, pCorrect);
                ptmp[0] = astrLog[0].String();
                ptmp[1] = astrLog[1].String();
                ptmp[2] = astrLog[2].String();
                ScrollLog(3u, ptmp);
                if (iNumGuesses == 1) {
                    const char* pWarning = sHackingWarning.operator const char*();
                    IOLines[1]->Animation.operator=(pWarning);
                    IOLines[1]->Animation.ForceFinish();
                    IOLines[1]->Animation.SetShouldBlink(true);
                }
            }
            else {
                const char* ptmp[4];
                astrLog[2].SPrintF("%s%s", ">", sHackingLockout1.String());
                astrLog[3].SPrintF("%s", sHackingLockout2.String());
                ptmp[0] = astrLog[0].String();
                ptmp[1] = astrLog[1].String();
                ptmp[2] = astrLog[2].String();
                ptmp[3] = astrLog[3].String();
                ScrollLog(4u, ptmp);
                eStage = HackingMenu::HMS_SHUTDOWN;
                if (pTerminal) {
                    BGSTerminal::SetLockedOut(pTerminalRef, 1);
                }
            }
            Sounds[2].PlayAfter(250u, 0);
            Tile* pSquare = xGuessTiles.RemoveTail();
            if (pSquare) {
                delete pSquare;
            }
        }
    }
    else {
        uint32_t ilogscroll;
        const char* ptmp[3];
        BSStringT<char> astrLog[3];
        astrLog[0].SPrintF("%s%s", ">", pguess);
        if (iWordsHave == 1) {
            astrLog[1].SPrintF("%s%s", ">", sHackingDenied.String());
            ilogscroll = 2;
        }
        else if (bUsedReset || RandomUnsignedInt(4u)) {
            RemoveDud();
            // The bracket's grid position: the row is converted through a 64-bit integer and only the low 32 bits are
            // kept.
            double fCol = a2->GetFloat(4100) * 17.0;
            uint32_t uiRow = static_cast<uint32_t>(static_cast<int64_t>(a2->GetFloat(4012) + fCol));
            uint32_t uiBonus = 12 * uiRow + GetSelectedTileIndex() - 7;
            xUsedBonuses.AddHead(uiBonus);
            astrLog[1].SPrintF("%s%s", ">", sHackingDudRemoved.String());
            ilogscroll = 2;
        }
        else {
            if (iNumGuesses == 1) {
                const char* pHeader = sHackingHeader2.operator const char*();
                IOLines[1]->Animation.operator=(pHeader);
                IOLines[1]->Animation.SetShouldBlink(false);
                IOLines[1]->Animation.ForceFinish();
            }
            astrLog[1].SPrintF("%s%s", ">", sHackingToleranceReset1.String());
            astrLog[2].SPrintF("%s%s", ">", sHackingToleranceReset2.String());
            iNumGuesses = iTotGuesses;
            bUsedReset = true;
            double fCol = a2->GetFloat(4100) * 17.0;
            uint32_t uiRow = static_cast<uint32_t>(static_cast<int64_t>(a2->GetFloat(4012) + fCol));
            uint32_t uiBonus = 12 * uiRow + GetSelectedTileIndex() - 7;
            xUsedBonuses.AddHead(uiBonus);
            while (xGuessTiles.m_uiCount < iTotGuesses) {
                AddAttemptsRemainingSquares();
            }
            ilogscroll = 3;
        }
        ptmp[0] = astrLog[0].String();
        ptmp[1] = astrLog[1].String();
        ptmp[2] = astrLog[2].String();
        ScrollLog(ilogscroll, ptmp);
    }
    BSStringT<char> strtmp;
    const char* pHeader3 = sHackingHeader3.String();
    strtmp.SPrintF("%d %s", iNumGuesses, pHeader3);
    IOLines[2]->Animation.operator=(strtmp.String());
    IOLines[2]->Animation.ForceFinish();
    ClearEntry();
    bSuppressEntry = true;
}

void HackingMenu::DoLeave(int32_t aiID, Tile* apTarget) {
    ClearEntry();
    bSuppressEntry = false;
    pLastTarget = nullptr;
}

void HackingMenu::DoIdle() {
    if (xFadeState == Interface::FADE_SHOWN) {
        Interface::enumMenus ClassStatic = static_cast<Interface::enumMenus>(HackingMenu::GetClassStatic());
        if (Interface::IsTopMenuID(ClassStatic)) {
            InterfaceManager* v2;
            if (!bTutorial ||
                (v2 = InterfaceManager::Instance(), v2->xTutorialManager.HasBeenShown(Interface::TUT_HACKING))) {
                InterfaceManager::Instance();
                if (InterfaceManager::Has360Controller()) {
                    HackingMenu::pim->pOverTileTarget = pOverTile;
                }
                xEntryText.Update();
                HandleCursorBlink();
                switch (eStage) {
                    case HackingMenu::HMS_INTRO: {
                        DoIntro();
                        break;
                    }
                    case HackingMenu::HMS_PREGAME: {
                        uint32_t i;
                        for (i = 0; i < 46; ++i) {
                            if (IOLines[i] && !IOLines[i]->Animation.bIsTextRemaining) {
                                Sounds[3].Play(false);
                                IOLines[i]->Animation.Update();
                                UpdateScrollingCursor(IOLines[i]);
                                break;
                            }
                        }
                        if (i == 46) {
                            int32_t iy = 0;
                            for (Tile* ptile = pTiles[5]; ptile && ptile != pTiles[4]->pParent;
                                 ptile = ptile->pParent) {
                                iy += static_cast<int32_t>(ptile->GetFloat(4002));
                            }
                            pTiles[4]->Set(4002, iy);
                            eStage = HackingMenu::HMS_INGAME;
                        }
                        break;
                    }
                    case HackingMenu::HMS_INGAME: {
                        UpdateCursorPos();
                        if (iNumGuesses == 1) {
                            IOLines[1]->Animation.Update();
                        }
                        if ((HackingMenu::pim->pOverTileTarget) &&
                            (HackingMenu::pim->pOverTileTarget->pParent == pTiles[2])) {
                            DoInGame();
                        }

                        break;
                    }
                    case HackingMenu::HMS_SHUTDOWN: {
                        UpdateCursorPos();
                        DoShutdown();
                        break;
                    }
                    case HackingMenu::HMS_FINISHED: {
                        UpdateCursorPos();
                        if ((iNumGuesses) && (xFadeState != Interface::FADING_OUT)) {
                            AccessTerminal();
                        }

                        break;
                    }
                    default: {
                        return;
                    }
                }
            }
        }
    }
}

int32_t HackingMenu::GetClass() {
    return 1055;
}

bool HackingMenu::DoGamepad(int32_t a1, float a2) {
    bool result;
    InterfaceManager::Instance();
    if (!InterfaceManager::Has360Controller()) {
        return false;
    }
    if (!Interface::IsTopMenuID(Interface::eHackingMenu)) {
        return false;
    }
    HackingMenu::pim->pOverTileTarget = pOverTile;
    if (eStage < HackingMenu::HMS_INGAME && a1 == 9) {
        DoClick(-1, nullptr);
        return true;
    }
    if (eStage != HackingMenu::HMS_INGAME) {
        return false;
    }
    // The column goes to FindTile as unsigned.
    switch (a1) {
        case 1: {
            int32_t iIndex = static_cast<int32_t>(pOverTile->GetFloat(4012));
            if (iIndex <= 0) {
                return false;
            }
            bSuppressEntry = false;
            uint32_t uiCol = static_cast<uint32_t>(static_cast<int64_t>(pOverTile->GetFloat(4100)));
            pOverTile = FindTile(iIndex - 1, uiCol);
            result = true;
            break;
        }
        case 2: {
            int32_t iIndex = static_cast<int32_t>(pOverTile->GetFloat(4012));
            if (iIndex >= 16) {
                return false;
            }
            bSuppressEntry = false;
            uint32_t uiCol = static_cast<uint32_t>(static_cast<int64_t>(pOverTile->GetFloat(4100)));
            pOverTile = FindTile(iIndex + 1, uiCol);
            result = true;
            break;
        }
        case 3: {
            bool v15;
            int32_t isave = this->iCursorX;
            if (pOverTile && pOverTile->GetString(4036)) {
                const char* String = pOverTile->GetString(4036);
                int32_t SelectedTileIndex = GetSelectedTileIndex();
                v15 = std::isalpha(String[SelectedTileIndex]) != 0;
            }
            else {
                v15 = false;
            }
            for (this->iCursorX += 17; v15; this->iCursorX += 17) {
                if (GetSelectedTileIndex() == -1) {
                    break;
                }
                const char* v10 = pOverTile->GetString(4036);
                int32_t v11 = GetSelectedTileIndex();
                if (!std::isalpha(v10[v11])) {
                    break;
                }
            }
            if (GetSelectedTileIndex() == -1) {
                if (!(pOverTile->GetFloat(4100) < 1.0)) {
                    this->iCursorX = isave;
                }
                else {
                    int32_t iindex_2 = static_cast<int32_t>(pOverTile->GetFloat(4012));
                    uint32_t v12 = static_cast<int32_t>(pOverTile->GetFloat(4100)) + 1;
                    Tile* ptile_2 = FindTile(iindex_2, v12);
                    if (ptile_2) {
                        bSuppressEntry = false;
                        pOverTile = ptile_2;
                        this->iCursorX = static_cast<int32_t>(pOverTile->GetRealX() + 119.0);
                    }
                    else {
                        this->iCursorX = isave;
                    }
                }
            }
            else {
                bSuppressEntry = false;
            }
            result = true;
            break;
        }
        case 4: {
            bool v17;
            uint32_t iCursorX = this->iCursorX;
            if (pOverTile && pOverTile->GetString(4036)) {
                const char* v4 = pOverTile->GetString(4036);
                int32_t v5 = GetSelectedTileIndex();
                v17 = std::isalpha(v4[v5]) != 0;
            }
            else {
                v17 = false;
            }
            for (this->iCursorX -= 17; v17; this->iCursorX -= 17) {
                if (GetSelectedTileIndex() < 7) {
                    break;
                }
                const char* v6 = pOverTile->GetString(4036);
                int32_t v7 = GetSelectedTileIndex();
                if (!std::isalpha(v6[v7])) {
                    break;
                }
            }
            if (GetSelectedTileIndex() >= 7) {
                bSuppressEntry = false;
            }
            else {
                if (!(pOverTile->GetFloat(4100) > 0.0)) {
                    this->iCursorX = iCursorX;
                }
                else {
                    int32_t iindex = static_cast<int32_t>(pOverTile->GetFloat(4012));
                    int32_t icol = static_cast<int32_t>(pOverTile->GetFloat(4100)) - 1;
                    Tile* ptile = FindTile(iindex, icol);
                    if (ptile) {
                        bSuppressEntry = false;
                        pOverTile = ptile;
                        double RealX = pOverTile->GetRealX();
                        this->iCursorX = static_cast<int32_t>(pOverTile->GetFloat(4017) + RealX - 17.0);
                    }
                    else {
                        this->iCursorX = iCursorX;
                    }
                }
            }
            result = true;
            break;
        }
        default: {
            return false;
        }
    }
    return result;
}

bool HackingMenu::DoGamepadUpEvent(int32_t a2, float a3) {
    if (a2 != 10) {
        return false;
    }
    eStage = HMS_FINISHED;
    HackingMenu::Close();
    return true;
}

void HackingMenu::TransitionToComputersMenu() {
    // 6002 is a menu trait with no known name.
    pMenu->Set(6002, 1);
    StartFadeOut();
    if (pTerminal) {
        Tile* pmenu = ComputersMenu::Create(pTerminalRef);
        pmenu->GetMenu()->SetRenderedMenu();
        Tile::UpdateAll(false);
        FORenderedMenu* CurrentRenderedMenu = Interface::GetCurrentRenderedMenu();
        FORenderedTerminal* prm = dynamic_cast<FORenderedTerminal*>(CurrentRenderedMenu);
        if (!prm) {
            FORenderedTerminal* prm_2;
            Warning(
                "MENUS: HackingMenu::TransitionToComputersMenu being called but we are not in a renderedterminal.  "
                "Creating a new one."
            );
            FORenderedTerminal* apMemory = static_cast<FORenderedTerminal*>(ourMalloc(sizeof(FORenderedTerminal)));
            if (apMemory) {
                prm_2 = ::new (apMemory) FORenderedTerminal;
            }
            else {
                prm_2 = nullptr;
            }
            prm = prm_2;
        }
        prm->pMenuTile = pmenu;
        prm->SetMenuRoot(pmenu->GetModel());
    }
}

void HackingMenu::AccessTerminal() {
    if (BSGetTickCount() > dwAccessTime) {
        TransitionToComputersMenu();
    }
}

void HackingMenu::DoShutdown() {
    float v2;
    uint32_t v3;
    if (BSGetTickCount() > dwAccessTime) {
        if (dwAccessTime) {
            v3 = ((BSGetTickCount() - dwAccessTime) / 50) + 1;
        }
        else {
            v3 = 1;
        }
        v2 = (float)(pMenu->Tile::GetFloat(4101) + ((double)(v3 * iYInterval)));
        pMenu->Tile::Set(4101, v2, true);
        dwAccessTime = BSGetTickCount() + 50;
        (&Sounds[3])->Play(false);
    }
    double realY = (double)pTiles[5]->GetRealY();
    if ((pTiles[5]->GetFloat(4016) + realY) < 0.0) {
        pMenu->Tile::Set(4102, 1);
        eStage = HMS_FINISHED;
    }
}

int32_t HackingMenu::GetSelectedTileIndex() {
    float pipboyMouseX;
    int32_t uiControllerSelectedTileIdx;
    InterfaceManager::Instance();
    if (InterfaceManager::Has360Controller()) {
        uiControllerSelectedTileIdx = (int32_t)iCursorX;
    }
    else {
        if (HackingMenu::pim->IsInRenderedMenu()) {
            pipboyMouseX = HackingMenu::pim->GetPipboyMouseX();
        }
        else {
            pipboyMouseX = (float)(double)HackingMenu::pim->GetCursorX();
        }
        uiControllerSelectedTileIdx = (int32_t)pipboyMouseX;
    }
    int32_t realX = (int32_t)HackingMenu::pim->pOverTileTarget->GetRealX();
    int32_t v6 = (int32_t)((HackingMenu::pim->pOverTileTarget->GetFloat(4017) + ((double)realX)) - 1.0);
    if ((realX > uiControllerSelectedTileIdx) || (uiControllerSelectedTileIdx > v6)) {
        return (-1);
    }

    return (uiControllerSelectedTileIdx - realX) / 17;
}

void HackingMenu::ClearEntry() {
    xEntryText = nullptr;
    pTiles[2]->Set(Tile::kTileValue_User1, -1);
    pTiles[2]->Set(Tile::kTileValue_User4, -1);
}

Tile* HackingMenu::FindTile(uint32_t aiIndex, uint32_t aiCol) {
    uint32_t i;
    for (i = 0; i < 46; ++i) {
        if (IOLines[i]->pTextTile &&
            (IOLines[i]->pTextTile->IsTrue(4015) &&
             ((((double)aiIndex) == IOLines[i]->pTextTile->GetFloat(4012)) &&
              (((double)aiCol) == IOLines[i]->pTextTile->GetFloat(4100))))) {
            return IOLines[i]->pTextTile;
        }
    }
    return nullptr;
}

bool HackingMenu::IsWellFormedMenu(int32_t& ai) {
    for (*(&ai) = 0; (*(&ai)) < 8; ++(*(&ai))) {
        if (!pTiles[*(&ai)]) {
            return false;
        }
    }
    return true;
}

void HackingMenu::ScrollLog(uint32_t aiTicks, const char** pNewEntries) {
    NiTListItem<Tile*>* pPos = xLogTiles.GetHeadPos();
    while (pPos) {
        Tile* pTile = xLogTiles.GetNext(pPos);
        float fY = pTile->GetFloat(4012) + static_cast<double>(aiTicks);
        pTile->Set(4012, fY, true);
    }
    for (uint32_t i = 0; i < aiTicks; ++i) {
        Tile* pTile = RenderTemplate(pTiles[3], "hacking_password_log_template", nullptr);
        pTile->Set(4012, aiTicks - i - 1);
        pTile->Set(4036, pNewEntries[i], true);
        xLogTiles.AddTail(pTile);
    }
}

// Picks the aiWordsWant candidate words of iWordLength letters from Data\Menus\FalloutDict.txt and the password among
// them, then bounds the guesses from the grid of the letters each pair of words shares. The words are drawn from the
// shuffled dictionary for at most 100 ms per round; a word is kept when no kept word shares all its letters, and then:
// the last word when it shares n letters (n < MAX_CHECKS) with at most imax kept words, the others when they share a
// letter with each, or a ZERO_RANDMAX roll falls under izerofreq. Every ZERO_RANDMAX rounds that fail, imax grows by
// one, up to aiWordsWant / 2.
void HackingMenu::Prepare(uint32_t aiWordsWant) {
    BSStringT<char> strDictFilename;
    BSFile* pdictFile = nullptr;
    HackingMenu::WordList candidateList;
    // dwBegin is never read.
    [[maybe_unused]] uint32_t dwBegin = BSGetTickCount();
    int32_t inumtries = 0;
    constexpr int32_t MAX_CHECKS = 8;
    constexpr int32_t ZERO_RANDMAX = 10;
    uint32_t imax = (aiWordsWant - 2) / iWordLength + 1;
    uint32_t izerofreq = 0;
    if (!iWordLength) {
        return;
    }
    strDictFilename.operator=("Data\\Menus\\FalloutDict.txt");
    // The misc archives (type 0x100) with a 0x4000-byte buffer.
    pdictFile = static_cast<BSFile*>(FileFinder::GetFile(strDictFilename.pString, NiFile::READ_ONLY, 0x4000, 0x100));
    if (!pdictFile || !pdictFile->Open(false, false)) {
        Warning(WARN_MENUS, "Hacking Menu: Couldn't locate dictionary file %s", strDictFilename.pString);
        HackingMenu::Close();
        return;
    }
    while (true) {
        char token[32];
        ourMemset(token, 0, sizeof(token));
        // One space-delimited word.
        if (!pdictFile->GetLine(token, sizeof(token), ' ')) {
            break;
        }
        if (token[0] && BSstrlen(token) == iWordLength) {
            candidateList.AppendStr(token);
        }
    }
    delete pdictFile;
    candidateList.Randomize();
    candidateList.Randomize();
    candidateList.Randomize();
    while (true) {
        uint32_t dwTimeout = BSGetTickCount() + 100;
        while (BSGetTickCount() < dwTimeout && ++inumtries != 0) {
            wordList.DeleteAll();
            iWordsHave = 0;
            candidateList.Randomize();
            for (BSSimpleList<char*>* pnode = &candidateList; iWordsHave < aiWordsWant && pnode && pnode->m_item;
                 pnode = pnode->m_pkNext) {
                char* pcandidate = pnode->m_item;
                bool bvalid = true;
                // How many kept words share each count of letters with pcandidate.
                uint8_t uctally[MAX_CHECKS];
                ourMemset(uctally, 0, sizeof(uctally));
                for (BSSimpleList<char*>* pnode2 = &wordList; bvalid && pnode2 && pnode2->m_item;
                     pnode2 = pnode2->m_pkNext) {
                    uint32_t inumCommon = get_num_matching_characters(pcandidate, pnode2->m_item);
                    if (inumCommon == iWordLength) {
                        bvalid = false;
                    }
                    else if (iWordsHave < aiWordsWant - 1) {
                        bvalid = inumCommon > 0 || RandomUnsignedInt(ZERO_RANDMAX) < izerofreq;
                    }
                    else {
                        bvalid = inumCommon >= MAX_CHECKS || ++uctally[inumCommon] <= imax;
                    }
                }
                if (bvalid) {
                    wordList.AppendStr(pcandidate);
                    ++iWordsHave;
                }
            }
            if (iWordsHave == aiWordsWant) {
                break;
            }
        }
        if (iWordsHave == aiWordsWant) {
            break;
        }
        if (++izerofreq == ZERO_RANDMAX) {
            izerofreq = 0;
            if (++imax > aiWordsWant >> 1) {
                break;
            }
        }
    }
    wordList.Randomize();
    wordList.Randomize();
    wordList.Randomize();
    wordList.Randomize();
    uint32_t irandom = RandomUnsignedInt(iWordsHave);
    BSSimpleList<char*>* plistword = &wordList;
    for (uint32_t i = 0; i < irandom; ++i) {
        plistword = plistword->m_pkNext;
    }
    pPassword = plistword->m_item;
    InitializationFlow("The password is '%s'", pPassword);
    {
        // The letters each pair of kept words shares; 0xFF where a word meets itself.
        HackingMenu::Grid ComPos(iWordsHave);
        uint32_t row = 0;
        for (BSSimpleList<char*>* pnode = &wordList; pnode && pnode->m_item; pnode = pnode->m_pkNext) {
            uint32_t col = 0;
            for (BSSimpleList<char*>* pnode2 = &wordList; pnode2 && pnode2->m_item; pnode2 = pnode2->m_pkNext) {
                uint32_t inumCommon;
                if (pnode == pnode2) {
                    inumCommon = 0xFF;
                }
                else {
                    inumCommon = get_num_matching_characters(pnode->m_item, pnode2->m_item);
                }
                ComPos.pData[row].ucData[col] = static_cast<uint8_t>(inumCommon);
                ++col;
            }
            ++row;
        }
        iNumGuesses = GetGuaranteedPossible(ComPos);
        iNumGuesses = BSMAX(4u, iNumGuesses);
        iTotGuesses = iNumGuesses;
    }
}

// Writes ausVal as "0x" and four upper-case hex digits, terminated, to apResult (11 bytes in MakePasswordFile).
void HackingMenu::MakeHexString(uint16_t ausVal, char* apResult) {
    ourMemcpy(apResult, "0x", 2);
    for (int32_t i = 0; i < 4; ++i) {
        uint8_t c = (ausVal & (0xF << 4 * i)) >> 4 * i;
        apResult[5 - i] = c >= 10 ? c + ('A' - 10) : c + '0';
    }
    apResult[6] = 0;
}

void HackingMenu::SetupWordTiles() {
    for (uint32_t i = 0; i < 46 && IOLines[i]; ++i) {
        HackingMenu::TerminalIO* pLine = IOLines[i];
        if (pLine) {
            delete pLine;
        }
    }
    ourMemset(IOLines, 0, sizeof(IOLines));
    BSStringT<char> sLine;
    sLine.SPrintF("%d %s", iNumGuesses, sHackingHeader3.String());
    HackingMenu::TerminalIO* pMem0 = static_cast<HackingMenu::TerminalIO*>(ourMalloc(sizeof(HackingMenu::TerminalIO)));
    HackingMenu::TerminalIO* pLine0;
    if (pMem0) {
        uint32_t iY = iYStart;
        Tile* pParent = pTiles[1];
        const char* pText = sHackingHeader.operator const char*();
        pLine0 = ::new (pMem0) HackingMenu::TerminalIO(pText, pParent, "hacking_intro_template", iY, 0);
    }
    else {
        pLine0 = nullptr;
    }
    IOLines[0] = pLine0;
    HackingMenu::TerminalIO* pMem1 = static_cast<HackingMenu::TerminalIO*>(ourMalloc(sizeof(HackingMenu::TerminalIO)));
    HackingMenu::TerminalIO* pLine1;
    if (pMem1) {
        uint32_t iY = iYInterval + iYStart;
        Tile* pParent = pTiles[1];
        const char* pText = sHackingHeader2.operator const char*();
        pLine1 = ::new (pMem1) HackingMenu::TerminalIO(pText, pParent, "hacking_intro_template", iY, 0);
    }
    else {
        pLine1 = nullptr;
    }
    IOLines[1] = pLine1;
    HackingMenu::TerminalIO* pMem2 = static_cast<HackingMenu::TerminalIO*>(ourMalloc(sizeof(HackingMenu::TerminalIO)));
    HackingMenu::TerminalIO* pLine2;
    if (pMem2) {
        pLine2 = ::new (pMem2)
            HackingMenu::TerminalIO(sLine.String(), pTiles[1], "hacking_intro_template", iYStart + 3 * iYInterval, 0);
    }
    else {
        pLine2 = nullptr;
    }
    IOLines[2] = pLine2;
    double dX = IOLines[2]->pTextTile->GetFloat(Tile::kTileValue_X);
    float fX = static_cast<double>(17 * sLine.GetLength()) + dX;
    pTiles[1]->Set(Tile::kTileValue_User1, fX, true);
    float fY = IOLines[2]->pTextTile->GetFloat(Tile::kTileValue_Y);
    pTiles[1]->Set(Tile::kTileValue_User2, fY, true);
    HackingMenu::TerminalIO* pMem3 = static_cast<HackingMenu::TerminalIO*>(ourMalloc(sizeof(HackingMenu::TerminalIO)));
    HackingMenu::TerminalIO* pLine3;
    if (pMem3) {
        pLine3 = ::new (pMem3) HackingMenu::TerminalIO;
    }
    else {
        pLine3 = nullptr;
    }
    IOLines[3] = pLine3;
    Tile::Value* pValue = pTiles[1]->GetValueQ(Tile::kTileValue_User0);
    IOLines[3]->Animation.SetTraits(nullptr, pValue);
    int32_t iLine = 4;
    for (uint32_t j = 0; j < iTotGuesses; ++j) {
        AddAttemptsRemainingSquares();
    }
    uint32_t uiJunkCount = BSstrlen("!@#$%^*()_+=-`[]{}|;':,./<>?\\\"");
    if (!iWordsHave) {
        return;
    }
    char acBuffer[409];
    ourFree(iBufferPos);
    // The allocation size is the count times 4, or 0xFFFFFFFF when that overflows, as the game's operator new[] does.
    iBufferPos = static_cast<uint32_t*>(
        ourMalloc(iWordsHave >> 30 ? 0xFFFFFFFFu : static_cast<uint32_t>(sizeof(uint32_t) * iWordsHave))
    );
    uint32_t uiTries = 0;
    while (true) {
        for (uint32_t k = 0; k < 408; ++k) {
            acBuffer[k] = "!@#$%^*()_+=-`[]{}|;':,./<>?\\\""[RandomUnsignedInt(uiJunkCount)];
        }
        acBuffer[408] = 0;
        BSSimpleList<char*>* pNode = &wordList;
        int32_t iWord = 0;
        uint32_t uiFailures = 0;
        while (pNode && pNode->m_item && uiFailures < 100) {
            uint32_t uiPos = RandomUnsignedInt(409 - iWordLength - 2);
            bool bFits = true;
            for (int32_t m = -2; bFits && m < static_cast<int32_t>(iWordLength + 2); ++m) {
                bFits = static_cast<int32_t>(uiPos + m) < 0 || acBuffer[uiPos + m] < 65 || acBuffer[uiPos + m] > 90;
            }
            if (bFits) {
                ourMemcpy(&acBuffer[uiPos], pNode->m_item, iWordLength);
                iBufferPos[iWord++] = uiPos;
                pNode = pNode->m_pkNext;
                uiFailures = 0;
            }
            else {
                ++uiFailures;
            }
        }
        if (!uiFailures) {
            break;
        }
        if (++uiTries > 4) {
            Warning(
                "MENUS: Unable to construct complete password file for hacking minigame.  Continuing with fewer "
                "words."
            );
            while (pNode->m_item) {
                // Stores the password back when it equals the word dropped here, as the game does.
                if (pPassword == pNode->m_item) {
                    pPassword = pNode->m_item;
                }
                ourFree(pNode->m_item);
                pNode->RemoveHead();
                --iWordsHave;
                iBufferPos[iWord++] = 408;
            }
            for (BSSimpleList<char*>* pIter = &wordList; pIter; pIter = pIter->m_pkNext) {
                if (pIter->m_pkNext == pNode) {
                    pIter->m_pkNext = nullptr;
                }
            }
        }
    }
    uint32_t uiColumn = 0;
    uint32_t uiRow = 0;
    char* pEnd = &acBuffer[408];
    for (char* pCur = acBuffer; pCur < pEnd; pCur += 12) {
        char acHex[11];
        char cSave = 0;
        if (pEnd - pCur > 12) {
            cSave = pCur[12];
            pCur[12] = 0;
        }
        // The shown address is the low 16 bits of the line's address in memory, as the game does.
        MakeHexString(static_cast<uint16_t>(reinterpret_cast<uintptr_t>(pCur)), acHex);
        sLine.SPrintF("%s %s", acHex, pCur);
        HackingMenu::TerminalIO* pMem =
            static_cast<HackingMenu::TerminalIO*>(ourMalloc(sizeof(HackingMenu::TerminalIO)));
        HackingMenu::TerminalIO* pLine;
        if (pMem) {
            pLine = ::new (pMem)
                HackingMenu::TerminalIO(sLine.String(), pTiles[2], "hacking_password_file_template", -1, 2);
        }
        else {
            pLine = nullptr;
        }
        IOLines[iLine] = pLine;
        IOLines[iLine]->pTextTile->Set(Tile::kTileValue_ListIndex, uiColumn);
        IOLines[iLine]->pTextTile->Set(Tile::kTileValue_User0, uiRow);
        InterfaceManager::Instance();
        if (InterfaceManager::Has360Controller() && !uiColumn && !uiRow) {
            pOverTile = IOLines[iLine]->pTextTile;
            iCursorX = static_cast<int32_t>(pOverTile->GetRealX() + 119.0);
        }
        if (++uiColumn == 17) {
            uiColumn = 0;
            ++uiRow;
        }
        ++iLine;
        if (pEnd - pCur > 12) {
            pCur[12] = cSave;
        }
    }
    pTiles[0]->Set(Tile::kTileValue_Visible, 0);
    eStage = HackingMenu::HMS_PREGAME;
}

uint32_t HackingMenu::GetGuaranteedPossible(const HackingMenu::Grid& aGrid) {
    HackingMenu::Grid v12;
    uint8_t v20 = -1;
    for (uint32_t i = 0; i < aGrid.iSize; ++i) {
        uint8_t MaxTally = aGrid.pData[i].GetMaxTally();
        if (MaxTally < static_cast<int32_t>(v20)) {
            v20 = MaxTally;
        }
    }
    if (v20 == 255) {
        return 0;
    }
    if (v20 <= 2u) {
        return v20 + 1;
    }
    uint32_t FormHealth_0 = aGrid.iSize;
    for (uint32_t j = 0;; ++j) {
        uint32_t v3 = aGrid.iSize;
        if (j >= v3) {
            break;
        }
        HackingMenu::GridRow* v4 = aGrid.pData;
        uint32_t v5 = v4[j].GetMaxTally();
        if (v5 == v20) {
            uint32_t auiA = 0;
            for (int32_t aiVal = 0;; ++aiVal) {
                uint8_t a2[32];
                uint32_t v6 = aGrid.iSize;
                if (aiVal >= v6) {
                    break;
                }
                uint32_t v9 = aiVal;
                HackingMenu::GridRow* v7 = aGrid.pData;
                uint32_t Tally = v7[j].GetTally(v9, a2);
                if (Tally == v20) {
                    aGrid.MakeSubGrid(&v12, a2, v20);
                    uint32_t auiB = GetGuaranteedPossible(v12);
                    auiA = BSMAX(auiA, auiB);
                }
            }
            if (auiA) {
                FormHealth_0 = BSMIN(FormHealth_0, auiA);
            }
            if (FormHealth_0 <= 2) {
                return FormHealth_0 + 1;
            }
        }
    }
    return FormHealth_0 + 1;
}

void HackingMenu::DoIntro() {
    int32_t i;
    for (i = 0; i < 14; ++i) {
        if (BSGetTickCount() <= HackingMenu::iNextAllowedHackTime) {
            IOLines[0]->pTextTile->Set(Tile::kTileValue_Visible, 1);
        }
        else {
            IOLines[0]->pTextTile->Set(Tile::kTileValue_Visible, 0);
        }
        if (IOLines[i] && !IOLines[i]->Animation.bIsTextRemaining) {
            if (IOLines[i]->pPromptTile) {
                if (!IOLines[i]->pPromptTile->IsTrue(Tile::kTileValue_Visible)) {
                    IOLines[i]->pPromptTile->Set(Tile::kTileValue_Visible, 1);
                    time_11D969C = BSGetTickCount() + 1000;
                }
                if (BSGetTickCount() > time_11D969C) {
                    const char* String = IOLines[i]->pTextTile->GetString(Tile::kTileValue_String);
                    uint32_t ioldlen = BSstrlen(String);
                    IOLines[i]->Animation.Update();
                    const char* v2 = IOLines[i]->pTextTile->GetString(Tile::kTileValue_String);
                    uint32_t inewlen = BSstrlen(v2);
                    for (int32_t j = 0; j < inewlen - ioldlen; ++j) {
                        static int32_t iinterval = [&] {
                            float v6;
                            if (iHackingInputRate.UInt() <= 1) {
                                v6 = 1000.0f;
                            }
                            else {
                                float v7 = 1000.0 / static_cast<double>(iHackingInputRate.UInt());
                                v6 = BSRoundToNearest(v7, 1.0f);
                            }
                            return static_cast<int32_t>(v6);
                        }();
                        int32_t itime = iinterval * j / (inewlen - ioldlen);
                        uint32_t afVal = itime;
                        BSAudioManager* v3 = &BSAudioManager::QInstance();
                        v3->GetSoundHandleByName("UIHackingCharSingle", 289u).PlayAfter(afVal, 0);
                    }
                    if (IOLines[i]->Animation.bIsTextRemaining) {
                        Sounds[4].Play(false);
                        time_11D969C = BSGetTickCount() + 500;
                    }
                }
            }
            else {
                if (BSGetTickCount() > time_11D969C) {
                    IOLines[i]->Animation.Update();
                    Sounds[3].Play(false);
                }
            }
            UpdateScrollingCursor(IOLines[i]);
            break;
        }
    }
    if (i == 14 && BSGetTickCount() > time_11D969C) {
        SetupWordTiles();
    }
}

void HackingMenu::DoInGame() {
    uint32_t ibufferpos;
    int32_t ioffset = GetSelectedTileIndex() - 7;
    if ((ioffset != iLastOffset || HackingMenu::pim->pOverTileTarget != pLastTarget) &&
        (ioffset >= 0 || static_cast<int32_t>(iLastOffset) >= 0)) {
        iLastOffset = ioffset;
        pLastTarget = HackingMenu::pim->pOverTileTarget;
        if (ioffset >= 0) {
            if (bSuppressEntry) {
                ClearEntry();
                const char* String = HackingMenu::pim->pOverTileTarget->GetString(Tile::kTileValue_String);
                if (!std::isalpha(String[ioffset + 7])) {
                    bSuppressEntry = false;
                    pLastTarget = nullptr;
                }
            }
            else {
                char floatingText[32];
                double v13 = HackingMenu::pim->pOverTileTarget->GetFloat(Tile::kTileValue_User0) * 17.0;
                ibufferpos = static_cast<int64_t>(
                    (HackingMenu::pim->pOverTileTarget->GetFloat(Tile::kTileValue_ListIndex) + v13) * 12.0 +
                    static_cast<double>(ioffset)
                );
                BSSimpleList<char*>* pnode = &wordList;
                for (uint32_t i = 0;
                     i < iWordsHave && (ibufferpos < iBufferPos[i] || ibufferpos >= iWordLength + iBufferPos[i]); ++i) {
                    pnode = pnode->m_pkNext;
                }
                ourMemset(floatingText, 0, 32u);
                if (pnode) {
                    ourMemcpy(floatingText, pnode->m_item, iWordLength);
                }
                else {
                    if (xUsedBonuses.FindPos(ibufferpos, nullptr)) {
                        const char* v4 = HackingMenu::pim->pOverTileTarget->GetString(Tile::kTileValue_String);
                        floatingText[0] = v4[GetSelectedTileIndex()];
                    }
                    else {
                        const char* v2 = HackingMenu::pim->pOverTileTarget->GetString(Tile::kTileValue_String);
                        int32_t SelectedTileIndex = GetSelectedTileIndex();
                        HackingMenu::GetGarbageString(floatingText, &v2[SelectedTileIndex]);
                    }
                }
                if (xEntryText.operator!=(floatingText)) {
                    xEntryText.operator=(floatingText);
                    // 0xFFFFFFFF: the ID of an empty handle, which ends the queue.
                    for (BSSimpleList<BSSoundHandle>* pnode_2 = &HackingMenu::listSoundsToPlay;
                         pnode_2 && pnode_2->m_item.operator!=(0xFFFFFFFFu); pnode_2 = pnode_2->m_pkNext) {
                        pnode_2->m_item.Release();
                    }
                    HackingMenu::listSoundsToPlay.RemoveAll();
                    uint32_t i_2 = BSstrlen(floatingText);
                    while (i_2--) {
                        BSAudioManager* v6 = &BSAudioManager::QInstance();
                        BSSoundHandle handle = v6->GetSoundHandleByName("UIHackingCharSingle", 289u);
                        HackingMenu::listSoundsToPlay.AddHead(handle);
                        if (i_2) {
                            uint32_t v7 = xEntryText.uiDisplayRate;
                            // The delay is converted to a 64-bit integer and only its low 32 bits are passed.
                            handle.PlayAfter(
                                static_cast<uint32_t>(static_cast<int64_t>(static_cast<double>(i_2 * v7) * 1.5)), 0
                            );
                        }
                        else {
                            handle.Play(false);
                        }
                    }
                }
                bool bbreakfound = false;
                ioffset = GetSelectedTileIndex();
                if (pnode) {
                    const char* poverstring = HackingMenu::pim->pOverTileTarget->GetString(Tile::kTileValue_String);
                    while (std::isalpha(poverstring[ioffset - 1])) {
                        --ioffset;
                    }
                    for (uint32_t ibreakpos = 1; !bbreakfound && ibreakpos < iWordLength; ++ibreakpos) {
                        int32_t ix1 =
                            static_cast<int32_t>(HackingMenu::pim->pOverTileTarget->GetFloat(Tile::kTileValue_X));
                        int32_t iy1 =
                            static_cast<int32_t>(HackingMenu::pim->pOverTileTarget->GetFloat(Tile::kTileValue_Y));
                        int32_t ix2 = ix1 + 119;
                        int32_t iy2 = iy1;
                        if (poverstring[ibreakpos + ioffset]) {
                            if (!std::isalpha(poverstring[ibreakpos + ioffset])) {
                                bbreakfound = true;
                                ibreakpos = iWordLength - ibreakpos;
                                if (HackingMenu::pim->pOverTileTarget->GetFloat(Tile::kTileValue_ListIndex) == 0.0) {
                                    int64_t Float = static_cast<int64_t>(
                                        HackingMenu::pim->pOverTileTarget->GetFloat(Tile::kTileValue_User0)
                                    );
                                    Tile* pprev = FindTile(16u, Float - 1);
                                    ix1 = static_cast<int32_t>(
                                        pprev->GetFloat(Tile::kTileValue_X) + static_cast<double>(17 * (19 - ibreakpos))
                                    );
                                    iy1 = static_cast<int32_t>(pprev->GetFloat(Tile::kTileValue_Y));
                                }
                                else {
                                    ix1 += 17 * (19 - ibreakpos);
                                    iy1 -= static_cast<int32_t>(
                                        HackingMenu::pim->pOverTileTarget->GetFloat(Tile::kTileValue_Height)
                                    );
                                }
                            }
                        }
                        else {
                            bbreakfound = true;
                            ix1 += 17 * ioffset;
                            if (!(HackingMenu::pim->pOverTileTarget->GetFloat(Tile::kTileValue_ListIndex) + 1.0 <
                                  17.0)) {
                                int64_t v12 = static_cast<int64_t>(
                                    HackingMenu::pim->pOverTileTarget->GetFloat(Tile::kTileValue_User0)
                                );
                                Tile* pnext = FindTile(0, v12 + 1);
                                ix2 = static_cast<int32_t>(pnext->GetFloat(Tile::kTileValue_X) + 119.0);
                                iy2 = static_cast<int32_t>(pnext->GetFloat(Tile::kTileValue_Y));
                            }
                            else {
                                iy2 = iy1 +
                                    static_cast<int32_t>(
                                          HackingMenu::pim->pOverTileTarget->GetFloat(Tile::kTileValue_Height)
                                    );
                            }
                        }
                        if (bbreakfound) {
                            char csave = floatingText[ibreakpos];
                            floatingText[ibreakpos] = 0;
                            pTiles[2]->Set(Tile::kTileValue_User0, floatingText, true);
                            pTiles[2]->Set(Tile::kTileValue_User1, ix1);
                            pTiles[2]->Set(Tile::kTileValue_User2, iy1);
                            floatingText[ibreakpos] = csave;
                            pTiles[2]->Set(Tile::kTileValue_User3, &floatingText[ibreakpos], true);
                            pTiles[2]->Set(Tile::kTileValue_User4, ix2);
                            pTiles[2]->Set(Tile::kTileValue_User5, iy2);
                        }
                    }
                }
                if (!bbreakfound) {
                    pTiles[2]->Set(Tile::kTileValue_User0, floatingText, true);
                    double v8 = HackingMenu::pim->pOverTileTarget->GetFloat(Tile::kTileValue_X);
                    float v10 = v8 + static_cast<double>(17 * ioffset);
                    pTiles[2]->Set(Tile::kTileValue_User1, v10, true);
                    float afValue = HackingMenu::pim->pOverTileTarget->GetFloat(Tile::kTileValue_Y);
                    pTiles[2]->Set(Tile::kTileValue_User2, afValue, true);
                    pTiles[2]->Set(Tile::kTileValue_User4, -1);
                }
            }
        }
        else {
            ClearEntry();
            bSuppressEntry = false;
        }
    }
}

void HackingMenu::RemoveDud() {
    uint32_t ibufferpos;
    uint32_t iword;
    uint32_t icnt;
    BSSimpleList<char*>* pnode;
    char newbuffer[20];
    do {
        iword = RandomUnsignedInt(iWordsHave);
        icnt = iword;
        for (pnode = &wordList; icnt--; pnode = pnode->m_pkNext) {
            ;
        }
    }
    while (!BSstrcmp(pnode->m_item, pPassword));
    uint32_t i = 4;
    for (ibufferpos = iBufferPos[iword]; ibufferpos > 12; ibufferpos -= 12) {
        ++i;
    }
    Tile* ptile = IOLines[i++]->pTextTile;
    const char* String = ptile->GetString(4036);
    ourMemcpy(newbuffer, String, 20u);
    for (icnt = 0; icnt < this->iWordLength && ibufferpos < 12; ++icnt) {
        newbuffer[ibufferpos++ + 7] = 46;
    }
    ptile->Set(4036, newbuffer, true);
    if ((IOLines[i]) && (IOLines[i]->pTextTile) && (IOLines[i]->pTextTile->IsTrue(4015) && icnt < this->iWordLength)) {
        int32_t v8 = 0;
        ptile = IOLines[i]->pTextTile;
        const char* v3 = ptile->GetString(4036);
        ourMemcpy(newbuffer, v3, 20u);
        while (true) {
            uint32_t v4 = icnt;
            uint32_t iWordLength = this->iWordLength;
            ++icnt;
            if (v4 >= iWordLength) {
                break;
            }
            newbuffer[v8++ + 7] = 46;
        }
        ptile->Set(4036, newbuffer, true);
    }

    while (iword < iWordsHave - 1) {
        iBufferPos[iword] = iBufferPos[iword + 1];
        ++iword;
    }
    iBufferPos[iword] = 408;
    ourFree(pnode->m_item);
    wordList.Remove(pnode->m_item);
    --iWordsHave;
}

// The blink interval (fBlinkInterval) is a guarded static read once from the cursor tile's _blink_interval trait;
// uiNextBlinkTime is the tick count of the next blink.
void HackingMenu::HandleCursorBlink() {
    static int32_t uiNextBlinkTime;
    static float fBlinkInterval = [&] {
        int32_t iTrait = Tile::AddUserTrait("_blink_interval", -1);
        return pTiles[4]->GetFloat(iTrait);
    }();
    if (eStage == HackingMenu::HMS_FINISHED && pMenu->IsTrue(Tile::kTileValue_User2)) {
        return;
    }
    // The compare is unsigned.
    if (BSGetTickCount() > static_cast<uint32_t>(uiNextBlinkTime) && pCursorVis) {
        if (pCursorVis->fValue == 1.0) {
            pCursorVis->SetFloat(0.0f, true);
        }
        else {
            pCursorVis->SetFloat(1.0f, true);
        }
        // The tick count is zero-extended to 64 bits, the sum truncated to 64 bits and its low 32 bits stored.
        int64_t iNext =
            static_cast<int64_t>(static_cast<double>(static_cast<int64_t>(BSGetTickCount())) + fBlinkInterval);
        uiNextBlinkTime = static_cast<int32_t>(iNext);
    }
    else if (!pCursorVis) {
        pCursorVis = pTiles[4]->GetValue(Tile::kTileValue_Visible);
    }
}

void HackingMenu::UpdateCursorPos() {
    uint32_t v3 = 0;
    int32_t v5 = 0;
    if (pTiles[5]->IsTrue(4003)) {
        const char* apString = pTiles[5]->GetString(4036);
        if (apString) {
            v3 = BSstrlen(apString);
        }
    }
    for (Tile* i = pTiles[5]; i && i != pTiles[4]->pParent; i = i->pParent) {
        v5 += static_cast<int32_t>(i->GetFloat(4001));
    }
    pTiles[4]->Set(4001, static_cast<int32_t>(v5 + 17 * v3));
}

void HackingMenu::UpdateScrollingCursor(HackingMenu::TerminalIO* apIO) {
    if ((apIO) && (apIO->pTextTile)) {
        int32_t ichars;
        const char* ptext = apIO->pTextTile->GetString(4036);
        if (!ptext || *ptext == 32) {
            ichars = 0;
        }
        else {
            ichars = BSstrlen(ptext);
        }
        int32_t ix = static_cast<int32_t>(apIO->pTextTile->GetFloat(4001) + static_cast<double>(17 * ichars));
        int32_t iy = static_cast<int32_t>(apIO->pTextTile->GetFloat(4002));
        for (Tile* pparent = apIO->pTextTile->pParent; pparent && pparent != pTiles[4]->pParent;
             pparent = pparent->pParent) {
            ix += static_cast<int32_t>(pparent->GetFloat(4001));
            iy += static_cast<int32_t>(pparent->GetFloat(4002));
        }
        pTiles[4]->Set(4001, ix);
        pTiles[4]->Set(4002, iy);
    }
}

void HackingMenu::AddAttemptsRemainingSquares() {
    Tile* arg0 = HackingMenu::pMe->RenderTemplate(pTiles[1], "hacking_guess_template", nullptr);
    uint32_t FormFlags = xGuessTiles.m_uiCount;
    arg0->Set(4012, FormFlags);
    xGuessTiles.AddTail(arg0);
    BSStringT<char> apString;
    uint32_t v2 = xGuessTiles.m_uiCount;
    apString.SPrintF("%s_%d", arg0->GetName(), v2);
    char* v3 = apString.pString;
    arg0->SetName(static_cast<const char*>(v3));
}

Tile* HackingMenu::Create(TESObjectREFR* apTerminalRef) {
    int32_t v4;
    int32_t ClassStatic = HackingMenu::GetClassStatic();
    Tile* pold_menu_tile = Menu::GetMenuByClass(ClassStatic);
    if (pold_menu_tile) {
        delete pold_menu_tile;
    }
    HackingMenu::pim = InterfaceManager::Instance();
    float ftop_depth = Tile::GetMaximumDepth();
    SystemColorManager* Instance = SystemColorManager::GetInstance();
    Instance->GetColor(3, HackingMenu::SaveColor);
    BGSTerminal* pterminal = static_cast<BGSTerminal*>(apTerminalRef->GetObjectReference());
    if (pterminal->UsesAltColor()) {
        NiColorA apMemory(1.0f, 1.0f, 1.0f, 0.0f);
        NiColorA* a2 = &apMemory;
        SystemColorManager* v3 = SystemColorManager::GetInstance();
        v3->SetColor(3, *a2);
    }
    g_iHackingFlashOnDuration = iHackingFlashOnDuration.Int();
    g_iHackingFlashOffDuration = iHackingFlashOffDuration.Int();
    Tile* ptile = HackingMenu::pim->GetMenusRoot()->ReadFile("Data\\Menus\\hacking_menu.xml");
    Menu* pmenu = ptile->GetMenu();
    if (pmenu && (v4 = pmenu->GetClass(), v4 == HackingMenu::GetClassStatic())) {
        int32_t imissingIndex;
        pmenu->SetMenuTile(static_cast<TileMenu*>(ptile), false);
        HackingMenu::pMe = static_cast<HackingMenu*>(pmenu);
        if (static_cast<HackingMenu*>(pmenu)->HackingMenu::IsWellFormedMenu(imissingIndex)) {
            BGSTerminal* pterm;
            if (ptile->GetFloat(4007) == 6006.0 || ptile->GetFloat(4007) == 102.0) {
                ptile->Set(4013, ftop_depth, true);
            }
            const char* v7 = sHackingLockout3.String();
            HackingMenu::pMe->pTiles[6]->Set(4036, v7, true);
            const char* v8 = sHackingLockout4.String();
            HackingMenu::pMe->pTiles[7]->Set(4036, v8, true);
            if (apTerminalRef) {
                pterm = static_cast<BGSTerminal*>(apTerminalRef->GetObjectReference());
            }
            else {
                pterm = nullptr;
            }
            if (pterm && pPlayer->GetTerminalAccess(apTerminalRef) == PlayerCharacter::TAT_LOCKED) {
                ptile->Set(4102, 1);
                HackingMenu::pMe->eStage = HackingMenu::HMS_FINISHED;
                HackingMenu::pMe->pTiles[4]->Set(4003, 0);
            }
            else {
                double fpct;
                uint32_t HackMiniGameValue;
                HackingMenu::pMe->pTerminal = pterm;
                HackingMenu::pMe->pTerminalRef = apTerminalRef;
                if (HackingMenu::pMe->pTerminal) {
                    HackMiniGameValue =
                        HackingMenu::pMe->pTerminal->GetHackMiniGameValue(HackingMenu::pMe->pTerminalRef);
                }
                else {
                    HackMiniGameValue = 12;
                }
                HackingMenu::pMe->iWordLength = HackMiniGameValue;
                HackingMenu::pMe->iWordLength = BSMIN(12u, HackingMenu::pMe->iWordLength);
                int32_t v9 = HackingMenu::GetClassStatic();
                HackingMenu::pMe->bTutorial =
                    HackingMenu::pim->xTutorialManager.ShowMessage(Interface::TUT_HACKING, v9, 512u);
                float fscienceSkill = pPlayer->GetClampedActorFloatValue(ActorValue::SCIENCE);
                BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::HACKING_SCIENCE_BONUS, pPlayer, &fscienceSkill);
                // BSMIN compares the settings' values as unsigned.
                uint32_t imaxWords = BSMIN(20u, static_cast<uint32_t>(iHackingMaxWords.Int()));
                uint32_t a2a = iHackingMinWords.Int();
                uint32_t iminWords = BSMIN(imaxWords, a2a);
                float a2b = HackingMenu::pMe->pTerminal->GetMinSkillRequiredToHack(apTerminalRef);
                float a2c = BSMAX(0.0f, a2b);
                float fminRqSkill = BSMIN(100.0f, a2c);
                float v91 = fscienceSkill - fminRqSkill;
                float fskillDiff = BSMAX(0.0f, v91);
                float fskillRange = 100.0 - fminRqSkill;
                if (fskillRange == 0.0) {
                    fpct = 0.5;
                }
                else {
                    fpct = 1.0 - fskillDiff / fskillRange;
                }
                float v112 = fpct;
                float v89 = static_cast<double>(imaxWords - iminWords) * v112;
                // The rounded count is converted to a 64-bit integer and its low 32 bits are added.
                uint32_t iwordsWant =
                    iminWords + static_cast<uint32_t>(static_cast<int64_t>(BSRoundToNearest(v89, 1.0f)));
                if (iwordsWant > 20) {
                    iwordsWant = 20;
                }
                HackingMenu::pMe->Prepare(iwordsWant);
                ptile->Set(4100, ">", true);
                Tile::Value* a2d = HackingMenu::pMe->pTiles[5]->GetValue(4003);
                Tile::Value* ValueQ = HackingMenu::pMe->pTiles[5]->GetValueQ(4036);
                HackingMenu::pMe->xEntryText.SetTraits(ValueQ, a2d);
                // The rate is tested and converted as unsigned, and the interval is converted through a 64-bit integer,
                // keeping its low 32 bits.
                if (static_cast<uint32_t>(iHackingInputRate.Int()) > 0) {
                    float v87 = 1000.0 / static_cast<double>(static_cast<uint32_t>(iHackingInputRate.Int()));
                    uint32_t uiInterval = static_cast<uint32_t>(static_cast<int64_t>(BSRoundToNearest(v87, 1.0f)));
                    HackingMenu::pMe->xEntryText.SetDisplayInterval(uiInterval);
                }
                const char* ptemplate = "hacking_intro_template";
                TerminalIO* pIO = static_cast<TerminalIO*>(ourMalloc(sizeof(TerminalIO)));
                HackingMenu::pMe->IOLines[0] = pIO
                    ? ::new (pIO) TerminalIO(
                          sHackingSecurityReset.operator const char*(), HackingMenu::pMe->pTiles[0], ptemplate,
                          HackingMenu::pMe->iYStart, 0
                      )
                    : nullptr;
                pIO = static_cast<TerminalIO*>(ourMalloc(sizeof(TerminalIO)));
                HackingMenu::pMe->IOLines[1] = pIO
                    ? ::new (pIO) TerminalIO(
                          sHackingIntro01.operator const char*(), HackingMenu::pMe->pTiles[0], ptemplate,
                          HackingMenu::pMe->iYStart + 2 * HackingMenu::pMe->iYInterval, 0
                      )
                    : nullptr;
                pIO = static_cast<TerminalIO*>(ourMalloc(sizeof(TerminalIO)));
                HackingMenu::pMe->IOLines[2] = pIO
                    ? ::new (pIO) TerminalIO(
                          sHackingIntro02.operator const char*(), HackingMenu::pMe->pTiles[0], ptemplate,
                          HackingMenu::pMe->iYStart + 4 * HackingMenu::pMe->iYInterval, 1
                      )
                    : nullptr;
                pIO = static_cast<TerminalIO*>(ourMalloc(sizeof(TerminalIO)));
                HackingMenu::pMe->IOLines[3] = pIO
                    ? ::new (pIO) TerminalIO(
                          sHackingIntro03.operator const char*(), HackingMenu::pMe->pTiles[0], ptemplate,
                          HackingMenu::pMe->iYStart + 6 * HackingMenu::pMe->iYInterval, 0
                      )
                    : nullptr;
                pIO = static_cast<TerminalIO*>(ourMalloc(sizeof(TerminalIO)));
                HackingMenu::pMe->IOLines[4] = pIO
                    ? ::new (pIO) TerminalIO(
                          sHackingIntro04.operator const char*(), HackingMenu::pMe->pTiles[0], ptemplate,
                          HackingMenu::pMe->iYStart + 8 * HackingMenu::pMe->iYInterval, 1
                      )
                    : nullptr;
                pIO = static_cast<TerminalIO*>(ourMalloc(sizeof(TerminalIO)));
                HackingMenu::pMe->IOLines[5] = pIO
                    ? ::new (pIO) TerminalIO(
                          sHackingIntro05.operator const char*(), HackingMenu::pMe->pTiles[0], ptemplate,
                          HackingMenu::pMe->iYStart + 9 * HackingMenu::pMe->iYInterval, 1
                      )
                    : nullptr;
                pIO = static_cast<TerminalIO*>(ourMalloc(sizeof(TerminalIO)));
                HackingMenu::pMe->IOLines[6] = pIO
                    ? ::new (pIO) TerminalIO(
                          sHackingIntro06.operator const char*(), HackingMenu::pMe->pTiles[0], ptemplate,
                          HackingMenu::pMe->iYStart + 11 * HackingMenu::pMe->iYInterval, 0
                      )
                    : nullptr;
                pIO = static_cast<TerminalIO*>(ourMalloc(sizeof(TerminalIO)));
                HackingMenu::pMe->IOLines[7] = pIO
                    ? ::new (pIO) TerminalIO(
                          sHackingIntro07.operator const char*(), HackingMenu::pMe->pTiles[0], ptemplate,
                          HackingMenu::pMe->iYStart + 12 * HackingMenu::pMe->iYInterval, 0
                      )
                    : nullptr;
                pIO = static_cast<TerminalIO*>(ourMalloc(sizeof(TerminalIO)));
                HackingMenu::pMe->IOLines[8] = pIO
                    ? ::new (pIO) TerminalIO(
                          sHackingIntro08.operator const char*(), HackingMenu::pMe->pTiles[0], ptemplate,
                          HackingMenu::pMe->iYStart + 13 * HackingMenu::pMe->iYInterval, 0
                      )
                    : nullptr;
                pIO = static_cast<TerminalIO*>(ourMalloc(sizeof(TerminalIO)));
                HackingMenu::pMe->IOLines[9] = pIO
                    ? ::new (pIO) TerminalIO(
                          sHackingIntro09.operator const char*(), HackingMenu::pMe->pTiles[0], ptemplate,
                          HackingMenu::pMe->iYStart + 14 * HackingMenu::pMe->iYInterval, 0
                      )
                    : nullptr;
                pIO = static_cast<TerminalIO*>(ourMalloc(sizeof(TerminalIO)));
                HackingMenu::pMe->IOLines[10] = pIO
                    ? ::new (pIO) TerminalIO(
                          sHackingIntro10.operator const char*(), HackingMenu::pMe->pTiles[0], ptemplate,
                          HackingMenu::pMe->iYStart + 15 * HackingMenu::pMe->iYInterval, 0
                      )
                    : nullptr;
                pIO = static_cast<TerminalIO*>(ourMalloc(sizeof(TerminalIO)));
                HackingMenu::pMe->IOLines[11] = pIO
                    ? ::new (pIO) TerminalIO(
                          sHackingIntro11.operator const char*(), HackingMenu::pMe->pTiles[0], ptemplate,
                          HackingMenu::pMe->iYStart + 16 * HackingMenu::pMe->iYInterval, 0
                      )
                    : nullptr;
                pIO = static_cast<TerminalIO*>(ourMalloc(sizeof(TerminalIO)));
                HackingMenu::pMe->IOLines[12] = pIO
                    ? ::new (pIO) TerminalIO(
                          sHackingIntro12.operator const char*(), HackingMenu::pMe->pTiles[0], ptemplate,
                          HackingMenu::pMe->iYStart + 17 * HackingMenu::pMe->iYInterval, 0
                      )
                    : nullptr;
                pIO = static_cast<TerminalIO*>(ourMalloc(sizeof(TerminalIO)));
                HackingMenu::pMe->IOLines[13] = pIO
                    ? ::new (pIO) TerminalIO(
                          sHackingIntro13.operator const char*(), HackingMenu::pMe->pTiles[0], ptemplate,
                          HackingMenu::pMe->iYStart + 19 * HackingMenu::pMe->iYInterval, 1
                      )
                    : nullptr;
            }
            HackingMenu::pMe->Sounds->Play(false);
            pmenu->PrepForVisibility(false);
            return ptile;
        }

        Warning("MENUS: Hacking Menu Creation Failed... Missing index = #%d", imissingIndex);
        return nullptr;
    }

    if ((pmenu) && (pmenu->pMenu) && (pmenu)) {
        delete pmenu;
    }

    return nullptr;
}

void HackingMenu::Close() {
    if (HackingMenu::pMe) {
        PlayTerminalShutdownSound();
        HackingMenu::pMe->pMenu->Set(4033, 0.75f, true);
        if (HackingMenu::pim->GetCurrentRenderedMenu()) {
            NiNode* MenuRoot = static_cast<NiNode*>(HackingMenu::pim->GetCurrentRenderedMenu()->GetMenuRoot());
            if (MenuRoot == HackingMenu::pMe->pMenu->GetModel()) {
                FORenderedMenu* CurrentRenderedMenu = HackingMenu::pim->GetCurrentRenderedMenu();
                FORenderedTerminal* pterm = dynamic_cast<FORenderedTerminal*>(CurrentRenderedMenu);
                pterm->FadeOut();
            }
        }
        HackingMenu::pMe->pMenu->Set(Tile::kTileValue_DeleteOnFade, 1);
        HackingMenu::pMe->StartFadeOut();
        HackingMenu::iNextAllowedHackTime = BSGetTickCount() + 0x1F40;
    }
}

int32_t HackingMenu::GetClassStatic() {
    return 1055;
}

void* HackingMenu::GetGarbageString(void* apDest, const char* apSrc) {
    uint32_t aCount = 1;
    char* v4 = std::strchr(Str, *apSrc);
    if (v4) {
        while (true) {
            if (!apSrc[aCount] || std::isalpha(apSrc[aCount])) {
                return ourMemcpy(apDest, apSrc, 1u);
            }
            if (apSrc[aCount] == pCloseBrackets[v4 - Str]) {
                break;
            }
            ++aCount;
        }
        ++aCount;
    }
    return ourMemcpy(apDest, apSrc, aCount);
}

void HackingText::SetDisplayInterval(uint32_t aiInterval) {
    uiDisplayRate = BSMAX(1u, aiInterval);
}

bool HackingText::operator!=(const char* apString) {
    return strText.operator!=(apString);
}
