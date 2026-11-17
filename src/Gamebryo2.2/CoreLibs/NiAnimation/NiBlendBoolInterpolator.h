#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendInterpolator.h"
#include "Reconstruction.hpp"

// Blends bools; the value byte holds 2 while it is invalid.
class NiBlendBoolInterpolator : public NiBlendInterpolator {
public:
	NiBlendBoolInterpolator();
	// The exe passes the caller's whole dword on; bool would be zero-extended. PDB: bool bManagerControlled,
	// unsigned char ucArraySize.
	NiBlendBoolInterpolator(int32_t abManagerControlled, float afWeightThreshold, int32_t aucArraySize);
	~NiBlendBoolInterpolator() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	// LinkObject and RegisterStreamables fold into jumps to the base bodies
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	bool Update(float fTime, NiObjectNET* pkInterpTarget, bool& bValue) override;
	bool IsBoolValueSupported() const override { return true; }
	bool InterpolatorIsCorrectType(NiInterpolator* pkInterpolator) const override;
	using NiInterpolator::Update;

protected:
	// Takes the value of the only interpolator; false when it has none at the time.
	bool StoreSingleValue(float fTime, NiObjectNET* pkInterpTarget, bool& bValue);
	// Weighs the values of all interpolators; false when none has one at the time.
	bool BlendValues(float fTime, NiObjectNET* pkInterpTarget, bool& bValue);

public:
	static NiObject* CreateObject();

	uint8_t m_bBoolValue;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiBlendBoolInterpolator_vftable{};
