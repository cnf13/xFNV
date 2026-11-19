#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustumPlanes.h"

#include "Fallout Shared/TempEffects/BSTempEffectSimpleDecal.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustum.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPlane.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "SDK/MSVC/ehvec.h"

#include <cmath>

NiFrustumPlanes::NiFrustumPlanes() : m_uiActivePlanes(63) {}

void NiFrustumPlanes::Set(const NiFrustum& kFrust, const NiTransform& kXform) {
    NiPoint3* p_a3;
    NiPoint3* p_kNormal;
    NiPoint3 kPoint;
    NiPoint3 a3;
    NiPoint3 a2;
    NiPoint3 normal;
    NiPoint3 v46;
    NiPoint3 kNormal;
    NiPoint3 v48;
    NiPoint3 v49;
    NiPoint3 v50;
    a2.x = kXform.m_Rotate.m_pEntry[0][0];
    a2.y = kXform.m_Rotate.m_pEntry[1][0];
    float y = kXform.m_Translate.y;
    a2.z = kXform.m_Rotate.m_pEntry[2][0];
    double v5 = kXform.m_Rotate.m_pEntry[0][1];
    kPoint.x = kXform.m_Translate.x;
    v46.x = v5;
    float z = kXform.m_Translate.z;
    double v7 = kXform.m_Rotate.m_pEntry[1][1];
    kPoint.y = y;
    v46.y = v7;
    kPoint.z = z;
    v46.z = kXform.m_Rotate.m_pEntry[2][1];
    normal.x = kXform.m_Rotate.m_pEntry[0][2];
    normal.y = kXform.m_Rotate.m_pEntry[1][2];
    normal.z = kXform.m_Rotate.m_pEntry[2][2];
    float m_fNear = kFrust.m_fNear;
    kNormal.x = a2.x * m_fNear;
    kNormal.y = a2.y * m_fNear;
    kNormal.z = m_fNear * a2.z;
    a3.x = kPoint.x + kNormal.x;
    a3.y = y + kNormal.y;
    a3.z = z + kNormal.z;
    m_akCullingPlanes[0] = NiPlane(a2, a3);
    float m_fFar = kFrust.m_fFar;
    kNormal.x = m_fFar * a2.x;
    kNormal.y = a2.y * m_fFar;
    kNormal.z = m_fFar * a2.z;
    v48.x = kNormal.x + kPoint.x;
    v48.y = kNormal.y + kPoint.y;
    v48.z = kNormal.z + kPoint.z;
    a3 = v48;
    v50.x = -a2.x;
    v50.y = -a2.y;
    v50.z = -a2.z;
    m_akCullingPlanes[1] = NiPlane(v50, a3);
    if (kFrust.m_bOrtho) {
        float m_fLeft = kFrust.m_fLeft;
        v48.x = normal.x * m_fLeft;
        v48.y = normal.y * m_fLeft;
        v48.z = m_fLeft * normal.z;
        kNormal.x = v48.x + kPoint.x;
        kNormal.y = v48.y + kPoint.y;
        kNormal.z = v48.z + kPoint.z;
        a3 = kNormal;
        m_akCullingPlanes[2] = NiPlane(normal, a3);
        float m_fRight = kFrust.m_fRight;
        v48.x = m_fRight * normal.x;
        v48.y = normal.y * m_fRight;
        v48.z = m_fRight * normal.z;
        kNormal.x = v48.x + kPoint.x;
        kNormal.y = v48.y + kPoint.y;
        kNormal.z = v48.z + kPoint.z;
        a3 = kNormal;
        v50.x = -normal.x;
        v50.y = -normal.y;
        v50.z = -normal.z;
        m_akCullingPlanes[3] = NiPlane(v50, a3);
        float m_fTop = kFrust.m_fTop;
        v48.x = m_fTop * v46.x;
        v48.y = v46.y * m_fTop;
        v48.z = m_fTop * v46.z;
        kNormal.x = v48.x + kPoint.x;
        kNormal.y = v48.y + kPoint.y;
        kNormal.z = v48.z + kPoint.z;
        a3 = kNormal;
        v50.x = -v46.x;
        v50.y = -v46.y;
        v50.z = -v46.z;
        m_akCullingPlanes[4] = NiPlane(v50, a3);
        float m_fBottom = kFrust.m_fBottom;
        v48.x = v46.x * m_fBottom;
        v48.y = v46.y * m_fBottom;
        v48.z = m_fBottom * v46.z;
        kNormal.x = v48.x + kPoint.x;
        p_a3 = &a3;
        kNormal.y = v48.y + kPoint.y;
        p_kNormal = &v46;
        kNormal.z = v48.z + kPoint.z;
        a3 = kNormal;
    }
    else {
        float fTmp = kFrust.m_fLeft * kFrust.m_fLeft;
        float v29 = fTmp + 1.0;
        float v30 = std::sqrt((double)v29);
        float v14 = 1.0 / v30;
        double v10 = v14;
        float v15 = -kFrust.m_fLeft * v14;
        v49.x = normal.x * v10;
        v49.y = normal.y * v10;
        v49.z = v10 * normal.z;
        v48.x = a2.x * v15;
        v48.y = a2.y * v15;
        v48.z = v15 * a2.z;
        kNormal.x = v48.x + v49.x;
        kNormal.y = v48.y + v49.y;
        kNormal.z = v48.z + v49.z;
        m_akCullingPlanes[2] = NiPlane(kNormal, kPoint);
        float fTmp_2 = kFrust.m_fRight * kFrust.m_fRight;
        float v32 = fTmp_2 + 1.0;
        float v33 = std::sqrt((double)v32);
        float v16 = 1.0 / v33;
        double v11 = v16;
        float v17 = kFrust.m_fRight * v16;
        float v34 = -v11;
        v48.x = normal.x * v34;
        v48.y = normal.y * v34;
        v48.z = v34 * normal.z;
        v49.x = a2.x * v17;
        v49.y = a2.y * v17;
        v49.z = v17 * a2.z;
        v50.x = v49.x + v48.x;
        v50.y = v49.y + v48.y;
        v50.z = v49.z + v48.z;
        kNormal = v50;
        m_akCullingPlanes[3] = NiPlane(kNormal, kPoint);
        float fTmp_3 = kFrust.m_fTop * kFrust.m_fTop;
        float v36 = fTmp_3 + 1.0;
        float v37 = std::sqrt((double)v36);
        float v18 = 1.0 / v37;
        double v12 = v18;
        float v19 = v18 * kFrust.m_fTop;
        float v38 = -v12;
        v49.x = v46.x * v38;
        v49.y = v46.y * v38;
        v49.z = v38 * v46.z;
        v50.x = a2.x * v19;
        v50.y = a2.y * v19;
        v50.z = v19 * a2.z;
        v48.x = v50.x + v49.x;
        v48.y = v50.y + v49.y;
        v48.z = v50.z + v49.z;
        kNormal = v48;
        m_akCullingPlanes[4] = NiPlane(kNormal, kPoint);
        float fTmp_4 = kFrust.m_fBottom * kFrust.m_fBottom;
        float v40 = fTmp_4 + 1.0;
        float v41 = std::sqrt((double)v40);
        float v20 = 1.0 / v41;
        double v13 = v20;
        float v21 = -kFrust.m_fBottom * v20;
        v49.x = v46.x * v13;
        v49.y = v46.y * v13;
        v49.z = v13 * v46.z;
        v50.x = a2.x * v21;
        v50.y = a2.y * v21;
        v50.z = v21 * a2.z;
        v48.x = v50.x + v49.x;
        p_a3 = &kPoint;
        v48.y = v50.y + v49.y;
        p_kNormal = &kNormal;
        v48.z = v50.z + v49.z;
        kNormal = v48;
    }
    m_akCullingPlanes[5] = NiPlane(*p_kNormal, *p_a3);
}

void NiFrustumPlanes::Set(const NiCamera& kCam) {
    Set(kCam.m_kViewFrustum, kCam.m_kWorld);
}
