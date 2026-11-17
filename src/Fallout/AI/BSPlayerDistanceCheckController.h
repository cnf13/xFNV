#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Reconstruction.hpp"

class NiAVObject;

// A controller that calls back once the player comes within fCheckDistance of its target.
class BSPlayerDistanceCheckController : public NiTimeController {
public:
	// The callback gets the controller's target (Update 0x8D1670 calls it cdecl with the target as NiAVObject).
	using Callback = void(NiAVObject*);

	BSPlayerDistanceCheckController();
	~BSPlayerDistanceCheckController() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void Update(NiUpdateData& UpdateData) override;
	bool TargetIsRequiredType() const override;

	float GetCheckDistance();
	void SetCheckDistance(float afDist);
	Callback* GetCallback() { return pfnCallback; }
	void SetCallback(Callback* apfnCallback) { pfnCallback = apfnCallback; }

protected:
	void CopyMembers(BSPlayerDistanceCheckController* pkDest, NiCloningProcess& kCloning);

public:
	float fCheckDistance;
	Callback* pfnCallback;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable BSPlayerDistanceCheckController_vftable{};
