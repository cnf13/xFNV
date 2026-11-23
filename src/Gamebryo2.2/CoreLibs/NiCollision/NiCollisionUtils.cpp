#include "Gamebryo2.2/CoreLibs/NiCollision/NiCollisionUtils.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"

// A ray starting inside the bound meets it; one starting outside meets it only when it points towards the centre and
// its squared closest approach is within the radius. kDiff, the squared lengths and the dot products are rounded to
// float, and the final products are kept in double precision.
bool NiCollisionUtils::FindBoundIntersect(NiAVObject* pkObj, const NiPoint3& kOrigin, const NiPoint3& kDir) {
    NiBound* pkBound = pkObj->m_kWorldBound;
    if (!pkBound) {
        pkBound = &NiAVObject::NullBoundS;
    }
    NiBound kBound = *pkBound;
    float fDiffX = kOrigin.x - kBound.m_kCenter.x;
    float fDiffY = kOrigin.y - kBound.m_kCenter.y;
    float fDiffZ = kOrigin.z - kBound.m_kCenter.z;
    float fDistSqr = static_cast<double>(fDiffX) * fDiffX + static_cast<double>(fDiffY) * fDiffY +
        static_cast<double>(fDiffZ) * fDiffZ;
    float fA0 = fDistSqr - static_cast<double>(kBound.m_fRadius) * kBound.m_fRadius;
    if (fA0 <= 0.0f) {
        return true;
    }
    float fA1 = static_cast<double>(fDiffZ) * kDir.z +
        (static_cast<double>(fDiffY) * kDir.y + static_cast<double>(fDiffX) * kDir.x);
    if (fA1 >= 0.0f) {
        return false;
    }
    float fDirSqr = static_cast<double>(kDir.z) * kDir.z +
        (static_cast<double>(kDir.x) * kDir.x + static_cast<double>(kDir.y) * kDir.y);
    return static_cast<double>(fA1) * fA1 >= static_cast<double>(fA0) * fDirSqr;
}

char NiCollisionUtils::IntersectTriangle(
    const NiPoint3* kOrigin,
    const NiPoint3* kDir,
    const NiPoint3* kV1,
    const NiPoint3* kV2,
    const float* kV3,
    char bCull,
    NiPoint3* kIntersect,
    float* r,
    float* s,
    float* t
) {
    const NiPoint3* v16;
    float* v17;
    float* v24;
    float* v25;
    NiPoint3 v29;
    NiPoint3 v30;
    NiPoint3 pt;
    NiPoint3 dst;
    v29.x = kV2->x - kV1->x;
    v29.y = kV2->y - kV1->y;
    v29.z = kV2->z - kV1->z;
    v30.x = *kV3 - kV1->x;
    v30.y = kV3[1] - kV1->y;
    v30.z = kV3[2] - kV1->z;
    pt.x = kDir->y * v30.z - kDir->z * v30.y;
    pt.y = kDir->z * v30.x - v30.z * kDir->x;
    pt.z = v30.y * kDir->x - v30.x * kDir->y;
    double y = pt.y;
    double v11 = v29.y;
    double x = pt.x;
    double v13 = v29.x;
    double z = pt.z;
    float v39 = pt.x * v29.x + pt.y * v29.y + pt.z * v29.z;
    double v15 = v39;
    if (!(v39 >= 1e-05f)) {
        double v27;
        v27 = pt.z;
        if (!(v15 <= -1e-05f) || bCull) {
            return 0;
        }
        float v36;
        v16 = kOrigin;
        v17 = s;
        pt.x = kOrigin->x - kV1->x;
        pt.y = kOrigin->y - kV1->y;
        pt.z = kOrigin->z - kV1->z;
        v36 = v27 * pt.z + (y * pt.y + x * pt.x);
        *s = v36;
        if (v36 > 0.0 || v36 < v15) {
            return 0;
        }
        float v37;
        dst = pt.Cross(v29);
        v37 = kDir->Dot(dst);
        v24 = t;
        *t = v37;
        if (v37 > 0.0 || v39 > v37 + *s) {
            return 0;
        }
        float v38;
        v38 = v30.Dot(dst);
        v25 = r;
        *r = v38;
        if (v38 > 0.0) {
            return 0;
        }
    }
    else {
        v16 = kOrigin;
        v17 = s;
        pt.x = kOrigin->x - kV1->x;
        pt.y = kOrigin->y - kV1->y;
        pt.z = kOrigin->z - kV1->z;
        double v18 = y * pt.y;
        double v19 = pt.x;
        double v20 = x * pt.x + v18;
        double v21 = pt.z;
        double v22 = z * pt.z + v20;
        double v23 = pt.y;
        float v33 = v22;
        *s = v33;
        if (v33 < 0.0) {
            return 0;
        }
        if (v39 < static_cast<double>(v33)) {
            return 0;
        }
        v24 = t;
        pt.x = v23 * v29.z - v21 * v11;
        pt.y = v21 * v13 - v19 * v29.z;
        pt.z = v11 * v19 - v13 * v23;
        float v34 = kDir->y * pt.y + pt.x * kDir->x + kDir->z * pt.z;
        *t = v34;
        if (v34 < 0.0 || v39 < v34 + *s) {
            return 0;
        }
        float v35 = v30.Dot(pt);
        v25 = r;
        *r = v35;
        if (v35 < 0.0) {
            return 0;
        }
    }
    float v40 = 1.0 / v39;
    *v17 = *v17 * v40;
    *v24 = *v24 * v40;
    float v41 = v40 * *v25;
    *v25 = v41;
    NiPoint3* v28 = &(dst = v41 * *kDir);
    *kIntersect = (pt = *v16 + *v28);
    return 1;
}
