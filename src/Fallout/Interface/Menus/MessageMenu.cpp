#include "Fallout/Interface/Menus/MessageMenu.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSAudioManager.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSMain/BSTaskManager.h"
#include "BSMain/MessageHandler.h"
#include "BSMain/Setting.h"
#include "BSMenu/Font.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileMenu.h"
#include "Fallout Shared/BGSEntryPointFunctionDataActivateChoice.h"
#include "Fallout Shared/BGSOpenCloseForm.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectDOOR.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/ListBox.h"
#include "Fallout/Interface/Menus/HUDMainMenu.h"
#include "Fallout/Interface/Menus/StartMenu.h"
#include "Fallout/Interface/Menus/StatsMenu.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

#include <cctype>
#include <cstdint>
#include <limits>
#include <string>

MessageMenu::MessageMenuEntryArray MessageMenu::xEntryList;
MessageMenu::xPerkActivations_t MessageMenu::xPerkActivations;

MessageMenu::MessageMenuEntry::MessageMenuEntry()
  : strTitle(nullptr), cFirstButtonResult(0), fpResultCallback(nullptr), iWarningContext(WARN_DEFAULT), bIsModal(true),
    iXPosOverride(-1), iYPosOverride(-1), fMillisecondsVisible(0.0f),
    fBackgroundTextureOpacity(
        InterfaceManager::Instance()->pStringRoot->GetFloat(Tile::TextToTrait("_background_fill_alpha"))
    ),
    uiWidthOverride(0) {}

MessageMenu::MessageMenuEntry::~MessageMenuEntry() {
    ourFree(strTitle);
    strTitle = nullptr;
}

MessageMenu::MessageMenu() : uiMessageStartTime(0) {
    InterfaceManager::Instance()->cLastMessageButtonClicked = -1;
}

MessageMenu::~MessageMenu() {
    Interface::bAllowPauseMenu = true;
    MessageMenu::pMe = nullptr;
    if (MessageMenu::bCheckForDeletion) {
        MessageMenu::bDeletedDuringCheck = true;
    }
}

void MessageMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
    if (aiID >= 0 && aiID < 6) {
        pTiles[aiID] = apTile;
    }
}

void MessageMenu::DoClick(int32_t aiID, Tile* apTarget) {
    if (!apTarget && aiID >= 0 && aiID < 6) {
        apTarget = pTiles[aiID];
    }
    if (aiID == 7 && apTarget) {
        char cButtonResult = buttonList.GetDataFor(apTarget);
        InterfaceManager::Instance()->cLastMessageButtonClicked = cButtonResult;
        MessageMenu::bCheckForDeletion = true;
        if (MessageMenu::GetQueueHead() && MessageMenu::GetQueueHead()->fpResultCallback) {
            MessageMenu::GetQueueHead()->fpResultCallback();
        }

        MessageMenu::bCheckForDeletion = false;
        if (MessageMenu::bDeletedDuringCheck) {
            MessageMenu::bDeletedDuringCheck = false;
        }
        else {
            MessageMenu::ClearMessageQueueHead();
        }
    }
}

bool MessageMenu::DoKey(int32_t aiKey) {
    bool bhandled = false;
    ListBoxBase* pbuttons = &buttonList;
    if (MessageMenu::GetQueueHead()) {
        if (aiKey != 1 || MessageMenu::GetQueueHead()->iWarningContext == WARN_INGAME_MESSAGE) {
            if (aiKey == 32) {
                DoClick(-1, buttonList.GetFirstItem());
                bhandled = true;
            }
            else if (MessageMenu::GetQueueHead()->strHotkeys.GetLength()) {
                using strtemp_t = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;
                strtemp_t strtemp(static_cast<const char*>(MessageMenu::GetQueueHead()->strHotkeys.pString));
                for (uint32_t hotkeyIndex = 0; hotkeyIndex < strtemp.size(); ++hotkeyIndex) {
                    int32_t iHotkey = std::toupper(strtemp[hotkeyIndex]);
                    if (iHotkey == std::toupper(aiKey)) {
                        DoClick(7, pbuttons->GetTileAt(hotkeyIndex, false));
                        bhandled = true;
                        break;
                    }
                }
            }
        }
        else {
            DoClick(7, pbuttons->GetTileAt(buttonList.NumItems() - 1, false));
            bhandled = true;
        }
    }
    return bhandled;
}

void MessageMenu::DoIdle() {
    MMTimer.Update();
    static int32_t uiLastUpdate = MMTimer.iLastTime;
    if (MMTimer.iLastTime - uiLastUpdate >= 200) {
        uiLastUpdate = MMTimer.iLastTime;
        if (MessageMenu::GetQueueHead()) {
            if (!MessageMenu::GetQueueHead()->bIsModal && pTiles[0]->IsTrue(4003)) {
                if (xFadeState == Interface::FADE_HIDDEN) {
                    if (uiMessageStartTime) {
                        MessageMenu::ClearMessageQueueHead();
                        uiMessageStartTime = 0;
                    }
                    else {
                        StartFadeIn();
                    }
                }
                else if (xFadeState == Interface::FADE_SHOWN) {
                    MessageMenu::MessageMenuEntry* pcurrMessage = MessageMenu::GetQueueHead();
                    if (!uiMessageStartTime) {
                        uiMessageStartTime = MMTimer.iLastTime;
                    }
                    if (pcurrMessage->fMillisecondsVisible > 0.0 &&
                        pcurrMessage->fMillisecondsVisible <=
                            static_cast<double>(MMTimer.iLastTime - uiMessageStartTime)) {
                        if (pcurrMessage ==
                            *static_cast<MessageMenu::MessageMenuEntry**>(MessageMenu::xEntryList.GetTail())) {
                            pMenu->Set(Tile::kTileValue_DeleteOnFade, 1);
                            MessageMenu::ClearMessageQueueHead();
                        }
                        else {
                            StartFadeOut();
                        }
                    }
                }
            }
        }
    }
}

void MessageMenu::LoadNextMessage() {
    std::basic_string<char, std::char_traits<char>, std::allocator<char>> strButtonText;
    std::basic_string<char, std::char_traits<char>, std::allocator<char>> strButton;
    if (MessageMenu::GetQueueHead()) {
        MessageMenu::MessageMenuEntry* pcurrMessage = MessageMenu::GetQueueHead();
        float fminHeaderWidth = 0.0f;
        float ftextWidth = 0.0f;
        float flistWidth = 0.0f;
        float fmenuWidth = 0.0f;
        int32_t iHorBufTrait = Tile::TextToTrait("_horbuf");
        float fhorizSpacing = pMenu->GetFloat(iHorBufTrait);
        bool bHasAudioOverride = false;
        if (pcurrMessage->strAudioOverride.GetLength()) {
            char* pcSound = pcurrMessage->strAudioOverride.pString;
            BSSoundHandle arHandle = BSAudio::QInstance().GetSoundHandleByName(pcSound, 289u);
            if (arHandle != 0xFFFFFFFFu) {
                bHasAudioOverride = true;
            }
        }
        if (bHasAudioOverride) {
            char* pcSound = pcurrMessage->strAudioOverride.pString;
            BSAudio::QInstance().GetSoundHandleByName(pcSound, 289u).Play(false);
        }
        else {
            BSAudio::QInstance().GetSoundHandleByName("UIPopUpMessageWindow", 289u).Play(false);
        }
        MessageMenu::pMe->pTiles[3]->Set(4036, pcurrMessage->strMessageText.pString, true);
        if (pcurrMessage->strTitle) {
            MessageMenu::pMe->pTiles[1]->Set(4036, pcurrMessage->strTitle, true);
            MessageMenu::pMe->pTiles[1]->Set(4003, 1);
            float fWrapWidth;
            if (pcurrMessage->uiWidthOverride) {
                fWrapWidth = static_cast<float>(pcurrMessage->uiWidthOverride);
            }
            else {
                fWrapWidth = MessageMenu::pMe->pTiles[1]->GetFloat(4026);
            }
            if (fWrapWidth == 0.0) {
                fWrapWidth = std::numeric_limits<float>::max();
            }
            char* pcTitle = pcurrMessage->strTitle;
            fminHeaderWidth = FontManager::Instance()->CalculateStringDimensions(pcTitle, 6, fWrapWidth, 0).x;
            float fHeaderPadding = pcurrMessage->uiWidthOverride ? 0.0f : 100.0f;
            fminHeaderWidth = fminHeaderWidth + fHeaderPadding;
        }
        else {
            MessageMenu::pMe->pTiles[1]->Set(4003, 0);
        }
        uint32_t currStartIndex = 0;
        int32_t imaxButtonWidth = 0;
        float ftotalButtonHeight = 0.0f;
        buttonList.RemoveAll();
        if (pcurrMessage->strButtonsText.pString) {
            using strButtons_t = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;
            strButtons_t strButtons(static_cast<const char*>(pcurrMessage->strButtonsText.pString));
            for (int32_t ibuttonCt = 0;; ++ibuttonCt) {
                uint32_t uiLength = strButtons.size();
                if (currStartIndex >= uiLength - 1) {
                    break;
                }
                strButton = strButtons.substr(currStartIndex, strButtons.find(";", currStartIndex) - currStartIndex);
                if (strButton.size()) {
                    strButtonText = strButtons.substr(
                        currStartIndex,
                        strButtons.find(";", currStartIndex) - currStartIndex
                    );
                    Tile* ptemp = MessageMenu::pMe->buttonList.AddItem(
                        ibuttonCt + pcurrMessage->cFirstButtonResult,
                        strButtonText.c_str(),
                        nullptr,
                        nullptr
                    );
                    Tile* ptextChildTile = ptemp->GetChildByName("ListItemText");
                    ptemp->Set(4010, 7);
                    if (ptextChildTile) {
                        float fWrapWidth;
                        if (pcurrMessage->uiWidthOverride) {
                            fWrapWidth = static_cast<float>(pcurrMessage->uiWidthOverride);
                        }
                        else {
                            fWrapWidth = ptextChildTile->GetFloat(4026);
                        }
                        if (fWrapWidth == 0.0) {
                            fWrapWidth = std::numeric_limits<float>::max();
                        }
                        const char* pcItemText = ptextChildTile->GetString(4036);
                        double dButtonWidth = FontManager::Instance()
                                                  ->CalculateStringDimensions(pcItemText, 2, fWrapWidth, 0)
                                                  .x;
                        imaxButtonWidth = BSMAX(imaxButtonWidth, static_cast<int32_t>(dButtonWidth));
                    }
                    double dButtonHeight = ptemp->GetFloat(4016);
                    ftotalButtonHeight = dButtonHeight + ftotalButtonHeight;
                }
                currStartIndex = strButtons.find(";", currStartIndex);
                uiLength = strButtons.size();
                if (currStartIndex < uiLength - 1) {
                    ++currStartIndex;
                }
            }
        }
        if (pcurrMessage->strIconFilename.GetLength()) {
            MessageMenu::pMe->pTiles[2]->Set(4044, pcurrMessage->strIconFilename.pString, true);
            MessageMenu::pMe->pTiles[2]->Set(4003, 1);
            if (pcurrMessage->uiWidthOverride) {
                float fOverrideWidth = static_cast<float>(pcurrMessage->uiWidthOverride);
                float fPaddedWidth = ftextWidth + 120.0;
                ftextWidth = BSMIN(fPaddedWidth, fOverrideWidth);
            }
            else {
                ftextWidth = ftextWidth + 120.0;
            }
        }
        else {
            MessageMenu::pMe->pTiles[2]->Set(4003, 0);
        }
        if (pcurrMessage->uiWidthOverride) {
            float fOverrideWidth = static_cast<float>(pcurrMessage->uiWidthOverride);
            float fPaddedWidth = static_cast<double>(imaxButtonWidth) + 80.0;
            flistWidth = BSMIN(fPaddedWidth, fOverrideWidth);
        }
        else {
            flistWidth = static_cast<double>(imaxButtonWidth) + 80.0;
        }
        MessageMenu::pMe->pTiles[4]->Set(4016, ftotalButtonHeight, false);
        MessageMenu::pMe->pTiles[4]->Set(4017, flistWidth, false);
        MessageMenu::pMe->buttonList.Synchronize();
        if (pcurrMessage->uiWidthOverride) {
            float fOverrideWidth = static_cast<float>(pcurrMessage->uiWidthOverride);
            float fPaddedWidth = fhorizSpacing + fhorizSpacing + flistWidth;
            flistWidth = BSMIN(fPaddedWidth, fOverrideWidth);
        }
        else {
            flistWidth = fhorizSpacing + fhorizSpacing + flistWidth;
        }
        float fWrapWidth = std::numeric_limits<float>::max();
        if (pcurrMessage->uiWidthOverride) {
            double dInterfaceWidth = InterfaceManager::GetScreenWidth();
            double dWidthOverride = static_cast<double>(pcurrMessage->uiWidthOverride);
            fWrapWidth = dWidthOverride / InterfaceManager::GetRealScreenWidth() * dInterfaceWidth;
        }
        else {
            int32_t iMaxWidthTrait = Tile::TextToTrait("_MaxMenuWidth");
            fWrapWidth = MessageMenu::pMe->pMenu->GetFloat(iMaxWidthTrait);
        }
        fWrapWidth = fWrapWidth - (fhorizSpacing + fhorizSpacing);
        if (MessageMenu::pMe->pTiles[2]->IsTrue(4003)) {
            fWrapWidth = fWrapWidth - 120.0;
        }
        MessageMenu::pMe->pTiles[3]->Set(4026, fWrapWidth, true);
        float fTextWrapWidth = MessageMenu::pMe->pTiles[3]->GetFloat(4026);
        char* pcText = pcurrMessage->strMessageText.pString;
        NiPoint3 textDims = FontManager::Instance()->CalculateStringDimensions(pcText, 2, fTextWrapWidth, 0);
        ftextWidth = fhorizSpacing + fhorizSpacing + textDims.x + ftextWidth;
        if (textDims.z > 1.0) {
            MessageMenu::pMe->pTiles[1]->Set(4023, 1);
            MessageMenu::pMe->pTiles[3]->Set(4023, 1);
        }
        else {
            MessageMenu::pMe->pTiles[1]->Set(4023, 2);
            MessageMenu::pMe->pTiles[3]->Set(4023, 2);
        }
        if (pcurrMessage->uiWidthOverride) {
            double dInterfaceWidth = InterfaceManager::GetScreenWidth();
            double dWidthOverride = static_cast<double>(pcurrMessage->uiWidthOverride);
            fmenuWidth = dWidthOverride / InterfaceManager::GetRealScreenWidth() * dInterfaceWidth;
            dInterfaceWidth = InterfaceManager::GetScreenWidth();
            double dScreenSafeX = InterfaceManager::GetScreenSafeX();
            float fSafeWidth = dInterfaceWidth - (dScreenSafeX + dScreenSafeX);
            fmenuWidth = BSMIN(fmenuWidth, fSafeWidth);
        }
        else {
            fmenuWidth = BSMAX(fminHeaderWidth, ftextWidth);
            fmenuWidth = BSMAX(fmenuWidth, flistWidth);
            int32_t iMaxWidthTrait = Tile::TextToTrait("_MaxMenuWidth");
            float fMaxMenuWidth = pMenu->GetFloat(iMaxWidthTrait);
            fmenuWidth = BSMIN(fmenuWidth, fMaxMenuWidth);
            int32_t iMinWidthTrait = Tile::TextToTrait("_MinMenuWidth");
            float fMinMenuWidth = pMenu->GetFloat(iMinWidthTrait);
            fmenuWidth = BSMAX(fmenuWidth, fMinMenuWidth);
        }
        MessageMenu::pMe->pTiles[0]->Set(4017, fmenuWidth, true);
        Interface::bAllowPauseMenu = pcurrMessage->iWarningContext == WARN_INGAME_MESSAGE;
        pMenu->Set(Tile::kTileValue_DeleteOnFade, 0);
        pMenu->Set(4003, 1);
        if (pcurrMessage->bIsModal) {
            MessageMenu::pMe->pTiles[0]->Set(4009, 1.0f, true);
            uint32_t uiClass = GetClass();
            if (!InterfaceManager::Instance()->IsMenuInStack(uiClass)) {
                Interface::AddToEnterStack(GetClass(), false);
            }
        }
        else {
            MessageMenu::pMe->pTiles[0]->Set(4009, 0.0f, true);
            uint32_t uiClass = GetClass();
            if (InterfaceManager::Instance()->IsMenuInStack(uiClass)) {
                Interface::PopFromEnterStack(GetClass(), true);
            }
            float fScreenWidth = InterfaceManager::GetScreenWidth();
            float fMainWidth = MessageMenu::pMe->pTiles[0]->GetFloat(4017);
            float fScreenHeight = InterfaceManager::GetScreenHeight();
            float fMainHeight = MessageMenu::pMe->pTiles[0]->GetFloat(4016);
            float fMenuX = (fScreenWidth - fMainWidth) / 2.0;
            float fMenuY = (fScreenHeight - fMainHeight) / 2.0;
            if (pcurrMessage->iXPosOverride > -1) {
                double dXPosOverride = static_cast<double>(pcurrMessage->iXPosOverride);
                float fConvertedXPosOverride = dXPosOverride / InterfaceManager::GetRealScreenWidth() * fScreenWidth;
                float fSafeX = InterfaceManager::GetScreenSafeX();
                fMenuX = BSMAX(fConvertedXPosOverride, fSafeX);
                double dFreeWidth = fScreenWidth - fMainWidth;
                float fMaxX = dFreeWidth - InterfaceManager::GetScreenSafeX();
                fMenuX = BSMIN(fMenuX, fMaxX);
            }
            if (pcurrMessage->iYPosOverride > -1) {
                double dYPosOverride = static_cast<double>(pcurrMessage->iYPosOverride);
                float fConvertedYPosOverride = dYPosOverride / InterfaceManager::GetRealScreenHeight() * fScreenHeight;
                float fSafeY = InterfaceManager::GetScreenSafeY();
                fMenuY = BSMAX(fConvertedYPosOverride, fSafeY);
                double dFreeHeight = fScreenHeight - fMainHeight;
                float fMaxY = dFreeHeight - InterfaceManager::GetScreenSafeY();
                fMenuY = BSMIN(fMenuY, fMaxY);
            }
            MessageMenu::pMe->pTiles[0]->Set(4001, fMenuX, true);
            MessageMenu::pMe->pTiles[0]->Set(4002, fMenuY, true);
        }
        MessageMenu::pMe->pTiles[5]->Set(4009, pcurrMessage->fBackgroundTextureOpacity, true);
    }
}

bool MessageMenu::IsWellFormedMenu() {
    for (Tile* pTile : pTiles) {
        if (!pTile) {
            return false;
        }
    }
    return true;
}

void MessageMenu::Close() {
    if (pMe) {
        pMe->GetMenuTile()->Set(6002, 1);
        if (Interface::GetCurrentRenderedMenu()) {
            pMe->InstantFadeOut();
        }
        else {
            pMe->StartFadeOut();
        }
        if (Interface::InDialog() && !StartMenu::PauseMenuIsActive()) {
            Interface::SetDialogPause(false);
        }
    }
}

void MessageMenu::ProcessYesToAll() {
    if (MessageMenu::GetQueueHead()) {
        WARNING_TYPES disableContext = MessageMenu::GetQueueHead()->iWarningContext;
        MessageHandler::pHandler->DisableContextMessages(disableContext);
        MessageHandler::SetYesToAll(true);
        for (int32_t imessageCt = MessageMenu::iStartIndex; imessageCt < MessageMenu::xEntryList.iSize; ++imessageCt) {
            if (MessageMenu::xEntryList.GetAt(imessageCt)->iWarningContext == disableContext) {
                MessageMenu::MessageMenuEntry* pEntry = MessageMenu::xEntryList[imessageCt];
                if (pEntry) {
                    delete pEntry;
                }
                MessageMenu::xEntryList.Remove(imessageCt--, true);
            }
        }
    }
}

int32_t MessageMenu::GetClassStatic() {
    return Interface::eMessageMenu;
}

bool MessageMenu::AddMessageToQueue(
    const char* axText,
    const char* apcIconFilename,
    const char* apcTitle,
    const char* apcSound,
    void (*afpResult)(),
    int32_t aiFirstButtonResult,
    int32_t aiWarningContext,
    const char* axFirst,
    va_list* axList,
    bool abModal,
    int32_t aiXPosOverride,
    int32_t aiYPosOverride,
    float afSecondsVisible,
    float afBackgroundTextureOpacityOverride,
    float afMenuWidthOverride
) {
    std::basic_string<char, std::char_traits<char>, std::allocator<char>> strHotkey;
    MessageMenu::MessageMenuEntry* pnewMessage = new MessageMenu::MessageMenuEntry;
    BSStringT<char> strbuttonName(axFirst);
    pnewMessage->fpResultCallback = afpResult;
    pnewMessage->cFirstButtonResult = aiFirstButtonResult;
    pnewMessage->iWarningContext = static_cast<WARNING_TYPES>(aiWarningContext);
    pnewMessage->strIconFilename = apcIconFilename;
    pnewMessage->strHotkeys = "";
    pnewMessage->strMessageText = axText;
    pnewMessage->strAudioOverride = apcSound;
    pnewMessage->bIsModal = abModal;
    pnewMessage->iXPosOverride = aiXPosOverride;
    pnewMessage->iYPosOverride = aiYPosOverride;
    pnewMessage->fMillisecondsVisible = afSecondsVisible * 1000.0;
    if (afBackgroundTextureOpacityOverride > 0.0) {
        pnewMessage->fBackgroundTextureOpacity = afBackgroundTextureOpacityOverride * 255.0;
    }
    pnewMessage->uiWidthOverride = static_cast<int64_t>(afMenuWidthOverride);
    int32_t ititleLen;
    if (apcTitle) {
        ititleLen = BSstrlen(apcTitle);
    }
    else {
        ititleLen = 0;
    }
    if (ititleLen <= 0) {
        pnewMessage->strTitle = nullptr;
    }
    else {
        pnewMessage->strTitle = static_cast<char*>(ourMalloc(ititleLen + 1));
        BSstrcpy(pnewMessage->strTitle, ititleLen + 1, apcTitle);
    }
    while (strbuttonName.GetLength()) {
        using strtemp_t = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;
        strtemp_t strtemp(static_cast<const char*>(strbuttonName.pString));
        uint32_t hotkeyIndex = strtemp.find(38, 0);
        if (hotkeyIndex != -1) {
            strHotkey = strtemp.substr(hotkeyIndex + 1, 1u);
            pnewMessage->strHotkeys += strHotkey.c_str();
            strtemp.replace(hotkeyIndex, 1u, "");
        }
        if (BSstrcmp(strtemp.c_str(), " ")) {
            pnewMessage->strButtonsText += strtemp.c_str();
        }
        pnewMessage->strButtonsText += ";";
        strbuttonName = va_arg(*axList, const char*);
    }
    if (MessageMenu::xEntryList.iSize) {
        for (int32_t i = 0; i < MessageMenu::xEntryList.iSize; ++i) {
            if (!MessageMenu::xEntryList.GetAt(i)->bIsModal && pnewMessage->bIsModal) {
                MessageMenu::xEntryList.Insert(static_cast<uint32_t>(i), pnewMessage);
                break;
            }
            if (MessageMenu::xEntryList.GetAt(i) ==
                *static_cast<MessageMenu::MessageMenuEntry**>(MessageMenu::xEntryList.GetTail())) {
                MessageMenu::xEntryList.Add(pnewMessage);
                break;
            }
        }
    }
    else {
        MessageMenu::xEntryList.Add(pnewMessage);
    }
    if (MessageMenu::pMe || (bMessageMenuDisabled || (Interface::InDialog() && !abModal))) {
        bDeferredCreation = true;
    }
    else if (iNumHWThreads.Int() <= 1) {
        MessageMenu::Create();
    }
    else {
        Interface::QueueMenuCreate(
            Interface::OPEN_MESSAGE,
            nullptr,
            nullptr,
            nullptr,
            Interface::CMM_STANDARD,
            !pnewMessage->bIsModal
        );
    }
    return true;
}

MessageMenu::MessageMenuEntry* MessageMenu::GetQueueHead() {
    MessageMenu::MessageMenuEntry* preturn = nullptr;
    if (MessageMenu::iStartIndex < MessageMenu::xEntryList.iSize) {
        return MessageMenu::xEntryList.GetAt(MessageMenu::iStartIndex);
    }
    return preturn;
}

void MessageMenu::SetPauseMenuMode(bool abFlag) {
    static char bwasDisabled;
    if (abFlag) {
        MessageMenu::iStartIndex = MessageMenu::xEntryList.iSize;
        MessageMenu::Close();
        bwasDisabled = bMessageMenuDisabled;
        MessageMenu_SetDisabled(false);
    }
    else {
        MessageMenu::iStartIndex = 0;
        // Passes the saved byte on as it is, without testing it.
        MessageMenu_SetDisabled(static_cast<uint8_t>(bwasDisabled));
    }
}

bool MessageMenu::Create() {
    InterfaceManager* pim = InterfaceManager::Instance();
    bool boverMenu = false;
    Tile* pold_menu_tile = Menu::GetMenuByClass(MessageMenu::GetClassStatic());
    if (pold_menu_tile) {
        delete pold_menu_tile;
    }
    if (Interface::GetTopMenuID()) {
        boverMenu = true;
    }
    Tile* ptile = pim->GetMenusRoot()->ReadFile("Data\\Menus\\message_menu.xml");
    MessageMenu::pMe = static_cast<MessageMenu*>(ptile->GetMenu());
    if (MessageMenu::pMe && MessageMenu::pMe->GetClass() == MessageMenu::GetClassStatic()) {
        MessageMenu::pMe->SetMenuTile(static_cast<TileMenu*>(ptile), false);
        if (MessageMenu::pMe->IsWellFormedMenu()) {
            if (ptile->GetFloat(4007) == 6006.0 || ptile->GetFloat(4007) == 102.0) {
                float fMaxDepth = Tile::GetMaximumDepth();
                ptile->Set(4013, fMaxDepth, true);
            }
            MessageMenu::pMe->buttonList.Initialize(MessageMenu::pMe->pTiles[4], "MM_ButtonTemplate");
            if (MessageMenu::GetQueueHead()) {
                MessageMenu::pMe->LoadNextMessage();
            }
            if (boverMenu) {
                float fOpacity = fPopUpBackgroundOpacity.Float() * 255.0;
                Tile* pBackground = MessageMenu::pMe->pMenu->GetChildByName("MM_Background");
                pBackground->Set(4009, fOpacity, true);
            }
            if (Interface::InDialog()) {
                Interface::SetDialogPause(true);
            }
            MessageMenu::pMe->PrepForVisibility(false);
            Interface::SetGunScopeVisible(false);
            return true;
        }

        InitializationFlow("Message Menu Creation Failed... Are your menu and art resources up to date?");
        return false;
    }

    if (MessageMenu::pMe && MessageMenu::pMe->pMenu && MessageMenu::pMe) {
        delete MessageMenu::pMe;
    }

    return false;
}

void MessageMenu::ClearMessageQueueHead() {
    if (MessageMenu::GetQueueHead()) {
        MessageMenu::MessageMenuEntry* pEntry = MessageMenu::xEntryList[MessageMenu::iStartIndex];
        if (pEntry) {
            delete pEntry;
        }
        MessageMenu::xEntryList.Remove(MessageMenu::iStartIndex, true);
    }
    if (MessageMenu::GetQueueHead()) {
        if (MessageMenu::pMe) {
            MessageMenu::pMe->LoadNextMessage();
        }
    }
    else {
        MessageMenu::Close();
    }
}

void MessageMenu::ClearMessageQueue() {
    for (MessageMenu::MessageMenuEntry* pEntry : MessageMenu::xEntryList) {
        if (pEntry) {
            delete pEntry;
        }
    }
    MessageMenu::xEntryList.Clear(true);
    MessageMenu::iStartIndex = 0;
}

bool MessageMenu::AddMessageToQueue(
    BSSimpleList<BGSEntryPointFunctionDataActivateChoice*>* apChoiceList,
    TESObjectREFR* apActivationTarget
) {
    std::basic_string<char, std::char_traits<char>, std::allocator<char>> strTargetHotkey;
    std::basic_string<char, std::char_traits<char>, std::allocator<char>> strHotkey;
    char acButtonText[268];
    MessageMenu::MessageMenuEntry* pnewMessage = new MessageMenu::MessageMenuEntry;
    pnewMessage->fpResultCallback = &MessageMenu::HandlePerkActivationResults;
    pnewMessage->cFirstButtonResult = 0;
    pnewMessage->iWarningContext = WARN_DEFAULT;
    pnewMessage->strIconFilename = nullptr;
    pnewMessage->strHotkeys = "";
    pnewMessage->strMessageText = sActivationChoiceMessage.String();
    while (apChoiceList && !apChoiceList->IsEmpty()) {
        BSstrcpy(acButtonText, 260, apChoiceList->m_item->pActivateChoiceLabel);
        if (apChoiceList->m_item->GetType() == 4 && apChoiceList->m_item->pActivateChoiceLabel) {
            // Tests the address of the item's ActivateChoiceScript, which is never null.
            {
                MessageMenu::PerkActivation* pperk = static_cast<MessageMenu::PerkActivation*>(
                    ourMalloc(sizeof(MessageMenu::PerkActivation))
                );
                pperk->DisplayText = apChoiceList->m_item->pActivateChoiceLabel;
                pperk->pScript = &apChoiceList->m_item->ActivateChoiceScript;
                pperk->pActivatedObject = apActivationTarget;
                MessageMenu::xPerkActivations.Add(pperk);
                if (BSstrlen(acButtonText)) {
                    using strtemp_t = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;
                    strtemp_t strtemp(static_cast<const char*>(acButtonText));
                    uint32_t hotkeyIndex = strtemp.find(38, 0);
                    if (hotkeyIndex != -1) {
                        strHotkey = strtemp.substr(hotkeyIndex + 1, 1u);
                        pnewMessage->strHotkeys += strHotkey.c_str();
                        strtemp.replace(hotkeyIndex, 1u, "");
                    }
                    if (BSstrcmp(strtemp.c_str(), " ")) {
                        pnewMessage->strButtonsText += strtemp.c_str();
                        pnewMessage->strButtonsText += ";";
                    }
                }
            }
        }

        apChoiceList = apChoiceList->m_pkNext;
    }
    int32_t itype = apActivationTarget->GetRefPromptType();
    ENUM_FORM_ID eFormType = apActivationTarget->GetObjectReference()->GetFormType();
    LockData* pExtraLock = apActivationTarget->GetLock();
    TESObjectDOOR* pdoor = static_cast<TESObjectDOOR*>(apActivationTarget->GetObjectReference());
    if (itype == 1 && apActivationTarget->IsCrimeToActivate()) {
        const char* pcLabel = eFormType == NPC__ID ? sPickpocket.String() : sSteal.String();
        BSstrcpy(acButtonText, 260, pcLabel);
    }
    else if (itype == 2 && (eFormType == NPC__ID || eFormType == CREA_ID)) {
        BSstrcpy(acButtonText, 260, sSearch.String());
    }
    else if (itype == 8 && eFormType == DOOR_ID) {
        bool bOpen = BGSOpenCloseForm::GetOpenState(apActivationTarget) == BGSOpenCloseForm::OBJECT_OPEN ||
            BGSOpenCloseForm::GetOpenState(apActivationTarget) == BGSOpenCloseForm::OBJECT_OPENING;
        if (!pExtraLock ||
            (!pExtraLock->IsLocked() ||
             (apActivationTarget->IsAnOwner(pPlayer, true) ||
              (pdoor && TESObjectDOOR::CanActorIgnoreLock(apActivationTarget, pPlayer, false, true))))) {
            const char* pcLabel = bOpen ? sCloseButton.String() : psTargetTypes[itype]->String();
            BSstrcpy(acButtonText, 260, pcLabel);
        }
        else {
            BSstrcpy(acButtonText, 260, sPick.String());
        }
    }
    else if (itype && itype < 20) {
        BSstrcpy(acButtonText, 260, psTargetTypes[itype]->String());
    }
    else if (!itype) {
        BSstrcpy(acButtonText, 260, sCancel.String());
    }
    if (BSstrlen(acButtonText)) {
        using strtemp_t = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;
        strtemp_t strtemp(static_cast<const char*>(acButtonText));
        uint32_t hotkeyIndex = strtemp.find(38, 0);
        if (hotkeyIndex != -1) {
            strTargetHotkey = strtemp.substr(hotkeyIndex + 1, 1u);
            pnewMessage->strHotkeys += strTargetHotkey.c_str();
            strtemp.replace(hotkeyIndex, 1u, "");
        }
        if (BSstrcmp(strtemp.c_str(), " ")) {
            pnewMessage->strButtonsText += strtemp.c_str();
            pnewMessage->strButtonsText += ";";
        }
    }
    MessageMenu::PerkActivation* pperk = static_cast<MessageMenu::PerkActivation*>(
        ourMalloc(sizeof(MessageMenu::PerkActivation))
    );
    pperk->DisplayText = acButtonText;
    pperk->pScript = nullptr;
    pperk->pActivatedObject = apActivationTarget;
    MessageMenu::xPerkActivations.Add(pperk);
    MessageMenu::xEntryList.Add(pnewMessage);
    if (!MessageMenu::pMe && !bMessageMenuDisabled) {
        if (iNumHWThreads.Int() <= 1) {
            MessageMenu::Create();
        }
        else {
            Interface::QueueMenuCreate(
                Interface::OPEN_MESSAGE,
                nullptr,
                nullptr,
                nullptr,
                Interface::CMM_STANDARD,
                false
            );
        }
    }
    return true;
}

void MessageMenu::HandlePerkActivationResults() {
    uint32_t auiIndex = Interface::GetMessageMenuResult();
    if (auiIndex == MessageMenu::xPerkActivations.iSize - 1) {
        int32_t iCount = MessageMenu::xPerkActivations[auiIndex]->pActivatedObject->GetCount();
        MessageMenu::xPerkActivations[auiIndex]->pActivatedObject->Activate(pPlayer, false, nullptr, iCount);
    }
    else {
        MessageMenu::xPerkActivations[auiIndex]
            ->pScript->Run(MessageMenu::xPerkActivations[auiIndex]->pActivatedObject, nullptr, nullptr, false);
    }
    MessageMenu::xPerkActivations.Clear(true);
}
