#pragma once

#include "Havok/Source/Physics/Internal/Collide/Mopp/Builder/Mediator/hkpMoppMediator.h"

class hkpShapeContainer;

// The MOPP mediator over a shape container: each child shape is one primitive.
class hkpMoppShapeMediator : public hkpMoppMediator {
public:
	hkpMoppShapeMediator(const hkpShapeContainer* a1);
	~hkpMoppShapeMediator() override;

	void setSplittingPlaneDirections(const hkpMoppSplittingPlaneDirection* directions, int32_t numDirections) override;
	int32_t getNumPrimitives() override;
	void getPrimitives(hkpMoppCompilerPrimitive* primitives) override;
	void projectPrimitives(const hkVector4& direction, int32_t directionIndex, hkpMoppCompilerPrimitive* primitives,
		int32_t numPrimitives, float* absMinOut, float* absMaxOut) override;
	void findExtents(const hkVector4& direction, int32_t directionIndex, const hkpMoppCompilerPrimitive* primitives,
		int32_t numPrimitives, float* absMinOut, float* absMaxOut) override;
	void splitPrimitive(const hkpMoppCompilerPrimitive& primitiveIn, const hkVector4& direction, float planeOffset,
		int32_t depth, hkpMoppCompilerPrimitive* primitiveOut) override;
	int32_t getPrimitiveProperties(const hkpMoppCompilerPrimitive& primitiveIn, uint32_t* cid) override;

	const hkpShapeContainer* m_shape;
	int32_t m_numChildShapes;
};
