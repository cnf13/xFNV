#pragma once

#include "BSShader/BSShaderPPLightingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"

// The per-pixel lit property of a SpeedTree's distant billboard.
class SpeedTreeBillboardShaderProperty : public BSShaderPPLightingProperty {
public:
	SpeedTreeBillboardShaderProperty();
	~SpeedTreeBillboardShaderProperty() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	BSShaderProperty* ClarifyShader(NiGeometry* apGeometry, bool abForceShader, bool abAllow30Shaders) override;

	void CopyMembers(SpeedTreeBillboardShaderProperty* pDest, NiCloningProcess& arCloneData);

	static NiRTTI ms_RTTI;
};
