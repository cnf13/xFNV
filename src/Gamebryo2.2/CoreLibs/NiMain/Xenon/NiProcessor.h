#pragma once

#include "Reconstruction.hpp"

#include <stdint.h>

class NiMatrix3;
class NiTransform;

// Inferred type: uiVerts: uint16_t: unsigned uses from 0xa74bf0
void TransformPointsBasic(uint16_t uiVerts, const float* pModel, float* pWorld, const NiTransform* pXform);
// Inferred type: uiVerts: uint16_t: unsigned uses from 0xa74ce0
void TransformVectorsBasic(uint16_t uiVerts, const float* pModel, float* pWorld, NiMatrix3* pMatrix);
void NiTransformVectorsPentium(uint16_t uiVerts, const float* pModel, float* pWorld, NiMatrix3* pMatrix);
void NiTransformPointsPentium(uint16_t uiVerts, const float* pModel, float* pWorld, const NiTransform* pXform);
