#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/hkpMoppCompilerInput.h"

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"

#include <bit>
#include <xmmintrin.h>

// The SDK's default tolerances; padding21 is left as it was.
hkpMoppCompilerInput::hkpMoppCompilerInput()
  : m_relativeFitToleranceOfInternalNodes(0.4f), m_absoluteFitToleranceOfInternalNodes(0.1f),
    m_absoluteFitToleranceOfTriangles(0.3f), m_useShapeKeys(true), m_enablePrimitiveSplitting(true),
    m_enableChunkSubdivision(false), m_enableInterleavedBuilding(true), m_cachePrimitiveExtents(false) {
    m_absoluteFitToleranceOfAxisAlignedTriangles.m_quad = _mm_setr_ps(0.05f, 0.05f, 0.05f, 0.0f);
}

void hkpMoppCompilerInput::setAbsoluteFitToleranceOfAxisAlignedTriangles(const hkVector4& inTight) {
    m_absoluteFitToleranceOfAxisAlignedTriangles.m_quad = inTight.m_quad;
}

hkVector4 hkpMoppCompilerInput::getAbsoluteFitToleranceOfAxisAlignedTriangles() const {
    return m_absoluteFitToleranceOfAxisAlignedTriangles;
}

float hkpMoppCompilerInput::getAbsoluteFitToleranceOfTriangles() const {
    return m_absoluteFitToleranceOfTriangles;
}

float hkpMoppCompilerInput::getAbsoluteFitToleranceOfInternalNodes() const {
    return m_absoluteFitToleranceOfInternalNodes;
}

void hkpMoppCompilerInput::setRelativeFitToleranceOfInternalNodes(float a1) {
    m_relativeFitToleranceOfInternalNodes = a1;
}

float hkpMoppCompilerInput::getRelativeFitToleranceOfInternalNodes() const {
    return m_relativeFitToleranceOfInternalNodes;
}

void hkpMoppCompilerInput::setAbsoluteFitToleranceOfInternalNodes(float a1) {
    m_absoluteFitToleranceOfInternalNodes = a1;
}
