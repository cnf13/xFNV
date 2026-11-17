#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"

class NiBlendInterpolator;
class NiFixedString;
class NiInterpolator;

// A time controller driven by one or more interpolators.
class NiInterpController : public NiTimeController {
public:
	NiInterpController();
	~NiInterpController() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	virtual uint16_t GetInterpolatorCount() const = 0;
	virtual const char* GetInterpolatorID(uint16_t ausIndex) = 0;
	virtual uint16_t GetInterpolatorIndex(const char* apcID) const = 0;
	virtual uint16_t GetInterpolatorIndexFx(const NiFixedString& akID) const = 0;
	virtual NiInterpolator* GetInterpolator(uint16_t ausIndex) const = 0;
	virtual void SetInterpolator(NiInterpolator* apInterpolator, uint16_t ausIndex) = 0;
	virtual void ResetTimeExtrema();
	virtual const char* GetCtlrID() { return nullptr; }
	virtual NiInterpolator* CreatePoseInterpolator(uint16_t ausIndex) = 0;
	virtual void SynchronizePoseInterpolator(NiInterpolator* apInterp, uint16_t ausIndex) = 0;
	// abManagerControlled and aucArraySize are int32_t so that every override passes the caller's whole value on as is;
	// bool and uint8_t would be zero-extended. They hold a bool bManagerControlled and an unsigned char ucArraySize.
	virtual NiBlendInterpolator* CreateBlendInterpolator(uint16_t ausIndex, int32_t abManagerControlled,
		bool abAccumulateAnimations, float afWeightThreshold, int32_t aucArraySize) const = 0;
	virtual void GuaranteeTimeRange(float afStartTime, float afEndTime) = 0;

protected:
	virtual bool InterpolatorIsCorrectType(NiInterpolator* apInterpolator, uint16_t ausIndex) const = 0;

public:
	void CopyMembers(NiInterpController* pkDest, NiCloningProcess& kCloning);
	// A manager-controlled controller is driven by its NiControllerManager's sequences, not by its own key range.
	bool GetManagerControlled() const { return (m_usFlags & MANAGERCONTROLLED_MASK) != 0; }
	void SetManagerControlled(bool bManagerControlled) { SetBit(bManagerControlled, MANAGERCONTROLLED_MASK); }

	static NiRTTI ms_RTTI;
	// The index of an interpolator the controller does not have (0xFFFF).
	static constexpr uint16_t INVALID_INDEX = 65535;
	// A time no key is at (-FLT_MAX).
	static constexpr float INVALID_TIME = -3.4028235e+38f;
};
