#include "BSHavok/hkBSHeightFieldShape.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

void hkBSHeightFieldShapeCinfo::CalcMinMax() {
    NiPoint3* pData = this->pData;
    if (pData) {
        float z = pData->z;
        float v4 = z;
        if (this->m_xRes > 0) {
            int32_t m_xRes = this->m_xRes;
            do {
                if (this->m_zRes > 0) {
                    int32_t m_zRes = this->m_zRes;
                    do {
                        float v6 = pData->z;
                        if (!(v6 > static_cast<double>(z))) {
                            z = v6;
                        }
                        if (!(v4 > static_cast<double>(v6))) {
                            v4 = v6;
                        }
                        ++pData;
                        --m_zRes;
                    }
                    while (m_zRes);
                }
                --m_xRes;
            }
            while (m_xRes);
        }
        m_maxHeight = fOffset + v4;
        m_minHeight = fOffset + z;
    }
}

float hkBSHeightFieldShape::getHeightAtImpl(int32_t aX, int32_t aZ) const {
    return (float)(pData[aX + (m_xRes * (invZRes - aZ))].z + fOffset);
}

// The heightfield's triangles are never flipped.
hkBool* hkBSHeightFieldShape::getTriangleFlipImpl(hkBool* arResult) const {
    arResult->m_bool = false;
    return arResult;
}
