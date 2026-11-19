#include "Gamebryo2.2/CoreLibs/NiCollision/NiOBBox.h"

#include "Fallout Shared/Magic/EffectSetting.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiOBBRoot.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"

#include <cmath>

// The separating axis that decided FindIntersection's first time of contact, as Gamebryo's NiOBBox.cpp names them.
enum {
    intrNone,
    intrA0,
    intrA1,
    intrA2,
    intrB0,
    intrB1,
    intrB2,
    intrA0B0,
    intrA0B1,
    intrA0B2,
    intrA1B0,
    intrA1B1,
    intrA1B2,
    intrA2B0,
    intrA2B1,
    intrA2B2
};

// FindIntersection's test of one separating axis, as Gamebryo writes it: u0 and u1 are the boxes' separation on the
// axis at the start and the end of the interval, rsum the sum of their projected radii.
#define UPDATE_TIME(TYPE)                                                                                              \
    {                                                                                                                  \
        if (u0 > rsum) {                                                                                               \
            if (u1 > rsum) return 0;                                                                                   \
            tintr = dt * (rsum - u0) / (u1 - u0);                                                                      \
            if (tintr > tmax) {                                                                                        \
                tmax = tintr;                                                                                          \
                type = TYPE;                                                                                           \
                side = +1;                                                                                             \
            }                                                                                                          \
        }                                                                                                              \
        else if (u0 < -rsum) {                                                                                         \
            if (u1 < -rsum) return 0;                                                                                  \
            tintr = dt * (-rsum - u0) / (u1 - u0);                                                                     \
            if (tintr > tmax) {                                                                                        \
                tmax = tintr;                                                                                          \
                type = TYPE;                                                                                           \
                side = -1;                                                                                             \
            }                                                                                                          \
        }                                                                                                              \
    }

#define GET_COEFF_PLUS_MINUS(coeff, side, cmat, ext)                                                                   \
    {                                                                                                                  \
        if (cmat > 0.0f)                                                                                               \
            coeff = side * ext;                                                                                        \
        else if (cmat < 0.0f)                                                                                          \
            coeff = -side * ext;                                                                                       \
        else                                                                                                           \
            coeff = 0.0f;                                                                                              \
    }

#define GET_COEFF_MINUS_PLUS(coeff, side, cmat, ext)                                                                   \
    {                                                                                                                  \
        if (cmat > 0.0f)                                                                                               \
            coeff = -side * ext;                                                                                       \
        else if (cmat < 0.0f)                                                                                          \
            coeff = side * ext;                                                                                        \
        else                                                                                                           \
            coeff = 0.0f;                                                                                              \
    }

void NiOBBox::Transform(const NiTransform& xform) {
    double m_fScale;
    NiPoint3 v17;
    NiPoint3 v18;
    float v8 = (((&xform)->m_Rotate.m_pEntry[0][0] * m_center.x) + ((&xform)->m_Rotate.m_pEntry[0][1] * m_center.y)) +
        ((&xform)->m_Rotate.m_pEntry[0][2] * m_center.z);
    float v9 = (((&xform)->m_Rotate.m_pEntry[1][0] * m_center.x) + ((&xform)->m_Rotate.m_pEntry[1][1] * m_center.y)) +
        ((&xform)->m_Rotate.m_pEntry[1][2] * m_center.z);
    float v10 = (((&xform)->m_Rotate.m_pEntry[2][1] * m_center.y) + ((&xform)->m_Rotate.m_pEntry[2][0] * m_center.x)) +
        ((&xform)->m_Rotate.m_pEntry[2][2] * m_center.z);
    m_fScale = (double)(&xform)->m_fScale;
    float v11 = (float)(v8 * m_fScale);
    float v13 = (float)(v9 * m_fScale);
    float v15 = (float)(m_fScale * v10);
    v17.x = ((&xform)->m_Translate.x + v11);
    v17.y = ((&xform)->m_Translate.y + v13);
    v17.z = ((&xform)->m_Translate.z + v15);
    m_wCenter = v17;
    float v12 =
        (((&xform)->m_Rotate.m_pEntry[0][0] * m_basis[0].x) + (m_basis[0].y * (&xform)->m_Rotate.m_pEntry[0][1])) +
        ((&xform)->m_Rotate.m_pEntry[0][2] * m_basis[0].z);
    float v14 =
        (((&xform)->m_Rotate.m_pEntry[1][0] * m_basis[0].x) + (m_basis[0].y * (&xform)->m_Rotate.m_pEntry[1][1])) +
        ((&xform)->m_Rotate.m_pEntry[1][2] * m_basis[0].z);
    double v4 = (double)(((m_basis[0].y * (&xform)->m_Rotate.m_pEntry[2][1]) +
                          ((&xform)->m_Rotate.m_pEntry[2][0] * m_basis[0].x)) +
                         ((&xform)->m_Rotate.m_pEntry[2][2] * m_basis[0].z));
    m_wBasis[0].x = v12;
    float v16 = (float)v4;
    m_wBasis[0].y = v14;
    m_wBasis[0].z = v16;
    v18.x =
        (((m_basis[1].y * (&xform)->m_Rotate.m_pEntry[0][1]) + (m_basis[1].x * (&xform)->m_Rotate.m_pEntry[0][0])) +
         (m_basis[1].z * (&xform)->m_Rotate.m_pEntry[0][2]));
    v18.y =
        (((m_basis[1].y * (&xform)->m_Rotate.m_pEntry[1][1]) + (m_basis[1].x * (&xform)->m_Rotate.m_pEntry[1][0])) +
         ((&xform)->m_Rotate.m_pEntry[1][2] * m_basis[1].z));
    v18.z =
        (((m_basis[1].y * (&xform)->m_Rotate.m_pEntry[2][1]) + ((&xform)->m_Rotate.m_pEntry[2][0] * m_basis[1].x)) +
         (m_basis[1].z * (&xform)->m_Rotate.m_pEntry[2][2]));
    m_wBasis[1] = v18;
    v18.x =
        (((m_basis[2].x * (&xform)->m_Rotate.m_pEntry[0][0]) + (m_basis[2].y * (&xform)->m_Rotate.m_pEntry[0][1])) +
         (m_basis[2].z * (&xform)->m_Rotate.m_pEntry[0][2]));
    v18.y =
        (((m_basis[2].x * (&xform)->m_Rotate.m_pEntry[1][0]) + (m_basis[2].y * (&xform)->m_Rotate.m_pEntry[1][1])) +
         ((&xform)->m_Rotate.m_pEntry[1][2] * m_basis[2].z));
    double v5 = (double)(((&xform)->m_Rotate.m_pEntry[2][1] * m_basis[2].y) +
                         ((&xform)->m_Rotate.m_pEntry[2][0] * m_basis[2].x));
    double v6 = (double)(m_basis[2].z * (&xform)->m_Rotate.m_pEntry[2][2]);
    m_wBasis[2].x = v18.x;
    m_wBasis[2].y = v18.y;
    v18.z = (float)(v5 + v6);
    double v7 = (double)m_extent[0];
    m_wBasis[2].z = v18.z;
    m_wExtent[0] = (float)(v7 * (&xform)->m_fScale);
    m_wExtent[1] = (m_extent[1] * (&xform)->m_fScale);
    m_wExtent[2] = (m_extent[2] * (&xform)->m_fScale);
}

void NiOBBox::ComputeNormal(const NiPoint3& p, NiPoint3& normal) const {
    NiPoint3 v8;
    v8.x = m_wExtent[0u];
    v8.y = m_wExtent[1];
    v8.z = m_wExtent[2];
    v8.Unitize();
    if (!(v8.x <= 9.999999747378752e-05)) {
        if (!(v8.y <= 9.999999747378752e-05)) {
            if (!(v8.z <= 9.999999747378752e-05)) {
                v8.x = p.x - m_wCenter.x;
                v8.y = p.y - m_wCenter.y;
                v8.z = p.z - m_wCenter.z;
                double y = v8.y;
                double x = v8.x;
                float pa = m_wBasis[0u].z * v8.z + (m_wBasis[0u].x * v8.x + m_wBasis[0u].y * v8.y);
                double z = v8.z;
                normal.x = pa / (m_wExtent[0u] * m_wExtent[0u]);
                float normala = m_wBasis[1].y * y + m_wBasis[1].x * x + m_wBasis[1].z * z;
                normal.y = normala / (m_wExtent[1] * m_wExtent[1]);
                float normalb = z * m_wBasis[2].z + (x * m_wBasis[2].x + y * m_wBasis[2].y);
                normal.z = normalb / (m_wExtent[2] * m_wExtent[2]);
                normal.Unitize();
            }
            else {
                normal = m_wBasis[2];
            }
        }
        else {
            normal = m_wBasis[1];
        }
    }
    else {
        normal = m_wBasis[0u];
    }
}

int32_t NiOBBox::TestIntersection(const NiOBBox& box) {
    double v47;
    double v81;
    double v82;
    double v100;
    double v108;
    float v184;
    NiPoint3 kVxD;
    NiPoint3 V;
    NiPoint3 D;
    double v4 = box.m_wCenter.x - m_wCenter.x;
    NiOBBRoot* m_pRoot = this->m_pRoot;
    NiPoint3* p_m_velocity = &box.m_pRoot->m_velocity;
    D.x = v4;
    const NiPoint3* v7 = &box.m_wBasis[2];
    const NiPoint3* v180 = &box.m_wBasis[2];
    D.y = box.m_wCenter.y - m_wCenter.y;
    D.z = box.m_wCenter.z - m_wCenter.z;
    V.x = p_m_velocity->x - m_pRoot->m_velocity.x;
    V.y = p_m_velocity->y - m_pRoot->m_velocity.y;
    V.z = p_m_velocity->z - m_pRoot->m_velocity.z;
    double m_fDeltaTime = m_pRoot->m_fDeltaTime;
    const NiPoint3* v9 = &box.m_wBasis[1];
    float v175 = m_fDeltaTime;
    const NiPoint3* v204 = &box.m_wBasis[1];
    float v229 =
        box.m_wBasis[0u].y * m_wBasis[0u].y + box.m_wBasis[0u].x * m_wBasis[0u].x + box.m_wBasis[0u].z * m_wBasis[0u].z;
    float v230 =
        box.m_wBasis[1].y * m_wBasis[0u].y + box.m_wBasis[1].x * m_wBasis[0u].x + box.m_wBasis[1].z * m_wBasis[0u].z;
    float v231 =
        box.m_wBasis[2].y * m_wBasis[0u].y + box.m_wBasis[2].x * m_wBasis[0u].x + box.m_wBasis[2].z * m_wBasis[0u].z;
    double y = D.y;
    double x = D.x;
    double z = D.z;
    kVxD.x = m_wBasis[0u].z * D.z + (m_wBasis[0u].x * D.x + D.y * m_wBasis[0u].y);
    double v13 = V.y;
    double v14 = V.x;
    float boxbb = m_wBasis[0u].x * V.x + V.y * m_wBasis[0u].y + m_wBasis[0u].z * V.z;
    float v224 = boxbb * v175 + kVxD.x;
    float boxbc = box.m_wExtent[0u] * v229;
    float boxbd = std::fabs((double)boxbc);
    double v15 = boxbd;
    float boxbe = box.m_wExtent[1] * v230;
    float boxbf = std::fabs((double)boxbe);
    double v16 = v15 + boxbf;
    float boxbg = v231 * box.m_wExtent[2];
    float boxbh = std::fabs((double)boxbg);
    float boxbi = v16 + boxbh;
    double v17 = boxbi;
    float boxbj = std::fabs((double)m_wExtent[0u]);
    float boxbk = v17 + boxbj;
    double v18 = boxbk;
    if (boxbk < static_cast<double>(kVxD.x) && v224 > v18) {
        return 0;
    }
    float boxa = -v18;
    if (boxa > static_cast<double>(kVxD.x) && v224 < static_cast<double>(boxa)) {
        return 0;
    }
    NiPoint3* v178 = &m_wBasis[1];
    float v232 =
        m_wBasis[1].y * box.m_wBasis[0u].y + box.m_wBasis[0u].x * m_wBasis[1].x + m_wBasis[1].z * box.m_wBasis[0u].z;
    float v233 = v9->y * m_wBasis[1].y + v9->x * m_wBasis[1].x + v9->z * m_wBasis[1].z;
    float v234 = v7->y * m_wBasis[1].y + v7->x * m_wBasis[1].x + v7->z * m_wBasis[1].z;
    kVxD.y = m_wBasis[1].y * y + m_wBasis[1].x * x + m_wBasis[1].z * z;
    float boxbl = m_wBasis[1].y * v13 + v14 * m_wBasis[1].x + m_wBasis[1].z * V.z;
    float v225 = boxbl * v175 + kVxD.y;
    float boxbm = box.m_wExtent[0u] * v232;
    float boxbn = std::fabs((double)boxbm);
    double v20 = boxbn;
    float boxbo = box.m_wExtent[1] * v233;
    float boxbp = std::fabs((double)boxbo);
    double v21 = v20 + boxbp;
    float boxbq = v234 * box.m_wExtent[2];
    float boxbr = std::fabs((double)boxbq);
    float boxbs = v21 + boxbr;
    double v22 = boxbs;
    float boxbt = std::fabs((double)m_wExtent[1]);
    float boxbu = v22 + boxbt;
    double v23 = boxbu;
    if (boxbu < static_cast<double>(kVxD.y) && v225 > v23) {
        return 0;
    }
    float boxb = -v23;
    if (boxb > static_cast<double>(kVxD.y) && v225 < static_cast<double>(boxb)) {
        return 0;
    }
    float v235 =
        m_wBasis[2].y * box.m_wBasis[0u].y + m_wBasis[2].x * box.m_wBasis[0u].x + m_wBasis[2].z * box.m_wBasis[0u].z;
    float v236 = v9->y * m_wBasis[2].y + v9->x * m_wBasis[2].x + v9->z * m_wBasis[2].z;
    float v237 = v7->y * m_wBasis[2].y + v7->x * m_wBasis[2].x + v7->z * m_wBasis[2].z;
    kVxD.z = m_wBasis[2].y * y + m_wBasis[2].x * x + m_wBasis[2].z * z;
    float boxbv = m_wBasis[2].y * v13 + m_wBasis[2].x * v14 + m_wBasis[2].z * V.z;
    float v226 = boxbv * v175 + kVxD.z;
    float boxbw = box.m_wExtent[0u] * v235;
    float boxbx = std::fabs((double)boxbw);
    double v24 = boxbx;
    float boxby = box.m_wExtent[1] * v236;
    float boxbz = std::fabs((double)boxby);
    double v25 = v24 + boxbz;
    float boxca = v237 * box.m_wExtent[2];
    float boxcb = std::fabs((double)boxca);
    float boxcc = v25 + boxcb;
    double v26 = boxcc;
    float boxcd = std::fabs((double)m_wExtent[2]);
    float boxce = v26 + boxcd;
    double v27 = boxce;
    if (boxce < static_cast<double>(kVxD.z) && v226 > v27) {
        return 0;
    }
    float boxc = -v27;
    if (boxc > static_cast<double>(kVxD.z) && v226 < static_cast<double>(boxc)) {
        return 0;
    }
    float boxcf = m_wExtent[0u] * v229;
    float boxcg = std::fabs((double)boxcf);
    double v28 = boxcg;
    float boxch = m_wExtent[1] * v232;
    float boxci = std::fabs((double)boxch);
    double v29 = v28 + boxci;
    float boxcj = m_wExtent[2] * v235;
    float boxck = std::fabs((double)boxcj);
    float boxcl = v29 + boxck;
    double v30 = boxcl;
    float boxcm = std::fabs((double)box.m_wExtent[0u]);
    float boxd = v30 + boxcm;
    float v159 = y * box.m_wBasis[0u].y + box.m_wBasis[0u].x * x + box.m_wBasis[0u].z * z;
    float v181 = v13 * box.m_wBasis[0u].y + box.m_wBasis[0u].x * v14 + box.m_wBasis[0u].z * V.z;
    double v31 = v159;
    float v160 = v181 * v175 + v159;
    if (boxd < v31 && boxd < static_cast<double>(v160)) {
        return 0;
    }
    float boxe = -boxd;
    if (boxe > v31 && v160 < static_cast<double>(boxe)) {
        return 0;
    }
    float boxcn = m_wExtent[0u] * v230;
    float boxco = std::fabs((double)boxcn);
    double v32 = boxco;
    float boxcp = m_wExtent[1] * v233;
    float boxcq = std::fabs((double)boxcp);
    double v33 = v32 + boxcq;
    float boxcr = m_wExtent[2] * v236;
    float boxcs = std::fabs((double)boxcr);
    float boxct = v33 + boxcs;
    double v34 = boxct;
    float boxcu = std::fabs((double)box.m_wExtent[1]);
    float boxf = v34 + boxcu;
    float v161 = v9->y * y + x * v9->x + v9->z * z;
    float v182 = v9->y * v13 + v14 * v9->x + v9->z * V.z;
    double v35 = v161;
    float v162 = v182 * v175 + v161;
    if (boxf < v35 && boxf < static_cast<double>(v162)) {
        return 0;
    }
    float boxg = -boxf;
    if (boxg > v35 && v162 < static_cast<double>(boxg)) {
        return 0;
    }
    float boxcv = m_wExtent[0u] * v231;
    float boxcw = std::fabs((double)boxcv);
    double v36 = boxcw;
    float boxcx = m_wExtent[1] * v234;
    float boxcy = std::fabs((double)boxcx);
    double v37 = v36 + boxcy;
    float boxcz = m_wExtent[2] * v237;
    float boxda = std::fabs((double)boxcz);
    float boxdb = v37 + boxda;
    double v38 = boxdb;
    float boxdc = std::fabs((double)box.m_wExtent[2]);
    float boxh = v38 + boxdc;
    float v163 = z * v7->z + (x * v7->x + y * v7->y);
    float v183 = V.x * v7->x + V.y * v7->y + v7->z * V.z;
    double v39 = v163;
    float v164 = v183 * v175 + v163;
    if (boxh < v39 && v164 > static_cast<double>(boxh)) {
        return 0;
    }
    float boxi = -boxh;
    if (boxi > v39 && v164 < static_cast<double>(boxi)) {
        return 0;
    }
    float boxdd = box.m_wExtent[1] * v231;
    float boxde = std::fabs((double)boxdd);
    double v40 = boxde;
    float boxdf = v230 * box.m_wExtent[2];
    float boxdg = std::fabs((double)boxdf);
    float boxdh = v40 + boxdg;
    double v41 = boxdh;
    double v42 = v235;
    float boxdi = m_wExtent[1] * v235;
    float boxdj = std::fabs((double)boxdi);
    double v43 = boxdj;
    float boxdk = m_wExtent[2] * v232;
    float boxdl = std::fabs((double)boxdk);
    float boxdm = v43 + boxdl;
    double v44 = v41 + boxdm;
    double v45 = v232;
    float boxj = v44;
    float v165 = kVxD.z * v232 - v235 * kVxD.y;
    if (!(boxj < static_cast<double>(v165))) {
        float boxk = -boxj;
        if (!(boxk > static_cast<double>(v165))) {
            v47 = v226;
        }
        else {
            double v50 = v45 * v226;
            v47 = v226;
            double v51 = v50 - v42 * v225;
            double v52 = boxk;
            float boxdo = v51;
            if (boxdo < v52) {
                return 0;
            }
        }
    }
    else {
        double v46 = v45 * v226;
        v47 = v226;
        double v48 = v46 - v42 * v225;
        double v49 = boxj;
        float boxdn = v48;
        if (boxdn > v49) {
            return 0;
        }
    }
    float boxdp = v231 * box.m_wExtent[0u];
    float boxdq = std::fabs((double)boxdp);
    double v54 = boxdq;
    float boxdr = v229 * box.m_wExtent[2];
    float boxds = std::fabs((double)boxdr);
    double v55 = v54 + boxds;
    double v56 = v229;
    float boxdt = v55;
    double v57 = boxdt;
    double v58 = v236;
    float boxdu = m_wExtent[1] * v236;
    float boxdv = std::fabs((double)boxdu);
    double v59 = boxdv;
    float boxdw = m_wExtent[2] * v233;
    float boxdx = std::fabs((double)boxdw);
    float boxdy = v59 + boxdx;
    double v60 = v57 + boxdy;
    double v61 = v233;
    float boxl = v60;
    float v166 = kVxD.z * v233 - v236 * kVxD.y;
    if (!(boxl < static_cast<double>(v166))) {
        float boxm = -boxl;
        if (boxm > static_cast<double>(v166)) {
            double v64 = v61 * v47;
            double v65 = boxm;
            float boxea = v64 - v58 * v225;
            if (boxea < v65) {
                return 0;
            }
        }
    }
    else {
        double v62 = v61 * v47;
        double v63 = boxl;
        float boxdz = v62 - v58 * v225;
        if (boxdz > v63) {
            return 0;
        }
    }
    float boxeb = box.m_wExtent[0u] * v230;
    float boxec = std::fabs((double)boxeb);
    double v66 = boxec;
    float boxed = box.m_wExtent[1] * v56;
    float boxee = std::fabs((double)boxed);
    float boxef = v66 + boxee;
    double v67 = boxef;
    double v68 = v237;
    float boxeg = m_wExtent[1] * v237;
    float boxeh = std::fabs((double)boxeg);
    double v69 = boxeh;
    float boxei = m_wExtent[2] * v234;
    float boxej = std::fabs((double)boxei);
    float boxek = v69 + boxej;
    double v70 = v67 + boxek;
    double v71 = v234;
    float boxn = v70;
    float v167 = kVxD.z * v234 - v237 * kVxD.y;
    double v72 = v167;
    if (!(boxn < static_cast<double>(v167))) {
        float boxem = -boxn;
        bool v74 = boxem < v72;
        bool v75 = boxem == v72;
        double v76 = boxem;
        if (!v74 && !v75) {
            float boxen = v47 * v71 - v68 * v225;
            if (boxen < v76) {
                return 0;
            }
        }
    }
    else {
        double v73 = boxn;
        float boxel = v47 * v71 - v68 * v225;
        if (boxel > v73) {
            return 0;
        }
    }
    float boxeo = m_wExtent[0u] * v235;
    float boxep = std::fabs((double)boxeo);
    double v77 = boxep;
    float boxeq = m_wExtent[2] * v56;
    float boxer = std::fabs((double)boxeq);
    float boxes = v77 + boxer;
    double v78 = boxes;
    float boxet = box.m_wExtent[1] * v71;
    float boxeu = std::fabs((double)boxet);
    double v79 = boxeu;
    float boxev = v233 * box.m_wExtent[2];
    float boxew = std::fabs((double)boxev);
    float boxex = v79 + boxew;
    float boxo = v78 + boxex;
    double v80 = v235;
    float v168 = v235 * kVxD.x - kVxD.z * v56;
    if (!(boxo < static_cast<double>(v168))) {
        float boxp = -boxo;
        if (!(boxp > static_cast<double>(v168))) {
            v82 = v224;
            goto LABEL_58;
        }
        double v83 = v80 * v224 - v47 * v56;
        v82 = v224;
        float v185 = v83;
        if (!(boxp > static_cast<double>(v185))) {
            goto LABEL_58;
        }
        return 0;
    }
    v81 = v80 * v224 - v47 * v56;
    v82 = v224;
    v184 = v81;
    if (boxo < static_cast<double>(v184)) {
        return 0;
    }
LABEL_58:;
    double v85 = v71 * box.m_wExtent[0u];
    double v86 = v82;
    float boxey = v85;
    float boxez = std::fabs((double)boxey);
    double v87 = boxez;
    float boxfa = v232 * box.m_wExtent[2];
    float boxfb = std::fabs((double)boxfa);
    float boxfc = v87 + boxfb;
    double v88 = boxfc;
    float boxfd = m_wExtent[0u] * v236;
    float boxfe = std::fabs((double)boxfd);
    double v89 = boxfe;
    float boxff = m_wExtent[2] * v230;
    float boxfg = std::fabs((double)boxff);
    float boxfh = v89 + boxfg;
    float boxq = v88 + boxfh;
    double v90 = v236;
    float v169 = v236 * kVxD.x - kVxD.z * v230;
    if (!(boxq < static_cast<double>(v169))) {
        float boxr = -boxq;
        if (boxr > static_cast<double>(v169)) {
            double v93 = v90 * v86 - v47 * v230;
            double v94 = boxr;
            float boxfj = v93;
            if (boxfj < v94) {
                return 0;
            }
        }
    }
    else {
        double v91 = v90 * v86 - v47 * v230;
        double v92 = boxq;
        float boxfi = v91;
        if (boxfi > v92) {
            return 0;
        }
    }
    float boxfk = box.m_wExtent[0u] * v233;
    float boxfl = std::fabs((double)boxfk);
    double v95 = boxfl;
    float boxfm = box.m_wExtent[1] * v232;
    float boxfn = std::fabs((double)boxfm);
    float boxfo = v95 + boxfn;
    double v96 = boxfo;
    float boxfp = m_wExtent[0u] * v68;
    float boxfq = std::fabs((double)boxfp);
    double v97 = boxfq;
    float boxfr = m_wExtent[2] * v231;
    float boxfs = std::fabs((double)boxfr);
    float boxft = v97 + boxfs;
    float boxs = v96 + boxft;
    double v98 = v231;
    float v170 = v68 * kVxD.x - kVxD.z * v231;
    if (!(boxs < static_cast<double>(v170))) {
        float boxt = -boxs;
        if (!(boxt > static_cast<double>(v170))) {
            v100 = v237;
        }
        else {
            double v101 = boxt;
            float boxfv = v68 * v86 - v98 * v47;
            v100 = v237;
            if (boxfv < v101) {
                return 0;
            }
        }
    }
    else {
        double v99 = boxs;
        float boxfu = v68 * v86 - v98 * v47;
        v100 = v237;
        if (boxfu > v99) {
            return 0;
        }
    }
    float boxfw = box.m_wExtent[1] * v100;
    float boxfx = std::fabs((double)boxfw);
    double v102 = boxfx;
    float boxfy = v236 * box.m_wExtent[2];
    float boxfz = std::fabs((double)boxfy);
    float boxga = v102 + boxfz;
    double v103 = boxga;
    float boxgb = m_wExtent[0u] * v232;
    float boxgc = std::fabs((double)boxgb);
    double v104 = boxgc;
    float boxgd = m_wExtent[1] * v56;
    float boxge = std::fabs((double)boxgd);
    float boxgf = v104 + boxge;
    double v105 = v103 + boxgf;
    double v106 = v232;
    float boxu = v105;
    float v171 = kVxD.y * v56 - v232 * kVxD.x;
    if (!(boxu < static_cast<double>(v171))) {
        float boxv = -boxu;
        if (!(boxv > static_cast<double>(v171))) {
            v108 = v225;
        }
        else {
            double v111 = v56 * v225;
            v108 = v225;
            double v112 = v111 - v106 * v86;
            double v113 = boxv;
            float boxgh = v112;
            if (boxgh < v113) {
                return 0;
            }
        }
    }
    else {
        double v107 = v56 * v225;
        v108 = v225;
        double v109 = v107 - v106 * v86;
        double v110 = boxu;
        float boxgg = v109;
        if (boxgg > v110) {
            return 0;
        }
    }
    double v115 = v100 * box.m_wExtent[0u];
    double v116 = v86;
    float boxgi = v115;
    float boxgj = std::fabs((double)boxgi);
    double v117 = boxgj;
    float boxgk = v235 * box.m_wExtent[2];
    float boxgl = std::fabs((double)boxgk);
    float boxgm = v117 + boxgl;
    double v118 = boxgm;
    double v119 = v233;
    float boxgn = m_wExtent[0u] * v233;
    float boxgo = std::fabs((double)boxgn);
    double v120 = boxgo;
    float boxgp = m_wExtent[1] * v230;
    float boxgq = std::fabs((double)boxgp);
    float boxgr = v120 + boxgq;
    double v121 = v118 + boxgr;
    double v122 = v230;
    float boxw = v121;
    float v172 = kVxD.y * v230 - v233 * kVxD.x;
    if (!(boxw < static_cast<double>(v172))) {
        float boxx = -boxw;
        if (boxx > static_cast<double>(v172)) {
            double v125 = v122 * v108;
            double v126 = boxx;
            float boxgt = v125 - v119 * v116;
            if (boxgt < v126) {
                return 0;
            }
        }
    }
    else {
        double v123 = v122 * v108;
        double v124 = boxw;
        float boxgs = v123 - v119 * v116;
        if (boxgs > v124) {
            return 0;
        }
    }
    float boxgu = box.m_wExtent[0u] * v236;
    float boxgv = std::fabs((double)boxgu);
    double v127 = boxgv;
    float boxgw = box.m_wExtent[1] * v235;
    float boxgx = std::fabs((double)boxgw);
    float boxgy = v127 + boxgx;
    double v128 = boxgy;
    double v129 = v234;
    float boxgz = m_wExtent[0u] * v234;
    float boxha = std::fabs((double)boxgz);
    double v130 = boxha;
    float boxhb = m_wExtent[1] * v231;
    float boxhc = std::fabs((double)boxhb);
    float boxhd = v130 + boxhc;
    double v131 = v128 + boxhd;
    double v132 = v231;
    float boxhe = v131;
    float v173 = kVxD.y * v231 - v234 * kVxD.x;
    double v133 = boxhe;
    if (!(boxhe < static_cast<double>(v173))) {
        float boxy = -v133;
        if (boxy > static_cast<double>(v173)) {
            double v134 = v132 * v108 - v116 * v129;
            double v135 = boxy;
            float boxhg = v134;
            if (boxhg < v135) {
                return 0;
            }
        }
    }
    else {
        float boxhf = v132 * v108 - v116 * v129;
        if (boxhf > v133) {
            return 0;
        }
    }
    ::new (&kVxD) NiPoint3(V.Cross(D));
    v225 = V.Dot(*v178);
    v226 = V.Dot(m_wBasis[2]);
    float boxz = std::fabs((double)v226);
    float v176 = std::fabs((double)v225);
    float v186 = std::fabs((double)m_wBasis->Dot(kVxD));
    double v136 = v186;
    float v187 = v226 * v233 - v225 * v236;
    float v188 = std::fabs((double)v187);
    double v137 = v188 * box.m_wExtent[1];
    float v189 = v226 * v232 - v225 * v235;
    float v190 = std::fabs((double)v189);
    double v138 = v137 + v190 * box.m_wExtent[0u];
    float v191 = v226 * v234 - v225 * v237;
    float v192 = std::fabs((double)v191);
    float v193 = v138 + v192 * box.m_wExtent[2];
    double v139 = v193;
    float v194 = boxz * m_wExtent[1] + m_wExtent[2] * v176;
    float v195 = v139 + v194;
    if (v195 < v136) {
        return 0;
    }
    v224 = V.Dot(*m_wBasis);
    float v174 = std::fabs((double)v224);
    float v196 = std::fabs((double)v178->Dot(kVxD));
    double v140 = v196;
    float v197 = v224 * v236 - v226 * v230;
    float v198 = std::fabs((double)v197);
    double v141 = v198 * box.m_wExtent[1];
    float v199 = v224 * v235 - v226 * v229;
    float v200 = std::fabs((double)v199);
    double v142 = v141 + v200 * box.m_wExtent[0u];
    float v201 = v224 * v237 - v226 * v231;
    float v202 = std::fabs((double)v201);
    float v203 = v142 + v202 * box.m_wExtent[2];
    float boxhh = m_wExtent[0u] * boxz + v174 * m_wExtent[2];
    float boxhi = v203 + boxhh;
    if (boxhi < v140) {
        return 0;
    }
    float boxhj = std::fabs((double)m_wBasis[2].Dot(kVxD));
    double v143 = boxhj;
    float boxhk = v225 * v230 - v224 * v233;
    float boxhl = std::fabs((double)boxhk);
    double v144 = boxhl * box.m_wExtent[1];
    float boxhm = v225 * v229 - v224 * v232;
    float boxhn = std::fabs((double)boxhm);
    double v145 = v144 + boxhn * box.m_wExtent[0u];
    float boxho = v225 * v231 - v224 * v234;
    float boxhp = std::fabs((double)boxho);
    float boxhq = v145 + boxhp * box.m_wExtent[2];
    double v146 = boxhq;
    float boxhr = m_wExtent[0u] * v176 + v174 * m_wExtent[1];
    float boxhs = v146 + boxhr;
    if (boxhs < v143) {
        return 0;
    }
    v225 = V.Dot(*v204);
    v226 = V.Dot(*v180);
    float boxba = std::fabs((double)v226);
    float v177 = std::fabs((double)v225);
    float v205 = std::fabs((double)box.m_wBasis->Dot(kVxD));
    double v148 = v205;
    float v206 = v226 * v230 - v225 * v231;
    float v207 = std::fabs((double)v206);
    double v149 = v207 * m_wExtent[0u];
    float v208 = v226 * v233 - v225 * v234;
    float v209 = std::fabs((double)v208);
    double v150 = v149 + v209 * m_wExtent[1];
    float v210 = v226 * v236 - v225 * v237;
    float v211 = std::fabs((double)v210);
    float v212 = v150 + v211 * m_wExtent[2];
    double v151 = v212;
    float v213 = v177 * box.m_wExtent[2] + boxba * box.m_wExtent[1];
    float v214 = v151 + v213;
    if (v214 < v148) {
        return 0;
    }
    v224 = V.Dot(*box.m_wBasis);
    float v179 = std::fabs((double)v224);
    float v215 = std::fabs((double)v204->Dot(kVxD));
    double v152 = v215;
    float v216 = v224 * v231 - v226 * v229;
    float v217 = std::fabs((double)v216);
    double v153 = v217 * m_wExtent[0u];
    float v218 = v224 * v234 - v226 * v232;
    float v219 = std::fabs((double)v218);
    double v154 = v153 + v219 * m_wExtent[1];
    float v220 = v224 * v237 - v226 * v235;
    float v221 = std::fabs((double)v220);
    float v222 = v154 + v221 * m_wExtent[2];
    float boxht = v179 * box.m_wExtent[2] + box.m_wExtent[0u] * boxba;
    float boxhu = v222 + boxht;
    if (boxhu < v152) {
        return 0;
    }
    float boxhv = std::fabs((double)v180->Dot(kVxD));
    double v155 = boxhv;
    float boxhw = v225 * v229 - v224 * v230;
    float boxhx = std::fabs((double)boxhw);
    double v156 = boxhx * m_wExtent[0u];
    float boxhy = v225 * v232 - v224 * v233;
    float boxhz = std::fabs((double)boxhy);
    double v157 = v156 + boxhz * m_wExtent[1];
    float boxia = v225 * v235 - v224 * v236;
    float boxib = std::fabs((double)boxia);
    float boxic = v157 + boxib * m_wExtent[2];
    double v158 = boxic;
    float boxid = v179 * box.m_wExtent[1] + box.m_wExtent[0u] * v177;
    float boxie = v158 + boxid;
    return !(boxie < v155);
}

// The time of first contact t of this box and box moving at their roots' velocities over the roots' delta time, and a
// point of contact p: Gamebryo's separating-axis code. As in Gamebryo, the A1 and A2 tests reuse u0 and u1 of the A0
// test. Each dot of the axis tests is rounded with V, and each NiAbs argument and result is rounded to float. The
// contact point computes B's dots before A's, and the cases share one tail: the four products from the last, then the
// sums from the centre; it reads tmax where Gamebryo reads t, as the game does.
int32_t NiOBBox::FindIntersection(const NiOBBox& box, float& t, NiPoint3& p) {
    const NiPoint3* A = m_wBasis;
    const NiPoint3* B = box.m_wBasis;
    const float* extA = m_wExtent;
    const float* extB = box.m_wExtent;

    float r0;
    float r1;
    float rsum;
    float u0;
    float u1;
    float C[3][3];
    float d0[3];
    float d1[3];
    float fDotV;

    NiPoint3 D = box.m_wCenter - m_wCenter;
    NiPoint3 V = box.m_pRoot->m_velocity - m_pRoot->m_velocity;

    float dt = m_pRoot->m_fDeltaTime;

    int32_t type = intrNone;
    int32_t side = 0;
    float tintr;
    float tmax = 0.0f;

    // A0
    C[0][0] = A[0].Dot(B[0]);
    C[0][1] = A[0].Dot(B[1]);
    C[0][2] = A[0].Dot(B[2]);
    u0 = d0[0] = A[0].Dot(D);
    fDotV = V.Dot(A[0]);
    u1 = d1[0] = d0[0] + dt * fDotV;
    r0 = NiAbs(extA[0]);
    r1 = NiAbs(extB[0] * C[0][0]) + NiAbs(extB[1] * C[0][1]) + NiAbs(extB[2] * C[0][2]);
    rsum = r0 + r1;
    UPDATE_TIME(intrA0);

    // A1
    C[1][0] = A[1].Dot(B[0]);
    C[1][1] = A[1].Dot(B[1]);
    C[1][2] = A[1].Dot(B[2]);
    d0[1] = A[1].Dot(D);
    fDotV = A[1].Dot(V);
    d1[1] = d0[1] + dt * fDotV;
    r0 = NiAbs(extA[1]);
    r1 = NiAbs(extB[0] * C[1][0]) + NiAbs(extB[1] * C[1][1]) + NiAbs(extB[2] * C[1][2]);
    rsum = r0 + r1;
    UPDATE_TIME(intrA1);

    // A2
    C[2][0] = A[2].Dot(B[0]);
    C[2][1] = A[2].Dot(B[1]);
    C[2][2] = A[2].Dot(B[2]);
    d0[2] = A[2].Dot(D);
    fDotV = A[2].Dot(V);
    d1[2] = d0[2] + dt * fDotV;
    r0 = NiAbs(extA[2]);
    r1 = NiAbs(extB[0] * C[2][0]) + NiAbs(extB[1] * C[2][1]) + NiAbs(extB[2] * C[2][2]);
    rsum = r0 + r1;
    UPDATE_TIME(intrA2);

    // B0
    r0 = NiAbs(extA[0] * C[0][0]) + NiAbs(extA[1] * C[1][0]) + NiAbs(extA[2] * C[2][0]);
    r1 = NiAbs(extB[0]);
    rsum = r0 + r1;
    u0 = B[0].Dot(D);
    fDotV = B[0].Dot(V);
    u1 = u0 + dt * fDotV;
    UPDATE_TIME(intrB0);

    // B1
    r0 = NiAbs(extA[0] * C[0][1]) + NiAbs(extA[1] * C[1][1]) + NiAbs(extA[2] * C[2][1]);
    r1 = NiAbs(extB[1]);
    rsum = r0 + r1;
    u0 = B[1].Dot(D);
    fDotV = B[1].Dot(V);
    u1 = u0 + dt * fDotV;
    UPDATE_TIME(intrB1);

    // B2
    r0 = NiAbs(extA[0] * C[0][2]) + NiAbs(extA[1] * C[1][2]) + NiAbs(extA[2] * C[2][2]);
    r1 = NiAbs(extB[2]);
    rsum = r0 + r1;
    u0 = B[2].Dot(D);
    fDotV = B[2].Dot(V);
    u1 = u0 + dt * fDotV;
    UPDATE_TIME(intrB2);

    // A0xB0
    r0 = NiAbs(extA[1] * C[2][0]) + NiAbs(extA[2] * C[1][0]);
    r1 = NiAbs(extB[1] * C[0][2]) + NiAbs(extB[2] * C[0][1]);
    rsum = r0 + r1;
    u0 = d0[2] * C[1][0] - d0[1] * C[2][0];
    u1 = d1[2] * C[1][0] - d1[1] * C[2][0];
    UPDATE_TIME(intrA0B0);

    // A0xB1
    r0 = NiAbs(extA[1] * C[2][1]) + NiAbs(extA[2] * C[1][1]);
    r1 = NiAbs(extB[0] * C[0][2]) + NiAbs(extB[2] * C[0][0]);
    rsum = r0 + r1;
    u0 = d0[2] * C[1][1] - d0[1] * C[2][1];
    u1 = d1[2] * C[1][1] - d1[1] * C[2][1];
    UPDATE_TIME(intrA0B1);

    // A0xB2
    r0 = NiAbs(extA[1] * C[2][2]) + NiAbs(extA[2] * C[1][2]);
    r1 = NiAbs(extB[0] * C[0][1]) + NiAbs(extB[1] * C[0][0]);
    rsum = r0 + r1;
    u0 = d0[2] * C[1][2] - d0[1] * C[2][2];
    u1 = d1[2] * C[1][2] - d1[1] * C[2][2];
    UPDATE_TIME(intrA0B2);

    // A1xB0
    r0 = NiAbs(extA[0] * C[2][0]) + NiAbs(extA[2] * C[0][0]);
    r1 = NiAbs(extB[1] * C[1][2]) + NiAbs(extB[2] * C[1][1]);
    rsum = r0 + r1;
    u0 = d0[0] * C[2][0] - d0[2] * C[0][0];
    u1 = d1[0] * C[2][0] - d1[2] * C[0][0];
    UPDATE_TIME(intrA1B0);

    // A1xB1
    r0 = NiAbs(extA[0] * C[2][1]) + NiAbs(extA[2] * C[0][1]);
    r1 = NiAbs(extB[0] * C[1][2]) + NiAbs(extB[2] * C[1][0]);
    rsum = r0 + r1;
    u0 = d0[0] * C[2][1] - d0[2] * C[0][1];
    u1 = d1[0] * C[2][1] - d1[2] * C[0][1];
    UPDATE_TIME(intrA1B1);

    // A1xB2
    r0 = NiAbs(extA[0] * C[2][2]) + NiAbs(extA[2] * C[0][2]);
    r1 = NiAbs(extB[0] * C[1][1]) + NiAbs(extB[1] * C[1][0]);
    rsum = r0 + r1;
    u0 = d0[0] * C[2][2] - d0[2] * C[0][2];
    u1 = d1[0] * C[2][2] - d1[2] * C[0][2];
    UPDATE_TIME(intrA1B2);

    // A2xB0
    r0 = NiAbs(extA[0] * C[1][0]) + NiAbs(extA[1] * C[0][0]);
    r1 = NiAbs(extB[1] * C[2][2]) + NiAbs(extB[2] * C[2][1]);
    rsum = r0 + r1;
    u0 = d0[1] * C[0][0] - d0[0] * C[1][0];
    u1 = d1[1] * C[0][0] - d1[0] * C[1][0];
    UPDATE_TIME(intrA2B0);

    // A2xB1
    r0 = NiAbs(extA[0] * C[1][1]) + NiAbs(extA[1] * C[0][1]);
    r1 = NiAbs(extB[0] * C[2][2]) + NiAbs(extB[2] * C[2][0]);
    rsum = r0 + r1;
    u0 = d0[1] * C[0][1] - d0[0] * C[1][1];
    u1 = d1[1] * C[0][1] - d1[0] * C[1][1];
    UPDATE_TIME(intrA2B1);

    // A2xB2
    r0 = NiAbs(extA[0] * C[1][2]) + NiAbs(extA[1] * C[0][2]);
    r1 = NiAbs(extB[0] * C[2][1]) + NiAbs(extB[1] * C[2][0]);
    rsum = r0 + r1;
    u0 = d0[1] * C[0][2] - d0[0] * C[1][2];
    u1 = d1[1] * C[0][2] - d1[0] * C[1][2];
    UPDATE_TIME(intrA2B2);

    t = tmax;

    const float epsilon = 1e-06f;
    int32_t i;
    int32_t j;
    float x[3];
    float y[3];
    float ad;
    float bd;
    float div;
    float tmp;

    NiPoint3 DTW;
    NiPoint3 kTV;
    const NiPoint3& V0 = m_pRoot->m_velocity;
    const NiPoint3& V1 = box.m_pRoot->m_velocity;

    // The contact point is center + tmax * velocity + the coefficients times the axes: box's for the A cases, this
    // box's otherwise.
    const NiPoint3* pkCenter = &m_wCenter;
    const NiPoint3* pkVelocity = &V0;
    const NiPoint3* pkAxis = A;
    const float* pfCoeff = x;

    switch (type) {
        case intrA0:
        case intrA1:
        case intrA2:
            i = type - 1;
            for (j = 0; j < 3; j++) {
                GET_COEFF_MINUS_PLUS(y[j], side, C[i][j], extB[j]);
            }
            pkCenter = &box.m_wCenter;
            pkVelocity = &V1;
            pkAxis = B;
            pfCoeff = y;
            break;
        case intrB0:
        case intrB1:
        case intrB2:
            j = type - 4;
            for (i = 0; i < 3; i++) {
                GET_COEFF_PLUS_MINUS(x[i], side, C[i][j], extA[i]);
            }
            break;
        case intrA0B0:
            GET_COEFF_MINUS_PLUS(x[1], side, C[2][0], extA[1]);
            GET_COEFF_PLUS_MINUS(x[2], side, C[1][0], extA[2]);
            GET_COEFF_MINUS_PLUS(y[1], side, C[0][2], extB[1]);
            GET_COEFF_PLUS_MINUS(y[2], side, C[0][1], extB[2]);
            div = 1.0f - C[0][0] * C[0][0];
            if (NiAbs(div) > epsilon) {
                kTV = tmax * V;
                DTW = D + kTV;
                bd = B[0].Dot(DTW);
                tmp = C[1][0] * x[1] + C[2][0] * x[2] - bd;
                ad = A[0].Dot(DTW);
                x[0] = (ad + C[0][0] * tmp + C[0][1] * y[1] + C[0][2] * y[2]) / div;
            }
            else {
                x[0] = 0.0f;
            }
            break;
        case intrA0B1:
            GET_COEFF_MINUS_PLUS(x[1], side, C[2][1], extA[1]);
            GET_COEFF_PLUS_MINUS(x[2], side, C[1][1], extA[2]);
            GET_COEFF_PLUS_MINUS(y[0], side, C[0][2], extB[0]);
            GET_COEFF_MINUS_PLUS(y[2], side, C[0][0], extB[2]);
            div = 1.0f - C[0][1] * C[0][1];
            if (NiAbs(div) > epsilon) {
                kTV = tmax * V;
                DTW = D + kTV;
                bd = B[1].Dot(DTW);
                tmp = C[1][1] * x[1] + C[2][1] * x[2] - bd;
                ad = A[0].Dot(DTW);
                x[0] = (ad + C[0][1] * tmp + C[0][0] * y[0] + C[0][2] * y[2]) / div;
            }
            else {
                x[0] = 0.0f;
            }
            break;
        case intrA0B2:
            GET_COEFF_MINUS_PLUS(x[1], side, C[2][2], extA[1]);
            GET_COEFF_PLUS_MINUS(x[2], side, C[1][2], extA[2]);
            GET_COEFF_MINUS_PLUS(y[0], side, C[0][1], extB[0]);
            GET_COEFF_PLUS_MINUS(y[1], side, C[0][0], extB[1]);
            div = 1.0f - C[0][2] * C[0][2];
            if (NiAbs(div) > epsilon) {
                kTV = tmax * V;
                DTW = D + kTV;
                bd = B[2].Dot(DTW);
                tmp = C[1][2] * x[1] + C[2][2] * x[2] - bd;
                ad = A[0].Dot(DTW);
                x[0] = (ad + C[0][2] * tmp + C[0][0] * y[0] + C[0][1] * y[1]) / div;
            }
            else {
                x[0] = 0.0f;
            }
            break;
        case intrA1B0:
            GET_COEFF_PLUS_MINUS(x[0], side, C[2][0], extA[0]);
            GET_COEFF_MINUS_PLUS(x[2], side, C[0][0], extA[2]);
            GET_COEFF_MINUS_PLUS(y[1], side, C[1][2], extB[1]);
            GET_COEFF_PLUS_MINUS(y[2], side, C[1][1], extB[2]);
            div = 1.0f - C[1][0] * C[1][0];
            if (NiAbs(div) > epsilon) {
                kTV = tmax * V;
                DTW = D + kTV;
                bd = B[0].Dot(DTW);
                tmp = C[0][0] * x[0] + C[2][0] * x[2] - bd;
                ad = A[1].Dot(DTW);
                x[1] = (ad + C[1][0] * tmp + C[1][1] * y[1] + C[1][2] * y[2]) / div;
            }
            else {
                x[1] = 0.0f;
            }
            break;
        case intrA1B1:
            GET_COEFF_PLUS_MINUS(x[0], side, C[2][1], extA[0]);
            GET_COEFF_MINUS_PLUS(x[2], side, C[0][1], extA[2]);
            GET_COEFF_PLUS_MINUS(y[0], side, C[1][2], extB[0]);
            GET_COEFF_MINUS_PLUS(y[2], side, C[1][0], extB[2]);
            div = 1.0f - C[1][1] * C[1][1];
            if (NiAbs(div) > epsilon) {
                kTV = tmax * V;
                DTW = D + kTV;
                bd = B[1].Dot(DTW);
                tmp = C[0][1] * x[0] + C[2][1] * x[2] - bd;
                ad = A[1].Dot(DTW);
                x[1] = (ad + C[1][1] * tmp + C[1][0] * y[0] + C[1][2] * y[2]) / div;
            }
            else {
                x[1] = 0.0f;
            }
            break;
        case intrA1B2:
            GET_COEFF_PLUS_MINUS(x[0], side, C[2][2], extA[0]);
            GET_COEFF_MINUS_PLUS(x[2], side, C[0][2], extA[2]);
            GET_COEFF_MINUS_PLUS(y[0], side, C[1][1], extB[0]);
            GET_COEFF_PLUS_MINUS(y[1], side, C[1][0], extB[1]);
            div = 1.0f - C[1][2] * C[1][2];
            if (NiAbs(div) > epsilon) {
                kTV = tmax * V;
                DTW = D + kTV;
                bd = B[2].Dot(DTW);
                tmp = C[0][2] * x[0] + C[2][2] * x[2] - bd;
                ad = A[1].Dot(DTW);
                x[1] = (ad + C[1][2] * tmp + C[1][0] * y[0] + C[1][1] * y[1]) / div;
            }
            else {
                x[1] = 0.0f;
            }
            break;
        case intrA2B0:
            GET_COEFF_MINUS_PLUS(x[0], side, C[1][0], extA[0]);
            GET_COEFF_PLUS_MINUS(x[1], side, C[0][0], extA[1]);
            GET_COEFF_MINUS_PLUS(y[1], side, C[2][2], extB[1]);
            GET_COEFF_PLUS_MINUS(y[2], side, C[2][1], extB[2]);
            div = 1.0f - C[2][0] * C[2][0];
            if (NiAbs(div) > epsilon) {
                kTV = tmax * V;
                DTW = D + kTV;
                bd = B[0].Dot(DTW);
                tmp = C[0][0] * x[0] + C[1][0] * x[1] - bd;
                ad = A[2].Dot(DTW);
                x[2] = (ad + C[2][0] * tmp + C[2][1] * y[1] + C[2][2] * y[2]) / div;
            }
            else {
                x[2] = 0.0f;
            }
            break;
        case intrA2B1:
            GET_COEFF_MINUS_PLUS(x[0], side, C[1][1], extA[0]);
            GET_COEFF_PLUS_MINUS(x[1], side, C[0][1], extA[1]);
            GET_COEFF_PLUS_MINUS(y[0], side, C[2][2], extB[0]);
            GET_COEFF_MINUS_PLUS(y[2], side, C[2][0], extB[2]);
            div = 1.0f - C[2][1] * C[2][1];
            if (NiAbs(div) > epsilon) {
                kTV = tmax * V;
                DTW = D + kTV;
                bd = B[1].Dot(DTW);
                tmp = C[0][1] * x[0] + C[1][1] * x[1] - bd;
                ad = A[2].Dot(DTW);
                x[2] = (ad + C[2][1] * tmp + C[2][0] * y[0] + C[2][2] * y[2]) / div;
            }
            else {
                x[2] = 0.0f;
            }
            break;
        case intrA2B2:
            GET_COEFF_MINUS_PLUS(x[0], side, C[1][2], extA[0]);
            GET_COEFF_PLUS_MINUS(x[1], side, C[0][2], extA[1]);
            GET_COEFF_MINUS_PLUS(y[0], side, C[2][1], extB[0]);
            GET_COEFF_PLUS_MINUS(y[1], side, C[2][0], extB[1]);
            div = 1.0f - C[2][2] * C[2][2];
            if (NiAbs(div) > epsilon) {
                kTV = tmax * V;
                DTW = D + kTV;
                bd = B[2].Dot(DTW);
                tmp = C[0][2] * x[0] + C[1][2] * x[1] - bd;
                ad = A[2].Dot(DTW);
                x[2] = (ad + C[2][2] * tmp + C[2][0] * y[0] + C[2][1] * y[1]) / div;
            }
            else {
                x[2] = 0.0f;
            }
            break;
        default: return 1;
    }

    NiPoint3 kTerm2 = pfCoeff[2] * pkAxis[2];
    NiPoint3 kTerm1 = pfCoeff[1] * pkAxis[1];
    NiPoint3 kTerm0 = pfCoeff[0] * pkAxis[0];
    NiPoint3 kMove = tmax * *pkVelocity;
    p = *pkCenter + kMove + kTerm0 + kTerm1 + kTerm2;

    return 1;
}

int32_t NiOBBox::TestIntersection(const NiPoint3& origin, const NiPoint3& dir) {
    float v24[3];
    float v17 = -3.4028235e+38f;
    int32_t v4 = 0;
    double v5 = m_wExtent[0u];
    float* p_z = &m_wBasis[0u].z;
    float v18 = std::fabs(v5);
    v24[0] = v18;
    float v19 = std::fabs((double)p_z[8]);
    v24[1] = v19;
    float v20 = std::fabs((double)p_z[9]);
    v24[2] = v20;
    float v21 = *(p_z - 5) - origin.x;
    float v22 = *(p_z - 4) - origin.y;
    float v23 = *(p_z - 3) - origin.z;
    double v7 = 3.4028235e+38f;
    do {
        float origina = v22 * *(p_z - 1) + v21 * *(p_z - 2) + v23 * *p_z;
        float dira = dir.x * *(p_z - 2) + *(p_z - 1) * dir.y + dir.z * *p_z;
        double v8 = dira;
        float dirb = std::fabs((double)dira);
        if (!(dirb > 1.0000000116860974e-07)) {
            float origind = std::fabs((double)origina);
            if (v24[v4] < static_cast<double>(origind)) {
                return 0;
            }
        }
        else {
            double v14;
            float dirc = 1.0 / v8;
            double v9 = origina;
            float originb = (v24[v4] + origina) * dirc;
            float dird = dirc * (v9 - v24[v4]);
            double v10 = originb;
            double v11 = dird;
            if (dird < static_cast<double>(originb)) {
                float dire = originb;
                float originc = v11;
                v10 = originc;
                v11 = dire;
            }
            if (!(v17 < v10)) {
                v14 = v11;
            }
            else {
                double v12 = v11;
                double v13 = v10;
                v14 = v12;
                v17 = v13;
            }
            if (v14 < v7) {
                float v16 = v14;
                v7 = v16;
            }
            if (v17 > v7 || v7 < 0.0) {
                return 0;
            }
        }
        ++v4;
        p_z += 3;
    }
    while (v4 < 3);
    return 1;
}
