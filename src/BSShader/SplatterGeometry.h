#pragma once

#include "BSShader/BSShaderProperty.h"
#include "Reconstruction.hpp"

class NiTriShape;

// One piece of blood splatter geometry and the render pass that draws it. IDA's names; the PDB has no such class.
class SplatterGeometry {
public:
	NiTriShape* pGeometry;
	BSShaderProperty::RenderPass* pPass;
};
