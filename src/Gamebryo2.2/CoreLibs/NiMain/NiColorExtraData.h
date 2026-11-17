#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"

// Extra data holding one colour with alpha.
class NiColorExtraData : public NiExtraData {
public:
	NiColorExtraData();
	~NiColorExtraData() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) override;

	void SetValue(NiColorA akColorA);

	static NiObject* CreateObject();

	NiColorA m_kColorA;

	static NiRTTI ms_RTTI;
};
