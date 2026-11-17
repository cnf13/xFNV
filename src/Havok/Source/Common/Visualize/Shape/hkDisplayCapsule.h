#pragma once

#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometry.h"
#include "Reconstruction.hpp"

// A capsule the visual debugger displays: two end points and a radius, drawn with a number of sides and height
// segments.
class hkDisplayCapsule : public hkDisplayGeometry {
public:
	hkDisplayCapsule(const hkVector4& top, const hkVector4& bottom, float radius, int32_t numSides,
		int32_t numHeightSegments);

	void buildGeometry() override;
	void getWireframeGeometry(hkArrayBase<hkVector4>& lines, hkMemoryAllocator& a) override;

	const hkVector4& getTop() { return m_top; }
	const hkVector4& getBottom() { return m_bottom; }
	float getRadius() { return m_radius; }
	int32_t getNumHeightSamples() { return m_numHeightSegments; }
	int32_t getNumSides() { return m_numSides; }

	hkVector4 m_top;
	hkVector4 m_bottom;
	float m_radius;
	int32_t m_numSides;
	int32_t m_numHeightSegments;
};
inline constexpr fnvd::Vtable hkDisplayCapsule_vftable{};
