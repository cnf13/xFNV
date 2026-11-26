#pragma once

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTimer.h"
#include "BSCore/NewOverloads.h"
#include "BSMenu/Menu.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout/Interface/ListBox.h"

#include <cstdarg>
#include <stdint.h>

class BGSEntryPointFunctionDataActivateChoice;
class Script;
class TESObjectREFR;

// Disables (or re-enables) the message menu: closes the open one, else reopens the next queued message. Stores the
// argument's raw byte into bMessageMenuDisabled (a uint8_t), and SetPauseMenuMode passes it a saved copy of that byte
// unnormalised.
void MessageMenu_SetDisabled(uint8_t abDisable);

// The message box menu, fed from a queue of pending messages.
class MessageMenu : public Menu {
public:
    // One queued message: its text, buttons and callback.
    class MessageMenuEntry {
    public:
        MessageMenuEntry();
        ~MessageMenuEntry();

        static void operator delete(void* apMemory) {
            ourFree(apMemory);
        }

        BSStringT<char> strIconFilename;
        char* strTitle;
        BSStringT<char> strMessageText;
        BSStringT<char> strButtonsText;
        char cFirstButtonResult;
        BSStringT<char> strHotkeys;
        void (*fpResultCallback)();
        WARNING_TYPES iWarningContext;
        BSStringT<char> strAudioOverride;
        bool bIsModal;
        int32_t iXPosOverride;
        int32_t iYPosOverride;
        float fMillisecondsVisible;
        float fBackgroundTextureOpacity;
        uint32_t uiWidthOverride;
    };

    using MessageMenuEntryArray = BSSimpleArray<MessageMenuEntry*>;

    // One activation choice a perk adds to the message box: AddMessageToQueue allocates 0xC bytes; the result
    // handler runs pScript on pActivatedObject, or for the last entry activates pActivatedObject.
    struct PerkActivation {
        const char* DisplayText;
        Script* pScript;
        TESObjectREFR* pActivatedObject;
    };

    // The perk activation queue's entries; only the type name is needed here.
    struct PerkActivation;
    using PerkActivationArray = BSSimpleArray<PerkActivation*>;
    using xPerkActivations_t = BSSimpleArray<MessageMenu::PerkActivation*, 1024>;

    MessageMenu();
    ~MessageMenu() override;

    int32_t GetClass() override {
        return 0x3E9;
    }
    void AttachTileByID(int32_t aiID, Tile* apTile) override;
    void DoClick(int32_t aiID, Tile* apTarget) override;
    bool DoKey(int32_t aiKey) override;
    void DoIdle() override;

    // Loads the next queued message into the menu.
    void LoadNextMessage();
    bool IsWellFormedMenu();

    // Hides the open message menu, fades it out and lifts the dialog pause unless the pause menu is open.
    static void Close();
    // Answers the open message box's Yes to All.
    static void ProcessYesToAll();
    // The number of buttons the open message box shows, 0 when none is open.
    static uint32_t GetCurrentButtonCount();
    // The message menu's class id.
    static int32_t GetClassStatic();
    // Queues a message box: its text, icon, title and sound, the result callback, the first button's result and the
    // warning context, then the first button's text and the va_list of the rest (null-terminated), whether the box is
    // modal, the position and size overrides and the timings; true when it was queued.
    static bool AddMessageToQueue(
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
    );
    // The message at the queue's head, xEntryList[iStartIndex], or null when iStartIndex is
    // past the end of the list.
    static MessageMenuEntry* GetQueueHead();
    // Holds or releases the message queue while the pause menu is up.
    static void SetPauseMenuMode(bool abFlag);
    // Shows the message at the queue's head; true when it did.
    static bool Create();
    static void ClearMessageQueueHead();
    static void ClearMessageQueue();
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
    // Queues the perk activation choices for apActivationTarget as one message box.
    static bool AddMessageToQueue(
        BSSimpleList<BGSEntryPointFunctionDataActivateChoice*>* apChoiceList,
        TESObjectREFR* apActivationTarget
    );
    static void HandlePerkActivationResults();

    // The menu's tiles, indexed by the nested TileIndex (TI_MAIN_RECT .. TI_BACKGROUND).
    Tile* pTiles[6];
    ListBox<int32_t> buttonList;
    BSTimer MMTimer;
    uint32_t uiMessageStartTime;

    static inline MessageMenu* pMe;
    static MessageMenuEntryArray xEntryList;
    static inline bool bCheckForDeletion;
    static inline bool bDeletedDuringCheck;
    static inline uint32_t iStartIndex;
    static xPerkActivations_t xPerkActivations;
    static inline bool bDeferredCreation;
};
