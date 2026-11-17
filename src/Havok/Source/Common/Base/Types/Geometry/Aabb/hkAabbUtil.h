#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"

class hkMotionState;

// Havok's bounding box helpers.
class hkAabbUtil {
public:
	// What sweeping a box along a motion needs: the motion state, the in-between transforms, the start transform
	// and the inverse of the end one. The SSE build's 16-aligned transforms make it 0x1A0 bytes.
	struct OffsetAabbInput {
		hkPadSpu<const hkMotionState*> m_motionState;
		hkTransform m_transforms[4];
		hkPadSpu<int32_t> m_numTransforms;
		hkTransform m_startTransform;
		hkTransform m_endTransformInv;
	};

	// The box around the numVertices points vectorArray points at; with no points, the inverted box from HK_REAL_MAX to
	// -HK_REAL_MAX. Returns &aabbOut.
	static hkAabb* calcAabb(const hkVector4* const* vectorArray, int32_t numVertices, hkAabb& aabbOut);
	// The box around the numVertices points vertices; with no points, the inverted box from HK_REAL_MAX to
	// -HK_REAL_MAX. Returns &aabbOut.
	static hkAabb* calcAabb(const hkVector4* vertices, int32_t numVertices, hkAabb& aabbOut);
	// The box around numVertices points striding bytes apart from vertexArray, w cleared; the first point is read
	// whatever numVertices is. Returns &aabbOut.
	static hkAabb* calcAabb(const float* vertexArray, int32_t numVertices, int32_t striding, hkAabb& aabbOut);
	// The box center +- halfExtents under localToWorld, grown by extraRadius.
	static void calcAabb(const hkTransform& localToWorld, const hkVector4& halfExtents, const hkVector4& center,
		float extraRadius, hkAabb& aabbOut);
	// The box the transform's translation +- its rotation applied to halfExtents, every axis grown by extraRadius.
	static void calcAabb(const hkTransform& localToWorld, const hkVector4& halfExtents, float extraRadius,
		hkAabb& aabbOut);
	// The squared distance from b to the box a, 0 when a contains b.
	static float distanceSquared(const hkAabb& a, const hkVector4& b);
};
