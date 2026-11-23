#pragma once

#include <stdint.h>

class NiMatrix3;
class NiTransform;

void TransformPointsBasic(uint16_t uiVerts, const float* pModel, float* pWorld, const NiTransform* pXform);
void TransformVectorsBasic(uint16_t uiVerts, const float* pModel, float* pWorld, NiMatrix3* pMatrix);
void NiTransformVectorsPentium(uint16_t uiVerts, const float* pModel, float* pWorld, NiMatrix3* pMatrix);
void NiTransformPointsPentium(uint16_t uiVerts, const float* pModel, float* pWorld, const NiTransform* pXform);
