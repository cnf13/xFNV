#pragma once

#include "BSCore/NewOverloads.h"
#include "BSMenu/Menu.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

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

    // A reel symbol; the B variants are the symbols' second texture set.
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
    // DoEnter and DoLeave are inherited, not overridden.
    void DoIdle() override;
    int32_t GetClass() override;

protected:
    // Draws the three reel results (keeping the previous ones as the start symbols) and returns InterpretSpin's result.
    SlotMachineMenu::SlotResults SpinReels();
    // The spin result of eReelResults.
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
    // Takes a spin result (DoClick passes SpinReels's, itself InterpretSpin's); DoClick stores the result into bLucky.
    bool CalculateShouldWin(SlotMachineMenu::SlotResults aeSpinResult);
    void UpdateEarningsString();

    static int32_t GetClassStatic();
    // Starts closing the open menu: the closing state, and the cursor shown again.
    static void Close();
    // Sets up the camera and draws the scene.
    static void DrawScene();
    static Tile*
    Create(TESCasino* apCasino, int32_t aiMinBet, int32_t aiMaxBet, int32_t aiMinWinnings, bool abDebugMode);
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    SlotState eSlotState;
    ReelSymbols eActiveReelSymbols[28];
    SlotResults eSpinResult;
    Tile* pTiles[10];
    int32_t iLuckSkill;
    int32_t iCurrentBet;
    int32_t iNumActiveSymbols;
    int32_t iChipCount;
    bool bLucky;
    ReelSymbols eReelStartSymbols[3];
    ReelSymbols eReelResults[3];
    uint32_t uiPrevIdleTick;
    bool bSwapTextures;
    bool bShowingTutorial;
    int32_t iNextSoundIndex;
    bool bRunScriptOnExit;
    NiAVObject* pSlotMachineGeometry;
    NiAVObject* pReelGeometry[3];
    NiPointer<NiSourceTexture> pSlotReelTextures[8];
    NiControllerManager* pSlotAnimManager;
    NiControllerSequence* pSlotLeverAnim;
    NiControllerSequence* pSlotCoinAnim;
    // The payout-in then payout-out sequences, as returned by NiControllerManager::GetSequenceByName.
    NiControllerSequence* pSlotPayoutListAnim[2];
    NiControllerSequence* pSlotReelAnim[3];
    int32_t iCheatToggle;

    // The open menu, or null.
    static inline SlotMachineMenu* pMe;
    static inline int32_t eLastStage;
    static inline float fTimeDelta;
    static inline int32_t iUT_X;
    static inline TESCasino* pCasino;
    static NiPointer<NiCamera> sp3DCamera;
    static NiPointer<NiNode> spSlotModel;
};

inline int32_t g_slotMachineMaxBet = 1;
inline int32_t g_slotMachineMinBet = 1;
inline int32_t g_slotMachineMenuTrait_Value;

// The casino's record of the player's winnings; iTotalWinnings and the int16 iWinningLevel are read through it.
inline CasinoData* g_slotMachineCasinoStats;
