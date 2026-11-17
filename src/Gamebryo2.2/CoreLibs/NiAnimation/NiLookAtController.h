#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"

class NiAVObject;

// Turns its target to face another object.
class NiLookAtController : public NiTimeController {
public:
	enum Axis {
		X,
		Y,
		Z,
	};

	NiLookAtController();
	~NiLookAtController() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void ProcessClone(NiCloningProcess& kCloning) override;
	void Update(NiUpdateData& UpdateData) override;
	bool IsTransformController() const override { return true; }

protected:
	bool TargetIsRequiredType() const override;

public:
	static NiObject* CreateObject();

	uint16_t m_uFlags;
	NiAVObject* m_pkLookAt;

	static NiRTTI ms_RTTI;
};
