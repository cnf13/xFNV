#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Reconstruction.hpp"

// Evaluates a point B-spline.
class NiBSplinePoint3Interpolator : public NiBSplineInterpolator {
public:
	NiBSplinePoint3Interpolator(NiBSplineData* pkData, uint32_t kPoint3CPHandle, NiBSplineBasisData* pkBasisData);
	~NiBSplinePoint3Interpolator() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	// LinkObject folds into a jump to the NiKeyBasedInterpolator body
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	bool Update(float fTime, NiObjectNET* pkInterpTarget, NiPoint3& kValue) override;
	bool IsPoint3ValueSupported() const override { return true; }
	uint16_t GetChannelCount() const override { return 1; }
	uint32_t GetDimension(int32_t usChannel) const override;
	uint32_t GetDegree(uint16_t usChannel) const override;
	bool GetChannelPosed(int32_t usChannel) const override;
	NiBSplineData::Handle GetControlHandle(int32_t usChannel) const override;
	void SetControlHandle(NiBSplineData::Handle kControlHandle, uint16_t usChannel) override;
	using NiInterpolator::Update;

	void CopyMembers(NiBSplinePoint3Interpolator* pkDest, NiCloningProcess& kCloning);

	static NiObject* CreateObject();

	NiPoint3 m_kPoint3Value;
	NiBSplineData::Handle m_kPoint3CPHandle;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiBSplinePoint3Interpolator_vftable{};
