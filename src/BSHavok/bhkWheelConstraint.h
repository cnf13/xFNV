#pragma once

#include "Fallout/AI/Combat/MissileProjectile.h"
#include "BSHavok/bhkConstraint.h"
#include "Reconstruction.hpp"

// The Gamebryo side of a wheel constraint. It adds no fields to bhkConstraint (its CreateObject allocates 0x10
// bytes).
class bhkWheelConstraint : public bhkConstraint {
public:
	// The deleting destructor (thiscall, ret 4, 0xCC5110; IDA calls it KillCinfo).
	bhkWheelConstraint();
	~bhkWheelConstraint() override;

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

// The creation info of a wheel constraint. It adds no fields to hkConstraintCinfo. The PDB defines it in this
// file's .cpp. GetViewerStrings is a thunk to hkConstraintCinfo's (0xCC5080).
class hkWheelConstraintCinfo : public hkConstraintCinfo {
public:
	// The default constructor.
	hkWheelConstraintCinfo();
	~hkWheelConstraintCinfo() override;

	void CloneData(hkpConstraintData* apData, NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	uint32_t CreateData() override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
};

inline constexpr fnvd::Vtable bhkWheelConstraint_vftable{};
inline constexpr fnvd::Vtable hkWheelConstraintCinfo_vftable{};
