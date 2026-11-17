#pragma once

#include "BSShader/BSShaderProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Reconstruction.hpp"

// The shader property of volumetric fog: the fog colour and, in alpha, its density.
class VolumetricFogShaderProperty : public BSShaderProperty {
public:
	VolumetricFogShaderProperty();
	~VolumetricFogShaderProperty() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;

protected:
	void CopyMembers(VolumetricFogShaderProperty* pDest, NiCloningProcess& arCloneData);

public:
	// The factory the stream registration holds: a property in a 0x70-byte engine block.
	static NiObject* CreateObject();

	NiColorA FogProps;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable VolumetricFogShaderProperty_vftable{};
