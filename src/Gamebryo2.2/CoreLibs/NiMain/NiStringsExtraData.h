#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"

// Extra data holding an array of strings.
class NiStringsExtraData : public NiExtraData {
public:
	NiStringsExtraData();
	~NiStringsExtraData() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) override;

protected:
	// Copies the base data, the array and every string.
	void CopyMembers(NiStringsExtraData* pDest, NiCloningProcess& kCloning);

public:
	static NiObject* CreateObject();

	uint32_t m_uiSize;
	char** m_ppcValue;

	static NiRTTI ms_RTTI;
};
