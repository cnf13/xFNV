#pragma once

#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierFloatCtlr.h"
#include "Reconstruction.hpp"

class NiCloningProcess;
class NiObjectNET;
class NiStream;

// animates the air friction of the particle air field it targets.
class NiPSysAirFieldAirFrictionCtlr : public NiPSysModifierFloatCtlr {
public:
	NiPSysAirFieldAirFrictionCtlr();
	~NiPSysAirFieldAirFrictionCtlr() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;

protected:
	bool InterpTargetIsCorrectType(NiObjectNET* pkTarget) const override;
	void GetTargetFloatValue(float& fValue) override;
	void SetTargetFloatValue(float fValue) override;

public:
	static NiObject* CreateObject();

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiPSysAirFieldAirFrictionCtlr_vftable{};
