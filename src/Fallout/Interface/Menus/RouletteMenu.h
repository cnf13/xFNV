#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSMain/Setting.h"
#include "BSMenu/Menu.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "SDK/MSVC/ehvec.h"

struct CasinoData;
class NiControllerManager;
class TESCasino;

class RouletteMenu : public Menu {
public:
    // The menu's states (eRouletteState).
    enum RouletteState : int32_t {
        RO_INTRO_ANIM = 0,
        RO_SPIN_ANIM = 1,
        RO_BET_ANIM = 2,
        RO_DETERMINE_RESULTS = 3,
        RO_IDLE = 4,
        RO_CLOSING_MENU = 5,
        NUM_STATES = 6,
    };

    // The tiles pTiles holds.
    enum TileIndex : int32_t {
        TI_CURRENT_BET = 0,
        TI_CHIP_COUNT = 1,
        TI_CASINO_INFO = 2,
        TI_PLACEBET_CNTRL = 3,
        TI_REMOVEBET_CNTRL = 4,
        TI_FINISHBETS_CNTRL = 5,
        TI_INCREASEBET_CNTRL = 6,
        TI_DECREASEBET_CNTRL = 7,
        TI_EXIT_CNTRL = 8,
        TI_STATUS_TEXT = 9,
        TI_TOTAL_BET = 10,
        NUM_TILES = 11,
    };

    // The colour of a bet tile.
    enum BetTileColor : int32_t {
        RO_COLOR_NONE = 0,
        RO_COLOR_BLACK = 1,
        RO_COLOR_RED = 2,
    };

    // A bet's kind, valued by its payout ratio (BetTileInfo::betPayoutRatio).
    enum BetType : int32_t {
        RO_ONE = 35,
        RO_TWO = 17,
        RO_THREE = 11,
        RO_FOUR = 8,
        RO_FIVE = 6,
        RO_SIX = 5,
        RO_TWELVE = 2,
        RO_EIGHTEEN = 1,
        RO_REDBLACK = 1,
        RO_EVENODD = 1,
    };

    // One betting field of the table: the numbers it covers, its payout and the names of its face and tiles.
    struct BetTileInfo {
        BetTileInfo();

        BSSimpleList<int32_t> numValueList;
        BetTileColor tileColor;
        BetType betPayoutRatio;
        NiPoint3 location;
        char faceName[8];
        char cBetTiles[64];
    };

    // One placed bet: the chips, the betting field (an index into betTileInfoArray) and the chip model.
    struct RouletteBet {
        int32_t betValue;
        int32_t betIndex;
        int32_t modelIndex;
    };

    RouletteMenu();
    ~RouletteMenu() override;

    void AttachTileByID(int32_t, Tile*) override;
    void DoClick(int32_t, Tile*) override;
    void DoEnter(int32_t, Tile*) override;
    void DoLeave(int32_t, Tile*) override;
    void DoIdle() override;
    int32_t GetClass() override;

    // Refreshes the casino earnings text.
    void UpdateEarningsString();
    // The betting field under the cursor, 159 for none.
    int32_t GetBetIndex();
    // Turns the wheel texture to aiResult.
    void ShiftWheelTexture(int32_t aiResult);
    // Sets the alpha of the text tiles under pMe's tile aeTile.
    void SetTileTextAlpha(TileIndex TI, float alpha);
    // Moves every bet chip and its shadow back off the table.
    void ResetBetChips();
    // Shows the chip model for the current bet.
    void SetBetChip();
    // Loads the table, wheel and chip models into spModel and fills the menu's geometry pointers.
    void Prepare3DElements();
    NiCamera* Create3DCamera();
    // Whether all eleven pTiles are attached.
    bool IsWellFormedMenu();
    // Picks the spin's result (0-38) and animation (0-3), and whether the player's luck makes the spin lucky or
    // unlucky.
    void SpinWheel();
    bool SetBet(int32_t aiIndex);
    bool IsBetChipHidden(int32_t a1);
    void SetupPossibleBets();
    bool RemoveBet(int32_t aiIndex);
    void RemoveBetNif(int32_t a1);
    void SetWinningSlotLuckyAndAnimToPlay();

    // Draws the roulette table scene.
    static void DrawScene();
    // Restores the cursor and switches pMe to RO_CLOSING_MENU.
    static void Close();
    // Opens the roulette menu for apCasino with the given bet limits; null when the player is barred, broke or
    // below aiMinWinnings, or the menu file is malformed. abDebugMode is not read.
    static Tile*
    Create(TESCasino* apCasino, int32_t aiMinBet, int32_t aiMaxBet, int32_t aiMinWinnings, bool abDebugMode);
    // The roulette menu's class id.
    static int32_t GetClassStatic();
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    RouletteState eRouletteState;
    Tile* pTiles[11];
    uint32_t uiPrevIdleTick;
    BetTileInfo betTileInfoArray[159];
    BSSimpleList<RouletteBet*>* pBetList;
    // The wheel's numbers in wheel order.
    int32_t iNumberStrip[38];
    NiAVObject* pRouletteTableGeometry;
    NiAVObject* pRouletteGridGeometry;
    NiAVObject* pCursorGeometry;
    NiAVObject* pBetGeometry[10];
    NiAVObject* pBetShadowGeometry[10];
    NiAVObject* pCurrentBetFace;
    NiAVObject* pValidBetFace;
    NiAVObject* pInvalidBetFace;
    // The wheel's texture; the destructor clears it through NiPointer::operator=.
    NiPointer<NiSourceTexture> pWheelTexture;
    NiControllerManager* pTableAnimManager;
    // One spin animation per SpinWheel animation index.
    NiControllerSequence* pTableAnimations[3];
    // The bet tiles' hover radii, set by SetupPossibleBets: fValidRadius is 0.49 of fRadius, and fSpecialRadius has one
    // per special bet tile (tiles 144 to 158; see ComputeHoveredTileID).
    float fRadius;
    float fValidRadius;
    float fSpecialRadius[15];
    float fxTranslate;
    float fyTranslate;
    float fzTranslate;
    int32_t iNextModelIndex;
    int32_t iNumBetsSet;
    int32_t iCurrentBet;
    int32_t iTotalBet;
    int32_t iChipCount;
    int32_t iLuckValue;
    int32_t iTotalWinnings;
    int32_t iMinBet;
    int32_t iMaxBet;
    int32_t iResultValue;
    int32_t iSpinAnimIndex;
    bool bCleanTable;
    bool bLucky;
    bool bUnlucky;
    bool bRunScriptOnExit;
    int32_t iCheatToggle;

    static inline CasinoData* pCasinoData;
    static inline TESCasino* pCasino;
    // The roulette scene: child 0 the wheel, 2 the cursor chip, 3-12 the bet chips and 13-22 their shadows.
    static NiPointer<NiNode> spModel;
    // The tile trait ids of the user traits "value" and "x".
    static inline int32_t iUT_Value;
    static inline int32_t iUT_X;
    // The open roulette menu.
    static inline RouletteMenu* pMe;
    static NiPointer<NiCamera> sp3DCamera;
};

using rdata_1075F74_t = const char[4];
inline rdata_1075F74_t rdata_1075F74{"_X"};

extern SettingT<GameSettingCollection> gs_sOddtext;

// The game settings DoIdle reads.
extern SettingT<GameSettingCollection> sYouWinText;

extern SettingT<GameSettingCollection> sYouLoseText;

extern SettingT<GameSettingCollection> sCasinoEarningsText;

extern SettingT<GameSettingCollection> gs_sBlacktext;
