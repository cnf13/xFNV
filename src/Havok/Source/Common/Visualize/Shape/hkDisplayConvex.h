#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometry.h"
#include "Reconstruction.hpp"

// A convex shape the visual debugger displays from a ready-made geometry: the exe's constructor (0xDE1D20) stores the
// geometry it is given in m_geometry and type HK_DISPLAY_CONVEX, and adds no data.
class hkDisplayConvex : public hkDisplayGeometry {
public:
	// Stores geom with type HK_DISPLAY_CONVEX and the identity transform.
	hkDisplayConvex(hkGeometry* geom);

	void getWireframeGeometry(hkArrayBase<hkVector4>& lines, hkMemoryAllocator& a) override;
	// Empty in the exe (0xDE1D10): the geometry is given at construction.
	void buildGeometry() override;

	using hkDisplayGeometry::getWireframeGeometry;
};
inline constexpr fnvd::Vtable hkDisplayConvex_vftable{};
