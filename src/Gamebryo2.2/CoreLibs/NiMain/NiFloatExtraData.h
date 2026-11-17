#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Reconstruction.hpp"

// Extra data holding one float.
class NiFloatExtraData : public NiExtraData {
public:
	NiFloatExtraData();
	~NiFloatExtraData() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	// Same function as NiIntegerExtraData::LoadBinary (0xA99D30).
	void LoadBinary(NiStream& kStream) override;
	// Same function as NiIntegerExtraData::LinkObject (0xA9AF00).
	void LinkObject(NiStream& kStream) override;
	// Same function as NiIntegerExtraData::RegisterStreamables (0xA89AE0).
	bool RegisterStreamables(NiStream& kStream) override;
	// Same function as NiIntegerExtraData::SaveBinary (0xA9A4E0).
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) override;

	static NiObject* CreateObject();

	float m_fValue;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiFloatExtraData_vftable{};
