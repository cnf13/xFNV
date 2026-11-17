#pragma once

#include "BSCore/BSStringT.h"
#include "BSShader/BSShaderProperty.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class NiColorA;

// A shader property that draws a texture without lighting.
class BSShaderNoLightingProperty : public BSShaderProperty {
public:
	struct FallOffData {
		float fFalloffStartAngle;
		float fFalloffStopAngle;
		float fFalloffStartOpacity;
		float fFalloffStopOpacity;
	};

	BSShaderNoLightingProperty();
	~BSShaderNoLightingProperty() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	void PostLinkObject(NiStream& kStream) override;
	bool SetupGeometry(NiGeometry* apGeometry) override;
	BSShaderProperty::RenderPassArray* GetRenderPasses(NiGeometry* apGeom, int32_t aiEnabledPasses, uint16_t& asCount,
		uint32_t auiRenderMode, BSShaderAccumulator* apAccumulator, int32_t abUpdateList) override;
	NiTexture* GetBaseTexture() override { return spBaseMap; }
	BSShaderProperty::RenderPassArray* GetWaterFogPassList(NiGeometry* apGeom) override;
	void GetTextureUse(void (*apTextureUseCB)(NiTexture*, uint32_t, void*), void* apParam) override;
	void PrecacheTextures() override;
	// The D3DTEXTUREADDRESS of the texture (the PDB's _D3DTEXTUREADDRESS is the Xbox 360 SDK's; the exe's is the PC
	// SDK's, its value 3 is D3DTADDRESS_CLAMP at 0xB6F480).
	virtual D3DTEXTUREADDRESS GetClampD3D() const { return D3DTADDRESS_WRAP; }
	virtual void SetClampD3D(D3DTEXTUREADDRESS aClampMode);
	virtual NiTexturingProperty::ClampMode GetClampNI() const { return TextureClampMode; }
	virtual void SetClampNI(NiTexturingProperty::ClampMode aClampMode) { TextureClampMode = aClampMode; }

	// Copies the property into pDest.
	void CopyMembers(BSShaderNoLightingProperty* pDest, NiCloningProcess& arCloneData);
	void SetBaseMap(NiTexture* apBaseMap);
	void SetFalloffData(const NiColorA& aFalloffData);

	static void RegisterRTTI();
	// The stream factory: a property in a new 0x80-byte block.
	static NiObject* CreateObject();
	// Sets aFalloffData on the no-lighting shader properties under apObject.
	static void SetFalloffRecurse(NiAVObject* apObject, const NiColorA& aFalloffData);

	NiPointer<NiTexture> spBaseMap;
	BSStringT<char> TextureFilename;
	NiTexturingProperty::ClampMode TextureClampMode;
	FallOffData FalloffData;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable BSShaderNoLightingProperty_vftable{};
