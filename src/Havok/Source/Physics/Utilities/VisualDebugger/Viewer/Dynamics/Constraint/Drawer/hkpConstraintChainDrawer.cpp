#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpConstraintChainDrawer.h"

#include "Havok/Source/Common/Base/Types/Color/hkColor.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/BallSocket/hkpBallSocketChainData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/Powered/hkpPoweredChainData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/StiffSpring/hkpStiffSpringChainData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/hkpConstraintChainInstance.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"

// KEEP: when the chain data's type is none of the three chain types, the exe skips the pivot loads and transforms
// pivotInA and pivotInB as they are, unwritten on the first pass, into m_bodyAWPivot and m_bodyBWPivot: an
// uninitialised read.
void hkpConstraintChainDrawer::drawConstraint(hkpConstraintInstance* constraint, hkDebugDisplayHandler* displayHandler,
	int32_t id, int32_t tag) {
	hkpConstraintChainInstance* instance = static_cast<hkpConstraintChainInstance*>(constraint);
	hkpConstraintData* data = instance->getDataRw();
	m_primitiveDrawer.setDisplayHandler(displayHandler);
	int32_t numConstraints = instance->m_chainedEntities.m_size - 1;
	hkVector4 pivotInA;
	hkVector4 pivotInB;
	for (int32_t i = 0; i < numConstraints; i++) {
		// The SDK's bodies are hkpRigidBody: the exe reads the entities' motion-state transforms (getTransform
		// inlined).
		hkTransform refLocalToWorld = instance->m_chainedEntities[i + 1]->m_motion.m_motionState.m_transform;
		hkTransform attLocalToWorld = instance->m_chainedEntities[i]->m_motion.m_motionState.m_transform;
		m_RB = refLocalToWorld.m_rotation;
		m_RA = attLocalToWorld.m_rotation;
		m_bodyBWPos = refLocalToWorld.m_translation;
		m_bodyAWPos = attLocalToWorld.m_translation;

		switch (data->getType()) {
		case hkpConstraintData::CONSTRAINT_TYPE_STIFF_SPRING_CHAIN: {
			const hkpStiffSpringChainData::ConstraintInfo& info =
				static_cast<hkpStiffSpringChainData*>(data)->m_infos[i];
			pivotInB = info.m_pivotInB;
			pivotInA = info.m_pivotInA;
			break;
		}
		case hkpConstraintData::CONSTRAINT_TYPE_BALL_SOCKET_CHAIN: {
			const hkpBallSocketChainData::ConstraintInfo& info = static_cast<hkpBallSocketChainData*>(data)->m_infos[i];
			pivotInB = info.m_pivotInB;
			pivotInA = info.m_pivotInA;
			break;
		}
		case hkpConstraintData::CONSTRAINT_TYPE_POWERED_CHAIN: {
			const hkpPoweredChainData::ConstraintInfo& info = static_cast<hkpPoweredChainData*>(data)->m_infos[i];
			pivotInB = info.m_pivotInB;
			pivotInA = info.m_pivotInA;
			break;
		}
		default:
			break;
		}

		m_bodyBWPivot.setTransformedPos(refLocalToWorld, pivotInB);
		m_bodyAWPivot.setTransformedPos(attLocalToWorld, pivotInA);
		drawPivots(id, tag);
		drawBodyFrames(id, tag);
		displayHandler->displayLine(m_bodyAWPivot, m_bodyBWPivot, static_cast<int32_t>(hkColor::RED), id, tag);
	}
}
