#pragma once

#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSStringT.h"
#include "BSCore/NewOverloads.h"
#include "BSMain/Setting.h"
#include "BSMenu/Menu.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "SDK/FaceGen/MatrixVT.h"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

class TESNPC;
class TileImage;
class TileMenu;

// The character generation menu: race, sex and face.
class RaceSexMenu : public Menu {
public:
    // The menu's option lists, the indices of pOptionListsA.
    enum ListID : int32_t {
        RSM_SEX = 0,
        RSM_RACE = 1,
        RSM_FACE = 2,
        RSM_HAIR = 3,
        RSM_CUSTOM = 4,
        RSM_HAIR_STYLE = 5,
        RSM_HAIR_COLOR = 6,
        RSM_FACIAL_HAIR = 7,
        RSM_EYE_COLOR = 8,
        RSM_SHAPE = 9,
        RSM_SHAPE_GENERAL = 10,
        RSM_SHAPE_SUBMENU_BEGIN = 10,
        RSM_SHAPE_FOREHEAD = 11,
        RSM_SHAPE_BROW = 12,
        RSM_SHAPE_EYES = 13,
        RSM_SHAPE_NOSE = 14,
        RSM_SHAPE_MOUTH = 15,
        RSM_SHAPE_CHEEKS = 16,
        RSM_SHAPE_JAW = 17,
        RSM_SHAPE_CHIN = 18,
        RSM_TONE = 19,
        RSM_SHAPE_SUBMENU_END = 19,
        RSM_LIST_COUNT = 20,
    };

    // Bit numbers in uiFlags.
    enum Flag : int32_t {
        RSM_FLAG_MAKE_NEW_HAIRLIST = 0,
        RSM_FLAG_MAKE_NEW_EYESLIST = 1,
        RSM_FLAG_SLIDER_UPDATE = 2,
        RSM_FLAG_PC_YOUNG = 3,
        RSM_FLAG_STICK_RELEASED = 4,
        RSM_FLAG_ROTATING = 5,
        RSM_FLAG_ZOOMING = 6,
        RSM_FLAG_RANDOM_HEAD = 7,
        RSM_FLAG_UPDATE_HEAD = 8,
        RSM_FLAG_UPDATE_BODY = 9,
    };

    // The IDs of the menu's options (Entry::id); SynchronizeFaceSliders tests 34 and 35.
    enum OptionID : int32_t {
        RSM_MALE = 21,
        RSM_FEMALE = 22,
        RSM_RACE_OPTION = 23,
        RSM_PRESET_FACE = 24,
        RSM_RANDOMIZE_FACE = 25,
        RSM_AGE_FACE = 26,
        RSM_HAIR_STYLE_OPTION = 27,
        RSM_FACIAL_HAIR_OPTION = 28,
        RSM_HAIR_COLOR_PRESET = 29,
        RSM_HAIR_COLOR_RED = 30,
        RSM_HAIR_COLOR_GREEN = 31,
        RSM_HAIR_COLOR_BLUE = 32,
        RSM_EYE_COLOR_OPTION = 33,
        RSM_SHAPE_SLIDER = 34,
        RSM_TONE_SLIDER = 35,
    };

    // One entry of the menu's lists (races, hair, eyes ...): its tile, an ID and the form or value it stands for.
    struct Entry {
        Entry();

        void* GetPointerData() {
            return data;
        }
        // Sets the option's data.
        void SetData(void* apData) {
            data = apData;
        }
        void SetData(int32_t aiData) {
            iData = aiData;
        }
        // The option's value: its tile's "_position" trait, truncated.
        int32_t GetValue();
        // Sets the tile's "_enabled" trait, when the option has a tile.
        void SetEnabled(bool abEnabled);
        // Only reads Src (it passes it to Tile::Set).
        void SetDisplay(const char* Src);
        void SetSelected2();
        void SetSelected1();
        bool HasSelectedValue2();
        // The slider's value: its tile's user1 trait, truncated; SetValue writes it.
        int32_t GetSliderValue();
        bool SetValue(int32_t aiVal);
        void GetRange(int32_t& aiLow, int32_t& aiHigh);
        void SetRange(int32_t aiLow, int32_t aiHigh);
        uint32_t GetIncrement();

        static void operator delete(void* apMemory) {
            ourFree(apMemory);
        }

        Tile* tile;
        uint32_t id;

        // The form the option stands for, or an integer (a face slider's FaceGen control index) in the same field.
        union {
            void* data;
            int32_t iData;
        };
    };

    // A list of entries shown in a tile of the menu.
    class List {
    public:
        List(int32_t aiID, const char* apName);
        ~List();

        char Focus(char acFocus);
        // Removes and frees every option.
        void Flush();
        // Adds an option showing apName with ID aiID, built from the list item template.
        Entry* AddBasicOption(const char* apName, int32_t aiID);
        // Adds an option showing apName from the template apTemplate with ID aiID, inserted before apIter, or appended
        // when it is null.
        Entry* AddMenuOption(const char* apName, const char* apTemplate, int32_t aiID, NiTListItem<Entry*>* apIter);
        // Adds an option "apName >" that opens apSubmenu, linking apSubmenu back to this list.
        Entry* AddSubmenuOption(const char* apName, List* apSubmenu);
        // Links this list forward to apList and apList back to this one, by their header tiles' user0 IDs.
        void LinkForwardTo(List* apList);
        void UpdateCurrentSelection();
        // Stores the callback that tells whether an option is the selected one.
        void SetIsItemSelected(bool (*apfnIsSelected)(Entry*)) {
            IsItemSelected = apfnIsSelected;
        }
        void SetParentID(uint32_t auiID) {
            parentID = auiID;
        }
        // Adds a slider option apDisplayText with ID aeID over aiLowVal..aiHighVal, starting at aiStartVal, stepping
        // afJumpPct.
        Entry* AddSliderOption(
            const char* apDisplayText,
            int32_t aeID,
            int32_t aiLowVal,
            int32_t aiHighVal,
            int32_t aiStartVal,
            float afJumpPct
        );
        // Adds an option before the aiPos-th entry: walks aiPos items, then calls AddMenuOption with that item. aiID is
        // int32_t to match AddMenuOption, to which it is passed on.
        Entry* InsertMenuOption(uint32_t aiPos, const char* apDisplayText, const char* apTemplate, int32_t aiID);
        // Removes the entry whose id is auiID and moves the entries after it up. ValidateHairOptionList passes 7.
        void RemoveByID(uint32_t auiID);
        // Returns the entry whose id is aiID, or null.
        Entry* GetSliderWithID(int32_t aiID);
        // Returns whether the key moved the selection.
        bool HandleKey(int32_t aiKey);
        void SetActive();
        void ScrollBy(int32_t aiDelta);
        // The option whose tile is apTile, or null; called on pOptionListsA's lists.
        Entry* Find(Tile* apTile);

        static void operator delete(void* apMemory) {
            ourFree(apMemory);
        }

        // The options, built with the engine's pointer-list code (AddTail, RemoveAll).
        NiTPointerList<Entry*> items;
        uint32_t parentID;
        uint32_t childID;
        Tile* listItemTemplate;
        float height;
        // Whether an option is the selected one.
        bool (*IsItemSelected)(Entry*);
        Entry* selectedEntry;
    };

    using pEquippedInventoryListA_t = ItemChange* [20];

    RaceSexMenu();
    ~RaceSexMenu() override;

    int32_t GetClass() override {
        return 0x40C;
    }
    void AttachTileByID(int32_t aiID, Tile* apTile) override;
    void DoDownClick(int32_t aiID, Tile* apTarget) override;
    void DoClick(int32_t aiID, Tile* apTarget) override;
    void DoStillDown(int32_t aiID, Tile* apTarget) override;
    void DoIdle() override;
    bool DoGamepad(int32_t aiButton, float afValue) override;

    // Builds the lists and options of the menu for the context aeContext (2 barber, 3 plastic surgery, else
    // character creation).
    void _Create(RaceSexMenuContext aeContext);
    // Shows list aeList.
    bool SetActiveList(RaceSexMenu::ListID aeList);
    // The list SetActiveList showed: the menu tile's trait 0x1004, where SetActiveList stores it.
    RaceSexMenu::ListID GetActiveList();
    // strProjectorFile's text, the model the rendered menu loads.
    const char* GetProjectorFile();
    // Fills the presets list.
    void MakePresetHeadList();
    // Adds the face shape and tone sliders.
    void AddFaceSliders();
    // Whether flag aeFlg is set in the menu's flags.
    bool IsFlagged(Flag aeFlg);
    // Sets or clears flag aeFlg.
    void SetFlag(Flag aeFlg, bool abOn);
    void SynchronizeFaceSliders(OptionID aeSliderType);
    void UpdatePlayerHead(bool abChangeRace, bool abShape, bool abTone);
    void UpdatePlayerBody();
    void SetPlayerNode(NiNode* apNode);
    void NiUpdatePlayer3D();
    // Reattaches (abSwapHairType) or re-morphs and re-tints the player's hair and accessories, then updates the
    // player's head; false when the player has no FaceGen head.
    bool UpdatePlayerHair(bool abSwapHairType);
    // Refills the eyes list with the playable eyes the player's NPC can have.
    void MakeEyeColorList();
    bool RotatePlayer(float afRadians, bool abForce);
    bool ZoomPlayer(float afDelta, bool abForce);
    void InitHairStyles();
    void ValidateHairOptionList();
    void UpdateScrollArrows();
    void OnFaceSliderChange(RaceSexMenu::Entry* apChangedSlider);
    void OnHairColorSliderChange(RaceSexMenu::Entry* apChangedSlider);
    void SetHairColorSliders(uint32_t aiColor);
    char UpdateEntriesBetween(int32_t aiStart, int32_t aiEnd, FanTypeE aeType);
    void ValidateHairAndEyes();
    bool UpdatePlayerEyes();
    void ChangeHairColor();
    void SliderUpdate();
    void SliderRelease();
    void InitPresetsListAndPickFirstPreset();

    // Opens the menu for the given context; its menu tile, or null.
    static Tile* Create(RaceSexMenuContext aeContext);
    static int32_t GetClassStatic();
    // Whether the option apOpt holds the player's race, hair, eye colour, sex or first head part.
    static bool IsPlayerRace(Entry* apOpt);
    static bool IsPlayerHair(Entry* apOpt);
    static bool IsPlayerEyes(Entry* apOpt);
    static bool IsPlayerSex(Entry* apOpt);
    static bool IsPlayerHeadPart(Entry* apOpt);
    static void RandomizeCallback();
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
    static void Close();

    List* pOptionListsA[20];

    // pTiles is indexed by tile ID in AttachTileByID and DoClick; the other functions use the six named pointers.
    union {
        Tile* pTiles[6];

        struct {
            TileImage* rsmBackground;
            TileImage* rsmFaceGrab;
            TileImage* rsmScrollUpTarget;
            TileImage* rsmScrollDownTarget;
            // pTiles[4]: the back button.
            TileImage* rsmBackButton;
            // pTiles[5]: the next button.
            TileImage* rsmNextButton;
        };
    };

    BSStringT<char> strProjectorFile;
    BSStringT<char> strConfirmationMsg;
    NiPoint3 ptZoomBound[2];
    float fDragStartX;
    // The slider's value when the drag started.
    int32_t iDragStartSliderVal;
    Tile* pSliderElem;
    RaceSexMenu::Entry* pSliderOpt;
    // The zoom DoIdle steers fZoomPct towards, clamped to [0, 1].
    float fZoomTarget;
    // The zoom's rate, which DoIdle integrates by the frame time.
    float fZoomVelocity;
    // The zoom applied, in [0, 1]: ZoomPlayer steps it and places the camera between ptZoomBound by it.
    float fZoomPct;
    NiPointer<NiAVObject> spPlayerNode;
    TESNPC* pPlayerBase;
    BSSimpleArray<TESNPC*> xPresetHeadsA;
    uint32_t uiFlags;
    BSSoundHandle Sounds[4];
    ListID eLastList;
    // The face coordinates the player's head was last morphed to.
    FR2MatrixVTC<float> LastFaceCoord[2][2];

    static inline RaceSexMenu* pMe;
    // The "BACK" button text.
    static BSStringT<char> strBackCaps;
    static inline bool bWantedWeaponOut;
    static BSStringT<char> strNextCaps;
    static BSStringT<char> strDoneCaps;
    static inline pEquippedInventoryListA_t pEquippedInventoryListA{};

protected:
    static SettingT<INISettingCollection> fStartingZoom;
};

extern SettingT<INISettingCollection> fRSMFaceSliderDefaultMin;
extern SettingT<INISettingCollection> fRSMFaceSliderDefaultMax;
inline int32_t iRSMStickAxisMin = -32768;

inline int32_t iRSMStickAxisMax = 32767;
