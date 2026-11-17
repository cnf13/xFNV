#pragma once

#include "BSHavok/bhkNiCollisionObject.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"

class bhkRigidBody;
class hkpRigidBody;

// The collision object attached to ordinary scene graph nodes; adds no data to bhkNiCollisionObject.
class bhkCollisionObject : public bhkNiCollisionObject {
public:
	bhkCollisionObject();
	// The constructor that attaches the new object to apObject.
	bhkCollisionObject(NiAVObject* apObject);
	~bhkCollisionObject() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void LinkObject(NiStream& arStream) override;
	bool RegisterStreamables(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	bool IsEqual(NiObject* apObject) override;
	void AddViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	void UpdateWorldData(NiUpdateData& UpdateData) override;
	NiPoint3 GetVelocity() override;
	void hktoNiTransform() override;
	void NitohkTransform() override;
	void ClearVelocities() override;
	void SetMotionType(hkpMotion::MotionType aeType, bhkWorldObject* apObject, bool abAllowActivate) override;
	bool QKeyframe() override;

	// The rigid body this object wraps (its world object, the body Sun::GetBase shares).
	bhkRigidBody* GetRigidBody();
	// Copies the collision object's members into apDest when cloning.
	void CopyMembers(bhkCollisionObject* apDest, NiCloningProcess& arCloneData);

	// apObject's collision object when it is a bhkCollisionObject, else null.
	static bhkCollisionObject* GetbhkCollisionObject(NiAVObject* apObject);
	// apBody's collision object when it is a bhkCollisionObject, else null.
	static bhkCollisionObject* Getbhk(const hkpRigidBody* apBody);
	// The class loader's factory: a new object, or null when the allocation fails.
	static NiObject* CreateObject();

	static NiRTTI ms_RTTI;
};
