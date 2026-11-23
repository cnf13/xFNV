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

int32_t get_num_matching_characters(const char* apGuess, const char* apString) {
    if (!apGuess || !apString) {
        return 0;
    }
    int32_t iMatches = 0;
    uint32_t uiStringLength = BSstrlen(apString);
    uint32_t uiGuessLength = BSstrlen(apGuess);
    uint32_t uiLength = BSMIN(uiGuessLength, uiStringLength);
    for (uint32_t i = 0; i < uiLength; ++i) {
        if (apGuess[i] == apString[i]) {
            ++iMatches;
        }
    }
    return iMatches;
}

uint8_t Max(uint8_t aucA, uint8_t aucB) {
    if (aucA <= aucB) {
        return aucB;
    }

    return aucA;
}

int32_t RandomCompare(void* apA, void* apB) {
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
    uint8_t ucTally = 0;
    for (uint32_t i = 0; i < iSize; ++i) {
        if (ucData[i] == aiVal) {
            apCols[ucTally++] = i;
        }
    }
    return ucTally;
}

uint32_t HackingMenu::GridRow::GetMaxTally() {
    uint8_t aucCounts[32];
    uint8_t ucMaxTally = 0;
    ourMemset(aucCounts, 0, 32u);
    for (uint32_t i = 0; i < iSize; ++i) {
        if (ucData[i] < 32u) {
            ++aucCounts[ucData[i]];
        }
    }
    for (uint8_t ucCount : aucCounts) {
        ucMaxTally = Max(ucMaxTally, ucCount);
    }
    return ucMaxTally;
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
        pData[i] = aCopy.pData[i];
    }
    return *this;
}

HackingMenu::Grid*
HackingMenu::Grid::MakeSubGrid(HackingMenu::Grid* apResult, uint8_t* apIndices, uint8_t ucSize) const {
    HackingMenu::Grid kSubGrid(ucSize);
    for (int32_t i = 0; i < ucSize; ++i) {
        for (int32_t j = 0; j < ucSize; ++j) {
            kSubGrid.pData[i].ucData[j] = pData[apIndices[i]].ucData[apIndices[j]];
        }
    }
    ::new (apResult) HackingMenu::Grid(kSubGrid);
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
    if (static_cast<uint32_t>(aiID) < 8) {
        pTiles[aiID] = apTile;
    }
}

// Each log branch builds its lines in a BSStringT array.
void HackingMenu::DoClick(int32_t aiID, Tile* apTarget) {
    InterfaceManager::Instance();
    if (InterfaceManager::Has360Controller()) {
        apTarget = pOverTile;
    }
    if (!apTarget && aiID >= 0 && aiID < 8) {
        apTarget = pTiles[aiID];
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
    if (eStage != HackingMenu::HMS_INGAME || !apTarget || apTarget->pParent != pTiles[2]) {
        return;
    }
    xEntryText.ForceFinish();
    const char* pGuess = xEntryText.GetFullText();
    if (!pGuess || !*pGuess) {
        return;
    }
    Sounds[4].Play(false);
    uint32_t uiNumCommon = get_num_matching_characters(pGuess, pPassword);
    if (std::isalpha(*pGuess) || BSstrlen(pGuess) <= 1) {
        if (uiNumCommon == iWordLength) {
            const char* ptmp[5];
            BSStringT<char> astrLog[5];
            astrLog[0].SPrintF("%s%s", ">", pGuess);
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
                int32_t iXP = ExperiencePoints::GetExperiencePoints(
                    ExperiencePoints::ACTIVITY_HACK_COMPUTER,
                    eLockLevel
                );
                pPlayer->RewardExperience(iXP);
                TESForm* pForm = pTerminalRef->GetOwner();
                if (pForm) {
                    pPlayer->TrespassAlarm(pTerminalRef, pForm, -1);
                }
            }
            MiscStatManager::Increment(MiscStatManager::MS_COMPUTERS_HACKED);
        }
        else {
            BSStringT<char> astrLog[4];
            astrLog[0].SPrintF("%s%s", ">", pGuess);
            astrLog[1].SPrintF("%s%s", ">", sHackingDenied.String());
            if (--iNumGuesses) {
                const char* ptmp[3];
                const char* pCorrect = sHackingCorrect.String();
                astrLog[2].SPrintF("%s%d/%d %s.", ">", uiNumCommon, iWordLength, pCorrect);
                ptmp[0] = astrLog[0].String();
                ptmp[1] = astrLog[1].String();
                ptmp[2] = astrLog[2].String();
                ScrollLog(3u, ptmp);
                if (iNumGuesses == 1) {
                    const char* pWarning = static_cast<const char*>(sHackingWarning);
                    IOLines[1]->Animation = pWarning;
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
        uint32_t uiLogScroll;
        const char* ptmp[3];
        BSStringT<char> astrLog[3];
        astrLog[0].SPrintF("%s%s", ">", pGuess);
        if (iWordsHave == 1) {
            astrLog[1].SPrintF("%s%s", ">", sHackingDenied.String());
            uiLogScroll = 2;
        }
        else if (bUsedReset || RandomUnsignedInt(4u)) {
            RemoveDud();
            // The bracket's grid position: the row is converted through a 64-bit integer and only the low 32 bits are
            // kept.
            double dCol = apTarget->GetFloat(4100) * 17.0;
            uint32_t uiRow = static_cast<uint32_t>(static_cast<int64_t>(apTarget->GetFloat(4012) + dCol));
            uint32_t uiBonus = 12 * uiRow + GetSelectedTileIndex() - 7;
            xUsedBonuses.AddHead(uiBonus);
            astrLog[1].SPrintF("%s%s", ">", sHackingDudRemoved.String());
            uiLogScroll = 2;
        }
        else {
            if (iNumGuesses == 1) {
                const char* pHeader = static_cast<const char*>(sHackingHeader2);
                IOLines[1]->Animation = pHeader;
                IOLines[1]->Animation.SetShouldBlink(false);
                IOLines[1]->Animation.ForceFinish();
            }
            astrLog[1].SPrintF("%s%s", ">", sHackingToleranceReset1.String());
            astrLog[2].SPrintF("%s%s", ">", sHackingToleranceReset2.String());
            iNumGuesses = iTotGuesses;
            bUsedReset = true;
            double dCol = apTarget->GetFloat(4100) * 17.0;
            uint32_t uiRow = static_cast<uint32_t>(static_cast<int64_t>(apTarget->GetFloat(4012) + dCol));
            uint32_t uiBonus = 12 * uiRow + GetSelectedTileIndex() - 7;
            xUsedBonuses.AddHead(uiBonus);
            while (xGuessTiles.m_uiCount < iTotGuesses) {
                AddAttemptsRemainingSquares();
            }
            uiLogScroll = 3;
        }
        ptmp[0] = astrLog[0].String();
        ptmp[1] = astrLog[1].String();
        ptmp[2] = astrLog[2].String();
        ScrollLog(uiLogScroll, ptmp);
    }
    BSStringT<char> strtmp;
    const char* pHeader3 = sHackingHeader3.String();
    strtmp.SPrintF("%d %s", iNumGuesses, pHeader3);
    IOLines[2]->Animation = strtmp.String();
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
        Interface::enumMenus eMenuID = static_cast<Interface::enumMenus>(HackingMenu::GetClassStatic());
        if (Interface::IsTopMenuID(eMenuID)) {
            if (!bTutorial || InterfaceManager::Instance()->xTutorialManager.HasBeenShown(Interface::TUT_HACKING)) {
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
                            int32_t iY = 0;
                            for (Tile* pTile = pTiles[5]; pTile && pTile != pTiles[4]->pParent;
                                 pTile = pTile->pParent) {
                                iY += static_cast<int32_t>(pTile->GetFloat(4002));
                            }
                            pTiles[4]->Set(4002, iY);
                            eStage = HackingMenu::HMS_INGAME;
                        }
                        break;
                    }
                    case HackingMenu::HMS_INGAME: {
                        UpdateCursorPos();
                        if (iNumGuesses == 1) {
                            IOLines[1]->Animation.Update();
                        }
                        if (HackingMenu::pim->pOverTileTarget &&
                            HackingMenu::pim->pOverTileTarget->pParent == pTiles[2]) {
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
                        if (iNumGuesses && xFadeState != Interface::FADING_OUT) {
                            AccessTerminal();
                        }

                        break;
                    }
                    default: {
                        break;
                    }
                }
            }
        }
    }
}

int32_t HackingMenu::GetClass() {
    return 1055;
}

bool HackingMenu::DoGamepad(int32_t aiButton, float afValue) {
    bool bResult;
    InterfaceManager::Instance();
    if (!InterfaceManager::Has360Controller()) {
        return false;
    }
    if (!Interface::IsTopMenuID(Interface::eHackingMenu)) {
        return false;
    }
    HackingMenu::pim->pOverTileTarget = pOverTile;
    if (eStage < HackingMenu::HMS_INGAME && aiButton == 9) {
        DoClick(-1, nullptr);
        return true;
    }
    if (eStage != HackingMenu::HMS_INGAME) {
        return false;
    }
    // The column goes to FindTile as unsigned.
    switch (aiButton) {
        case 1: {
            int32_t iIndex = static_cast<int32_t>(pOverTile->GetFloat(4012));
            if (iIndex <= 0) {
                return false;
            }
            bSuppressEntry = false;
            uint32_t uiCol = static_cast<uint32_t>(static_cast<int64_t>(pOverTile->GetFloat(4100)));
            pOverTile = FindTile(iIndex - 1, uiCol);
            bResult = true;
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
            bResult = true;
            break;
        }
        case 3: {
            bool bAlpha = false;
            int32_t iSave = this->iCursorX;
            if (pOverTile && pOverTile->GetString(4036)) {
                const char* pcString = pOverTile->GetString(4036);
                int32_t iIndex = GetSelectedTileIndex();
                bAlpha = std::isalpha(pcString[iIndex]) != 0;
            }
            for (this->iCursorX += 17; bAlpha; this->iCursorX += 17) {
                if (GetSelectedTileIndex() == -1) {
                    break;
                }
                const char* pcString = pOverTile->GetString(4036);
                int32_t iIndex = GetSelectedTileIndex();
                if (!std::isalpha(pcString[iIndex])) {
                    break;
                }
            }
            if (GetSelectedTileIndex() == -1) {
                if (pOverTile->GetFloat(4100) < 1.0) {
                    int32_t iIndex = static_cast<int32_t>(pOverTile->GetFloat(4012));
                    uint32_t uiCol = static_cast<int32_t>(pOverTile->GetFloat(4100)) + 1;
                    Tile* pTile = FindTile(iIndex, uiCol);
                    if (pTile) {
                        bSuppressEntry = false;
                        pOverTile = pTile;
                        this->iCursorX = static_cast<int32_t>(pOverTile->GetRealX() + 119.0);
                    }
                    else {
                        this->iCursorX = iSave;
                    }
                }
                else {
                    this->iCursorX = iSave;
                }
            }
            else {
                bSuppressEntry = false;
            }
            bResult = true;
            break;
        }
        case 4: {
            bool bAlpha = false;
            uint32_t uiSave = this->iCursorX;
            if (pOverTile && pOverTile->GetString(4036)) {
                const char* pcString = pOverTile->GetString(4036);
                int32_t iIndex = GetSelectedTileIndex();
                bAlpha = std::isalpha(pcString[iIndex]) != 0;
            }
            for (this->iCursorX -= 17; bAlpha; this->iCursorX -= 17) {
                if (GetSelectedTileIndex() < 7) {
                    break;
                }
                const char* pcString = pOverTile->GetString(4036);
                int32_t iIndex = GetSelectedTileIndex();
                if (!std::isalpha(pcString[iIndex])) {
                    break;
                }
            }
            if (GetSelectedTileIndex() >= 7) {
                bSuppressEntry = false;
            }
            else if (pOverTile->GetFloat(4100) > 0.0) {
                int32_t iIndex = static_cast<int32_t>(pOverTile->GetFloat(4012));
                int32_t iCol = static_cast<int32_t>(pOverTile->GetFloat(4100)) - 1;
                Tile* pTile = FindTile(iIndex, iCol);
                if (pTile) {
                    bSuppressEntry = false;
                    pOverTile = pTile;
                    double dRealX = pOverTile->GetRealX();
                    this->iCursorX = static_cast<int32_t>(pOverTile->GetFloat(4017) + dRealX - 17.0);
                }
                else {
                    this->iCursorX = uiSave;
                }
            }
            else {
                this->iCursorX = uiSave;
            }
            bResult = true;
            break;
        }
        default: {
            return false;
        }
    }
    return bResult;
}

bool HackingMenu::DoGamepadUpEvent(int32_t aiButton, float afValue) {
    if (aiButton != 10) {
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
        Tile* pComputersMenu = ComputersMenu::Create(pTerminalRef);
        pComputersMenu->GetMenu()->SetRenderedMenu();
        Tile::UpdateAll(false);
        FORenderedMenu* pRenderedMenu = Interface::GetCurrentRenderedMenu();
        FORenderedTerminal* prm = dynamic_cast<FORenderedTerminal*>(pRenderedMenu);
        if (!prm) {
            Warning(
                "MENUS: HackingMenu::TransitionToComputersMenu being called but we are not in a renderedterminal.  "
                "Creating a new one."
            );
            prm = new FORenderedTerminal;
        }
        prm->pMenuTile = pComputersMenu;
        prm->SetMenuRoot(pComputersMenu->GetModel());
    }
}

void HackingMenu::AccessTerminal() {
    if (BSGetTickCount() > dwAccessTime) {
        TransitionToComputersMenu();
    }
}

void HackingMenu::DoShutdown() {
    if (BSGetTickCount() > dwAccessTime) {
        uint32_t uiTicks = dwAccessTime ? (BSGetTickCount() - dwAccessTime) / 50 + 1 : 1;
        float fY = static_cast<float>(pMenu->Tile::GetFloat(4101) + static_cast<double>(uiTicks * iYInterval));
        pMenu->Tile::Set(4101, fY, true);
        dwAccessTime = BSGetTickCount() + 50;
        Sounds[3].Play(false);
    }
    double dRealY = pTiles[5]->GetRealY();
    if (pTiles[5]->GetFloat(4016) + dRealY < 0.0) {
        pMenu->Tile::Set(4102, 1);
        eStage = HMS_FINISHED;
    }
}

int32_t HackingMenu::GetSelectedTileIndex() {
    int32_t iControllerSelectedTileIdx;
    InterfaceManager::Instance();
    if (InterfaceManager::Has360Controller()) {
        iControllerSelectedTileIdx = static_cast<int32_t>(iCursorX);
    }
    else {
        float fPipboyMouseX;
        if (HackingMenu::pim->IsInRenderedMenu()) {
            fPipboyMouseX = HackingMenu::pim->GetPipboyMouseX();
        }
        else {
            fPipboyMouseX = static_cast<float>(static_cast<double>(HackingMenu::pim->GetCursorX()));
        }
        iControllerSelectedTileIdx = static_cast<int32_t>(fPipboyMouseX);
    }
    int32_t iRealX = static_cast<int32_t>(HackingMenu::pim->pOverTileTarget->GetRealX());
    int32_t iRightX = static_cast<int32_t>(
        HackingMenu::pim->pOverTileTarget->GetFloat(4017) + static_cast<double>(iRealX) - 1.0
    );
    if (iRealX > iControllerSelectedTileIdx || iControllerSelectedTileIdx > iRightX) {
        return -1;
    }

    return (iControllerSelectedTileIdx - iRealX) / 17;
}

void HackingMenu::ClearEntry() {
    xEntryText = nullptr;
    pTiles[2]->Set(Tile::kTileValue_User1, -1);
    pTiles[2]->Set(Tile::kTileValue_User4, -1);
}

Tile* HackingMenu::FindTile(uint32_t aiIndex, uint32_t aiCol) {
    for (uint32_t i = 0; i < 46; ++i) {
        if (IOLines[i]->pTextTile &&
            IOLines[i]->pTextTile->IsTrue(4015) &&
            static_cast<double>(aiIndex) == IOLines[i]->pTextTile->GetFloat(4012) &&
            static_cast<double>(aiCol) == IOLines[i]->pTextTile->GetFloat(4100)) {
            return IOLines[i]->pTextTile;
        }
    }
    return nullptr;
}

bool HackingMenu::IsWellFormedMenu(int32_t& ai) {
    for (ai = 0; ai < 8; ++ai) {
        if (!pTiles[ai]) {
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
    int32_t inumtries = 0;
    constexpr int32_t MAX_CHECKS = 8;
    constexpr int32_t ZERO_RANDMAX = 10;
    uint32_t imax = (aiWordsWant - 2) / iWordLength + 1;
    uint32_t izerofreq = 0;
    if (!iWordLength) {
        return;
    }
    strDictFilename = "Data\\Menus\\FalloutDict.txt";
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
    IOLines[0] = new HackingMenu::TerminalIO(
        static_cast<const char*>(sHackingHeader),
        pTiles[1],
        "hacking_intro_template",
        iYStart,
        0
    );
    IOLines[1] = new HackingMenu::TerminalIO(
        static_cast<const char*>(sHackingHeader2),
        pTiles[1],
        "hacking_intro_template",
        iYInterval + iYStart,
        0
    );
    HackingMenu::TerminalIO* pLine2 = new HackingMenu::TerminalIO(
        sLine.String(),
        pTiles[1],
        "hacking_intro_template",
        iYStart + 3 * iYInterval,
        0
    );
    IOLines[2] = pLine2;
    double dX = IOLines[2]->pTextTile->GetFloat(Tile::kTileValue_X);
    float fX = static_cast<double>(17 * sLine.GetLength()) + dX;
    pTiles[1]->Set(Tile::kTileValue_User1, fX, true);
    float fY = IOLines[2]->pTextTile->GetFloat(Tile::kTileValue_Y);
    pTiles[1]->Set(Tile::kTileValue_User2, fY, true);
    HackingMenu::TerminalIO* pLine3 = new HackingMenu::TerminalIO;
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
                // Stores the password back when it equals the word dropped here.
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
        // The shown address is the low 16 bits of the line's address in memory.
        MakeHexString(static_cast<uint16_t>(reinterpret_cast<uintptr_t>(pCur)), acHex);
        sLine.SPrintF("%s %s", acHex, pCur);
        HackingMenu::TerminalIO*
            pLine = new HackingMenu::TerminalIO(sLine.String(), pTiles[2], "hacking_password_file_template", -1, 2);
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
    HackingMenu::Grid kSubGrid;
    uint8_t ucMinTally = -1;
    for (uint32_t i = 0; i < aGrid.iSize; ++i) {
        uint8_t ucMaxTally = aGrid.pData[i].GetMaxTally();
        if (ucMaxTally < static_cast<int32_t>(ucMinTally)) {
            ucMinTally = ucMaxTally;
        }
    }
    if (ucMinTally == 255) {
        return 0;
    }
    if (ucMinTally <= 2u) {
        return ucMinTally + 1;
    }
    uint32_t uiGuesses = aGrid.iSize;
    for (uint32_t j = 0; j < aGrid.iSize; ++j) {
        uint32_t uiMaxTally = aGrid.pData[j].GetMaxTally();
        if (uiMaxTally == ucMinTally) {
            uint32_t uiMostGuesses = 0;
            for (int32_t iVal = 0; iVal < aGrid.iSize; ++iVal) {
                uint8_t aucCols[32];
                uint32_t uiTally = aGrid.pData[j].GetTally(iVal, aucCols);
                if (uiTally == ucMinTally) {
                    aGrid.MakeSubGrid(&kSubGrid, aucCols, ucMinTally);
                    uint32_t uiSubGuesses = GetGuaranteedPossible(kSubGrid);
                    uiMostGuesses = BSMAX(uiMostGuesses, uiSubGuesses);
                }
            }
            if (uiMostGuesses) {
                uiGuesses = BSMIN(uiGuesses, uiMostGuesses);
            }
            if (uiGuesses <= 2) {
                return uiGuesses + 1;
            }
        }
    }
    return uiGuesses + 1;
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
                    iNextTextUpdateTime = BSGetTickCount() + 1000;
                }
                if (BSGetTickCount() > iNextTextUpdateTime) {
                    const char* pcOldText = IOLines[i]->pTextTile->GetString(Tile::kTileValue_String);
                    uint32_t uiOldLen = BSstrlen(pcOldText);
                    IOLines[i]->Animation.Update();
                    const char* pcNewText = IOLines[i]->pTextTile->GetString(Tile::kTileValue_String);
                    uint32_t uiNewLen = BSstrlen(pcNewText);
                    for (int32_t j = 0; j < uiNewLen - uiOldLen; ++j) {
                        static int32_t iinterval = [&] {
                            float fInterval;
                            if (iHackingInputRate.UInt() <= 1) {
                                fInterval = 1000.0f;
                            }
                            else {
                                float fRaw = 1000.0 / static_cast<double>(iHackingInputRate.UInt());
                                fInterval = BSRoundToNearest(fRaw, 1.0f);
                            }
                            return static_cast<int32_t>(fInterval);
                        }();
                        int32_t iTime = iinterval * j / (uiNewLen - uiOldLen);
                        uint32_t uiDelay = iTime;
                        BSAudioManager::QInstance()
                            .GetSoundHandleByName("UIHackingCharSingle", 289u)
                            .PlayAfter(uiDelay, 0);
                    }
                    if (IOLines[i]->Animation.bIsTextRemaining) {
                        Sounds[4].Play(false);
                        iNextTextUpdateTime = BSGetTickCount() + 500;
                    }
                }
            }
            else if (BSGetTickCount() > iNextTextUpdateTime) {
                IOLines[i]->Animation.Update();
                Sounds[3].Play(false);
            }
            UpdateScrollingCursor(IOLines[i]);
            break;
        }
    }
    if (i == 14 && BSGetTickCount() > iNextTextUpdateTime) {
        SetupWordTiles();
    }
}

void HackingMenu::DoInGame() {
    int32_t ioffset = GetSelectedTileIndex() - 7;
    if ((ioffset != iLastOffset || HackingMenu::pim->pOverTileTarget != pLastTarget) &&
        (ioffset >= 0 || static_cast<int32_t>(iLastOffset) >= 0)) {
        iLastOffset = ioffset;
        pLastTarget = HackingMenu::pim->pOverTileTarget;
        if (ioffset >= 0) {
            if (bSuppressEntry) {
                ClearEntry();
                const char* pcString = HackingMenu::pim->pOverTileTarget->GetString(Tile::kTileValue_String);
                if (!std::isalpha(pcString[ioffset + 7])) {
                    bSuppressEntry = false;
                    pLastTarget = nullptr;
                }
            }
            else {
                char floatingText[32];
                double dCol = HackingMenu::pim->pOverTileTarget->GetFloat(Tile::kTileValue_User0) * 17.0;
                uint32_t uiBufferPos = static_cast<int64_t>(
                    (HackingMenu::pim->pOverTileTarget->GetFloat(Tile::kTileValue_ListIndex) + dCol) * 12.0 +
                    static_cast<double>(ioffset)
                );
                BSSimpleList<char*>* pnode = &wordList;
                for (uint32_t i = 0;
                     i < iWordsHave && (uiBufferPos < iBufferPos[i] || uiBufferPos >= iWordLength + iBufferPos[i]);
                     ++i) {
                    pnode = pnode->m_pkNext;
                }
                ourMemset(floatingText, 0, 32u);
                if (pnode) {
                    ourMemcpy(floatingText, pnode->m_item, iWordLength);
                }
                else if (xUsedBonuses.FindPos(uiBufferPos, nullptr)) {
                    const char* pcString = HackingMenu::pim->pOverTileTarget->GetString(Tile::kTileValue_String);
                    floatingText[0] = pcString[GetSelectedTileIndex()];
                }
                else {
                    const char* pcString = HackingMenu::pim->pOverTileTarget->GetString(Tile::kTileValue_String);
                    int32_t iIndex = GetSelectedTileIndex();
                    HackingMenu::GetGarbageString(floatingText, &pcString[iIndex]);
                }
                if (xEntryText != floatingText) {
                    xEntryText = floatingText;
                    // 0xFFFFFFFF: the ID of an empty handle, which ends the queue.
                    for (BSSimpleList<BSSoundHandle>* pSoundNode = &HackingMenu::listSoundsToPlay;
                         pSoundNode && pSoundNode->m_item != 0xFFFFFFFFu;
                         pSoundNode = pSoundNode->m_pkNext) {
                        pSoundNode->m_item.Release();
                    }
                    HackingMenu::listSoundsToPlay.RemoveAll();
                    uint32_t uiCount = BSstrlen(floatingText);
                    while (uiCount--) {
                        BSSoundHandle handle = BSAudioManager::QInstance().GetSoundHandleByName(
                            "UIHackingCharSingle",
                            289u
                        );
                        HackingMenu::listSoundsToPlay.AddHead(handle);
                        if (uiCount) {
                            uint32_t uiDisplayRate = xEntryText.uiDisplayRate;
                            // The delay is converted to a 64-bit integer and only its low 32 bits are passed.
                            handle.PlayAfter(
                                static_cast<uint32_t>(
                                    static_cast<int64_t>(static_cast<double>(uiCount * uiDisplayRate) * 1.5)
                                ),
                                0
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
                        int32_t ix1 = static_cast<int32_t>(
                            HackingMenu::pim->pOverTileTarget->GetFloat(Tile::kTileValue_X)
                        );
                        int32_t iy1 = static_cast<int32_t>(
                            HackingMenu::pim->pOverTileTarget->GetFloat(Tile::kTileValue_Y)
                        );
                        int32_t ix2 = ix1 + 119;
                        int32_t iy2 = iy1;
                        if (poverstring[ibreakpos + ioffset]) {
                            if (!std::isalpha(poverstring[ibreakpos + ioffset])) {
                                bbreakfound = true;
                                ibreakpos = iWordLength - ibreakpos;
                                if (HackingMenu::pim->pOverTileTarget->GetFloat(Tile::kTileValue_ListIndex) == 0.0) {
                                    int64_t iRow = static_cast<int64_t>(
                                        HackingMenu::pim->pOverTileTarget->GetFloat(Tile::kTileValue_User0)
                                    );
                                    Tile* pprev = FindTile(16u, iRow - 1);
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
                            if (HackingMenu::pim->pOverTileTarget->GetFloat(Tile::kTileValue_ListIndex) + 1.0 < 17.0) {
                                iy2 = iy1 +
                                    static_cast<int32_t>(
                                          HackingMenu::pim->pOverTileTarget->GetFloat(Tile::kTileValue_Height)
                                    );
                            }
                            else {
                                int64_t iRow = static_cast<int64_t>(
                                    HackingMenu::pim->pOverTileTarget->GetFloat(Tile::kTileValue_User0)
                                );
                                Tile* pnext = FindTile(0, iRow + 1);
                                ix2 = static_cast<int32_t>(pnext->GetFloat(Tile::kTileValue_X) + 119.0);
                                iy2 = static_cast<int32_t>(pnext->GetFloat(Tile::kTileValue_Y));
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
                    double dX = HackingMenu::pim->pOverTileTarget->GetFloat(Tile::kTileValue_X);
                    float fX = dX + static_cast<double>(17 * ioffset);
                    pTiles[2]->Set(Tile::kTileValue_User1, fX, true);
                    float fY = HackingMenu::pim->pOverTileTarget->GetFloat(Tile::kTileValue_Y);
                    pTiles[2]->Set(Tile::kTileValue_User2, fY, true);
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
    const char* pcString = ptile->GetString(4036);
    ourMemcpy(newbuffer, pcString, 20u);
    for (icnt = 0; icnt < this->iWordLength && ibufferpos < 12; ++icnt) {
        newbuffer[ibufferpos++ + 7] = 46;
    }
    ptile->Set(4036, newbuffer, true);
    if (IOLines[i] && IOLines[i]->pTextTile && IOLines[i]->pTextTile->IsTrue(4015) && icnt < this->iWordLength) {
        int32_t iPos = 0;
        ptile = IOLines[i]->pTextTile;
        pcString = ptile->GetString(4036);
        ourMemcpy(newbuffer, pcString, 20u);
        while (icnt++ < this->iWordLength) {
            newbuffer[iPos++ + 7] = 46;
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
        int64_t iNext = static_cast<int64_t>(
            static_cast<double>(static_cast<int64_t>(BSGetTickCount())) + fBlinkInterval
        );
        uiNextBlinkTime = static_cast<int32_t>(iNext);
    }
    else if (!pCursorVis) {
        pCursorVis = pTiles[4]->GetValue(Tile::kTileValue_Visible);
    }
}

void HackingMenu::UpdateCursorPos() {
    uint32_t uiLength = 0;
    int32_t iX = 0;
    if (pTiles[5]->IsTrue(4003)) {
        const char* pcString = pTiles[5]->GetString(4036);
        if (pcString) {
            uiLength = BSstrlen(pcString);
        }
    }
    for (Tile* pTile = pTiles[5]; pTile && pTile != pTiles[4]->pParent; pTile = pTile->pParent) {
        iX += static_cast<int32_t>(pTile->GetFloat(4001));
    }
    pTiles[4]->Set(4001, static_cast<int32_t>(iX + 17 * uiLength));
}

void HackingMenu::UpdateScrollingCursor(HackingMenu::TerminalIO* apIO) {
    if (apIO && apIO->pTextTile) {
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
    Tile* pSquare = HackingMenu::pMe->RenderTemplate(pTiles[1], "hacking_guess_template", nullptr);
    uint32_t uiCount = xGuessTiles.m_uiCount;
    pSquare->Set(4012, uiCount);
    xGuessTiles.AddTail(pSquare);
    BSStringT<char> strName;
    uiCount = xGuessTiles.m_uiCount;
    strName.SPrintF("%s_%d", pSquare->GetName(), uiCount);
    pSquare->SetName(static_cast<const char*>(strName.pString));
}

Tile* HackingMenu::Create(TESObjectREFR* apTerminalRef) {
    int32_t iClassID = HackingMenu::GetClassStatic();
    Tile* pold_menu_tile = Menu::GetMenuByClass(iClassID);
    if (pold_menu_tile) {
        delete pold_menu_tile;
    }
    HackingMenu::pim = InterfaceManager::Instance();
    float ftop_depth = Tile::GetMaximumDepth();
    SystemColorManager* pColorManager = SystemColorManager::GetInstance();
    pColorManager->GetColor(3, HackingMenu::SaveColor);
    BGSTerminal* pterminal = static_cast<BGSTerminal*>(apTerminalRef->GetObjectReference());
    if (pterminal->UsesAltColor()) {
        NiColorA kAltColor(1.0f, 1.0f, 1.0f, 0.0f);
        pColorManager = SystemColorManager::GetInstance();
        pColorManager->SetColor(3, kAltColor);
    }
    g_iHackingFlashOnDuration = iHackingFlashOnDuration.Int();
    g_iHackingFlashOffDuration = iHackingFlashOffDuration.Int();
    Tile* ptile = HackingMenu::pim->GetMenusRoot()->ReadFile("Data\\Menus\\hacking_menu.xml");
    Menu* pmenu = ptile->GetMenu();
    if (pmenu && pmenu->GetClass() == HackingMenu::GetClassStatic()) {
        int32_t imissingIndex;
        pmenu->SetMenuTile(static_cast<TileMenu*>(ptile), false);
        HackingMenu::pMe = static_cast<HackingMenu*>(pmenu);
        if (static_cast<HackingMenu*>(pmenu)->HackingMenu::IsWellFormedMenu(imissingIndex)) {
            if (ptile->GetFloat(4007) == 6006.0 || ptile->GetFloat(4007) == 102.0) {
                ptile->Set(4013, ftop_depth, true);
            }
            const char* pcLockout3 = sHackingLockout3.String();
            HackingMenu::pMe->pTiles[6]->Set(4036, pcLockout3, true);
            const char* pcLockout4 = sHackingLockout4.String();
            HackingMenu::pMe->pTiles[7]->Set(4036, pcLockout4, true);
            BGSTerminal* pterm = nullptr;
            if (apTerminalRef) {
                pterm = static_cast<BGSTerminal*>(apTerminalRef->GetObjectReference());
            }
            if (pterm && pPlayer->GetTerminalAccess(apTerminalRef) == PlayerCharacter::TAT_LOCKED) {
                ptile->Set(4102, 1);
                HackingMenu::pMe->eStage = HackingMenu::HMS_FINISHED;
                HackingMenu::pMe->pTiles[4]->Set(4003, 0);
            }
            else {
                HackingMenu::pMe->pTerminal = pterm;
                HackingMenu::pMe->pTerminalRef = apTerminalRef;
                uint32_t uiWordLength;
                if (HackingMenu::pMe->pTerminal) {
                    uiWordLength = HackingMenu::pMe->pTerminal->GetHackMiniGameValue(HackingMenu::pMe->pTerminalRef);
                }
                else {
                    uiWordLength = 12;
                }
                HackingMenu::pMe->iWordLength = uiWordLength;
                HackingMenu::pMe->iWordLength = BSMIN(12u, HackingMenu::pMe->iWordLength);
                iClassID = HackingMenu::GetClassStatic();
                HackingMenu::pMe->bTutorial = HackingMenu::pim->xTutorialManager
                                                  .ShowMessage(Interface::TUT_HACKING, iClassID, 512u);
                float fscienceSkill = pPlayer->GetClampedActorFloatValue(ActorValue::SCIENCE);
                BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::HACKING_SCIENCE_BONUS, pPlayer, &fscienceSkill);
                // BSMIN compares the settings' values as unsigned.
                uint32_t imaxWords = BSMIN(20u, static_cast<uint32_t>(iHackingMaxWords.Int()));
                uint32_t uiMinWordsSetting = iHackingMinWords.Int();
                uint32_t iminWords = BSMIN(imaxWords, uiMinWordsSetting);
                float fMinSkill = HackingMenu::pMe->pTerminal->GetMinSkillRequiredToHack(apTerminalRef);
                float fClampedMinSkill = BSMAX(0.0f, fMinSkill);
                float fminRqSkill = BSMIN(100.0f, fClampedMinSkill);
                float fSkillOver = fscienceSkill - fminRqSkill;
                float fskillDiff = BSMAX(0.0f, fSkillOver);
                float fskillRange = 100.0 - fminRqSkill;
                double dPct = fskillRange == 0.0 ? 0.5 : 1.0 - fskillDiff / fskillRange;
                float fWordFraction = dPct;
                float fExtraWords = static_cast<double>(imaxWords - iminWords) * fWordFraction;
                // The rounded count is converted to a 64-bit integer and its low 32 bits are added.
                uint32_t iwordsWant = iminWords +
                    static_cast<uint32_t>(static_cast<int64_t>(BSRoundToNearest(fExtraWords, 1.0f)));
                if (iwordsWant > 20) {
                    iwordsWant = 20;
                }
                HackingMenu::pMe->Prepare(iwordsWant);
                ptile->Set(4100, ">", true);
                Tile::Value* pVisibility = HackingMenu::pMe->pTiles[5]->GetValue(4003);
                Tile::Value* pString = HackingMenu::pMe->pTiles[5]->GetValueQ(4036);
                HackingMenu::pMe->xEntryText.SetTraits(pString, pVisibility);
                // The rate is tested and converted as unsigned, and the interval is converted through a 64-bit integer,
                // keeping its low 32 bits.
                if (static_cast<uint32_t>(iHackingInputRate.Int()) > 0) {
                    float fRaw = 1000.0 / static_cast<double>(static_cast<uint32_t>(iHackingInputRate.Int()));
                    uint32_t uiInterval = static_cast<uint32_t>(static_cast<int64_t>(BSRoundToNearest(fRaw, 1.0f)));
                    HackingMenu::pMe->xEntryText.SetDisplayInterval(uiInterval);
                }
                const char* ptemplate = "hacking_intro_template";
                HackingMenu::pMe->IOLines[0] = new TerminalIO(
                    static_cast<const char*>(sHackingSecurityReset),
                    HackingMenu::pMe->pTiles[0],
                    ptemplate,
                    HackingMenu::pMe->iYStart,
                    0
                );
                HackingMenu::pMe->IOLines[1] = new TerminalIO(
                    static_cast<const char*>(sHackingIntro01),
                    HackingMenu::pMe->pTiles[0],
                    ptemplate,
                    HackingMenu::pMe->iYStart + 2 * HackingMenu::pMe->iYInterval,
                    0
                );
                HackingMenu::pMe->IOLines[2] = new TerminalIO(
                    static_cast<const char*>(sHackingIntro02),
                    HackingMenu::pMe->pTiles[0],
                    ptemplate,
                    HackingMenu::pMe->iYStart + 4 * HackingMenu::pMe->iYInterval,
                    1
                );
                HackingMenu::pMe->IOLines[3] = new TerminalIO(
                    static_cast<const char*>(sHackingIntro03),
                    HackingMenu::pMe->pTiles[0],
                    ptemplate,
                    HackingMenu::pMe->iYStart + 6 * HackingMenu::pMe->iYInterval,
                    0
                );
                HackingMenu::pMe->IOLines[4] = new TerminalIO(
                    static_cast<const char*>(sHackingIntro04),
                    HackingMenu::pMe->pTiles[0],
                    ptemplate,
                    HackingMenu::pMe->iYStart + 8 * HackingMenu::pMe->iYInterval,
                    1
                );
                HackingMenu::pMe->IOLines[5] = new TerminalIO(
                    static_cast<const char*>(sHackingIntro05),
                    HackingMenu::pMe->pTiles[0],
                    ptemplate,
                    HackingMenu::pMe->iYStart + 9 * HackingMenu::pMe->iYInterval,
                    1
                );
                HackingMenu::pMe->IOLines[6] = new TerminalIO(
                    static_cast<const char*>(sHackingIntro06),
                    HackingMenu::pMe->pTiles[0],
                    ptemplate,
                    HackingMenu::pMe->iYStart + 11 * HackingMenu::pMe->iYInterval,
                    0
                );
                HackingMenu::pMe->IOLines[7] = new TerminalIO(
                    static_cast<const char*>(sHackingIntro07),
                    HackingMenu::pMe->pTiles[0],
                    ptemplate,
                    HackingMenu::pMe->iYStart + 12 * HackingMenu::pMe->iYInterval,
                    0
                );
                HackingMenu::pMe->IOLines[8] = new TerminalIO(
                    static_cast<const char*>(sHackingIntro08),
                    HackingMenu::pMe->pTiles[0],
                    ptemplate,
                    HackingMenu::pMe->iYStart + 13 * HackingMenu::pMe->iYInterval,
                    0
                );
                HackingMenu::pMe->IOLines[9] = new TerminalIO(
                    static_cast<const char*>(sHackingIntro09),
                    HackingMenu::pMe->pTiles[0],
                    ptemplate,
                    HackingMenu::pMe->iYStart + 14 * HackingMenu::pMe->iYInterval,
                    0
                );
                HackingMenu::pMe->IOLines[10] = new TerminalIO(
                    static_cast<const char*>(sHackingIntro10),
                    HackingMenu::pMe->pTiles[0],
                    ptemplate,
                    HackingMenu::pMe->iYStart + 15 * HackingMenu::pMe->iYInterval,
                    0
                );
                HackingMenu::pMe->IOLines[11] = new TerminalIO(
                    static_cast<const char*>(sHackingIntro11),
                    HackingMenu::pMe->pTiles[0],
                    ptemplate,
                    HackingMenu::pMe->iYStart + 16 * HackingMenu::pMe->iYInterval,
                    0
                );
                HackingMenu::pMe->IOLines[12] = new TerminalIO(
                    static_cast<const char*>(sHackingIntro12),
                    HackingMenu::pMe->pTiles[0],
                    ptemplate,
                    HackingMenu::pMe->iYStart + 17 * HackingMenu::pMe->iYInterval,
                    0
                );
                HackingMenu::pMe->IOLines[13] = new TerminalIO(
                    static_cast<const char*>(sHackingIntro13),
                    HackingMenu::pMe->pTiles[0],
                    ptemplate,
                    HackingMenu::pMe->iYStart + 19 * HackingMenu::pMe->iYInterval,
                    1
                );
            }
            HackingMenu::pMe->Sounds->Play(false);
            pmenu->PrepForVisibility(false);
            return ptile;
        }

        Warning("MENUS: Hacking Menu Creation Failed... Missing index = #%d", imissingIndex);
        return nullptr;
    }

    if (pmenu && pmenu->pMenu && pmenu) {
        delete pmenu;
    }

    return nullptr;
}

void HackingMenu::Close() {
    if (HackingMenu::pMe) {
        PlayTerminalShutdownSound();
        HackingMenu::pMe->pMenu->Set(4033, 0.75f, true);
        if (HackingMenu::pim->GetCurrentRenderedMenu()) {
            NiNode* pMenuRoot = static_cast<NiNode*>(HackingMenu::pim->GetCurrentRenderedMenu()->GetMenuRoot());
            if (pMenuRoot == HackingMenu::pMe->pMenu->GetModel()) {
                FORenderedMenu* pRenderedMenu = HackingMenu::pim->GetCurrentRenderedMenu();
                FORenderedTerminal* pterm = dynamic_cast<FORenderedTerminal*>(pRenderedMenu);
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
    uint32_t uiCount = 1;
    char* pcOpen = std::strchr(Str, *apSrc);
    if (pcOpen) {
        while (true) {
            if (!apSrc[uiCount] || std::isalpha(apSrc[uiCount])) {
                return ourMemcpy(apDest, apSrc, 1u);
            }
            if (apSrc[uiCount] == pCloseBrackets[pcOpen - Str]) {
                break;
            }
            ++uiCount;
        }
        ++uiCount;
    }
    return ourMemcpy(apDest, apSrc, uiCount);
}

void HackingText::SetDisplayInterval(uint32_t aiInterval) {
    uiDisplayRate = BSMAX(1u, aiInterval);
}

bool HackingText::operator!=(const char* apString) {
    return strText != apString;
}
