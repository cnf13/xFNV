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
    float fDistSqr = (double)fDiffX * fDiffX + (double)fDiffY * fDiffY + (double)fDiffZ * fDiffZ;
    float fA0 = fDistSqr - (double)kBound.m_fRadius * kBound.m_fRadius;
    if (fA0 <= 0.0f) {
        return true;
    }
    float fA1 = (double)fDiffZ * kDir.z + ((double)fDiffY * kDir.y + (double)fDiffX * kDir.x);
    if (fA1 >= 0.0f) {
        return false;
    }
    float fDirSqr = (double)kDir.z * kDir.z + ((double)kDir.x * kDir.x + (double)kDir.y * kDir.y);
    return (double)fA1 * fA1 >= (double)fA0 * fDirSqr;
}

char NiCollisionUtils::IntersectTriangle(
    const NiPoint3* a1,
    const NiPoint3* a2,
    const NiPoint3* a3,
    const NiPoint3* a4,
    const float* a5,
    char a6,
    NiPoint3* a7,
    float* a8,
    float* a9,
    float* a10
) {
    const NiPoint3* v16;
    float* v17;
    float* v24;
    float* v25;
    NiPoint3 v29;
    NiPoint3 v30;
    NiPoint3 pt;
    NiPoint3 dst;
    v29.x = a4->x - a3->x;
    v29.y = a4->y - a3->y;
    v29.z = a4->z - a3->z;
    v30.x = *a5 - a3->x;
    v30.y = a5[1] - a3->y;
    v30.z = a5[2] - a3->z;
    pt.x = a2->y * v30.z - a2->z * v30.y;
    pt.y = a2->z * v30.x - v30.z * a2->x;
    pt.z = v30.y * a2->x - v30.x * a2->y;
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
        if (v15 <= -1e-05f && !a6) {
            float v36;
            v16 = a1;
            v17 = a9;
            pt.x = a1->x - a3->x;
            pt.y = a1->y - a3->y;
            pt.z = a1->z - a3->z;
            v36 = v27 * pt.z + (y * pt.y + x * pt.x);
            *a9 = v36;
            if (!(v36 > 0.0) && !(v36 < v15)) {
                float v37;
                dst = pt.Cross(v29);
                v37 = a2->Dot(dst);
                v24 = a10;
                *a10 = v37;
                if (!(v37 > 0.0) && !(v39 > v37 + *a9)) {
                    float v38;
                    v38 = v30.Dot(dst);
                    v25 = a8;
                    *a8 = v38;
                    if (!(v38 > 0.0)) {
                    LABEL_17:;
                        float v40 = 1.0 / v39;
                        *v17 = *v17 * v40;
                        *v24 = *v24 * v40;
                        float v41 = v40 * *v25;
                        *v25 = v41;
                        NiPoint3* v28 = &(dst = v41 * *a2);
                        *a7 = (pt = *v16 + *v28);
                        return 1;
                    }
                }
            }
        }
    }
    else {
        v16 = a1;
        v17 = a9;
        pt.x = a1->x - a3->x;
        pt.y = a1->y - a3->y;
        pt.z = a1->z - a3->z;
        double v18 = y * pt.y;
        double v19 = pt.x;
        double v20 = x * pt.x + v18;
        double v21 = pt.z;
        double v22 = z * pt.z + v20;
        double v23 = pt.y;
        float v33 = v22;
        *a9 = v33;
        if (v33 < 0.0) {
            return 0;
        }
        if (v39 < static_cast<double>(v33)) {
            return 0;
        }
        v24 = a10;
        pt.x = v23 * v29.z - v21 * v11;
        pt.y = v21 * v13 - v19 * v29.z;
        pt.z = v11 * v19 - v13 * v23;
        float v34 = a2->y * pt.y + pt.x * a2->x + a2->z * pt.z;
        *a10 = v34;
        if (!(v34 < 0.0) && !(v39 < v34 + *a9)) {
            float v35 = v30.Dot(pt);
            v25 = a8;
            *a8 = v35;
            if (v35 < 0.0) {
                return 0;
            }
            goto LABEL_17;
        }
    }
    return 0;
}
