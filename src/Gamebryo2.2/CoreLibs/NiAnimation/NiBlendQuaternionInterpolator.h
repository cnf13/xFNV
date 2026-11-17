#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"

// Blends rotations.
class NiBlendQuaternionInterpolator : public NiBlendInterpolator {
public:
	NiBlendQuaternionInterpolator();
	~NiBlendQuaternionInterpolator() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	bool Update(float fTime, NiObjectNET* pkInterpTarget, NiQuaternion& kValue) override;
	bool IsQuaternionValueSupported() const override { return true; }
	bool InterpolatorIsCorrectType(NiInterpolator* pkInterpolator) const override;
	using NiInterpolator::Update;

protected:
	// Takes the value of the only interpolator; false when it has none at the time.
	bool StoreSingleValue(float fTime, NiObjectNET* pkInterpTarget, NiQuaternion& kValue);
	// Weighs the rotations of all interpolators, each turned into the hemisphere of the sum so far; false when none
	// has one at the time.
	bool BlendValues(float fTime, NiObjectNET* pkInterpTarget, NiQuaternion& kValue);

public:
	static NiObject* CreateObject();

	NiQuaternion m_kQuaternionValue;

	static NiRTTI ms_RTTI;
};
