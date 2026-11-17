#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiExtraDataController.h"
#include "Reconstruction.hpp"

// Animates a colour extra data.
class NiColorExtraDataController : public NiExtraDataController {
public:
	NiColorExtraDataController();
	~NiColorExtraDataController() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void Update(NiUpdateData& UpdateData) override;
	NiInterpolator* CreatePoseInterpolator(uint16_t usIndex) override;
	void SynchronizePoseInterpolator(NiInterpolator* apInterp, uint16_t usIndex) override;
	NiBlendInterpolator* CreateBlendInterpolator(uint16_t usIndex, int32_t bManagerControlled,
		bool bAccumulateAnimations, float fWeightThreshold, int32_t ucArraySize) const override;

protected:
	bool InterpolatorIsCorrectType(NiInterpolator* pkInterpolator, uint16_t usIndex) const override;

public:
	static NiObject* CreateObject();

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiColorExtraDataController_vftable{};
