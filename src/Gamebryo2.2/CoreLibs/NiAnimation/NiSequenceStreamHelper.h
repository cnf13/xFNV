#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Reconstruction.hpp"

// Carries the sequences of an old-format stream; all its streaming folds into the NiObjectNET bodies.
class NiSequenceStreamHelper : public NiObjectNET {
public:
	NiSequenceStreamHelper();
	~NiSequenceStreamHelper() override;

	const NiRTTI* GetRTTI() const override;
	void LoadBinary(NiStream& stream) override;
	void LinkObject(NiStream& stream) override;
	bool RegisterStreamables(NiStream& stream) override;
	void SaveBinary(NiStream& stream) override;
	bool IsEqual(NiObject* pObject) override;

	static NiObject* CreateObject();

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiSequenceStreamHelper_vftable{};
