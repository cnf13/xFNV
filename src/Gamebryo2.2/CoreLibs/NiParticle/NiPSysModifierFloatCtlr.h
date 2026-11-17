#pragma once

#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierCtlr.h"
#include "Reconstruction.hpp"

class NiBlendInterpolator;
class NiInterpolator;
class NiStream;
struct NiUpdateData;

// base of the controllers that animate a float of a particle modifier.
class NiPSysModifierFloatCtlr : public NiPSysModifierCtlr {
public:
	NiPSysModifierFloatCtlr();
	~NiPSysModifierFloatCtlr() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
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
	virtual void GetTargetFloatValue(float& afValue) = 0;
	virtual void SetTargetFloatValue(float afValue) = 0;

public:
	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiPSysModifierFloatCtlr_vftable{};
