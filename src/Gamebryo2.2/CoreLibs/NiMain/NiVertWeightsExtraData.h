#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"

// Obsolete vertex weights extra data: streams nothing of its own.
class NiVertWeightsExtraData : public NiExtraData {
public:
	NiVertWeightsExtraData();
	~NiVertWeightsExtraData() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	// SaveBinary is NiObject's empty one: nothing is written, not even NiExtraData's name.
	void SaveBinary(NiStream&) override {}
	bool IsEqual(NiObject* pObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) override;

	// Allocates and constructs an empty object.
	static NiObject* CreateObject();

	static NiRTTI ms_RTTI;
};
