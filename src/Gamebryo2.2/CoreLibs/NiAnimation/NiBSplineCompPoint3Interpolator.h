#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplinePoint3Interpolator.h"
#include "Reconstruction.hpp"

// A point B-spline with compressed control points.
class NiBSplineCompPoint3Interpolator : public NiBSplinePoint3Interpolator {
public:
	// Indices into m_afCompScalars: the offset and half range that decompress each channel's 16-bit control points.
	enum {
		POINT3_OFFSET,
		POINT3_RANGE,
		NUM_SCALARS,
	};

	NiBSplineCompPoint3Interpolator(NiBSplineData* pkData, uint32_t kPoint3CPHandle, NiBSplineBasisData* pkBasisData);
	~NiBSplineCompPoint3Interpolator() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	// LinkObject folds into a jump to the base body
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	bool Update(float fTime, NiObjectNET* pkInterpTarget, NiPoint3& kValue) override;
	bool UsesCompressedControlPoints() const override { return true; }
	using NiInterpolator::Update;

	static NiObject* CreateObject();

	float m_afCompScalars[NUM_SCALARS];

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiBSplineCompPoint3Interpolator_vftable{};
