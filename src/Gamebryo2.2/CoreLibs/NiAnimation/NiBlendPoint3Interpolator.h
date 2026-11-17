#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Reconstruction.hpp"

// Blends points.
class NiBlendPoint3Interpolator : public NiBlendInterpolator {
public:
	NiBlendPoint3Interpolator();
	// Runs the constructor (0xA55A10) on apMemory and returns it. The exe passes the caller's whole dword on; bool
	// would be zero-extended. PDB: bool bManagerControlled, unsigned char ucArraySize.
	NiBlendPoint3Interpolator(int32_t abManagerControlled, float afWeightThreshold, int32_t aucArraySize);
	~NiBlendPoint3Interpolator() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	// LinkObject and RegisterStreamables fold into jumps to the base bodies
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	bool Update(float fTime, NiObjectNET* pkInterpTarget, NiPoint3& kValue) override;
	bool IsPoint3ValueSupported() const override { return true; }
	bool InterpolatorIsCorrectType(NiInterpolator* pkInterpolator) const override;
	using NiInterpolator::Update;

protected:
	// Takes the value of the only interpolator; false when it has none at the time.
	bool StoreSingleValue(float fTime, NiObjectNET* pkInterpTarget, NiPoint3& kValue);
	// Weighs the values of all interpolators; false when none has one at the time.
	bool BlendValues(float fTime, NiObjectNET* pkInterpTarget, NiPoint3& kValue);

public:
	static NiObject* CreateObject();

	NiPoint3 m_kPoint3Value;
	bool m_bValidValue;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiBlendPoint3Interpolator_vftable{};
