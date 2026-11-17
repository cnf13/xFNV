#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Reconstruction.hpp"

// Extra data holding an array of integers.
class NiIntegersExtraData : public NiExtraData {
public:
	NiIntegersExtraData();
	NiIntegersExtraData(const uint32_t uiSize, const int32_t* piValue);
	~NiIntegersExtraData() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	// Same function as NiFloatsExtraData::LoadBinary (0xA99A90).
	void LoadBinary(NiStream& kStream) override;
	// Same function as NiIntegerExtraData::LinkObject (0xA9AF00).
	void LinkObject(NiStream& kStream) override;
	// Same function as NiIntegerExtraData::RegisterStreamables (0xA89AE0).
	bool RegisterStreamables(NiStream& kStream) override;
	// Same function as NiFloatsExtraData::SaveBinary (0xA89210).
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) override;

	// Frees the array, then copies auiSize values from apiValue into a new one (empty when either is 0/null).
	void SetArray(uint32_t uiSize, const int32_t* piValue);

	// Copies the NiExtraData members and a fresh copy of the array into apDest (used by CreateClone).
	void CopyMembers(NiIntegersExtraData* pDest, NiCloningProcess& kCloning);

	static NiObject* CreateObject();

	uint32_t m_uiSize;
	int32_t* m_piValue;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiIntegersExtraData_vftable{};
