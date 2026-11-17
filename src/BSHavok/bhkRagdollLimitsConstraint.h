#pragma once

#include "Fallout/AI/Combat/MissileProjectile.h"
#include "BSHavok/bhkConstraint.h"
#include "Reconstruction.hpp"

// The creation info of a ragdoll limits constraint. It adds no fields to hkConstraintCinfo (CreateCinfo 0xCC6310
// allocates 0x14 bytes). The exe's vtable 0x10C9BE8 overrides all six slots; the deleting destructor 0xCC8920 is
// shared with hkStiffSpringConstraintCinfo's.
class hkRagdollLimitsConstraintCinfo : public hkConstraintCinfo {
public:
	hkRagdollLimitsConstraintCinfo();
	~hkRagdollLimitsConstraintCinfo() override;

	void CloneData(hkpConstraintData* apData, NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	uint32_t CreateData() override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;

	// The exe frees the info through MemoryManager::Deallocate.
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

inline constexpr fnvd::Vtable hkRagdollLimitsConstraintCinfo_vftable{};
inline constexpr fnvd::Vtable bhkRagdollLimitsConstraint_vftable{};
