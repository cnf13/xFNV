#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/ActionPoints.h"
#include "Fallout Shared/ActorValue.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

#include <stdint.h>

class Actor;
class BGSCameraPath;
class BGSCameraShot;
class ExtraDataList;
class HitData;
class ImageSpaceModifierInstanceForm;
class ImageSpaceModifierInstanceRB;
class NiLight;
class Projectile;
class ShadowSceneLight;
class TESBoundObject;
class TESIdleForm;
class TESObjectREFR;
struct VATS_COMMAND;

// A queued VATS action.
class VATSQueuedAction {
public:
    // Whether the action type is one of the two grenade/mine throws (7 and 8).
    static bool IsActionGrenadeOrMine(ActionPoints::Action aeAction);
};

// The VATS manager: the mode, the camera shots and the target light.
class VATS {
public:
    // The VATS mode; the engine's VATS_MODE_ENUM.
    enum VATS_MODE_ENUM : int32_t {
        VATS_MODE_NONE = 0,
        VATS_TARGET_SELECT = 1,
        VATS_MENU_MODE = 2,
        VATS_SCAN_MODE = 3,
        VATS_PLAYBACK = 4,
        VATS_MODE_COUNT = 5,
    };

    VATS();
    ~VATS();

    void QueueAction(VATS_COMMAND* apCommand);
    // The point at afPercentage along the target.
    NiPoint3 GetTargetPercentagePos(float afPercentage);
    // The action being run: the first queued command, or null.
    VATS_COMMAND* GetCurrentAction();
    // The last queued command, or null.
    VATS_COMMAND* GetLastAction();
    // Whether apCommand, or the first queued command when apCommand is null, is a melee or unarmed attack.
    bool IsMeleeCommand(VATS_COMMAND* apCommand);
    // True while a VATS mode other than none is active.
    bool IsInVats();
    // The light that highlights the VATS target.
    ShadowSceneLight* GetVATSLight();
    VATS_MODE_ENUM GetMode();
    Projectile* GetFirstProjectile() {
        return pFirstProjectile;
    }
    Projectile* GetLastProjectile() {
        return pLastProjectile;
    }
    // Points the camera shot's projectile-bound location and target at apProjectile's 3D, then records it as the
    // first projectile.
    void SetFirstProjectile(Projectile* apProjectile);
    // Records apProjectile as the last one fired and moves the camera on to a shot for the current action.
    void SetLastProjectile(Projectile* apProjectile);
    // Updates the camera for apCommand.
    void UpdateCurrentCameraAction(VATS_COMMAND* apCommand);
    // Switches to the next camera shot.
    bool UseNextCamera();
    // Switches to aeMode.
    void SetMode(VATS_MODE_ENUM aeMode, bool bForce);
    float GetPlayerUpdateMult();
    float GetTargetUpdateMult();
    // Drops apRef as the VATS target.
    void ClearTarget(TESObjectREFR* apRef);
    // Ends the VATS playback.
    void QuitVATSPlayback(bool abForceQuit, bool abFinishCommandPlayback);
    // bFirstVatsAttack (the byte at +0x38).
    bool GetFirstAttack() {
        return bFirstVatsAttack;
    }
    // Sets bFirstVatsAttack.
    void SetFirstAttack(bool bFirst) {
        bFirstVatsAttack = bFirst;
    }
    TESIdleForm* GetMeleeAttack();
    BGSCameraShot* GetCurrentCamera();
    float GetCurrentCameraMinTimer();
    // Frees the queued commands and releases the melee attack idle.
    void ClearCommandList();
    // The queued commands' list.
    BSSimpleList<VATS_COMMAND*>* GetCommandList();
    // Clears the command list and drops the camera shot and both imagespace modifiers.
    void ClearData();
    void IncrementKillCount();
    void EvaluateCommand(VATS_COMMAND* apCommand);
    void InitPlayerDeath();
    void Update();
    void FreeHeadAction();
    void UpdateMeleeAttack(VATS_COMMAND* apCommand);
    void UpdateVATSLight(TESObjectREFR* apVictim);
    void RemoveLastAction(ActionPoints::Action& aeAction, bool& abIronSights);
    void QuitVATSPlayback_0(TESBoundObject* apObject, ExtraDataList* apExtra);
    NiPoint3 GetCamshotTargetPos();
    bool GetLineOfSight(VATS_COMMAND* apCommand);
    void SetVATSLight(NiLight* apLight, bool abRemoveOld);
    void AppendCamTarget(TESObjectREFR* apTarget, int32_t aiLimb);
    uint32_t GetCount();

    // The queued VATS commands.
    BSSimpleList<VATS_COMMAND*> commandList;
    VATS_MODE_ENUM eMode;
    // The camera shots to pick from (a camera path's ShotList).
    BSSimpleList<BGSCameraShot*>* pCameraList;
    BGSCameraShot* pCamera;
    // How long the camera shot has run and its minimum time.
    float fCameraTimer;
    float fCameraMinTimer;
    Projectile* pFirstProjectile;
    Projectile* pLastProjectile;
    TESIdleForm* pMeleeAttack;
    ImageSpaceModifierInstanceForm* pImagespaceMod;
    ImageSpaceModifierInstanceRB* pTransitionImagespaceMod;
    bool bAiming;
    NiPointer<ShadowSceneLight> spVATSLight;
    bool bFirstVatsAttack;
    int32_t iKillCount;
    // The weapon VATS unequipped and its extra data.
    TESBoundObject* pUnEquipWeapon;
    ExtraDataList* pUnEquipExtra;

    // The Mysterious Stranger actor while his VATS kill runs.
    static inline Actor* pStranger;
    // The Miss Fortune actor while her VATS effect runs.
    static inline Actor* pMissFortune;
};

// One queued VATS action: what the action is, the target and limb, and its outcome and damage.
struct VATS_COMMAND {
    VATS_COMMAND();
    ~VATS_COMMAND();

    void Initialize();

    static void operator delete(void* apMemory);

    ActionPoints::Action eAction;
    bool bSuccess;
    bool bIronSights;
    bool bStranger;
    bool bParalyzingPalm;
    uint8_t cFireShots;
    uint8_t cFakeFireShots;
    TESObjectREFR* pTarget;
    ActorValue::Index eLimb;
    HitData* pHitData;
    float fTotalDamage;
    float fCumulativeDamage;
    float fActionPoints;
    bool bMissFortune;
};

// The actor whose VATS attack is running.
inline Actor* pVATSAttacker;

// The VATS manager; pVATS is an object despite the prefix.
extern VATS pVATS;

// The running fraction of a VATS burst that TESObjectWEAP::Fire adds each shot's burst-time ratio to, wrapping it down
// by one.
inline float fVATSShotBurstFraction;

// The reference VATS is targeting.
inline TESObjectREFR* pVATSTarget;

// The camera action of the VATS command being played; it is compared signed.
inline int32_t eVATSCameraAction;
inline BGSCameraPath* pForceCameraPath;
inline BGSCameraPath* pForceDefaultCameraPath;
inline int32_t eCameraZoom;
inline bool bVATSFlushCommandWhenAnimActionIsDone;
inline bool bSwitchTo3rdPerson;
inline float fVATSPlaybackTimer;
inline float fVATSCommandTimer;
inline BGSCameraPath* pCameraPath;
inline float fEndVATSDelay;
inline char bLastCommandBlock;

// VATS::Update's function-local statics.
inline int32_t icameraIndex;

// The command the camera list was last chosen for: Update stores the head command there and compares its eAction
// (+0) with ACTION_TARGET_KILL_SHOT.
inline VATS_COMMAND* pcameraCommand;

extern SettingT<INISettingCollection> gs_fVatsLightColorR_VATS;

extern SettingT<INISettingCollection> gs_fVATSLightLevelMax_VATS;

extern SettingT<INISettingCollection> gs_fVATSLightElevation_VATS;

extern SettingT<INISettingCollection> gs_fVATSLightDistance_VATS;

extern SettingT<INISettingCollection> gs_fVATSLightLevelMin_VATS;

extern SettingT<INISettingCollection> gs_fVatsLightColorG_VATS;

extern SettingT<INISettingCollection> gs_fVatsLightColorB_VATS;

extern SettingT<INISettingCollection> gs_fVATSLightAngle_VATS;
