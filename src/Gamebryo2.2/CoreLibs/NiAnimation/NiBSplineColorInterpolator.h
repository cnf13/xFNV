#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Reconstruction.hpp"

// Evaluates a color B-spline.
class NiBSplineColorInterpolator : public NiBSplineInterpolator {
public:
	NiBSplineColorInterpolator(NiBSplineData* pkData, uint32_t kColorACPHandle, NiBSplineBasisData* pkBasisData);
	~NiBSplineColorInterpolator() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	// LinkObject folds into a jump to the NiKeyBasedInterpolator body
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	bool Update(float fTime, NiObjectNET* pkInterpTarget, NiColorA& kValue) override;
	bool IsColorAValueSupported() const override { return true; }
	uint16_t GetChannelCount() const override { return 1; }
	uint32_t GetDimension(int32_t usChannel) const override;
	uint32_t GetDegree(uint16_t usChannel) const override;
	bool GetChannelPosed(int32_t usChannel) const override;
	NiBSplineData::Handle GetControlHandle(int32_t usChannel) const override;
	void SetControlHandle(NiBSplineData::Handle kControlHandle, uint16_t usChannel) override;
	using NiInterpolator::Update;

	// The color and its control-point handle into a clone (m_kBasisHandle is not copied).
	void CopyMembers(NiBSplineColorInterpolator* pkDest, NiCloningProcess& kCloning);

	static NiObject* CreateObject();

	NiColorA m_kColorAValue;
	NiBSplineData::Handle m_kColorACPHandle;
	NiBSplineData::Handle m_kBasisHandle;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiBSplineColorInterpolator_vftable{};
