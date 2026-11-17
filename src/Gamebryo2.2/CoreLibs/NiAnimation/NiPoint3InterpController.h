#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiSingleInterpController.h"
#include "Reconstruction.hpp"

class NiPoint3;

// A single-interpolator controller that animates one vector or colour of its target.
class NiPoint3InterpController : public NiSingleInterpController {
public:
	NiPoint3InterpController();
	~NiPoint3InterpController() override;

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
	virtual void GetTargetPoint3Value(NiPoint3& akValue) = 0;

public:
	void CopyMembers(NiPoint3InterpController* pkDest, NiCloningProcess& kCloning);

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiPoint3InterpController_vftable{};
