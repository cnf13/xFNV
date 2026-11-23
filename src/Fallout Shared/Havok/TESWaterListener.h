#pragma once

#include "BSCore/BSSimpleArray.h"
#include "BSHavok/bhkWaterListener.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/Havok/BGSZoneTargetListener.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"

class hkpRigidBody;
class PlaceableWaterGroup;
class WaterZone;

float NiSign(float fValue);

using NiTMapHkpRigidBodyBool = NiTMap<hkpRigidBody*, bool>;

// A reference queued to leave a water zone: the reference, its rigid body, the water reference, its zone and the
// collidable.
struct QueuedRemoval {
    TESObjectREFR* pTarget;
    bhkRigidBody* pRB;
    TESObjectREFR* pWaterRef;
    WaterZone* pWZone;
    const hkpCollidable* pCollidable;
};

// A body of water the water listener tracks: its zone entry plus the water's height and placeable water group.
class WaterZone : public ZoneEntry {
public:
    WaterZone(TESObjectREFR* apRef, const hkpCollidable* apCol);

    // Returns apTarget's entry, with apCol added to its collidables.
    TargetEntry* AddTarget(TESObjectREFR* apTarget, const hkpCollidable* apCol, uint32_t aiFlags) override;

    // Drops apCol from apTarget's entry, and the entry when no collidable is left.
    void RemoveCollidable(TESObjectREFR* apTarget, const hkpCollidable* apCol);

    float fWaterHeight;
    PlaceableWaterGroup* pGroup;
};

// A reference in water: its target entry plus the collidables of the water bodies it touches.
class WaterTargetEntry : public TargetEntry {
public:
    WaterTargetEntry(TESObjectREFR* apRef, const hkpCollidable* apCol);

    // The entry of the next node in the list, or null at its end.
    WaterTargetEntry* GetNextEntry();

    BSSimpleArray<hkpCollidable const*, 1024> CollidableList;
};

// The Havok water listener: tracks water collisions and rigid bodies underwater, and reports the player's
// forced underwater state to the rest of the engine.
class TESWaterListener : public bhkWaterListener, public BGSZoneTargetListener {
public:
    TESWaterListener();
    TESWaterListener(float afWaterLevel);
    ~TESWaterListener() override;

    void entityRemovedCallback(hkpEntity* apEntity) override;
    void Update(float afTime) override;
    bool contactPointAddedCallback(hkpRootCdPoint& CPoint) override;
    bool contactPointRemovedCallback(hkpRootCdPoint& CPoint) override;
    bhkRigidBody* GetRB(hkpEntity* apEntity) override;
    // a random value between minus and plus the range
    float GetRandom(float afSize) override;

    virtual bool collidableEnterCallback(const hkpCollidable* collidableA, const hkpCollidable* collidableB);
    virtual bool collidableLeaveCallback(const hkpCollidable* collidableA, const hkpCollidable* collidableB);
    ZoneEntry* CreateZoneEntry(TESObjectREFR* apRef, const hkpCollidable* apCol) override;

    // Processes the queued removals for apSpecificReference, or for every reference when it is null.
    void HandleQueuedRemovals(TESObjectREFR* apSpecificReference, bool abCheckBound, bool abCancel);
    // Takes apQR's reference out of its water zone.
    void RemoveReferenceFromZone(QueuedRemoval* apQR);
    // Stores afWaterHeight as the water level of grid cell (auX, auY).
    void SetWaterHeight(float afWaterHeight, uint32_t auX, uint32_t auY);
    double GetWaterHeightForCoord(int32_t aiX, int32_t aiY);
    // Floats phkBody in water whose surface is at fwaterHeight (Havok units) over afTime: buoyancy against gravity,
    // at a point off the centre of mass for long bodies, then the submerged damping; splashes on entering or leaving.
    // Returns 1 on entering the water, 2 on leaving it, else 0.
    int32_t ProcessRigidBody(hkpRigidBody* phkBody, float fwaterHeight, float afTime, bool abPlaceableWater);
    // Plays the splash of apBody at the game-unit water height afWaterHeight.
    void PlaySplashEffects(bhkRigidBody* apBody, float afWaterHeight);
    // Whether phkBody is near enough to the player and close enough to its water's surface (written to
    // fwaterHeight) to float; deactivates a far one.
    bool ShouldProcessRigidBody(
        hkpRigidBody* phkBody,
        hkVector4& positionPlayer,
        float fdistanceLODSquared,
        WaterZone* apZone,
        float& fwaterHeight
    );
    void RemoveWaterZoneForReference(TESObjectREFR* apWaterRef);

    // The water listener, created on first use.
    static TESWaterListener* GetExteriorListener();
    static void ClearExteriorListener();
    // Reads the water height of every loaded exterior cell into the grid, and the grid's base cell.
    static void RefreshExteriorWaterHeights();

    // Finds apWater's zone entry.
    static WaterZone* FindWater(TESObjectREFR* apWater);
    // Whether the player is forced underwater, set by scripts through TESWaterSystem.
    static bool GetUnderwater();
    static void operator delete(void* apMemory);

    float fSplashTimer;
    float* pfWaterLevels;
    float fHighestWaterLevel;
    uint32_t uiCellArrayWidth;
    int32_t iBaseCellNumberX;
    int32_t iBaseCellNumberY;
    // The zone removals waiting until their references leave the water (HandleQueuedRemovals).
    NiTList<QueuedRemoval*> QueuedRemovals;

    // The bodies ProcessRigidBody found underwater this update.
    static NiTMapHkpRigidBodyBool RigidBodyUnderwaterMap;
};

inline TESWaterListener* g_TESWaterListener;

// Floats ProcessRigidBody reads (0.0 in
// the image): the buoyancy used between half and three quarters submerged, and the lowest depth fraction that still
// gets the negative buoyancy.
inline float fSubmergedMidBuoyancy;
inline float fSubmergedLowDepth;

// The game settings TESWaterListener.cpp defines.
extern SettingT<INISettingCollection> bAutoWaterUsesHavokShapes;
