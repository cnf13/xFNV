#pragma once

#include "BSShader/BSShaderLightingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Reconstruction.hpp"

// The shader property of a decal projected onto skinned geometry.
class GeometryDecalShaderProperty : public BSShaderLightingProperty {
public:
	GeometryDecalShaderProperty();
	~GeometryDecalShaderProperty() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	bool SetupGeometry(NiGeometry*) override { return true; }
	BSShaderProperty::RenderPassArray* GetRenderPasses(NiGeometry* apGeom, int32_t aiEnabledPasses, uint16_t& asCount,
		uint32_t auiRenderMode, BSShaderAccumulator* apAccumulator, int32_t abUpdateList) override;

	void CopyMembers(GeometryDecalShaderProperty* pDest, NiCloningProcess& arCloneData);

	NiPointer<NiSourceTexture> spDecalTexture;
	int32_t iGeometryIndex;
	NiTransform pProjection;
	uint32_t iDestFormID;
	float fTimer;
	uint8_t cSubTexture;
	uint8_t paddingC1[3];
	NiPoint3 fPosition;
	float fRadius;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable GeometryDecalShaderProperty_vftable{};
