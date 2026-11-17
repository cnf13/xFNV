#pragma once

#include "BSShader/Lighting30ShaderProperty.h"
#include "Reconstruction.hpp"

// A Lighting30ShaderProperty that overrides the render passes and the specular and environment map LOD fades.
class NoFaderShaderProperty : public Lighting30ShaderProperty {
public:
	NoFaderShaderProperty();
	~NoFaderShaderProperty() override;

	BSShaderProperty::RenderPassArray* GetRenderPasses(NiGeometry* apGeom, int32_t aiEnabledPasses, uint16_t& asCount,
		uint32_t auiRenderMode, BSShaderAccumulator* apAccumulator, int32_t abUpdateList) override;
	float GetSpecularLODFade() override;
	float GetEnvMapLODFade() override;
};
inline constexpr fnvd::Vtable NoFaderShaderProperty_vftable{};
