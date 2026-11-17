#pragma once

#include "Reconstruction.hpp"

#include <xmmintrin.h>

class hkTransform;
class hkpConvexShape;
class hkpTriangleShape;

// The simplex the GJK distance algorithm ended on, kept between steps so that the next one starts from it: the
// vertices of the simplex on both shapes and their dimensions.
class hkpGskCache {
public:
	// The largest value m_maxDimA and m_maxDimB hold (4 bits; the SDK's value).
	enum MaxDim : int32_t {
		MAX_DIM_MAX = 15,
	};

	// Values from the Havok SDK's hkpGskCache::GskFlagValues.
	enum GskFlagValues : int32_t {
		GSK_FLAGS_SHAPE_A_IS_CYLINDER = 0x01,
		GSK_FLAGS_SHAPE_B_IS_CYLINDER = 0x02,
		GSK_FLAGS_REPLACE_SHAPE_A_WITH_CAPSULE = 0x04,
		GSK_FLAGS_REPLACE_SHAPE_B_WITH_CAPSULE = 0x08,
		GSK_FLAGS_CYLINDER_AGENT_FLAGS = 0x0F,
		GSK_FLAGS_DISABLE_CONTACT_TIMS = 0x10,
		GSK_FLAGS_CONVEX_LIST_IN_GSK_MODE = 0x20,
		GSK_FLAGS_PROCESS_FUNCTION_CALLED = 0x40,
		ONE_BIT_STILL_UNUSED = 0x80,
	};

	// Starts the cache from the shapes' first vertices, btoa placing shapeB in shapeA's space.
	void init(const hkpConvexShape* shapeA, const hkpConvexShape* shapeB, const hkTransform& aTb);
	// init for a triangle shapeB.
	void initTriangle(const hkpConvexShape* shapeA, const hkpTriangleShape* shapeB, const hkTransform& btoa);

	// Swaps the A and B sides: m_dimA with m_dimB, m_maxDimA with m_maxDimB, and the order of the vertex ids, so
	// that B's come first (the exe's out-of-line copy: thiscall, ret 0, 0xD2EB50).
	void flip() {
		uint8_t h;
		h = m_maxDimA;
		m_maxDimA = m_maxDimB;
		m_maxDimB = h;
		h = m_dimA;
		m_dimA = m_dimB;
		m_dimB = h;
		int32_t last = m_dimA + m_dimB - 1;
		uint16_t k;
		k = m_vertices[0];
		m_vertices[0] = m_vertices[last];
		m_vertices[last] = k;
		if ((m_dimA & m_dimB) == 2) {
			k = m_vertices[1];
			m_vertices[1] = m_vertices[2];
			m_vertices[2] = k;
		}
	}

	uint16_t m_vertices[4];
	uint8_t m_dimA;
	uint8_t m_dimB;
	uint8_t m_maxDimA : 4;
	uint8_t m_maxDimB : 4;
	uint8_t m_gskFlags;
};
