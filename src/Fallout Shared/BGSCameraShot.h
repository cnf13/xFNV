#pragma once

#include "BSMain/Setting.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESImageSpaceModifiableForm.h"
#include "Fallout Shared/TESModel.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class NiNode;
class TESObjectREFR;

// A camera shot form (CAMS).
class BGSCameraShot : public TESForm, public TESModel, public TESImageSpaceModifiableForm {
public:
    // What the camera does during the shot.
    enum CAM_ACTION : uint32_t {
        CA_SHOOT = 0,
        CA_FLY = 1,
        CA_HIT = 2,
        CA_ZOOM = 3,
        CA_COUNT = 4,
    };

    // The object a shot's camera is placed at or aimed at.
    enum CAM_OBJECT : uint32_t {
        CO_ATTACKER = 0,
        CO_PROJECTILE = 1,
        CO_TARGET = 2,
        CO_COUNT = 3,
    };

    // Bits of CAMERA_SHOT_DATA::iFlags.
    enum : uint32_t {
        POS_FOLLOWS_TARGET = 0x1,
        ROT_FOLLOWS_TARGET = 0x2,
        DONT_FOLLOW_BONE = 0x4,
        FIRST_PERSON_CAMERA = 0x8,
        NO_TRACER = 0x10,
        START_AT_TIME_ZERO = 0x20,
    };

    // The shot's data (DATA).
    struct CAMERA_SHOT_DATA {
        // Swaps every field read from a big-endian file.
        void Endian();

        CAM_ACTION eCameraAction;
        CAM_OBJECT eLocation;
        CAM_OBJECT eTarget;
        uint32_t iFlags;
        float fPlayerTimeMult;
        float fTargetTimeMult;
        float fGlobalTimeMult;
        float fMaxTime;
        float fMinTime;
        float fTargetPercentBetweenActors;
    };

    // The shot's data as older files store it, without the target time multiplier and the target percentage.
    struct OLD_CAMERA_SHOT_DATA {
        // Swaps every field read from a big-endian file.
        void Endian();

        CAM_ACTION eCameraAction;
        CAM_OBJECT eLocation;
        CAM_OBJECT eTarget;
        uint32_t iFlags;
        float fPlayerTimeMult;
        float fGlobalTimeMult;
        float fMaxTime;
        float fMinTime;
    };

    BGSCameraShot();
    ~BGSCameraShot() override;

    void InitializeData() override;
    bool Load(TESFile* apFile) override;
    void Save() override;
    void InitItem() override;
    void Copy(TESForm* apCopy) override;
    bool Compare(TESForm* apCompare) override;
    using TESForm::Save;

    float GetTargetTimeMult();
    // Returns Data.eCameraAction.
    CAM_ACTION GetCameraAction();
    bool GetPosFollowsTarget();
    bool GetRotFollowsTarget();
    bool GetIsFirstPersonCamera();
    bool IsInitialized();
    bool IsStartAtTimeZero();
    // Releases the shot's camera.
    void FreeCamera();
    bool GetFollowsBone();
    bool GetNoTracer();
    // Returns spLocationNode's object.
    NiAVObject* GetLocationNode();
    // Returns spTargetNode's object.
    NiAVObject* GetTargetNode();
    // Returns spImagespaceTarget's object.
    NiAVObject* GetImagespaceTarget();
    void SetCameraLocation(NiAVObject* apLocation);
    void InitCamera(TESObjectREFR* apTargetRef, int32_t aiCameraZoom);
    void SetCameraTarget(NiAVObject* apTarget);
    void Update(float afTime);
    NiPoint3 GetTargetPos();

    CAMERA_SHOT_DATA Data;
    NiPointer<NiAVObject> spLocationNode;
    NiPointer<NiAVObject> spTargetNode;
    TESObjectREFR* pTargetRef;
    NiPointer<NiNode> spCameraNode;
    NiPointer<NiAVObject> spImagespaceTarget;
    int8_t cCameraHasRotation;
    bool bCameraTooClose;
    bool bCameraIsKillCam;
};

inline NiPoint3 kVATSCameraAngle{};

// The game settings BGSCameraShot.cpp defines.
extern SettingT<GameSettingCollection> fVATSCameraDollyMin;
extern SettingT<GameSettingCollection> fVATSCameraDollyTime;
extern SettingT<GameSettingCollection> fVATSCameraEffectVolume;
extern SettingT<GameSettingCollection> fVATSCameraCutAwayDistance;
inline bool bDollyToNewPos;
inline bool bPlayCameraZoomSounds;
inline float fDollyStartTime;
inline float fDollyPercentage;
inline float g_fVATSCameraDollyTime;
extern NiPoint3 dollyStartingPos;
extern NiPoint3 currentCameraPos;
extern NiPoint3 dollyDelta;
