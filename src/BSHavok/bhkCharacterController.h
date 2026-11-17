#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSHavok/bhkCharacterListener.h"
#include "BSHavok/bhkCharacterProxy.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkShape.h"
#include "BSHavok/bhkShapePhantom.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"
#include "Havok/Source/Common/Base/Types/Physics/hkStepInfo.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/CharacterProxy/hkpCharacterProxyCinfo.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/StateMachine/hkpCharacterContext.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/StateMachine/hkpCharacterStateManager.h"
#include "Havok/Source/Physics/Utilities/CharacterControl/StateMachine/hkpSurfaceInfo.h"

#include <stdint.h>
#include <xmmintrin.h>

class bhkCachingShapePhantom;
class bhkCharacterControllerCinfo;
class bhkCharacterState;
class bhkRigidBody;
struct DamageImpactData;
class hkpCapsuleShape;
class hkpCharacterProxy;
class hkpPhantom;
class hkpRigidBody;
class hkpShape;
class hkpShapeCollection;
struct hkpSimplexSolverInput;
struct MoveData;
class NiAVObject;
class NiNode;

// The triangle collection of a shape (through a MOPP's child), or null.
const hkpShapeCollection* GetTriCollectionFromShape(const hkpShape* apShape);
bhkShape* MakeConvex(const NiPoint3& arTop, const NiPoint3& arBottom, float afRadius, float afStepHeight);
// Gives the top of a stilt capsule placed by arTransf.
void ExtractStiltCapsuleInfo(const hkpCapsuleShape& arCapShape, const hkTransform& arTransf,
	hkVector4& arCapsuleTopOut);

// What bhkCharacterController::Move reports.
enum MOVE_RESULTS : int32_t {
	MOVE_OK = 0,
	MOVE_ERROR = 1,
	MOVE_HIT = 2,
};

// The game's character controller: a Havok character proxy together with the state machine that decides how the
// character moves, the listener that watches what it touches and the description of the surface under it.
class bhkCharacterController : public bhkCharacterProxy,
							   public hkpCharacterContext,
							   public bhkCharacterListener,
							   public hkpSurfaceInfo {
public:
	// The collision shapes a character can have.
	enum SHAPE_TYPES : int32_t {
		ST_CAPSULE = 0x0,
		ST_CONVEX = 0x1,
		ST_NUMTYPES = 0x2,
	};

	enum CHARACTER_SIZE : int32_t {
		CS_NORMAL = 0x0,
		CS_SIZED = 0x1,
		CS_SIZING = 0x2,
	};

	// The behavior flags (bhkCharacterListener::iFlags).
	enum CHARACTER_FLAGS : uint32_t {
		QUADRUPED = 0x1,
		AT_LADDER = 0x2,
		TRY_STEP = 0x4,
		NO_FRICTION = 0x8,
		TEST_IT = 0x10,
		STUCK_QUAD = 0x20,
		HITSTEPS = 0x40,
		HITDAMAGE = 0x80,
		HITFLAGS = 0xC0,
		SUPPORT = 0x100,
		SUPPORT2 = 0x200,
		CANJUMP = 0x400,
		CHASEBIP = 0x800,
		FOLLOWRAGDOLL = 0x1000,
		JUMPING = 0x2000,
		NOTPUSHABLE = 0x4000,
		// The controller has a collision display node to look up; GetCollisionDebugNode clears it when it finds none.
		COLLISION_DEBUG = 0x8000,
		DEBUGDISP = 0x8000,
		CHECKSUPPORT = 0x10000,
		ON_ELEVATOR = 0x20000,
		FAR_AWAY = 0x40000,
		PROJECTILE = 0x80000,
		QUICK_SIMULATE = 0x100000,
		RECORD_HITS = 0x200000,
		WAIT_FOR_BATCH_ADD = 0x400000,
		ON_PLATFORM = 0x800000,
		ON_STAIRS = 0x1000000,
		CAN_PITCH = 0x2000000,
		CAN_ROLL = 0x4000000,
		NO_CHARACTER_COLLISIONS = 0x8000000,
		NOTPUSHABLE_PERMANENT = 0x10000000,
		POSSIBLE_PATH_OBSTACLE = 0x20000000,
	};

	using pfGetMaterialStairCallback_t = bool (*)(HavokMaterialType);

	bhkCharacterController(bhkCharacterControllerCinfo& arInfo);

	uint32_t QCinfoSize() const override { return 0xC0; }
	// Builds the controller's creation info from the proxy when it has none yet.
	hkSerializableCinfo& CreateCinfo(bool& arCreated) override;
	// Builds the controller from arInfo. A new virtual beside bhkSerializable's Init(const hkSerializableCinfo&), which
	// stays bhkCharacterProxy's.
	virtual void Init(bhkCharacterControllerCinfo& arInfo);
	using bhkCharacterProxy::Init;
	// Moves the character by one frame's request: MOVE_HIT when a HITFLAGS bit is set after a full move, else MOVE_OK
	// (MoveImmobile passes it on).
	virtual MOVE_RESULTS Move(MoveData& arData);
	virtual void UpdateCollisionBox();
	virtual void BuildDisplayGeometry(NiNode* apNode);
	void processConstraintsCallback(const hkpCharacterProxy* apProxy, const hkArray<hkpRootCdPoint>& arManifold,
		hkpSimplexSolverInput& arInput) override;
	void characterInteractionCallback(hkpCharacterProxy* apProxy, hkpCharacterProxy* apOtherProxy,
		const hkContactPoint& arContact) override;
	void objectInteractionCallback(hkpCharacterProxy* apProxy, const hkpCharacterObjectInteractionEvent& arInput,
		hkpCharacterObjectInteractionResult& arOutput) override;
	~bhkCharacterController() override;

	// The character controller this one bumped into.
	bhkCharacterController* GetBumpedCharacterController();
	// The body this controller bumped into, null for none.
	bhkRigidBody* GetBumpedBody();
	void SetShapeType(SHAPE_TYPES aeType);
	bhkCachingShapePhantom* GetShapePhantom();
	BSSimpleList<DamageImpactData*>* GetHurtfulHits();
	float QPitchAngle();
	float QRollAngle();
	float GetLODDistance();
	// The radius of the controller's capsule shape.
	float GetRadius();

	void SetWantState(hkpCharacterStateType aeType);
	void SetScale(float afScale);
	void SetPriority(uint32_t aiVal);

	// Makes the wanted state the current one, once, and forgets the wish.
	void UpdateState();

	hkpCharacterStateType GetState();
	bool QIsSwimming();
	// Whether the controller is in the flying state.
	bool QIsFlying();
	bool GetUseZ();

	bool QChaseBip();
	bool QHitDamage();
	bool QCanPitch();
	bool QCanRoll();
	void SetWaitForBatchAdd(bool abTF);
	void SetQuickSimulate(bool abTF);

	// Sets the water height, in game units, that the controller swims at.
	void SetWaterHeight(float afHeight);

	// Puts the character's phantom and collision box in a collision group.
	void SetCollisionGroup(uint32_t aiGroup);

	// Forgets the contact points the proxy has gathered.
	void ClearManifold();

	// Writes the shape phantom's collision filter (0 without one) to apResult.
	CFilter* GetCollisionFilter(CFilter* apResult) const;
	// The Bethesda world of the shape phantom's Havok world.
	bhkWorld* GetWorld();
	// The 3D of the shape phantom's game object, null without a phantom.
	NiAVObject* GetNiObject();
	// Sets the shape phantom's collision filter.
	void SetCollisionFilter(CFilter aiFilter);
	bool QSupported2();
	// Asks for the jumping state with a jump of afHeight game units.
	void Jump(float afHeight);

	void SetSize(float afRadius);
	void SetWorld(bhkWorld* apWorld);
	// Sets the shape phantom's capsule radius.
	void SetCapsuleRadius(float afNewRadius);
	// Sets the 3D the controller drives, or none.
	void SetNiObject(NiAVObject* apObject);
	void RemoveCollisionDebug();
	// Stores afHeight as fFallStartHeight.
	void SetStartingHeight(float afHeight);
	// Moves the proxy to arPos, given in game units.
	void SetPosition(const NiPoint3& arPos);
	// Moves the proxy to arPos, given in Havok units, less kRotCenter turned by the shape phantom's rotation.
	void SetPosition(const hkVector4& arPos);
	// Gives the proxy the velocity arVelocity, in game units.
	void SetLinearVelocityNi(const NiPoint3& arVelocity);
	// Sets the normal of the surface the character stands on.
	void SetSupportNormal(const hkVector4& arNormal);
	// Adds arVelocity to the character's movement for afTime seconds.
	void SetVelocityModifier(const NiPoint3& arVelocity, float afTime);
	// The "bhkColDisp" node under the shape phantom's first child node, once the debug
	// display has asked for it; clears the request and answers null when there is none.
	NiAVObject* GetCollisionDebugNode();
	// Reads the proxy's position, in game units, into arPos.
	void GetPosition(NiPoint3& arPos);
	// Reads the proxy's position in Havok units into arPos.
	void GetPosition_hk(hkVector4& arPos) const;
	float QFallStartHeight();
	// Binds apObject to aiKey as a property of the shape phantom's Havok object.
	void SetBSReference(uint32_t aiKey, void* apObject);
	bhkCharacterController::SHAPE_TYPES QShapeType() const;
	bhkCharacterController::CHARACTER_SIZE GetSize() const;
	void SetMOBObject(void* apObject) { SetBSReference(0x3E8, apObject); }
	// Sets or clears the projectile flag 0x80000 of the listener
	// part (+0x410).
	void SetIsProjectile(bool abTF);
	void SetFarAway(bool abTF) { bhkCharacterListener::SetFlag(0x40000, abTF); }
	float QFallTime() const;
	hkVector4* GetRotCenter();
	// Whether the listener's supported flag 0x100 is set, as 0 or 1.
	bool QSupported();
	// Sets the controller's level of detail from apObject's distance to aReferencePoint.
	void UpdateLODDistance(const NiPoint3& aReferencePoint, NiAVObject* pobject);
	const hkVector4& GetSupportNormal() { return hkSupportNorm; }
	void ClearHitDamage() { bhkCharacterListener::ClearFlag(0x80); }
	void SetSupported(bool abTF) { bhkCharacterListener::SetFlag(0x100, abTF); }
	int32_t GetSupportCount() const { return static_cast<int32_t>(iSupportCount); }
	void SetFallTime(float afFallTime);
	void SetFollowRagdoll(bool abTF);
	void SetSwimFloatHeight(float afHeight);
	void SetTrueState(hkpCharacterStateType aeType);
	void SetPitchMult(float afPitchMult);
	void ResetCalculatePitchTimer();
	void UpdateFalling();
	// Turns the shape phantom to arRot (normalized), tilted by the pitch about x and the roll about y where the
	// CAN_PITCH / CAN_ROLL flags allow.
	void ApplyPitchAndRoll(const hkQuaternion& arRot);
	int32_t GetOnStairs();
	int32_t CanJump();
	int32_t IsOnElevator();
	bhkCharacterState* GetCurrentState();
	float ApplyRotationModifier(float afOldRotation, float afDeltaTime);
	bhkRigidBody* GetSupportBody() const;
	uint32_t GetFilterGroup();
	float QWaterHeight() const;
	float QSwimFloatHeight() const;
	// Eases fPitchAngle and fRollAngle towards the slope under the character.
	void CalculatePitchAndRoll();
	// Finds support point aiFrontBack (0-3: back, front, left, right) into arPos.
	void FindSupportPoint(uint32_t aiFrontBack, hkVector4& arPos);
	// Builds the capsule shapes the creation info asks for.
	void InitShapes(bhkCharacterControllerCinfo& arInfo);
	// Clears the push the controller has gathered.
	void ClearPush();
	// Fills kFrontBack, the support probe points.
	void InitFrontBack();
	// Moves the radius towards fDestRadius over afTime.
	void ChangeSize(float afTime);
	// Forgets the hurtful hits of the last move.
	void ClearHurtfulHits();
	// Moves a projectile's controller by afTimeDelta seconds.
	void MoveAsProjectile(float afTimeDelta);
	// The controller's current shape.
	bhkShape* GetShape();
	void UnMarkWrite();
	int32_t QFarAway();
	void ApplyVelocityMod();
	bool IsStep(const hkpRootCdPoint& arPoint);
	float GetFallDistance() const;
	void MoveImmobile(MoveData& arData, bool abAllowFalling);
	bool SetShape(bhkShape* apShape, const hkVector4& a2);
	bhkCharacterController* Getbhk(hkpPhantom* apPhantom);
	void SetSupportMaterial(const hkpRootCdPoint& arManifold);
	bool CheckInsideOfObject();
	void SetRotationModifier(float a1, float a2);

	static void SetMinHurtfulMass(float afValue);
	static void SetMultipleStepSpeed(float afValue);
	static void SetGravFrictDampSpeed(float afSpeed);
	// Releases the default convex shape's reference (spDefaultConvexShape) and clears it.
	static void ReleaseDefaultShape();
	// The property aiKey of apPhantom (hkpWorldObject::getProperty), or null for a null phantom.
	static void* FindBSReference(uint32_t aiKey, hkpPhantom* apPhantom);
	// Frees with ourMemalignFree. Declared here also because the bases bhkCharacterProxy and bhkCharacterListener
	// each declare their own.
	static void operator delete(void* apMemory);
	static bool IsShapeConsideredPenetrating(const hkpShape* apShape, bool abPenetrating, bool abStairsMaterialOnly,
		uint32_t auiShapeKey);
	static bool IsHurtfulBody(hkpRigidBody* apBody);

	// Havok vectors: UpVec is passed to hkQuaternion::setAxisAngle, and ForwardVec and VelocityMod are each read and
	// written as one __m128.
	hkVector4 UpVec;
	hkVector4 ForwardVec;
	hkStepInfo StepInfo;
	hkVector4 OutVelocity;
	hkVector4 VelocityMod;
	hkVector4 Direction;
	hkpCharacterStateType eWantState;
	float fVelocityTime;
	float fRotMod;
	float fRotModTime;
	float fCalculatePitchTimer;
	float fAcrobatics;
	float fCenter;
	float fWaterHeight;
	float fJumpHeight;
	float fFallStartHeight;
	float fFallTime;
	float fGravity;
	float fPitchAngle;
	float fRollAngle;
	float fPitchMult;
	float fScale;
	float fSwimFloatHeight;
	float fActorHeight;
	float fSpeedPct;
	hkVector4 RotCenter;
	hkVector4 PushDelta;
	uint32_t iPushCount;
	bhkCachingShapePhantom* spShapePhantom;
	bhkWorldObject* spCollisionBox;
	SHAPE_TYPES eShapeType;
	SHAPE_TYPES eSizedShapeType;
	// Holds the capsule, list and convex shapes stored by InitShapes.
	NiPointer<bhkShape> spShapes[2];
	hkVector4 FrontBack[4];
	float fRadius;
	float fHeight;
	float fDestRadius;
	float fLODDistance;
	CHARACTER_SIZE eSize;
	uint32_t iPriority;
	uint32_t iSupportCount;
	hkpRigidBody* spSupportBody;
	bool bFakeSupport;
	hkVector4 FakeSupportStart;
	hkRefPtr<hkpRigidBody> spBumpedBody;
	float fBumpedForce;
	hkRefPtr<hkpCharacterProxy> spBumpedCharProxy;
	hkArray<hkpRootCdPoint> LastManifold;
	BSSimpleList<DamageImpactData*> DamageImpacts;

	static inline float fMultipleStepSpeedSquared = 10.0f;
	static inline float fMinHurtfulMass;
	static inline float fGravFrictionDampSpeed;
	static inline float fCalculatePitchTimeOut = 5.0f;
	static inline float fMaxSlopeDefault = 47.0f;
	static inline float fJumpMoveMult = 0.3f;
	static inline float fJumpMoveBase;
	static inline pfGetMaterialStairCallback_t pfGetMaterialStairCallback{};
	// ReleaseDefaultShape releases it.
	static NiPointer<bhkShape> spDefaultConvexShape;
};

NiSmartPointer(bhkCharacterController);

// The creation info of a bhkCharacterProxy: the empty serializable tag and Havok's character proxy cinfo, which the
// object starts with.
class bhkCharacterProxyCinfo : public hkSerializableCinfo, public hkpCharacterProxyCinfo {};

// The creation info of a bhkCharacterController: what it is attached to, its collision filter, gravity, start state,
// shape and state manager.
class alignas(16) bhkCharacterControllerCinfo : public bhkCharacterProxyCinfo {
public:
	bhkCharacterControllerCinfo(bool abDefaultStates);
	~bhkCharacterControllerCinfo();

	// Sets the gravity factor.
	void SetGravity(float afValue);
	void SetScale(float afScale);
	void SetRoll(bool abValue);
	void SetAcrobatics(float afValue);
	void SetWorld(bhkWorld* apWorld);
	void SetPitch(bool abValue);
	void SetUseDefaultShape(bool abSetDefault);
	void SetAvoidBox(bool abTF);
	void SetColFilter(COL_LAYER aeLayer, int32_t aiGroup);
	// Sets the proxy's start position, arPos in game units converted to Havok units.
	void SetPosition(const NiPoint3& arPos);
	void SetShape(bhkShape* apShape);
	void SetCollisionBox(bool abTF);
	void SetObject(NiAVObject* apObject) { pObject = apObject; }
	void SetStartState(hkpCharacterStateType aeState) { eStartState = aeState; }

	// Releases the default state manager's reference, inside the Havok heap context.
	static void ReleaseDefaultManager();

	NiAVObject* pObject;
	CFilter iColFilter;
	bhkWorld* pWorld;
	float fAcrobatics;
	float fGravity;
	bool bAvoidBox;
	bool bCollisionBox;
	bool bUseDefaultShape;
	bool bCanPitch;
	bool bCanRoll;
	hkpCharacterStateType eStartState;
	NiPointer<bhkShape> spShape;
	hkpCharacterStateManager* spManager;
	float fStepHeight;
	float fScale;
	bhkCharacterController::SHAPE_TYPES eShapeType;
	uint32_t iPriority;

	static inline hkpCharacterStateManager* pDefaultManager;
};

// A character controller's impact: the body hit, where, the surface normal and the velocity.
struct DamageImpactData {
	DamageImpactData();

	NiPointer<bhkRigidBody> spBody;
	NiPoint3 kLocation;
	NiPoint3 kNormal;
	NiPoint3 kVelocity;
};

// The creation info of a caching shape phantom; adds nothing to the shape phantom's.
struct hkCachingShapePhantomCinfo : public hkShapePhantomCinfo {
	hkCachingShapePhantomCinfo();
};

using fCircleX_t = float[8];
inline fCircleX_t fCircleX{1.0f, 0.7071f, 0.0f, -0.7071f, -1.0f, -0.7071f, 0.0f, 0.7071f};
using fCircleY_t = float[8];
inline fCircleY_t fCircleY{0.0f, 0.7071f, 1.0f, 0.7071f, 0.0f, -0.7071f, -1.0f, -0.7071f};
inline float gHeightOfFloor;

// The elevator the character stands on, which the listener's callbacks find during the move, and its floor height.
inline hkpRigidBody* gElevator;
