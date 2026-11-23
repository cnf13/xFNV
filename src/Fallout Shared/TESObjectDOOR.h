#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSOpenCloseForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESMagicCasterForm.h"
#include "Fallout Shared/TESMagicTargetForm.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESScriptableForm.h"

#include <stdint.h>

class Actor;
class DoorLock;
class MobileObject;
class TESObjectREFR;
class TESSound;

// A door base form (DOOR).
class TESObjectDOOR : public TESBoundAnimObject,
                      public TESFullName,
                      public TESModelTextureSwap,
                      public TESScriptableForm,
                      public TESMagicCasterForm,
                      public TESMagicTargetForm,
                      public BGSDestructibleObjectForm,
                      public BGSOpenCloseForm {
public:
    TESObjectDOOR();
    ~TESObjectDOOR() override;

    void InitializeData() override;
    void ClearData() override;
    bool Load(TESFile* apFile) override;
    void Save() override;
    void InitItem() override;
    void Copy(TESForm* apCopy) override;
    bool Compare(TESForm* apCompare) override;
    bool Activate(
        TESObjectREFR* apItemActivated,
        TESObjectREFR* apActionRef,
        bool abIdFlag,
        TESBoundObject* apObjecttoGet,
        int32_t aicount
    ) override;
    NiAVObject* Clone3D(TESObjectREFR* apRequester) override;

    using TESBoundObject::Clone3D;
    using TESForm::Save;

    // Whether the door slides rather than swings: flag 0x10 of the byte at +0x84.
    bool IsSlidingDoor();
    // Returns the looping sound (pLoopSound).
    TESSound* GetLoopSound();
    bool IsRandomDoor();
    void ClearRandomTeleportList();
    bool IsHidden();
    bool IsMinimalUse();
    void SetMinimalUse(bool abVal);
    bool IsAutomatic();
    void HandlePlayerTeleport(TESObjectREFR* apActivatedDoor);
    bool RandomlyTeleportsTo(TESForm* apSpace);
    TESObjectREFR* FindRandomTeleportTarget(TESObjectREFR* apStartDoor, TESObjectREFR* apActionRef);

    // Whether apActor may pass apLock without unlocking it: a follower of the player through a teleport door, a
    // trespasser, or whoever the lock lets through.
    static bool
    CanActorIgnoreLock(DoorLock* apLock, Actor* apActor, TESObjectREFR* apDoorRef, bool abActivate, bool abMovement);
    // The overload for a door reference: builds a DoorLock from apDoorRef, asks the overload
    // above with it, then destroys it.
    static bool CanActorIgnoreLock(TESObjectREFR* apDoorRef, Actor* apActor, bool abActivate, bool abMovement);
    static void AutoDoorCallback(NiAVObject* apDoor3D);
    static void DoorTeleportPlayerArrivalCallback(uintptr_t aiArrivalData);
    static void PlayerPullFollower();
    static void LinkRandomTeleportDoors(TESObjectREFR* apDoor1, TESObjectREFR* apDoor2);

    enum DOOR_FLAGS : uint8_t {
        FLAG_AUTOMATIC = 0x2,
        FLAG_HIDDEN = 0x4,
        FLAG_MINIMAL_USE = 0x8,
        FLAG_SLIDING = 0x10,
    };

    TESSound* pOpenSound;
    TESSound* pCloseSound;
    TESSound* pLoopSound;
    uint8_t cFlags;
    BSSimpleList<TESForm*> RandomTeleports;
};

inline MobileObject* pstaticdoor;
inline bool bStaticWasInInterior;

extern SettingT<GameSettingCollection> gs_sRandomDoorTeleportFailureMessage;

extern SettingT<GameSettingCollection> gs_fRandomDoorDistance;

extern SettingT<GameSettingCollection> gs_fAutoDoorActivateDistance;

extern SettingT<INIPrefSettingCollection> gs_bSaveOnTravel_GamePlay;
