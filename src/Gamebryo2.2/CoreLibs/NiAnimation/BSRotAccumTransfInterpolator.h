#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTransformInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Reconstruction.hpp"

// A transform interpolator that accumulates its rotation from one key to the next.
class BSRotAccumTransfInterpolator : public NiTransformInterpolator {
public:
	BSRotAccumTransfInterpolator();
	~BSRotAccumTransfInterpolator() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	// LoadBinary, RegisterStreamables, SaveBinary and IsEqual fold into jumps to the base bodies
	void LoadBinary(NiStream& arStream) override;
	void LinkObject(NiStream& arStream) override;
	bool RegisterStreamables(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	bool IsEqual(NiObject* apObject) override;
	bool Update(float afTime, NiObjectNET* apInterpTarget, NiQuatTransform& arValue) override;
	using NiInterpolator::Update;

	// The stream's factory: a new interpolator, PrevKeyRot the identity.
	static NiObject* CreateObject();

	NiQuaternion PrevKeyRot;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable BSRotAccumTransfInterpolator_vftable{};
