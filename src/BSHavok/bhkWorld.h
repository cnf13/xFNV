#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSCore/NewOverloads.h"
#include "BSHavok/ahkpWorld.h"
#include "BSHavok/bhkAction.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkConvert.h"
#include "BSHavok/bhkEntityListener.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkTaskletManagerCallback.h"
#include "BSHavok/bhkWorldNS.h"
#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastInput.h"
#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantomListener.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

class BGSAcousticSpaceListener;
class bhkMultithreadedSystem;
class bhkNiCollisionObject;
class bhkWaterListener;
class bhkWorldObject;
class CFilter;
class hkpAction;
class hkpAllRayHitCollector;
class hkpClosestRayHitCollector;
class hkpCollidable;
class hkpConstraintInstance;
class hkpContactListener;
class hkpEntity;
class hkpPhantom;
class hkpRigidBody;
class hkpWorldObject;
class hkVisualDebugger;
class NiAVObject;
class NiNode;
class NiPoint3;
class TESWaterListener;

// A ray pick: the ray (its end point is calculated from the start and kLength when kLength is not zero), the collision
// filter, the collectors the hits go to and the result. Not polymorphic.
class bhkPickData : public hkpWorldRayCastInput, public hkpWorldRayCastOutput {
public:
    bhkPickData();

    // The end point of the ray.
    const hkVector4& GethkTo();
    // Calculates the end point of the ray from its start point and length.
    void CalcTo();
    // Copies arFrom into the ray's start point.
    void SetFrom(const hkVector4& arFrom);
    void SetAllCollector(hkpAllRayHitCollector* apCollector);
    void SetClosestCollector(hkpClosestRayHitCollector* apCollector);
    bool GetPickFailed();
    NiAVObject* GetNiAVObject();
    // The end point of the ray in game units, written to arTo.
    void GetTo(NiPoint3& arTo);
    // True when the ray has a length (kLength is not zero within 0.001).
    bool QLength();
    // kLength.
    hkVector4& GethkLength() {
        return hkLength;
    }
    // The ray's start point.
    hkVector4* GethkFrom();
    // The projectiles pass their pick cache's name.
    void SetCache(char* apCache) {
        pCache = apCache;
    }
    // pAllCollector.
    hkpAllRayHitCollector* GetAllCollector() {
        return pAllCollector;
    }
    // The ray's length vector in game units, written to arLength.
    void GetLength(NiPoint3& arLength);
    // Sets the ray's length vector; with a length the end point is calculated from it.
    void SetLength(const hkVector4& arLength);
    // Sets the ray's end point and clears its length.
    void SetTo(const hkVector4& arTo);
    // Sets the collision filter info of the ray.
    void SetFilter(CFilter aiFilter);
    // The rigid body the ray hit, or null.
    hkpRigidBody* GethkRigidBody();
    // Sets the ray's start point from a game-unit point.
    void NiSetFrom(const NiPoint3& arFrom);
    // Sets the ray's end point from a game-unit point.
    void NiSetTo(const NiPoint3& arTo);
    // Writes the ray's hit point, calculated by CalcHitPoint, in game units.
    void NiCalcHitPoint(NiPoint3& arPoint);
    // Sets the ray's length vector from a game-unit vector.
    void NiSetLength(const NiPoint3& arLength);
    // The ray's hit point in Havok units: the start interpolated towards the end by the hit fraction.
    void CalcHitPoint(hkVector4& arPoint);
    // The ray's start point in game units, written to arFrom.
    void GetFrom(NiPoint3& arFrom);
    // The ray's filter, built from its filter info into apResult.
    CFilter* GetFilter(CFilter* apResult);

    hkVector4 hkLength;
    char* pCache;
    hkpClosestRayHitCollector* pCloseCollector;
    hkpAllRayHitCollector* pAllCollector;
    bool bPickFailed;
};

inline bhkPickData::bhkPickData()
  : hkLength(hkVector4::getZero()), pCache(nullptr), pCloseCollector(nullptr), pAllCollector(nullptr),
    bPickFailed(false) {}

// m_to and kLength are reached by name rather than as neighbours of m_from, as the layout is not the exe's.
inline void bhkPickData::CalcTo() {
    if (QLength()) {
        m_to.setAdd4(m_from, hkLength);
    }
}

inline bool bhkPickData::QLength() {
    return hkLength.equals3(hkVector4::getZero(), 0.001f) == 0;
}

// An entity listener that also hears about phantoms: the Havok side of a trap trigger.
class bhkTrapListener : public bhkEntityListener, public hkpPhantomListener {
public:
    bhkTrapListener();
    ~bhkTrapListener() override;

    // DeletingDestruct's free (ourFree); both bases declare one.
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
};

inline bhkTrapListener::~bhkTrapListener() {}

// The creation info a bhkWorld is built from: the Havok world's cinfo with the serializable tag base after it (at
// +0xF0, where bhkSerializable::pInfo points).
struct bhkWorldCinfo : public hkSerializableCinfo, public hkpWorldCinfo {
    bhkWorldCinfo();
    ~bhkWorldCinfo() override;

    static void SetSimulationType(uint32_t aiType);
    // Deleting returns the block to the Havok heap, of the size in m_memSizeAndFlags.
    static void operator delete(void* apMemory) {
        hkReferencedObject::deleteObject(apMemory);
    }

    static inline int32_t eDefaultSimType = 3;
    // The default world gravity, -98.1 in Havok units.
    static inline float fDefaultGravityZ = -98.100006f;
};

// The game's physics world: the wrapper around the Havok world, with the batches of entities, phantoms, actions and
// constraints that are added to or removed from it a few at a time, and the listeners the game hangs off it.
class bhkWorld : public bhkSerializable {
public:
    // The main thread's Havok stack memory (0x60000 bytes), aligned to 16 by InitSystem.
    using StackBuffer = char[0x60000];
    // What a recursive walk over a node's collision objects does to each of them, and the parameters of one step of the
    // walk.
    using OBJECTACTION = bhkWorldNS::OBJECTACTION;
    using enum bhkWorldNS::OBJECTACTION;
    using ObjectRecData = bhkWorldNS::ObjectRecData;

    // A collidable the debug display draws in a color.
    struct DebugColor {
        hkpCollidable* pCollidable;
        uint32_t uiColor;
    };

    // The per-action handler of a recursive walk; the engine keeps one per OBJECTACTION.
    using RecFunction = void (*)(bhkNiCollisionObject*, ObjectRecData&);
    using RecFunctionTable = RecFunction[OA_TOTAL];

    bhkWorld(const bhkWorldCinfo& arInfo);
    ~bhkWorld() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    // a jump to bhkRefObject::SethkObject
    void SethkObject(hkReferencedObject* aphkObject) override;
    void Link(bool abAttach) override;
    const hkpWorld* GethkWorld() const override;
    hkpWorld* GethkWorldRw() override {
        return static_cast<hkpWorld*>(phkObject);
    }
    void KillCinfo(bool abCreated) override;
    uint32_t QCinfoSize() const override {
        return 0xF0;
    }
    void Init(const hkSerializableCinfo& arInfo) override;
    hkSerializableCinfo& CreateCinfo(bool& arCreated) override;

    // Steps the simulation; true when it ran.
    virtual bool Update();

    virtual bool PickObject(bhkPickData& arPickData);
    virtual bool HasActiveItems();
    virtual bool AddObjects(NiAVObject* apObject, bool abRecurse, bool abNotify, uint32_t aiGroup, bool abForce);
    virtual bool AddObjectsAndMoveController(NiAVObject* apObject, NiAVObject* apRoot);
    virtual void SetDebugDisplay(NiAVObject* apRoot, bool abTF);

    // Removes apConstraint from the world.
    int8_t RemoveConstraint(hkpConstraintInstance* apConstraint);
    // Queues apEntity's removal from the world.
    bool RemoveEntity(hkpEntity* apEntity);
    // Adds apAction to the Havok world. Returns nothing.
    void Add(hkpAction* apAction);
    // Takes apAction out of the Havok world.
    bool RemoveAction(hkpAction* apAction);
    // Fits the solver's substeps, tau and damping to this frame's step time.
    void ScaleSolverInfo();
    // Ticks every entity listener by afDelta (none for a zero delta) while the world has its Havok world.
    void UpdateListeners(float afDelta);
    // Ticks the game's actions by afDelta and drops those no longer in a Havok world.
    void UpdateActions(float afDelta);
    // Steps the visual debugger and draws the queued debug colors.
    void UpdateDebugger();
    // Applies the queued debug colors while the visual debugger runs, then empties the queue.
    void UpdateDebugColor();
    // Flush the batched removals of entities (at most 3000), phantoms (at most 200) and actions (at most 100).
    void RemoveBatchedEntities();
    void RemoveBatchedPhantoms();
    void RemoveBatchedActions();
    void RemoveBatchedConstraints();
    // Removes one frame's share of the queued entity removals: at most 3000, iEntityBatchRemoveRate of them.
    void RemoveBatchedEntitiesPartial();

    bool QDebugDisplay();
    void SetEnabled(bool abTF);
    // Draws the world object's collidable in aiColor while the debug display is on.
    void SetDebugObjectColor(const bhkWorldObject* apObject, uint32_t aiColor);
    // Turns the Havok visual debugger on or off.
    void SetVisualDebugger(bool abTF);
    // Writes the Havok world to apFileName as a snapshot.
    bool SaveSnapshot(const char* apFileName);
    // Count nested batching of entity adds or removals.
    void StartBatchingAdds();
    void StartBatchingRemovals();
    // Closes one level of add batching and flushes the batches at the last; abAllowPartial adds only one frame's
    // share of the entities.
    void StopBatchingAdds(bool abAllowPartial);
    // Closes one level of removal batching.
    void StopBatchingRemovals(bool abAllowPartial);
    bool IsBatchingAdds();

    // Adds the entities to the Havok world in one go.
    void AddEntities(hkpEntity** apEntityA, uint32_t auiNumEntities);
    // Flush the batched entities (at most 3000), phantoms (at most 200), actions and constraints into the world.
    void AddBatchedEntities();
    void AddBatchedPhantoms();
    void AddBatchedActions();
    void AddBatchedConstraints();

    // The Havok world, for callers that do not care that it is a virtual lookup.
    ahkpWorld* GetahkpWorld() const;
    // Writes the world's origin in game units to arOrigin (HK2NI of Origin).
    void GetOrigin(NiPoint3& arOrigin);

    // Moves the world's origin to a point in game units.
    void SetOrigin(const NiPoint3& arOrigin);

    // Moves the world's origin to a point in Havok units.
    void SethkOrigin(const hkVector4& arOrigin);

    // The Havok world, null when this is null.
    ahkpWorld* operator*() const;

    // The threading-check marks around a write to the Havok world; empty in the game's build.
    void MarkForWrite();
    void UnMarkForWrite();

    // Adds the wrapped object to this world; false for no object.
    bool Add(bhkSerializable* apObj);
    // Adds the constraint to the world, or to the batch while adds are batched.
    void Add(hkpConstraintInstance* apConstraint);
    // Removes the phantom immediately or queues it while removals are batched.
    bool Remove(hkpPhantom* apPhantom);

    const hkVector4& GetGravity();
    // The threading-check marks around a read of the Havok world; empty in the game's build, so both are one function.
    void MarkForRead();
    void UnMarkForRead();
    // Adds a Havok entity or phantom to the world, or to the batch while adds are batched.
    void Add(hkpWorldObject* apObject);

    // Registers the listener with the Havok world and remembers it as the one the game reads back.
    void AddEntityListener(bhkEntityListener* apListener);
    void AddContactListener(hkpContactListener* apListener);
    void AddWaterListener(bhkWaterListener* apListener);
    void AddTrapListener(bhkTrapListener* apListener);
    void AddAcousticSpaceListener(BGSAcousticSpaceListener* apListener);
    bhkEntityListener* QAcousticSpaceListener();
    bhkWaterListener* QWaterListener();
    // pTrapListener.
    bhkTrapListener* QTrapListener();
    void ProcessBatchRemovals(bool abAllowPartial);
    // Takes rigid bodies, phantoms and constraints alike (TESObjectREFR::~TESObjectREFR passes the phantom,
    // bhkUtilFunctions::RemoveSpringConstraints a bhkConstraint); Remove is called through the vtable.
    char RemoveBody(bhkSerializable* apObject);
    void* GetWaterListener();
    void DeactivateAllIslands();

    // Sets up Havok once: the free-list memory system and the main thread's router (its stack in pStack), the base
    // system, the collision filter, the NiStream loaders of the bhk classes and the multithreaded system.
    static void InitSystem();
    static void QuitSystem();
    // The longest time a pick may take, in ms: the VATS or the normal setting.
    static float QMaxPickTime();
    static bool IsHavokDebug();
    static uint8_t QPickCountOverride();
    static void SetPickCountOverride(uint8_t abVal);
    static void ResetBatchProcessed();
    static void SetUpdateType(uint32_t aiType);
    static void SetAddBipedWhenKeyframed(bool ab);
    static void SetEntityBatchRemoveRate(uint32_t aiRate);
    static void SetChaseDeltaMult(float afMult);
    static void SetDebrisMaxVelocity(float af);
    // Gives the rigid bodies under apObject the motion aeType, recursively when abRecurse.
    static bool
    SetMotion(NiAVObject* apObject, hkpMotion::MotionType aeType, bool abRecurse, bool abForce, bool abAllowActivate);
    static void SetDebrisMinExtent(float af);
    static void SetMaxPickTimeNormal(float afTime);
    static void SetMaxPickTimeVATS(float afTime);
    static void SetRecFunction(
        bhkWorldNS::OBJECTACTION aeAction,
        void (*apFunc)(bhkNiCollisionObject*, bhkWorldNS::ObjectRecData&)
    );
    // Activates or deactivates the Havok bodies of apObject.
    static bool Activate(NiAVObject* apObject, bool abActivate, bool abRecurse, bool abForce);
    // The wrapping bhkWorld of a Havok world, null for no world.
    static bhkWorld* Getbhk(const hkpWorld* apWorld);
    static bool RemoveObjects(NiAVObject* apObject, bool abRecurse, bool abForce);
    // Adds the collision object's Havok body to arData's world, as the OA_ADD step of a recursive walk.
    static void AddObject(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData);
    // Runs apFunc on the Havok collision object of apObject, and of its subtree when arData asks to recurse.
    static void DoObjectRec(
        NiAVObject* apObject,
        bhkWorldNS::ObjectRecData& arData,
        void (*apFunc)(bhkNiCollisionObject*, bhkWorldNS::ObjectRecData&)
    );
    static bool UpdatePosition(NiAVObject* apObject, bool abRecurse, bool abForce);
    static bool KillHavok(NiAVObject* apObject, bool abRecurse, bool abForce);
    static void DoObjects(NiAVObject* apObject, bhkWorldNS::ObjectRecData& arData);
    // Runs apFunc over apObject's Havok collision objects when its BSX flags mark it HAVOK; true when they do.
    static bool DoObjects(
        NiAVObject* apObject,
        bhkWorldNS::ObjectRecData& arData,
        void (*apFunc)(bhkNiCollisionObject*, bhkWorldNS::ObjectRecData&)
    );

    // Flags the collision object as reset (flag 0x40) so the next update rebuilds its Havok transform. A handler of the
    // recursive walk, which ignores the walk's data.
    static void NI2HKObject(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData&);

    // Handlers of the recursive walk.
    static void SetMotionOnObject(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData);
    static void AdjustForceAddCount(bhkNiCollisionObject* apObject, bhkWorldNS::ObjectRecData& apData);
    // Sets or clears the collision object's flag 1 from arData's first value (OA_ENABLE).
    static void EnableObject(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData);
    // Takes the collision object's Havok body out of its world (OA_REMOVE).
    static void RemoveObject(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData);
    // Turns the collision of the collision object's Havok body on or off (OA_ENABLECOLLISION).
    static void DoEnableCollision(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData);
    // Activates or deactivates the collision object's rigid body (OA_ACTIVATE).
    static void ActivateObject(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData);
    // Sets or clears the rigid body's disabled flag (0x40) from arData's first value (OA_DISABLE).
    static void DoDisable(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData);
    // Counts apObject's blend collision objects' forced adds up (abIncrement) or down, over
    // its subtree with abRecurse.
    static bool AdjustForceAddCount(NiAVObject* apObject, bool abIncrement, bool abRecurse, bool abForce);
    // Splits the frame's time into the simulation steps to run (iSteps of fStepTime), carrying the rest over.
    static void SetDeltaTime(float afDeltaTime, bool abQuickSimulate, bool abSimulateEveryFrame);
    static void ClearPickData();
    static void SetVatsReplay(bool abValue);
    static bool Enable(NiAVObject* apObject, bool abEnable, bool abRecurse, bool abForce);
    static bool EnableCollision(NiAVObject* apObject, bool abEnable, bool abRecurse, bool abForce);
    static bool SetDisabled(NiAVObject* apObject, bool abDisabled, bool abRecurse, bool abForce);
    static bool SetGroup(NiAVObject* apObject, uint32_t aiGroup, bool abRecurse, bool abForce);
    static bool IsActive(NiAVObject* apObject, bool abRecurse, bool abForce);
    static bool QPickTimeOverMax();
    static bool ChangeLayer(NiAVObject* apObject, COL_LAYER aeTo, COL_LAYER aeFrom, bool abRecurse, bool abForce);
    static bool SetRestitution(NiAVObject* apObject, float afRestitution, bool abRecurse, bool abForce);
    static bool SetFriction(NiAVObject* apObject, float afFriction, bool abRecurse, bool abForce);

protected:
    // Gives the world object the collision group arData carries.
    static bool SetCollisionGroup(bhkWorldObject* apObject, bhkWorldNS::ObjectRecData& arData);

public:
    static void DebugDisplayObject(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData);
    // Sets the wrapped rigid body's restitution to arData.uDataA[0].f.
    static void DoSetRestitution(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData);
    static void DoSetFriction(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData);
    // The walk handler form of SetCollisionGroup: gives the collision object's world object the group in
    // arData.uDataA[0]. Its first argument is a collision object, as for every walk handler; it leaves no result its
    // callers read.
    static void SetCollisionGroup(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData);
    // Counts an active rigid body into arData.uDataA[0] and clears bRecurse.
    static void QActiveObject(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData);
    // Moves the wrapped object from layer arData.uDataA[0] (any layer when LAYER_UNIDENTIFIED) to layer
    // arData.uDataA[1] and updates its filter.
    static void ChangeLayerObject(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData);
    static void ShiftPosition(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData);

    // The game's heap (MemoryManager).
    static void* operator new(size_t aSize);
    static void operator delete(void* apMemory) {
        MemoryManager::Instance().Deallocate(apMemory);
    }

    static void KillHavokOnObject(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData);
    static void FixedToKeyframedObject(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData);

    hkVisualDebugger* pDebugger;
    bool bAdded;
    bool bEnabled;
    bool bDebugDisplay;
    uint32_t iBatchAdd;
    uint32_t iBatchRemove;
    bhkTrapListener* pTrapListener;
    TESWaterListener* pWaterListener;
    BGSAcousticSpaceListener* pAcousticSpaceListener;
    hkpEntity** pEntityBatch;
    uint32_t iEntityBatchCount;
    hkpPhantom** pPhantomBatch;
    int32_t iPhantomBatchCount;
    hkpAction** pActionBatch;
    uint32_t iActionBatchCount;
    hkpConstraintInstance** pConstraintInstanceBatch;
    int32_t iConstraintInstanceBatchCount;
    hkpEntity** pEntityRemoveBatch;
    uint32_t iEntityRemoveBatchCount;
    uint8_t padding54[0xC];
    hkVector4 Origin;
    hkArray<hkpEntityListener*> pEntityListenerA;
    hkArray<DebugColor> DebugColorA;
    float fOriginalTau;
    float fOriginalDamping;
    BSSimpleList<NiPointer<bhkAction>> ActionList;
    uint8_t padding98[8];

    static inline StackBuffer pStack{};
    static inline bool bVatsReplay;
    static NiRTTI ms_RTTI;
    static SettingT<INISettingCollection> bHavokDebug;
    static inline uint8_t bPickCountOverride;
    static inline uint8_t bFrameBatchProcessed;
    static inline int32_t iUpdateType;
    static inline uint8_t bAddBipedWhenKeyframed = 1;
    // The fHavokTauRatio setting, copied here every frame.
    static inline float fTauRatio = 1.0f;
    static inline float fDebrisMinExtent;
    static inline float fDebrisMaxVelocity;
    static inline float fChaseDeltaMultPerFrame = 0.05f;
    static inline float fMaxPickTime = 0.003f;
    static inline float fMaxPickTimeVATS = 0.03f;
    static inline int32_t iEntityBatchRemoveRate = 100;
    static RecFunctionTable pRecFunctionA;
    // The frame's simulation timing SetDeltaTime computes.
    static inline float fBSDeltaTime = 0.016f;
    static inline float fDeltaTime = 0.016f;
    static inline float fTimeRemainder;
    static inline float fStepTime;
    static inline uint32_t iSteps;
    static inline float fChaseDelta = 0.032f;
    // How many batched entities a partial flush adds per frame.
    static inline uint32_t iEntityBatchAddRate = 100;
    // The fMaxStepTime:HAVOK setting.
    static SettingT<INISettingCollection> fMaxStepTime;
    static inline bool bInitGroupDuringAdd = true;
    // Fixed bodies are made keyframed as they are added.
    static inline bool bNoFixedObjects;
    // The INI settings of the substepped (type 2) update.
    static SettingT<INISettingCollection> fTimePerSubStep;
    static SettingT<INISettingCollection> iMinNumSubSteps;
    // bFindContactPointsOnAdd:HAVOK: added entities collide at once.
    static SettingT<INISettingCollection> bFindContactPointsOnAddS;
    // The pick budget, reset every update.
    static inline float fCurrentPickTime;
    static inline uint32_t iCurrentPickCount;
    static inline bhkMultithreadedSystem* pMultithreadedSystem;
    // The game's collision filter the worlds share.
    static inline bhkCollisionFilter* pFilter;
    // The number of live worlds; the destructor decrements it.
    static inline uint32_t iCount;
};

inline bhkWorld::RecFunctionTable bhkWorld::pRecFunctionA{
    &bhkWorld::AddObject,
    &bhkWorld::RemoveObject,
    &bhkWorld::NI2HKObject,
    &bhkWorld::EnableObject,
    &bhkWorld::KillHavokOnObject,
    &bhkWorld::SetMotionOnObject,
    &bhkWorld::DebugDisplayObject,
    &bhkWorld::SetCollisionGroup,
    &bhkWorld::QActiveObject,
    &bhkWorld::ChangeLayerObject,
    &bhkWorld::FixedToKeyframedObject,
    &bhkWorld::ShiftPosition,
    &bhkWorld::ActivateObject,
    &bhkWorld::AdjustForceAddCount,
    &bhkWorld::DoEnableCollision,
    &bhkWorld::DoSetRestitution,
    &bhkWorld::DoSetFriction,
    &bhkWorld::DoDisable
};

// The game settings bhkWorld.cpp defines.
extern SettingT<INISettingCollection> iHWThread1;
extern SettingT<INISettingCollection> iHWThread2;
extern SettingT<INISettingCollection> iHWThread3;
extern SettingT<INISettingCollection> iHWThread4;
extern SettingT<INISettingCollection> iHWThread5;
extern SettingT<INISettingCollection> iHWThread6;

// HavokWorldProps holds the world's properties: a static of every file that includes this header, as in the original
// game.
namespace HavokWorldProps {
    static const float fGravitySC = 10.0f * -9.8100004196167;
} // namespace HavokWorldProps

// The Havok world's user data is the bhkWorld that wraps it (bhkWorld::Link sets it).
inline bhkWorld* bhkWorld::Getbhk(const hkpWorld* apWorld) {
    if (apWorld) {
        return const_cast<ahkpWorld*>(static_cast<const ahkpWorld*>(apWorld))->getUserData();
    }
    return nullptr;
}

inline void bhkWorld::AddEntityListener(bhkEntityListener* apListener) {
    ahkpWorld* pw = **this;
    if (pw) {
        MarkForWrite();
        pw->addEntityListener(apListener);
        UnMarkForWrite();
    }
    pEntityListenerA.pushBack(static_cast<hkpEntityListener*>(apListener));
}

inline float bhkWorld::QMaxPickTime() {
    if (bhkWorld::bVatsReplay) {
        return bhkWorld::fMaxPickTimeVATS;
    }

    return bhkWorld::fMaxPickTime;
}

// The threading-check mark is empty in the game's build, so MarkForWrite and UnMarkForWrite compile to one function.
inline void bhkWorld::MarkForWrite() {
    ahkpWorld* pw = **this;
    if (pw) {
        pw->markForWrite();
    }
}

inline void bhkWorld::UnMarkForWrite() {
    ahkpWorld* pw = **this;
    if (pw) {
        pw->unmarkForWrite();
    }
}

// The Havok side of the arrow targets: an entity listener that does not implement Update itself (TESArrowTargetListener
// does).
class bhkArrowTargetListener : public bhkEntityListener {
public:
    bhkArrowTargetListener();
    ~bhkArrowTargetListener() override;

    static void operator delete(void* apMemory);
};

inline bhkArrowTargetListener::~bhkArrowTargetListener() {}

inline void bhkArrowTargetListener::operator delete(void* apMemory) {
    ourFree(apMemory);
}

// The read mark is empty in the game's build: only the Havok world's lookup remains.
inline void bhkWorld::MarkForRead() {
    ahkpWorld* pw = static_cast<ahkpWorld*>(const_cast<hkpWorld*>(GethkWorld()));
    if (pw) {
        pw->markForRead();
    }
}

inline void bhkWorld::UnMarkForRead() {
    ahkpWorld* pw = static_cast<ahkpWorld*>(const_cast<hkpWorld*>(GethkWorld()));
    if (pw) {
        pw->unmarkForRead();
    }
}

inline bhkArrowTargetListener::bhkArrowTargetListener() : bhkEntityListener(0) {}

inline void bhkWorld::AddContactListener(hkpContactListener* apListener) {
    ahkpWorld* pw = **this;
    if (pw) {
        MarkForWrite();
        pw->addContactListener(apListener);
        UnMarkForWrite();
    }
}

// The listener hears entities through the entity listeners and phantoms through its hkpPhantomListener base.
inline void bhkWorld::AddTrapListener(bhkTrapListener* apListener) {
    AddEntityListener(apListener);
    ahkpWorld* pw = **this;
    if (pw) {
        MarkForWrite();
        pw->addPhantomListener(apListener);
        UnMarkForWrite();
    }
    pTrapListener = apListener;
}

inline bhkTrapListener* bhkWorld::QTrapListener() {
    return pTrapListener;
}

// The Havok world's gravity, or zero without a world. The world is looked up before the read mark.
inline const hkVector4& bhkWorld::GetGravity() {
    ahkpWorld* pWorld = static_cast<ahkpWorld*>(const_cast<hkpWorld*>(GethkWorld()));
    MarkForRead();
    const hkVector4& kGravity = pWorld ? pWorld->getGravity() : hkVector4::getZero();
    UnMarkForRead();
    return kGravity;
}

inline const hkVector4& bhkPickData::GethkTo() {
    CalcTo();
    return m_to;
}

inline void bhkPickData::SetFrom(const hkVector4& arFrom) {
    m_from = arFrom;
}

inline void bhkPickData::SetAllCollector(hkpAllRayHitCollector* apCollector) {
    pAllCollector = apCollector;
    pCloseCollector = nullptr;
}

inline void bhkPickData::SetClosestCollector(hkpClosestRayHitCollector* apCollector) {
    pCloseCollector = apCollector;
    pAllCollector = nullptr;
}

inline bool bhkPickData::GetPickFailed() {
    return bPickFailed;
}

inline void bhkPickData::SetLength(const hkVector4& arLength) {
    hkLength = arLength;
}

inline void bhkPickData::SetTo(const hkVector4& arTo) {
    SetLength(hkVector4::getZero());
    m_to = arTo;
}

inline void bhkPickData::SetFilter(CFilter aiFilter) {
    m_filterInfo = aiFilter;
}

inline bhkTrapListener::bhkTrapListener() : bhkEntityListener(0) {}

inline void bhkPickData::GetTo(NiPoint3& arTo) {
    HK2NI(arTo, GethkTo());
}

inline hkVector4* bhkPickData::GethkFrom() {
    return &m_from;
}

inline void bhkPickData::GetFrom(NiPoint3& arFrom) {
    HK2NI(arFrom, *GethkFrom());
}

// The hit point: the ray's start interpolated towards its end by the hit fraction.
inline void bhkPickData::CalcHitPoint(hkVector4& arPoint) {
    alignas(16) hkSimdReal kFraction(m_hitFraction);
    arPoint.setInterpolate4(m_from, m_to, kFraction);
}

inline void bhkPickData::GetLength(NiPoint3& arLength) {
    HK2NI(arLength, GethkLength());
}

// The ray's filter, built from its filter info through CFilter's one-field constructor.
inline CFilter* bhkPickData::GetFilter(CFilter* apResult) {
    ::new (apResult) CFilter(m_filterInfo);
    return apResult;
}
