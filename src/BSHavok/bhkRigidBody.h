#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSHavok/bhkEntity.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"

#include <stdint.h>
#include <xmmintrin.h>

class bhkRigidBodyCinfo;
class BSSpinLock;
class hkAabb;
class hkpRigidBody;
class NiAVObject;
class NiPoint3;
class NiQuaternion;

// Frees a 16-aligned block whose offset byte sits just before it.
void AlignedFreeOffset(void* apMemory);
void GetTotalMassData(NiAVObject* a1, hkVector4* arMassCenter, float* a3);
NiQuaternion* HK2NI_QUATERNION(NiQuaternion* a1, hkQuaternion* a2);

// A simulated solid body: the wrapper around an hkpRigidBody, plus the constraints that act on it.
class bhkRigidBody : public bhkEntity {
public:
	// The bare body that bhkRigidBodyT's constructors start from, without Init.
	bhkRigidBody();
	bhkRigidBody(bhkRigidBodyCinfo& arInfo);
	~bhkRigidBody() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	bhkRigidBody* IsBhkRigidBody() const override { return const_cast<bhkRigidBody*>(this); }
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void LinkObject(NiStream& arStream) override;
	bool RegisterStreamables(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	bool IsEqual(NiObject* apObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	bool Add(bhkWorld* apWorld) override;
	bool Remove() override;
	void KillCinfo(bool abCreated) override;
	uint32_t QCinfoSize() const override { return 0xE0; }
	int32_t LoadCInfo(NiStream& arStream) override;
	void Init(const hkSerializableCinfo& arInfo) override;
	hkSerializableCinfo& CreateCinfo(bool& arCreated) override;
	void Kill() override;
	bool ForceAdd(bhkWorld* apWorld) override;
	NiAVObject* CreateDisplayGeometry(NiNode* apNode) override;
	// The body's position, rotation and centers of mass in Havok units, written to the argument and returned.
	virtual hkVector4& GetPosition(hkVector4& arPos);
	virtual hkQuaternion& GetRotation(hkQuaternion& arRot);
	virtual void SetPosition(const hkVector4& arPos);
	virtual void SetRotation(const hkQuaternion& arRot);

	virtual bool SetMotionType(hkpMotion::MotionType aeMotion);

	virtual void SetPosRot(const hkVector4& arPos, const hkQuaternion& arRot);
	virtual hkVector4& GetCenterOfMassLocal(hkVector4& arCOM);
	virtual hkVector4& GetCenterOfMassInWorld(hkVector4& arCOM);
	virtual hkTransform& GetTransform(hkTransform& arTrans);
	virtual void GetAabb(hkAabb& arOut);
	virtual void SetHavokAssertState(bool abEnabled);

	float GetMass();
	// Moves the keyframed body toward arNewPos and arNewRot over 1 / afInvDeltaTime seconds
	// with afVelGain.
	void ApplyKeyframe(const hkVector4& arNewPos, const hkQuaternion& arNewRot, float afInvDeltaTime, float afVelGain);
	// The mass of the body's dynamic motion, 0 without one.
	float GetDynamicMass();
	// The wrapped body's stored dynamic motion, or null.
	hkpMotion* GetStoredDynamicMotion();
	// The Havok body.
	hkpRigidBody* Gethk() const;
	// The Havok body, or null for a null this.
	hkpRigidBody* operator*() const;
	// The body's linear velocity, or the zero vector without a Havok body.
	const hkVector4& GetLinearVelocity();
	// The body's angular velocity, or the zero vector without a Havok body.
	const hkVector4& GetAngularVelocity();
	// The body's angular velocity converted to game units without scaling, written to arPos.
	void GetAngularVelocity(NiPoint3& arPos);
	// Set the body's velocities, or apply an impulse, inside the world's write marks; nothing without a Havok body.
	void SetLinearVelocity(const hkVector4& arVel);
	void SetAngularVelocity(const hkVector4& arVel);
	void ApplyLinearImpulse(const hkVector4& arImpulse);
	void ApplyAngularImpulse(const hkVector4& arImpulse);
	// The impulse in game units, converted to Havok units and applied through the hkVector4 overload above.
	void ApplyAngularImpulse(const NiPoint3& arImpulse);
	// Adds an action or a constraint to the body; MissileProjectile::ProcessImpale passes a bhkHingeConstraint.
	void AddAction(bhkSerializable* apAction);
	// Activates or deactivates the body.
	void SetActive(bool abTF);
	// Fills arInfo from the wrapped body: bhkWorldObject's part, the Havok body's cinfo, the stored dynamic motion's
	// mass, inertia and damping, and the velocity limits. Not virtual: the virtual CopyCinfo stays bhkSerializable's
	// empty one.
	void CopyCinfo(bhkRigidBodyCinfo& arInfo);
	// The body's shape bounds in its own frame, or a 0.001 box without a shape.
	void GetAabbLocal(hkAabb& arOut);
	// Declared after the virtuals: an earlier same-named non-virtual would make the MS ABI group the virtual
	// SetPosition(const hkVector4&) ahead of GetPosition, changing the vtable order. Moves the body to the game-unit
	// position arPos.
	void SetPosition(const NiPoint3& arPos);
	// Writes the body's position in game units to arPos.
	void GetPosition(NiPoint3& arPos);
	// Writes the body's rotation as a Gamebryo quaternion to arRot.
	void GetRotation(NiQuaternion& arRot);
	// Normalizes the Gamebryo rotation arRot into a Havok one and sets it through the virtual SetRotation.
	void SetRotation(const NiQuaternion& arRot);
	// Sets the linear velocity from game units.
	void SetLinearVelocity(const NiPoint3& arVel);
	// Sets the angular velocity from arVel, unscaled.
	void SetAngularVelocity(const NiPoint3& arVel);
	// Whether the body is awake in its world.
	bool IsActive();
	void SethkMaxLinearVelocity(float afVel);
	// Copies this body's members onto apDestination.
	void CopyMembers(bhkSerializable* apDest, NiCloningProcess& arCloneData);
	// The number of non-null entries of kActionConstrants.
	int32_t CountActions();
	// Returns the address of kActionConstrants.
	BSSimpleList<NiPointer<bhkSerializable>>* GetActionConstraints();
	void RemoveActionConstraints();
	float GethkMaxLinearVelocity();
	const float GetMaxAngularVelocity();
	void AddActConToWorld(bhkWorld* apWorld);
	void ApplyHardKeyframe(const NiPoint3& arPos, const NiQuaternion& arRot, BSSpinLock* apSetPosLock);
	void Fix();
	// The impulse is in game units: it is converted with NI2HK, then applied through ApplyLinearImpulse.
	void ApplyLinearImpulseNi(const NiPoint3& a2);
	void RemoveActionConstraintsFromWorld();
	void UnFix(int32_t abAddRemainingConstraints);

	// Makes apObject's body fixed or dynamic, then its children's. abFix and abAddRemainingConstraints are int32_t
	// rather than bool because they are passed on as the caller's whole 32-bit values; bool would be zero-extended.
	static void SetFixed(NiAVObject* apObject, int32_t abFix, int32_t abAddRemainingConstraints);
	// Whether the motion type aeMotion is a dynamic one.
	static bool IsMotionTypeDynamic(const hkpMotion::MotionType& areMotion);
	// The class loader's factory: returns a newly constructed body.
	static NiObject* CreateObject();
	// Adds the MOTION line for aeType to the viewer strings.
	static void AddMotionType(NiTPrimitiveArray<char*>* apStrings, hkpMotion::MotionType aeType);
	// The MOTION line for a motion type outside the known set.
	static void AddMotionTypeUnknown(NiTPrimitiveArray<char*>* apStrings);
	// The heaviest body in ap3D's subtree: its children's heaviest, or its own body when that is at least as heavy.
	static bhkRigidBody* FindHeaviestRigidBody(NiAVObject* ap3D);
	static void RemoveAllActionConstraints(NiAVObject* apObject);
	static void CalculateCOMAndMassRecursive(NiAVObject* a1, __m128* arMassCenter, float* a3);

	// Holds constraints and actions alike: bhkRigidBody::AddAction adds to the head of the list,
	// WeaponObject::Constrain takes them off the head, and Kill releases them all.
	BSSimpleList<NiPointer<bhkSerializable>> kActionConstrants;

	static NiRTTI ms_RTTI;
	// Live bodies, counted by the constructors.
	static inline uint32_t iCount;
	// Bodies at rest are added to a world deactivated (bipeds excepted).
	static inline bool bStartInactive = true;
};

NiSmartPointer(bhkRigidBody);
