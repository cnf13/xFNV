#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendInterpolator.h"
#include "Reconstruction.hpp"

// Blends transforms.
class NiBlendTransformInterpolator : public NiBlendInterpolator {
public:
	NiBlendTransformInterpolator();
	NiBlendTransformInterpolator(int32_t abManagerControlled, float afWeightThreshold, int32_t aucArraySize);
	~NiBlendTransformInterpolator() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	// LinkObject and RegisterStreamables fold into jumps to the base bodies
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	bool Update(float fTime, NiObjectNET* pkInterpTarget, NiQuatTransform& kValue) override;
	bool IsTransformValueSupported() const override { return true; }
	bool InterpolatorIsCorrectType(NiInterpolator* pkInterpolator) const override;
	using NiInterpolator::Update;

	// Protected in the PDB; public because NiMultiTargetTransformController clones its blend interpolators with it.
	void CopyMembers(NiBlendTransformInterpolator* pkDest, NiCloningProcess& kCloning);

protected:
	// Takes the transform of the only interpolator, its rotation normalized; false when it has none at the time.
	bool StoreSingleValue(float fTime, NiObjectNET* pkInterpTarget, NiQuatTransform& kValue);
	// Weighs translation, rotation and scale of all interpolators separately; false when no part is valid.
	bool BlendValues(float fTime, NiObjectNET* pkInterpTarget, NiQuatTransform& kValue);

public:
	// Sets the game's "always blend" switch for transform blending.
	static void SetAlwaysBlend(bool abTF);
	static NiObject* CreateObject();

	static NiRTTI ms_RTTI;
	static inline bool bsAlwaysBlend;
};
inline constexpr fnvd::Vtable NiBlendTransformInterpolator_vftable{};
