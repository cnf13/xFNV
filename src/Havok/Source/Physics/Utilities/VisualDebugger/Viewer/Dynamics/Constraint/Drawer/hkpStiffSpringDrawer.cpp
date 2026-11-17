#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpStiffSpringDrawer.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/Color/hkColor.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/StiffSpring/hkpStiffSpringConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"

#include <xmmintrin.h>

// Draws the pivots, the body frames, the spring's rest length from body A's pivot towards body B's and the rest of
// the way to body B's pivot in red.
void hkpStiffSpringDrawer::drawConstraint(hkpConstraintInstance* constraint, hkDebugDisplayHandler* displayHandler,
	int32_t id, int32_t tag) {
	hkpStiffSpringConstraintData* spring = static_cast<hkpStiffSpringConstraintData*>(constraint->getDataRw());
	m_primitiveDrawer.setDisplayHandler(displayHandler);
	hkTransform refLocalToWorld;
	hkTransform attLocalToWorld;
	updateCommonParameters(constraint, refLocalToWorld, attLocalToWorld);
	m_bodyBWPivot.setTransformedPos(refLocalToWorld, spring->m_atoms.m_pivots.m_translationB);
	m_bodyAWPivot.setTransformedPos(attLocalToWorld, spring->m_atoms.m_pivots.m_translationA);
	drawPivots(id, tag);
	drawBodyFrames(id, tag);
	float restLength = spring->m_atoms.m_spring.m_length;
	hkVector4 dir;
	dir.m_quad = _mm_sub_ps(m_bodyBWPivot.m_quad, m_bodyAWPivot.m_quad);
	// EXE-INLINED 0x62AEA0
	// hkVector4::normalize3 written out: src's inline builds its intermediate reals through hkSimdReal::Construct
	// (0x4A3C90), a call the exe does not make here.
	{
		__m128 x2 = _mm_mul_ps(dir.m_quad, dir.m_quad);
		__m128 len2 = _mm_add_ss(_mm_shuffle_ps(x2, x2, _MM_SHUFFLE(2, 2, 2, 2)),
			_mm_add_ss(_mm_shuffle_ps(x2, x2, _MM_SHUFFLE(1, 1, 1, 1)), x2));
		__m128 approx = _mm_rsqrt_ss(len2);
		__m128 refined = _mm_mul_ss(_mm_mul_ss(_mm_set_ss(0.5f), approx),
			_mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(len2, approx), approx)));
		__m128 lenInv = _mm_andnot_ps(_mm_cmpeq_ss(len2, _mm_setzero_ps()), refined);
		dir.m_quad = _mm_mul_ps(_mm_shuffle_ps(lenInv, lenInv, 0), dir.m_quad);
	}
	hkVector4 restEnd;
	restEnd.m_quad = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(restLength), dir.m_quad), m_bodyAWPivot.m_quad);
	displayHandler->displayLine(m_bodyAWPivot, restEnd, hkColor::rgbFromFloats(0.0f, 0.5f, 1.0f, 1.0f), id, tag);
	displayHandler->displayLine(restEnd, m_bodyBWPivot, static_cast<int32_t>(hkColor::RED), id, tag);
}
