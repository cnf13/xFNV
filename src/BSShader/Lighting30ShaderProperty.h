#pragma once

#include "BSShader/BSShaderPPLightingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"

// The shader model 3.0 variant of the per-pixel lit property (all lights in one pass).
class Lighting30ShaderProperty : public BSShaderPPLightingProperty {
public:
	Lighting30ShaderProperty();
	Lighting30ShaderProperty(BSShaderPPLightingProperty* aPPLightingProp);
	~Lighting30ShaderProperty() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject*) override { return false; }
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	void PostLinkObject(NiStream& kStream) override;
	bool SetupGeometry(NiGeometry* apGeometry) override;
	BSShaderProperty::RenderPassArray* GetRenderPasses(NiGeometry* apGeom, int32_t aiEnabledPasses, uint16_t& asCount,
		uint32_t auiRenderMode, BSShaderAccumulator* apAccumulator, int32_t abUpdateList) override;
	uint16_t GetNumberofPasses(NiGeometry*) override { return 0; }
	BSShaderProperty::RenderPass* GetRenderDepthPass(NiGeometry* apGeom) override;
	BSShaderProperty::RenderPassArray* GetWaterFogPassList(NiGeometry* apGeom) override;

	void CopyMembers(Lighting30ShaderProperty* pDest, NiCloningProcess& arCloneData);

	// The stream's factory: the per-pixel constructor in a 0x108-byte engine block, or null when the block could
	// not be had.
	static NiObject* CreateObject();

	// A 32-bit flag on PC.
	uint32_t bLightsTransformed;

	static NiRTTI ms_RTTI;
};
