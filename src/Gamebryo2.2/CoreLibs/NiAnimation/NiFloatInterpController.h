#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiSingleInterpController.h"
#include "Reconstruction.hpp"

// A single-interpolator controller that animates one float of its target.
class NiFloatInterpController : public NiSingleInterpController {
public:
	NiFloatInterpController();
	~NiFloatInterpController() override;

	const NiRTTI* GetRTTI() const override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	NiInterpolator* CreatePoseInterpolator(uint16_t usIndex) override;
	void SynchronizePoseInterpolator(NiInterpolator* apInterp, uint16_t usIndex) override;
	NiBlendInterpolator* CreateBlendInterpolator(uint16_t usIndex, int32_t bManagerControlled,
		bool bAccumulateAnimations, float fWeightThreshold, int32_t ucArraySize) const override;

protected:
	bool InterpolatorIsCorrectType(NiInterpolator* pkInterpolator, uint16_t usIndex) const override;
	// the animated value as the target holds it now
	virtual void GetTargetFloatValue(float& afValue) = 0;

public:
	void CopyMembers(NiFloatInterpController* pkDest, NiCloningProcess& kCloning);

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiFloatInterpController_vftable{};
