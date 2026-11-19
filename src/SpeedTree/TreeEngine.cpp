#include "SpeedTree/TreeEngine.h"

#include "BSCore/MemoryManager.h"
#include "SDK/MSVC/crtdefs.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/iosfwd.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"
#include "SDK/MSVC/time.h"
#include "SpeedTree/BillboardLeaf.h"
#include "SpeedTree/Branch.h"
#include "SpeedTree/BranchInfo.h"
#include "SpeedTree/CIdvFilename.h"
#include "SpeedTree/Camera.h"
#include "SpeedTree/EFileToken.h"
#include "SpeedTree/FileAccess.h"
#include "SpeedTree/IndexedGeometry.h"
#include "SpeedTree/LeafInfo.h"
#include "SpeedTree/LeafLod.h"
#include "SpeedTree/LibGlobals_Source/IdvGlobals.h"
#include "SpeedTree/LibRandom_Source/IdvRandom.h"
#include "SpeedTree/SpeedTreeRT.h"
#include "SpeedTree/StructsSupport.h"
#include "SpeedTree/WindInfo.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using SIdvLeafTextureVector = std::vector<SIdvLeafTexture, std::allocator<SIdvLeafTexture>>;

SIdvLeafTexture* Copy(SIdvLeafTexture* a1, SIdvLeafTexture* a2, SIdvLeafTexture* a3) {
    SIdvLeafTexture* v4 = a3;
    for (SIdvLeafTexture* arOther = a1; arOther != a2; ++arOther) {
        v4++->Assign(*arOther);
    }
    return &a3[a2 - a1];
}

SIdvLeafTexture* CopyOpt(SIdvLeafTexture* a1, SIdvLeafTexture* a2, SIdvLeafTexture* a3) {
    SIdvLeafTexture* v4 = a3;
    for (SIdvLeafTexture* arOther = a1; arOther != a2; ++arOther) {
        v4++->Assign(*arOther);
    }
    return &a3[a2 - a1];
}

SIdvTreeInfo::SIdvTreeInfo()
  : m_fFar(1100.0f), m_fNear(100.0f), m_nSeed(1200), m_fSize(50.0f), m_fSizeVariance(0.0f), m_nFlareSeed(444) {}

CTreeEngine::CTreeEngine(CIndexedGeometry* pBranchGeometry)
  : CIdvCamera(), m_fBillboardSize(1.0f), m_fLod(1.0f), m_fOverrideTreeSize(-1.0f), m_fOverrideTreeVariance(0.0f),
    m_bTransientDataIntact(true), m_pBranch(nullptr), m_pBranchGeometry(pBranchGeometry), m_nNumBranchLodLevels(6),
    m_vLeafLods(nullptr), m_bParsedLeafLods(false), m_fMinBranchVolumePercent(0.5f), m_fMaxBranchVolumePercent(1.0f),
    m_fLeafReductionPercentage(0.3f), m_fBranchReductionFuzziness(0.0f), m_fLargeBranchPercent(0.05f),
    m_nBranchesWeightLevel(1) {
    SetWind(0.25f);
}

// The members and the base are destroyed by their own destructors; the branch info pointers are not deleted here.
CTreeEngine::~CTreeEngine() {}

// It throws IdvFileError with a C++ throw, as SIdvBranchInfo::Parse (BranchInfo.cpp, which says where it is caught).
bool CTreeEngine::Parse(CTreeFileAccess& cFile) {
    CIdvFilename v5;
    std::basic_string<char, std::char_traits<char>, std::allocator<char>> v14;
    int32_t nToken = cFile.ParseInt();
    if (nToken != 1000) {
        using apMemory_t = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;
        apMemory_t apMemory("missing begin_file token");
        throw IdvFileError(apMemory, false);
    }
    std::basic_string<char, std::char_traits<char>, std::allocator<char>> strHeader = cFile.ParseString();
    std::basic_string<char, std::char_traits<char>, std::allocator<char>>* _Right = ::new (static_cast<void*>(&v14))
        std::basic_string<char, std::char_traits<char>, std::allocator<char>>(g_pszHeader);
    bool v15 = strHeader.compare(*_Right) != 0;
    if (v15) {
        using v12_t = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;
        v12_t v12("not a valid SpeedTree SPT file");
        throw IdvFileError(v12, false);
    }
    nToken = cFile.ParseInt();
    do {
        switch (nToken) {
            case 1002: {
                ParseTreeInfo(cFile);
                break;
            }
            case 1004: {
                m_sLeafInfo.Parse(cFile);
                break;
            }
            case 1011: {
                m_sWindInfo.Parse(cFile);
                break;
            }
            default: {
                using v10_t = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;
                v10_t v10("malformed SpeedTree SPT file");
                throw IdvFileError(v10, false);
            }
        }
        nToken = cFile.ParseInt();
    }
    while (nToken != 1001);
    // EXE-INLINED none: CTreeFileAccess::EndOfFile (an unsigned compare of the position with the buffer's size).
    if (!(static_cast<uint32_t>(cFile.m_nIndex) >= cFile.m_vMemory.size()) &&
        cFile.PeekToken() == File_BeginLeafCluster) {
        cFile.ParseInt();
        ParseLeafCluster(cFile);
    }
    CIdvFilename* v3 = &v5.operator=(m_sTreeInfo.m_strBranchTextureFilename);
    m_sTreeInfo.m_strBranchTextureFilename.assign(*v3);
    // SetLeafTextures(m_sLeafInfo.m_vLeafTextures): the leaf textures' operator=.
    m_sLeafInfo.m_vLeafTextures = std::as_const(m_sLeafInfo.m_vLeafTextures);
    if (m_fOverrideTreeSize > -1.0) {
        SetSize(m_fOverrideTreeSize, m_fOverrideTreeVariance);
    }
    return true;
}

// It throws IdvFileError with a C++ throw, as SIdvBranchInfo::Parse (BranchInfo.cpp, which says where it is caught).
void CTreeEngine::ParseTextureControls(CTreeFileAccess& cFile) {
    for (uint32_t i = 0; i < m_vBranchInfo.size(); ++i) {
        if (cFile.ParseInt() != File_TextureOffset) {
            using apMemory_t = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;
            apMemory_t apMemory("malformed texture controls");
            throw IdvFileError(apMemory, false);
        }
        SIdvBranchInfo** v4 = &m_vBranchInfo[i];
        (*v4)->m_bRandomTCoordOffset = cFile.ParseBool();
        if (cFile.ParseInt() != File_TextureTwist) {
            using v7_t = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;
            v7_t v7("malformed texture controls");
            throw IdvFileError(v7, false);
        }
        SIdvBranchInfo** v3 = &m_vBranchInfo[i];
        (*v3)->m_fTwist = cFile.ParseFloat();
    }
    if (cFile.ParseInt() != File_EndTextureControls) {
        using v5_t = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;
        v5_t v5("malformed texture controls");
        throw IdvFileError(v5, false);
    }
}

// It throws IdvFileError with a C++ throw, as SIdvBranchInfo::Parse (BranchInfo.cpp, which says where it is caught).
void CTreeEngine::ParseFlareInfo(CTreeFileAccess& cFile) {
    for (uint32_t i = 0; i < m_vBranchInfo.size(); ++i) {
        if (cFile.ParseInt() != File_FlareSegmentPackingExponent) {
            using apMemory_t = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;
            apMemory_t apMemory("malformed flare info");
            throw IdvFileError(apMemory, false);
        }
        SIdvBranchInfo** v13 = &m_vBranchInfo[i];
        (*v13)->m_fSegmentPackingExponent = cFile.ParseFloat();
        if (cFile.ParseInt() != File_FlareNumFlares) {
            using v34_t = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;
            v34_t v34("malformed flare info");
            throw IdvFileError(v34, false);
        }
        SIdvBranchInfo** v12 = &m_vBranchInfo[i];
        (*v12)->m_nNumFlares = cFile.ParseInt();
        if (cFile.ParseInt() != File_FlareBalance) {
            using v32_t = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;
            v32_t v32("malformed flare info");
            throw IdvFileError(v32, false);
        }
        SIdvBranchInfo** v11 = &m_vBranchInfo[i];
        (*v11)->m_fFlareBalance = cFile.ParseFloat();
        if (cFile.ParseInt() != File_FlareRadialInfluence) {
            using v30_t = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;
            v30_t v30("malformed flare info");
            throw IdvFileError(v30, false);
        }
        SIdvBranchInfo** v10 = &m_vBranchInfo[i];
        (*v10)->m_fRadialInfluence = cFile.ParseFloat();
        if (cFile.ParseInt() != File_FlareRadialInfluenceVariance) {
            using v28_t = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;
            v28_t v28("malformed flare info");
            throw IdvFileError(v28, false);
        }
        SIdvBranchInfo** v9 = &m_vBranchInfo[i];
        (*v9)->m_fRadialInfluenceVariance = cFile.ParseFloat();
        if (cFile.ParseInt() != File_FlareRadialExponent) {
            using v26_t = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;
            v26_t v26("malformed flare info");
            throw IdvFileError(v26, false);
        }
        SIdvBranchInfo** v8 = &m_vBranchInfo[i];
        (*v8)->m_fRadialExponent = cFile.ParseFloat();
        if (cFile.ParseInt() != File_FlareRadialDistance) {
            using v24_t = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;
            v24_t v24("malformed flare info");
            throw IdvFileError(v24, false);
        }
        SIdvBranchInfo** v7 = &m_vBranchInfo[i];
        (*v7)->m_fRadialDistance = cFile.ParseFloat();
        if (cFile.ParseInt() != File_FlareRadialDistanceVariance) {
            using v22_t = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;
            v22_t v22("malformed flare info");
            throw IdvFileError(v22, false);
        }
        float v5 = cFile.ParseFloat();
        m_vBranchInfo[i]->m_fRadialVariance = v5;
        if (cFile.ParseInt() != File_FlareLengthDistance) {
            using v20_t = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;
            v20_t v20("malformed flare info");
            throw IdvFileError(v20, false);
        }
        float v4 = cFile.ParseFloat();
        m_vBranchInfo[i]->m_fLengthDistance = v4;
        if (cFile.ParseInt() != File_FlareLengthDistanceVariance) {
            using v18_t = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;
            v18_t v18("malformed flare info");
            throw IdvFileError(v18, false);
        }
        float v3 = cFile.ParseFloat();
        m_vBranchInfo[i]->m_fLengthVariance = v3;
        if (cFile.ParseInt() != File_LengthExponent) {
            using v16_t = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;
            v16_t v16("malformed flare info");
            throw IdvFileError(v16, false);
        }
        float v2 = cFile.ParseFloat();
        m_vBranchInfo[i]->m_fLengthExponent = v2;
    }
    if (cFile.ParseInt() != File_EndFlareInfo) {
        using v14_t = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;
        v14_t v14("malformed flare info");
        throw IdvFileError(v14, false);
    }
}

void CTreeEngine::GetSize(float& fSize, float& fVariance) {
    fSize = m_sTreeInfo.m_fSize;
    fVariance = m_sTreeInfo.m_fSizeVariance;
}

void CTreeEngine::SetWind(float fStrength) {
    m_sWindInfo.m_fStrength = fStrength;
    float v3 = (0x1p+3f * fStrength) * m_sWindInfo.m_cLeafFactors.m_afData[1];
    float v4 = (180.0f * fStrength) * m_sWindInfo.m_cLeafFactors.m_afData[0];
    m_sWindInfo.m_cLeafOscillation.m_afData[0] = (-v4);
    CVec3* p_m_cLeafOscillation = &m_sWindInfo.m_cLeafOscillation;
    p_m_cLeafOscillation->m_afData[1] = v4;
    p_m_cLeafOscillation->m_afData[2] = v3;
}

float CTreeEngine::ComputeLod() {
    float v3 = (&CIdvCamera::m_cCameraPos)->Distance(m_cPosition);
    m_fLod = (float)(1.0 - ((v3 - m_sTreeInfo.m_fNear) / (m_sTreeInfo.m_fFar - m_sTreeInfo.m_fNear)));
    if (!(m_fLod > 1.0)) {
        if (m_fLod < 0.0) {
            m_fLod = (float)0.0;
        }
    }
    else {
        m_fLod = (float)1.0;
    }
    return m_fLod;
}

// m_vLeafLods is allocated with new[], each element built by the leaf pointers' vector constructor.
void CTreeEngine::Compute(float fLeafSizeIncreaseFactor) {
    CIdvBranch* v9;
    CRotTransform cZAxis;
    CVec3 v17;
    bool v15 = !m_bParsedLeafLods;
    CIdvBranch::m_bComputeLeaves = v15;
    void* apMemory = ourMalloc(sizeof(CIdvBranch));
    if (apMemory) {
        v9 = ::new (apMemory) CIdvBranch(nullptr);
    }
    else {
        v9 = nullptr;
    }
    m_pBranch = v9;
    if (!m_bParsedLeafLods) {
        m_vLeafLods =
            new std::vector<CBillboardLeaf*, std::allocator<CBillboardLeaf*>>[m_sLeafInfo.m_nNumLeafLodLevels];
    }
    int32_t m_nBranchesWeightLevel = this->m_nBranchesWeightLevel;
    CIdvBranch::m_nWeightLevel = m_nBranchesWeightLevel;
    CIdvBranch::m_vBranchInfo.clear();
    CIdvBranch::m_pLeafInfo = &m_sLeafInfo;
    for (uint32_t i = 0; i < m_vBranchInfo.size(); ++i) {
        // push_back takes a copy of the element in a local.
        SIdvBranchInfo* pInfo = m_vBranchInfo[i];
        CIdvBranch::m_vBranchInfo.push_back(pInfo);
    }
    m_cRandom.Reseed(m_sTreeInfo.m_nSeed);
    float fMax = m_sTreeInfo.m_fSize + m_sTreeInfo.m_fSizeVariance;
    float fMin = m_sTreeInfo.m_fSize - m_sTreeInfo.m_fSizeVariance;
    float fTreeSize = m_cRandom.GetUniform(fMin, fMax);
    rt::srand(static_cast<uint32_t>(m_sTreeInfo.m_nFlareSeed));
    if (m_sTreeInfo.m_fSize > 0.0) {
        for (uint32_t j = 0; j < m_sLeafInfo.m_vLeafTextures.size(); ++j) {
            SIdvLeafTexture* v23 = &m_sLeafInfo.m_vLeafTextures[j];
            float v5 = m_sTreeInfo.m_fSize * v23->m_cSize.m_afData[0u];
            v23->m_cSizeUsed.m_afData[0u] = v5;
            float v4 = m_sTreeInfo.m_fSize * v23->m_cSize.m_afData[1];
            v23->m_cSizeUsed.m_afData[1] = v4;
        }
    }
    m_cRandom.Reseed(m_sTreeInfo.m_nSeed);
    m_pBranchGeometry->SetNumLodLevels(m_nNumBranchLodLevels);
    m_pBranchGeometry->ResetStripCounter(0);
    CIdvBranch::m_nRoundRobinWindLevel = m_sTreeInfo.m_nSeed;
    v17.m_afData[0] = 0.0f;
    v17.m_afData[1] = 0.0f;
    v17.m_afData[2] = 1.0f;
    cZAxis.m_afData[0][0] = 1.0f;
    cZAxis.m_afData[0][1] = 0.0f;
    cZAxis.m_afData[0][2] = 0.0f;
    cZAxis.m_afData[1][0] = 0.0f;
    cZAxis.m_afData[1][1] = 1.0f;
    cZAxis.m_afData[1][2] = 0.0f;
    cZAxis.m_afData[2][0] = 0.0f;
    cZAxis.m_afData[2][1] = 0.0f;
    cZAxis.m_afData[2][2] = 1.0f;
    m_pBranch->Compute(
        static_cast<uint32_t>(m_sTreeInfo.m_nSeed), fTreeSize, 0, m_cPosition, 0.0f, 0.0f, cZAxis, v17,
        m_pBranchGeometry, m_vBillboardLeaves, 1.0f, m_sTreeInfo.m_nSeed, -1.0f
    );
    BuildBranchLods();
    if (!m_bParsedLeafLods) {
        BuildLeafLods(fLeafSizeIncreaseFactor);
    }
    // Seeds with the low 32 bits of the 64-bit time.
    rt::srand(static_cast<uint32_t>(std::time(nullptr)));
}

void CTreeEngine::ParseFlareSeed(CTreeFileAccess& cFile) {
    m_sTreeInfo.m_nFlareSeed = (&cFile)->ParseToken();
}

void CTreeEngine::SetSeed(uint32_t nSeed) {
    if (nSeed == 0) {
        m_cRandom.Reseed(-1);
        m_sTreeInfo.m_nSeed = (int32_t)m_cRandom.GetUniform(2.0f, 1000000.0f);
    }
    else if (nSeed != 1) {
        m_sTreeInfo.m_nSeed = nSeed;
    }
}

void CTreeEngine::FreeTransientData() {
    if (m_bTransientDataIntact) {
        CIdvBranch* Block = m_pBranch;
        if (Block) {
            Block->~CIdvBranch();
            ourFree(Block);
        }
        m_pBranch = nullptr;
        for (int32_t i = 0; i < m_sLeafInfo.m_nNumLeafLodLevels; ++i) {
            if (m_vLeafLods) {
                for (uint32_t j = 0; j < m_vLeafLods[i].size(); ++j) {
                    CBillboardLeaf* v6 = m_vLeafLods[i][j];
                    if (v6) {
                        delete v6;
                    }
                    m_vLeafLods[i][j] = nullptr;
                }
                m_vLeafLods[i].clear();
            }
        }
        std::vector<CBillboardLeaf*, std::allocator<CBillboardLeaf*>>* apThis = m_vLeafLods;
        if (apThis) {
            delete[] apThis;
        }
        m_vLeafLods = nullptr;
        for (uint32_t k = 0; k < m_vBranchInfo.size(); ++k) {
            SIdvBranchInfo* apMemory = m_vBranchInfo[k];
            if (apMemory) {
                apMemory->~SIdvBranchInfo();
                ourFree(apMemory);
            }
            m_vBranchInfo[k] = nullptr;
        }
        m_vBranchInfo.clear();
        m_bTransientDataIntact = false;
    }
}

void CTreeEngine::SetLodLimits(float fNear, float fFar) {
    m_sTreeInfo.m_fNear = fNear;
    m_sTreeInfo.m_fFar = fFar;
}

void CTreeEngine::SetSize(float fSize, float fVariance) {
    m_sTreeInfo.m_fSize = fSize;
    m_sTreeInfo.m_fSizeVariance = fVariance;
}

void CTreeEngine::ParseTreeInfo(CTreeFileAccess& cFile) {
    CIdvFilename v7;
    int32_t nSeed = cFile.ParseInt();
    do {
        if (nSeed > 2000) {
            switch (nSeed) {
                case 2001: {
                    m_sTreeInfo.m_fFar = cFile.ParseFloat();
                    goto LABEL_17;
                }
                case 2002: {
                    cFile.ParseBool();
                    goto LABEL_17;
                }
                case 2003: {
                    m_sTreeInfo.m_fNear = cFile.ParseFloat();
                    goto LABEL_17;
                }
                case 2004: {
                    cFile.ParseInt();
                    goto LABEL_17;
                }
                case 2005: {
                    int32_t nSeed_2 = cFile.ParseInt();
                    SetSeed(static_cast<uint32_t>(nSeed_2));
                    goto LABEL_17;
                }
                case 2006: {
                    m_sTreeInfo.m_fSize = cFile.ParseFloat();
                    goto LABEL_17;
                }
                case 2007: {
                    m_sTreeInfo.m_fSizeVariance = cFile.ParseFloat();
                    goto LABEL_17;
                }
                default: {
                    goto LABEL_16;
                }
            }
        }
        if (nSeed == 2000) {
            std::basic_string<char, std::char_traits<char>, std::allocator<char>> result = cFile.ParseString();
            std::basic_string<char, std::char_traits<char>, std::allocator<char>> a2;
            a2.assign(result, 0, std::string::npos);
            m_sTreeInfo.m_strBranchTextureFilename.assign(a2, 0, std::string::npos);
            CIdvFilename* _Right = m_sTreeInfo.m_strBranchTextureFilename.NoPath(&v7);
            m_sTreeInfo.m_strBranchTextureFilename.assign(*_Right, 0, std::string::npos);
        }
        else {
            if (nSeed != 1014) {
            LABEL_16:;
                using apMemory_t = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;
                apMemory_t apMemory("malformed general tree information");
                throw IdvFileError(apMemory, false);
            }
            ParseBranchInfo(cFile);
        }
    LABEL_17:;
        nSeed = cFile.ParseInt();
    }
    while (nSeed != 1003);
}

// The branches sorted by volume into LOD levels: the large branches (above m_fLargeBranchPercent of the largest) moved
// to the front, the others' volumes fuzzed. Both vectors are vector<CIdvBranch*>.
void CTreeEngine::BuildBranchLods() {
    using BranchVector = std::vector<CIdvBranch*, std::allocator<CIdvBranch*>>;
    using BranchConstIterator = BranchVector::const_iterator;
    BranchVector vAllBranches;
    if (m_pBranch && static_cast<uint16_t>(m_pBranchGeometry->m_vCoords.size() / 3) != 0) {
        m_pBranch->BuildBranchVector(vAllBranches);
    }
    float fTotalVolume = 0.0f;
    float fLargestVolume = 0.0f;
    for (uint32_t i = 0; i < vAllBranches.size(); ++i) {
        float fVolume = vAllBranches[i]->m_fVolume;
        fTotalVolume = fTotalVolume + fVolume;
        if (fLargestVolume < fVolume) {
            fLargestVolume = fVolume;
        }
    }
    CIdvRandom cRandom;
    float fSavePercent = 1.0 - m_fLargeBranchPercent;
    BranchVector vLargeBranches;
    for (uint32_t i = 0; i < vAllBranches.size(); ++i) {
        float fVolume = vAllBranches[i]->m_fVolume;
        if (fVolume > fLargestVolume * fSavePercent) {
            vLargeBranches.push_back(vAllBranches[i]);
            BranchVector::iterator kWhere = vAllBranches.begin();
            kWhere += i;
            vAllBranches.erase(kWhere);
            --i;
        }
        else {
            float fFuzziness = cRandom.GetUniform(0.0f, m_fBranchReductionFuzziness);
            float fOneMinusFuzziness = 1.0 - fFuzziness;
            fVolume = fOneMinusFuzziness * fVolume + fFuzziness * fLargestVolume;
            // max(fVolume, 0.0f): the larger one by reference.
            float fZero = 0.0f;
            float* pVolume;
            if (!(fZero > fVolume)) {
                pVolume = &fVolume;
            }
            else {
                pVolume = &fZero;
            }
            fVolume = *pVolume;
            vAllBranches[i]->m_fFuzzyVolume = fVolume;
        }
    }
    CIdvBranch::SortBranchVector(vAllBranches);
    CIdvBranch::SortBranchVector(vLargeBranches);
    for (uint32_t i = 0; i < vLargeBranches.size(); ++i) {
        BranchVector::iterator kWhere = vAllBranches.begin();
        CIdvBranch*& pLarge = vLargeBranches[i];
        vAllBranches.insert(kWhere, pLarge);
    }
    int32_t nNumBranchLodLevels = m_nNumBranchLodLevels;
    for (uint32_t uiLod = 0; uiLod < static_cast<uint32_t>(nNumBranchLodLevels); ++uiLod) {
        if (!uiLod) {
            m_pBranchGeometry->DeleteLodStrip(0);
        }
        float fPercent;
        if (nNumBranchLodLevels >= 2) {
            float fMaxPercent = m_fMaxBranchVolumePercent;
            float fLodRatio = static_cast<double>(uiLod) / static_cast<double>(nNumBranchLodLevels - 1);
            fPercent = fLodRatio * (m_fMinBranchVolumePercent - fMaxPercent) + fMaxPercent;
        }
        else {
            fPercent = 1.0f;
        }
        float fTargetVolume = fPercent * fTotalVolume;
        int32_t nBranchCount = 0;
        float fLodVolume = 0.0f;
        for (uint32_t j = 0; j < vAllBranches.size() && fTargetVolume > fLodVolume; ++j) {
            float fBranchVolume = vAllBranches[j]->m_fVolume;
            fLodVolume = fLodVolume + fBranchVolume;
            ++nBranchCount;
        }
        m_pBranchGeometry->ResetStripCounter(static_cast<uint16_t>(uiLod));
        if (uiLod || nBranchCount) {
            for (int32_t k = 0; k < nBranchCount; ++k) {
                vAllBranches[k]->ComputeLod(uiLod, m_pBranchGeometry);
            }
        }
        else {
            m_pBranchGeometry->AddStrip(0, nullptr, 0);
        }
    }
}

// The largest leaf texture's used size (the larger of its width and height) times the spacing tolerance, 10 when there
// are no textures; then each leaf LOD level after the first computed from the one before it.
void CTreeEngine::BuildLeafLods(float fLeafSizeIncreaseFactor) {
    float fLargestLeaf = -1.0f;
    for (uint32_t i = 0; i < m_sLeafInfo.m_vLeafTextures.size(); ++i) {
        SIdvLeafTexture* pTexture = &m_sLeafInfo.m_vLeafTextures[i];
        float fMax;
        if (!(pTexture->m_cSizeUsed.m_afData[1] < static_cast<double>(pTexture->m_cSizeUsed.m_afData[0]))) {
            fMax = pTexture->m_cSizeUsed.m_afData[1];
        }
        else {
            fMax = pTexture->m_cSizeUsed.m_afData[0];
        }
        float fLeafSize = fMax;
        if (fLargestLeaf < fLeafSize * m_sLeafInfo.m_fSpacingTolerance) {
            fLargestLeaf = fLeafSize * m_sLeafInfo.m_fSpacingTolerance;
        }
    }
    if (fLargestLeaf == -1.0) {
        fLargestLeaf = 10.0f;
    }
    if (m_vLeafLods) {
        *m_vLeafLods = m_vBillboardLeaves;
        for (int32_t j = 1; j < m_sLeafInfo.m_nNumLeafLodLevels; ++j) {
            float fSizeIncreaseFactor = static_cast<double>(j) * fLeafSizeIncreaseFactor + 1.0;
            float fSpacingTolerance = fLargestLeaf * 10.0;
            CLeafLodEngine cLodEngine(m_sLeafInfo, fSpacingTolerance, m_fLeafReductionPercentage, fSizeIncreaseFactor);
            LeafArray vArgument;
            vArgument.~LeafArray();
            ::new (&vArgument) LeafArray(m_vLeafLods[j - 1]);
            LeafArray vNextLevel;
            LeafArray* pNextLevel = cLodEngine.ComputeNextLevel(&vNextLevel, vArgument);
            m_vLeafLods[j] = *pNextLevel;
        }
    }
}

// It throws IdvFileError with a C++ throw, as SIdvBranchInfo::Parse (BranchInfo.cpp, which says where it is caught).
void CTreeEngine::ParseLeafCluster(CTreeFileAccess& cFile) {
    std::vector<CBillboardLeaf*, std::allocator<CBillboardLeaf*>>* v3;
    int32_t v17 = 0;
    int32_t nNumLeafLodLevels = cFile.ParseInt();
    m_sLeafInfo.m_nNumLeafLodLevels = nNumLeafLodLevels;
    std::vector<CBillboardLeaf*, std::allocator<CBillboardLeaf*>>* m_vLeafLods = this->m_vLeafLods;
    if (m_vLeafLods) {
        delete[] m_vLeafLods;
    }
    uint32_t m_nNumLeafLodLevels = m_sLeafInfo.m_nNumLeafLodLevels;
    this->m_vLeafLods = new std::vector<CBillboardLeaf*, std::allocator<CBillboardLeaf*>>[m_nNumLeafLodLevels];
    for (int32_t nToken = cFile.ParseInt(); nToken != File_EndLeafCluster; nToken = cFile.ParseInt()) {
        if (v17 >= m_sLeafInfo.m_nNumLeafLodLevels) {
            using apMemory_t = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;
            apMemory_t apMemory("too many leaf lod levels");
            throw IdvFileError(apMemory, false);
        }
        if (nToken != File_BeginLeafLodLevel) {
            using v8_t = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;
            v8_t v8("malformed leaf lod data");
            throw IdvFileError(v8, false);
        }
        for (nToken = cFile.ParseInt(); nToken != File_EndLeafLodLevel; nToken = cFile.ParseInt()) {
            if (nToken != File_BeginBillboardLeaf) {
                using v6_t = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;
                v6_t v6("malformed leaf lod data");
                throw IdvFileError(v6, false);
            }
            CBillboardLeaf* _Val = CBillboardLeaf::Parse(cFile);
            this->m_vLeafLods[v17].push_back(_Val);
        }
        ++v17;
    }
    m_bParsedLeafLods = true;
}

// It throws IdvFileError with a C++ throw, as SIdvBranchInfo::Parse (BranchInfo.cpp, which says where it is caught).
void CTreeEngine::ParseLodInfo(CTreeFileAccess& cFile) {
    int32_t eToken = cFile.ParseInt();
    do {
        switch (eToken) {
            case File_BranchNumLods: {
                m_nNumBranchLodLevels = cFile.ParseInt();
                break;
            }
            case File_BranchMinVolumePercent: {
                m_fMinBranchVolumePercent = cFile.ParseFloat();
                break;
            }
            case File_LeafReductionFactor: {
                m_fLeafReductionPercentage = cFile.ParseFloat();
                if (m_fLeafReductionPercentage == 0.0) {
                    m_fLeafReductionPercentage = 0.1f;
                }
                break;
            }
            case File_LeafNumLods: {
                m_sLeafInfo.m_nNumLeafLodLevels = cFile.ParseInt();
                break;
            }
            case File_BranchMaxVolumePercent: {
                m_fMaxBranchVolumePercent = cFile.ParseFloat();
                break;
            }
            case File_BranchReductionFuzziness: {
                m_fBranchReductionFuzziness = cFile.ParseFloat();
                break;
            }
            case File_LargeBranchPercent: {
                m_fLargeBranchPercent = cFile.ParseFloat();
                break;
            }
            default: {
                using apMemory_t = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;
                apMemory_t apMemory("malformed engine lod data");
                throw IdvFileError(apMemory, false);
            }
        }
        eToken = cFile.ParseInt();
    }
    while (eToken != File_EndEngineLodInfo);
}

// It throws IdvFileError with a C++ throw, as SIdvBranchInfo::Parse (BranchInfo.cpp, which says where it is caught).
void CTreeEngine::ParseBranchInfo(CTreeFileAccess& cFile) {
    m_vBranchInfo.clear();
    int32_t nSize = cFile.ParseInt();
    for (int32_t i = 0; i < nSize; ++i) {
        SIdvBranchInfo* pInfo = SIdvBranchInfo::Parse(cFile);
        m_vBranchInfo.push_back(pInfo);
    }
    int32_t eToken = cFile.ParseInt();
    if (eToken != File_EndBranchInfo) {
        using apMemory_t = std::basic_string<char, std::char_traits<char>, std::allocator<char>>;
        apMemory_t apMemory("malformed branch data");
        throw IdvFileError(apMemory, false);
    }
}

SIdvLeafTexture::SIdvLeafTexture()
  : m_bBlossom(false), m_cColor(0.8f, 0.8f, 0.8f), m_fColorVariance(0.2f), m_cOrigin(0.5f, 1.0f, 0.0f),
    m_cSize(0.12f, 0.12f, 0.0f), m_cSizeUsed(10.0f, 10.0f, 0.0f) {}

SIdvLeafTexture::SIdvLeafTexture(const SIdvLeafTexture& arOther)
  : m_bBlossom(arOther.m_bBlossom), m_cColor(arOther.m_cColor), m_fColorVariance(arOther.m_fColorVariance),
    m_strFilename(arOther.m_strFilename), m_cOrigin(arOther.m_cOrigin), m_cSize(arOther.m_cSize),
    m_cSizeUsed(arOther.m_cSizeUsed) {}

SIdvLeafTexture::~SIdvLeafTexture() {}

SIdvLeafTexture& SIdvLeafTexture::Assign(const SIdvLeafTexture& arOther) {
    m_bBlossom = arOther.m_bBlossom;
    m_cColor = arOther.m_cColor;
    m_fColorVariance = arOther.m_fColorVariance;
    m_strFilename.assign(arOther.m_strFilename, static_cast<size_t>(0), std::string::npos);
    m_cOrigin = arOther.m_cOrigin;
    m_cSize = arOther.m_cSize;
    m_cSizeUsed = arOther.m_cSizeUsed;
    return *this;
}
