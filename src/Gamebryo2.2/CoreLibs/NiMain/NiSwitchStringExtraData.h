#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"

// Extra data holding an array of strings and the index of the active one.
class NiSwitchStringExtraData : public NiExtraData {
public:
	NiSwitchStringExtraData();
	~NiSwitchStringExtraData() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
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
