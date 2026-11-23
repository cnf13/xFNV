#include "Havok/Source/Physics/Collide/BoxBox/hkpBoxBoxManifold.h"

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/BoxBox/hkpBoxBoxContactPoint.h"

// No points: every slot's contact point id cleared, the counts zero and the manifold not complete.
hkpBoxBoxManifold::hkpBoxBoxManifold() : m_faceVertexFeatureCount(0), m_numPoints(0), m_isComplete(false) {
    for (hkpFeatureContactPoint& point : m_contactPoints) {
        point.m_contactPointId = 0;
    }
}

// Copies the last point (index m_numPoints - 1 as an int) before it decrements m_numPoints.
void hkpBoxBoxManifold::removePoint(int32_t i) {
    m_isComplete.m_bool = 0;
    m_contactPoints[i] = m_contactPoints[m_numPoints - 1];
    --m_numPoints;
}

int32_t hkpBoxBoxManifold::addPoint(const hkpCdBody& bodyA, const hkpCdBody& bodyB, hkpFeatureContactPoint& fcp) {
    hkpFeatureContactPoint* pFcp = &fcp;
    uint8_t m_numPoints = this->m_numPoints;
    if (m_numPoints > 8u) {
        return -1;
    }
    hkpFeatureContactPoint* v7 = pFcp;
    if (findInManifold(*pFcp).m_bool) {
        return -1;
    }
    int32_t result = m_numPoints;
    if (m_numPoints >= 8u) {
        return -1;
    }
    m_contactPoints[m_numPoints] = *v7;
    ++this->m_numPoints;
    return result;
}

hkBool hkpBoxBoxManifold::findInManifold(const hkpFeatureContactPoint& fcp) {
    int32_t v3 = m_numPoints - 1;
    if (v3 < 0) {
        return 0;
    }

    while (m_contactPoints[v3].m_featureIdA != fcp.m_featureIdA ||
           m_contactPoints[v3].m_featureIdB != fcp.m_featureIdB) {
        if (--v3 < 0) {
            return 0;
        }
    }
    return 1;
}
