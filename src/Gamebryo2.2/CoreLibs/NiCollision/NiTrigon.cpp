#include "Gamebryo2.2/CoreLibs/NiCollision/NiTrigon.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

NiPoint3 NiTrigon::GetNormal() {
    float v3 = (m_kEdge1.x * m_kEdge0.z) - (m_kEdge1.z * m_kEdge0.x);
    float v4 = (m_kEdge0.x * m_kEdge1.y) - (m_kEdge1.x * m_kEdge0.y);
    NiPoint3 kNormal(((m_kEdge1.z * m_kEdge0.y) - (m_kEdge0.z * m_kEdge1.y)), v3, v4);
    kNormal.Unitize();
    return kNormal;
}
