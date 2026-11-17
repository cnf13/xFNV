#pragma once

#include "Fallout/AI/Combat/MissileProjectile.h"
#include "BSHavok/bhkConstraint.h"
#include "Reconstruction.hpp"

// The creation info of a ragdoll constraint. It adds no fields to hkConstraintCinfo. The PDB defines it in
// bhkRagdollConstraint.cpp; src has no header for the wrapper bhkRagdollConstraint yet.
class hkRagdollConstraintCinfo : public hkConstraintCinfo {
public:
	hkRagdollConstraintCinfo();
	~hkRagdollConstraintCinfo() override;

	void CloneData(hkpConstraintData* apData, NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	uint32_t CreateData() override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
};

// The Gamebryo side of a Havok ragdoll constraint: the cone, plane and twist motors between two bodies. The PDB
// defines it in bhkRagdollConstraint.cpp; only the members src has bodies for are declared here.
class bhkRagdollConstraint : public bhkConstraint {
public:
	bhkRagdollConstraint();
	bhkRagdollConstraint(const hkRagdollConstraintCinfo& arInfo);

	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	const NiRTTI* GetRTTI() const override;
	// Releases the creation info through its virtual deleting destructor when this wrapper made it;
	// bhkRagdollLimitsConstraint's slot 41 holds the same body.
	void KillCinfo(bool abCreated) override;
	// The exe's slot 42 (0xCC74D0, bhkBallAndSocketConstraint's body).
	uint32_t QCinfoSize() const override;
	// The exe's slot 45 (0xCC6AB0).
	hkSerializableCinfo& CreateCinfo(bool& arCreated) override;
	virtual ~bhkRagdollConstraint();

	// Allocates the constraint, builds its base and its creation info pointer, and counts it.
	static NiObject* CreateObject();

	// Number of live instances; CreateObject counts them.
	static inline uint32_t iCount;
};

inline constexpr fnvd::Vtable hkRagdollConstraintCinfo_vftable{};
inline constexpr fnvd::Vtable bhkRagdollConstraint_vftable{};
