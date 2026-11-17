#pragma once

#include "BSShader/BSShaderProperty.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBillboardNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"

// The shader property of the rain and snow particles: how many, how fast they fall, the box they fill, and the
// texture and billboard mode they use.
class PrecipitationShaderProperty : public BSShaderProperty {
public:
	enum PrecipitationType : int32_t {
		PRECIPTYPE_RAIN = 0x0,
		PRECIPTYPE_SNOW = 0x1,
		PRECIPTYPE_MAX = 0x2,
	};

	PrecipitationShaderProperty();
	PrecipitationShaderProperty(int32_t aiParticleCount, float afRotationSpeed, NiPoint3& akVelocity, NiPoint3& akMin,
		NiPoint3& akMax, PrecipitationShaderProperty::PrecipitationType aePrecipType,
		NiBillboardNode::FaceMode aeFaceMode);
	~PrecipitationShaderProperty() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	bool SetupGeometry(NiGeometry* apGeometry) override;
	BSShaderProperty::RenderPassArray* GetRenderPasses(NiGeometry* apGeom, int32_t aiEnabledPasses, uint16_t& asCount,
		uint32_t auiRenderMode, BSShaderAccumulator* apAccumulator, int32_t abUpdateList) override;
	NiTexture* GetBaseTexture() override { return spBaseTexture; }
	// The D3DTEXTUREADDRESS of the texture, using the PC SDK's values (3 is D3DTADDRESS_CLAMP).
	virtual D3DTEXTUREADDRESS GetClampD3D() const;
	virtual void SetClampD3D(D3DTEXTUREADDRESS aClampMode);
	virtual NiTexturingProperty::ClampMode GetClampNI() const;
	virtual void SetClampNI(NiTexturingProperty::ClampMode aClampMode);

	// Copies this property's members into pDest while cloning.
	void CopyMembers(PrecipitationShaderProperty* pDest, NiCloningProcess& arCloneData);

	int32_t iParticleCount;
	float fRenderPCT;
	float fRotationSpeed;
	NiPoint3 kMin;
	NiPoint3 kMax;
	NiPoint3 kVelocity;
	NiPointer<NiTexture> spBaseTexture;
	NiTexturingProperty::ClampMode TextureClampMode;
	NiBillboardNode::FaceMode eFaceMode;
	PrecipitationType ePrecipType;

	static NiRTTI ms_RTTI;
};
