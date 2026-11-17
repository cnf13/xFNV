#pragma once

#include "BSHavok/bhkConstraint.h"
#include "Fallout/AI/Combat/MissileProjectile.h"

// The Havok prismatic (sliding) constraint wrapper.
class bhkPrismaticConstraint : public bhkConstraint {
public:
	bhkPrismaticConstraint();
	~bhkPrismaticConstraint() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void KillCinfo(bool abCreated) override;
	uint32_t QCinfoSize() const override;
	void Init(const hkSerializableCinfo& arInfo) override;
	hkSerializableCinfo& CreateCinfo(bool& arCreated) override;

	// The class factory: bhkRefObject's constructor, then the constraint's and this class's vtables.
	static NiObject* CreateObject();

	static NiRTTI ms_RTTI;
	// Number of live instances; the class factory counts them.
	static inline uint32_t iCount;
};

// The creation info of a prismatic constraint. It adds no fields to hkConstraintCinfo (its data is
// hkpPrismaticConstraintData).
class hkPrismaticConstraintCinfo : public hkConstraintCinfo {
public:
	hkPrismaticConstraintCinfo();
	~hkPrismaticConstraintCinfo() override;

	void CloneData(hkpConstraintData* apData, NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	uint32_t CreateData() override;
	// A thunk to hkConstraintCinfo's.
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
};
