#pragma once

#include "BSCore/BSSimpleArray.h"
#include "BSMenu/Menu.h"
#include "Fallout Shared/ActorValue.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

class BSShaderPPLightingProperty;

class LoveTesterMenu : public Menu {
public:
    // The machine's state: the page shown (a SPECIAL's page, LTMS_STRENGTH through LTMS_LUCK, is that actor value minus
    // 4; it also indexes Pages) or the summary. Signed: eState and eXBoxIndexState are compared signed.
    enum LOVE_TESTER_MENU_STATE : int32_t {
        LTMS_START = 0,
        LTMS_STRENGTH = 1,
        LTMS_PERCEPTION = 2,
        LTMS_ENDURANCE = 3,
        LTMS_CHARISMA = 4,
        LTMS_INTELLIGENCE = 5,
        LTMS_AGILITY = 6,
        LTMS_LUCK = 7,
        LTMS_SUMMARY = 8,
        LTMS_NUM_STATES = 9,
    };

    // A clickable 3D button of the cabinet, built by InitMeshes from the two callbacks, the button object found by name
    // and its shader property. The larger buttons InitMeshes also creates extend this with an array of text items and
    // an amount.
    class LoveTesterButton {
    public:
        // A click callback takes the button and, by its type, varargs (the callbacks read at most the button); a
        // refresh callback takes the button.
        using ButtonClickFunc = void (*)(LoveTesterButton* apButton, ...);
        using RefreshFunc = void (*)(LoveTesterButton* apButton);

        LoveTesterButton(ButtonClickFunc apfnButtonClick, RefreshFunc apfnRefresh, const char* apName);
        ~LoveTesterButton();

        static void operator delete(void* apMemory) {
            ourFree(apMemory);
        }

        ButtonClickFunc pfButtonClick;
        RefreshFunc pfRefresh;
        NiPointer<NiAVObject> spButton;
        // The button's shader property (null when it has none of the lighting shaders, types 8 to 12).
        BSShaderPPLightingProperty* pImage;

        // The base constructor writes neither member.
        union {
            bool bPageForward;
            int16_t sModifyAmount;
        };
    };

    class LoveTesterTextItem;

    // A button that changes one actor value: the base button plus the text items it refreshes and the actor value it
    // changes.
    class ActorValueButton : public LoveTesterButton {
    public:
        ActorValueButton(
            ButtonClickFunc apfnButtonClick,
            RefreshFunc apfnRefresh,
            const char* apName,
            int32_t aiActorValue,
            int32_t aiCount,
            ...
        );

        BSSimpleArray<LoveTesterTextItem*> AffectedFields;
        ActorValue::Index eActorValue;
    };

    // A number shown on the cabinet: holds the refresh callback, up to two digit objects found by name with their
    // shader properties, the actor value and the threshold.
    class LoveTesterTextItem {
    public:
        using RefreshFieldFunc = void (*)(LoveTesterTextItem* apItem);

        LoveTesterTextItem(
            RefreshFieldFunc apRefreshFieldCallback,
            const char* apTextObjectName1,
            const char* apTextObjectName2,
            int32_t aeActorValue,
            bool bElementOnDynamicGeometry,
            int32_t anThresholdToLightAt
        );

        RefreshFieldFunc pfRefreshField;
        // The digit objects' shader properties, kept only when their shader type is 8 to 12 (the lighting shaders).
        BSShaderPPLightingProperty* pText[2];
        NiAVObject* pTextObj[2];
        ActorValue::Index eActorValue;
        int32_t nThreshold;
    };

    // One point light of the cabinet: 8 bytes (InitMeshes allocates 8 per "P1_PointVal_0%i_GLOW:%i" object found).
    class LoveTesterLightBulbGlows {
    public:
        LoveTesterLightBulbGlows(NiAVObject* a_GlowCard, int32_t a_nValueToGlowAt);

        NiAVObject* m_GlowCard;
        int32_t m_nValueToGlowAt;
    };

    // One page of the menu: its buttons and its text items.
    struct LoveTesterPage {
        LoveTesterPage();

        BSSimpleArray<LoveTesterButton*> Buttons;
        BSSimpleArray<LoveTesterTextItem*> TextItems;
    };

    LoveTesterMenu();
    ~LoveTesterMenu() override;

    void AttachTileByID(int32_t, Tile*) override;
    void DoClick(int32_t, Tile*) override;
    void DoIdle() override;
    bool DoKey(int32_t) override;
    int32_t GetClass() override;
    bool DoGamepad(int32_t, float) override;

    bool TurnPage(bool bForward);
    // Lights the bulbs up to the selected SPECIAL's value and runs the refresh callbacks of the current page's and the
    // global page's text items and buttons.
    void RefreshPage();
    void LoadButtonAndTextImages();
    // Loads the cabinet's two models into the 3D menu scene, starts its "Forward" animation, lights it, and builds the
    // light bulbs, buttons and text items of every page.
    void InitMachine();
    ActorValue::Index GetSelectedSPECIAL();
    char CheckTilesExist(int32_t* a1);
    NiCamera* SetupCamera();

    // The next-page button's callback, which once the open instance's animation has finished runs TurnPage(1) and
    // RefreshPage on it. Typed as ButtonClickFunc, which InitMachine stores it as.
    static void NextPage(LoveTesterButton* apButton, ...);
    // Draws the menu's 3D scene.
    static void Draw3DElements();
    // The menu's ID, without an instance; called from Create.
    static int32_t GetClassStatic();
    // The menu's close code, which Interface::CloseAllSupportMenus runs before it removes the menu's tile. Static: it
    // reads only pInstance.
    static void Close();
    // The text items' refresh callback (InitMeshes stores it as their pfRefreshField): the player's permanent actor
    // value picks the digit texture, which goes to the item's first text property.
    static void RefreshActorValue(LoveTesterTextItem* apTextItem);
    // Once the open instance's animation has finished, runs TurnPage(0) and RefreshPage on it. Typed as
    // ButtonClickFunc, which InitMachine stores it as; the button is never read.
    static void PreviousPage(LoveTesterButton* apButton, ...);
    // Opens the menu with the default 35 points.
    static void Create();
    // Opens the menu with aiTotalPoints to spend.
    static void Create(int32_t aiTotalPoints);
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
    // The points text item's refresh callback (set by InitMachine's LoveTesterTextItem::Construct): reads apItem's
    // pText and pTextObj.
    static void RefreshTotalPointsValue(LoveTesterMenu::LoveTesterTextItem* apItem);
    // A decrease button's refresh callback: culls the button at its actor value's minimum.
    static void RefreshLeftAVButton(LoveTesterMenu::LoveTesterButton* apButton);
    // An increase button's refresh callback: culls the button at the maximum or with no points left.
    static void RefreshRightAVButton(LoveTesterMenu::LoveTesterButton* apButton);
    // A page button's refresh callback (installed by InitMachine's LoveTesterButton::Construct): calls apButton's
    // pImage.
    static void RefreshAllDoneButton(LoveTesterMenu::LoveTesterButton* apButton);
    // The previous-page button's refresh callback:
    // calls apButton's pImage (+0xC) with spPreviousPageButtonImage. Typed as LoveTesterMenu::RefreshAllDoneButton.
    static void RefreshPreviousPageButton(LoveTesterMenu::LoveTesterButton* apButton);
    // The decrease buttons' click callback (InitMachine stores it only in ActorValueButtons): reads apButton's
    // eActorValue. Typed as ButtonClickFunc, which InitMachine stores it as.
    static void DecreaseActorValue(LoveTesterMenu::LoveTesterButton* apButton, ...);
    // A click callback, typed as ButtonClickFunc, which InitMachine stores it as; the button argument is never read.
    static void DecreaseCurrentActorValue(LoveTesterMenu::LoveTesterButton* apButton, ...);
    // The increase buttons' click callback: reads the ActorValueButton's eActorValue. Typed as ButtonClickFunc, which
    // InitMachine stores it as.
    static void IncreaseActorValue(LoveTesterMenu::LoveTesterButton* apButton, ...);
    // A click callback, typed as ButtonClickFunc, which InitMachine stores it as; the button argument is never read.
    static void IncreaseCurrentActorValue(LoveTesterMenu::LoveTesterButton* apButton, ...);
    // The all-done button's click callback: closes the menu when every point is spent. Typed as ButtonClickFunc, which
    // InitMachine stores it as; the button argument is never read.
    static void AllDoneButtonClick(LoveTesterMenu::LoveTesterButton* apButton, ...);

    Tile* pTiles[8];
    LOVE_TESTER_MENU_STATE eState;
    // The SPECIAL selected with the controller on the LTMS_SUMMARY page.
    LOVE_TESTER_MENU_STATE eXBoxIndexState;
    float fFrameAnimTimer;
    bool bAnimFinished;
    uint8_t padding55[3];
    int32_t iAvailablePoints;
    // The machine's models, as the model loader returns them (IsNode is checked before attaching nodes to them).
    NiAVObject* pDynamicGeometry;
    NiAVObject* pCabinetGeometry;
    // InitMachine fills LightBulbGlows. LightBulbs is an array of text items, and each element's nThreshold is read.
    BSSimpleArray<LoveTesterTextItem*> LightBulbs;
    BSSimpleArray<LoveTesterLightBulbGlows*> LightBulbGlows;
    LoveTesterPage Pages[9];
    LoveTesterPage GlobalPage;
    // Texture smart pointers; LoadButtonAndTextImages passes them by reference.
    NiPointer<NiSourceTexture> spAVValues[11];
    NiPointer<NiSourceTexture> spPointsText[4];
    NiPointer<NiSourceTexture> spNextPageButtonImage[2];
    NiPointer<NiSourceTexture> spPreviousPageButtonImage[2];
    NiPointer<NiSourceTexture> spOpenCloseBookButtonImage[2];
    NiPointer<NiSourceTexture> spAVLeftArrowButtonImage[2];
    NiPointer<NiSourceTexture> spAVRightArrowButtonImage[2];
    NiPointer<NiSourceTexture> spOnOffImage[2];

    static inline LoveTesterMenu* pInstance;
    static NiPointer<NiCamera> sp3DCamera;
};

// The actor-value buttons' text-item array.
using LoveTesterTextItemArray = BSSimpleArray<LoveTesterMenu::LoveTesterTextItem*>;
using LoveTesterButtonArray = BSSimpleArray<LoveTesterMenu::LoveTesterButton*>;
using LoveTesterLightBulbGlowsArray = BSSimpleArray<LoveTesterMenu::LoveTesterLightBulbGlows*>;

// The backward page-turn sequence names ("AimUp" ... "TurnLeft"), one per state from LTMS_START on: DoIdle and TurnPage
// index it by eState.
using LoveTesterAnimations_t = char* [8];
inline LoveTesterAnimations_t LoveTesterAnimations{const_cast<char*>("AimUp"),     const_cast<char*>("Aim"),
                                                   const_cast<char*>("Holster"),   const_cast<char*>("JumpLand"),
                                                   const_cast<char*>("JumpLoop"),  const_cast<char*>("JumpStart"),
                                                   const_cast<char*>("TurnRight"), const_cast<char*>("TurnLeft")};

// Which sequence table DoIdle and TurnPage read: TurnPage stores 1 when turning forward and 0 when turning back.
// Initialised to 1.
inline bool bLoveTesterAnimForward = true;

// The forward page-turn sequence names ("Forward" ... "FastRight"), one per state from LTMS_STRENGTH on; it is indexed
// by eState, one entry below its start.
using LoveTesterReverseAnimations_t = const char* [8];
inline LoveTesterReverseAnimations_t LoveTesterReverseAnimations{"Forward",  "Backward",    "Left",
                                                                 "Right",    "FastForward", "FastBackward",
                                                                 "FastLeft", "FastRight"};
