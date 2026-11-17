#pragma once

#include "BSShader/BSShaderProperty.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Reconstruction.hpp"

class NiAVObject;

// The shader property of the blood splatters drawn over the screen.
class BSShaderBloodSplatterProperty : public BSShaderProperty {
public:
	// The splatter textures.
	enum TEXTURE : int32_t {
		TEXTURE_BASE = 0,
		TEXTURE_SHADE = 1,
		TEXTURE_COUNT = 2,
	};

	// A splatter shape and its pass, recorded for debugging.
	struct DebugData {
		NiGeometry* pGeom;
		BSShaderProperty::RenderPass* pRenderPass;
	};

	// The first 100 shapes drawn since ResetShapeCount (uiShapeCount counts them).
	using DebugDataArray = DebugData[100];

	BSShaderBloodSplatterProperty();
	~BSShaderBloodSplatterProperty() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	bool SetupGeometry(NiGeometry*) override { return true; }
	BSShaderProperty::RenderPassArray* GetRenderPasses(NiGeometry* apGeom, int32_t aiEnabledPasses, uint16_t& asCount,
		uint32_t auiRenderMode, BSShaderAccumulator* apAccumulator, int32_t abUpdateList) override;
	void GetTextureUse(void (*apTextureUseCB)(NiTexture*, uint32_t, void*), void* apParam) override;
	// The D3DTEXTUREADDRESS of the textures (the PDB's _D3DTEXTUREADDRESS is the Xbox 360 SDK's; the exe's is the PC
	// SDK's, its value 3 is D3DTADDRESS_CLAMP at 0xB81110).
	virtual D3DTEXTUREADDRESS GetClampD3D() const { return D3DTADDRESS_WRAP; }
	virtual void SetClampD3D(D3DTEXTUREADDRESS aClampMode);
	// folded with NiDX9TextureData::GetLevels at 0xE687F0
	virtual NiTexturingProperty::ClampMode GetClampNI() const { return TextureClampMode; }
	virtual void SetClampNI(NiTexturingProperty::ClampMode aClampMode) { TextureClampMode = aClampMode; }

	// Holds apTex as texture auiIndex.
	void SetTexture(uint32_t auiIndex, NiTexture* apTex);
	// Copies the property into pDest.
	void CopyMembers(BSShaderBloodSplatterProperty* pDest, NiCloningProcess& arCloneData);

	// Points the fade alpha source of every blood splatter property in apObject's subtree at apfAlphaSource.
	static void SetFadeSourceRecurse(NiAVObject* apObject, float* apfAlphaSource);
	static void ResetShapeCount();

	NiPointer<NiTexture> spTexture[2];
	NiTexturingProperty::ClampMode TextureClampMode;
	float* pfFadeAlpha;

	static NiRTTI ms_RTTI;
	static inline int32_t uiShapeCount;
	static inline DebugDataArray pDD{};
};
inline constexpr fnvd::Vtable BSShaderBloodSplatterProperty_vftable{};
