#pragma once

#include "BSHavok/bhkSerializable.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"

class bhkEntity;
class hkConstraintCinfo;
class hkpConstraintData;
class hkpEntity;
class NiCloningProcess;

// The Gamebryo side of a Havok constraint between two entities. It has no fields of its own: the constraint is the
// wrapped Havok object, reached through phkObject, and the creation info through pInfo.
class bhkConstraint : public bhkSerializable {
public:
	// The default constructor, on apMemory: the ref object's part and the constraint's vtable.
	bhkConstraint();
	bhkConstraint(const hkConstraintCinfo& arInfo);
	~bhkConstraint() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	void LoadBinary(NiStream& arStream) override;
	void LinkObject(NiStream& arStream) override;
	bool RegisterStreamables(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	bool IsEqual(NiObject* apObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	void SethkObject(hkReferencedObject* aphkObject) override;
	void Link(bool abAttach) override;
	const hkpWorld* GethkWorld() const override;
	hkpWorld* GethkWorldRw() override;
	bool Add(bhkWorld* apWorld) override;
	bool Remove() override;
	int32_t LoadCInfo(NiStream& arStream) override;
	void Init(const hkSerializableCinfo& arInfo) override;
	virtual void SetEntityA(hkpEntity* apEntity);
	virtual void SetEntityB(hkpEntity* apEntity);
	virtual hkpEntity* GethkEntityA() const;
	virtual hkpEntity* GethkEntityB() const;
	virtual bool ProcessEntityB(bhkWorld* apWorld, bool abAdd);

	// Copies the creation info's members. Not an override: bhkConstraint keeps bhkSerializable's CopyCinfo. The caller
	// passes the cinfo object's own address, not its hkSerializableCinfo base, unchecked after the allocation, as a
	// dereferenced new cinfo is.
	void CopyCinfo(hkConstraintCinfo& arInfo);
	// Fills the clone apClone from this constraint for CreateClone: the creation info and the two entities, remapped
	// through the cloning process.
	void CopyCloneMembers(bhkSerializable* apClone, NiCloningProcess& arCloning);
	bhkEntity* GetEntityB();
	// The creation info: CreateCinfo's hkSerializableCinfo base, back to the hkConstraintCinfo four bytes before it,
	// or null.
	hkConstraintCinfo* GetCinfo();
	// Null-safe wrapped-object getter, typed as the constraint instance this wrapper holds; returns null for a null
	// this.
	hkpConstraintInstance* operator*();
	hkpConstraintData* GetData();

	static NiRTTI ms_RTTI;
	// Number of live instances; the constructors count them.
	static inline uint32_t iCount;
};

// The instance the constraint wrapper's entity setters go through. It adds no fields to hkpConstraintInstance.
class ahkpConstraintInstance : public hkpConstraintInstance {
public:
	// Replaces the first entity the instance constrains, releasing the old one.
	void setEntityA(hkpEntity* apEntity);
	// Replaces the second entity the instance constrains, releasing the old one.
	void setEntityB(hkpEntity* apEntity);
};
