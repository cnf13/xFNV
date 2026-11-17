#pragma once

#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSStringT.h"
#include "BSMain/Setting.h"
#include "BSMenu/Menu.h"
#include "Reconstruction.hpp"

class Actor;
class CompanionWheelMenu;
class TESTopic;

int32_t return_1075();

// The companion command wheel.
class CompanionWheelMenu : public Menu {
public:
	// The menu's tiles (pTiles). Values from the Xbox PDB.
	enum TILE_INDEX : int32_t {
		TI_AGGRESSIVE_PASSIVE_BUTTON = 0,
		TI_USE_STIM_BUTTON = 1,
		TI_STAY_FOLLOW_BUTTON = 2,
		TI_TALK_BUTTON = 3,
		TI_BACK_UP_BUTTON = 4,
		TI_NEAR_FAR_BUTTON = 5,
		TI_OPEN_INVENTORY_BUTTON = 6,
		TI_RANGED_MELEE_BUTTON = 7,
		TI_RADIAL_LABEL = 8,
		TI_ACTION_LABEL = 9,
		TI_CONTEXT_LABEL = 10,
		TI_EXIT_BUTTON = 11,
		TI_SUBTITLE = 12,
		NUM_TILES = 13,
		// Not in this enum in the PDB; its name in CaravanMenu's TileIndex. No tile: m_CurrentlySelectedTile is
		// tested against -1 (0x7557D0).
		TI_NO_TILE = -1,
	};

	CompanionWheelMenu();
	~CompanionWheelMenu() override;

	// The exe's vtable slot 1 holds the fold 0x754C00 (ComputersMenu::AttachTileByID): pTiles[aiID] for 0..15.
	void AttachTileByID(int32_t aiID, Tile* apTile) override;
	int32_t GetClass() override { return 0x433; }
	void DoClick(int32_t aiID, Tile* apTarget) override;
	void DoEnter(int32_t aiID, Tile* apTarget) override;
	void DoIdle() override;
	bool DoGamepad(int32_t aiButton, float afValue) override;

	bool IsWellFormedMenu();
	// Starts the backup package on the companion, if there is one, and closes the wheel.
	void Do_Backup();
	// Marks the companion to be talked to when the wheel closes and closes it.
	void Do_Talk();
	void RunTopicScript(TESTopic* a2);
	void SetButtonPreviewText(const char* asText);
	// Shows the companion's health and stimpaks, carry weight or best weapon for button aiID.
	void SetButtonContextText(int32_t aiID);
	// Inferred type: a1: TESTopic* from 0x756896 (result of FollowerBarks::GetBarkTopicByType)
	void SpeakTopic(TESTopic* a1);
	void OpenInventory();
	void HandleMouseoverForTile(int32_t aiID, uint8_t a2);
	void SetButtonAsHighlighted(float a1);
	void Do_ToggleNearFar();
	void Do_ToggleAggressivePassive();
	void Do_ToggleRangedMelee();
	void Do_ToggleStayFollow();
	void Do_UseStim();
	void TransitionFromCloseToTalkTo();
	// PDB name (0x757690; the exe reads ahSound's duration through the by-value handle on the
	// stack, BSSoundHandle::GetDuration 0xAD8B30).
	void DisplayCompanionWheelSubtitle(BSStringT<char> asText, BSSoundHandle ahSound);

	// Opens the wheel for apRelatedCompanion; true when it opened.
	static bool CreateCompanionWheelMenu(Actor* apRelatedCompanion);
	// Fades the companion's response out when abClearOutSpeech, closes the wheel's tiles and
	// starts the menu's fade-out.
	static void Close(bool abClearOutSpeech);
	static void operator delete(void* apMemory) { ourFree(apMemory); }
	static Tile* Create(Actor* apRelatedCompanion);

	Tile* pTiles[16];
	// IDA name pCompanionRef.
	Actor* m_pRelatedCompanion;
	bool m_bFollowingLong;
	bool m_bAggressiveStance;
	bool m_bRangedCombat;
	bool m_bFollowing;
	// -1 while no button is selected.
	TILE_INDEX m_CurrentlySelectedTile;
	bool bTalkToOnClose;
	// The tick count at which DoIdle clears the subtitle tile, 0 when none is pending.
	uint32_t m_nTimeToClearSubtitle;
	// IDA name kResponse.
	BSSoundHandle m_hCurrentReactivityBark;

	static inline CompanionWheelMenu* pMe;
};
inline constexpr fnvd::Vtable CompanionWheelMenu_vftable{};

// GameSettingCollection: 0xf75c9d calls 0x40c150.
extern SettingT<GameSettingCollection> gs_sCWheelUseStimpak;
