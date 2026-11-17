#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpPointToPathDrawer.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/Color/hkColor.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/PointToPath/hkpParametricCurve.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/PointToPath/hkpPointToPathConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpPrimitiveDrawer.h"

// Draws the common frames and pivots, then the path as light blue lines between its consecutive points, each taken
// from the path's space (the reference body's frame times the constraint's second constraint space) to world space.
void hkpPointToPathDrawer::drawConstraint(hkpConstraintInstance* constraint, hkDebugDisplayHandler* displayHandler,
	int32_t id, int32_t tag) {
	hkpPointToPathConstraintData* pData = static_cast<hkpPointToPathConstraintData*>(constraint->getDataRw());
	m_primitiveDrawer.setDisplayHandler(displayHandler);
	hkTransform refLocalToWorld;
	hkTransform attLocalToWorld;
	updateCommonParameters(constraint, refLocalToWorld, attLocalToWorld);
	m_bodyBWPivot = refLocalToWorld.m_translation;
	pData->calcPivot(constraint->m_entities[0]->m_motion.m_motionState.m_transform, m_bodyAWPivot);
	drawBodyFrames(id, tag);
	drawPivots(id, tag);
	if (pData->m_path) {
		hkTransform pathToWorld;
		pathToWorld.setMul(refLocalToWorld, pData->m_transform_OS_KS[1]);
		hkpParametricCurve* pPath = pData->m_path;
		if (pPath) {
			// Raw storage: the body frees the storage itself, so no C++ destructor may free it again.
			alignas(16) FNVD_INLINED_LOCAL(hkArray<hkVector4>, pathPoints, std::nullopt, std::nullopt);
			// EXE-INLINED none: hkArray<hkVector4>::hkArray
			pathPoints.m_data = nullptr;
			pathPoints.m_size = 0;
			pathPoints.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
			pPath->getPointsToDraw(pathPoints);
			for (int32_t i = 1; i < pathPoints.getSize(); i++) {
				// Copies each point to a temporary and transforms that copy.
				hkVector4 point;
				hkVector4 start;
				hkVector4 end;
				point = pathPoints.m_data[i];
				start.setTransformedPos(pathToWorld, point);
				point = pathPoints.m_data[i - 1];
				end.setTransformedPos(pathToWorld, point);
				displayHandler->displayLine(start, end, hkColor::rgbFromFloats(0.0f, 0.25f, 1.0f, 1.0f), id, tag);
			}
			// EXE-INLINED none: hkArray<hkVector4>::~hkArray
			hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
			pathPoints.m_size = 0;
			if (pathPoints.m_capacityAndFlags >= 0) {
				pHeap->bufFree(pathPoints.m_data,
					static_cast<int32_t>(sizeof(hkVector4)) *
						static_cast<int32_t>(pathPoints.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK));
			}
		}
	}
}
