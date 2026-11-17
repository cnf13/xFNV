#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometry.h"
#include "Reconstruction.hpp"

#include <xmmintrin.h>

// Builds the visual debugger geometry for a cone.
class hkDisplayCone : public hkDisplayGeometry {
public:
	hkDisplayCone();
	hkDisplayCone(float coneAngle, float coneHeight, int32_t numSegments, const hkVector4& coneAxis,
		const hkVector4& startPos);

	void getWireframeGeometry(hkArrayBase<hkVector4>& lines, hkMemoryAllocator& a) override;
	void buildGeometry() override;

	using hkDisplayGeometry::getWireframeGeometry;

	// Stores the cone's angle, height, segment count, axis and apex.
	void setParameters(float coneAngle, float coneHeight, int32_t numSegments, const hkVector4& coneAxis,
		const hkVector4& startPos);
	hkVector4* getPosition(hkVector4* apResult);
	hkVector4* getAxis(hkVector4* apResult);
	float getHeight();
	float getAngle();
	int32_t getNumSegments();
	// Resizes vertices to m_numSegments and fills it with the cone's base ring: the body reserves through
	// hkArrayUtil::_reserve (16-byte elements) and stores m_size, and buildGeometry passes its hkGeometry's
	// m_vertices.
	void generateConeVertices(hkArrayBase<hkVector4>& vertices);

	hkVector4 m_startPos;
	hkVector4 m_coneAxis;
	int32_t m_numSegments;
	float m_coneAngle;
	float m_coneHeight;
	uint8_t padding8C[4];
};
inline constexpr fnvd::Vtable hkDisplayCone_vftable{};
