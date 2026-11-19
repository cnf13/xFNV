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
    if ((aiID == 7) && (apTarget)) {
        char DataFor = buttonList.GetDataFor(apTarget);
        InterfaceManager::Instance()->cLastMessageButtonClicked = DataFor;
        MessageMenu::bCheckForDeletion = true;
        if ((MessageMenu::GetQueueHead()) && (MessageMenu::GetQueueHead()->fpResultCallback)) {
            MessageMenu::MessageMenuEntry* QueueHead = MessageMenu::GetQueueHead();
            QueueHead->fpResultCallback();
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
                Tile* FirstItem = buttonList.GetFirstItem();
                DoClick(-1, FirstItem);
                bhandled = true;
            }
            else {
                MessageMenu::MessageMenuEntry* QueueHead = MessageMenu::GetQueueHead();
                if (QueueHead->strHotkeys.GetLength()) {
                    MessageMenu::MessageMenuEntry* v5 = MessageMenu::GetQueueHead();
                    using strtemp_t = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;
                    strtemp_t strtemp(static_cast<const char*>(v5->strHotkeys.pString));
                    for (uint32_t hotkeyIndex = 0; hotkeyIndex < strtemp.size(); ++hotkeyIndex) {
                        char* v6 = &strtemp[hotkeyIndex];
                        int32_t v7 = std::toupper(*v6);
                        if (v7 == std::toupper(aiKey)) {
                            Tile* v8 = pbuttons->GetTileAt(hotkeyIndex, false);
                            DoClick(7, v8);
                            bhandled = true;
                            break;
                        }
                    }
                }
            }
        }
        else {
            Tile* v2 = pbuttons->GetTileAt(buttonList.NumItems() - 1, false);
            DoClick(7, v2);
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
            MessageMenu::MessageMenuEntry* QueueHead = MessageMenu::GetQueueHead();
            if (!QueueHead->bIsModal && pTiles[0]->IsTrue(4003)) {
                if (xFadeState == Interface::FADE_HIDDEN) {
                    if (uiMessageStartTime) {
                        MessageMenu::ClearMessageQueueHead();
                        uiMessageStartTime = 0;
                    }
                    else {
                        StartFadeIn();
                    }
                }
                else {
                    if (xFadeState == Interface::FADE_SHOWN) {
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
}

void MessageMenu::LoadNextMessage() {
    std::basic_string<char, std::char_traits<char>, std::allocator<char>> _Result;
    std::basic_string<char, std::char_traits<char>, std::allocator<char>> apMemory;
    char v66 = 0;
    if (MessageMenu::GetQueueHead()) {
        MessageMenu::MessageMenuEntry* pcurrMessage = MessageMenu::GetQueueHead();
        float fminHeaderWidth = 0.0f;
        float ftextWidth = 0.0f;
        float flistWidth = 0.0f;
        float fmenuWidth = 0.0f;
        int32_t v1 = Tile::TextToTrait("_horbuf");
        float fhorizSpacing = pMenu->GetFloat(v1);
        bool v63 = false;
        if (pcurrMessage->strAudioOverride.GetLength()) {
            char* v26 = pcurrMessage->strAudioOverride.pString;
            BSAudio* v2 = &BSAudio::QInstance();
            BSSoundHandle arHandle = v2->GetSoundHandleByName(v26, 289u);
            v66 = 1;
            if (arHandle != 0xFFFFFFFFu) {
                v63 = true;
            }
        }
        if ((v66 & 1) != 0) {}
        if (v63) {
            char* v27 = pcurrMessage->strAudioOverride.pString;
            BSAudio* v3 = &BSAudio::QInstance();
            v3->GetSoundHandleByName(v27, 289u).Play(false);
        }
        else {
            BSAudio* v4 = &BSAudio::QInstance();
            v4->GetSoundHandleByName("UIPopUpMessageWindow", 289u).Play(false);
        }
        char* a1 = pcurrMessage->strMessageText.pString;
        MessageMenu::pMe->pTiles[3]->Set(4036, a1, true);
        if (pcurrMessage->strTitle) {
            float v58;
            float fWrapWidth_4;
            float fWrapWidth_3;
            MessageMenu::pMe->pTiles[1]->Set(4036, pcurrMessage->strTitle, true);
            MessageMenu::pMe->pTiles[1]->Set(4003, 1);
            if (pcurrMessage->uiWidthOverride) {
                fWrapWidth_3 = static_cast<float>(pcurrMessage->uiWidthOverride);
            }
            else {
                fWrapWidth_3 = MessageMenu::pMe->pTiles[1]->GetFloat(4026);
            }
            if (fWrapWidth_3 == 0.0) {
                fWrapWidth_4 = 3.4028235e+38f;
            }
            else {
                fWrapWidth_4 = fWrapWidth_3;
            }
            char* strTitle = pcurrMessage->strTitle;
            FontManager* Singleton = FontManager::Instance();
            fminHeaderWidth = Singleton->CalculateStringDimensions(strTitle, 6, fWrapWidth_4, 0).x;
            if (pcurrMessage->uiWidthOverride) {
                v58 = 0.0f;
            }
            else {
                v58 = 100.0f;
            }
            fminHeaderWidth = fminHeaderWidth + v58;
        }
        else {
            MessageMenu::pMe->pTiles[1]->Set(4003, 0);
        }
        uint32_t currStartIndex = 0;
        int32_t imaxButtonWidth = 0;
        float ftotalButtonHeight = 0.0f;
        buttonList.RemoveAll();
        if (pcurrMessage->strButtonsText.pString) {
            using apMemory_28_t = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;
            apMemory_28_t apMemory_28(static_cast<const char*>(pcurrMessage->strButtonsText.pString));
            for (int32_t ibuttonCt = 0;; ++ibuttonCt) {
                uint32_t Time = apMemory_28.size();
                if (currStartIndex >= Time - 1) {
                    break;
                }
                size_t v7 = apMemory_28.find(";", currStartIndex);
                apMemory = apMemory_28.substr(currStartIndex, v7 - currStartIndex);
                if (apMemory.size()) {
                    size_t v8 = apMemory_28.find(";", currStartIndex);
                    _Result = apMemory_28.substr(currStartIndex, v8 - currStartIndex);
                    std::string* v57 = &_Result;
                    const char* v9 = v57->c_str();
                    Tile* ptemp = MessageMenu::pMe->buttonList.AddItem(
                        ibuttonCt + pcurrMessage->cFirstButtonResult, v9, nullptr, nullptr
                    );
                    Tile* ptextChildTile = ptemp->GetChildByName("ListItemText");
                    ptemp->Set(4010, 7);
                    if (ptextChildTile) {
                        float fWrapWidth_6;
                        float fWrapWidth_5;
                        if (pcurrMessage->uiWidthOverride) {
                            fWrapWidth_5 = static_cast<float>(pcurrMessage->uiWidthOverride);
                        }
                        else {
                            fWrapWidth_5 = ptextChildTile->GetFloat(4026);
                        }
                        float fWrapWidth_2 = fWrapWidth_5;
                        if (fWrapWidth_5 == 0.0) {
                            fWrapWidth_6 = 3.4028235e+38f;
                        }
                        else {
                            fWrapWidth_6 = fWrapWidth_2;
                        }
                        fWrapWidth_2 = fWrapWidth_6;
                        const char* String = ptextChildTile->GetString(4036);
                        FontManager* v10 = FontManager::Instance();
                        double x = v10->CalculateStringDimensions(String, 2, fWrapWidth_6, 0).x;
                        imaxButtonWidth = BSMAX(imaxButtonWidth, static_cast<int32_t>(x));
                    }
                    double Float = ptemp->GetFloat(4016);
                    ftotalButtonHeight = Float + ftotalButtonHeight;
                }
                currStartIndex = apMemory_28.find(";", currStartIndex);
                uint32_t v13 = apMemory_28.size();
                if (currStartIndex < v13 - 1) {
                    ++currStartIndex;
                }
            }
        }
        if (pcurrMessage->strIconFilename.GetLength()) {
            char* a1a = pcurrMessage->strIconFilename.pString;
            MessageMenu::pMe->pTiles[2]->Set(4044, a1a, true);
            MessageMenu::pMe->pTiles[2]->Set(4003, 1);
            if (pcurrMessage->uiWidthOverride) {
                float a2 = static_cast<float>(pcurrMessage->uiWidthOverride);
                float v54 = ftextWidth + 120.0;
                ftextWidth = BSMIN(v54, a2);
            }
            else {
                ftextWidth = ftextWidth + 120.0;
            }
        }
        else {
            MessageMenu::pMe->pTiles[2]->Set(4003, 0);
        }
        if (pcurrMessage->uiWidthOverride) {
            float a2a = static_cast<float>(pcurrMessage->uiWidthOverride);
            float v53 = static_cast<double>(imaxButtonWidth) + 80.0;
            flistWidth = BSMIN(v53, a2a);
        }
        else {
            flistWidth = static_cast<double>(imaxButtonWidth) + 80.0;
        }
        MessageMenu::pMe->pTiles[4]->Set(4016, ftotalButtonHeight, false);
        MessageMenu::pMe->pTiles[4]->Set(4017, flistWidth, false);
        MessageMenu::pMe->buttonList.Synchronize();
        if (pcurrMessage->uiWidthOverride) {
            float a2b = static_cast<float>(pcurrMessage->uiWidthOverride);
            float v52 = fhorizSpacing + fhorizSpacing + flistWidth;
            flistWidth = BSMIN(v52, a2b);
        }
        else {
            flistWidth = fhorizSpacing + fhorizSpacing + flistWidth;
        }
        float fWrapWidth_7 = 3.4028235e+38f;
        if (pcurrMessage->uiWidthOverride) {
            double ScreenWidth = InterfaceManager::GetScreenWidth();
            double uiWidthOverride = static_cast<double>(pcurrMessage->uiWidthOverride);
            fWrapWidth_7 = uiWidthOverride / InterfaceManager::GetRealScreenWidth() * ScreenWidth;
        }
        else {
            int32_t v14 = Tile::TextToTrait("_MaxMenuWidth");
            fWrapWidth_7 = MessageMenu::pMe->pMenu->GetFloat(v14);
        }
        fWrapWidth_7 = fWrapWidth_7 - (fhorizSpacing + fhorizSpacing);
        if (MessageMenu::pMe->pTiles[2]->IsTrue(4003)) {
            fWrapWidth_7 = fWrapWidth_7 - 120.0;
        }
        MessageMenu::pMe->pTiles[3]->Set(4026, fWrapWidth_7, true);
        float a1b = MessageMenu::pMe->pTiles[3]->GetFloat(4026);
        char* v25 = pcurrMessage->strMessageText.pString;
        FontManager* v15 = FontManager::Instance();
        NiPoint3 textDims = v15->CalculateStringDimensions(v25, 2, a1b, 0);
        ftextWidth = fhorizSpacing + fhorizSpacing + textDims.x + ftextWidth;
        if (!(textDims.z > 1.0)) {
            MessageMenu::pMe->pTiles[1]->Set(4023, 2);
            MessageMenu::pMe->pTiles[3]->Set(4023, 2);
        }
        else {
            MessageMenu::pMe->pTiles[1]->Set(4023, 1);
            MessageMenu::pMe->pTiles[3]->Set(4023, 1);
        }
        if (pcurrMessage->uiWidthOverride) {
            double v49 = InterfaceManager::GetScreenWidth();
            double v48 = static_cast<double>(pcurrMessage->uiWidthOverride);
            fmenuWidth = v48 / InterfaceManager::GetRealScreenWidth() * v49;
            double v47 = InterfaceManager::GetScreenWidth();
            double ScreenSafeX = InterfaceManager::GetScreenSafeX();
            float v46 = v47 - (ScreenSafeX + ScreenSafeX);
            fmenuWidth = BSMIN(fmenuWidth, v46);
        }
        else {
            fmenuWidth = BSMAX(fminHeaderWidth, ftextWidth);
            fmenuWidth = BSMAX(fmenuWidth, flistWidth);
            int32_t v16 = Tile::TextToTrait("_MaxMenuWidth");
            float a2c = pMenu->GetFloat(v16);
            fmenuWidth = BSMIN(fmenuWidth, a2c);
            int32_t v17 = Tile::TextToTrait("_MinMenuWidth");
            float a2d = pMenu->GetFloat(v17);
            fmenuWidth = BSMAX(fmenuWidth, a2d);
        }
        MessageMenu::pMe->pTiles[0]->Set(4017, fmenuWidth, true);
        Interface::bAllowPauseMenu = pcurrMessage->iWarningContext == WARN_INGAME_MESSAGE;
        pMenu->Set(Tile::kTileValue_DeleteOnFade, 0);
        pMenu->Set(4003, 1);
        if (pcurrMessage->bIsModal) {
            MessageMenu::pMe->pTiles[0]->Set(4009, 1.0f, true);
            uint32_t a2h = GetClass();
            InterfaceManager* v21 = InterfaceManager::Instance();
            if (!v21->IsMenuInStack(a2h)) {
                int32_t v22 = GetClass();
                Interface::AddToEnterStack(v22, false);
            }
        }
        else {
            MessageMenu::pMe->pTiles[0]->Set(4009, 0.0f, true);
            uint32_t a2e = GetClass();
            InterfaceManager* v19 = InterfaceManager::Instance();
            if (v19->IsMenuInStack(a2e)) {
                int32_t v20 = GetClass();
                Interface::PopFromEnterStack(v20, true);
            }
            float fScreenWidth = InterfaceManager::GetScreenWidth();
            float fMainWidth = MessageMenu::pMe->pTiles[0]->GetFloat(4017);
            float fScreenHeight = InterfaceManager::GetScreenHeight();
            float fMainHeight = MessageMenu::pMe->pTiles[0]->GetFloat(4016);
            float fMenuX = (fScreenWidth - fMainWidth) / 2.0;
            float fMenuY = (fScreenHeight - fMainHeight) / 2.0;
            if (pcurrMessage->iXPosOverride > -1) {
                double iXPosOverride = static_cast<double>(pcurrMessage->iXPosOverride);
                float fConvertedXPosOverride = iXPosOverride / InterfaceManager::GetRealScreenWidth() * fScreenWidth;
                float a2f = InterfaceManager::GetScreenSafeX();
                fMenuX = BSMAX(fConvertedXPosOverride, a2f);
                double v44 = fScreenWidth - fMainWidth;
                float v43 = v44 - InterfaceManager::GetScreenSafeX();
                fMenuX = BSMIN(fMenuX, v43);
            }
            if (pcurrMessage->iYPosOverride > -1) {
                double iYPosOverride = static_cast<double>(pcurrMessage->iYPosOverride);
                float fConvertedYPosOverride = iYPosOverride / InterfaceManager::GetRealScreenHeight() * fScreenHeight;
                float a2g = InterfaceManager::GetScreenSafeY();
                fMenuY = BSMAX(fConvertedYPosOverride, a2g);
                double v41 = fScreenHeight - fMainHeight;
                float v40 = v41 - InterfaceManager::GetScreenSafeY();
                fMenuY = BSMIN(fMenuY, v40);
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
                MessageMenu::MessageMenuEntry* v0 = MessageMenu::xEntryList[imessageCt];
                if (v0) {
                    delete v0;
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
    int32_t ititleLen;
    MessageMenu::MessageMenuEntry* pnewMessage;
    std::basic_string<char, std::char_traits<char>, std::allocator<char>> result_1;
    void* apMemory = ourMalloc(sizeof(MessageMenu::MessageMenuEntry));
    if (apMemory) {
        pnewMessage = ::new (apMemory) MessageMenu::MessageMenuEntry;
    }
    else {
        pnewMessage = nullptr;
    }
    MessageMenu::MessageMenuEntry* pnewMessage_2 = pnewMessage;
    BSStringT<char> strbuttonName(axFirst);
    pnewMessage_2->fpResultCallback = afpResult;
    pnewMessage_2->cFirstButtonResult = aiFirstButtonResult;
    pnewMessage_2->iWarningContext = static_cast<WARNING_TYPES>(aiWarningContext);
    pnewMessage_2->strIconFilename = apcIconFilename;
    pnewMessage_2->strHotkeys = "";
    pnewMessage_2->strMessageText = axText;
    pnewMessage_2->strAudioOverride = apcSound;
    pnewMessage_2->bIsModal = abModal;
    pnewMessage_2->iXPosOverride = aiXPosOverride;
    pnewMessage_2->iYPosOverride = aiYPosOverride;
    pnewMessage_2->fMillisecondsVisible = afSecondsVisible * 1000.0;
    if (afBackgroundTextureOpacityOverride > 0.0) {
        pnewMessage_2->fBackgroundTextureOpacity = afBackgroundTextureOpacityOverride * 255.0;
    }
    pnewMessage_2->uiWidthOverride = static_cast<int64_t>(afMenuWidthOverride);
    if (apcTitle) {
        ititleLen = BSstrlen(apcTitle);
    }
    else {
        ititleLen = 0;
    }
    if (ititleLen <= 0) {
        pnewMessage_2->strTitle = nullptr;
    }
    else {
        char* v26 = static_cast<char*>(ourMalloc(ititleLen + 1));
        pnewMessage_2->strTitle = v26;
        BSstrcpy(pnewMessage_2->strTitle, ititleLen + 1, apcTitle);
    }
    while (strbuttonName.GetLength()) {
        using strtemp_t = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;
        strtemp_t strtemp(static_cast<const char*>(strbuttonName.pString));
        uint32_t hotkeyIndex = strtemp.find(38, 0);
        if (hotkeyIndex != -1) {
            result_1 = strtemp.substr(hotkeyIndex + 1, 1u);
            std::string* v22 = &result_1;
            const char* v15 = v22->c_str();
            pnewMessage_2->strHotkeys += v15;
            strtemp.replace(hotkeyIndex, 1u, "");
        }
        const char* v16 = strtemp.c_str();
        if (BSstrcmp(v16, " ")) {
            const char* v17 = strtemp.c_str();
            pnewMessage_2->strButtonsText += v17;
        }
        pnewMessage_2->strButtonsText += ";";
        strbuttonName = va_arg(*axList, const char*);
    }
    if (MessageMenu::xEntryList.iSize) {
        for (int32_t i = 0;; ++i) {
            uint32_t FormFlags = MessageMenu::xEntryList.iSize;
            if (i >= FormFlags) {
                break;
            }
            if (!MessageMenu::xEntryList.GetAt(i)->bIsModal && pnewMessage_2->bIsModal) {
                MessageMenu::xEntryList.Insert(static_cast<uint32_t>(i), pnewMessage_2);
                break;
            }
            MessageMenu::MessageMenuEntry** v19 = &MessageMenu::xEntryList.GetAt(i);
            if (*v19 == *static_cast<MessageMenu::MessageMenuEntry**>(MessageMenu::xEntryList.GetTail())) {
                goto LABEL_27;
            }
        }
    }
    else {
    LABEL_27:;
        MessageMenu::xEntryList.Add(pnewMessage_2);
    }
    if (MessageMenu::pMe || (bMessageMenuDisabled || (Interface::InDialog() && !abModal))) {
        bDeferredCreation = true;
    }
    else {
        if (iNumHWThreads.Int() <= 1) {
            MessageMenu::Create();
        }
        else {
            bool IsTrait = pnewMessage_2->bIsModal;
            Interface::QueueMenuCreate(
                Interface::OPEN_MESSAGE, nullptr, nullptr, nullptr, Interface::CMM_STANDARD, !IsTrait
            );
        }
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
    int32_t v1;
    InterfaceManager* pim = InterfaceManager::Instance();
    bool boverMenu = false;
    int32_t ClassStatic = MessageMenu::GetClassStatic();
    Tile* pold_menu_tile = Menu::GetMenuByClass(ClassStatic);
    if (pold_menu_tile) {
        delete pold_menu_tile;
    }
    if (Interface::GetTopMenuID()) {
        boverMenu = true;
    }
    Tile* ptile = pim->GetMenusRoot()->ReadFile("Data\\Menus\\message_menu.xml");
    MessageMenu::pMe = static_cast<MessageMenu*>(ptile->GetMenu());
    if (MessageMenu::pMe && (v1 = MessageMenu::pMe->GetClass(), v1 == MessageMenu::GetClassStatic())) {
        MessageMenu::pMe->SetMenuTile(static_cast<TileMenu*>(ptile), false);
        if (MessageMenu::pMe->IsWellFormedMenu()) {
            if (ptile->GetFloat(4007) == 6006.0 || ptile->GetFloat(4007) == 102.0) {
                float afValue = Tile::GetMaximumDepth();
                ptile->Set(4013, afValue, true);
            }
            MessageMenu::pMe->buttonList.Initialize(MessageMenu::pMe->pTiles[4], "MM_ButtonTemplate");
            if (MessageMenu::GetQueueHead()) {
                MessageMenu::pMe->LoadNextMessage();
            }
            if (boverMenu) {
                float v5 = fPopUpBackgroundOpacity.Float() * 255.0;
                Tile* ChildByName = MessageMenu::pMe->pMenu->GetChildByName("MM_Background");
                ChildByName->Set(4009, v5, true);
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

    if ((MessageMenu::pMe) && (MessageMenu::pMe->pMenu) && (MessageMenu::pMe)) {
        delete MessageMenu::pMe;
    }

    return false;
}

void MessageMenu::ClearMessageQueueHead() {
    if (MessageMenu::GetQueueHead()) {
        MessageMenu::MessageMenuEntry* v0 = MessageMenu::xEntryList[MessageMenu::iStartIndex];
        if (v0) {
            delete v0;
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
    MessageMenu::MessageMenuEntry* pnewMessage;
    std::basic_string<char, std::char_traits<char>, std::allocator<char>> v23;
    std::basic_string<char, std::char_traits<char>, std::allocator<char>> _Result;
    char Dst[268];
    void* apMemory = ourMalloc(sizeof(MessageMenu::MessageMenuEntry));
    if (apMemory) {
        pnewMessage = ::new (apMemory) MessageMenu::MessageMenuEntry;
    }
    else {
        pnewMessage = nullptr;
    }
    MessageMenu::MessageMenuEntry* pnewMessage_2 = pnewMessage;
    pnewMessage->fpResultCallback = &MessageMenu::HandlePerkActivationResults;
    pnewMessage_2->cFirstButtonResult = 0;
    pnewMessage_2->iWarningContext = WARN_DEFAULT;
    pnewMessage_2->strIconFilename = nullptr;
    pnewMessage_2->strHotkeys = "";
    const char* v2 = sActivationChoiceMessage.String();
    pnewMessage_2->strMessageText = v2;
    while (apChoiceList && !apChoiceList->IsEmpty()) {
        BSstrcpy(Dst, 260, apChoiceList->m_item->pActivateChoiceLabel);
        BGSEntryPointFunctionDataActivateChoice* m_item = apChoiceList->m_item;
        if ((m_item->GetType() == 4) && (apChoiceList->m_item->pActivateChoiceLabel)) {
            // Tests the address of the item's ActivateChoiceScript, which is never null, as the game does.
            {
                MessageMenu::PerkActivation* pperk =
                    static_cast<MessageMenu::PerkActivation*>(ourMalloc(sizeof(MessageMenu::PerkActivation)));
                MessageMenu::PerkActivation* pperk_2 = pperk;
                char* FormHealth_0 = apChoiceList->m_item->pActivateChoiceLabel;
                pperk_2->DisplayText = FormHealth_0;
                Script* Script_ = &apChoiceList->m_item->ActivateChoiceScript;
                pperk_2->pScript = Script_;
                pperk_2->pActivatedObject = apActivationTarget;
                MessageMenu::xPerkActivations.Add(pperk_2);
                if (BSstrlen(Dst)) {
                    using strtemp_t = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;
                    strtemp_t strtemp(static_cast<const char*>(Dst));
                    uint32_t hotkeyIndex = strtemp.find(38, 0);
                    if (hotkeyIndex != -1) {
                        _Result = strtemp.substr(hotkeyIndex + 1, 1u);
                        std::string* v20 = &_Result;
                        const char* v5 = v20->c_str();
                        pnewMessage_2->strHotkeys += v5;
                        strtemp.replace(hotkeyIndex, 1u, "");
                    }
                    const char* v6 = strtemp.c_str();
                    if (BSstrcmp(v6, " ")) {
                        const char* v7 = strtemp.c_str();
                        pnewMessage_2->strButtonsText += v7;
                        pnewMessage_2->strButtonsText += ";";
                    }
                }
            }
        }

        apChoiceList = apChoiceList->m_pkNext;
    }
    int32_t itype = apActivationTarget->GetRefPromptType();
    ENUM_FORM_ID eFormType = apActivationTarget->GetObjectReference()->GetFormType();
    LockData* pExtraLock = apActivationTarget->GetLock();
    TESObjectDOOR* pdoor_2 = static_cast<TESObjectDOOR*>(apActivationTarget->GetObjectReference());
    if (itype == 1 && apActivationTarget->IsCrimeToActivate()) {
        const char* Src;
        if (eFormType == NPC__ID) {
            Src = sPickpocket.String();
        }
        else {
            Src = sSteal.String();
        }
        BSstrcpy(Dst, 260, Src);
    }
    else {
        if (itype == 2 && (eFormType == NPC__ID || eFormType == CREA_ID)) {
            const char* v8 = sSearch.String();
            BSstrcpy(Dst, 260, v8);
        }
        else {
            if (itype == 8 && eFormType == DOOR_ID) {
                bool v18 = BGSOpenCloseForm::GetOpenState(apActivationTarget) == BGSOpenCloseForm::OBJECT_OPEN ||
                    BGSOpenCloseForm::GetOpenState(apActivationTarget) == BGSOpenCloseForm::OBJECT_OPENING;
                if (!pExtraLock ||
                    (!pExtraLock->IsLocked() ||
                     (apActivationTarget->IsAnOwner(pPlayer, true) ||
                      (pdoor_2 && TESObjectDOOR::CanActorIgnoreLock(apActivationTarget, pPlayer, false, true))))) {
                    const char* apSource;
                    if (v18) {
                        apSource = sCloseButton.String();
                    }
                    else {
                        apSource = psTargetTypes[itype]->String();
                    }
                    BSstrcpy(Dst, 260, apSource);
                }
                else {
                    const char* v9 = sPick.String();
                    BSstrcpy(Dst, 260, v9);
                }
            }
            else {
                if (itype && itype < 20) {
                    const char* v10 = psTargetTypes[itype]->String();
                    BSstrcpy(Dst, 260, v10);
                }
                else {
                    if (!itype) {
                        const char* v11 = sCancel.String();
                        BSstrcpy(Dst, 260, v11);
                    }
                }
            }
        }
    }
    if (BSstrlen(Dst)) {
        using strtemp_2_t = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;
        strtemp_2_t strtemp_2(static_cast<const char*>(Dst));
        uint32_t hotkeyIndex_2 = strtemp_2.find(38, 0);
        if (hotkeyIndex_2 != -1) {
            v23 = strtemp_2.substr(hotkeyIndex_2 + 1, 1u);
            std::string* v16 = &v23;
            const char* v12 = v16->c_str();
            pnewMessage_2->strHotkeys += v12;
            strtemp_2.replace(hotkeyIndex_2, 1u, "");
        }
        const char* v13 = strtemp_2.c_str();
        if (BSstrcmp(v13, " ")) {
            const char* v14 = strtemp_2.c_str();
            pnewMessage_2->strButtonsText += v14;
            pnewMessage_2->strButtonsText += ";";
        }
    }
    MessageMenu::PerkActivation* pperk_3 =
        static_cast<MessageMenu::PerkActivation*>(ourMalloc(sizeof(MessageMenu::PerkActivation)));
    pperk_3->DisplayText = Dst;
    pperk_3->pScript = nullptr;
    pperk_3->pActivatedObject = apActivationTarget;
    MessageMenu::xPerkActivations.Add(pperk_3);
    MessageMenu::xEntryList.Add(pnewMessage_2);
    if (!MessageMenu::pMe && !bMessageMenuDisabled) {
        if (iNumHWThreads.Int() <= 1) {
            MessageMenu::Create();
        }
        else {
            Interface::QueueMenuCreate(
                Interface::OPEN_MESSAGE, nullptr, nullptr, nullptr, Interface::CMM_STANDARD, false
            );
        }
    }
    return true;
}

void MessageMenu::HandlePerkActivationResults() {
    uint32_t auiIndex = Interface::GetMessageMenuResult();
    if (auiIndex == MessageMenu::xPerkActivations.iSize - 1) {
        int32_t a5 = MessageMenu::xPerkActivations[auiIndex]->pActivatedObject->GetCount();
        MessageMenu::xPerkActivations[auiIndex]->pActivatedObject->Activate(pPlayer, false, nullptr, a5);
    }
    else {
        MessageMenu::xPerkActivations[auiIndex]->pScript->Run(
            MessageMenu::xPerkActivations[auiIndex]->pActivatedObject, nullptr, nullptr, false
        );
    }
    MessageMenu::xPerkActivations.Clear(true);
}

void MessageHandler::SetYesToAll(bool abTF) {
    bYesToAll = abTF;
}
