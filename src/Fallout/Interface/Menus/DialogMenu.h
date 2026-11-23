#pragma once

#include "BSAudio/BSSoundHandle.h"
#include "BSMain/Setting.h"
#include "BSMenu/Menu.h"
#include "Fallout/Interface/ListBox.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"

#include <stdint.h>

class ImageSpaceModifierInstanceDOF;
class MenuTopic;
class MenuTopicManager;
class MobileObject;
class TESObjectREFR;
class TESTopic;
class TESTopicInfo;

// The reference the open dialogue menu talks to, or null without one.
TESObjectREFR* GetDialogMenuParterRef();

// The dialogue menu. Only its statics are declared; its members are not.
class DialogMenu : public Menu {
public:
    // The dialogue's state: the value LoadNextScreen returns and DoClick, DoIdle and SetPause store in eDialogState,
    // which ePausedState saves across a pause.
    enum DialogState : int32_t {
        eCameraZoomIn = 0,
        eLoadSpeech = 1,
        eSpeechIdle = 2,
        ePlaySpeech = 3,
        eCameraZoomOut = 4,
        eServiceFadeOut = 5,
    };

    // Values of eGoodbyeState.
    enum GoodbyeState : int32_t {
        eInDialogue = 0,
        eNaturalGoodbye = 1,
        eForceGoodbye = 2,
        eInvalidChoiceList = 3,
    };

    DialogMenu();
    ~DialogMenu() override;

    int32_t GetClass() override {
        return 0x3F1;
    }
    void AttachTileByID(int32_t aiID, Tile* apTile) override;
    void DoClick(int32_t aiID, Tile* apTarget) override;
    bool DoGamepad(int32_t aiButton, float afValue) override;
    void DoIdle() override;

    MobileObject* GetSpeaker();
    bool IsWellFormedMenu();
    // Pauses or resumes the speech and moves the dialogue state to or from the paused state.
    void SetPause(bool abPause);

protected:
    // Forces the goodbye after a goodbye info; a topic that is the second conversation topic ends naturally.
    void SetGoodbyeState(MenuTopic* apTopic);

public:
    void LoadTopicsList();
    DialogMenu::DialogState LoadNextScreen(bool abShowChallengePrefix);

    // Starts the dialogue service.
    static void ServiceStart();
    // The reference the player is talking to, or null without a dialogue menu.
    static MobileObject* GetDialogSpeaker();
    // True while the open dialogue menu is paused.
    static bool GetPaused();
    // Opens the dialogue menu of apRef talking about apTopic to apTarget and returns its tile.
    static Tile* Create(MobileObject* apSpeaker, TESTopic* apGreetTopic, MobileObject* apSpeakas);
    static int32_t GetClassStatic() {
        return 0x3F1;
    }
    static bool IsDialogActive();
    // Ends the service the player picked through apTopic.
    static void ServiceEnd(TESTopic* apTopic);
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
    static void Close();
    static void DebugResetSaidOnceFlags();

    DialogState eDialogState;
    GoodbyeState eGoodbyeState;
    Tile* pTiles[4];
    // The topic list box.
    ListBox<int32_t> lbResponseList;
    MenuTopicManager* pTopicManager;
    BSSoundHandle hSpeechFile;
    // The speaker, the current topic ID, the response and timing state and the depth-of-field state.
    MobileObject* pSpeaker;
    // The current topic, as "<editor id> (<info form id>)", formatted into 128 bytes.
    char strCurrTopicID[128];
    bool bResponseShown;
    float fElapsedSecs;
    uint32_t uiIgnoreInputStartTime;
    // The depth of field the dialogue camera uses: its depth, range and strength.
    NiPoint2 DOFDepth;
    NiPoint2 DOFRange;
    NiPoint2 DOFStrength;
    float fPercentZoomed;
    ImageSpaceModifierInstanceDOF* pDepthOfFieldInstance;
    bool bIsIn3rdPerson;
    bool bPaused;
    DialogState ePausedState;
    // The service menu is open, a tutorial message is up, and the service menu's load is pending.
    bool bInServiceMenu;
    bool bShowingTutorial;
    bool bDoServiceLoad;

    static inline bool bDialogActive;
    static inline DialogMenu* pMe;
    // The time a line without a voice file stays up, in seconds.
    static inline float fUseTimerNoVoice;
    static inline float fPackagePercentZoom = 100.0f;
    static inline int32_t iUT_ShowingText;
    static inline int32_t iUT_DialogVisible;
    static inline TESTopicInfo* pLastClickedInfo;
};

inline int32_t g_dialogMenu_TraitCurrentValue;
inline int32_t iStopSpeechStartTime;

extern SettingT<INIPrefSettingCollection> gs_bDialogueSubtitles_GamePlay;
