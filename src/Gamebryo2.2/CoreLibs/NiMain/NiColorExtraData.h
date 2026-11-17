#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Reconstruction.hpp"

// Extra data holding one colour with alpha.
class NiColorExtraData : public NiExtraData {
public:
	NiColorExtraData();
	~NiColorExtraData() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	// Same function as NiIntegerExtraData::LinkObject (0xA9AF00).
	void LinkObject(NiStream& kStream) override;
	// Same function as NiIntegerExtraData::RegisterStreamables (0xA89AE0).
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) override;

	// The PC exe passes the colour by value (ret 0x10).
	void SetValue(NiColorA akColorA);

	static NiObject* CreateObject();

	NiColorA m_kColorA;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiColorExtraData_vftable{};
