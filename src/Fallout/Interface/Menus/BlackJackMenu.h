#pragma once

#include "BSAudio/BSSoundHandle.h"
#include "BSMenu/Menu.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

struct CasinoData;
class NiControllerManager;
class TESCasino;
class TESObjectREFR;

class BlackJackMenu : public Menu {
public:
	// A card's rank, BL_2 to BL_A; BL_CV_EMPTY marks a card with no value.
	enum CardValue : int32_t {
		BL_2 = 0,
		BL_3 = 1,
		BL_4 = 2,
		BL_5 = 3,
		BL_6 = 4,
		BL_7 = 5,
		BL_8 = 6,
		BL_9 = 7,
		BL_10 = 8,
		BL_J = 9,
		BL_Q = 10,
		BL_K = 11,
		BL_A = 12,
		NUM_CVALUES = 13,
		BL_CV_EMPTY = 14,
	};

	// One playing card: its value, suit and the textures of its two faces (IDA's names; the PDB has none). The
	// textures are the source textures TES::CreateTextureImage creates for them.
	struct BlackJackCard {
		BlackJackCard();

		// The PDB's BlackJackMenu::CardData::operator=.
		BlackJackCard& operator=(const BlackJackCard& aOther);

		// The PDB's eCardValue.
		CardValue uiValue;
		// enum BlackJackMenu::Suits spelled as uint32_t (src has no such enum).
		uint32_t eSuit;
		NiPointer<NiSourceTexture> spFrontTexture;
		NiPointer<NiSourceTexture> spBackTexture;
	};

	// The iFlags bits. CanDoubleDown sets BLF_DOUBLEDOWN_SPLIT (0x200) for the split hand where DoClick
	// reads and clears BLF_CANDOUBLEDOWN_SPLIT (0x100) beside BLF_CANDOUBLEDOWN_MAIN.
	enum enumFlags : uint32_t {
		BLF_DISABLESURRENDER = 0x1,
		BLF_SURRENDER = 0x2,
		BLF_CANSPLIT = 0x4,
		BLF_SPLIT = 0x8,
		BLF_SPLITHANDACTIVE = 0x10,
		BLF_BLACKJACK = 0x20,
		BLF_CANDOUBLEDOWN_MAIN = 0x40,
		BLF_DOUBLEDOWN_MAIN = 0x80,
		BLF_CANDOUBLEDOWN_SPLIT = 0x100,
		BLF_DOUBLEDOWN_SPLIT = 0x200,
		BLF_DISABLEHAND_MAIN = 0x400,
		BLF_DISABLEHAND_SPLIT = 0x800,
		BLF_DEALERS_TURN = 0x1000,
		BLF_PLAYER_BUST = 0x2000,
		BLF_PLAYER_SPLIT_ANIM = 0x4000,
	};

	// The game's state (the animation playing or the turn being processed).
	enum BlackJackState : uint32_t {
		BL_INTRO_ANIM = 0,
		BL_DEALING_ANIM = 1,
		BL_HIT_ANIM = 2,
		BL_SPLIT_ANIM = 3,
		BL_PLAYER_TURN = 4,
		BL_PLAYER_SPLIT = 5,
		BL_PLAYER_DOUBLE = 6,
		BL_CLEAN_TABLE_ANIM = 7,
		BL_DEALER_FLIP_ANIM = 8,
		BL_DEALER_HIT_ANIM = 9,
		BL_IDLE = 10,
		BL_DEALER_TURN = 11,
		BL_DETERMINE_WINNER = 12,
		BL_CLOSING_MENU = 13,
		NUM_STATES = 14,
	};

	BlackJackMenu();
	~BlackJackMenu() override;

	void AttachTileByID(int32_t, Tile*) override;
	void DoClick(int32_t, Tile*) override;
	// The PC inherits DoEnter and DoLeave, unlike the Xbox PDB.
	void DoIdle() override;
	int32_t GetClass() override;

	// Whether flag aiFlag is set.
	bool IsFlagSet(int32_t aiFlag);
	// The points of a card value: the pip cards their number, the face cards 10 and the ace 11. Private in the PDB.
	int32_t GetNumericCardValue(CardValue cv);
	// Reads the casino's blackjack settings, loads the table's and the chips' models and builds the card textures
	// into pDeckOfCards (thiscall, 0x731D70; the PDB types it void, the PC returns a bool in al).
	bool InitCasinoData();
	// Sets or clears aeFlag in iFlags.
	void SetFlag(enumFlags aeFlag, bool abValue);
	// Whether the chips cover doubling the hand's bet (the main hand's or, with isSplitHand, the split hand's) on
	// top of the other bet; sets the matching can-double-down flag.
	bool CanDoubleDown(bool isSplitHand);
	// Clears every iFlags bit.
	void ZeroFlags();
	// Lowers the split hand's geometry 5 units out of view, or puts it back with abVisible.
	void SetSideHandVisibility(bool bVisible);
	// Refills the shoe: 52 cards per deck remain.
	void Shuffle();
	// Hides both arrows over the player's hand.
	void ClearDeckHighlight();
	char HasTiles();
	// The PDB's pHand: one of the hands' card lists (DoClick passes pDealerHand, pPlayerHand
	// and pPlayerSplitHand).
	int32_t GetHandValue(BSSimpleList<BlackJackCard*>* pHand);
	// The PDB's pHand: one of the hands' card lists (as GetHandValue).
	uint32_t DealCard(BSSimpleList<BlackJackCard*>* pHand, char a2, char a3);
	int32_t DealHand();
	void SetTileCasinoEarnings();
	void LoadNifs();
	void UpdateHandCardVisibility();
	void ShowAllHandCards();
	void* SetTileAlphasForStage(int32_t a1);
	char CanSplitHand();
	void HighlightSelectedDeck();
	void SetCorrectChipMeshesVisibleForCurrentBet();
	NiCamera* SetupCamera();
	void SwapCardTextures(int32_t aiHandIndex, int32_t aiCardIndex, BlackJackMenu::BlackJackCard* cd);

	// Draws the blackjack table scene.
	static void DrawScene();
	// Sets the alpha of every child of the aiIndex-th tile of BlackJackMenu::pMe (stdcall, ret 8); the exe returns
	// the last list position (IDA: NiTListItem_void*).
	static void* SetNthTileAlpha(int32_t aiIndex, float afAlpha);
	static void SetShouldExit();
	static void SetDealNotSwitchHands(char a1);
	static void operator delete(void* apMemory) { ourFree(apMemory); }
	static Tile* Create(TESCasino* apCasino, int32_t aiMinBet, int32_t aiMaxBet, int32_t aiMinWinnings,
		bool abDebugMode);
	static int32_t GetClassStatic();

	BlackJackState eBlackJackState;
	uint32_t eHandResults[2];
	Tile* pTiles[10];
	// The blackjack table's reference (the PDB's BlackJackMenu::pBlackjackRef, its pointee from the PDB).
	TESObjectREFR* pBlackjackRef;
	float fBlackJackWinRatio;
	float fBlackJackShuffle;
	int32_t iLuckSkill;
	int32_t iCurrentBet;
	int32_t iCurrentSplitBet;
	int32_t iChipCount;
	bool bGoodLuck;
	bool bBadLuck;
	uint8_t padding7A[2];
	uint32_t uiPrevIdleTick;
	bool bRunScriptOnExit;
	uint8_t padding81[3];
	int32_t iAnimIndex[3];
	// The shoe: up to four decks of 52 cards, one flat array as in the PDB (IDA kCards; the constructor 0x7315E0
	// builds it with a vector constructor iterator, the exe indexes it by card number times 16).
	BlackJackCard pDeckOfCards[208];
	// The hands' cards (the PDB's BSSimpleList<BlackJackMenu::CardData*>; its CardData is BlackJackCard).
	BSSimpleList<BlackJackCard*>* pDealerHand;
	BSSimpleList<BlackJackCard*>* pPlayerHand;
	BSSimpleList<BlackJackCard*>* pPlayerSplitHand;
	int32_t iNumOfDecks;
	int32_t iCardsRemaining;
	uint8_t bCleanDealerHand;
	// The PDB's bSkipDeal, then the two bytes the PDB has as padding.
	bool bSkipDeal;
	uint8_t paddingDA6[2];
	float fStartKeyTime;
	bool bHoldOnSoft17;
	uint8_t paddingDAD[3];
	// The PC has no iDebugAxis word here; subsequent members are four bytes earlier than the Xbox PDB.
	uint32_t iFlags;
	// The models, as the model loader returns them; the exe only attaches, moves, animates and searches them.
	NiAVObject* pTableGeometry;
	NiAVObject* pHandGeometry;
	NiAVObject* pHandSplitGeometry;
	NiAVObject* pHandDealerGeometry;
	NiAVObject* pBetChipGeometry[6];
	NiAVObject* pBetShadowGeometry[6];
	NiControllerManager* pTableAnimManager;
	NiControllerManager* pPlayerAnimManager;
	NiControllerManager* pPlayerSplitAnimManager;
	NiControllerManager* pDealerAnimManager;
	NiControllerSequence* pTableAnimations[2];
	NiControllerSequence* pPlayerAnimations[8];
	NiControllerSequence* pPlayerSplitAnimations[8];
	NiControllerSequence* pDealerAnimations[8];
	// IDA name; not in the PDB. The "Dealer_Reveal" sequence LoadNifs (0x732540) stores here (0x733309); the PDB's
	// pDealerAnimations holds nine entries, the PC keeps the ninth in this separate field.
	NiControllerSequence* pDealerReveal;

	// The casino the menu plays at (the PDB's BlackJackMenu::pCasino).
	static inline TESCasino* pCasino;
	static NiPointer<NiNode> spSceneRoot;
	static NiPointer<NiCamera> spCamera;
	static inline int32_t iUT_X;
	// The open blackjack menu.
	static inline BlackJackMenu* pMe;
};

// The player's record for one casino: its form ID, the chips won there and the winnings level reached (quarters of
// the casino's maximum winnings).
struct CasinoData {
	// Clears the three fields.
	CasinoData();

	uint32_t casinoID;
	int32_t iTotalWinnings;
	int16_t iWinningLevel;
};

inline constexpr fnvd::Vtable BlackJackMenu_vftable{};
inline int32_t g_blackJackMaxBet = 1;
inline int32_t g_blackJackMinChipsToPlay = 1;
inline int32_t g_blackJackMenuTrait_Value;
inline CasinoData* g_blackJackCasinoStats;
