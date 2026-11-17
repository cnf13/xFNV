#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Builder/Mediator/hkpMoppMediator.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Builder/Splitter/hkpMoppSplitter.h"

#include <stdint.h>
#include <xmmintrin.h>

class hkpConvexShape;
class hkpShapeContainer;

// The MOPP mediator over a shape container that caches each child's extents along the splitting directions.
class hkpMoppCachedShapeMediator : public hkpMoppMediator {
public:
	// A child's shape key and its cached extents.
	struct hkpConvexShapeData;

	hkpMoppCachedShapeMediator(const hkpShapeContainer* a1);
	~hkpMoppCachedShapeMediator() override;

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

	// Havok SDK signature (hkpInternal.lib, tied by reloc).
	void addConvexShape(const hkpConvexShape* a1, uint32_t a2, const hkpMoppSplittingPlaneDirection* a3, int32_t a4);

	hkArray<hkpConvexShapeData> m_arrayConvexShapeData;
	const hkpShapeContainer* m_shapeCollection;
	int32_t m_numChildShapes;
};

// The child's shape key and its extents along each of the 13 splitting directions (0x6C bytes per child, the stride
// projectPrimitives and findExtents index m_arrayConvexShapeData with).
struct hkpMoppCachedShapeMediator::hkpConvexShapeData {
	uint32_t m_key;
	hkpMoppExtent m_extents[13];
};
