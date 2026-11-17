#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Reconstruction.hpp"

// Extra data holding one string.
class NiStringExtraData : public NiExtraData {
public:
	NiStringExtraData();
	~NiStringExtraData() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& stream) override;
	// Same function as NiIntegerExtraData::LinkObject (0xA9AF00).
	void LinkObject(NiStream& stream) override;
	bool RegisterStreamables(NiStream& stream) override;
	void SaveBinary(NiStream& stream) override;
	// Same function as NiIntegerExtraData::IsEqual (0xA742A0).
	bool IsEqual(NiObject* pObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) override;

	static NiObject* CreateObject();

	NiFixedString m_kString;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiStringExtraData_vftable{};
