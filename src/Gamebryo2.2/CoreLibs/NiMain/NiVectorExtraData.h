#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"

// Extra data holding one four-component vector.
class NiVectorExtraData : public NiExtraData {
public:
	NiVectorExtraData();
	~NiVectorExtraData() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) override;

	// Allocates and constructs an object whose vector is all zeros.
	static NiObject* CreateObject();

	float m_afVector[4];

	static NiRTTI ms_RTTI;
};
