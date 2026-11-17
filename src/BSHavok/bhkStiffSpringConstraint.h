#pragma once

#include "Fallout/AI/Combat/MissileProjectile.h"
#include "BSHavok/bhkConstraint.h"
#include "Reconstruction.hpp"

// The creation info of a stiff spring constraint. It adds no fields to hkConstraintCinfo. The PDB defines it in
// bhkUtilFunctions.cpp.
class hkStiffSpringConstraintCinfo : public hkConstraintCinfo {
public:
	hkStiffSpringConstraintCinfo();
	~hkStiffSpringConstraintCinfo() override;

	// hkConstraintCinfo slots 1 to 4.
	void CloneData(hkpConstraintData* apData, NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	uint32_t CreateData() override;

	// The exe frees the info through MemoryManager::Deallocate.
	static void operator delete(void* apMemory);
};

// A constraint that keeps two bodies at a fixed distance. Only its RTTI is modelled; it adds no members src reads.
class bhkStiffSpringConstraint : public bhkConstraint {
public:
	bhkStiffSpringConstraint();
	bhkStiffSpringConstraint(const hkStiffSpringConstraintCinfo* apInfo);
	~bhkStiffSpringConstraint() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void KillCinfo(bool abCreated) override;
	uint32_t QCinfoSize() const override;
	hkSerializableCinfo& CreateCinfo(bool& arCreated) override;

	// Allocates the constraint, builds its base and its creation info pointer, and counts it.
	static NiObject* CreateObject();

	static NiRTTI ms_RTTI;
	// Number of live instances; CreateObject and CreateClone count them.
	static inline uint32_t iCount;
};

inline constexpr fnvd::Vtable hkStiffSpringConstraintCinfo_vftable{};
inline constexpr fnvd::Vtable bhkStiffSpringConstraint_vftable{};
