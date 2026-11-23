#include "SpeedTree/IndexedGeometry.h"

#include "BSCore/MemoryManager.h"
#include "SDK/MSVC/crtdefs.h"
#include "SDK/MSVC/string.h"
#include "SpeedTree/LibVector_Source/IdvVector.h"
#include "SpeedTree/SpeedTreeRT.h"
#include "SpeedTree/WindEngine.h"

#include <memory>
#include <vector>

// The wind flags are a raw ourMalloc buffer; the strips are freed by DeleteIndexData.
CIndexedGeometry::~CIndexedGeometry() {
    ourFree(m_pVertexWindComputed);
    m_pVertexWindComputed = nullptr;
    DeleteIndexData();
}

void CIndexedGeometry::AddVertexTexCoord0(const float* pTexCoord, int16_t sIndex) {
    float fTexCoordT;
    if (m_nVertexCounter == m_vTexCoords0.size() / 2) {
        m_vTexCoords0.resize(m_vTexCoords0.size() + 2, 0.0f);
        if (sIndex > -1 && m_bRetainTexCoords) {
            m_vTexIndices0.resize(m_vTexIndices0.size() + 1, 0);
            m_vOrigTexCoords0.resize(m_vOrigTexCoords0.size() + 2, 0.0f);
        }
    }
    if (sIndex > -1 && m_bRetainTexCoords) {
        m_vTexIndices0[m_nVertexCounter] = static_cast<uint8_t>(sIndex);
        m_vOrigTexCoords0[2 * m_nVertexCounter] = pTexCoord[0];
        m_vOrigTexCoords0[2 * m_nVertexCounter + 1] = pTexCoord[1];
    }
    m_vTexCoords0[2 * m_nVertexCounter] = pTexCoord[0];
    if (CSpeedTreeRT::GetTextureFlip()) {
        fTexCoordT = -pTexCoord[1];
    }
    else {
        fTexCoordT = pTexCoord[1];
    }
    m_vTexCoords0[2 * m_nVertexCounter + 1] = fTexCoordT;
}

void CIndexedGeometry::ChangeTexCoord(uint8_t chChangedMapIndex, const float* pNewTexCoords) {
    float pTexCoord;
    uint32_t nVertexCounter = this->m_nVertexCounter;
    if (chChangedMapIndex == m_vTexIndices0[nVertexCounter]) {
        pTexCoord = m_vOrigTexCoords0[2 * this->m_nVertexCounter] * (*pNewTexCoords - pNewTexCoords[2]) +
            pNewTexCoords[2];
        float afTexCoords = m_vOrigTexCoords0[2 * this->m_nVertexCounter + 1] * (pNewTexCoords[1] - pNewTexCoords[5]) +
            pNewTexCoords[5];
        if (CSpeedTreeRT::GetTextureFlip()) {
            afTexCoords = -afTexCoords;
        }
        AddVertexTexCoord0(&pTexCoord, -1);
    }
}

void CIndexedGeometry::AddVertexNormal(const float* pNormal) {
    m_vNormals.push_back(*pNormal);
    m_vNormals.push_back(*(pNormal + 1));
    m_vNormals.push_back(*(pNormal + 2));
}

void CIndexedGeometry::AddVertexCoord(const float* pCoord) {
    int32_t nX;
    nX = *reinterpret_cast<const int32_t*>(pCoord);
    float _Val = pCoord[1];
    float v5 = pCoord[2];
    m_vCoords.push_back(*reinterpret_cast<float*>(&nX));
    m_vCoords.push_back(_Val);
    m_vCoords.push_back(v5);
    if (m_bVertexWeighting && m_eWindMethod == CSpeedTreeRT::WIND_CPU) {
        m_vOrigCoords.push_back(*reinterpret_cast<float*>(&nX));
        m_vOrigCoords.push_back(_Val);
        m_vOrigCoords.push_back(v5);
    }
}

void CIndexedGeometry::AddVertexTangent(const float* pTangent) {
    int32_t nX;
    nX = *reinterpret_cast<const int32_t*>(pTangent);
    float _Val = pTangent[1];
    float v5 = pTangent[2];
    m_vTangents.push_back(*reinterpret_cast<float*>(&nX));
    m_vTangents.push_back(_Val);
    m_vTangents.push_back(v5);
}

void CIndexedGeometry::AddVertexBinormal(const float* pBinormal) {
    int32_t nX;
    nX = *reinterpret_cast<const int32_t*>(pBinormal);
    float _Val = pBinormal[1];
    float v5 = pBinormal[2];
    m_vBinormals.push_back(*reinterpret_cast<float*>(&nX));
    m_vBinormals.push_back(_Val);
    m_vBinormals.push_back(v5);
}

const float* CIndexedGeometry::GetVertexTexCoord0(uint32_t nVertexIndex) const {
    return &m_vTexCoords0[2 * nVertexIndex];
}

void CIndexedGeometry::DeleteLodStrip(uint16_t usLodLevel) {
    for (int32_t i = 0; i < GetNumStrips(usLodLevel); ++i) {
        delete[] m_vStrips[usLodLevel][i];
        m_vStrips[usLodLevel][i] = nullptr;
    }
}

void CIndexedGeometry::AddStrip(uint16_t nLodLevel, uint16_t* pStrip, uint16_t nStripLength) {
    m_vStripLengths[nLodLevel].resize(m_nStripCounter + 1);
    m_vStripLengths[nLodLevel][m_nStripCounter] = nStripLength;
    m_vStrips[nLodLevel].resize(m_nStripCounter + 1);
    m_vStrips[nLodLevel][m_nStripCounter] = pStrip;
    m_vTriangleCounts[nLodLevel] += nStripLength - 2;
}

void CIndexedGeometry::AddVertexTexCoord1(const float* pTexCoord) {
    m_vTexCoords1.push_back(pTexCoord[0]);
    if (CSpeedTreeRT::GetTextureFlip()) {
        float fFlipped = -pTexCoord[1];
        m_vTexCoords1.push_back(fFlipped);
    }
    else {
        m_vTexCoords1.push_back(pTexCoord[1]);
    }
}

// The engine's local matrices are read inline into two locals that start at 0 and 1; the weight and the index go to
// the vectors by reference from their argument slots.
void CIndexedGeometry::AddVertexWind(float fWindWeight, uint8_t ucWindMatrixIndex) {
    fWindWeight = 1.0f - fWindWeight;
    int32_t nStartingMatrix = m_pWindEngine->m_nStartingMatrix;
    int32_t nMatrixSpan = m_pWindEngine->m_nMatrixSpan;
    ucWindMatrixIndex = static_cast<uint8_t>(ucWindMatrixIndex % static_cast<uint32_t>(nMatrixSpan) + nStartingMatrix);
    m_vWindWeights.push_back(fWindWeight);
    m_vWindMatrixIndices.push_back(ucWindMatrixIndex);
}

void CIndexedGeometry::ResetStripCounter(uint16_t nLodLevel) {
    m_nVertexCounterLodLevel = nLodLevel;
    m_nStripCounter = 0;
    m_vTriangleCounts[nLodLevel] = 0;
}

uint16_t** CIndexedGeometry::GetStripsPointer(uint16_t nLodLevel) const {
    // The const getter hands out the strips as non-const pointers.
    if (m_vStrips[nLodLevel].size() != 0) {
        return const_cast<uint16_t**>(&m_vStrips[nLodLevel][0]);
    }
    return nullptr;
}

const float* CIndexedGeometry::GetVertexCoord(uint32_t nVertexIndex) const {
    return &m_vCoords[0] + nVertexIndex * 3;
}

const float* CIndexedGeometry::GetOrigVertexCoord(uint32_t nVertexIndex) const {
    return &m_vOrigCoords[0] + nVertexIndex * 3;
}

// The colour packed by ColorFloatsToLong into a local, pushed by reference.
void CIndexedGeometry::AddVertexColor(const float* pColor) {
    uint32_t uiColor = ColorFloatsToLong(pColor);
    m_vColors.push_back(uiColor);
}

uint32_t CIndexedGeometry::ColorFloatsToLong(const float* pColor) const {
    return static_cast<uint32_t>(
        (((static_cast<int32_t>(pColor[1] * 255.0) << 8) + static_cast<int32_t>((*pColor) * 255.0)) +
         (static_cast<int32_t>(pColor[2] * 255.0) << 16)) -
        16777216
    );
}

bool CIndexedGeometry::ComputeWindEffect(uint16_t nLodLevel) {
    CVec3 cInterpWindEffect;
    bool bGeometryUpdated = false;
    if (!m_bValid) {
        CVec3 cFullWindEffect;
        uint16_t usVertexCount = static_cast<uint16_t>(m_vCoords.size() / 3);
        if (!m_pVertexWindComputed) {
            m_pVertexWindComputed = static_cast<bool*>(ourMalloc(usVertexCount));
        }
        for (int32_t i = 0; i < usVertexCount; ++i) {
            m_pVertexWindComputed[i] = false;
        }
        cFullWindEffect.m_afData[2] = 0.0f;
        cFullWindEffect.m_afData[1] = 0.0f;
        cFullWindEffect.m_afData[0] = 0.0f;
        cInterpWindEffect.m_afData[2] = 0.0f;
        cInterpWindEffect.m_afData[1] = 0.0f;
        cInterpWindEffect.m_afData[0] = 0.0f;
        for (int32_t nStrip = 0; nStrip < GetNumStrips(static_cast<int16_t>(nLodLevel)); ++nStrip) {
            for (int32_t nVertex = 0; nVertex < m_vStripLengths[nLodLevel][nStrip]; ++nVertex) {
                uint16_t nIndex = m_vStrips[nLodLevel][nStrip][nVertex];
                if (!m_pVertexWindComputed[nIndex]) {
                    float fWeight = m_vWindWeights[nIndex];
                    float* pOrigCoord = &m_vOrigCoords[0] + 3 * nIndex;
                    float fOrigX = pOrigCoord[0];
                    float fOrigY = pOrigCoord[1];
                    float fOrigZ = pOrigCoord[2];
                    cFullWindEffect.m_afData[0] = fOrigX;
                    cFullWindEffect.m_afData[1] = fOrigY;
                    cFullWindEffect.m_afData[2] = fOrigZ;
                    uint16_t usWindGroup = m_vWindMatrixIndices[nIndex];
                    cFullWindEffect = cFullWindEffect * CWindEngine::m_cWindMatrices.m_pMatrices[usWindGroup];
                    float fInterpZ = (cFullWindEffect.m_afData[2] - pOrigCoord[2]) * fWeight + pOrigCoord[2];
                    float fInterpY = (cFullWindEffect.m_afData[1] - pOrigCoord[1]) * fWeight + pOrigCoord[1];
                    float fInterpX = fWeight * (cFullWindEffect.m_afData[0] - pOrigCoord[0]) + pOrigCoord[0];
                    cInterpWindEffect.m_afData[0] = fInterpX;
                    cInterpWindEffect.m_afData[1] = fInterpY;
                    cInterpWindEffect.m_afData[2] = fInterpZ;
                    rt::memcpy(&m_vCoords[0] + 3 * nIndex, &cInterpWindEffect, sizeof(CVec3));
                    m_pVertexWindComputed[nIndex] = true;
                }
            }
        }
        m_bValid = true;
        bGeometryUpdated = true;
    }
    return bGeometryUpdated;
}

// Rebuilds every LOD level as one strip: the strips joined through degenerate triangles into a block from ourMalloc,
// whose length, pointer and triangle count replace the level's.
void CIndexedGeometry::CombineStrips(bool bToggleFaceOrdering) {
    std::vector<
        std::vector<uint16_t, std::allocator<uint16_t>>,
        std::allocator<std::vector<uint16_t, std::allocator<uint16_t>>>>
        vLengths;
    std::vector<
        std::vector<uint16_t*, std::allocator<uint16_t*>>,
        std::allocator<std::vector<uint16_t*, std::allocator<uint16_t*>>>>
        vStrips;
    std::vector<int32_t, std::allocator<int32_t>> vTriangleCounts;
    if (static_cast<uint16_t>(m_vCoords.size() / 3u)) {
        vLengths.~vector<
            std::vector<uint16_t, std::allocator<uint16_t>>,
            std::allocator<std::vector<uint16_t, std::allocator<uint16_t>>>>();
        ::new (&vLengths) std::vector<
            std::vector<uint16_t, std::allocator<uint16_t>>,
            std::allocator<std::vector<uint16_t, std::allocator<uint16_t>>>>(m_nNumLodLevels);
        vStrips.~vector<
            std::vector<uint16_t*, std::allocator<uint16_t*>>,
            std::allocator<std::vector<uint16_t*, std::allocator<uint16_t*>>>>();
        ::new (&vStrips) std::vector<
            std::vector<uint16_t*, std::allocator<uint16_t*>>,
            std::allocator<std::vector<uint16_t*, std::allocator<uint16_t*>>>>(m_nNumLodLevels);
        vTriangleCounts.~vector<int32_t, std::allocator<int32_t>>();
        ::new (&vTriangleCounts) std::vector<int32_t, std::allocator<int32_t>>(m_nNumLodLevels);
        for (uint16_t usLod = 0; usLod < static_cast<int32_t>(m_nNumLodLevels); ++usLod) {
            int32_t nCompositeLength = 0;
            for (uint16_t usStrip = 0;; ++usStrip) {
                uint16_t usNumStrips = GetNumStrips(static_cast<int16_t>(usLod));
                if (usStrip >= static_cast<int32_t>(usNumStrips)) {
                    break;
                }
                nCompositeLength += m_vStripLengths[usLod][usStrip];
            }
            if (nCompositeLength <= 0) {
                vLengths[usLod].clear();
                vStrips[usLod].clear();
                vTriangleCounts[usLod] = 0;
            }
            else {
                if (bToggleFaceOrdering) {
                    for (uint16_t usStrip = 0;; ++usStrip) {
                        uint16_t usNumStrips = GetNumStrips(static_cast<int16_t>(usLod));
                        if (usStrip >= usNumStrips - 1) {
                            break;
                        }
                        uint16_t usStripLength = m_vStripLengths[usLod][usStrip];
                        if ((usStripLength & 1) != 0) {
                            nCompositeLength += 3;
                        }
                        else {
                            nCompositeLength += 2;
                        }
                    }
                }
                else {
                    uint16_t usNumStrips = GetNumStrips(static_cast<int16_t>(usLod));
                    int32_t nNumStrips = usNumStrips;
                    if (nNumStrips > 0) {
                        --nNumStrips;
                    }
                    nCompositeLength += 2 * nNumStrips;
                }
                uint16_t* pCompositeStrip = new uint16_t[static_cast<uint32_t>(nCompositeLength)];
                uint16_t* pStripPointer = pCompositeStrip;
                for (uint16_t usStrip = 0;; ++usStrip) {
                    uint16_t usNumStrips = GetNumStrips(static_cast<int16_t>(usLod));
                    if (usStrip >= static_cast<int32_t>(usNumStrips)) {
                        break;
                    }
                    uint16_t usStripLength = m_vStripLengths[usLod][usStrip];
                    rt::memcpy(pStripPointer, m_vStrips[usLod][usStrip], sizeof(uint16_t) * usStripLength);
                    pStripPointer += usStripLength;
                    uint16_t usNumStripsNow = GetNumStrips(static_cast<int16_t>(usLod));
                    if (usStrip < usNumStripsNow - 1) {
                        if (bToggleFaceOrdering) {
                            if ((usStripLength & 1) != 0) {
                                *pStripPointer++ = m_vStrips[usLod][usStrip][usStripLength - 1];
                                *pStripPointer++ = m_vStrips[usLod][usStrip][usStripLength - 1];
                                uint16_t usNext = *m_vStrips[usLod][usStrip + 1];
                                *pStripPointer++ = usNext;
                            }
                            else {
                                *pStripPointer++ = m_vStrips[usLod][usStrip][usStripLength - 1];
                                uint16_t usNext = *m_vStrips[usLod][usStrip + 1];
                                *pStripPointer++ = usNext;
                            }
                        }
                        else {
                            *pStripPointer++ = m_vStrips[usLod][usStrip][usStripLength - 1];
                            uint16_t usNext = *m_vStrips[usLod][usStrip + 1];
                            *pStripPointer++ = usNext;
                        }
                    }
                }
                uint16_t usCompositeLength = nCompositeLength;
                vLengths[usLod].push_back(usCompositeLength);
                vStrips[usLod].push_back(pCompositeStrip);
                vTriangleCounts[usLod] = nCompositeLength - 2;
            }
        }
        DeleteIndexData();
        m_vStripLengths = vLengths;
        m_vStrips = vStrips;
        m_vTriangleCounts = vTriangleCounts;
    }
}

void CIndexedGeometry::ComputeExtents(CRegion& cExtents) const {
    uint16_t usVertexCount = m_vCoords.size() / 3u;
    for (int32_t i = 0; i < usVertexCount; ++i) {
        const float* pCoord = GetVertexCoord(i);
        CVec3 cCoord;
        cCoord.m_afData[0] = pCoord[0];
        cCoord.m_afData[1] = pCoord[1];
        cCoord.m_afData[2] = pCoord[2];
        cExtents = cExtents ^ cCoord;
    }
}

// The transformed coordinates are copied back over the vertex's through the const pointer GetVertexCoord returns.
void CIndexedGeometry::Transform(const CTransform& cTransform) {
    CVec3 cCoord;
    uint16_t usVertexCount = m_vCoords.size() / 3u;
    for (int32_t i = 0; i < usVertexCount; ++i) {
        const float* pCoord = GetVertexCoord(static_cast<uint32_t>(i));
        float v5 = pCoord[1];
        float v6 = pCoord[2];
        cCoord.m_afData[0] = *pCoord;
        cCoord.m_afData[1] = v5;
        cCoord.m_afData[2] = v6;
        cCoord = cCoord * cTransform;
        rt::memcpy(const_cast<float*>(pCoord), cCoord.m_afData, 3 * sizeof(float));
        if (m_bVertexWeighting && m_vOrigCoords.size()) {
            const float* pOrigCoord = GetOrigVertexCoord(static_cast<uint32_t>(i));
            float v3 = pOrigCoord[1];
            float v4 = pOrigCoord[2];
            cCoord.m_afData[0] = *pOrigCoord;
            cCoord.m_afData[1] = v3;
            cCoord.m_afData[2] = v4;
            cCoord = cCoord * cTransform;
            rt::memcpy(const_cast<float*>(pOrigCoord), cCoord.m_afData, 3 * sizeof(float));
        }
    }
}

void CIndexedGeometry::SetNumLodLevels(uint16_t nLevels) {
    m_vStripLengths.clear();
    m_vStrips.clear();
    m_vTriangleCounts.clear();
    m_nNumLodLevels = nLevels;
    std::vector<uint16_t, std::allocator<uint16_t>> vEmptyLengths;
    vEmptyLengths.~vector<uint16_t, std::allocator<uint16_t>>();
    ::new (&vEmptyLengths) std::vector<uint16_t, std::allocator<uint16_t>>;
    m_vStripLengths.resize(nLevels, vEmptyLengths);
    std::vector<uint16_t*, std::allocator<uint16_t*>> vEmptyStrips;
    vEmptyStrips.~vector<uint16_t*, std::allocator<uint16_t*>>();
    ::new (&vEmptyStrips) std::vector<uint16_t*, std::allocator<uint16_t*>>;
    m_vStrips.resize(nLevels, vEmptyStrips);
    m_vTriangleCounts.resize(nLevels, 0);
    for (int32_t i = 0; i < nLevels; ++i) {
        m_vTriangleCounts[i] = 0;
    }
}

void CIndexedGeometry::DeleteIndexData() {
    for (uint32_t i = 0; i < m_vStrips.size(); ++i) {
        for (uint32_t j = 0;; ++j) {
            if (j >= m_vStrips[i].size()) {
                break;
            }
            delete[] m_vStrips[i][j];
            m_vStrips[i][j] = nullptr;
        }
        m_vStrips[i].clear();
    }
    m_vStrips.clear();
}
