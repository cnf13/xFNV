#include "SpeedTree/LeafInfo.h"

#include "BSCore/MemoryManager.h"
#include "SpeedTree/CIdvFilename.h"
#include "SpeedTree/EFileToken.h"
#include "SpeedTree/FileAccess.h"
#include "SpeedTree/LibGlobals_Source/IdvGlobals.h"
#include "SpeedTree/LibRandom_Source/IdvRandom.h"
#include "SpeedTree/StructsSupport.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

using std_nvector_lSIdvLeafTexture_cstd_nallocator_lSIdvLeafTexture = std::
    vector<SIdvLeafTexture, std::allocator<SIdvLeafTexture>>;

SIdvLeafInfo::SIdvLeafInfo()
  : m_bDimming(true), m_fDimmingScalar(1.0f), m_eCollisionType(TREE), m_fSpacingTolerance(0.5f),
    m_fBlossomDistance(0.75f), m_fBlossomWeighting(0.8f), m_nBlossomLevel(1), m_nNumRockingGroups(3),
    m_nNumLeafLodLevels(4), m_nNumTextures(0), m_pLeafVertexes(nullptr), m_pLeafTexCoords(nullptr),
    m_pTimeOffsets(nullptr) {}

// The tables are raw ourMalloc buffers, the vertex tables one row per LOD level.
SIdvLeafInfo::~SIdvLeafInfo() {
    ourFree(m_pTimeOffsets);
    m_pTimeOffsets = nullptr;
    if (m_pLeafVertexes) {
        for (int32_t i = 0; i < m_nNumLeafLodLevels; ++i) {
            ourFree(m_pLeafVertexes[i]);
        }
        ourFree(m_pLeafVertexes);
        m_pLeafVertexes = nullptr;
    }
    ourFree(m_pLeafTexCoords);
    m_pLeafTexCoords = nullptr;
}

void SIdvLeafInfo::InitTables(int32_t nNumTextures) {
    m_nNumTextures = nNumTextures;
    uint32_t uiGroups = static_cast<uint32_t>(m_nNumRockingGroups);
    m_pTimeOffsets = static_cast<float*>(
        ourMalloc(uiGroups > SIZE_MAX / sizeof(float) ? SIZE_MAX : sizeof(float) * uiGroups)
    );
    CIdvRandom cDice;
    for (int32_t i = 0; i < m_nNumRockingGroups; ++i) {
        m_pTimeOffsets[i] = cDice.GetUniform(0.0f, 10000.0f);
    }
    uint32_t uiTexCoords = static_cast<uint32_t>(nNumTextures) << 4;
    m_pLeafTexCoords = static_cast<float*>(
        ourMalloc(uiTexCoords > SIZE_MAX / sizeof(float) ? SIZE_MAX : sizeof(float) * uiTexCoords)
    );
    float* pTexCoord = m_pLeafTexCoords;
    for (int32_t j = 0; j < 2 * nNumTextures; ++j) {
        *pTexCoord++ = 0.0f;
        *pTexCoord++ = 1.0f;
        *pTexCoord++ = 1.0f;
        *pTexCoord++ = 1.0f;
        *pTexCoord++ = 1.0f;
        *pTexCoord++ = 0.0f;
        *pTexCoord++ = 0.0f;
        *pTexCoord++ = 0.0f;
    }
    uint32_t uiLevels = static_cast<uint32_t>(m_nNumLeafLodLevels);
    m_pLeafVertexes = static_cast<float**>(
        ourMalloc(uiLevels > SIZE_MAX / sizeof(float*) ? SIZE_MAX : sizeof(float*) * uiLevels)
    );
    int32_t nVertexFloats = 16 * (m_nNumRockingGroups * (2 * nNumTextures));
    for (int32_t k = 0; k < m_nNumLeafLodLevels; ++k) {
        uint32_t uiFloats = static_cast<uint32_t>(nVertexFloats);
        m_pLeafVertexes[k] = static_cast<float*>(
            ourMalloc(uiFloats > SIZE_MAX / sizeof(float) ? SIZE_MAX : sizeof(float) * uiFloats)
        );
        for (int32_t m = 0; m < nVertexFloats; ++m) {
            m_pLeafVertexes[k][m] = 0.0f;
        }
    }
}

// The file name is parsed into a temporary, copied into a second one and assigned, then replaced by its NoPath.
void SIdvLeafInfo::Parse(CTreeFileAccess& cFile) {
    int32_t nToken = cFile.ParseInt();
    do {
        switch (nToken) {
            case File_Leaf_Blossom_Distance: m_fBlossomDistance = cFile.ParseFloat(); break;
            case File_Leaf_Blossom_Level: m_nBlossomLevel = cFile.ParseInt(); break;
            case File_Leaf_Blossom_Weighting: m_fBlossomWeighting = cFile.ParseFloat(); break;
            case File_Leaf_ColorShift:
            case File_Leaf_Lit: cFile.ParseBool(); break;
            case File_Leaf_ColorShiftScalar:
            case File_Leaf_LightScalar: cFile.ParseFloat(); break;
            case File_Leaf_SpacingTolerance: m_fSpacingTolerance = cFile.ParseFloat(); break;
            case File_Leaf_CollisionDetection: m_eCollisionType = static_cast<ECollisionType>(cFile.ParseInt()); break;
            case File_Leaf_Dimming: m_bDimming = cFile.ParseBool(); break;
            case File_Leaf_DimmingScalar: m_fDimmingScalar = cFile.ParseFloat(); break;
            case File_BeginSingleLeafInfo: {
                nToken = cFile.ParseInt();
                int32_t nCount = cFile.ParseInt();
                m_vLeafTextures.clear();
                SIdvLeafTexture sDefault;
                m_vLeafTextures.resize(nCount, sDefault);
                for (int32_t i = 0; i < nCount; ++i) {
                    SIdvLeafTexture sLeaf;
                    nToken = cFile.ParseInt();
                    nToken = cFile.ParseInt();
                    do {
                        switch (nToken) {
                            case File_Leaf_Blossom: sLeaf.m_bBlossom = cFile.ParseBool(); break;
                            case File_Leaf_Color: {
                                float afColor[3];
                                float* pColor = cFile.ParseVector3(afColor);
                                sLeaf.m_cColor.m_afData[0] = pColor[0];
                                sLeaf.m_cColor.m_afData[1] = pColor[1];
                                sLeaf.m_cColor.m_afData[2] = pColor[2];
                                break;
                            }
                            case File_Leaf_ColorVariance: sLeaf.m_fColorVariance = cFile.ParseFloat(); break;
                            case File_Leaf_Filename: {
                                std::string strParsed = cFile.ParseString();
                                std::string strFilename(strParsed);
                                sLeaf.m_strFilename.assign(strFilename, 0, std::string::npos);
                                CIdvFilename strNoPath;
                                CIdvFilename* pNoPath = sLeaf.m_strFilename.NoPath(&strNoPath);
                                sLeaf.m_strFilename.assign(*pNoPath, 0, std::string::npos);
                                break;
                            }
                            case File_Leaf_Origin: {
                                float afOrigin[3];
                                float* pOrigin = cFile.ParseVector3(afOrigin);
                                sLeaf.m_cOrigin.m_afData[0] = pOrigin[0];
                                sLeaf.m_cOrigin.m_afData[1] = pOrigin[1];
                                sLeaf.m_cOrigin.m_afData[2] = pOrigin[2];
                                break;
                            }
                            case File_Leaf_Size: {
                                float afSize[3];
                                float* pSize = cFile.ParseVector3(afSize);
                                sLeaf.m_cSize.m_afData[0] = pSize[0];
                                sLeaf.m_cSize.m_afData[1] = pSize[1];
                                sLeaf.m_cSize.m_afData[2] = pSize[2];
                                break;
                            }
                            case File_Leaf_SizeUsed: {
                                float afSizeUsed[3];
                                float* pSizeUsed = cFile.ParseVector3(afSizeUsed);
                                sLeaf.m_cSizeUsed.m_afData[0] = pSizeUsed[0];
                                sLeaf.m_cSizeUsed.m_afData[1] = pSizeUsed[1];
                                sLeaf.m_cSizeUsed.m_afData[2] = pSizeUsed[2];
                                break;
                            }
                            case File_Leaf_Ratio: cFile.ParseFloat(); break;
                            default: {
                                std::string strDetails("malformed single leaf information");
                                throw IdvFileError(strDetails, false);
                            }
                        }
                        nToken = cFile.ParseInt();
                    }
                    while (nToken != File_EndLeafLevel);
                    m_vLeafTextures[i].Assign(sLeaf);
                }
                nToken = cFile.ParseInt();
                break;
            }
            default: {
                std::string strDetails("malformed general leaf information");
                throw IdvFileError(strDetails, false);
            }
        }
        nToken = cFile.ParseInt();
    }
    while (nToken != File_EndGeneralLeafInfo);
}
