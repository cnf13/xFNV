#pragma once

#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "BSHavok/bhkBinaryAction.h"
#include "Reconstruction.hpp"

struct hkAngularDashpotActionCinfo;

// An action that damps the relative rotation of two bodies.
class bhkAngularDashpotAction : public bhkBinaryAction {
public:
	bhkAngularDashpotAction();
	~bhkAngularDashpotAction() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	uint32_t QCinfoSize() const override;
	void Init(const hkSerializableCinfo& arInfo) override;
	hkSerializableCinfo& CreateCinfo(bool& arCreated) override;

	// Fills apInfo from the wrapped hkpAngularDashpotAction.
	void CopyCinfo(hkAngularDashpotActionCinfo& arInfo);

	// The class loader's factory: a new action built by the inlined constructor.
	static NiObject* CreateObject();

	static NiRTTI ms_RTTI;
	// Live actions, counted by the inlined constructors.
	static inline uint32_t iCount;
};

// The creation info of an angular dashpot action. The PDB defines it in this file's .cpp. Its constructor is the
// exe's 0xCC4500 (thiscall, ret 0, below), which also stores the identity quaternion.
struct hkAngularDashpotActionCinfo : public hkBinaryActionCinfo {
	hkAngularDashpotActionCinfo();

	hkQuaternion m_rotation;
	float m_strength;
	float m_damping;
};

inline constexpr fnvd::Vtable bhkAngularDashpotAction_vftable{};
