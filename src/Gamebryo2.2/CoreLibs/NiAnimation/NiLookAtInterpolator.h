#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiQuatTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class NiAVObject;
class NiCloningProcess;

// Turns a transform to look at a target object; sub-interpolators supply its translation, roll and scale.
class NiLookAtInterpolator : public NiInterpolator {
public:
	// The axis that looks at the target.
	enum Axis {
		X,
		Y,
		Z,
	};

	// m_uFlags bits: the look direction is not negated when flipped; the Axis sits above it.
	enum {
		FLIP_MASK = 0x1,
		AXIS_MASK = 0x6,
		AXIS_POS = 1,
	};

	NiLookAtInterpolator(NiAVObject* pkLookAt, NiLookAtInterpolator::Axis eAxis, bool bFlip);
	~NiLookAtInterpolator() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void ProcessClone(NiCloningProcess& kCloning) override;
	bool Update(float fTime, NiObjectNET* pkInterpTarget, NiQuatTransform& kValue) override;
	bool IsTransformValueSupported() const override { return true; }
	void Collapse() override;
	void GetActiveTimeRange(float& fBeginKeyTime, float& fEndKeyTime) const override;
	void GuaranteeTimeRange(float fStartTime, float fEndTime) override;
	NiInterpolator* GetSequenceInterpolator(float fStartTime, float fEndTime) override;
	bool ResolveDependencies(NiAVObjectPalette* pkPalette) override;
	bool SetUpDependencies() override;
	bool AlwaysUpdate() const override { return true; }
	using NiInterpolator::Update;

	// Holds pkInterp as the translation (slot 0) or scale (slot 2) interpolator; null releases the current one.
	void SetTranslateInterpolator(NiInterpolator* pkInterp);
	void SetScaleInterpolator(NiInterpolator* pkInterp);
	// Takes kName as the look-at target's name, its handle's reference counted.
	void SetLookAtName(const NiFixedString& kName);
	void SetRollInterpolator(NiInterpolator* pkInterp);
	// Copies the base part, then the flags, the look-at target, an empty name, the transform and clones of the three
	// interpolators. CreateClone calls it on this with the new object.
	void CopyMembers(NiLookAtInterpolator* pkDest, NiCloningProcess& kCloning);

	static NiObject* CreateObject();

	uint16_t m_uFlags;
	NiAVObject* m_pkLookAt;
	NiFixedString m_kLookAtName;
	NiQuatTransform m_kTransformValue;
	NiPointer<NiInterpolator> m_aspInterpolators[3];

	static NiRTTI ms_RTTI;
};
