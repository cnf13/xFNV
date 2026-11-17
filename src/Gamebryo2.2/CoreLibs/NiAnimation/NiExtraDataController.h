#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiSingleInterpController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

class NiExtraData;

// A single-interpolator controller that animates a named extra data of its target.
class NiExtraDataController : public NiSingleInterpController {
public:
	NiExtraDataController();
	~NiExtraDataController() override;

	const NiRTTI* GetRTTI() const override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void ProcessClone(NiCloningProcess& kCloning) override;
	void PostLinkObject(NiStream& kStream) override;
	void SetTarget(NiObjectNET* pkTarget) override;
	const char* GetCtlrID() override { return m_kExtraDataName; }

protected:
	bool InterpTargetIsCorrectType(NiObjectNET* pkTarget) const override;

	// The NiSingleInterpController members, then the name (used by the subclasses' CreateClone).
	void CopyMembers(NiExtraDataController* pkDest, NiCloningProcess& kCloning);

	// Points m_spExtraData at the target's extra data named m_kExtraDataName.
	void GetExtraDataPointerFromName();

public:
	NiFixedString m_kExtraDataName;
	NiPointer<NiExtraData> m_spExtraData;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiExtraDataController_vftable{};
