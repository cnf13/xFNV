#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendInterpolator.h"

// Blends transforms of tread interpolators by the current lerp value.
class BSBlendTreadTransfInterpolator : public NiBlendInterpolator {
public:
	BSBlendTreadTransfInterpolator();
	~BSBlendTreadTransfInterpolator() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& arStream) override;
	// LinkObject and RegisterStreamables only call the base implementations.
	void LinkObject(NiStream& arStream) override;
	bool RegisterStreamables(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	bool IsEqual(NiObject* apObject) override;
	bool Update(float fTime, NiObjectNET* pkInterpTarget, NiQuatTransform& kValue) override;
	bool InterpolatorIsCorrectType(NiInterpolator* pkInterpolator) const override;
	using NiInterpolator::Update;

protected:
	// Takes the transform of the only tread interpolator and advances the lerp by its lerp step; false when it has
	// none at the time.
	bool StoreSingleValue(float fTime, NiObjectNET* pkInterpTarget, NiQuatTransform& kValue);
	// Advances the lerp by the weighted lerp steps of the tread interpolators that update; the transform stays
	// invalid.
	bool BlendValues(float fTime, NiObjectNET* pkInterpTarget, NiQuatTransform& kValue);

public:
	static NiObject* CreateObject();

	float fCurrLerp;

	static NiRTTI ms_RTTI;
};
