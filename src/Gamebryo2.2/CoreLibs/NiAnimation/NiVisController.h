#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolInterpController.h"
#include "Reconstruction.hpp"

// Animates the visibility (app-culled flag) of its target.
class NiVisController : public NiBoolInterpController {
public:
	NiVisController();
	~NiVisController() override;

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
	void GetTargetBoolValue(bool& bValue) override;

public:
	static NiObject* CreateObject();

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiVisController_vftable{};
