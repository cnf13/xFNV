#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolInterpolator.h"

// A bool interpolator that does not skip keys: it reports every key it passed since the previous update.
class NiBoolTimelineInterpolator : public NiBoolInterpolator {
public:
	NiBoolTimelineInterpolator(NiBoolData* apBoolData);
	~NiBoolTimelineInterpolator() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	bool Update(float fTime, NiObjectNET* pkInterpTarget, bool& bValue) override;
	using NiInterpolator::Update;

	static NiObject* CreateObject();

	uint32_t m_uiPreviousIdx;
	uint8_t m_bPreviousValue;

	static NiRTTI ms_RTTI;
};
