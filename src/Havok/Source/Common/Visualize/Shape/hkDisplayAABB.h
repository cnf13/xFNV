#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometry.h"
#include "Reconstruction.hpp"

// An axis-aligned box the visual debugger displays, given by its two corners. The exe's constructors (0xDE1190 with
// the corners, 0xDE1210 without) store type HK_DISPLAY_AABB.
class hkDisplayAABB : public hkDisplayGeometry {
public:
	hkDisplayAABB();
	hkDisplayAABB(const hkVector4& min, const hkVector4& max);
	~hkDisplayAABB() override;

	void getWireframeGeometry(hkArrayBase<hkVector4>& lines, hkMemoryAllocator& a) override;
	void buildGeometry() override;

	using hkDisplayGeometry::getWireframeGeometry;

	const hkVector4& getMinExtent();
	const hkVector4& getMaxExtent();
	void setExtents(const hkVector4& min, const hkVector4& max);

	hkVector4 m_minExtent;
	hkVector4 m_maxExtent;
};
inline constexpr fnvd::Vtable hkDisplayAABB_vftable{};
