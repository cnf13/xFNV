#include "SpeedTree/LeafGeometry.h"

#include "BSCore/MemoryManager.h"
#include "SDK/MSVC/string.h"
#include "SpeedTree/BillboardLeaf.h"
#include "SpeedTree/LeafInfo.h"
#include "SpeedTree/LibVector_Source/IdvVector.h"
#include "SpeedTree/SpeedTreeRT.h"
#include "SpeedTree/UpVector.h"
#include "SpeedTree/WindEngine.h"

// SLeaf's constructor is its Init.
CLeafGeometry::SLodGeometry::SLodGeometry() : m_bValid(false), m_pOrigCenterCoords(nullptr) {
    Init();
}

// The leaf arrays are raw ourMalloc buffers, const in SLeaf; freed as FreeLODDataArrays does.
CLeafGeometry::SLodGeometry::~SLodGeometry() {
    ourFree(const_cast<uint8_t*>(m_pLeafMapIndices));
    ourFree(const_cast<uint8_t*>(m_pLeafClusterIndices));
    ourFree(const_cast<float*>(m_pCenterCoords));
    ourFree(const_cast<uint32_t*>(m_pColors));
    ourFree(const_cast<float*>(m_pNormals));
    ourFree(const_cast<float*>(m_pBinormals));
    ourFree(const_cast<float*>(m_pTangents));
    ourFree(const_cast<float**>(m_pLeafMapTexCoords));
    ourFree(const_cast<float**>(m_pLeafMapCoords));
    ourFree(const_cast<float*>(m_pWindWeights));
    ourFree(const_cast<uint8_t*>(m_pWindMatrixIndices));
    m_pLeafMapIndices = nullptr;
    m_pLeafClusterIndices = nullptr;
    m_pCenterCoords = nullptr;
    m_pColors = nullptr;
    m_pNormals = nullptr;
    m_pBinormals = nullptr;
    m_pTangents = nullptr;
    m_pLeafMapTexCoords = nullptr;
    m_pLeafMapCoords = nullptr;
    m_pWindWeights = nullptr;
    m_pWindMatrixIndices = nullptr;
    ourFree(m_pOrigCenterCoords);
    m_pOrigCenterCoords = nullptr;
}

void CLeafGeometry::SLodGeometry::operator delete(void* apMemory) {
    ourFree(apMemory);
}

void CLeafGeometry::SLodGeometry::operator delete[](void* apMemory) {
    ourFree(apMemory);
}

CLeafGeometry::CLeafGeometry(CWindEngine* pWindEngine)
  : m_bManualLighting(false), m_bVertexWeighting(false), m_pWindEngine(pWindEngine), m_usNumRockingGroups(3),
    m_pTimeOffsets(nullptr), m_pLeafVertexes(nullptr), m_pLeafTexCoords(nullptr), m_pVertexProgramTable(nullptr),
    m_usNumTextures(0), m_pLeafDimensions(nullptr), m_pLeafOrigins(nullptr), m_usNumLods(0), m_pLods(nullptr) {}

// The dimensions, origins and vertex program table are raw ourMalloc buffers; the LODs an array of SLodGeometry.
CLeafGeometry::~CLeafGeometry() {
    ourFree(m_pLeafDimensions);
    ourFree(m_pLeafOrigins);
    delete[] m_pLods;
    ourFree(m_pVertexProgramTable);
    m_pTimeOffsets = nullptr;
    m_pLeafTexCoords = nullptr;
    m_pLeafDimensions = nullptr;
    m_pLeafOrigins = nullptr;
    m_pLods = nullptr;
    m_pVertexProgramTable = nullptr;
}

void CLeafGeometry::Invalidate() {
    if (m_pLods) {
        for (int32_t i = 0; i < m_usNumLods; ++i) {
            m_pLods[i].m_bValid = false;
        }
    }
}

const float* CLeafGeometry::GetLeafBillboardTable(uint32_t& nEntryCount) {
    float* pvDest = m_pVertexProgramTable;
    uint32_t nCount = m_usNumTextures * m_usNumRockingGroups * 2 * 4 * 4;
    if (pvDest && m_pLeafVertexes && *m_pLeafVertexes && m_pVertexProgramTable) {
        rt::memcpy(pvDest, *m_pLeafVertexes, nCount * sizeof(float));
        pvDest += nCount;
        nEntryCount = pvDest - m_pVertexProgramTable;
    }
    return m_pVertexProgramTable;
}

void CLeafGeometry::SetTextureCoords(uint32_t nLeafMapIndex, const float* pTexCoords) {
    float fSign;
    float* v5;
    float* v6;
    if (m_pLeafTexCoords && pTexCoords) {
        if (CSpeedTreeRT::GetTextureFlip()) {
            fSign = (-0x1p+0f);
        }
        else {
            fSign = 1.0f;
        }
        v5 = &m_pLeafTexCoords[16 * nLeafMapIndex];
        v5[0] = pTexCoords[0];
        v5[1] = fSign * pTexCoords[1];
        v5[2] = pTexCoords[2];
        v5[3] = fSign * pTexCoords[3];
        v5[4] = pTexCoords[4];
        v5[5] = fSign * pTexCoords[5];
        v5[6] = pTexCoords[6];
        v5[7] = fSign * pTexCoords[7];
        v6 = &m_pLeafTexCoords[(16 * nLeafMapIndex) + 8];
        v6[0] = pTexCoords[2];
        v6[1] = fSign * pTexCoords[1];
        v6[2] = pTexCoords[0];
        v6[3] = fSign * pTexCoords[3];
        v6[4] = pTexCoords[6];
        v6[5] = fSign * pTexCoords[5];
        v6[6] = pTexCoords[4];
        v6[7] = fSign * pTexCoords[7];
    }
}

// Bethesda's addition (not in speedtree-2).
void CLeafGeometry::SmallUpdate(
    CSpeedTreeRT::SGeometry::SLeaf& sGeometry,
    uint16_t usLodLevel,
    float fAzimuth,
    float fPitch,
    float fSizeIncreaseFactor
) {
    if (m_pLods &&
        usLodLevel < m_usNumLods &&
        m_pLeafVertexes &&
        m_pLeafOrigins &&
        m_pLeafDimensions &&
        m_pWindEngine &&
        m_pTimeOffsets) {
        sGeometry = m_pLods[usLodLevel];
        sGeometry.m_bIsActive = true;
        sGeometry.m_nDiscreteLodLevel = usLodLevel;
    }
}

// Builds the LOD's leaf billboards once, then copies its SLeaf part into sGeometry. There is no rocking rotation: each
// billboard corner is the raw corner, and fAzimuth and fPitch are unused.
void CLeafGeometry::Update(
    CSpeedTreeRT::SGeometry::SLeaf& sGeometry,
    uint16_t usLodLevel,
    float fAzimuth,
    float fPitch,
    float fSizeIncreaseFactor
) {
    if (m_pLods &&
        usLodLevel < m_usNumLods &&
        m_pLeafVertexes &&
        m_pLeafOrigins &&
        m_pLeafDimensions &&
        m_pWindEngine &&
        m_pTimeOffsets) {
        SLodGeometry* pLod = m_pLods + usLodLevel;
        if (!pLod->m_bValid) {
            if (m_pWindEngine->m_eLeafMethod == CSpeedTreeRT::WIND_CPU) {
                ComputeWindEffect(usLodLevel);
            }
            float fSizeMultiplier = 1.0f;
            if (usLodLevel > 0) {
                fSizeMultiplier = 1.0f + fSizeIncreaseFactor * static_cast<float>(usLodLevel);
            }
            // Value-initialised, so zeroed.
            CVec3 acRawCorners[4]{};
            CVec3 acBillboardedCorners[4]{};
            for (int32_t i = 0; i < m_usNumTextures * 2; ++i) {
                CVec3 cOrigin = m_pLeafOrigins[i / 2];
                if (!(i % 2)) {
                    cOrigin.m_afData[0] = 1.0f - cOrigin.m_afData[0];
                }
                const CVec3& cSize = m_pLeafDimensions[i / 2];
                float afP1[3] = {
                    0.0f,
                    (fSizeMultiplier * (1.0f - cOrigin.m_afData[0]) * cSize.m_afData[0]),
                    (fSizeMultiplier * (1.0f - cOrigin.m_afData[1]) * cSize.m_afData[1])
                };
                float afP2[3] = {0.0f, (fSizeMultiplier * -(cOrigin.m_afData[0] * cSize.m_afData[0])), afP1[2]};
                float afP3[3] = {0.0f, afP2[1], (fSizeMultiplier * -(cOrigin.m_afData[1] * cSize.m_afData[1]))};
                float afP4[3] = {0.0f, afP1[1], afP3[2]};
                Assign3d(acRawCorners[0].m_afData, afP1);
                Assign3d(acRawCorners[1].m_afData, afP2);
                Assign3d(acRawCorners[2].m_afData, afP3);
                Assign3d(acRawCorners[3].m_afData, afP4);
                for (int32_t j = 0; j < m_usNumRockingGroups; ++j) {
                    acBillboardedCorners[0] = acRawCorners[0];
                    acBillboardedCorners[1] = acRawCorners[1];
                    acBillboardedCorners[2] = acRawCorners[2];
                    acBillboardedCorners[3] = acRawCorners[3];
                    float* pBillboard = m_pLeafVertexes[usLodLevel] + i * (m_usNumRockingGroups * 16) + j * 16;
                    rt::memcpy(pBillboard, acBillboardedCorners[0].m_afData, 3 * sizeof(float));
                    pBillboard += 4;
                    rt::memcpy(pBillboard, acBillboardedCorners[1].m_afData, 3 * sizeof(float));
                    pBillboard += 4;
                    rt::memcpy(pBillboard, acBillboardedCorners[2].m_afData, 3 * sizeof(float));
                    pBillboard += 4;
                    rt::memcpy(pBillboard, acBillboardedCorners[3].m_afData, 3 * sizeof(float));
                    pBillboard += 4;
                }
            }
            pLod->m_bValid = true;
        }
        // The SLeaf part of the LOD, all 0x3C bytes (rep movsd of 15 dwords).
        sGeometry = *pLod;
        sGeometry.m_bIsActive = true;
        sGeometry.m_nDiscreteLodLevel = usLodLevel;
    }
}

void CLeafGeometry::FreeLODDataArrays() {
    if (m_pLods && m_usNumLods) {
        for (int32_t i = 0; i < m_usNumLods; ++i) {
            ourFree(const_cast<uint8_t*>(m_pLods[i].m_pLeafMapIndices));
            ourFree(const_cast<uint8_t*>(m_pLods[i].m_pLeafClusterIndices));
            ourFree(const_cast<float*>(m_pLods[i].m_pCenterCoords));
            ourFree(const_cast<uint32_t*>(m_pLods[i].m_pColors));
            ourFree(const_cast<float*>(m_pLods[i].m_pNormals));
            ourFree(const_cast<float*>(m_pLods[i].m_pBinormals));
            ourFree(const_cast<float*>(m_pLods[i].m_pTangents));
            ourFree(const_cast<float**>(m_pLods[i].m_pLeafMapTexCoords));
            ourFree(const_cast<float**>(m_pLods[i].m_pLeafMapCoords));
            ourFree(const_cast<float*>(m_pLods[i].m_pWindWeights));
            ourFree(const_cast<uint8_t*>(m_pLods[i].m_pWindMatrixIndices));
            m_pLods[i].m_pLeafMapIndices = nullptr;
            m_pLods[i].m_pLeafClusterIndices = nullptr;
            m_pLods[i].m_pCenterCoords = nullptr;
            m_pLods[i].m_pColors = nullptr;
            m_pLods[i].m_pNormals = nullptr;
            m_pLods[i].m_pBinormals = nullptr;
            m_pLods[i].m_pTangents = nullptr;
            m_pLods[i].m_pLeafMapTexCoords = nullptr;
            m_pLods[i].m_pLeafMapCoords = nullptr;
            m_pLods[i].m_pWindWeights = nullptr;
            m_pLods[i].m_pWindMatrixIndices = nullptr;
            ourFree(m_pLods[i].m_pOrigCenterCoords);
            m_pLods[i].m_pOrigCenterCoords = nullptr;
        }
    }
}

// Each LOD's leaf centres (and, with vertex weighting, the still centres) are moved by cTransform: copied into a
// CVec3, transformed and written back through memcpy. The leaf dimensions then scale by the
// matrix's [0][0], read once.
void CLeafGeometry::Transform(const CTransform& cTransform) {
    if (m_pLods && m_pLeafDimensions) {
        for (int32_t nLod = 0; nLod < m_usNumLods; ++nLod) {
            SLodGeometry* pLod = &m_pLods[nLod];
            for (int32_t nLeaf = 0; nLeaf < pLod->m_usLeafCount; ++nLeaf) {
                float* pCenter = const_cast<float*>(pLod->m_pCenterCoords) + nLeaf * 3;
                float fX = pCenter[0];
                float fY = pCenter[1];
                float fZ = pCenter[2];
                CVec3 cCenter;
                cCenter.m_afData[0] = fX;
                cCenter.m_afData[1] = fY;
                cCenter.m_afData[2] = fZ;
                cCenter = cCenter * cTransform;
                rt::memcpy(pCenter, &cCenter, sizeof(float) * 3);
                if (m_bVertexWeighting) {
                    float* pOrigCenter = pLod->m_pOrigCenterCoords + nLeaf * 3;
                    float fOrigX = pOrigCenter[0];
                    float fOrigY = pOrigCenter[1];
                    float fOrigZ = pOrigCenter[2];
                    CVec3 cOrigCenter;
                    cOrigCenter.m_afData[0] = fOrigX;
                    cOrigCenter.m_afData[1] = fOrigY;
                    cOrigCenter.m_afData[2] = fOrigZ;
                    cOrigCenter = cOrigCenter * cTransform;
                    rt::memcpy(pOrigCenter, &cOrigCenter, sizeof(float) * 3);
                }
            }
        }
        float fScale = cTransform.m_afData[0][0];
        for (int32_t i = 0; i < m_usNumTextures; ++i) {
            m_pLeafDimensions[i] *= fScale;
        }
    }
}

void CLeafGeometry::Init(
    uint16_t usNumLeafLods,
    const std::vector<CBillboardLeaf*, std::allocator<CBillboardLeaf*>>* pvLeafLods,
    const SIdvLeafInfo& sLeafInfo
) {
    CVec3* v4;
    CVec3* v5;
    m_usNumRockingGroups = sLeafInfo.m_nNumRockingGroups;
    m_pTimeOffsets = sLeafInfo.m_pTimeOffsets;
    m_pLeafVertexes = sLeafInfo.m_pLeafVertexes;
    m_pLeafTexCoords = sLeafInfo.m_pLeafTexCoords;
    this->m_usNumTextures = static_cast<uint16_t>(sLeafInfo.m_vLeafTextures.size());
    int32_t m_usNumTextures = this->m_usNumTextures;
    CVec3* v13 = static_cast<CVec3*>(
        ourMalloc(this->m_usNumTextures <= SIZE_MAX / sizeof(CVec3) ? sizeof(CVec3) * m_usNumTextures : SIZE_MAX)
    );
    if (v13) {
        int32_t v9 = m_usNumTextures;
        for (float* i = v13->m_afData; --v9 >= 0; i += 3) {
            i[2] = 0.0f;
            i[1] = 0.0f;
            i[0] = 0.0f;
        }
        v5 = v13;
    }
    else {
        v5 = nullptr;
    }
    m_pLeafDimensions = v5;
    int32_t v12 = this->m_usNumTextures;
    CVec3* v11 = static_cast<CVec3*>(
        ourMalloc(this->m_usNumTextures <= SIZE_MAX / sizeof(CVec3) ? sizeof(CVec3) * v12 : SIZE_MAX)
    );
    if (v11) {
        int32_t v7 = v12;
        for (float* j = v11->m_afData; --v7 >= 0; j += 3) {
            j[2] = 0.0f;
            j[1] = 0.0f;
            j[0] = 0.0f;
        }
        v4 = v11;
    }
    else {
        v4 = nullptr;
    }
    m_pLeafOrigins = v4;
    for (int32_t i_2 = 0; i_2 < this->m_usNumTextures; ++i_2) {
        m_pLeafDimensions[i_2] = sLeafInfo.m_vLeafTextures[i_2].m_cSizeUsed;
        m_pLeafOrigins[i_2] = sLeafInfo.m_vLeafTextures[i_2].m_cOrigin;
    }
    uint32_t v16 = 32 * (m_usNumRockingGroups * this->m_usNumTextures);
    m_pVertexProgramTable = static_cast<float*>(
        ourMalloc(v16 <= SIZE_MAX / sizeof(float) ? sizeof(float) * v16 : SIZE_MAX)
    );
    InitLods(usNumLeafLods, pvLeafLods);
}

void CLeafGeometry::ComputeExtents(CRegion& cExtents) const {
    if (m_pLeafOrigins && m_pLeafDimensions && m_pLods) {
        using apMemory_t = std::vector<float, std::allocator<float>>;
        apMemory_t apMemory;
        for (int32_t i = 0; i < m_usNumTextures; ++i) {
            CVec3 v36;
            CVec3 v38;
            CVec3 v40;
            CVec3 cOrigin;
            CVec3 cScaledOrigin;
            float v20 = m_pLeafDimensions[i].m_afData[0] * m_pLeafOrigins[i].m_afData[0];
            float v21 = m_pLeafDimensions[i].m_afData[1] * m_pLeafOrigins[i].m_afData[1];
            float fLongest = 0.0f;
            cScaledOrigin.m_afData[0] = v20;
            cScaledOrigin.m_afData[1] = v21;
            cScaledOrigin.m_afData[2] = 0.0f;
            cOrigin.m_afData[0] = 0.0f;
            cOrigin.m_afData[1] = 0.0f;
            cOrigin.m_afData[2] = 0.0f;
            double v2 = cScaledOrigin.Distance(cOrigin);
            if (fLongest < v2) {
                CVec3 cVec;
                cVec.m_afData[0] = 0.0f;
                cVec.m_afData[1] = 0.0f;
                cVec.m_afData[2] = 0.0f;
                fLongest = cScaledOrigin.Distance(cVec);
            }
            v40.m_afData[0] = m_pLeafDimensions[i].m_afData[0];
            v40.m_afData[1] = 0.0f;
            v40.m_afData[2] = 0.0f;
            double v3 = cScaledOrigin.Distance(v40);
            if (fLongest < v3) {
                CVec3 v39;
                v39.m_afData[0] = m_pLeafDimensions[i].m_afData[0];
                v39.m_afData[1] = 0.0f;
                v39.m_afData[2] = 0.0f;
                fLongest = cScaledOrigin.Distance(v39);
            }
            float v19 = m_pLeafDimensions[i].m_afData[1];
            v38.m_afData[0] = m_pLeafDimensions[i].m_afData[0];
            v38.m_afData[1] = v19;
            v38.m_afData[2] = 0.0f;
            double v4 = cScaledOrigin.Distance(v38);
            if (fLongest < v4) {
                CVec3 v37;
                float v18 = m_pLeafDimensions[i].m_afData[1];
                v37.m_afData[0] = m_pLeafDimensions[i].m_afData[0];
                v37.m_afData[1] = v18;
                v37.m_afData[2] = 0.0f;
                fLongest = cScaledOrigin.Distance(v37);
            }
            float v17 = m_pLeafDimensions[i].m_afData[1];
            v36.m_afData[0] = 0.0f;
            v36.m_afData[1] = v17;
            v36.m_afData[2] = 0.0f;
            double v5 = cScaledOrigin.Distance(v36);
            if (fLongest < v5) {
                CVec3 v35;
                float v16 = m_pLeafDimensions[i].m_afData[1];
                v35.m_afData[0] = 0.0f;
                v35.m_afData[1] = v16;
                v35.m_afData[2] = 0.0f;
                fLongest = cScaledOrigin.Distance(v35);
            }
            apMemory.push_back(fLongest);
        }
        for (int32_t nLod = 0; nLod < m_usNumLods; ++nLod) {
            CLeafGeometry::SLodGeometry* pLod = &m_pLods[nLod];
            for (uint32_t j = 0; j < pLod->m_usLeafCount; ++j) {
                CVec3 cMinZ;
                CVec3 cMaxZ;
                CVec3 cMinY;
                CVec3 cMaxY;
                CVec3 cMinX;
                CVec3 cMaxX;
                CRegion cLeafSwingExtents;
                const float* pPos = &pLod->m_pCenterCoords[3 * j];
                float fY = pPos[1];
                float fZ = pPos[2];
                float fX = pPos[0];
                float fLongestSide = apMemory[pLod->m_pLeafMapIndices[j] / 2];
                float fMaxX = fLongestSide + fX;
                float fMinX = fX - fLongestSide;
                float fMaxY = fLongestSide + fY;
                float fMinY = fY - fLongestSide;
                float fMaxZ = fLongestSide + fZ;
                float fMinZ = fZ - fLongestSide;
                cMaxX.m_afData[0] = fMaxX;
                cMaxX.m_afData[1] = fY;
                cMaxX.m_afData[2] = fZ;
                cLeafSwingExtents = cLeafSwingExtents ^ cMaxX;
                cMinX.m_afData[0] = fMinX;
                cMinX.m_afData[1] = fY;
                cMinX.m_afData[2] = fZ;
                cLeafSwingExtents = cLeafSwingExtents ^ cMinX;
                cMaxY.m_afData[0] = fX;
                cMaxY.m_afData[1] = fMaxY;
                cMaxY.m_afData[2] = fZ;
                cLeafSwingExtents = cLeafSwingExtents ^ cMaxY;
                cMinY.m_afData[0] = fX;
                cMinY.m_afData[1] = fMinY;
                cMinY.m_afData[2] = fZ;
                cLeafSwingExtents = cLeafSwingExtents ^ cMinY;
                cMaxZ.m_afData[0] = fX;
                cMaxZ.m_afData[1] = fY;
                cMaxZ.m_afData[2] = fMaxZ;
                cLeafSwingExtents = cLeafSwingExtents ^ cMaxZ;
                cMinZ.m_afData[0] = fX;
                cMinZ.m_afData[1] = fY;
                cMinZ.m_afData[2] = fMinZ;
                cLeafSwingExtents = cLeafSwingExtents ^ cMinZ;
                cExtents = cExtents + cLeafSwingExtents;
            }
        }
    }
}

// Each leaf centre is moved by its wind matrix and blended back towards the still centre by the leaf's wind weight. The
// blend is computed in double and each component is stored as a float, z first.
void CLeafGeometry::ComputeWindEffect(uint16_t nLodLevel) {
    if (m_pWindEngine && nLodLevel < m_usNumLods && m_bVertexWeighting && m_pLods) {
        SLodGeometry* pLod = &m_pLods[nLodLevel];
        if (pLod && pLod->m_pOrigCenterCoords && pLod->m_pCenterCoords) {
            CVec3 cWindEffect;
            cWindEffect.m_afData[2] = 0.0f;
            cWindEffect.m_afData[1] = 0.0f;
            cWindEffect.m_afData[0] = 0.0f;
            CVec3 cInterpWindEffect;
            cInterpWindEffect.m_afData[2] = 0.0f;
            cInterpWindEffect.m_afData[1] = 0.0f;
            cInterpWindEffect.m_afData[0] = 0.0f;
            for (int32_t i = 0; i < pLod->m_usLeafCount; ++i) {
                float fWeight = pLod->m_pWindWeights[i];
                const float* pOrigCoord = &pLod->m_pOrigCenterCoords[3 * i];
                cWindEffect.m_afData[0] = pOrigCoord[0];
                cWindEffect.m_afData[1] = pOrigCoord[1];
                cWindEffect.m_afData[2] = pOrigCoord[2];
                uint8_t ucWindGroup = pLod->m_pWindMatrixIndices[i];
                cWindEffect = cWindEffect * CWindEngine::m_cWindMatrices.m_pMatrices[ucWindGroup];
                float fOrigZ = pOrigCoord[2];
                float fZ = static_cast<float>(
                    (static_cast<double>(cWindEffect.m_afData[2]) - fOrigZ) * fWeight + fOrigZ
                );
                float fOrigY = pOrigCoord[1];
                float fY = static_cast<float>(
                    (static_cast<double>(cWindEffect.m_afData[1]) - fOrigY) * fWeight + fOrigY
                );
                float fOrigX = pOrigCoord[0];
                float fX = static_cast<float>(
                    static_cast<double>(fWeight) * (static_cast<double>(cWindEffect.m_afData[0]) - fOrigX) + fOrigX
                );
                cInterpWindEffect.m_afData[0] = fX;
                cInterpWindEffect.m_afData[1] = fY;
                cInterpWindEffect.m_afData[2] = fZ;
                rt::memcpy(const_cast<float*>(&pLod->m_pCenterCoords[3 * i]), &cInterpWindEffect, sizeof(float) * 3);
            }
        }
    }
}

// Each leaf access goes through the vector's checked operator[]. The first four leaf loops use a signed index and the
// rest an unsigned one. GetLocalMatrices is called with its 0 and 1 defaults.
void CLeafGeometry::InitLods(
    uint16_t usNumLeafLods,
    const std::vector<CBillboardLeaf*, std::allocator<CBillboardLeaf*>>* pvLeafLods
) {
    if (pvLeafLods && m_pLeafVertexes && m_pWindEngine && m_pLeafTexCoords) {
        m_usNumLods = usNumLeafLods;
        uint32_t uiCount = usNumLeafLods;
        void* pMemory = ourMalloc(uiCount * sizeof(SLodGeometry) + sizeof(size_t));
        SLodGeometry* pLods;
        if (pMemory) {
            // Array cookie: the element count, before the elements.
            size_t* pCookie = static_cast<size_t*>(pMemory);
            *pCookie = uiCount;
            pLods = ::new (static_cast<void*>(pCookie + 1)) SLodGeometry[static_cast<int32_t>(uiCount)];
        }
        else {
            pLods = nullptr;
        }
        m_pLods = pLods;
        for (uint16_t usLod = 0; usLod < usNumLeafLods; ++usLod) {
            SLodGeometry* pLod = &m_pLods[usLod];
            pLod->m_usLeafCount = static_cast<uint16_t>(pvLeafLods[usLod].size());

            pLod->m_pLeafMapCoords = static_cast<const float**>(ourMalloc(pLod->m_usLeafCount * sizeof(float*)));
            for (int32_t j = 0; j < pLod->m_usLeafCount; ++j) {
                uint32_t uiAltTextureIndex = pvLeafLods[usLod][j]->m_ucTextureIndex;
                const CBillboardLeaf* pLeaf = pvLeafLods[usLod][j];
                pLod->m_pLeafMapCoords[j] = m_pLeafVertexes[usLod] + uiAltTextureIndex * (m_usNumRockingGroups * 16u) +
                    pLeaf->m_ucAngleIndex * 16u;
            }

            if (m_bVertexWeighting) {
                pLod->m_pOrigCenterCoords = static_cast<float*>(ourMalloc(pLod->m_usLeafCount * 3u * sizeof(float)));
                for (int32_t i = 0; i < pLod->m_usLeafCount; ++i) {
                    const float* pSource = pvLeafLods[usLod][i]->m_cPosition.m_afData;
                    float* pDest = pLod->m_pOrigCenterCoords + i * 3;
                    pDest[0] = pSource[0];
                    pDest[1] = pSource[1];
                    pDest[2] = pSource[2];
                }

                pLod->m_pWindWeights = static_cast<const float*>(ourMalloc(pLod->m_usLeafCount * sizeof(float)));
                for (int32_t i = 0; i < pLod->m_usLeafCount; ++i) {
                    float fWeight = pvLeafLods[usLod][i]->m_fWindWeight;
                    const_cast<float*>(pLod->m_pWindWeights)[i] = 1.0f - fWeight;
                }

                pLod->m_pWindMatrixIndices = static_cast<const uint8_t*>(ourMalloc(pLod->m_usLeafCount));
                for (int32_t i = 0; i < pLod->m_usLeafCount; ++i) {
                    uint8_t ucGroup = static_cast<uint8_t>(pvLeafLods[usLod][i]->m_nWindGroup);
                    CWindEngine* pWindEngine = m_pWindEngine;
                    int32_t nStartingMatrix = pWindEngine->m_nStartingMatrix;
                    int32_t nMatrixSpan = pWindEngine->m_nMatrixSpan;
                    ucGroup = static_cast<uint8_t>(
                        static_cast<uint8_t>(nStartingMatrix) + ucGroup % static_cast<uint8_t>(nMatrixSpan)
                    );
                    const_cast<uint8_t*>(pLod->m_pWindMatrixIndices)[i] = ucGroup;
                }
            }

            pLod->m_pCenterCoords = static_cast<const float*>(ourMalloc(pLod->m_usLeafCount * 3u * sizeof(float)));
            for (uint32_t i = 0; i < pLod->m_usLeafCount; ++i) {
                const float* pSource = pvLeafLods[usLod][i]->m_cPosition.m_afData;
                float* pDest = const_cast<float*>(pLod->m_pCenterCoords + i * 3);
                pDest[0] = pSource[0];
                pDest[1] = pSource[1];
                pDest[2] = pSource[2];
            }

            pLod->m_pLeafMapIndices = static_cast<const uint8_t*>(ourMalloc(pLod->m_usLeafCount));
            for (uint32_t i = 0; i < pLod->m_usLeafCount; ++i) {
                const_cast<uint8_t*>(pLod->m_pLeafMapIndices)[i] = pvLeafLods[usLod][i]->m_ucTextureIndex;
            }

            pLod->m_pLeafClusterIndices = static_cast<const uint8_t*>(ourMalloc(pLod->m_usLeafCount));
            for (uint32_t i = 0; i < pLod->m_usLeafCount; ++i) {
                uint32_t uiAltTextureIndex = pvLeafLods[usLod][i]->m_ucTextureIndex;
                const CBillboardLeaf* pLeaf = pvLeafLods[usLod][i];
                const_cast<uint8_t*>(pLod->m_pLeafClusterIndices)[i] = static_cast<uint8_t>(
                    uiAltTextureIndex * m_usNumRockingGroups + pLeaf->m_ucAngleIndex
                );
            }

            pLod->m_pLeafMapTexCoords = static_cast<const float**>(ourMalloc(pLod->m_usLeafCount * sizeof(float*)));
            for (uint32_t i = 0; i < pLod->m_usLeafCount; ++i) {
                pLod->m_pLeafMapTexCoords[i] = m_pLeafTexCoords + pvLeafLods[usLod][i]->m_ucTextureIndex * 8u;
            }

            // On PC EndianSwap32 is the identity, so the colors are copied unswapped.
            pLod->m_pColors = static_cast<const uint32_t*>(ourMalloc(pLod->m_usLeafCount * sizeof(uint32_t)));
            for (uint32_t i = 0; i < pLod->m_usLeafCount; ++i) {
                const_cast<uint32_t*>(pLod->m_pColors)[i] = pvLeafLods[usLod][i]->m_lColor;
            }

            if (!m_bManualLighting) {
                pLod->m_pNormals = static_cast<const float*>(ourMalloc(pLod->m_usLeafCount * 3u * sizeof(float)));
                for (uint32_t i = 0; i < pLod->m_usLeafCount; ++i) {
                    const float* pSource = pvLeafLods[usLod][i]->m_cNormal.m_afData;
                    float* pDest = const_cast<float*>(pLod->m_pNormals + i * 3);
                    pDest[0] = pSource[0];
                    pDest[1] = pSource[1];
                    pDest[2] = pSource[2];
                }
            }
            if (!m_bManualLighting) {
                pLod->m_pTangents = static_cast<const float*>(ourMalloc(pLod->m_usLeafCount * 3u * sizeof(float)));
                pLod->m_pBinormals = static_cast<const float*>(ourMalloc(pLod->m_usLeafCount * 3u * sizeof(float)));
                for (uint32_t i = 0; i < pLod->m_usLeafCount; ++i) {
                    const float* pTangent = pvLeafLods[usLod][i]->m_cTangent.m_afData;
                    float* pDest = const_cast<float*>(pLod->m_pTangents + i * 3);
                    pDest[0] = pTangent[0];
                    pDest[1] = pTangent[1];
                    pDest[2] = pTangent[2];
                    const float* pBinormal = pvLeafLods[usLod][i]->m_cBinormal.m_afData;
                    pDest = const_cast<float*>(pLod->m_pBinormals + i * 3);
                    pDest[0] = pBinormal[0];
                    pDest[1] = pBinormal[1];
                    pDest[2] = pBinormal[2];
                }
            }
        }
    }
}

CVec3& CVec3::operator*=(float fScale) {
    m_afData[0] = m_afData[0] * fScale;
    m_afData[1] = m_afData[1] * fScale;
    m_afData[2] = m_afData[2] * fScale;
    return *this;
}
