#pragma once

#include "BSShader/BSShader.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceShaderParam.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class NiD3DPixelShader;
class NiD3DVertexShader;

// A full-screen post-processing shader: a BSShader paired with an ImageSpaceEffect, holding one vertex and one pixel
// shader plus the ImageSpaceShaderParam constants it uploads.
class ImageSpaceShader : public BSShader, public ImageSpaceEffect {
public:
	// Values from the Xbox PDB.
	enum TextureFlagsEnum : int32_t {
		TEXTURE_FLAG_STATIC = 0x1,
		TEXTURE_FLAG_NOTEXRATIO = 0x2,
	};

	ImageSpaceShader();
	~ImageSpaceShader() override;

	// The ImageSpaceEffect Render slot, which the exe reaches only through the shader's secondary vtable at +0x6C:
	// draws the effect's screen geometry with the effect's programs and output texture.
	void Render(NiScreenElements* apGeometry, NiDX9Renderer* apRenderer, ImageSpaceEffectParam* apParam,
		int32_t abEndFrame) override;
	bool SetupGeometry(NiGeometry* apGeometry) override;
	void SetupGeometryTextures(const NiPropertyState* apState) override;
	void SetupGeometryConstants(const NiPropertyState* apState) override;
	void PostGeometry(const NiPropertyState* apState) override;
	void InitPass(uint32_t auiCurrentPass) override;
	// The exe's slot holds _purecall (0xEC60FA): every image-space shader loads its own programs.
	void ReloadShaders() override = 0;
	// Compiles the vertex shader aVShader and, with a pixel target, the pixel shader aPShader.
	virtual void LoadShaders(BSShaderMacro& aVShader, BSShaderMacro& aPShader, const char* apcName,
		const char* apcVTarget, const char* apcPTarget);
	virtual void SetupVertexStream();
	// PDB UpdateParams(apParam); IDA drops the parameter.
	virtual void UpdateParams(ImageSpaceShaderParam* apParam) {}
	virtual void SetConstantMap(ImageSpaceShaderParam* apParam);
	virtual bool LoadStagesAndPasses();
	virtual void SetState() {}

	// Uploads the texture offsets of the effect's inputs to its parameters (thiscall, ret 0, 0xC03C80). IDA's name;
	// the PDB does not name this entry.
	void SetVSConstants();

	// The exe's deleting destructors free shaders with NiMemObject's delete; declared here because the
	// ImageSpaceEffect base declares its own (the MemoryManager), which would make delete ambiguous.
	static void operator delete(void* apMemory, size_t aSize) { NiMemObject::operator delete(apMemory, aSize); }

	NiPointer<NiD3DVertexShader> spVertexShader;
	NiPointer<NiD3DPixelShader> spPixelShader;
	ImageSpaceShaderParam* pParam;

	// The shader parameters an effect renders with when its caller passes none (PDB: ImageSpaceShader::DefaultParam).
	static ImageSpaceShaderParam DefaultParam;
};
// The table of its ImageSpaceEffect subobject (at 0x6C).
inline constexpr fnvd::Vtable ImageSpaceShader_vftable_ImageSpaceEffect{};
inline constexpr fnvd::Vtable ImageSpaceShader_vftable{};
