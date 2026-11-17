#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

// The tolerances and options the MOPP code generator is run with.
class hkpMoppCompilerInput {
public:
	hkpMoppCompilerInput();

	// The SDK's setter: copies inTight into m_absoluteFitToleranceOfAxisAlignedTriangles.
	void setAbsoluteFitToleranceOfAxisAlignedTriangles(const hkVector4& inTight);
	hkVector4 getAbsoluteFitToleranceOfAxisAlignedTriangles() const;
	float getAbsoluteFitToleranceOfTriangles() const;
	float getAbsoluteFitToleranceOfInternalNodes() const;
	void setRelativeFitToleranceOfInternalNodes(float a1);
	float getRelativeFitToleranceOfInternalNodes() const;
	void setAbsoluteFitToleranceOfInternalNodes(float a1);

	hkVector4 m_absoluteFitToleranceOfAxisAlignedTriangles;
	float m_relativeFitToleranceOfInternalNodes;
	float m_absoluteFitToleranceOfInternalNodes;
	float m_absoluteFitToleranceOfTriangles;
	hkBool m_useShapeKeys;
	hkBool m_enablePrimitiveSplitting;
	hkBool m_enableChunkSubdivision;
	hkBool m_enableInterleavedBuilding;
	hkBool m_cachePrimitiveExtents;
	uint8_t padding21[15];
};
