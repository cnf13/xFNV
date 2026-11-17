#pragma once

#include "BSHavok/bhkUnaryAction.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

// The creation info of a motor action: the unary action's body, the axis the body spins about, the spin rate, the
// gain and whether the motor is active. The PDB defines it in BSHavok\bhkMotorAction.cpp. The axis is the PDB's
// bhkVector4: CopyCinfo (0xCC3320) fills it through bhkVector4::operator= (0x4B4DB0).
struct hkMotorActionCinfo : public hkUnaryActionCinfo {
	bhkVector4 m_axis;
	float m_spinRate;
	float m_gain;
	hkBool m_active;
};

// The wrapper of an hkpMotorAction, which spins its body about an axis. Only its RTTI and the virtuals it overrides
// are modelled.
class bhkMotorAction : public bhkUnaryAction {
public:
	bhkMotorAction();
	~bhkMotorAction() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	void KillCinfo(bool abCreated) override;
	uint32_t QCinfoSize() const override;
	void Init(const hkSerializableCinfo& arInfo) override;
	hkSerializableCinfo& CreateCinfo(bool& arCreated) override;

	// Fills arInfo from the wrapped hkpMotorAction.
	void CopyCinfo(hkMotorActionCinfo& arInfo);

	static NiObject* CreateObject();

	static NiRTTI ms_RTTI;
	// The live object count, kept per class level by CreateObject and CreateClone.
	static inline uint32_t iCount;
};
inline constexpr fnvd::Vtable bhkMotorAction_vftable{};
