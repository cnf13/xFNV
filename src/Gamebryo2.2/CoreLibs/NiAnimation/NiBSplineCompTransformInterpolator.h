#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineTransformInterpolator.h"
#include "Reconstruction.hpp"

// A transform B-spline with compressed control points.
class NiBSplineCompTransformInterpolator : public NiBSplineTransformInterpolator {
public:
	// Indices into m_afCompScalars: the offset and half range that decompress each channel's 16-bit control points.
	enum {
		POSITION_OFFSET,
		POSITION_RANGE,
		ROTATION_OFFSET,
		ROTATION_RANGE,
		SCALE_OFFSET,
		SCALE_RANGE,
		NUM_SCALARS,
	};

	NiBSplineCompTransformInterpolator(NiBSplineData* pkData, uint32_t kTransCPHandle, uint32_t kRotCPHandle,
		uint32_t kScaleCPHandle, NiBSplineBasisData* pkBasisData);
	~NiBSplineCompTransformInterpolator() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	// LinkObject folds into a jump to the base body
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	bool Update(float fTime, NiObjectNET* pkInterpTarget, NiQuatTransform& kValue) override;
	bool UsesCompressedControlPoints() const override { return true; }
	using NiInterpolator::Update;

	// The stream's factory: a new interpolator, the scalars FLT_MAX.
	static NiObject* CreateObject();

	float m_afCompScalars[NUM_SCALARS];

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiBSplineCompTransformInterpolator_vftable{};
