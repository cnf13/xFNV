#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatInterpController.h"
#include "Reconstruction.hpp"

// Animates the field of view of its target camera's frustum.
class BSFrustumFOVController : public NiFloatInterpController {
public:
	BSFrustumFOVController();
	~BSFrustumFOVController() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void Update(NiUpdateData& UpdateData) override;

protected:
	bool InterpTargetIsCorrectType(NiObjectNET* pkTarget) const override;
	void GetTargetFloatValue(float& fValue) override;

public:
	static NiObject* CreateObject();

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable BSFrustumFOVController_vftable{};
