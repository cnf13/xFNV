#include "Havok/Source/Common/Visualize/Shape/hkDisplayCapsule.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"
#include "Havok/Source/Common/GeometryUtilities/Misc/hkGeometryUtils.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometry.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplayGeometryTypes.h"

#include <xmmintrin.h>

hkDisplayCapsule::hkDisplayCapsule(const hkVector4& top, const hkVector4& bottom, float radius, int32_t numSides,
	int32_t numHeightSegments)
	: hkDisplayGeometry(HK_DISPLAY_CAPSULE), m_top(top), m_bottom(bottom), m_radius(radius), m_numSides(numSides),
	  m_numHeightSegments(numHeightSegments) {}

void hkDisplayCapsule::buildGeometry() {
	hkGeometryUtils::createCapsuleGeometry(m_top, m_bottom, this->getRadius(), this->getNumHeightSamples(),
		this->getNumSides(), m_transform, m_geometry);
}

void hkDisplayCapsule::getWireframeGeometry(hkArrayBase<hkVector4>& lines, hkMemoryAllocator& a) {
	;
}
