#pragma once

#include "Fallout/AI/Combat/MissileProjectile.h"
#include "BSHavok/bhkConstraint.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Breakable/hkpBreakableConstraintData.h"
#include "Reconstruction.hpp"

// The wrapper of an hkpBreakableConstraintData, a constraint that breaks past an impulse limit. Only its RTTI and
// the virtuals it overrides are modelled.
class bhkBreakableConstraint : public bhkConstraint {
public:
	bhkBreakableConstraint();
	~bhkBreakableConstraint() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void LinkObject(NiStream& arStream) override;
	void KillCinfo(bool abCreated) override;
	uint32_t QCinfoSize() const override;
	hkSerializableCinfo& CreateCinfo(bool& arCreated) override;

	// The class's factory: a new default-built breakable constraint.
	static NiObject* CreateObject();

	static NiRTTI ms_RTTI;
	// Number of live instances; the constructors count them.
	static inline uint32_t iCount;
};

// The creation info of a breakable constraint. It adds no fields to hkConstraintCinfo. The PDB defines it in this
// file's .cpp.
class hkBreakableConstraintCinfo : public hkConstraintCinfo {
public:
	hkBreakableConstraintCinfo();
	~hkBreakableConstraintCinfo() override;

	void CloneData(hkpConstraintData* apData, NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	uint32_t CreateData() override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
};

// The game's breakable constraint data (the PDB defines it in bhkBreakableConstraint.cpp): hkpBreakableConstraintData
// with the game's defaults. It adds no data; its vtable holds only hkpBreakableConstraintData's functions.
class ahkpBreakableConstraintData : public hkpBreakableConstraintData {
public:
	ahkpBreakableConstraintData();
};

inline constexpr fnvd::Vtable bhkBreakableConstraint_vftable{};
inline constexpr fnvd::Vtable hkBreakableConstraintCinfo_vftable{};
inline constexpr fnvd::Vtable ahkpBreakableConstraintData_vftable{};
