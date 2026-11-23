#pragma once

#include "BSMain/Setting.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/ProcessArray.h"

#include <stdint.h>

class Actor;
class BaseProcess;
class bhkCharacterController;
class TESIdleForm;
class TESPackage;

int32_t get_iFrameCounter();

// One frame's movement request that MobileObject::Move hands to the character controller (bhkCharacterController's
// Move).
struct MoveData {
    MoveData();

    float fTime;
    NiPoint3 Angle;
    NiPoint3 Displacement;
    // The movement flags MobileObject::Move passes in.
    uint32_t iMoveDirection;
    float fCurrMoveSpeed;
    bool bVisible;
};

// A reference that moves and runs AI: the base of Actor and of projectiles. It owns the process that drives it.
class MobileObject : public TESObjectREFR {
public:
    MobileObject();
    ~MobileObject() override;

    uint16_t GetSaveSize(uint32_t aiFlags) override;
    void SaveGame(uint32_t aiFlags) override;
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
    void LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) override;
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void InitLoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) override;
    void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void FinishInitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) override;
    void Revert(uint32_t aiFlags) override;
    void Revert(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void SetDelete(bool abDeleted) override;
    bool IsMobileObject() override {
        return true;
    }
    void UpdateSoundCallBack() override;
    bool DetachHavok() override;
    void InitHavok() override;
    NiAVObject* Load3D(bool abBackgroundLoading) override;
    NiPoint3 GetBoundMin() override;
    NiPoint3 GetBoundMax() override;
    void SetRunsInLow(bool abFlag) override;
    void MoveHavok(bool abForceRec) override;
    void SetParentCell(TESObjectCELL* apCell) override;

    virtual bool MovetoHigh();
    virtual bool MovetoLow() {
        return false;
    }
    virtual bool MovetoMiddleLow() {
        return false;
    }
    virtual bool MovetoMiddleHigh() {
        return false;
    }
    virtual bhkCharacterController* Move(float afDeltaTime, NiPoint3& arDeltaMove, uint32_t aiMoveDir);
    virtual bhkCharacterController* Jump();
    virtual void SetAvoidanceDisabled(bool) {}
    virtual void Process(float afDelta);
    virtual bool ChangeProcessLevel();
    virtual void UpdateInDialogue(float afDelta, DialogueResponse* pResponse, bool abChangeAnimationEndVoice);
    virtual void UpdateAnimation(float afDelta);
    virtual bool GetEVPBuffered() {
        return false;
    }
    virtual void EvaluatePackage() {}
    virtual bool IsInCombat(bool) {
        return false;
    }
    virtual void SetDialoguewithPlayer(bool abFlag);
    virtual bool InitiateDialogue(
        MobileObject* apTarget,
        PackageLocation* pPackLoc,
        PackageLocation* pPackSecondLoc,
        bool bHeadTrack,
        bool bMove,
        bool bflag,
        TESTopic* pTopic,
        bool abTargetContinueMoving,
        bool abTargetMoveTowardStarter
    );
    virtual bool InitiateDialogue(TESPackage* pDialoguePackage);
    virtual float SpeakSoundFunction(
        const char* apSoundFile,
        BSSoundHandle& aSoundHand,
        DIALOGUE_EMOTION aeEmotiontype,
        uint32_t aiEmotionValue,
        int32_t aiStringLength,
        TESIdleForm* apResponseSpeakerIdle,
        TESIdleForm* apResponseListenerIdle,
        TESObjectREFR* apTarget,
        bool abSetEmotion,
        bool ab2D,
        bool abQueue,
        bool abLip,
        bool abUseIdleManager
    );
    virtual void EndDialogue();
    virtual void SetRunOnceExtraPackage(TESPackage*, char) {}
    virtual bool HasEditorLocation() {
        return false;
    }
    virtual TESWorldSpace* GetEditorLocationWorld() {
        return nullptr;
    }
    virtual TESObjectCELL* GetEditorLocationInteriorCell() {
        return nullptr;
    }
    virtual NiPoint3 GetEditorLocationCoord();
    virtual bool DetachCharController();
    virtual bool RemoveCharController();
    virtual void SetPosition(const NiPoint3& arPos);
    virtual void GetLinearVelocity(NiPoint3& arVel);
    virtual void EvaluateDetection(float) {}
    virtual float GetDetectionTimer() {
        return 0.0f;
    }
    virtual void ReduceDetectionTimer() {}
    virtual float GetHeading(bool abIgnoreImmobile);
    virtual void SetHeading(float afAngle);
    virtual void SetHeading(TESObjectREFR* apTargetReference);
    virtual TESObjectREFR* GetCurrentTarget();
    virtual bool IsAtPoint(const NiPoint3& aPoint, float afRadius, bool abExpandRadius, bool abAlwaysTestHeight);
    virtual int32_t GetSpeakingEmotion() {
        return 0;
    }
    virtual void SetSpeakingEmotion(int32_t) {}
    virtual int32_t GetSpeakingEmotionValue() {
        return 0;
    }
    virtual void SetSpeakingEmotionValue(int32_t) {}
    virtual void SetSize(float afRadius);
    virtual float GetAcrobatics() {
        return 1.0f;
    }
    virtual bool IsDying() {
        return false;
    }
    virtual bool IsTransient() {
        return false;
    }
    virtual void SetDisplayGeometry(bool abTF);
    virtual void PutCreatedPackage(TESPackage* apPack, bool abTempPack, bool abIsaCreatedPackage);
    virtual void Update(float) {}
    virtual void CleanUpPointersOnDisable() {}
    virtual MobileObject* SetUpTalkingActivatorActor(MobileObject* apTarget, MobileObject*& aptalkingactivator);

    // Records that apTopicInfo was said today.
    void AddSayOnceTopicInfo(TESTopicInfo* apTopicInfo);
    void StopCurrentDialogue();
    // The process driving this object, or null when it has none.
    BaseProcess* GetCurrentAIProcess();
    // Stores apProcess as the current process.
    void SetCurrentAIProcess(BaseProcess* appross);

    // The level of the current process, PROCESS_TYPE_NONE without one.
    PROCESS_TYPE GetCurrentProcessType();

    // Records where the object is now as its package start location.
    void SetPackageStartLocationFromCurrentLocation();

    // The package the current process is running, or null.
    TESPackage* GetCurrentPackage();

    // The character controller of the current process, or null.
    bhkCharacterController* GetCharController();
    // Whether the character controller's current state is the in-air state.
    bool IsInAir();
    // Plays the sound form whose editor ID is apEditorName on the reference, placed at and following it for 3D sounds,
    // and returns its handle.
    BSSoundHandle PlaySoundByEditorName(const char* apEditorName, bool abLoop, uint32_t aiSoundType, bool abAllowCopy);
    // Writes the character controller's collision filter (an empty filter without one) to apResult.
    uint32_t* GetCollisionFilter(uint32_t* apResult);
    bool GetRunsInLow();

    // The activator this object is talking through.
    TESObjectREFR* GetTalkingActivator();
    void SetTalkingActivatorRef(TESObjectREFR* apRef);
    bool IsTalkingActivator();
    // Whether the 3D's "Bip01 NonAccum"
    // node is at most afMaxHeight above the character controller; true without a controller, 3D or node.
    bool IsNonAccumNearController(float afMaxHeight);
    // Whether the object is speaking dialogue.
    bool IsTalking();
    // Has the current process, if any, compute its last-processed time.
    void InitLastTimeProcessed();

    void SetSoundCallBackSuccess(bool abSuccess);
    // Drops the once-a-day topic infos whose day has passed.
    void UpdateTopicInfoSayOnce();

    // The pitch of the object's angle on its reference.
    float GetLooking();

    // Adds to the heading through the engine's GetHeading/SetHeading.
    void ModHeading(float afModAngle);
    void ModLooking(float afModAngle);

    // Destroys the current process and clears the pointer.
    void DeleteCurrentProcess();

    void SetChaseBip(bool abTF);
    PROCESS_TYPE GetDesiredProcessLevel();
    bool IsInDialoguewithPlayer();
    void HandleTextureUpgrade(bool abForceRecheck);
    uint32_t CheckHighDetail();
    // Who the object's next dialogue item is said to.
    MobileObject* GetDialogueItemTarget();
    void SetDialogueItemTarget(MobileObject* pref);
    // Sets the angle on the reference to arRot.
    void SetRotation(const NiPoint3& arRot);
    // Sets the pitch to afAngle, clamped to the looking limits (tighter while sitting).
    void SetLooking(float afAngle);
    // Clears the other mobile objects' pointers to this one.
    void CleanUpOtherMobileObjects(bool abMagicCaster);
    void SetSoundCallBack(char acCallBack);
    bool GetShouldUseEmotionAnimation();
    void SetTalkingActivator(bool abFlag);
    void SetShouldUseEmotionAnimation(bool abFlag);
    bool IsinDialogue();
    // Moves the object to process level aeLevel.
    bool SetProcessLevel(PROCESS_TYPE aeLevel);
    // Plays sound form aiSoundFormID with aiSoundType's flags, following the 3D when flag 2 is set, and returns its
    // handle; abAllowCopy is unused.
    BSSoundHandle PlaySound(uint32_t aiSoundFormID, bool abLoop, uint32_t aiSoundType, bool abAllowCopy);
    BSSoundHandle PlaySound_0(char* apSoundFileName, bool abLoop, uint32_t aiSoundType);
    float GetActualHeading();

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
    // Ends a say-to for the speaker whose form ID apContext holds.
    static void SayToCallBack(void* apContext, bool abSucceeded);
    // Sets sound callback 3 on the mobile object whose form ID apContext holds.
    static void DisplaySubTitleCallback(void* apContext, int32_t aiDuration);
    static void ChangeDialogueAnimation(void* apContext, bool abSucceeded);
    static void DialogueConversationCallBack(void* apContext, bool abSucceeded);
    static void SoundFileCallBack(void* apContext, bool abSucceeded);

    BaseProcess* pCurrentProcess;
    TESObjectREFR* pTalkingActivator;
    MobileObject* pDialogueItemTarget;
    float fUpdateTargetTimer;
    float fSayToFailVoiceTimer;
    bool bSpeakingDone;
    bool bTalkingToPlayer;
    bool bIgnoreChangeAnimationCall;
    bool bSoundFileDone;
    bool bVoiceFileDone;
    bool bTalkingActivator;
    bool bInTempChangeList;
    bool bDoNotRunSayToCallBack;
    int8_t cSoundCallBackSet;
    bool bSoundCallBackSuccess;
    bool bUseEmotion;
    bool bRunsInLow;

    static inline Actor* pTalkingToPlayer;
    // The bound box corners GetBoundMin and GetBoundMax return.
    static NiPoint3 boundMin;
    static NiPoint3 boundMax;
};

// The game settings MobileObject.cpp defines.
extern SettingT<INISettingCollection> fQuadrupedPitchMult;
extern SettingT<INISettingCollection> bBackgroundLoadLipFiles;
extern SettingT<INISettingCollection> fDBVoiceAttenuationIn2D;
inline int32_t iLastFrameCounter;

extern SettingT<INISettingCollection> gs_iHavokSkipFrameCountTEST_Havok;

extern SettingT<INISettingCollection> gs_fCharControllerWarpDistSqr_HAVOK;
