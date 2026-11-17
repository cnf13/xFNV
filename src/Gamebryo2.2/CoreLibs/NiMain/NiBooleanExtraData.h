#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"

// Extra data holding one boolean.
class NiBooleanExtraData : public NiExtraData {
public:
	NiBooleanExtraData();
	~NiBooleanExtraData() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) override;

	static NiObject* CreateObject();

	bool m_bValue;

	static NiRTTI ms_RTTI;
};
