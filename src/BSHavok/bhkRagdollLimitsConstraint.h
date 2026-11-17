#pragma once

#include "BSHavok/bhkConstraint.h"
#include "Fallout/AI/Combat/MissileProjectile.h"

// The creation info of a ragdoll limits constraint. It adds no fields to hkConstraintCinfo.
class hkRagdollLimitsConstraintCinfo : public hkConstraintCinfo {
public:
	hkRagdollLimitsConstraintCinfo();
	~hkRagdollLimitsConstraintCinfo() override;

	void CloneData(hkpConstraintData* apData, NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	uint32_t CreateData() override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;

	// Frees the info through MemoryManager::Deallocate.
	static void operator delete(void* apMemory);
};

// The wrapper of an hkpRagdollLimitsData constraint. Only its RTTI and the virtuals it overrides are modelled.
class bhkRagdollLimitsConstraint : public bhkConstraint {
public:
	bhkRagdollLimitsConstraint();
	~bhkRagdollLimitsConstraint() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void KillCinfo(bool abCreated) override;
	uint32_t QCinfoSize() const override;
	hkSerializableCinfo& CreateCinfo(bool& arCreated) override;

	static NiObject* CreateObject();

	static NiRTTI ms_RTTI;
	// Number of live instances; the constructors count them.
	static inline uint32_t iCount;
};
