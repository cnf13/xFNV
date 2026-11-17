#pragma once

#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkMatrix4;
class hkTransform;

// Not named in the PDB or IDA; name chosen here. The SDK's hkGeometryUtils::computeVolume (cdecl, float in ST0).
float computeVolume(const hkGeometry& geometry);
// The SDK's hkGeometryUtils::transformGeometry.
void transformGeometry(const hkMatrix4& transform, hkGeometry& geometryInOut);

// Static helpers that build and edit hkGeometry. Only what the exe's callers in src use is declared.
class hkGeometryUtils {
public:
	// Builds a capsule from start to end of the given radius, heightSamples rings along the axis and thetaSamples
	// vertices around it, transforms its vertices by transform and stores a new hkGeometry in geometryOut.
	// hkDisplayCapsule::buildGeometry passes its own m_geometry.
	static void createCapsuleGeometry(const hkVector4& start, const hkVector4& end, float radius, int32_t heightSamples,
		int32_t thetaSamples, const hkTransform& transform, hkGeometry*& geometryOut);
	// Havok SDK signature (hkGeometryUtilities.lib, tied by bytes).
	static void weldVertices(hkGeometry& meshGeometry, float threshold);
};
