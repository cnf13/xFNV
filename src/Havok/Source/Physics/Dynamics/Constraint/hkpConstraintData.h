#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInfo.h"
#include "Reconstruction.hpp"

#include <stdint.h>

struct hkpConstraintAtom;
class hkpConstraintInstance;
class hkpConstraintQueryIn;
class hkpConstraintQueryOut;
class hkpSolverResults;

// The description of what a constraint does: its type and the atoms the solver runs. Each constraint kind derives
// from it. The exe's vtable has 16 slots: the hkReferencedObject ones (0 to 2) and the constraint interface (3 to
// 15); slots 7, 8, 9 and 15 are _purecall in this base.
class hkpConstraintData : public hkReferencedObject {
public:
	// Values from the Xbox PDB.
	enum ConstraintType : int32_t {
		CONSTRAINT_TYPE_BALLANDSOCKET = 0x0,
		CONSTRAINT_TYPE_HINGE = 0x1,
		CONSTRAINT_TYPE_LIMITEDHINGE = 0x2,
		CONSTRAINT_TYPE_POINTTOPATH = 0x3,
		CONSTRAINT_TYPE_PRISMATIC = 0x6,
		CONSTRAINT_TYPE_RAGDOLL = 0x7,
		CONSTRAINT_TYPE_STIFFSPRING = 0x8,
		CONSTRAINT_TYPE_WHEEL = 0x9,
		CONSTRAINT_TYPE_GENERIC = 0xa,
		CONSTRAINT_TYPE_CONTACT = 0xb,
		CONSTRAINT_TYPE_BREAKABLE = 0xc,
		CONSTRAINT_TYPE_MALLEABLE = 0xd,
		CONSTRAINT_TYPE_POINTTOPLANE = 0xe,
		CONSTRAINT_TYPE_PULLEY = 0xf,
		CONSTRAINT_TYPE_ROTATIONAL = 0x10,
		CONSTRAINT_TYPE_HINGE_LIMITS = 0x12,
		CONSTRAINT_TYPE_RAGDOLL_LIMITS = 0x13,
		CONSTRAINT_TYPE_CUSTOM = 0x14,
		BEGIN_CONSTRAINT_CHAIN_TYPES = 0x64,
		CONSTRAINT_TYPE_STIFF_SPRING_CHAIN = 0x64,
		CONSTRAINT_TYPE_BALL_SOCKET_CHAIN = 0x65,
		CONSTRAINT_TYPE_POWERED_CHAIN = 0x66,
	};

	// The solver memory a constraint needs, with the atoms it is made of.
	struct ConstraintInfo : public hkpConstraintInfo {
		hkpConstraintAtom* m_atoms;
		uint32_t m_sizeOfAllAtoms;
	};

	// How much runtime memory a constraint needs and how many solver results it owns.
	struct RuntimeInfo {
		int32_t m_sizeOfExternalRuntime;
		int32_t m_numSolverResults;
	};

	hkpConstraintData() : m_userData(0) {}
	// The SDK's finish constructor: a loaded object keeps the fields the load wrote.
	hkpConstraintData(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag) {}

	// Does nothing here (vtable slot 3; the SDK's inline, whose warning the release build drops). Same function as
	// BSShaderLightingProperty::CopyToMembers (0xA89AF0), registered there.
	virtual void setMaxLinearImpulse(float maxImpulse) {}
	// The largest impulse the constraint may apply: HK_REAL_MAX, 3.40282e+38f (vtable slot 4; the SDK's inline; the
	// exe's out-of-line copy 0xCA98E0, which loads 0x104DA48, is registered in bhkMalleableConstraint.cpp).
	virtual float getMaxLinearImpulse() const { return 3.40282e+38f; }
	// Does nothing here (vtable slot 5; the SDK's inline). Same function as BSShaderLightingProperty::CopyToMembers
	// (0xA89AF0), registered there.
	virtual void setBodyToNotify(int32_t bodyIdx) {}
	// Returns 0xFF here (vtable slot 6; the SDK's inline, whose assert the release build drops; the exe's
	// out-of-line copy 0xCA98F0 is registered in hkpConstraintData.cpp).
	virtual uint8_t getNotifiedBodyIndex() const { return 0xff; }
	// Whether the constraint's atoms are consistent (vtable slot 7). The exe returns the hkBool through a hidden
	// pointer.
	virtual hkBool* isValid(hkBool* arResult) const = 0;
	// The ConstraintType (vtable slot 8).
	virtual int32_t getType() const = 0;
	// Fills infoOut with the runtime this constraint needs (vtable slot 9).
	virtual void getRuntimeInfo(hkBool abWantRuntime, hkpConstraintData::RuntimeInfo& arInfoOut) const = 0;
	// The solver results kept in apRuntime; the base returns apRuntime (vtable slot 10;
	// thiscall, ret 4, 0xD21A50).
	virtual hkpSolverResults* getSolverResults(void* runtime);
	// Clears the runtime memory (vtable slot 11; thiscall, ret 12, 0xD21A30).
	virtual void addInstance(hkpConstraintInstance* constraint, void* runtime, int32_t sizeOfRuntime) const;
	// Does nothing here (vtable slot 12; the SDK's inline, whose assert the release build drops). Same function as
	// BSShaderProperty::GetTextureUse (0xD070A0), registered there.
	virtual void buildJacobian(const hkpConstraintQueryIn& in, hkpConstraintQueryOut& out) {}

	// Whether buildJacobianCallback must run; the base writes false through the hidden result pointer (vtable
	// slot 13; the SDK's inline; the exe's out-of-line copy 0xCB0020 is registered in hkpConstraintData.cpp).
	virtual hkBool* isBuildJacobianCallbackRequired(hkBool* arResult) const {
		arResult->m_bool = false;
		return arResult;
	}

	// Does nothing here (vtable slot 14; the SDK's inline, whose assert the release build drops). Same function as
	// BSShaderLightingProperty::CopyToMembers (0xA89AF0), registered there.
	virtual void buildJacobianCallback(const hkpConstraintQueryIn& in) {}
	// Fills arInfoOut with the solver memory the constraint needs (vtable slot 15).
	virtual void getConstraintInfo(hkpConstraintData::ConstraintInfo& arInfoOut) const = 0;
	~hkpConstraintData() override {}

	uintptr_t getUserData() const { return m_userData; }
	void setUserData(uintptr_t data) { m_userData = data; }
	// The SDK's inline hook for when the constraint's instance is removed: nothing to do.
	void removeInstance(hkpConstraintInstance* constraint, void* runtime, int32_t sizeOfRuntime) const {}

	// Fills infoOut from the sizeOfAllAtoms bytes of atoms at atoms.
	static void getConstraintInfoUtil(const hkpConstraintAtom* atoms, int32_t sizeOfAllAtoms, ConstraintInfo& infoOut);

	// The SDK's hkUlong, a pointer-sized integer: bhkConstraint stores its hkConstraintCinfo's address here.
	uintptr_t m_userData;
};

// The class's reflection record.
extern hkClass hkpConstraintDataClass;
