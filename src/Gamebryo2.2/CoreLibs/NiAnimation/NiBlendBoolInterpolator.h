#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendInterpolator.h"

// Blends bools; the value byte holds 2 while it is invalid.
class NiBlendBoolInterpolator : public NiBlendInterpolator {
public:
	NiBlendBoolInterpolator();
	// abManagerControlled and aucArraySize are int32_t rather than bool and unsigned char so that the caller's whole
	// value is passed on; a bool would be zero-extended.
	NiBlendBoolInterpolator(int32_t abManagerControlled, float afWeightThreshold, int32_t aucArraySize);
	~NiBlendBoolInterpolator() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
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
