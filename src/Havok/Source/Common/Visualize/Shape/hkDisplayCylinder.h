#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometry.h"
#include "Reconstruction.hpp"

// A cylinder the visual debugger displays: the centres of its two caps and a radius, drawn with a number of sides and
// height segments. The Havok 7.1 SDK has no such header; the layout is the PC PDB's, matching the exe's constructor
// (0xD58510), which stores type HK_DISPLAY_CYLINDER.
class hkDisplayCylinder : public hkDisplayGeometry {
public:
	hkDisplayCylinder(const hkVector4& top, const hkVector4& bottom, float radius, int32_t numSides,
		int32_t numHeightSegments);

	// Empty in the exe (0xD58500, ret 8).
	void getWireframeGeometry(hkArrayBase<hkVector4>& lines, hkMemoryAllocator& a) override;
	void buildGeometry() override;

	using hkDisplayGeometry::getWireframeGeometry;

	hkVector4 m_top;
	hkVector4 m_bottom;
	float m_radius;
	int32_t m_numSides;
	int32_t m_numHeightSegments;
};
inline constexpr fnvd::Vtable hkDisplayCylinder_vftable{};
