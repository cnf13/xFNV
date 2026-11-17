#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSHavok/ahkpWorld.h"
#include "BSHavok/bhkAction.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkTaskletManagerCallback.h"
#include "BSHavok/bhkWorldNS.h"
#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

class BGSAcousticSpaceListener;
class bhkEntityListener;
class bhkMultithreadedSystem;
class bhkNiCollisionObject;
class bhkPickData;
class bhkTrapListener;
class bhkWaterListener;
class bhkWorldObject;
class hkpAction;
class hkpCollidable;
class hkpConstraintInstance;
class hkpContactListener;
class hkpEntity;
class hkpEntityListener;
class hkpPhantom;
class hkpWorldObject;
class hkVisualDebugger;
class NiAVObject;
class NiNode;
class NiPoint3;
class TESWaterListener;

// The creation info a bhkWorld is built from: the Havok world's cinfo with the serializable tag base after it (at
// +0xF0, where bhkSerializable::pInfo points).
struct bhkWorldCinfo : public hkSerializableCinfo, public hkpWorldCinfo {
	bhkWorldCinfo();
	~bhkWorldCinfo();

	static void SetSimulationType(uint32_t aiType);
	// Deleting returns the block to the Havok heap, of the size in m_memSizeAndFlags.
	static void operator delete(void* apMemory) { hkReferencedObject::deleteObject(apMemory); }

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

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	// a jump to bhkRefObject::SethkObject
	void SethkObject(hkReferencedObject* aphkObject) override;
	void Link(bool abAttach) override;
	const hkpWorld* GethkWorld() const override;
	hkpWorld* GethkWorldRw() override { return static_cast<hkpWorld*>(phkObject); }
	void KillCinfo(bool abCreated) override;
	uint32_t QCinfoSize() const override { return 0xF0; }
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
	static bool SetMotion(NiAVObject* apObject, hkpMotion::MotionType aeType, bool abRecurse, bool abForce,
		bool abAllowActivate);
	static void SetDebrisMinExtent(float af);
	static void SetMaxPickTimeNormal(float afTime);
	static void SetMaxPickTimeVATS(float afTime);
	static void SetRecFunction(bhkWorldNS::OBJECTACTION aeAction,
		void (*apFunc)(bhkNiCollisionObject*, bhkWorldNS::ObjectRecData&));
	// Activates or deactivates the Havok bodies of apObject.
	static bool Activate(NiAVObject* apObject, bool abActivate, bool abRecurse, bool abForce);
	// The wrapping bhkWorld of a Havok world, null for no world.
	static bhkWorld* Getbhk(const hkpWorld* apWorld);
	static bool RemoveObjects(NiAVObject* apObject, bool abRecurse, bool abForce);
	// Adds the collision object's Havok body to arData's world, as the OA_ADD step of a recursive walk.
	static void AddObject(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData);
	// Runs apFunc on the Havok collision object of apObject, and of its subtree when arData asks to recurse.
	static void DoObjectRec(NiAVObject* apObject, bhkWorldNS::ObjectRecData& arData,
		void (*apFunc)(bhkNiCollisionObject*, bhkWorldNS::ObjectRecData&));
	static bool UpdatePosition(NiAVObject* apObject, bool abRecurse, bool abForce);
	static bool KillHavok(NiAVObject* apObject, bool abRecurse, bool abForce);
	static void DoObjects(NiAVObject* apObject, bhkWorldNS::ObjectRecData& arData);
	// Runs apFunc over apObject's Havok collision objects when its BSX flags mark it HAVOK; true when they do.
	static bool DoObjects(NiAVObject* apObject, bhkWorldNS::ObjectRecData& arData,
		void (*apFunc)(bhkNiCollisionObject*, bhkWorldNS::ObjectRecData&));

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
	static void SetCollisionGroup_00C67240(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData);
	// Counts an active rigid body into arData.uDataA[0] and clears bRecurse.
	static void QActiveObject(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData);
	// Moves the wrapped object from layer arData.uDataA[0] (any layer when LAYER_UNIDENTIFIED) to layer
	// arData.uDataA[1] and updates its filter.
	static void ChangeLayerObject(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData);
	static void ShiftPosition(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& arData);

	// Frees the aligned block: the alignment padding byte sits in front of the object.
	static void operator delete(void* apMemory) {
		uint8_t* pMemory = static_cast<uint8_t*>(apMemory);
		MemoryManager_Singleton.Deallocate(pMemory - pMemory[-1]);
	}

	static void KillHavokOnObject(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& a2);
	static void FixedToKeyframedObject(bhkNiCollisionObject* apCollision, bhkWorldNS::ObjectRecData& a2);

	hkVisualDebugger* pDebugger;
	bool bAdded;
	bool bEnabled;
	bool bDebugDisplay;
	uint8_t padding17;
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

inline bhkWorld::RecFunctionTable bhkWorld::pRecFunctionA{&bhkWorld::AddObject, &bhkWorld::RemoveObject,
	&bhkWorld::NI2HKObject, &bhkWorld::EnableObject, &bhkWorld::KillHavokOnObject, &bhkWorld::SetMotionOnObject,
	&bhkWorld::DebugDisplayObject, &bhkWorld::SetCollisionGroup_00C67240, &bhkWorld::QActiveObject,
	&bhkWorld::ChangeLayerObject, &bhkWorld::FixedToKeyframedObject, &bhkWorld::ShiftPosition,
	&bhkWorld::ActivateObject, &bhkWorld::AdjustForceAddCount, &bhkWorld::DoEnableCollision,
	&bhkWorld::DoSetRestitution, &bhkWorld::DoSetFriction, &bhkWorld::DoDisable};

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
