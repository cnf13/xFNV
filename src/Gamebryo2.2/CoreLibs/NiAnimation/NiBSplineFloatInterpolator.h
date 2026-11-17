#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineInterpolator.h"

// Evaluates a float B-spline.
class NiBSplineFloatInterpolator : public NiBSplineInterpolator {
public:
	NiBSplineFloatInterpolator(NiBSplineData* pkData, uint32_t kFloatCPHandle, NiBSplineBasisData* pkBasisData);
	~NiBSplineFloatInterpolator() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	bool Update(float fTime, NiObjectNET* pkInterpTarget, float& fValue) override;
	bool IsFloatValueSupported() const override { return true; }
	uint16_t GetChannelCount() const override { return 1; }
	uint32_t GetDimension(int32_t usChannel) const override;
	uint32_t GetDegree(uint16_t usChannel) const override;
	bool GetChannelPosed(int32_t usChannel) const override;
	NiBSplineData::Handle GetControlHandle(int32_t usChannel) const override;
	void SetControlHandle(NiBSplineData::Handle kControlHandle, uint16_t usChannel) override;
	using NiInterpolator::Update;

	void CopyMembers(NiBSplineFloatInterpolator* pkDest, NiCloningProcess& kCloning);

	static NiObject* CreateObject();

	float m_fFloatValue;
	NiBSplineData::Handle m_kFloatCPHandle;

	static NiRTTI ms_RTTI;
};
