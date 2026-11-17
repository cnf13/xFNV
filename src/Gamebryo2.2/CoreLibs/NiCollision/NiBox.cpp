#include "Gamebryo2.2/CoreLibs/NiCollision/NiBox.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"

// The centres, then each axis and its extent; any difference (or a NaN) makes the boxes unequal.
bool NiBox::operator==(const NiBox& kBox) const {
    if (kBox.m_kCenter.x != m_kCenter.x || kBox.m_kCenter.y != m_kCenter.y || kBox.m_kCenter.z != m_kCenter.z) {
        return false;
    }
    for (int32_t i = 0; i < 3; ++i) {
        if (kBox.m_akAxis[i].x != m_akAxis[i].x || kBox.m_akAxis[i].y != m_akAxis[i].y ||
            kBox.m_akAxis[i].z != m_akAxis[i].z || kBox.m_afExtent[i] != m_afExtent[i]) {
            return false;
        }
    }
    return true;
}

// The centre and the three axes, then the three extents in one read.
void NiBox::LoadBinary(NiStream& stream) {
    m_kCenter.LoadBinary(stream);
    for (uint32_t i = 0; i < 3; ++i) {
        m_akAxis[i].LoadBinary(stream);
    }
    NiStreamLoadBinary(stream, m_afExtent, 3);
}

void NiBox::SaveBinary(NiStream& stream) {
    m_kCenter.SaveBinary(stream);
    for (uint32_t i = 0; i < 3; ++i) {
        m_akAxis[i].SaveBinary(stream);
    }
    NiStreamSaveBinary(stream, m_afExtent, 3);
}

void NiBox::CreateFromData(int32_t aiQuantity, const NiPoint3* pkVertex) {
    uint16_t usQuantity = static_cast<uint16_t>(aiQuantity);
    float x = pkVertex->x;
    float v31 = pkVertex->x;
    double v6 = v31;
    float v35 = v31;
    float fMaxY = pkVertex->y;
    double v7 = fMaxY;
    float pkVertexa = fMaxY;
    double fMaxZ = pkVertex->z;
    m_kCenter.x = x;
    float v33 = fMaxZ;
    double v9 = v33;
    m_kCenter.y = pkVertex->y;
    float v30 = v33;
    m_kCenter.z = pkVertex->z;
    if (usQuantity > 1u) {
        const float* p_z = &pkVertex[1].z;
        int32_t v11 = static_cast<uint16_t>(usQuantity - 1);
        do {
            m_kCenter.x = *(p_z - 2) + m_kCenter.x;
            m_kCenter.y = m_kCenter.y + *(p_z - 1);
            m_kCenter.z = *p_z + m_kCenter.z;
            if (!(v35 > static_cast<double>(*(p_z - 2)))) {
                if (*(p_z - 2) > v6) {
                    v6 = *(p_z - 2);
                }
            }
            else {
                v35 = *(p_z - 2);
            }
            if (!(pkVertexa > static_cast<double>(*(p_z - 1)))) {
                if (*(p_z - 1) > v7) {
                    v7 = *(p_z - 1);
                }
            }
            else {
                pkVertexa = *(p_z - 1);
            }
            if (!(v30 > static_cast<double>(*p_z))) {
                if (*p_z > v9) {
                    v9 = *p_z;
                }
            }
            else {
                v30 = *p_z;
            }
            p_z += 3;
            --v11;
        }
        while (v11);
    }
    float v34 = 1.0 / static_cast<double>(usQuantity);
    float v40 = v34 * m_kCenter.x;
    float v42 = m_kCenter.y * v34;
    double v12 = v34 * m_kCenter.z;
    m_kCenter.x = v40;
    m_kCenter.y = v42;
    float v44 = v12;
    m_kCenter.z = v44;
    if (v6 == v35 || (v7 == pkVertexa || v9 == v30)) {
        m_akAxis[0].x = 1.0f;
        m_akAxis[0].y = 0.0f;
        m_akAxis[0].z = 0.0f;
        m_akAxis[1].x = 0.0f;
        m_akAxis[1].y = 1.0f;
        m_akAxis[2].x = 0.0f;
        m_akAxis[2].y = 0.0f;
        m_akAxis[1].z = 0.0f;
        m_akAxis[2].z = 1.0f;
        float fXLength = (v6 - v35) * 0.5;
        float fYLength = (v7 - pkVertexa) * 0.5;
        float fZLength = 0.5 * (v9 - v30);
        if (fXLength <= 0.0f) {
            fXLength = 0.1f;
        }
        if (fYLength <= 0.0f) {
            fYLength = 0.1f;
        }
        if (fZLength <= 0.0f) {
            fZLength = 0.1f;
        }
        m_afExtent[0] = fXLength;
        m_afExtent[1] = fYLength;
        m_afExtent[2] = fZLength;
    }
    else {
        float a2[3];
        NiMatrix3 kCovar = NiMatrix3::ZERO;
        if (usQuantity) {
            int32_t v13 = usQuantity;
            double v14 = m_kCenter.x * m_kCenter.x;
            double v15 = m_kCenter.y * m_kCenter.x;
            double v16 = m_kCenter.x * m_kCenter.z;
            double v17 = m_kCenter.y * m_kCenter.y;
            double v18 = m_kCenter.y * m_kCenter.z;
            double v19 = m_kCenter.z * m_kCenter.z;
            do {
                --v13;
                kCovar.m_pEntry[0][0] = kCovar.m_pEntry[0][0] + v14;
                kCovar.m_pEntry[0][1] = kCovar.m_pEntry[0][1] + v15;
                kCovar.m_pEntry[0][2] = kCovar.m_pEntry[0][2] + v16;
                kCovar.m_pEntry[1][1] = kCovar.m_pEntry[1][1] + v17;
                kCovar.m_pEntry[1][2] = kCovar.m_pEntry[1][2] + v18;
                kCovar.m_pEntry[2][2] = kCovar.m_pEntry[2][2] + v19;
            }
            while (v13);
        }
        kCovar.m_pEntry[1][0] = kCovar.m_pEntry[0][1];
        kCovar.m_pEntry[2][0] = kCovar.m_pEntry[0][2];
        kCovar.m_pEntry[2][1] = kCovar.m_pEntry[1][2];
        NiMatrix3::EigenSolveSymmetric(&kCovar, nullptr, a2, &m_akAxis[0].x);
        float v37 = pkVertex->x - m_kCenter.x;
        float v41 = v37;
        float v38 = pkVertex->y - m_kCenter.y;
        float v43 = v38;
        float v39 = pkVertex->z - m_kCenter.z;
        float v45 = v39;
        if (usQuantity > 1u) {
            const float* v20 = &pkVertex[1].z;
            int32_t v21 = static_cast<uint16_t>(usQuantity - 1);
            do {
                float v46 = *(v20 - 2) - m_kCenter.x;
                float v50 = *(v20 - 1) - m_kCenter.y;
                float v54 = *v20 - m_kCenter.z;
                double v22 = v46;
                double v23 = v50;
                float usQuantitya = m_akAxis[0].z * v54 + (m_akAxis[0].y * v50 + v46 * m_akAxis[0].x);
                if (!(v37 > static_cast<double>(usQuantitya))) {
                    if (v41 < static_cast<double>(usQuantitya)) {
                        v41 = m_akAxis[0].z * v54 + (m_akAxis[0].y * v50 + v46 * m_akAxis[0].x);
                    }
                }
                else {
                    v37 = m_akAxis[0].z * v54 + (m_akAxis[0].y * v50 + v46 * m_akAxis[0].x);
                }
                float usQuantityb = m_akAxis[1].x * v22 + v23 * m_akAxis[1].y + m_akAxis[1].z * v54;
                if (!(v38 > static_cast<double>(usQuantityb))) {
                    if (v43 < static_cast<double>(usQuantityb)) {
                        v43 = m_akAxis[1].x * v22 + v23 * m_akAxis[1].y + m_akAxis[1].z * v54;
                    }
                }
                else {
                    v38 = m_akAxis[1].x * v22 + v23 * m_akAxis[1].y + m_akAxis[1].z * v54;
                }
                float usQuantityc = v54 * m_akAxis[2].z + (v23 * m_akAxis[2].y + v22 * m_akAxis[2].x);
                if (!(v39 > static_cast<double>(usQuantityc))) {
                    if (v45 < static_cast<double>(usQuantityc)) {
                        v45 = v54 * m_akAxis[2].z + (v23 * m_akAxis[2].y + v22 * m_akAxis[2].x);
                    }
                }
                else {
                    v39 = v54 * m_akAxis[2].z + (v23 * m_akAxis[2].y + v22 * m_akAxis[2].x);
                }
                v20 += 3;
                --v21;
            }
            while (v21);
        }
        float usQuantityd = (v41 + v37) * 0.5;
        float v47 = m_akAxis[0].x * usQuantityd;
        float v51 = m_akAxis[0].y * usQuantityd;
        float v55 = usQuantityd * m_akAxis[0].z;
        float usQuantitye = v47 + m_kCenter.x;
        double v24 = usQuantitye;
        m_kCenter.x = usQuantitye;
        float usQuantityf = m_kCenter.y + v51;
        double v25 = usQuantityf;
        m_kCenter.y = usQuantityf;
        float usQuantityg = v55 + m_kCenter.z;
        double v26 = usQuantityg;
        m_kCenter.z = usQuantityg;
        float usQuantityh = (v43 + v38) * 0.5;
        float v48 = m_akAxis[1].x * usQuantityh;
        float v52 = m_akAxis[1].y * usQuantityh;
        float v56 = usQuantityh * m_akAxis[1].z;
        float usQuantityi = v24 + v48;
        m_kCenter.x = usQuantityi;
        double v27 = usQuantityi;
        float usQuantityj = v25 + v52;
        m_kCenter.y = usQuantityj;
        double v28 = usQuantityj;
        float usQuantityk = v26 + v56;
        double v29 = usQuantityk;
        m_kCenter.z = usQuantityk;
        float usQuantityl = (v45 + v39) * 0.5;
        float v49 = usQuantityl * m_akAxis[2].x;
        float v53 = m_akAxis[2].y * usQuantityl;
        float v57 = usQuantityl * m_akAxis[2].z;
        m_kCenter.x = v27 + v49;
        m_kCenter.y = v28 + v53;
        m_kCenter.z = v29 + v57;
        m_afExtent[0] = (v41 - v37) * 0.5;
        m_afExtent[1] = (v43 - v38) * 0.5;
        m_afExtent[2] = 0.5 * (v45 - v39);
    }
}
