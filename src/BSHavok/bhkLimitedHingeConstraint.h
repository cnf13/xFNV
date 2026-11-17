#pragma once

#include "BSHavok/bhkConstraint.h"
#include "Fallout/AI/Combat/MissileProjectile.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/LimitedHinge/hkpLimitedHingeConstraintData.h"
#include "Reconstruction.hpp"

class hkLimitedHingeConstraintCinfo;
class hkpConstraintMotor;

// The game wrapper of a limited hinge constraint. Its members are not declared yet.
class bhkLimitedHingeConstraint : public bhkConstraint {
public:
	bhkLimitedHingeConstraint();
	bhkLimitedHingeConstraint(const hkLimitedHingeConstraintCinfo& arInfo);
	~bhkLimitedHingeConstraint() override;

	const NiRTTI* GetRTTI() const override;
	bhkLimitedHingeConstraint* IsBhkLimitedHingeConstraint() override { return this; }
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void KillCinfo(bool abCreated) override;
	uint32_t QCinfoSize() const override;
	hkSerializableCinfo& CreateCinfo(bool& arCreated) override;
	void SetEntityB(hkpEntity* apEntity) override;

	// The constraint's hinge data, which holds the motor; the exe calls the data's setMotorActive on it (thiscall,
	// ret 0).
	hkpLimitedHingeConstraintData* GetMotor();
	// Turns the motor on or off with abActive, when the constraint has one.
	void SetMotorActive(bool abActive);
	// The exe's shared null-safe wrapped-object getter (0x4AE750, the one bhkWorldObject::operator* declares),
	// typed as the constraint instance this wrapper holds; null for a null this.
	hkpConstraintInstance* operator*();
	void SetMotorTargetAngle(float afAngle);
	hkLimitedHingeConstraintCinfo& GetCinfo();

	// Builds a bhkLimitedHingeConstraint from NiMemObject's allocator; the stream's create function.
	static NiObject* CreateObject();

	static NiRTTI ms_RTTI;
	// Number of live instances; CreateObject counts it (the exe's constructor bodies are inlined there).
	static inline uint32_t iCount;
};

// The creation info of a limited hinge constraint. It adds no fields to hkConstraintCinfo (its data is
// hkpLimitedHingeConstraintData, 0xE0 bytes). The PDB names WeaponObject.cpp as its file; it is put with its
// wrapper here.
class hkLimitedHingeConstraintCinfo : public hkConstraintCinfo {
public:
	hkLimitedHingeConstraintCinfo();
	~hkLimitedHingeConstraintCinfo() override;

	void CloneData(hkpConstraintData* apData, NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	uint32_t CreateData() override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
};

inline constexpr fnvd::Vtable bhkLimitedHingeConstraint_vftable{};
inline constexpr fnvd::Vtable hkLimitedHingeConstraintCinfo_vftable{};
