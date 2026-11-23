#pragma once

#include "BSCore/BSSimpleArray.h"
#include "BSMenu/Menu.h"
#include "Fallout Shared/TESCaravan.h"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

class Actor;
class BSShaderPPLightingProperty;
class CaravanAIPackage;
class InterfaceManager;
class NiControllerManager;

class CaravanMenu : public Menu {
public:
    // The tiles pTiles holds.
    enum TileIndex : int32_t {
        TI_ANTE_MENU_TILESET = 0,
        TI_DECK_MENU_TILESET = 1,
        TI_GAME_MENU_TILESET = 2,
        TI_RESULTS_MENU_TILESET = 3,
        TI_ANTE_NPC_BET = 4,
        TI_ANTE_PLAYER_BET = 5,
        TI_ANTE_TOTAL_FUNDS = 6,
        TI_ANTE_ACCEPT_BUTTON = 7,
        TI_DECK_CARDS = 8,
        TI_DECK_TOTAL_CARDS = 9,
        TI_DECK_CARAVANDECK_BUTTON = 10,
        TI_DECK_ALLCARDS_BUTTON = 11,
        TI_DECK_ADD_BUTTON = 12,
        TI_DECK_REMOVE_BUTTON = 13,
        TI_DECK_PLAY_BUTTON = 14,
        TI_DECK_METER = 15,
        TI_GAME_ANTE_POT = 16,
        TI_GAME_NET_TOTAL = 17,
        TI_GAME_WIN_LOSS = 18,
        TI_GAME_SELECT_CARD = 19,
        TI_GAME_DISCARD_CARD = 20,
        TI_GAME_DISCARD_TRACK = 21,
        TI_GAME_CANCEL = 22,
        TI_GAME_PLAYER_T1 = 23,
        TI_GAME_PLAYER_T2 = 24,
        TI_GAME_PLAYER_T3 = 25,
        TI_GAME_NPC_T1 = 26,
        TI_GAME_NPC_T2 = 27,
        TI_GAME_NPC_T3 = 28,
        TI_GAME_PLAYER_NUMCARDS = 29,
        TI_GAME_NPC_NUMCARDS = 30,
        TI_RESULTS_WINNINGS = 31,
        TI_RESULTS_LOSSES = 32,
        TI_RESULTS_WIN_LOSS = 33,
        TI_RESULTS_BIGGEST_WIN = 34,
        TI_RESULTS_STATUS = 35,
        TI_ANTE_RAISE_BUTTON = 36,
        TI_ANTE_AUTOMATCH_BUTTON = 37,
        TI_ANTE_EXIT_BUTTON = 38,
        TI_DECK_NAVIGATE_BUTTON = 39,
        NUM_TILES = 51,
        TI_NO_TILE = -1,
    };

    // The menu's flags in iFlags.
    enum enumFlags : int32_t {
        CAF_DISABLE_ALLCONTROLS = 0x1,
        CAF_DISABLE_Y = 0x2,
        CAF_DISABLE_A = 0x4,
        CAF_DISABLE_B = 0x8,
        CAF_DISABLE_X = 0x10,
        CAF_DISABLE_LB = 0x20,
        CAF_DISABLE_RB = 0x40,
        CAF_DISABLE_LT = 0x80,
        CAF_DISABLE_RT = 0x100,
        CAF_PLAYERFORFEIT = 0x200,
        CAF_PLAYERWIN = 0x400,
        CAF_TRACK_SETUP = 0x800,
        CAF_SELECT_TRACK = 0x1000,
        CAF_DESTROY_TRACK = 0x2000,
        CAF_AI = 0x4000,
        CAF_PROCESS_JOKER = 0x8000,
        CAF_ISONTOP = 0x10000,
        CAF_MARKFORCLOSE = 0x20000,
        CAF_PROCESSONIDLE = 0x40000,
    };

    // The menu page shown, in eActiveMenu.
    enum ActiveMenu : uint32_t {
        CA_ANTE_MENU = 0,
        CA_DECK_MENU = 1,
        CA_GAME_MENU = 2,
        CA_RESULTS_MENU = 3,
    };

    // The game's state (the animation playing or the step being processed), in eGameState.
    enum CaravanState : uint32_t {
        CA_IDLE = 0,
        CA_CAMERA_TO_DECK_ANIM = 1,
        CA_CAMERA_TO_GAME_ANIM = 2,
        CA_CAMERA_TO_RESULTS_ANIM = 3,
        CA_ANTE_MONEYIN_ANIM = 4,
        CA_DECK_FORWARD_ANIM = 5,
        CA_DECK_BACK_ANIM = 6,
        CA_DECK_FLIP_ANIM = 7,
        CA_DECK_FASTFORWARD_ANIM = 8,
        CA_DECK_REWIND_ANIM = 9,
        CA_DECK_WIPE_ANIM = 10,
        CA_GAME_INTRO_DEAL_ANIM = 11,
        CA_GAME_INTRO_DISCARD_ANIM = 12,
        CA_GAME_PLACECARD_ANIM = 13,
        CA_GAME_DISCARDCARD_ANIM = 14,
        CA_GAME_DISCARDTRACK_ANIM = 15,
        CA_GAME_REMOVECARD_ANIM = 16,
        CA_GAME_JACK_CLEAR_ANIM = 17,
        CA_GAME_JACK_FINISH_ANIM = 18,
        CA_GAME_JOKER_CLEAR_ANIM = 19,
        CA_GAME_JOKER_FINISH_ANIM = 20,
        CA_GAME_PROCESS_AI = 21,
        CA_GAME_GAMEOVER = 22,
        CA_CLOSING_MENU = 23,
    };

    // One track's direction, suit and value.
    struct TrackInfo {
        int32_t eDirection;
        int32_t eCardSuit;
        int32_t iValue;
    };

    CaravanMenu();
    ~CaravanMenu() override;

    void AttachTileByID(int32_t, Tile*) override;
    void DoDownClick(int32_t, Tile*) override;
    void DoClick(int32_t, Tile*) override;
    // DoEnter, DoLeave and DoStillDown are inherited, not overridden.
    void DoIdle() override;
    bool DoKey(int32_t) override;
    int32_t GetClass() override;
    bool DoGamepad(int32_t, float) override;
    bool DoGamepadUpEvent(int32_t, float) override;

    // Whether flag aeFlag is set.
    bool IsFlagSet(enumFlags aeFlag);
    // Updates the track values and decides whether a side has won; sets CAF_PLAYERWIN when the game is over.
    bool IsGameOver();
    // Sets flag aeFlag to abValue.
    void SetFlag(enumFlags aeFlag, bool abValue);
    // Updates the alpha of the tiles of the active menu; -1 uses eActiveMenu.
    void UpdateTileAlpha(TileIndex eTile);
    // Sets the alpha trait of the child tiles of pTiles[aeTile].
    void SetTileTextAlpha(TileIndex TI, float alpha);
    void UpdateCaravanFlags();
    void UpdateBettingUI(int32_t maxraise, int32_t raise);
    void UpdateTrackValue(int32_t aiTrack);
    bool IsWellFormedMenu();
    void LoadNifs();
    void SwapDeckBuildingTextures(int32_t aiTarget, bool abFastScrolling);
    void UpdateDealCardTexture(bool abPlayer);
    void SelectDeckCard(bool abUnselectOnly);
    void SelectTrackCard(int32_t aiTrack, int32_t aiRow);
    void SelectWholeTrack(int32_t aiTrack);
    void DeselectWholeTrack(int32_t aiTrack);
    void HandleTrackSelection(int32_t aiXShift, int32_t aiYShift);
    void LoadNewBill(int32_t aiBill);
    void LoadNewCoin(int32_t aiCoin);

protected:
    void ZeroFlags();
    void DeselectTrackCard();
    int32_t CalculateWinningState(int32_t aivalue);

public:
    uint32_t LoadNifs2();
    // The menu's 3D camera, built fresh; it never reads this.
    NiCamera* Create3DCamera();
    char PrepareCaravanData();
    void SetupCurrency();
    void PrepareBettingCurrency();
    void PrepareGameMenu();
    void PrepareResultsMenu();
    void FlipSelectedCardTextures(int32_t aiTarget);
    void RewardOrChargePlayer(char abReward);
    void UpdateGameHandTextures(bool abPlayer, bool bStartup);
    void UpdateSelectedTrackCardTexture();
    void UpdateTrackCardTexture(int32_t aiTrack, int32_t aiRow, int32_t aiColumn);
    int32_t GetRowValue(int32_t aiTrack, int32_t aiRow);
    bool IsValidCardPlacement(int32_t aiTrack, int32_t aiRow, int32_t aiDeckCard, bool abIsNPC);
    CaravanAIPackage* ProcessAI(CaravanAIPackage* apResult);
    // Returns the face value aiFaceValue times the number of track aiTrack's cards with that face value; each card's
    // data.iFaceValue is compared with aiFaceValue.
    int32_t GetValueOfCardValueInTrack(int32_t aiTrack, int32_t aiFaceValue);
    // The sum of the face values of track aiTrack's cards whose suit is auiSuit; auiSuit is compared with the card's
    // iCardSuit.
    int32_t GetSumCardsB44WithSuit(int32_t aiTrack, uint32_t auiSuit);

    // The menu's class ID, 1083.
    static int32_t GetMenuID_();
    // The ante quantity menu's callback: raises the player's ante by aiValue, matches it
    // with the NPC's ante up to the NPC's limit and updates the betting tiles.
    static void ItemSelectCallback(int32_t aiValue);
    // Closes the menu through pMe.
    static void Close();
    static void CancelConfirmFunc();
    static void Draw3DElements();
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
    static Tile* Create(
        Actor* apNPCChallenger,
        TESCaravanDeck* apNPCDeck,
        int32_t aiDiffLevel,
        float afBetPercentage,
        bool abDebugMode
    );

    ActiveMenu eActiveMenu;
    CaravanState eGameState;
    // The PC version has 51 tiles, where the Xbox version has 40.
    Tile* pTiles[51];
    uint32_t uiPrevIdleTick;
    Actor* pChallenger;
    BSSimpleList<TESCaravanCard*>* pDeckParam;
    int32_t iDiffLevel;
    float fBetPercentage;
    int32_t iNPCTotalFunds;
    NiAVObject* pBillsGrid;
    NiAVObject* pCoinsGrid;
    NiAVObject* pCoinGeometry[3];
    NiAVObject* pBillGeometry[3];
    // The animation lists hold the NiControllerSequence*s that AddCurrencyCoinNif/AddCurrencyBillNif get from
    // GetSequenceByName.
    BSSimpleArray<NiAVObject*> pListOfCoins;
    BSSimpleList<NiControllerSequence*>* pListOfCoinAnimations;
    int32_t iCoinIndexToAnim;
    BSSimpleArray<NiAVObject*> pListOfBills;
    BSSimpleList<NiControllerSequence*>* pListOfBillAnimations;
    int32_t iBillIndexToAnim;
    BSSimpleArray<int32_t> intlistbills;
    BSSimpleArray<int32_t> intlistcoins;
    NiAVObject* pTableGeometry;
    NiAVObject* pInactiveDeckGeometry;
    NiAVObject* pActiveDeckGeometry;
    NiAVObject* pHandGeometry[2];
    NiAVObject* pTrackGeometry[6][7];
    NiControllerManager* pCameraAnimManager;
    NiControllerSequence* pCameraAnimations[4];
    NiControllerManager* pInactiveDeckAnimManager;
    NiControllerSequence* pInactiveDeckAnimations[6];
    NiControllerManager* pActiveDeckAnimManager;
    NiControllerSequence* pActiveDeckAnimations[6];
    NiControllerManager* pHandAnimManager[2];
    NiControllerSequence* pHandAnimations[2][6];
    NiControllerSequence* pStartAnimations[2][8];
    NiControllerManager* pTrackAnimManager[6][7];
    NiControllerSequence* pTrackAnimations[6][7][4][2];
    BSShaderPPLightingProperty* deckFaceShaders[96];
    BSSimpleArray<TESCaravanCard*> pDeckArray;
    BSSimpleArray<int32_t> jokerLocations[6];
    int32_t iPlayerAnte;
    int32_t iNPCAnte;
    int32_t iTotalFunds;
    int32_t iAnte;
    uint32_t iCardsInDeck;
    uint32_t iTotalCards;
    int32_t iJumpAmount;
    int32_t iPrevValue;
    int32_t iNumSteps;
    int32_t iPrevXLocation;
    // BSSimpleArray<TESCaravanCard*> decks, hands and tracks.
    BSSimpleArray<TESCaravanCard*> pPlayerDeck;
    BSSimpleArray<TESCaravanCard*> pNPCDeck;
    BSSimpleArray<TESCaravanCard*> pPlayerHand;
    BSSimpleArray<TESCaravanCard*> pNPCHand;
    BSSimpleArray<TESCaravanCard*> pCardTracks[6][7];
    TrackInfo trackInfo[6];
    int32_t iSelectedDeckCard;
    int32_t iSelectedTrack;
    int32_t iSelectedRow;
    int32_t iSelectedColumn;
    float fSelectionDelay;
    int32_t iNextPlayerCard;
    int32_t iNextNPCCard;
    TESCaravanCard* pSelectedCard;
    NiAVObject* pSelectedObject;
    NiAVObject* pSelectedShadow;
    NiAVObject* pTrackGrids[6];
    NiAVObject* pCursors[2];
    uint32_t iFlags;
    bool bTutorial;
    int32_t iTutorial;
    // The arrow key DoKey received while the game was idle, which DoIdle moves the selection by; 0 when none.
    uint32_t uiPendingArrowKey;
    bool bClickedScrollbar;
    bool bScrolling;
    bool bContinueProcessing;

    static inline CaravanMenu* pMe;
    static inline int32_t iUT_CurrentValue;
    static inline int32_t iUT_Value;
    static inline int32_t iUT_X;
    static inline bool bDebugAI;
    static inline float fTimeDelta;
    static NiPointer<NiCamera> sp3DCamera;
    static NiPointer<NiNode> spModel;
};

// One move the caravan AI is weighing: the value it aims for, what the move calculates to, the action, and the
// track, row, deck card and priority it concerns.
class CaravanAIPackage {
public:
    void Print_WhatIsHeThinking(BSSimpleArray<TESCaravanCard*, 1024>* pNPCHandArray);

    // The BSSimpleArray<CaravanAIPackage *> Sort compare (qsort form): each argument points to an element; compares the
    // packages' iPriority, highest first.
    static int32_t SortPackagesFunc(const void* t1, const void* t2);

    int32_t iValueGoal;
    int32_t iCalcResults;
    int32_t iActionType;
    int32_t iTrack;
    int32_t iRow;
    int32_t iDeckCard;
    int32_t iPriority;
};

inline InterfaceManager* g_caravanMenu_InterfaceManager;
inline char bStarted;
inline int32_t ePrevGameState;
