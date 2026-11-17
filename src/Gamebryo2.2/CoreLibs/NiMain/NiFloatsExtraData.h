#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"

// Extra data holding an array of floats.
class NiFloatsExtraData : public NiExtraData {
public:
	NiFloatsExtraData();
	~NiFloatsExtraData() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	// Also NiIntegersExtraData::LoadBinary (the two bodies are one function).
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	// Also NiIntegersExtraData::SaveBinary (the two bodies are one function).
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) override;

	// Stores afValue at auiIndex; false (and nothing stored) when the index is out of range.
	bool SetValue(uint32_t uiIndex, float fValue);
	// The value at auiIndex, or 0 when the index is out of range.
	float GetValue(uint32_t uiIndex) const;

	// Copies the NiExtraData members and a fresh copy of the array into apDest (used by CreateClone).
	void CopyMembers(NiFloatsExtraData* pDest, NiCloningProcess& kCloning);

	static NiObject* CreateObject();

	uint32_t m_uiSize;
	float* m_pfValue;

	static NiRTTI ms_RTTI;
};
