#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

// Havok's helpers for the closest points between line segments. It only groups functions and result types; it has no
// members.
class hkLineSegmentUtil {
public:
	// The closest points of two line segments: on segment A, the vector to the one on segment B and the squared
	// distance. 16-byte aligned.
	struct ClosestLineSegLineSegResult {
		hkVector4 m_closestPointA;
		hkVector4 m_closestAminusClosestB;
		hkPadSpu<float> m_distanceSquared;
	};

	// The closest point of a line segment to a point.
	struct ClosestPointLineSegResult {
		hkVector4 m_pointOnEdge;
	};

	// The closest points of two infinite lines: the squared distance, each point's fraction along its line and the
	// two points. The SSE build's 16-byte vectors put them at 0x10 and 0x20.
	struct ClosestPointInfLineInfLineResult {
		hkPadSpu<float> m_distanceSquared;
		hkPadSpu<float> m_fractionA;
		hkPadSpu<float> m_fractionB;
		hkVector4 m_closestPointA;
		hkVector4 m_closestPointB;
	};

	// The closest points of the segments A to A + dA and B to B + dB; nonzero when they are
	// parallel.
	static int32_t closestLineSegLineSeg(const hkVector4& A, const hkVector4& dA, const hkVector4& B,
		const hkVector4& dB, ClosestLineSegLineSegResult& result);
	static int32_t closestPointLineSeg(const hkVector4& A, const hkVector4& a2, const hkVector4& a3,
		hkLineSegmentUtil::ClosestPointLineSegResult& a4);
};
