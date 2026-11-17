#pragma once

#include "BSAudio/BSSoundHandle.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSMenu/Menu.h"
#include "Fallout Shared/ActorValue.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

#include <stdint.h>

class ItemChange;
class NiControllerManager;
class NiGeometry;

// Moves and scales the surgery menu's 3D player by the three offsets; despite its name it is not about the terminal
// menu.
void SetTerminalMenuPos(float afX, float afY, float afScale);

class SurgeryMenu : public Menu {
public:
    // How badly a limb is wounded.
    enum SURGERY_MENU_WOUND_DAMAGE : int32_t {
        SMWD_NONE = 0,
        SMWD_LIGHT = 1,
        SMWD_MODERATE = 2,
        SMWD_HEAVY = 3,
        SMWD_SEVERE = 4,
    };

    // The menu's animations (SetAnimation).
    enum SURGERY_MENU_ANIMATION : int32_t {
        SMA_NONE = 0,
        SMA_GAME_START = 1,
        SMA_MORPHINE = 2,
        SMA_STIMPAK = 3,
    };

    // The menu's tiles (the tile IDs GetIndex maps, pTiles' indices).
    enum TileIndex : int32_t {
        SM_BACKGROUND = 0,
        SM_CLOSE_BUTTON = 1,
        SM_AUTOATTEMPT_BUTTON = 2,
        SM_SCAR_LOCATION = 3,
        SM_STIMPAK = 4,
        SM_MORPHINE = 5,
        SM_TOOL = 6,
        SM_HITPOINTS = 7,
        SM_LIMB_DAMAGE = 8,
        SM_MEDICINE_SKILL = 9,
        SM_PAIN_METER = 10,
        SM_FACE = 11,
        SM_SMOKE = 12,
        SM_CAUTERIZE = 13,
        SM_SWITCH_LEFT_BUTTON = 14,
        SM_SWITCH_RIGHT_BUTTON = 15,
        SM_WOUND = 16,
        SM_LIMB_METER = 17,
        SM_NUM_TILES = 18,
    };

    // Defined below, after LimbWound, which holds a list of them.
    struct SequenceInfo;

    // A wound the surgery menu tracks on one limb. Only some of its members are declared.
    struct LimbWound {
        // Restores the limb.
        void ResetLimb();

        // The wound's skin sequences, which ResetLimb walks and clears.
        BSSimpleList<SequenceInfo*>* xSequenceInfo;
        ActorValue::Index eLimb;
        SURGERY_MENU_WOUND_DAMAGE eDamage;
        // Set when the limb's surgery is done; ResetLimb clears it.
        bool bResetLimb;
    };

    // One skin sequence of a wound and the bones it drives, allocated by PrepareSequences.
    struct SequenceInfo {
        NiControllerSequence* Sequence;
        // The bones whose interpolators the sequence keeps.
        BSSimpleList<NiAVObject*>* xBoneList;
        // The sequence's time, passed to NiControllerSequence::SetTimePassed.
        float fSequenceTime;
    };

    SurgeryMenu();
    ~SurgeryMenu() override;

    void AttachTileByID(int32_t, Tile*) override;
    void DoClick(int32_t, Tile*) override;
    void DoIdle() override;
    bool DoKey(int32_t) override;
    int32_t GetClass() override;

    // The menu is not read.
    SurgeryMenu::TileIndex GetIndex(int32_t aiID);
    // The wound on the menu's current damaged part, or null.
    LimbWound* GetCurrentLimbWound();
    int32_t GetIndexFromActorValue(ActorValue::Index aeLimb);
    SurgeryMenu::SURGERY_MENU_WOUND_DAMAGE GetLimbDamage(int32_t aiLimbCondition);
    // Plays the menu's tool and wound animations.
    void UpdateAnimations();
    // Moves the tool tile; abBurning while it burns.
    void UpdateToolLocation(bool abBurning);
    // Burns the wound under the tool.
    void BurnFlesh();
    // True when the cursor is over the wound.
    bool IsCursorInFlesh();
    // Updates the player's face for the pain.
    void UpdateFace();
    // Starts animation aeAnimation.
    void SetAnimation(SURGERY_MENU_ANIMATION aeAnimation);
    // Swaps the skin geometry of limb aeOldLimb for aeNewLimb's.
    void SwitchSkinGeometry(ActorValue::Index aeOldLimb, ActorValue::Index aeNewLimb);
    // Shows the current limb's condition; returns it.
    int32_t ShowLimbCondition();
    // The current limb's healed fraction read from its skin sequences.
    float GetLimbPercentFromSequences();
    // Sets the current limb's skin sequences to afPercent.
    void SetLimbSequences(float afPercent);
    // Burns the flesh under the tool; returns 0 inside the wound, else 100.
    float UpdateFlesh();
    void PlaceToolInWindow(int32_t aiX, int32_t aiY);
    void PlaceDecal(NiPoint2 aToolLocation, bool abInWound);

    static void Draw3DElements();
    static void SetCursorPos(int32_t aiX, int32_t aiY);
    static int32_t GetClassStatic();
    // Resets every limb's surgery state while the menu exists.
    static void ResetAllLimbs();
    // Resets the wound on limb aeLimb while the menu exists and is not on top.
    static void ResetLimb(ActorValue::Index aeLimb);
    // Closes the menu.
    static void Close();
    // Builds the wound's skin sequences for apNode.
    static void PrepareSequences(NiAVObject* apNode);
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
    static void Shutdown3DElements();
    static int32_t
    SortSequenceByCursorDistance(SurgeryMenu::SequenceInfo* apObject1, SurgeryMenu::SequenceInfo* apObject2);
    static int32_t PointInPoly(double (*pgon)[2], int32_t numverts, double* point);

    SURGERY_MENU_ANIMATION eAnimation;
    Tile* pTiles[18];
    NiPoint2 ToolLocation;
    NiPoint2 Resistance;
    float fToolMovement;
    uint32_t uiWoundBurnTime;
    uint32_t uiWoundCloseTime;
    bool bBurning;
    bool bToolBuriedInFlesh;
    uint8_t padding92[2];
    BSSoundHandle BurningFleshSoundHand;
    BSSoundHandle BurningWoundSoundHand;
    float fPainPercent;
    uint32_t uiPainTimer;
    uint32_t uiHitPointTimer;
    uint32_t uiDamageHealthTime;
    ItemChange* pStimPaks;
    ActorValue::Index eCurrentDamagedPart;
    SURGERY_MENU_WOUND_DAMAGE eCurrentDamageLevel;
    NiPointer<NiObject> pSkinGeometry[6];
    NiAVObject* pFrameGeometry;
    // The PC types do not establish the controller pointee type.
    NiControllerManager* pSkinControllerManager;
    // The face geometry PlaceDecal picks on and attaches the wound's decals to.
    NiGeometry* pScar;
    float fFrameAnimTimer;
    NiPointer<NiAVObject> spPlayerLights;
    NiPointer<NiRefObject> spPlayer3DParent;
    NiPoint3 ptSavedPlayerTranslation;
    NiMatrix3 mtxSavedPlayerRotation;
    NiMatrix3 mtxSavedBip01Rotation;
    float fSavedPlayerScale;
    NiPoint3 ptSavedBip01Translation;
    bool bPlayer3DCulled;
    bool bPipboyLightEffectCulled;
    uint8_t padding15E[2];

    static inline SurgeryMenu* pSurgeryMenu;
    // The wounds of the limbs.
    static BSSimpleList<LimbWound*> xLimbWounds;
    // The player's 3D root, which the menu moves and scales. The PC types do not establish its pointee class; the
    // menu's code treats it as a NiAVObject.
    static NiPointer<NiAVObject> spPlayer3D;
    static NiPointer<NiCamera> sp3DCamera;
    static NiPointer<NiNode> sp3DSkin;
};

// The menu tile's trait for the current limb's injury percent.
inline int32_t iUT_InjuryPercent;

// The game settings SurgeryMenu.cpp defines.
extern SettingT<INISettingCollection> fSurgeryPlayerZoom;
extern SettingT<INISettingCollection> fSurgeryPlayerDefaultVPos;
extern SettingT<INISettingCollection> fSurgeryPlayerDefaultScale;
extern SettingT<INISettingCollection> fSurgerySkinHPos;
extern SettingT<INISettingCollection> fSurgeryPlayerDefaultHPos;
extern SettingT<INISettingCollection> fSurgerySkinVPos;
extern SettingT<INISettingCollection> fSurgerySkinZoom;
extern SettingT<INISettingCollection> fSurgerySkinScale;
