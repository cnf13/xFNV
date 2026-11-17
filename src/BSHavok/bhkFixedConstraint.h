#pragma once

#include "BSHavok/bhkGenericConstraint.h"
#include "BSHavok/bhkMalleableConstraint.h"
#include "Reconstruction.hpp"

// The creation info of a fixed constraint: generic constraint data built to hold two bodies at a reference
// transform, with the indices of its pivot and angular basis in that data. The PDB defines it in this file's .cpp;
// only what src calls is declared.
class hkFixedConstraintCinfo : public hkGenericConstraintCinfo {
public:
	hkFixedConstraintCinfo();

	void CloneData(hkpConstraintData* apData, NiCloningProcess& arCloneData) override;
	// Only jumps to hkGenericConstraintCinfo's (0xD50000) (thiscall, ret 4, 0xCC7CE0, slot 2).
	void LoadBinary(NiStream& arStream) override;
	// Only jumps to hkGenericConstraintCinfo's (0xD50010) (thiscall, ret 4, 0xCC7CF0, slot 3).
	void SaveBinary(NiStream& arStream) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;

	// IDA name sub_CC7FE0; the PDB's method taking the reference transform. Builds the generic data's pivot and basis
	// from arTransform and stores their indices.
	void SetTransformInRef(const hkTransform& arTrans);

	int32_t RefPivotIndex;
	int32_t RefAngularBasisIndex;
};

// A generic constraint that fixes two bodies to each other. Only its virtual table and RTTI are modelled.
class bhkFixedConstraint : public bhkGenericConstraint {
public:
	bhkFixedConstraint();
	bhkFixedConstraint(const hkFixedConstraintCinfo& arInfo);
	~bhkFixedConstraint() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	uint32_t QCinfoSize() const override;
	hkSerializableCinfo& CreateCinfo(bool& arCreated) override;

	// Makes a fixed constraint: the default constructor chain, its counters and its vtable.
	static NiObject* CreateObject();
	// Fixes apConstraint's two bodies where they are: a fixed constraint at full strength, else a malleable one
	// of strength afStrength around it.
	static bhkConstraint* FixConstraint(bhkConstraint* apConstraint, float afStrength);

	static NiRTTI ms_RTTI;
	// Number of live instances; the constructor counts them.
	static inline uint32_t iCount;
};

inline constexpr fnvd::Vtable hkFixedConstraintCinfo_vftable{};
inline constexpr fnvd::Vtable bhkFixedConstraint_vftable{};
