#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpConstraintDrawer.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/Color/hkColor.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Visualize/hkDebugDisplayHandler.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpPrimitiveDrawer.h"

hkpConstraintDrawer::hkpConstraintDrawer()
  : m_lineLength(1.0f), m_arcRadius(0.75f), m_pivotSize(0.25f), m_arrowSize(1.0f), m_coneSize(0.5f), m_planeSize(0.25f),
    m_numArcSegments(24), m_scale(1.0f), m_primitiveDrawer() {}

void hkpConstraintDrawer::setScale(float scale) {
    m_scale = scale;
}

// Draws m_RA's columns as arrows at body A (red, green, blue) and m_RB's at body B (yellow, orange, magenta), each
// arrow's head opened towards the next column.
void hkpConstraintDrawer::drawBodyFrames(int32_t id, int32_t tag) {
    int32_t yellow = static_cast<int32_t>(hkColor::rgbFromFloats(1.0f, 1.0f, 0.0f, 1.0f));
    int32_t orange = static_cast<int32_t>(hkColor::rgbFromFloats(1.0f, 0.5f, 0.0f, 1.0f));
    int32_t magenta = static_cast<int32_t>(hkColor::rgbFromFloats(1.0f, 0.0f, 1.0f, 1.0f));

    alignas(16) hkVector4 axisA0 = m_RA.m_col0;
    alignas(16) hkVector4 axisA1 = m_RA.m_col1;
    alignas(16) hkVector4 axisA2 = m_RA.m_col2;
    alignas(16) hkVector4 axisB0 = m_RB.m_col0;
    alignas(16) hkVector4 axisB1 = m_RB.m_col1;
    alignas(16) hkVector4 axisB2 = m_RB.m_col2;

    // Each size is the arrow size for drawing (m_scale * m_arrowSize, a float) times a double 0.2f, stored as a
    // float.
    // EXE-INLINED none: hkpConstraintDrawer::getArrowSizeForDraw
    float arrowSize = m_scale * m_arrowSize;
    m_primitiveDrawer.displayArrow(
        m_bodyAWPos, axisA0, axisA1, static_cast<int32_t>(0xFFFF0000),
        static_cast<float>(arrowSize * 0.20000000298023224), id, tag
    );
    // EXE-INLINED none: hkpConstraintDrawer::getArrowSizeForDraw
    arrowSize = m_scale * m_arrowSize;
    m_primitiveDrawer.displayArrow(
        m_bodyAWPos, axisA1, axisA2, static_cast<int32_t>(0xFF008000),
        static_cast<float>(arrowSize * 0.20000000298023224), id, tag
    );
    // EXE-INLINED none: hkpConstraintDrawer::getArrowSizeForDraw
    arrowSize = m_scale * m_arrowSize;
    m_primitiveDrawer.displayArrow(
        m_bodyAWPos, axisA2, axisA0, static_cast<int32_t>(0xFF0000FF),
        static_cast<float>(arrowSize * 0.20000000298023224), id, tag
    );
    // EXE-INLINED none: hkpConstraintDrawer::getArrowSizeForDraw
    arrowSize = m_scale * m_arrowSize;
    m_primitiveDrawer.displayArrow(
        m_bodyBWPos, axisB0, axisB1, yellow, static_cast<float>(arrowSize * 0.20000000298023224), id, tag
    );
    // EXE-INLINED none: hkpConstraintDrawer::getArrowSizeForDraw
    arrowSize = m_scale * m_arrowSize;
    m_primitiveDrawer.displayArrow(
        m_bodyBWPos, axisB1, axisB2, orange, static_cast<float>(arrowSize * 0.20000000298023224), id, tag
    );
    // EXE-INLINED none: hkpConstraintDrawer::getArrowSizeForDraw
    arrowSize = m_scale * m_arrowSize;
    m_primitiveDrawer.displayArrow(
        m_bodyBWPos, axisB2, axisB0, magenta, static_cast<float>(arrowSize * 0.20000000298023224), id, tag
    );
}

void hkpConstraintDrawer::drawPivots(int32_t id, int32_t tag) {
    float taga = m_scale * m_pivotSize;
    hkpPrimitiveDrawer* p_m_primitiveDrawer = &m_primitiveDrawer;
    m_primitiveDrawer.displayOrientedPoint(m_bodyAWPivot, m_RA, taga, -0x10000, id, tag);
    float tagb = m_scale * m_pivotSize;
    p_m_primitiveDrawer->displayOrientedPoint(m_bodyBWPivot, m_RB, tagb, -1, id, tag);
}

// The bodies are hkpRigidBody; reads the entities' motion-state transforms.
void hkpConstraintDrawer::updateCommonParameters(
    const hkpConstraintInstance* constraint,
    hkTransform& refLocalToWorld,
    hkTransform& attLocalToWorld
) {
    refLocalToWorld = constraint->m_entities[1]->m_motion.m_motionState.m_transform;
    attLocalToWorld = constraint->m_entities[0]->m_motion.m_motionState.m_transform;
    m_RB = refLocalToWorld.m_rotation;
    m_RA = attLocalToWorld.m_rotation;
    m_bodyBWPos = refLocalToWorld.m_translation;
    m_bodyAWPos = attLocalToWorld.m_translation;
}

void hkpConstraintDrawer::setLineLength(float lineLength) {
    m_lineLength = lineLength;
}

void hkpConstraintDrawer::setArcRadius(float arcRadius) {
    m_arcRadius = arcRadius;
}

void hkpConstraintDrawer::setNumArcSegments(int32_t numSegments) {
    m_numArcSegments = numSegments;
}

void hkpConstraintDrawer::setPivotSize(float size) {
    m_pivotSize = size;
}

void hkpConstraintDrawer::setArrowSize(float arrrowSize) {
    m_arrowSize = arrrowSize;
}
