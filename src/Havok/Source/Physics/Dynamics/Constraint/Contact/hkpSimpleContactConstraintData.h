#pragma once

#include "Havok/Source/Physics/Dynamics/Constraint/Contact/hkpDynamicsCpIdMgr.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Reconstruction.hpp"

class hkContactPoint;
struct hkpBodyVelocity;
class hkpConstraintInstance;
class hkpConstraintOwner;
class hkpContactPointProperties;
class hkpRigidBody;
class hkpSimpleConstraintContactMgr;
struct hkpSimpleConstraintInfoInitInput;
struct hkpSimpleContactConstraintAtom;

// The constraint data of a contact constraint: the contact point id manager and the contact atom holding the
// points. Its code stays with the exe; slots 16 and 17 are the PDB's two intro virtuals.
class hkpSimpleContactConstraintData : public hkpConstraintData {
public:
	hkpSimpleContactConstraintData(hkpConstraintInstance* constraint, hkpRigidBody* bodyA, hkpRigidBody* bodyB);
	~hkpSimpleContactConstraintData() override;

	hkBool* isValid(hkBool* arResult) const override;
	int32_t getType() const override;
	void getRuntimeInfo(hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut) const override;
	hkpSolverResults* getSolverResults(void* runtime) override;
	void getConstraintInfo(hkpConstraintData::ConstraintInfo& infoOut) const override;
	virtual void collisionResponseBeginCallback(const hkContactPoint& cp, hkpSimpleConstraintInfoInitInput& inA,
		hkpBodyVelocity& velA, hkpSimpleConstraintInfoInitInput& inB, hkpBodyVelocity& velB);
	virtual void collisionResponseEndCallback(const hkContactPoint& cp, float impulseApplied,
		hkpSimpleConstraintInfoInitInput& inA, hkpBodyVelocity& velA, hkpSimpleConstraintInfoInitInput& inB,
		hkpBodyVelocity& velB);

	// Frees the contact point id and removes its point from the atom; returns the number of points left.
	// id is an hkContactPointId (an unsigned short).
	int32_t freeContactPoint(hkpConstraintOwner& constraintOwner, uint16_t id);
	// The contact manager this data is embedded in (thiscall, ret 0, 0xD92820: the exe's lea eax,[ecx-0x14], the
	// offset of the manager's m_contactConstraintData).
	hkpSimpleConstraintContactMgr* getSimpleConstraintContactMgr();
	// Adds a contact point and its properties to the atom; returns its id.
	uint16_t allocateContactPoint(hkpConstraintOwner& constraintOwner, hkContactPoint** cpOut,
		hkpContactPointProperties** cpPropsOut);

	// The PDB has m_clientData at 0x20: m_idMgrA is 20 bytes (its m_values is an hkInplaceArray<hkUint8, 8>).
	hkpDynamicsCpIdMgr m_idMgrA;
	void* m_clientData;
	hkpConstraintInstance* m_constraint;
	hkpSimpleContactConstraintAtom* m_atom;
	int32_t m_atomSize;
};
inline constexpr fnvd::Vtable hkpSimpleContactConstraintData_vftable{};
