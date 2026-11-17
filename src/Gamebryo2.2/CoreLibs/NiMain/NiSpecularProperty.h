#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"

// Turns specular lighting on or off.
class NiSpecularProperty : public NiProperty {
public:
	NiSpecularProperty();
	~NiSpecularProperty() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	int32_t Type() const override { return SPECULAR; }

	static NiObject* CreateObject();

	uint16_t m_uFlags;

	static NiRTTI ms_RTTI;
	// Nothing is ever built into it; it is only released at exit.
	static NiPointer<NiSpecularProperty> ms_spDefault;
};
