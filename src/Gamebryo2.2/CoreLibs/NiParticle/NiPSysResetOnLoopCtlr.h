#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"

class NiCloningProcess;
class NiStream;
struct NiUpdateData;

// resets its particle system each time the animation loops.
class NiPSysResetOnLoopCtlr : public NiTimeController {
public:
	NiPSysResetOnLoopCtlr();
	~NiPSysResetOnLoopCtlr() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void Update(NiUpdateData& UpdateData) override;

protected:
	bool TargetIsRequiredType() const override;

public:
	static NiObject* CreateObject();

	float m_fLastScaledTime;

	static NiRTTI ms_RTTI;
};
