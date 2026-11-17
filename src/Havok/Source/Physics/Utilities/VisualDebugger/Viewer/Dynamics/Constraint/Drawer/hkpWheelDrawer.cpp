#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpWheelDrawer.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/Color/hkColor.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/Wheel/hkpWheelConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"

#include <xmmintrin.h>

// Draws the pivots, the body frames, the line between the pivots, the suspension axis through body B's pivot with a
// short cross line at each suspension limit, the steering axis through body B's pivot and the axle through body A's.
FNVD_NO_STACK_COOKIE void hkpWheelDrawer::drawConstraint(hkpConstraintInstance* constraint,
	hkDebugDisplayHandler* displayHandler, int32_t id, int32_t tag) {
	using Atoms = hkpWheelConstraintData::Atoms;
	hkpWheelConstraintData* wheel = static_cast<hkpWheelConstraintData*>(constraint->getDataRw());
	const Atoms& atoms = wheel->m_atoms;
	m_primitiveDrawer.setDisplayHandler(displayHandler);
	hkTransform refLocalToWorld;
	hkTransform attLocalToWorld;
	updateCommonParameters(constraint, refLocalToWorld, attLocalToWorld);
	m_bodyBWPivot.setTransformedPos(refLocalToWorld, atoms.m_suspensionBase.m_transformB.getTranslation());
	m_bodyAWPivot.setTransformedPos(attLocalToWorld, atoms.m_suspensionBase.m_transformA.getTranslation());
	drawBodyFrames(id, tag);
	drawPivots(id, tag);
	displayHandler->displayLine(m_bodyAWPivot, m_bodyBWPivot, static_cast<int32_t>(hkColor::RED), id, tag);

	hkVector4 start;
	hkVector4 end;
	// The suspension axis, a line of the drawing length centred on body B's pivot.
	hkVector4 suspension;
	suspension.setRotatedDir(m_RB, atoms.m_suspensionBase.m_transformB.getColumn(Atoms::AXIS_SUSPENSION));
	{
		float length = m_scale * m_lineLength;
		float low = -0.5 * length;
		float high = length * 0.5;
		start.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(low), suspension.m_quad), m_bodyBWPivot.m_quad);
		end.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(high), suspension.m_quad), m_bodyBWPivot.m_quad);
		displayHandler->displayLine(start, end, static_cast<int32_t>(hkColor::GREEN), id, tag);
	}

	// The suspension limits: a short line along the axis perpendicular to the suspension at each limit, the upper
	// limit's first.
	float minLimit = atoms.m_lin0Limit.m_min;
	float maxLimit = atoms.m_lin0Limit.m_max;
	hkVector4 axis;
	axis.setRotatedDir(m_RB, atoms.m_suspensionBase.m_transformB.getColumn(Atoms::AXIS_PERP_SUSPENSION));
	{
		float length = m_scale * m_lineLength;
		__m128 center = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(maxLimit), suspension.m_quad), m_bodyBWPivot.m_quad);
		float low = 0.10000000149011612 * length;
		float high = length * -0.10000000149011612;
		start.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(low), axis.m_quad), center);
		end.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(high), axis.m_quad), center);
		displayHandler->displayLine(start, end, static_cast<int32_t>(hkColor::WHITE), id, tag);
	}
	{
		float length = m_scale * m_lineLength;
		__m128 center = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(minLimit), suspension.m_quad), m_bodyBWPivot.m_quad);
		float low = 0.10000000149011612 * length;
		float high = length * -0.10000000149011612;
		start.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(low), axis.m_quad), center);
		end.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(high), axis.m_quad), center);
		displayHandler->displayLine(start, end, static_cast<int32_t>(hkColor::WHITE), id, tag);
	}

	// The steering axis, centred on body B's pivot.
	axis.setRotatedDir(m_RB, atoms.m_steeringBase.m_rotationB.getColumn(Atoms::AXIS_STEERING));
	{
		float length = m_scale * m_lineLength;
		float low = -0.5 * length;
		float high = length * 0.5;
		start.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(low), axis.m_quad), m_bodyBWPivot.m_quad);
		end.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(high), axis.m_quad), m_bodyBWPivot.m_quad);
		displayHandler->displayLine(start, end, static_cast<int32_t>(hkColor::YELLOW), id, tag);
	}

	// The axle, centred on body A's pivot.
	axis.setRotatedDir(m_RA, atoms.m_steeringBase.m_rotationA.getColumn(Atoms::AXIS_AXLE));
	{
		float length = m_scale * m_lineLength;
		float low = -0.5 * length;
		float high = length * 0.5;
		start.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(low), axis.m_quad), m_bodyAWPivot.m_quad);
		end.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(high), axis.m_quad), m_bodyAWPivot.m_quad);
		displayHandler->displayLine(start, end, static_cast<int32_t>(hkColor::BLUE), id, tag);
	}
}
