#pragma once

#include "BSHavok/bhkConstraint.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Malleable/hkpMalleableConstraintData.h"
#include "Fallout/AI/Combat/MissileProjectile.h"
#include "Reconstruction.hpp"

// Bethesda's hkpMalleableConstraintData: the same layout and vtable, with the engine's own constructor and setup
// functions. The PDB names no source file for it; the name is IDA's (`a` for Bethesda), not in the PDB.
class ahkpMalleableConstraintData : public hkpMalleableConstraintData {
public:
	ahkpMalleableConstraintData();

	// Inferred type: a1: hkpConstraintData* from 0xca9c8a (result of bhkConstraintManager::Clone)
	void SetChild(hkpConstraintData* a1);
};

// The creation info of a malleable constraint: hkConstraintCinfo's four members and its own overrides of the five
// serialization slots (the destructor is the base's). Not in IDA's header; the PDB defines it in this file.
class hkMalleableConstraintCinfo : public hkConstraintCinfo {
public:
	hkMalleableConstraintCinfo();

	void CloneData(hkpConstraintData* apData, NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	// Creates the creation info's malleable constraint data when it has none, under the Havok heap context, and
	// returns the data's size.
	uint32_t CreateData() override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;

	void CA9E50(hkpConstraintData* a2);
	// Stores afStrength in the malleable data when there is data.
	void SetStrength(float afStrength);

	// Scales the data apInfo's malleable data wraps by afScale.
	static void ApplyScale(float afScale, hkpConstraintData* apInfo);
};

// The wrapper of an hkpMalleableConstraintData, a constraint whose strength is scaled. Only its RTTI and the
// virtuals it overrides are modelled.
class bhkMalleableConstraint : public bhkConstraint {
public:
	// The constructor from a creation info.
	bhkMalleableConstraint();
	bhkMalleableConstraint(const hkMalleableConstraintCinfo& arInfo);
	~bhkMalleableConstraint() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void LinkObject(NiStream& arStream) override;
	void KillCinfo(bool abCreated) override;
	uint32_t QCinfoSize() const override;
	hkSerializableCinfo& CreateCinfo(bool& arCreated) override;

	// The constraint's creation info.
	hkMalleableConstraintCinfo& GetCinfo();

	// Creates an uninitialised instance for the stream's factory.
	static NiObject* CreateObject();

	static NiRTTI ms_RTTI;
	// Not in the PDB; IDA's g_iNumBhkMalleableConstraints. Number of live instances; the constructors count them.
	static inline uint32_t iCount;
};

inline constexpr fnvd::Vtable ahkpMalleableConstraintData_vftable{};
inline constexpr fnvd::Vtable hkMalleableConstraintCinfo_vftable{};
inline constexpr fnvd::Vtable bhkMalleableConstraint_vftable{};
