#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/Geometry/Sphere/hkSphere.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometry.h"
#include "Reconstruction.hpp"

// A sphere the visual debugger displays, tessellated into m_xRes by m_yRes patches. The exe's constructor (0xE0AF50)
// stores type HK_DISPLAY_SPHERE and moves the transform to the sphere's centre.
class hkDisplaySphere : public hkDisplayGeometry {
public:
	hkDisplaySphere(const hkSphere& sphere, int32_t xRes, int32_t yRes);

	// Empty in the exe (0xE0AF40, ret 8).
	void getWireframeGeometry(hkArrayBase<hkVector4>& lines, hkMemoryAllocator& a) override;
	void buildGeometry() override;

	using hkDisplayGeometry::getWireframeGeometry;

	hkSphere m_sphere;
	int32_t m_xRes;
	int32_t m_yRes;
};
inline constexpr fnvd::Vtable hkDisplaySphere_vftable{};
