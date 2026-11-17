#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiExtraDataController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Reconstruction.hpp"

// Animates three consecutive elements of a floats extra data as a vector.
class NiFloatsExtraDataPoint3Controller : public NiExtraDataController {
public:
	NiFloatsExtraDataPoint3Controller();
	~NiFloatsExtraDataPoint3Controller() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void Update(NiUpdateData& UpdateData) override;
	const char* GetCtlrID() override;
	NiInterpolator* CreatePoseInterpolator(uint16_t usIndex) override;
	void SynchronizePoseInterpolator(NiInterpolator* apInterp, uint16_t usIndex) override;
	NiBlendInterpolator* CreateBlendInterpolator(uint16_t usIndex, int32_t bManagerControlled,
		bool bAccumulateAnimations, float fWeightThreshold, int32_t ucArraySize) const override;

protected:
	bool InterpolatorIsCorrectType(NiInterpolator* pkInterpolator, uint16_t usIndex) const override;

public:
	static NiObject* CreateObject();

	int32_t m_iFloatsExtraDataIndex;
	NiFixedString m_kCtlrID;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiFloatsExtraDataPoint3Controller_vftable{};
