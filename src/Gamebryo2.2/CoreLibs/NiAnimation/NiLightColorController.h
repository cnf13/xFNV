#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPoint3InterpController.h"

class NiPoint3;

// Animates the ambient or diffuse colour of its target light.
class NiLightColorController : public NiPoint3InterpController {
public:
	// The bit of m_uFlags set when the ambient colour is animated; the diffuse colour otherwise.
	enum {
		ISAMBIENT_MASK = 0x0001,
	};

	NiLightColorController();
	~NiLightColorController() override;

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
