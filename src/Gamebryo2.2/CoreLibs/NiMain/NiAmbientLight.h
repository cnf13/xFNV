#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Reconstruction.hpp"

// a light that lights every surface evenly
class NiAmbientLight : public NiLight {
public:
	NiAmbientLight();
	~NiAmbientLight() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	// the stream overrides fold into jumps to the NiLight bodies
	void LoadBinary(NiStream& stream) override;
	void LinkObject(NiStream& stream) override;
	bool RegisterStreamables(NiStream& stream) override;
	void SaveBinary(NiStream& stream) override;
	bool IsEqual(NiObject* pObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) override;

	static NiObject* CreateObject();

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiAmbientLight_vftable{};
