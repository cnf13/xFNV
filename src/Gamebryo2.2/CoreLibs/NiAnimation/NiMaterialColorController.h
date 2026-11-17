#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPoint3InterpController.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class NiPoint3;

// Animates one colour of its target's material.
class NiMaterialColorController : public NiPoint3InterpController {
public:
	// The colour animated, in the low bits of m_uFlags.
	enum Field {
		AMB,
		DIFF,
		SPEC,
		SELF_ILLUM
	};

	enum {
		FIELD_MASK = 0x0007,
		FIELD_POS = 0,
	};

	NiMaterialColorController();
	~NiMaterialColorController() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void Update(NiUpdateData& UpdateData) override;
	const char* GetCtlrID() override;

protected:
	bool InterpTargetIsCorrectType(NiObjectNET* pkTarget) const override;
	void GetTargetPoint3Value(NiPoint3& kValue) override;

public:
	static NiObject* CreateObject();

	// which colour is animated
	uint16_t m_uFlags;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiMaterialColorController_vftable{};
