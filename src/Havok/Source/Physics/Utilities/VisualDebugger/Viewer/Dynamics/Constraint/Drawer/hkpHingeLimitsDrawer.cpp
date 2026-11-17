#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpHingeLimitsDrawer.h"

#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/Color/hkColor.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"
#include "Havok/Source/Common/Visualize/Shape/hkDisplaySemiCircle.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/HingeLimits/hkpHingeLimitsData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpConstraintDrawer.h"

#include <xmmintrin.h>

hkpHingeLimitsDrawer::hkpHingeLimitsDrawer() : hkpConstraintDrawer(), m_angularLimit() {}

// The vector code is written as lane operations with intrinsics, each operation's operands in the game's order. The
// line lengths and the arc radius are floats; the factors 0.75, -0.75 and 0.5 are doubles. The normalize3 Newton step's
// constants 0.0, 3.0 and 0.5 are the SDK's. The lines' start points are copies of the pivots.
void hkpHingeLimitsDrawer::drawConstraint(hkpConstraintInstance* constraint, hkDebugDisplayHandler* displayHandler,
	int32_t id, int32_t tag) {
	const hkpHingeLimitsData* hingeLimits = static_cast<const hkpHingeLimitsData*>(constraint->m_data);
	m_primitiveDrawer.setDisplayHandler(displayHandler);

	alignas(16) hkTransform refLocalToWorld;
	alignas(16) hkTransform attLocalToWorld;
	updateCommonParameters(constraint, refLocalToWorld, attLocalToWorld);
	m_bodyBWPivot.setTransformedPos(refLocalToWorld, hkVector4Zero);
	m_bodyAWPivot.setTransformedPos(attLocalToWorld, hkVector4Zero);
	drawBodyFrames(id, tag);
	drawPivots(id, tag);

	const hkRotation& baseA = hingeLimits->m_atoms.m_rotations.m_rotationA;
	const hkRotation& baseB = hingeLimits->m_atoms.m_rotations.m_rotationB;
	alignas(16) hkVector4 axisInWorld;
	axisInWorld.setRotatedDir(m_RB, baseB.m_col0);
	alignas(16) hkVector4 perpInWorld;
	perpInWorld.setRotatedDir(m_RB, baseB.m_col2);

	// The error line between the pivots.
	displayHandler->displayLine(m_bodyAWPivot, m_bodyBWPivot, static_cast<int32_t>(hkColor::RED), id, tag);

	// The hinge axis through the B pivot.
	float lineLength = m_scale * m_lineLength;
	const float axisPlus = static_cast<float>(0.75 * lineLength);
	const float axisMinus = static_cast<float>(lineLength * -0.75);
	// EXE-INLINED none: hkVector4::setAddMul4
	alignas(16) hkVector4 axisEnd;
	axisEnd.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(axisPlus), axisInWorld.m_quad), m_bodyBWPivot.m_quad);
	// EXE-INLINED none: hkVector4::setAddMul4
	alignas(16) hkVector4 axisStart;
	axisStart.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(axisMinus), axisInWorld.m_quad), m_bodyBWPivot.m_quad);
	displayHandler->displayLine(axisStart, axisEnd, hkColor::rgbFromFloats(0.0f, 0.5f, 1.0f, 1.0f), id, tag);

	// The angular limit's arc.
	const float radius = m_arcRadius * m_scale;
	m_angularLimit.setParameters(radius, hingeLimits->m_atoms.m_angLimit.m_minAngle,
		hingeLimits->m_atoms.m_angLimit.m_maxAngle, m_numArcSegments, m_bodyBWPivot, axisInWorld, perpInWorld);

	// Raw storage: the body frees the storage itself, so no C++ destructor may free it again.
	alignas(16) FNVD_INLINED_LOCAL(hkArray<hkDisplayGeometry*>, geometry, std::nullopt, std::nullopt);
	geometry.m_data = nullptr;
	geometry.m_size = 0;
	geometry.m_capacityAndFlags = hkArrayBase<hkDisplayGeometry*>::DONT_DEALLOCATE_FLAG;
	// EXE-INLINED none: hkArray<hkDisplayGeometry*>::setSize
	hkMemoryAllocator* heap = hkMemoryRouter::getInstance().m_heap;
	const int32_t capacity =
		static_cast<int32_t>(geometry.m_capacityAndFlags & hkArrayBase<hkDisplayGeometry*>::CAPACITY_MASK);
	if (capacity < 1) {
		int32_t newCapacity = capacity * 2;
		if (newCapacity <= 1) {
			newCapacity = 1;
		}
		hkArrayUtil::_reserve(heap, &geometry, newCapacity, sizeof(hkDisplayGeometry*));
	}
	geometry.m_size = 1;
	geometry.m_data[0] = &m_angularLimit;
	displayHandler->displayGeometry(geometry, -1, id, tag);
	// EXE-INLINED none: hkArray<hkDisplayGeometry*>::~hkArray
	heap = hkMemoryRouter::getInstance().m_heap;
	geometry.m_size = 0;
	if (!(geometry.m_capacityAndFlags & hkArrayBase<hkDisplayGeometry*>::DONT_DEALLOCATE_FLAG)) {
		heap->bufFree(geometry.m_data,
			static_cast<int32_t>((geometry.m_capacityAndFlags & hkArrayBase<hkDisplayGeometry*>::CAPACITY_MASK) *
				sizeof(hkDisplayGeometry*)));
	}

	// The limit's start direction from the B pivot.
	lineLength = m_scale * m_lineLength;
	const float perpLength = static_cast<float>(lineLength * 0.75);
	alignas(16) hkVector4 perpStart;
	perpStart.m_quad = m_bodyBWPivot.m_quad;
	// EXE-INLINED none: hkVector4::setAddMul4
	alignas(16) hkVector4 perpEnd;
	perpEnd.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(perpLength), perpInWorld.m_quad), perpStart.m_quad);
	displayHandler->displayLine(perpStart, perpEnd, static_cast<int32_t>(hkColor::YELLOW), id, tag);

	// Body A's direction from the A pivot, normalized.
	alignas(16) hkVector4 attInWorld;
	attInWorld.setRotatedDir(m_RA, baseA.m_col2);
	lineLength = m_scale * m_lineLength;
	const float attLength = static_cast<float>(lineLength * 0.5);
	// EXE-INLINED 0x62AEA0
	const __m128 squares = _mm_mul_ps(attInWorld.m_quad, attInWorld.m_quad);
	const __m128 length2 =
		_mm_add_ss(_mm_shuffle_ps(squares, squares, 0xAA), _mm_add_ss(_mm_shuffle_ps(squares, squares, 0x55), squares));
	const __m128 estimate = _mm_rsqrt_ss(length2);
	const __m128 zeroMask = _mm_cmpeq_ss(length2, _mm_set_ss(0.0f));
	const __m128 step = _mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(length2, estimate), estimate));
	const __m128 inverse = _mm_andnot_ps(zeroMask, _mm_mul_ss(_mm_mul_ss(_mm_set_ss(0.5f), estimate), step));
	attInWorld.m_quad = _mm_mul_ps(_mm_shuffle_ps(inverse, inverse, 0), attInWorld.m_quad);
	alignas(16) hkVector4 attStart;
	attStart.m_quad = m_bodyAWPivot.m_quad;
	// EXE-INLINED none: hkVector4::setAddMul4
	alignas(16) hkVector4 attEnd;
	attEnd.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(attLength), attInWorld.m_quad), attStart.m_quad);
	displayHandler->displayLine(attStart, attEnd, static_cast<int32_t>(hkColor::YELLOW), id, tag);
}
