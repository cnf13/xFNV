#pragma once

#include "BSCore/MemoryManager.h"
#include "BSMenu/Menu.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

#include <stdint.h>

struct CasinoData;
class NiControllerManager;
class TESCasino;

class SlotMachineMenu : public Menu {
public:
	// The machine's states, kept in eSlotState.
	enum SlotState : uint32_t {
		SL_INTRO_ANIM = 0,
		SL_SPIN_REELS_ANIM = 1,
		SL_CALC_RESULTS = 2,
		SL_WIN_ANIM = 3,
		SL_PAYOUT_IN_ANIM = 4,
		SL_PAYOUT_OUT_ANIM = 5,
		SL_IDLE = 6,
		SL_CLOSING_MENU = 7,
		NUM_STATES = 8,
	};

	// The outcome of a spin (InterpretSpin), kept in eSpinResult.
	enum SlotResults : uint32_t {
		SL_NO_MATCH = 0,
		SL_PAIR = 1,
		SL_WILD_NULL_NULL = 2,
		SL_WINNING_VALUES = 3,
		SL_WILD_SYM_ = 4,
		SL_WILD_PAIR = 5,
		SL_THREE_MATCH = 6,
	};

	// A reel symbol (the PDB's ReelSymbols; the B variants are the symbols' second texture set).
	enum ReelSymbols : int32_t {
		SL_A = 0,
		SL_AB = 1,
		SL_B = 2,
		SL_BB = 3,
		SL_C = 4,
		SL_CB = 5,
		SL_D = 6,
		SL_DB = 7,
		SL_E = 8,
		SL_EB = 9,
		SL_F = 10,
		SL_FB = 11,
		SL_WILD = 12,
		SL_WILDB = 13,
		NUM_SYMBOLS = 14,
	};

	SlotMachineMenu();
	~SlotMachineMenu() override;

	void AttachTileByID(int32_t, Tile*) override;
	void DoClick(int32_t, Tile*) override;
	// The PC inherits DoEnter and DoLeave, unlike the Xbox PDB.
	void DoIdle() override;
	int32_t GetClass() override;

protected:
	// Draws the three reel results (keeping the previous ones as the start symbols) and returns InterpretSpin's
	// result. PDB: SlotResults, which src has no definition of.
	SlotMachineMenu::SlotResults SpinReels();
	// The spin result of eReelResults (thiscall, ret 0, 0x7C5760). PDB: SlotResults.
	SlotMachineMenu::SlotResults InterpretSpin();

public:
	char InitializeCasinoData();
	// True when all ten tiles exist.
	char IsWellFormedMenu();
	char SetupActiveReels();
	// Builds spSlotModel from the machine and reel geometry.
	void Prepare3DElements();
	void SwapBackfacingTextures();
	NiCamera* Create3DCamera();
	void SwapFrontfacingTextures();
	// IDA name; not in the PDB. Takes a spin result (DoClick passes SpinReels's, itself InterpretSpin's) and returns
	// 0 or 1 in al, which DoClick stores into bLucky as a byte.
	bool CalculateShouldWin(SlotMachineMenu::SlotResults a2);
	void UpdateEarningsString();

	static int32_t GetClassStatic();
	// Starts closing the open menu: the closing state, and the cursor shown again.
	static void Close();
	// IDA name HandleCamera; the PDB name is used.
	// Sets up the camera and draws the scene.
	static void DrawScene();
	static Tile* Create(TESCasino* apCasino, int32_t aiMinBet, int32_t aiMaxBet, int32_t aiMinWinnings,
		bool abDebugMode);
	// The exe's destructor: clears the menu's casino, model and scene state, then the base (thiscall, ret 0,
	// 0x7C0870).
	static void operator delete(void* apMemory) { ourFree(apMemory); }

	SlotState eSlotState;
	ReelSymbols eActiveReelSymbols[28];
	SlotResults eSpinResult;
	Tile* pTiles[10];
	int32_t iLuckSkill;
	int32_t iCurrentBet;
	int32_t iNumActiveSymbols;
	int32_t iChipCount;
	bool bLucky;
	uint8_t paddingD9[3];
	ReelSymbols eReelStartSymbols[3];
	ReelSymbols eReelResults[3];
	uint32_t uiPrevIdleTick;
	bool bSwapTextures;
	bool bShowingTutorial;
	// PC types leave the other bytes of this word unnamed.
	uint8_t paddingFA[2];
	int32_t iNextSoundIndex;
	bool bRunScriptOnExit;
	uint8_t padding101[3];
	NiAVObject* pSlotMachineGeometry;
	NiAVObject* pReelGeometry[3];
	NiPointer<NiSourceTexture> pSlotReelTextures[8];
	NiControllerManager* pSlotAnimManager;
	NiControllerSequence* pSlotLeverAnim;
	NiControllerSequence* pSlotCoinAnim;
	// The payout-in then payout-out sequences: the exe stores NiControllerManager::GetSequenceByName's results here.
	// The PDB declares the two as one array.
	NiControllerSequence* pSlotPayoutListAnim[2];
	NiControllerSequence* pSlotReelAnim[3];
	int32_t iCheatToggle;

	// The open menu, or null.
	static inline SlotMachineMenu* pMe;
	static inline int32_t eLastStage;
	static inline float fTimeDelta;
	static inline int32_t iUT_X;
	static inline TESCasino* pCasino;
	// IDA: SlotMachineMenu::spCamera.
	static NiPointer<NiCamera> sp3DCamera;
	// IDA: SlotMachineMenu::spRootNode.
	static NiPointer<NiNode> spSlotModel;
};

inline int32_t g_slotMachineMaxBet = 1;
inline int32_t g_slotMachineMinBet = 1;
inline int32_t g_slotMachineMenuTrait_Value;

// IDA name; the PDB's SlotMachineMenu::casinoData: the casino's record of the player's winnings (the exe reads
// iTotalWinnings at +4 and the int16 iWinningLevel at +8 through it).
inline CasinoData* g_slotMachineCasinoStats;
inline constexpr fnvd::Vtable SlotMachineMenu_vftable{};
