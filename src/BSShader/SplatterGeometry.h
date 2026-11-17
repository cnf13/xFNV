#pragma once

#include "BSShader/BSShaderProperty.h"

class NiTriShape;

// One piece of blood splatter geometry and the render pass that draws it.
class SplatterGeometry {
public:
	NiTriShape* pGeometry;
	BSShaderProperty::RenderPass* pPass;
};
