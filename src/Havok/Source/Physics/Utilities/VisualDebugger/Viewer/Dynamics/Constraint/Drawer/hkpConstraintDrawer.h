#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpPrimitiveDrawer.h"
#include "Reconstruction.hpp"

class hkTransform;
class hkpConstraintInstance;

// The common drawing sizes and per-draw body frames of the visual debugger's constraint drawers. No vtable. Layout
// from the PC PDB; updateCommonParameters (0xDE47E0) writes m_bodyAWPos, m_bodyBWPos, m_RA and m_RB at these
// offsets and hkpBallSocketDrawer::drawConstraint (0xDE8090) uses the pivots and m_primitiveDrawer.
class hkpConstraintDrawer {
public:
	hkpConstraintDrawer();

	// Scales the sizes the drawer draws with.
	void setScale(float scale);

protected:
	// Draws the two bodies' frames: arrows along the columns of m_RA at m_bodyAWPos and of m_RB at m_bodyBWPos.
	void drawBodyFrames(int32_t id, int32_t tag);
	// Draws the two pivots.
	void drawPivots(int32_t id, int32_t tag);
	// Reads the bodies' transforms into refLocalToWorld and attLocalToWorld and sets the body positions and
	// rotations from them.
	void updateCommonParameters(const hkpConstraintInstance* constraint, hkTransform& refLocalToWorld,
		hkTransform& attLocalToWorld);

public:
	void setLineLength(float lineLength);
	void setArcRadius(float arcRadius);
	void setNumArcSegments(int32_t numSegments);
	void setPivotSize(float size);
	void setArrowSize(float arrrowSize);

	float m_lineLength;
	float m_arcRadius;
	float m_pivotSize;
	float m_arrowSize;
	float m_coneSize;
	float m_planeSize;
	int32_t m_numArcSegments;
	float m_scale;
	hkVector4 m_bodyAWPos;
	hkVector4 m_bodyBWPos;
	hkVector4 m_bodyAWPivot;
	hkVector4 m_bodyBWPivot;
	hkRotation m_RA;
	hkRotation m_RB;
	hkpPrimitiveDrawer m_primitiveDrawer;
};
