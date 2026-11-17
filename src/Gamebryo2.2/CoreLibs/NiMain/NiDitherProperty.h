#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Reconstruction.hpp"

// Turns dithering on or off.
class NiDitherProperty : public NiProperty {
public:
	NiDitherProperty();
	~NiDitherProperty() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	int32_t Type() const override { return DITHER; }

	static NiObject* CreateObject();

	uint16_t m_uFlags;

	static NiRTTI ms_RTTI;
	// The PDB's; the exe builds nothing into it: its initialiser only registers the release at exit (0xFDB200).
	static NiPointer<NiDitherProperty> ms_spDefault;
};
inline constexpr fnvd::Vtable NiDitherProperty_vftable{};
