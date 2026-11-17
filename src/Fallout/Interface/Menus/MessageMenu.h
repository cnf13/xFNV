#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTimer.h"
#include "BSMenu/Menu.h"
#include "Fallout/Interface/ListBox.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Reconstruction.hpp"

#include <cstdarg>
#include <stdint.h>

class BGSEntryPointFunctionDataActivateChoice;
class Script;
class TESObjectREFR;

// IDA name; not in the PDB. Disables (or re-enables) the message menu: closes the open one, else reopens the next
// queued message (cdecl, 0x7AA6B0). The exe stores the argument's raw byte into bMessageMenuDisabled (a uint8_t,
// 0x11DA4EC), and SetPauseMenuMode (0x7AAF20) passes it a saved copy of that byte unnormalised.
void MessageMenu_SetDisabled(uint8_t abDisable);

// The message box menu, fed from a queue of pending messages.
class MessageMenu : public Menu {
public:
	// One queued message: its text, buttons and callback.
	class MessageMenuEntry {
	public:
		MessageMenuEntry();
		~MessageMenuEntry();

		static void operator delete(void* apMemory) { ourFree(apMemory); }

		BSStringT<char> strIconFilename;
		char* strTitle;
		BSStringT<char> strMessageText;
		BSStringT<char> strButtonsText;
		char cFirstButtonResult;
		uint8_t padding1D[3];
		BSStringT<char> strHotkeys;
		void (*fpResultCallback)();
		WARNING_TYPES iWarningContext;
		BSStringT<char> strAudioOverride;
		bool bIsModal;
		uint8_t padding39[3];
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

	// The perk activation queue's entries (the PDB's MessageMenu::PerkActivation); only the type name is needed here.
	struct PerkActivation;
	using PerkActivationArray = BSSimpleArray<PerkActivation*>;
	// IDA: MessageMenu::kPerkActivations.
	using xPerkActivations_t = BSSimpleArray<MessageMenu::PerkActivation*, 1024>;

	MessageMenu();
	~MessageMenu() override;

	int32_t GetClass() override { return 0x3E9; }
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
	// Queues a message box: its text, icon, title and sound, the result callback, the first
	// button's result and the warning context, then the first button's text and the va_list of the rest (null-
	// terminated), whether the box is modal, the position and size overrides and the timings; true when it was
	// queued (cdecl, 15 arguments, 0x7A8E60).
	static bool AddMessageToQueue(const char* axText, const char* apcIconFilename, const char* apcTitle,
		const char* apcSound, void (*afpResult)(), int32_t aiFirstButtonResult, int32_t aiWarningContext,
		const char* axFirst, va_list* axList, bool abModal, int32_t aiXPosOverride, int32_t aiYPosOverride,
		float afSecondsVisible, float afBackgroundTextureOpacityOverride, float afMenuWidthOverride);
	// The message at the queue's head, xEntryList[iStartIndex], or null when iStartIndex is
	// past the end of the list.
	static MessageMenuEntry* GetQueueHead();
	// IDA leaves it unnamed; the PDB's SetPauseMenuMode by what it does. Holds or releases the message queue while
	// the pause menu is up (cdecl, 0x7AAF20).
	static void SetPauseMenuMode(bool abFlag);
	// Shows the message at the queue's head; true when it did.
	static bool Create();
	static void ClearMessageQueueHead();
	static void ClearMessageQueue();
	static void operator delete(void* apMemory) { ourFree(apMemory); }
	// Queues the perk activation choices for apActivationTarget as one message box.
	static bool AddMessageToQueue(BSSimpleList<BGSEntryPointFunctionDataActivateChoice*>* apChoiceList,
		TESObjectREFR* apActivationTarget);
	static void HandlePerkActivationResults();

	// The menu's tiles, by the PDB's nested TileIndex (TI_MAIN_RECT .. TI_BACKGROUND).
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

inline constexpr fnvd::Vtable MessageMenu_vftable{};
