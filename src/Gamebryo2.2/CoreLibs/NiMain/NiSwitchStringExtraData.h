#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Reconstruction.hpp"

// Extra data holding an array of strings and the index of the active one.
class NiSwitchStringExtraData : public NiExtraData {
public:
	NiSwitchStringExtraData();
	~NiSwitchStringExtraData() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	// Same function as NiIntegerExtraData::LinkObject (0xA9AF00).
	void LinkObject(NiStream& kStream) override;
	// Same function as NiIntegerExtraData::RegisterStreamables (0xA89AE0).
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) override;

protected:
	// Copies the base data, the array, every string and the index.
	void CopyMembers(NiSwitchStringExtraData* pDest, NiCloningProcess& kCloning);

public:
	static NiObject* CreateObject();

	uint32_t m_uiSize;
	char** m_ppcValue;
	int32_t m_iIndex;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiSwitchStringExtraData_vftable{};
