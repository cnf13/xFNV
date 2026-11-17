#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineColorInterpolator.h"
#include "Reconstruction.hpp"

// A color B-spline with compressed control points.
class NiBSplineCompColorInterpolator : public NiBSplineColorInterpolator {
public:
	// Indices into m_afCompScalars: the offset and half range that decompress each channel's 16-bit control points.
	enum {
		COLORA_OFFSET,
		COLORA_RANGE,
		NUM_SCALARS,
	};

	NiBSplineCompColorInterpolator(NiBSplineData* pkData, uint32_t kColorCPHandle, NiBSplineBasisData* pkBasisData);
	~NiBSplineCompColorInterpolator() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	// LinkObject folds into a jump to the base body
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	bool Update(float fTime, NiObjectNET* pkInterpTarget, NiColorA& kValue) override;
	bool UsesCompressedControlPoints() const override { return true; }
	using NiInterpolator::Update;

	static NiObject* CreateObject();

	float m_afCompScalars[NUM_SCALARS];

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiBSplineCompColorInterpolator_vftable{};
