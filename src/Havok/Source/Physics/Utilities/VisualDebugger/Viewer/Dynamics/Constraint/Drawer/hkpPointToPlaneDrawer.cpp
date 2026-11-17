#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpPointToPlaneDrawer.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/Color/hkColor.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/PointToPlane/hkpPointToPlaneConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"

#include <xmmintrin.h>

// Draws the pivots, the body frames, the plane as a grid of six lines around body B's pivot (three along each of the
// plane's two axes, the rotation columns of B after the constraint's axis index) and the line from body A's pivot to
// its position.
void hkpPointToPlaneDrawer::drawConstraint(hkpConstraintInstance* constraint, hkDebugDisplayHandler* displayHandler,
	int32_t id, int32_t tag) {
	hkpPointToPlaneConstraintData* plane = static_cast<hkpPointToPlaneConstraintData*>(constraint->getDataRw());
	const hkpPointToPlaneConstraintData::Atoms& atoms = plane->m_atoms;
	m_primitiveDrawer.setDisplayHandler(displayHandler);
	hkTransform refLocalToWorld;
	hkTransform attLocalToWorld;
	updateCommonParameters(constraint, refLocalToWorld, attLocalToWorld);
	m_bodyBWPivot.setTransformedPos(refLocalToWorld, atoms.m_transforms.m_transformB.getTranslation());
	m_bodyAWPivot.setTransformedPos(attLocalToWorld, atoms.m_transforms.m_transformA.getTranslation());
	drawPivots(id, tag);
	drawBodyFrames(id, tag);
	hkVector4 perp1;
	perp1.setRotatedDir(m_RB, atoms.m_transforms.m_transformB.getColumn((atoms.m_lin.m_axisIndex + 1) % 3));
	hkVector4 perp2;
	perp2.setRotatedDir(m_RB, atoms.m_transforms.m_transformB.getColumn((atoms.m_lin.m_axisIndex + 2) % 3));

	// Each grid line runs from twice the drawing size to minus twice it along one axis, at -1.4, 0 and 1.4 times
	// the size along the other (the double constant is 1.4f widened).
	float size = m_scale * m_lineLength;
	float twoSize = size + size;
	float negTwoSize = -2.0 * size;
	hkVector4 lines[12];
	for (int32_t i = 0; i < 2; i++) {
		for (int32_t j = -1; j <= 1; j++) {
			const hkVector4& along = i != 0 ? perp1 : perp2;
			const hkVector4& across = i != 0 ? perp2 : perp1;
			hkVector4& start = lines[(i * 3 + j + 1) * 2];
			hkVector4& end = lines[(i * 3 + j + 1) * 2 + 1];
			start.m_quad = m_bodyBWPivot.m_quad;
			end.m_quad = m_bodyBWPivot.m_quad;
			float offset = j * 1.399999976158142 * size;
			start.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(twoSize), along.m_quad), start.m_quad);
			end.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(negTwoSize), along.m_quad), end.m_quad);
			start.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(offset), across.m_quad), start.m_quad);
			end.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(offset), across.m_quad), end.m_quad);
		}
	}
	for (int32_t k = 0; k < 6; k++) {
		displayHandler->displayLine(lines[k * 2], lines[k * 2 + 1], hkColor::rgbFromFloats(0.8f, 0.8f, 0.8f, 1.0f), id,
			tag);
	}
	displayHandler->displayLine(m_bodyAWPivot, m_bodyAWPos, hkColor::rgbFromFloats(0.3f, 0.3f, 0.8f, 1.0f), id, tag);
}
