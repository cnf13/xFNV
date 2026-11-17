#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpHingeDrawer.h"

#include "Havok/Source/Common/Base/Types/Color/hkColor.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/Hinge/hkpHingeConstraintData.h"

#include <xmmintrin.h>

// The axis line's two ends: the pivot plus and minus 0.75 * (m_scale * m_lineLength) along the world axis. The exe
// stores the line length and each factor as floats (the factors computed in double from the stored length) and
// scales the axis by each factor with SSE.
void hkpHingeDrawer::drawConstraint(const hkpConstraintInstance* constraint, hkDebugDisplayHandler* displayHandler,
	int32_t id, int32_t tag) {
	const hkpHingeConstraintData* hingeData = static_cast<const hkpHingeConstraintData*>(constraint->getData());
	m_primitiveDrawer.setDisplayHandler(displayHandler);
	hkTransform refLocalToWorld;
	hkTransform attLocalToWorld;
	updateCommonParameters(constraint, refLocalToWorld, attLocalToWorld);
	m_bodyBWPivot.setTransformedPos(refLocalToWorld, hingeData->m_atoms.m_transforms.m_transformB.m_translation);
	m_bodyAWPivot.setTransformedPos(attLocalToWorld, hingeData->m_atoms.m_transforms.m_transformA.m_translation);
	drawBodyFrames(id, tag);
	drawPivots(id, tag);
	displayHandler->displayLine(m_bodyAWPivot, m_bodyBWPivot, static_cast<int32_t>(hkColor::RED), id, tag);

	hkVector4 axisInWorld;
	axisInWorld.setRotatedDir(m_RB, hingeData->m_atoms.m_transforms.m_transformB.m_rotation.m_col0);
	float lineLength = m_scale * m_lineLength;
	float plusFactor = 0.75 * lineLength;
	float minusFactor = lineLength * -0.75;
	hkVector4 p1;
	p1.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(plusFactor), axisInWorld.m_quad), m_bodyBWPivot.m_quad);
	hkVector4 p2;
	p2.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(minusFactor), axisInWorld.m_quad), m_bodyBWPivot.m_quad);
	displayHandler->displayLine(p2, p1, hkColor::rgbFromFloats(0.0f, 0.5f, 1.0f, 1.0f), id, tag);
}
