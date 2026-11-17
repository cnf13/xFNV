#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Reconstruction.hpp"

// Obsolete vertex weights extra data: streams nothing of its own.
class NiVertWeightsExtraData : public NiExtraData {
public:
	NiVertWeightsExtraData();
	~NiVertWeightsExtraData() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	// Same function as NiIntegerExtraData::LinkObject (0xA9AF00).
	void LinkObject(NiStream& kStream) override;
	// Same function as NiIntegerExtraData::RegisterStreamables (0xA89AE0).
	bool RegisterStreamables(NiStream& kStream) override;
	// NiObject's empty SaveBinary (the shared 0xA89AF0): nothing is written, not even NiExtraData's name.
	void SaveBinary(NiStream&) override {}
	bool IsEqual(NiObject* pObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) override;

	// Allocates and constructs an empty object.
	static NiObject* CreateObject();

	static NiRTTI ms_RTTI;
};

inline constexpr fnvd::Vtable NiVertWeightsExtraData_vftable{};
