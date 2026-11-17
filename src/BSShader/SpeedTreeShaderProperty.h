#pragma once

#include "BSShader/BSShaderLightingProperty.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Reconstruction.hpp"

// The wind data a SpeedTree property shares with its tree model (constructor 0xBB33C0, vtable 0x10B9488).
class STSPData : public NiRefObject {
public:
	STSPData();
	~STSPData() override;

	// The exe's callers (0x6682B2, 0x6692EF) ignore eax, as the PDB's void has it.
	void SetData(D3DXVECTOR4* apWindInfo, uint16_t asVertexCount);

	D3DXVECTOR4* pWindInfo;
	uint16_t sVertexCount;
	uint8_t padding0E[2];
};

// The property that selects the SpeedTree shader and its lighting for a tree model.
class SpeedTreeShaderLightingProperty : public BSShaderLightingProperty {
public:
	SpeedTreeShaderLightingProperty(STSPData* apData);
	~SpeedTreeShaderLightingProperty() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	BSShaderProperty* ClarifyShader(NiGeometry* apGeometry, bool abForceShader, bool abAllow30Shaders) override;
	NiTexture* GetBaseTexture() override { return spBaseTexture; }
	void GetTextureUse(void (*apTextureUseCB)(NiTexture*, uint32_t, void*), void* apParam) override;
	virtual STSPData* GetData() { return spData; }
	virtual D3DXVECTOR4* GetWindInfo();
	virtual uint32_t GetVertexCount();
	virtual void SetData(D3DXVECTOR4* apWindInfo, uint32_t aiWindInfoCount);
	virtual void SetBaseTexture(NiTexture* apBaseTexture);
	// The D3DTEXTUREADDRESS of the textures (the PDB's _D3DTEXTUREADDRESS is the Xbox 360 SDK's; the exe's is the PC
	// SDK's, its value 3 is D3DTADDRESS_CLAMP at 0xBB33A0).
	virtual D3DTEXTUREADDRESS GetClampD3D() const { return D3DTADDRESS_WRAP; }
	virtual void SetClampD3D(D3DTEXTUREADDRESS aClampMode);
	virtual NiTexturingProperty::ClampMode GetClampNI() const { return TextureClampMode; }
	virtual void SetClampNI(NiTexturingProperty::ClampMode aClampMode) { TextureClampMode = aClampMode; }
	virtual SpeedTreeShaderLightingProperty* CreateCopy() = 0;

	NiPointer<NiTexture> spBaseTexture;
	NiTexturingProperty::ClampMode TextureClampMode;
	NiPointer<STSPData> spData;

	static NiRTTI ms_RTTI;
};

// The per-pixel lit variant: the base of the branch shader property.
class SpeedTreeShaderPPLightingProperty : public BSShaderPPLightingProperty {
public:
	SpeedTreeShaderPPLightingProperty(STSPData* a1);
	~SpeedTreeShaderPPLightingProperty() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	BSShaderProperty* ClarifyShader(NiGeometry* apGeometry, bool abForceShader, bool abAllow30Shaders) override;
	virtual STSPData* GetData() { return spData; }
	virtual D3DXVECTOR4* GetWindInfo();
	virtual uint16_t GetVertexCount();
	virtual void SetData(D3DXVECTOR4* apWindInfo, uint32_t asVertexCount);
	virtual SpeedTreeShaderPPLightingProperty* CreateCopy() = 0;

	NiPointer<STSPData> spData;

	static NiRTTI ms_RTTI;
};

inline constexpr fnvd::Vtable STSPData_vftable{};
inline constexpr fnvd::Vtable SpeedTreeShaderLightingProperty_vftable{};
inline constexpr fnvd::Vtable SpeedTreeShaderPPLightingProperty_vftable{};
