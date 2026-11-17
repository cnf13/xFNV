#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Reconstruction.hpp"

// Extra data holding a block of raw bytes.
class NiBinaryExtraData : public NiExtraData {
public:
	// The default constructor: no blob.
	NiBinaryExtraData();
	// The object takes over pcBinaryData (SetValue).
	NiBinaryExtraData(const uint32_t uiSize, char* pcBinaryData);
	~NiBinaryExtraData() override;

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

	// Takes over apcBinaryData (an _NiMalloc block of auiSize bytes); the old block is not freed.
	void SetValue(uint32_t uiSize, char* pcBinaryData);

	static NiObject* CreateObject();

	char* m_pcBinaryData;
	uint32_t m_uiSize;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiBinaryExtraData_vftable{};
