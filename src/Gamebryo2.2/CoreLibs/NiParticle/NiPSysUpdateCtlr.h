#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Reconstruction.hpp"

class NiCloningProcess;
class NiObjectNET;
class NiStream;
struct NiUpdateData;

// updates its particle system each frame.
class NiPSysUpdateCtlr : public NiTimeController {
public:
	NiPSysUpdateCtlr();
	~NiPSysUpdateCtlr() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void Update(NiUpdateData& UpdateData) override;
	void SetTarget(NiObjectNET* pkTarget) override;

protected:
	bool TargetIsRequiredType() const override;

public:
	static NiObject* CreateObject();

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiPSysUpdateCtlr_vftable{};
