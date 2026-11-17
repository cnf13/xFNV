#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpPulleyDrawer.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/Color/hkColor.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Pulley/hkpPulleyConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"

#include <xmmintrin.h>

// Draws the pivots, the body frames, the line between the two fixed world pivots, body A's rope and one rope line per
// unit of leverage on body B, spread 0.07 apart along body B's x axis.
void hkpPulleyDrawer::drawConstraint(hkpConstraintInstance* constraint, hkDebugDisplayHandler* displayHandler,
	int32_t id, int32_t tag) {
	hkpPulleyConstraintData* pulley = static_cast<hkpPulleyConstraintData*>(constraint->getDataRw());
	m_primitiveDrawer.setDisplayHandler(displayHandler);
	hkTransform refLocalToWorld;
	hkTransform attLocalToWorld;
	updateCommonParameters(constraint, refLocalToWorld, attLocalToWorld);
	m_bodyBWPivot.setTransformedPos(refLocalToWorld, pulley->m_atoms.m_translations.m_translationB);
	m_bodyAWPivot.setTransformedPos(attLocalToWorld, pulley->m_atoms.m_translations.m_translationA);
	drawPivots(id, tag);
	drawBodyFrames(id, tag);
	const hkpPulleyConstraintAtom& atom = pulley->m_atoms.m_pulley;
	displayHandler->displayLine(atom.m_fixedPivotAinWorld, atom.m_fixedPivotBinWorld,
		static_cast<int32_t>(hkColor::GREY75), id, tag);
	displayHandler->displayLine(m_bodyAWPivot, atom.m_fixedPivotAinWorld, static_cast<int32_t>(hkColor::WHITE), id,
		tag);

	// The rope count: the leverage on body B, at least 1, stored as a float and truncated.
	float leverage = atom.m_leverageOnBodyB;
	float numRopesReal = leverage > 1.0f ? leverage : 1.0f;
	int32_t numRopes = static_cast<int32_t>(numRopesReal);
	hkVector4 offset;
	offset.set(0.07f, 0.0f, 0.0f, 0.0f);
	offset.setRotatedDir(constraint->m_entities[1]->m_motion.getMotionState()->m_transform.m_rotation, offset);
	float baseScale = static_cast<double>(numRopes - 1) * -0.5;
	__m128 baseOffset = _mm_mul_ps(_mm_set1_ps(baseScale), offset.m_quad);
	for (int32_t i = 0; i < numRopes; i++) {
		float scale = static_cast<float>(i);
		__m128 thisOffset = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(scale), offset.m_quad), baseOffset);
		hkVector4 a;
		hkVector4 b;
		a.m_quad = _mm_add_ps(m_bodyBWPivot.m_quad, thisOffset);
		b.m_quad = _mm_add_ps(thisOffset, atom.m_fixedPivotBinWorld.m_quad);
		displayHandler->displayLine(a, b, static_cast<int32_t>(hkColor::WHITE), id, tag);
	}
}
