#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastOutput.h"

#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastCollectorOutput.h"

#include <cstring>

hkpShapeRayCastOutput::hkpShapeRayCastOutput() {
    reset();
}

hkpShapeRayCastOutput::hkpShapeRayCastOutput(const hkpShapeRayCastOutput* apOther)
  : hkpShapeRayCastCollectorOutput(apOther), m_shapeKeyIndex(apOther->m_shapeKeyIndex) {
    std::memcpy(m_shapeKeys, apOther->m_shapeKeys, sizeof(m_shapeKeys));
}

void hkpShapeRayCastOutput::reset() {
    m_shapeKeyIndex = 0;
    m_shapeKeys[0] = (-1);
}

void hkpShapeRayCastOutput::reset_0() {
    FNVD_RECONSTRUCT(hkpShapeRayCastCollectorOutput)(this);
    reset();
}

// The constructor is empty because each output's own default constructor initialises it.
hkpShapeRayBundleCastOutput::hkpShapeRayBundleCastOutput() {}

int32_t hkpShapeRayBundleCastOutput::reset() {
    m_outputs[0u].m_hitFraction = 1.0;
    m_outputs[0u].m_extraInfo = -1;
    m_outputs[0u].m_shapeKeys[0u] = -1;
    m_outputs[0u].m_shapeKeyIndex = 0;
    m_outputs[1].m_hitFraction = 1.0;
    m_outputs[1].m_extraInfo = -1;
    m_outputs[1].m_shapeKeyIndex = 0;
    m_outputs[1].m_shapeKeys[0] = -1;
    m_outputs[2].m_hitFraction = 1.0;
    m_outputs[2].m_extraInfo = -1;
    m_outputs[2].m_shapeKeyIndex = 0;
    m_outputs[2].m_shapeKeys[0] = -1;
    m_outputs[3].m_hitFraction = 1.0;
    m_outputs[3].m_extraInfo = -1;
    m_outputs[3].m_shapeKeyIndex = 0;
    m_outputs[3].m_shapeKeys[0] = -1;
    return -1;
}

// The copy of a1's fields, its padding words included.
hkpShapeRayCastCollectorOutput::hkpShapeRayCastCollectorOutput(const hkpShapeRayCastCollectorOutput* a1)
  : m_normal(a1->m_normal), m_hitFraction(a1->m_hitFraction), m_extraInfo(a1->m_extraInfo) {
    std::memcpy(m_pad, a1->m_pad, sizeof(m_pad));
}
