#pragma once

#include "BSShader/BSShaderProperty.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBillboardNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Reconstruction.hpp"

// The shader property of the rain and snow particles: how many, how fast they fall, the box they fill, and the
// texture and billboard mode they use.
class PrecipitationShaderProperty : public BSShaderProperty {
public:
	// Values from the Xbox PDB.
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
	// The D3DTEXTUREADDRESS of the texture (the PDB's _D3DTEXTUREADDRESS is the Xbox 360 SDK's; the exe's is the PC
	// SDK's, its value 3 is D3DTADDRESS_CLAMP at 0xBB0CF0).
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

// The vtable the exe's inlined constructor stores at offset 0.
inline constexpr fnvd::Vtable PrecipitationShaderProperty_vftable{};
