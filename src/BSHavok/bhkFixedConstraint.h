#pragma once

#include "BSHavok/bhkGenericConstraint.h"
#include "BSHavok/bhkMalleableConstraint.h"

// The creation info of a fixed constraint: generic constraint data built to hold two bodies at a reference transform,
// with the indices of its pivot and angular basis in that data. Only part of the class is declared.
class hkFixedConstraintCinfo : public hkGenericConstraintCinfo {
public:
	hkFixedConstraintCinfo();

	void CloneData(hkpConstraintData* apData, NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	// Only jumps to hkGenericConstraintCinfo's version.
	void SaveBinary(NiStream& arStream) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;

	// Builds the generic data's pivot and basis from arTrans and stores their indices.
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
