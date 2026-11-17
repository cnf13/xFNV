#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendInterpolator.h"
#include "Reconstruction.hpp"

// Blends floats.
class NiBlendFloatInterpolator : public NiBlendInterpolator {
public:
	NiBlendFloatInterpolator();
	// Runs the constructor (0xA40520) on apMemory and returns it. The exe passes the caller's whole dword on; bool
	// would be zero-extended. PDB: bool bManagerControlled, unsigned char ucArraySize.
	NiBlendFloatInterpolator(int32_t abManagerControlled, float afWeightThreshold, int32_t aucArraySize);
	~NiBlendFloatInterpolator() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	// LinkObject and RegisterStreamables fold into jumps to the base bodies
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	bool Update(float fTime, NiObjectNET* pkInterpTarget, float& fValue) override;
	bool IsFloatValueSupported() const override { return true; }
	bool InterpolatorIsCorrectType(NiInterpolator* pkInterpolator) const override;
	using NiInterpolator::Update;

protected:
	// Takes the value of the only interpolator; false when it has none at the time.
	bool StoreSingleValue(float fTime, NiObjectNET* pkInterpTarget, float& fValue);
	// Weighs the values of all interpolators; false when none has one at the time.
	bool BlendValues(float fTime, NiObjectNET* pkInterpTarget, float& fValue);

public:
	static NiObject* CreateObject();

	float m_fFloatValue;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiBlendFloatInterpolator_vftable{};
