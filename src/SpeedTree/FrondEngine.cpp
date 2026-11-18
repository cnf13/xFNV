#include "SpeedTree/FrondEngine.h"

#include "BSCore/MemoryManager.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/crtdefs.h"
#include "SDK/MSVC/exception.h"
#include "SDK/MSVC/string.h"
#include "SpeedTree/BillboardLeaf.h"
#include "SpeedTree/CIdvFilename.h"
#include "SpeedTree/CRotTransform.h"
#include "SpeedTree/EFileToken.h"
#include "SpeedTree/FileAccess.h"
#include "SpeedTree/IndexedGeometry.h"
#include "SpeedTree/LibGlobals_Source/IdvGlobals.h"
#include "SpeedTree/LibRandom_Source/IdvRandom.h"
#include "SpeedTree/LibSpline_Source/IdvSpline.h"
#include "SpeedTree/LibVector_Source/IdvVector.h"
#include "SpeedTree/SpeedTreeRT.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

// The frond engine's vectors under one name each, for the registrations: a macro argument can't hold the template's
// commas.
using SFrondGuideVector = std::vector<SFrondGuide, std::allocator<SFrondGuide>>;
using CVecVector = std::vector<CVec, std::allocator<CVec>>;
using SFrondTextureVector = std::vector<SFrondTexture, std::allocator<SFrondTexture>>;
using CVec3Vector = std::vector<CVec3, std::allocator<CVec3>>;
using FloatVector = std::vector<float, std::allocator<float>>;

// The element vectors under one name each, for the registrations: a macro argument can't hold the template's commas.
using SFrondGuideVector = std::vector<SFrondGuide, std::allocator<SFrondGuide>>;

using FloatVector = std::vector<float, std::allocator<float>>;

// The transform is the identity through its own constructor.
SFrondVertex::SFrondVertex() : m_cPos(0.0f, 0.0f, 0.0f), m_fCrossSectionWeight(0.0f), m_nWindGroup(0) {}

SFrondGuide::SFrondGuide()
  : m_fLength(0.0f), m_fRadius(0.0f), m_chFrondMapIndex(0), m_fOffsetAngle(0.0f), m_fSurfaceArea(0.0f),
    m_fFuzzySurfaceArea(0.0f), m_nVertexStartIndex(0), m_nVerticesPerGuideVertex(0) {}

SFrondGuide::SFrondGuide(const SFrondGuide& arOther)
  : m_vVertices(arOther.m_vVertices), m_fLength(arOther.m_fLength), m_fRadius(arOther.m_fRadius),
    m_chFrondMapIndex(arOther.m_chFrondMapIndex), m_fOffsetAngle(arOther.m_fOffsetAngle),
    m_fSurfaceArea(arOther.m_fSurfaceArea), m_fFuzzySurfaceArea(arOther.m_fFuzzySurfaceArea),
    m_nVertexStartIndex(arOther.m_nVertexStartIndex), m_nVerticesPerGuideVertex(arOther.m_nVerticesPerGuideVertex) {}

// The vertex vector is destroyed by its own destructor.
SFrondGuide::~SFrondGuide() {}

// operator= of a guide: the vertex vector assigned, then the other fields; returns apThis.
SFrondGuide* SFrondGuide::Assign(SFrondGuide* apThis, const SFrondGuide& arOther) {
    apThis->m_vVertices = arOther.m_vVertices;
    apThis->m_fLength = arOther.m_fLength;
    apThis->m_fRadius = arOther.m_fRadius;
    apThis->m_chFrondMapIndex = arOther.m_chFrondMapIndex;
    apThis->m_fOffsetAngle = arOther.m_fOffsetAngle;
    apThis->m_fSurfaceArea = arOther.m_fSurfaceArea;
    apThis->m_fFuzzySurfaceArea = arOther.m_fFuzzySurfaceArea;
    apThis->m_nVertexStartIndex = arOther.m_nVertexStartIndex;
    apThis->m_nVerticesPerGuideVertex = arOther.m_nVerticesPerGuideVertex;
    return apThis;
}

SFrondTexture::SFrondTexture(const SFrondTexture& arOther)
  : m_strFilename(arOther.m_strFilename), m_fAspectRatio(arOther.m_fAspectRatio), m_fSizeScale(arOther.m_fSizeScale),
    m_fMinAngleOffset(arOther.m_fMinAngleOffset), m_fMaxAngleOffset(arOther.m_fMaxAngleOffset) {}

// The file name is destroyed by its own destructor.
SFrondTexture::~SFrondTexture() {}

// operator= of a texture: the file name assigned, then the other fields; returns apThis.
SFrondTexture* SFrondTexture::Assign(SFrondTexture* apThis, const SFrondTexture& arOther) {
    apThis->m_strFilename.assign(arOther.m_strFilename, static_cast<size_t>(0), std::string::npos);
    apThis->m_fAspectRatio = arOther.m_fAspectRatio;
    apThis->m_fSizeScale = arOther.m_fSizeScale;
    apThis->m_fMinAngleOffset = arOther.m_fMinAngleOffset;
    apThis->m_fMaxAngleOffset = arOther.m_fMaxAngleOffset;
    return apThis;
}

// vsprintf into a 1024-byte buffer, then the result string built from the buffer.
std::string IdvFormatString(const char* szFormat, ...) {
    char szBuffer[1024];
    va_list vlArgs;
    va_start(vlArgs, szFormat);
    std::vsprintf(szBuffer, szFormat, vlArgs);
    std::string strResult;
    XFNV_RECONSTRUCT(std::string)(&strResult, szBuffer);
    return strResult;
}

// The profile spline is parsed from the default text.
CFrondEngine::CFrondEngine()
  : m_pGeometry(nullptr), m_pLightingEngine(nullptr), m_eFrondType(FROND_EXTRUSION), m_nNumBlades(2),
    m_nProfileSegments(4), m_nLevel(1), m_bEnabled(false), m_nNumLodLevels(4), m_fMaxSurfaceAreaPercent(1.0f),
    m_fMinSurfaceAreaPercent(0.0f), m_fReductionFuzziness(0.0f), m_fLargeRetentionPercent(0.05f),
    m_nMinLengthSegments(2), m_nMinCrossSegments(1) {
    m_pProfile = new CIdvBezierSpline(
        std::string(
            "BezierSpline 0.0 1.0 0.0 { 3 0 0.00138887 0.337009 0.941501 "
            "0.132767 0.493215 0.998903 1 0.00102074 "
            "0.23702 1 -6.24607e-008 0.307222 -0.951638 0.126974 }"
        )
    );
}

// The profile spline is owned.
CFrondEngine::~CFrondEngine() {
    delete m_pProfile;
    m_pProfile = nullptr;
}

// The vertex is built in a local (its constructor, then the arguments copied in) and pushed onto the last guide's
// vertices; the last guide is m_vGuides[size() - 1].
void CFrondEngine::AddGuideVertex(CVec3 cPos, CRotTransform cTrans, float fCrossSectionWeight, int32_t nWindGroup) {
    SFrondVertex sVertex;
    sVertex.~SFrondVertex();
    ::new (&sVertex) SFrondVertex;
    sVertex.m_cPos = cPos;
    sVertex.m_cTrans = cTrans;
    sVertex.m_fCrossSectionWeight = fCrossSectionWeight;
    sVertex.m_nWindGroup = nWindGroup;
    m_vGuides[m_vGuides.size() - 1].m_vVertices.push_back(sVertex);
}

void CFrondEngine::StartGuide() {
    SFrondGuide sGuide;
    m_vGuides.push_back(sGuide);
}

// The empty-texture branch clears the map index of guide i (the length loop's last index), not of the last guide, as
// the game does.
void CFrondEngine::EndGuide(float fLodSizeScalar) {
    SFrondGuide& sGuide = m_vGuides[m_vGuides.size() - 1];
    uint32_t i;
    for (i = 0; i < sGuide.m_vVertices.size() - 1; ++i) {
        // Subscripts i + 1 before i.
        const SFrondVertex& sNext = sGuide.m_vVertices[i + 1];
        sGuide.m_fLength = sGuide.m_vVertices[i].m_cPos.Distance(sNext.m_cPos) + sGuide.m_fLength;
    }
    CIdvRandom cRandom;
    if (m_vTextures.size() != 0) {
        uint32_t uiNumTextures = m_vTextures.size();
        sGuide.m_chFrondMapIndex = static_cast<uint8_t>(
            static_cast<uint32_t>(static_cast<int64_t>(cRandom.GetUniform(0.0f, 100000.0f))) % uiNumTextures
        );
        sGuide.m_fRadius = sGuide.m_fLength * m_vTextures[sGuide.m_chFrondMapIndex].m_fAspectRatio * 0.5;
        sGuide.m_fRadius = sGuide.m_fRadius * m_vTextures[sGuide.m_chFrondMapIndex].m_fSizeScale;
        // The maximum's texture is subscripted before the minimum's.
        const SFrondTexture& sMaxTexture = m_vTextures[sGuide.m_chFrondMapIndex];
        const SFrondTexture& sMinTexture = m_vTextures[sGuide.m_chFrondMapIndex];
        sGuide.m_fOffsetAngle = cRandom.GetUniform(sMinTexture.m_fMinAngleOffset, sMaxTexture.m_fMaxAngleOffset);
        if (m_vGuides.size() % 2 == 1) {
            sGuide.m_fOffsetAngle = sGuide.m_fOffsetAngle * -1.0;
        }
    }
    else {
        m_vGuides[i].m_chFrondMapIndex = 0;
        sGuide.m_fRadius = sGuide.m_fLength * 0.5;
        sGuide.m_fOffsetAngle = 0.0f;
    }
    sGuide.m_fSurfaceArea = sGuide.m_fLength * fLodSizeScalar;
}

// The old profile goes as `delete p`: the destructor, then the engine's free.
void CFrondEngine::SetProfile(CIdvBezierSpline* pProfile) {
    if (m_pProfile != pProfile) {
        CIdvBezierSpline* pOld = m_pProfile;
        if (pOld) {
            pOld->~CIdvBezierSpline();
            ourFree(pOld);
        }
        m_pProfile = pProfile;
    }
}

int32_t CFrondEngine::GetLevel() const {
    return m_nLevel;
}

// The token and the texture tokens share one local.
void CFrondEngine::Parse(CTreeFileAccess& cFile) {
    int32_t nToken = cFile.ParseToken();
    do {
        switch (nToken) {
            case File_FrondLevel: m_nLevel = cFile.ParseInt(); break;
            case File_FrondType: m_eFrondType = static_cast<EFrondType>(cFile.ParseInt()); break;
            case File_FrondNumBlades: m_nNumBlades = cFile.ParseInt(); break;
            case File_FrondProfile: SetProfile(cFile.ParseBranchParameter()); break;
            case File_FrondProfileSegments: m_nProfileSegments = cFile.ParseInt(); break;
            case File_FrondEnabled: m_bEnabled = cFile.ParseBool(); break;
            case File_FrondNumTextures: {
                m_vTextures.clear();
                int32_t nNumTextures = cFile.ParseInt();
                for (int32_t i = 0; i < nNumTextures; ++i) {
                    SFrondTexture sTexture;
                    ::new (&sTexture.m_strFilename) std::string;
                    sTexture.m_fAspectRatio = 0.5f;
                    sTexture.m_fSizeScale = 1.0f;
                    sTexture.m_fMinAngleOffset = 0.0f;
                    sTexture.m_fMaxAngleOffset = 0.0f;
                    // The texture's begin token, then its first token.
                    nToken = cFile.ParseToken();
                    nToken = cFile.ParseToken();
                    do {
                        switch (nToken) {
                            case File_FrondTextureFilename: {
                                std::string strParsed = cFile.ParseString();
                                std::string strFilename;
                                XFNV_RECONSTRUCT_AS(CopyConstruct, std::string)(&strFilename, strParsed);
                                sTexture.m_strFilename.assign(strFilename, 0, std::string::npos);
                                CIdvFilename strNoPath;
                                CIdvFilename* pNoPath = sTexture.m_strFilename.NoPath(&strNoPath);
                                sTexture.m_strFilename.assign(*pNoPath, 0, std::string::npos);
                                break;
                            }
                            case File_FrondTextureAspectRatio: sTexture.m_fAspectRatio = cFile.ParseFloat(); break;
                            case File_FrondTextureSizeScale: sTexture.m_fSizeScale = cFile.ParseFloat(); break;
                            case File_FrondTextureMinAngleOffset:
                                sTexture.m_fMinAngleOffset = cFile.ParseFloat();
                                break;
                            case File_FrondTextureMaxAngleOffset:
                                sTexture.m_fMaxAngleOffset = cFile.ParseFloat();
                                break;
                            default: {
                                std::string strDetails =
                                    IdvFormatString("malformed frond texture information (token %d)", nToken);
                                throw IdvFileError(strDetails, false);
                            }
                        }
                        nToken = cFile.ParseToken();
                    }
                    while (nToken != File_EndFrondTexture);
                    m_vTextures.push_back(sTexture);
                }
                break;
            }
            case File_FrondNumLodLevels: m_nNumLodLevels = cFile.ParseInt(); break;
            case File_FrondMaxSurfaceAreaPercent: m_fMaxSurfaceAreaPercent = cFile.ParseFloat(); break;
            case File_FrondMinSurfaceAreaPercent: m_fMinSurfaceAreaPercent = cFile.ParseFloat(); break;
            case File_FrondReductionFuzziness: m_fReductionFuzziness = cFile.ParseFloat(); break;
            case File_FrondLargeFrondPercent: m_fLargeRetentionPercent = cFile.ParseFloat(); break;
            case File_FrondMinLengthSegments: m_nMinLengthSegments = cFile.ParseInt(); break;
            case File_FrondMinCrossSegments: m_nMinCrossSegments = cFile.ParseInt(); break;
            default: {
                std::string strDetails = IdvFormatString("malformed frond info (token %d)", nToken);
                throw IdvFileError(strDetails, false);
            }
        }
        nToken = cFile.ParseToken();
    }
    while (nToken != File_EndFrondInfo);
}

bool CFrondEngine::Enabled() const {
    return (bool)((uint8_t)m_bEnabled);
}

// Copies the eight texture coordinates (the odd ones negated when bFlip) to each vertex of the geometry, one
// ChangeTexCoord per vertex; the vertex count, a third of the coordinates, is recomputed every pass.
void CFrondEngine::SetTextureCoords(
    CIndexedGeometry* pGeometry,
    uint32_t nFrondMapIndex,
    const float* pTexCoords,
    bool bFlip
) {
    float afCoords[8];
    rt::memcpy(afCoords, pTexCoords, sizeof(afCoords));
    if (bFlip) {
        for (int32_t i = 1; i < 8; i += 2) {
            afCoords[i] = -afCoords[i];
        }
    }
    uint8_t chMapIndex = static_cast<uint8_t>(nFrondMapIndex);
    pGeometry->m_nVertexCounter = 0;
    for (int32_t j = 0; j < static_cast<uint16_t>(pGeometry->m_vCoords.size() / 3); j++) {
        pGeometry->ChangeTexCoord(chMapIndex, afCoords);
        pGeometry->m_nVertexCounter++;
    }
}

void CFrondEngine::Compute(CIndexedGeometry* pGeometry, CLightingEngine* pLightingEngine) {
    m_pGeometry = pGeometry;
    m_pLightingEngine = pLightingEngine;
    int32_t nVertices = 0;
    BuildGuideLods();
    for (uint32_t i = 0; i < m_vGuideLods[0].size(); ++i) {
        switch (m_eFrondType) {
            case FROND_BLADES: nVertices += m_nNumBlades * (2 * m_vGuideLods[0][i].m_vVertices.size()); break;
            case FROND_EXTRUSION:
                nVertices += (2 * m_nProfileSegments - 1) * m_vGuideLods[0][i].m_vVertices.size();
                break;
            default: {
                std::string strDetails;
                XFNV_RECONSTRUCT(std::string)(&strDetails, "default reached in CFrondEngine::Compute()");
                throw IdvFileError(strDetails, false);
            }
        }
    }
    if (nVertices > 0xFFFF) {
        std::string strMessage = IdvFormatString("frond vertices exceed %d", 0xFFFF);
        XFNV_INLINED_LOCAL(rt::runtime_error, cError, std::nullopt, std::nullopt);
        // Natively runtime_error's own constructor: its vtable, then the message string's copy.
        ::new (&cError) rt::runtime_error(strMessage);
        throw cError;
    }
    m_pGeometry->SetNumLodLevels(static_cast<uint16_t>(m_nNumLodLevels));
    switch (m_eFrondType) {
        case FROND_BLADES:
            for (uint32_t i = 0; i < m_vGuideLods[0].size(); ++i) {
                BuildBladeVertices(m_vGuideLods[0][i]);
            }
            for (uint16_t nLod = 0; nLod < m_vGuideLods.size(); ++nLod) {
                m_pGeometry->ResetStripCounter(nLod);
                for (uint32_t k = 0; k < m_vGuideLods[nLod].size(); ++k) {
                    // Subscripts the LOD's guide before the first LOD's.
                    SFrondGuide& sGuide = m_vGuideLods[nLod][k];
                    ComputeBlade(nLod, m_vGuideLods[0][k].m_nVertexStartIndex, sGuide);
                }
            }
            break;
        case FROND_EXTRUSION:
            for (uint32_t i = 0; i < m_vGuideLods[0].size(); ++i) {
                BuildExtrusionVertices(m_vGuideLods[0][i]);
            }
            for (uint16_t nLod = 0; nLod < m_vGuideLods.size(); ++nLod) {
                m_pGeometry->ResetStripCounter(nLod);
                for (uint32_t k = 0; k < m_vGuideLods[nLod].size(); ++k) {
                    // Subscripts the first LOD's guide before the destination's.
                    const SFrondGuide& sFirst = m_vGuideLods[0][k];
                    m_vGuideLods[nLod][k].m_nVerticesPerGuideVertex = sFirst.m_nVerticesPerGuideVertex;
                    SFrondGuide& sGuide = m_vGuideLods[nLod][k];
                    ComputeExtrusion(nLod, m_vGuideLods[0][k].m_nVertexStartIndex, sGuide);
                }
            }
            break;
        default: {
            std::string strDetails;
            XFNV_RECONSTRUCT(std::string)(&strDetails, "default reached in CFrondEngine::Compute()");
            throw IdvFileError(strDetails, false);
        }
    }
}

// There is no static lighting branch (GetFrondLightingMethod, ComputeStandardStaticLighting), so each vertex always
// gets the colour (1, 1, 1, 1) and then the normal, tangent and binormal; m_pLightingEngine is only tested. The blade's
// locals are destroyed at its end.
void CFrondEngine::BuildBladeVertices(SFrondGuide& sGuide) {
    if (m_pGeometry != nullptr && m_pLightingEngine != nullptr) {
        float fAngleBetween = static_cast<float>(180.0 / static_cast<double>(m_nNumBlades));
        sGuide.m_nVertexStartIndex = m_pGeometry->m_nVertexCounter;
        sGuide.m_nVerticesPerGuideVertex = 2;
        for (uint32_t k = 0; k < m_nNumBlades; ++k) {
            int32_t nLocalStart = m_pGeometry->m_nVertexCounter;
            std::vector<CVec3, std::allocator<CVec3>> vRightDirections;
            vRightDirections.~vector<CVec3, std::allocator<CVec3>>();
            ::new (&vRightDirections) std::vector<CVec3, std::allocator<CVec3>>;
            CVec3 cBinormal;
            cBinormal.m_afData[2] = 0.0f;
            cBinormal.m_afData[1] = 0.0f;
            cBinormal.m_afData[0] = 0.0f;
            for (uint32_t i = 0; i < sGuide.m_vVertices.size(); ++i) {
                CRotTransform cTrans;
                cTrans.m_afData[0][0] = 1.0f;
                cTrans.m_afData[0][1] = 0.0f;
                cTrans.m_afData[0][2] = 0.0f;
                cTrans.m_afData[1][0] = 0.0f;
                cTrans.m_afData[1][1] = 1.0f;
                cTrans.m_afData[1][2] = 0.0f;
                cTrans.m_afData[2][0] = 0.0f;
                cTrans.m_afData[2][1] = 0.0f;
                cTrans.m_afData[2][2] = 1.0f;
                if (i < sGuide.m_vVertices.size() - 1) {
                    cTrans = sGuide.m_vVertices[i].m_cTrans;
                }
                else {
                    cTrans = sGuide.m_vVertices[i - 1].m_cTrans;
                }
                // The blade index is converted as unsigned.
                cTrans.RotateX(static_cast<float>(static_cast<double>(k) * fAngleBetween + sGuide.m_fOffsetAngle));

                CVec3 cUp;
                cUp.m_afData[0] = 0.0f;
                cUp.m_afData[1] = 1.0f;
                cUp.m_afData[2] = 0.0f;
                CVec3 cRightDir = cUp * cTrans;
                vRightDirections.push_back(cRightDir);

                CVec3 cNormal;
                cNormal.m_afData[2] = 0.0f;
                cNormal.m_afData[1] = 0.0f;
                cNormal.m_afData[0] = 0.0f;
                CVec3 cScaled;
                CVec3 cResult;
                if (i == 0) {
                    SFrondVertex& rFirst = sGuide.m_vVertices[0];
                    SFrondVertex& rNext = sGuide.m_vVertices[i + 1];
                    cNormal = *rNext.m_cPos.Diff(&cResult, &rFirst.m_cPos);
                }
                else {
                    SFrondVertex& rFirst = sGuide.m_vVertices[0];
                    SFrondVertex& rThis = sGuide.m_vVertices[i];
                    cNormal = *rThis.m_cPos.Diff(&cResult, &rFirst.m_cPos);
                }
                cNormal.Normalize();

                CVec3 cRight;
                cRight.m_afData[2] = 0.0f;
                cRight.m_afData[1] = 0.0f;
                cRight.m_afData[0] = 0.0f;
                CVec3 cLeft;
                cLeft.m_afData[2] = 0.0f;
                cLeft.m_afData[1] = 0.0f;
                cLeft.m_afData[0] = 0.0f;
                if (i == sGuide.m_vVertices.size() - 1 && sGuide.m_vVertices.size() > 0) {
                    CVec3& rPrevDir = vRightDirections[i - 1];
                    SFrondVertex& rThis = sGuide.m_vVertices[i];
                    cRight = *rThis.m_cPos.Add(&cResult, rPrevDir.Scale(&cScaled, sGuide.m_fRadius));
                    CVec3& rPrevDir2 = vRightDirections[i - 1];
                    SFrondVertex& rThis2 = sGuide.m_vVertices[i];
                    cLeft = *rThis2.m_cPos.Diff(&cResult, rPrevDir2.Scale(&cScaled, sGuide.m_fRadius));
                }
                else if (i > 0 && i < sGuide.m_vVertices.size() - 1) {
                    CVec3 cTemp;
                    cTemp.m_afData[2] = 0.0f;
                    cTemp.m_afData[1] = 0.0f;
                    cTemp.m_afData[0] = 0.0f;

                    // The right side: the mean of the edge along the previous and the current direction.
                    CVec3& rPrevDir = vRightDirections[i - 1];
                    SFrondVertex& rThis = sGuide.m_vVertices[i];
                    cTemp = *rThis.m_cPos.Add(&cResult, rPrevDir.Scale(&cScaled, sGuide.m_fRadius));
                    SFrondVertex& rThis2 = sGuide.m_vVertices[i];
                    cRight = *rThis2.m_cPos.Add(&cResult, cRightDir.Scale(&cScaled, sGuide.m_fRadius));
                    cRight = *cRight.Add(&cResult, &cTemp)->Scale(&cScaled, 0.5f);

                    // The left side, the same way.
                    CVec3& rPrevDir3 = vRightDirections[i - 1];
                    SFrondVertex& rThis3 = sGuide.m_vVertices[i];
                    cTemp = *rThis3.m_cPos.Diff(&cResult, rPrevDir3.Scale(&cScaled, sGuide.m_fRadius));
                    SFrondVertex& rThis4 = sGuide.m_vVertices[i];
                    cLeft = *rThis4.m_cPos.Diff(&cResult, cRightDir.Scale(&cScaled, sGuide.m_fRadius));
                    cLeft = *cLeft.Add(&cResult, &cTemp)->Scale(&cScaled, 0.5f);
                }
                else {
                    SFrondVertex& rThis = sGuide.m_vVertices[i];
                    cRight = *rThis.m_cPos.Add(&cResult, cRightDir.Scale(&cScaled, sGuide.m_fRadius));
                    SFrondVertex& rThis2 = sGuide.m_vVertices[i];
                    cLeft = *rThis2.m_cPos.Diff(&cResult, cRightDir.Scale(&cScaled, sGuide.m_fRadius));
                }

                // Both counts are converted as unsigned.
                float fLengthPercent =
                    static_cast<float>(static_cast<double>(i) / (static_cast<double>(sGuide.m_vVertices.size()) - 1.0));

                // The right vertex.
                m_pGeometry->AddVertexCoord(cRight.m_afData);
                float afTexCoords[2];
                afTexCoords[0] = 1.0f;
                afTexCoords[1] = fLengthPercent;
                m_pGeometry->AddVertexTexCoord0(afTexCoords, sGuide.m_chFrondMapIndex);
                float afColor[4];
                afColor[0] = afColor[1] = afColor[2] = afColor[3] = 1.0f;
                m_pGeometry->AddVertexColor(afColor);
                m_pGeometry->AddVertexNormal(cNormal.m_afData);
                m_pGeometry->AddVertexTangent(cRightDir.m_afData);
                cBinormal = cRightDir.Cross(cNormal);
                // A NaN magnitude normalizes.
                if (cBinormal.Magnitude() < 0.1f) {
                    cBinormal = cNormal;
                }
                else {
                    cBinormal.Normalize();
                }
                m_pGeometry->AddVertexBinormal(cBinormal.m_afData);
                if (m_pGeometry->m_bVertexWeighting) {
                    SFrondVertex& rGroupVertex = sGuide.m_vVertices[i];
                    SFrondVertex& rWeightVertex = sGuide.m_vVertices[i];
                    m_pGeometry->AddVertexWind(
                        rWeightVertex.m_fCrossSectionWeight, static_cast<uint8_t>(rGroupVertex.m_nWindGroup)
                    );
                }
                ++m_pGeometry->m_nVertexCounter;

                // The left vertex.
                m_pGeometry->AddVertexCoord(cLeft.m_afData);
                afTexCoords[0] = 0.0f;
                m_pGeometry->AddVertexTexCoord0(afTexCoords, sGuide.m_chFrondMapIndex);
                afColor[0] = afColor[1] = afColor[2] = afColor[3] = 1.0f;
                m_pGeometry->AddVertexColor(afColor);
                m_pGeometry->AddVertexNormal(cNormal.m_afData);
                m_pGeometry->AddVertexTangent(cRightDir.m_afData);
                cBinormal = cRightDir.Cross(cNormal);
                if (cBinormal.Magnitude() < 0.1f) {
                    cBinormal = cNormal;
                }
                else {
                    cBinormal.Normalize();
                }
                m_pGeometry->AddVertexBinormal(cBinormal.m_afData);
                if (m_pGeometry->m_bVertexWeighting) {
                    SFrondVertex& rGroupVertex = sGuide.m_vVertices[i];
                    uint8_t ucWindGroup = static_cast<uint8_t>(rGroupVertex.m_nWindGroup);
                    float fWeight = sGuide.m_vVertices[i].m_fCrossSectionWeight;
                    m_pGeometry->AddVertexWind(fWeight, ucWindGroup);
                }
                ++m_pGeometry->m_nVertexCounter;
            }

            // The running lengths of the right and left sides.
            float fRightLength = 0.0f;
            float fLeftLength = 0.0f;
            std::vector<float, std::allocator<float>> vRightLengths;
            vRightLengths.~vector<float, std::allocator<float>>();
            ::new (&vRightLengths) std::vector<float, std::allocator<float>>;
            std::vector<float, std::allocator<float>> vLeftLengths;
            vLeftLengths.~vector<float, std::allocator<float>>();
            ::new (&vLeftLengths) std::vector<float, std::allocator<float>>;
            uint32_t nSize = sGuide.m_vVertices.size();
            for (uint32_t i = 0; i < nSize - 1; ++i) {
                const float* pThisCoord =
                    m_pGeometry->GetVertexCoord(static_cast<int32_t>(sGuide.m_nVertexStartIndex + i * 2));
                const float* pNextCoord =
                    m_pGeometry->GetVertexCoord(static_cast<int32_t>(sGuide.m_nVertexStartIndex + i * 2 + 2));
                CVec3 cThis;
                cThis.m_afData[0] = pThisCoord[0];
                cThis.m_afData[1] = pThisCoord[1];
                cThis.m_afData[2] = pThisCoord[2];
                CVec3 cNext;
                cNext.~CVec3();
                ::new (&cNext) CVec3(pNextCoord[0], pNextCoord[1], pNextCoord[2]);
                fRightLength = cThis.Distance(cNext) + fRightLength;
                vRightLengths.push_back(fRightLength);

                pThisCoord = m_pGeometry->GetVertexCoord(static_cast<int32_t>(sGuide.m_nVertexStartIndex + i * 2 + 1));
                pNextCoord = m_pGeometry->GetVertexCoord(static_cast<int32_t>(sGuide.m_nVertexStartIndex + i * 2 + 3));
                cThis.Set(pThisCoord[0], pThisCoord[1], pThisCoord[2]);
                cNext.Set(pNextCoord[0], pNextCoord[1], pNextCoord[2]);
                fLeftLength = cThis.Distance(cNext) + fLeftLength;
                vLeftLengths.push_back(fLeftLength);
            }

            // The inner vertices' texture coordinates along the side lengths.
            m_pGeometry->m_nVertexCounter = static_cast<uint16_t>(nLocalStart + 2);
            for (uint32_t i = 1; i < nSize - 1; ++i) {
                const float* pTexCoord = m_pGeometry->GetVertexTexCoord0(sGuide.m_nVertexStartIndex + i * 2);
                float afRightTexCoord[2];
                afRightTexCoord[0] = pTexCoord[0];
                afRightTexCoord[1] =
                    static_cast<float>(static_cast<double>(vRightLengths[i - 1]) / fRightLength * 0.999f);
                m_pGeometry->AddVertexTexCoord0(afRightTexCoord, sGuide.m_chFrondMapIndex);
                ++m_pGeometry->m_nVertexCounter;

                pTexCoord = m_pGeometry->GetVertexTexCoord0(sGuide.m_nVertexStartIndex + i * 2 + 1);
                float afLeftTexCoord[2];
                afLeftTexCoord[0] = pTexCoord[0];
                afLeftTexCoord[1] = static_cast<float>(static_cast<double>(vLeftLengths[i - 1]) / fLeftLength * 0.999f);
                m_pGeometry->AddVertexTexCoord0(afLeftTexCoord, sGuide.m_chFrondMapIndex);
                ++m_pGeometry->m_nVertexCounter;
            }

            // Skips the last pair.
            ++m_pGeometry->m_nVertexCounter;
            ++m_pGeometry->m_nVertexCounter;
        }
    }
}

// The last profile vertex's normal angle adds c_fHalfPi where speedtree-2 adds c_fQuarterPi, as the game does. The
// percentages are computed in double from the counts converted unsigned. atan2f, sinf and cosf are computed as double
// atan2, sin and cos, the arguments and results rounded to float; sinf is taken before cosf. Each index goes through
// operator[] with its range check: for atan2f(y, x) the elements of x's difference are taken before y's, and y is
// computed first.
void CFrondEngine::BuildProfileVectors(
    SFrondGuide& sGuide,
    std::vector<CVec3, std::allocator<CVec3>>& vProfile,
    std::vector<CVec3, std::allocator<CVec3>>& vNormals
) {
    float fAdjust = (m_pProfile->Evaluate(0.0f) * sGuide.m_fRadius);
    for (uint32_t i = 0; i < m_nProfileSegments; ++i) {
        float fPercent =
            static_cast<float>(1.0 - static_cast<double>(i) / (static_cast<double>(m_nProfileSegments) - 1.0));
        CVec3 cVertex;
        cVertex.m_afData[2] = 0.0f;
        cVertex.m_afData[1] = 0.0f;
        cVertex.m_afData[0] = 0.0f;
        cVertex.m_afData[0] = 0.0f;
        cVertex.m_afData[1] = -fPercent * sGuide.m_fRadius;
        cVertex.m_afData[2] = (m_pProfile->Evaluate(fPercent) * sGuide.m_fRadius - fAdjust);
        vProfile.push_back(cVertex);
    }

    // The mirror image, back towards the start.
    for (int32_t j = static_cast<int32_t>(m_nProfileSegments - 2); j >= 0; --j) {
        CVec3 cVertex = vProfile[j];
        cVertex.m_afData[1] = static_cast<float>(cVertex.m_afData[1] * -1.0);
        vProfile.push_back(cVertex);
    }

    for (uint32_t i = 0; i < vProfile.size(); ++i) {
        float fAngle;
        if (i == m_nProfileSegments - 1) {
            fAngle = c_fHalfPi;
        }
        else if (i == 0) {
            CVec3& rXNext = vProfile[i + 1];
            CVec3& rXThis = vProfile[i];
            CVec3& rYNext = vProfile[i + 1];
            CVec3& rYThis = vProfile[i];
            float fY = rYNext.m_afData[2] - rYThis.m_afData[2];
            float fX = rXNext.m_afData[1] - rXThis.m_afData[1];
            float fNextAngle = static_cast<float>(std::atan2(static_cast<double>(fY), static_cast<double>(fX)));
            fAngle = fNextAngle + c_fHalfPi;
        }
        else if (i == vProfile.size() - 1) {
            CVec3& rXThis = vProfile[i];
            CVec3& rXPrev = vProfile[i - 1];
            CVec3& rYThis = vProfile[i];
            CVec3& rYPrev = vProfile[i - 1];
            float fY = rYThis.m_afData[2] - rYPrev.m_afData[2];
            float fX = rXThis.m_afData[1] - rXPrev.m_afData[1];
            float fPreviousAngle = static_cast<float>(std::atan2(static_cast<double>(fY), static_cast<double>(fX)));
            fAngle = fPreviousAngle + c_fHalfPi;
        }
        else {
            CVec3& rXNext = vProfile[i + 1];
            CVec3& rXThis = vProfile[i];
            CVec3& rYNext = vProfile[i + 1];
            CVec3& rYThis = vProfile[i];
            float fNextY = rYNext.m_afData[2] - rYThis.m_afData[2];
            float fNextX = rXNext.m_afData[1] - rXThis.m_afData[1];
            float fNextAngle = static_cast<float>(std::atan2(static_cast<double>(fNextY), static_cast<double>(fNextX)));
            CVec3& rXThis2 = vProfile[i];
            CVec3& rXPrev = vProfile[i - 1];
            CVec3& rYThis2 = vProfile[i];
            CVec3& rYPrev = vProfile[i - 1];
            float fPrevY = rYThis2.m_afData[2] - rYPrev.m_afData[2];
            float fPrevX = rXThis2.m_afData[1] - rXPrev.m_afData[1];
            float fPreviousAngle =
                static_cast<float>(std::atan2(static_cast<double>(fPrevY), static_cast<double>(fPrevX)));
            // The mean is scaled by the double 0.5 before c_fHalfPi is added.
            fAngle = static_cast<float>((static_cast<double>(fPreviousAngle) + fNextAngle) * 0.5 + c_fHalfPi);
        }
        float fSine = static_cast<float>(std::sin(static_cast<double>(fAngle)));
        float fCosine = static_cast<float>(std::cos(static_cast<double>(fAngle)));
        CVec3 cNormal;
        cNormal.m_afData[0] = 0.0f;
        cNormal.m_afData[1] = fCosine;
        cNormal.m_afData[2] = fSine;
        cNormal.Normalize();
        vNormals.push_back(cNormal);
    }
}

// The strip length is truncated to 16 bits before the allocation.
void CFrondEngine::ComputeBlade(uint32_t nLod, uint32_t nStart, SFrondGuide& sGuide) {
    if (m_pGeometry) {
        for (uint32_t i = 0; i < m_nNumBlades; ++i) {
            uint16_t usIndexCount = static_cast<uint16_t>(sGuide.m_vVertices.size() * 2);
            uint16_t* pStrip = static_cast<uint16_t*>(ourMalloc(sizeof(uint16_t) * usIndexCount));
            int32_t nVertexIndex = 0;
            for (uint32_t k = 0; k < sGuide.m_vVertices.size() * 2; ++k) {
                pStrip[nVertexIndex++] = static_cast<uint16_t>(i * 2 * sGuide.m_vVertices.size() + nStart + k);
            }
            m_pGeometry->AddStrip(static_cast<uint16_t>(nLod), pStrip, usIndexCount);
            ++m_pGeometry->m_nStripCounter;
        }
    }
}

void CFrondEngine::BuildExtrusionVertices(SFrondGuide& sGuide) {
    CVec3 v14;
    CVec3 v15;
    CVec3 apResult;
    CRotTransform v33;
    CVec3 cVec;
    CRotTransform cTransform;
    CVec3 v38;
    CVec3 v43;
    std::vector<std::vector<float, std::allocator<float>>, std::allocator<std::vector<float, std::allocator<float>>>>
        v50;
    if ((m_pGeometry) && (m_pLightingEngine)) {
        CVec3Vector vProfile;
        CVec3Vector vNormals;
        BuildProfileVectors(sGuide, vProfile, vNormals);
        sGuide.m_nVerticesPerGuideVertex = static_cast<uint32_t>(vProfile.size());
        sGuide.m_nVertexStartIndex = m_pGeometry->m_nVertexCounter;
        for (uint32_t _Pos = 0; _Pos < sGuide.m_vVertices.size(); ++_Pos) {
            float v47 = static_cast<double>(_Pos) /
                (static_cast<double>(static_cast<uint32_t>(sGuide.m_vVertices.size())) - 1.0);
            for (uint32_t i = 0; i < sGuide.m_nVerticesPerGuideVertex; ++i) {
                CRotTransform* uExitCode;
                float pTexCoord[2];
                float pColor[4];
                float v44 = static_cast<double>(i) / (static_cast<double>(sGuide.m_nVerticesPerGuideVertex) - 1.0);
                cTransform = sGuide.m_vVertices[_Pos].m_cTrans;
                if (_Pos) {
                    uExitCode = &sGuide.m_vVertices[_Pos - 1].m_cTrans;
                }
                else {
                    uExitCode = &cTransform;
                }
                v33 = *uExitCode;
                cTransform.RotateX(sGuide.m_fOffsetAngle);
                v33.RotateX(sGuide.m_fOffsetAngle);
                CVec3 hProcess = vProfile[i] * cTransform;
                CVec3 apOther = vProfile[i] * v33;
                SFrondVertex* v11 = &sGuide.m_vVertices[_Pos];
                CVec3* v2 = hProcess.Add(&apResult, &apOther);
                CVec3* v3 = v2->Scale(&v15, 0.5f);
                v11->m_cPos.Add(&v38, v3);
                hProcess.Add(&cVec, &apOther);
                cVec.Normalize();
                m_pGeometry->AddVertexCoord(v38.m_afData);
                pTexCoord[0] = v44;
                pTexCoord[1] = v47;
                m_pGeometry->AddVertexTexCoord0(pTexCoord, sGuide.m_chFrondMapIndex);
                CVec3 out = vNormals[i] * cTransform;
                CVec3 v41 = vNormals[i] * v33;
                CVec3* v4 = out.Add(&v14, &v41);
                v4->Scale(&v43, 0.5f);
                v43.Normalize();
                pColor[3] = 1.0f;
                pColor[2] = 1.0f;
                pColor[1] = 1.0f;
                pColor[0] = 1.0f;
                m_pGeometry->AddVertexColor(pColor);
                m_pGeometry->AddVertexNormal(v43.m_afData);
                m_pGeometry->AddVertexTangent(cVec.m_afData);
                CVec3 result = v43.Cross(cVec);
                result.Normalize();
                m_pGeometry->AddVertexBinormal(result.m_afData);
                if (m_pGeometry->m_bVertexWeighting) {
                    uint8_t fAngle = sGuide.m_vVertices[_Pos].m_nWindGroup;
                    float fWindWeight = sGuide.m_vVertices[_Pos].m_fCrossSectionWeight;
                    m_pGeometry->AddVertexWind(fWindWeight, fAngle);
                }
                ++m_pGeometry->m_nVertexCounter;
            }
        }
        using apMemory_t = std::vector<float, std::allocator<float>>;
        apMemory_t apMemory;
        ::new (&v50) std::vector<CVec, std::allocator<CVec>>;
        for (uint32_t j = 0; j < sGuide.m_nVerticesPerGuideVertex; ++j) {
            float v31 = 0.0f;
            using v30_t = std::vector<float, std::allocator<float>>;
            v30_t v30;
            float _Val = 0.0f;
            v30.push_back(_Val);
            for (uint32_t k = 1; k < sGuide.m_vVertices.size(); ++k) {
                int32_t nVertexIndex = j + (sGuide.m_nVerticesPerGuideVertex * k + sGuide.m_nVertexStartIndex);
                uint32_t v26 = j + (sGuide.m_nVerticesPerGuideVertex * (k - 1) + sGuide.m_nVertexStartIndex);
                const float* VertexCoord = m_pGeometry->GetVertexCoord(nVertexIndex);
                const float* v24 = m_pGeometry->GetVertexCoord(v26);
                CVec3 v27(*VertexCoord, VertexCoord[1], VertexCoord[2]);
                CVec3 a2(*v24, v24[1], v24[2]);
                double v5 = v27.Distance(a2);
                v31 = v5 + v31;
                v30.push_back(v31);
            }
            v50.push_back(v30);
            apMemory.push_back(v31);
        }
        for (size_t m = 0; m < sGuide.m_nVerticesPerGuideVertex; ++m) {
            for (size_t n = 1; n < sGuide.m_vVertices.size(); ++n) {
                float v19[2];
                uint16_t v18 = m +
                    (static_cast<uint16_t>(sGuide.m_nVerticesPerGuideVertex) * n +
                     static_cast<uint16_t>(sGuide.m_nVertexStartIndex));
                m_pGeometry->m_nVertexCounter = v18;
                const float* VertexTexCoord0 = m_pGeometry->GetVertexTexCoord0(v18);
                v19[0] = *VertexTexCoord0;
                float* v6 = &v50[m][n];
                v19[1] = *v6 / apMemory[m] * 0.999f;
                m_pGeometry->AddVertexTexCoord0(v19, sGuide.m_chFrondMapIndex);
                ++m_pGeometry->m_nVertexCounter;
            }
        }
        using v50_t = std::vector<
            std::vector<float, std::allocator<float>>, std::allocator<std::vector<float, std::allocator<float>>>>;
    }
}

// The guides' total and largest surface area; the guides above the large-retention share of the largest move to a side
// vector (erased from m_vGuides), the rest get a fuzzy surface area; both sets are sorted by it and the large ones
// inserted back at the front one by one; then each LOD level takes the guides up to its share of the total.
void CFrondEngine::BuildGuideLods() {
    float fTotalArea = 0.0f;
    float fMaxArea = 0.0f;
    for (uint32_t i = 0; i < m_vGuides.size(); ++i) {
        fTotalArea = fTotalArea + m_vGuides[i].m_fSurfaceArea;
        if (fMaxArea < static_cast<double>(m_vGuides[i].m_fSurfaceArea)) {
            fMaxArea = m_vGuides[i].m_fSurfaceArea;
        }
    }
    float fMaxAreaSquared = fMaxArea * fMaxArea;
    CIdvRandom cRandom;
    float fSmallShare = 1.0 - m_fLargeRetentionPercent;
    SFrondGuideVector vLargeGuides;
    for (ptrdiff_t _Off = 0; static_cast<size_t>(_Off) < m_vGuides.size(); ++_Off) {
        float fArea = m_vGuides[static_cast<size_t>(_Off)].m_fSurfaceArea;
        if (!(fArea > fMaxArea * fSmallShare)) {
            float fUniform = cRandom.GetUniform(0.0f, m_fReductionFuzziness);
            float fWeight = 1.0 - fUniform;
            fArea = fWeight * fArea + fUniform * fMaxAreaSquared;
            m_vGuides[static_cast<size_t>(_Off)].m_fFuzzySurfaceArea = fArea;
        }
        else {
            vLargeGuides.push_back(m_vGuides[static_cast<size_t>(_Off)]);
            SFrondGuideVector::iterator _Where = m_vGuides.begin();
            _Where += _Off;
            m_vGuides.erase(_Where);
            --_Off;
        }
    }
    CGuideSorter cSorter;
    cSorter.unk0 = 0;
    std::sort(m_vGuides.begin(), m_vGuides.end(), cSorter);
    // The same sort of the large guides.
    CGuideSorter cLargeSorter;
    cLargeSorter.unk0 = 0;
    std::sort(vLargeGuides.begin(), vLargeGuides.end(), cLargeSorter);
    for (uint32_t j = 0; j < vLargeGuides.size(); ++j) {
        SFrondGuideVector::iterator _Where = m_vGuides.begin();
        m_vGuides.insert(_Where, vLargeGuides[j]);
    }
    for (uint32_t k = 0; k < m_nNumLodLevels; ++k) {
        float fPercent;
        if (m_nNumLodLevels == 1) {
            fPercent = m_fMaxSurfaceAreaPercent;
        }
        else {
            float fT = static_cast<double>(k) / static_cast<double>(m_nNumLodLevels - 1);
            fPercent = fT * (m_fMinSurfaceAreaPercent - m_fMaxSurfaceAreaPercent) + m_fMaxSurfaceAreaPercent;
        }
        float fLodArea = fPercent * fTotalArea;
        float fArea = 0.0;
        SFrondGuideVector vLod;
        if (fLodArea > 0.0) {
            for (uint32_t m = 0; m < m_vGuides.size() && fLodArea >= static_cast<double>(fArea); ++m) {
                fArea = fArea + m_vGuides[m].m_fSurfaceArea;
                vLod.push_back(m_vGuides[m]);
            }
        }
        m_vGuideLods.push_back(vLod);
    }
}

// Every unsigned count is converted to floating point as unsigned; the float intermediates are float locals, as the
// game stores them. The vertex counts are size() (a signed division); the length clamp compares it minus one signed.
// The strip allocation is new uint16_t[] (its byte count saturates on overflow).
void CFrondEngine::ComputeExtrusion(uint32_t nLod, uint32_t nStart, SFrondGuide& sGuide) {
    if (m_pGeometry) {
        float fLodPercent;
        if (m_nNumLodLevels == 1) {
            fLodPercent = 1.0f;
        }
        else {
            fLodPercent = 1.0 - static_cast<double>(nLod) / (static_cast<double>(m_nNumLodLevels) - 1.0);
        }
        uint32_t uiNumGuideVertices = static_cast<uint32_t>(sGuide.m_vVertices.size());
        float fMinLength = static_cast<float>(m_nMinLengthSegments + 1);
        float fNumLength = fLodPercent * (static_cast<double>(uiNumGuideVertices) - fMinLength) + fMinLength;
        int32_t nNumLengthVertices = static_cast<int32_t>(fNumLength);
        float fLengthStep = static_cast<double>(static_cast<uint32_t>(sGuide.m_vVertices.size())) /
            static_cast<double>(nNumLengthVertices - 1);
        float fMinCross = static_cast<float>(2 * m_nMinCrossSegments + 1);
        float fNumCross = fLodPercent * (static_cast<double>(sGuide.m_nVerticesPerGuideVertex) - fMinCross) + fMinCross;
        int32_t nNumCrossVertices = static_cast<int32_t>(fNumCross);
        float fCrossStep =
            static_cast<double>(sGuide.m_nVerticesPerGuideVertex) / static_cast<double>(nNumCrossVertices - 1);
        int32_t nNumLengthSegments = nNumLengthVertices - 1;
        int32_t nStripLength = (2 * nNumCrossVertices + 1) * nNumLengthSegments - 1;
        uint64_t ullStripBytes = static_cast<uint64_t>(static_cast<uint32_t>(nStripLength)) * sizeof(uint16_t);
        uint16_t* pStrip = static_cast<uint16_t*>(
            ourMalloc((ullStripBytes >> 32) != 0 ? 0xFFFFFFFFu : static_cast<uint32_t>(ullStripBytes))
        );
        int32_t nVertexIndex = 0;
        for (int32_t i = 0; i < nNumLengthVertices - 1; ++i) {
            int32_t nThisLength = static_cast<int32_t>(static_cast<double>(i) * fLengthStep);
            int32_t nNextLength = static_cast<int32_t>(static_cast<double>(i + 1) * fLengthStep);
            if (i == nNumLengthVertices - 2 || nNextLength > static_cast<int32_t>(sGuide.m_vVertices.size()) - 1) {
                nNextLength = static_cast<int32_t>(sGuide.m_vVertices.size()) - 1;
            }
            int32_t nLengthSpan = nNextLength - nThisLength;
            if (i % 2 == 0) {
                int32_t nBase = nThisLength * sGuide.m_nVerticesPerGuideVertex;
                for (int32_t j = 0; j < nNumCrossVertices; ++j) {
                    uint32_t nThisCrossVertex =
                        static_cast<uint32_t>(static_cast<int64_t>(static_cast<double>(j) * fCrossStep));
                    if (j == nNumCrossVertices - 1) {
                        nThisCrossVertex = sGuide.m_nVerticesPerGuideVertex - 1;
                    }
                    if (nThisCrossVertex > sGuide.m_nVerticesPerGuideVertex - 1) {
                        nThisCrossVertex = sGuide.m_nVerticesPerGuideVertex - 1;
                    }
                    pStrip[nVertexIndex++] = static_cast<uint16_t>(nStart + nBase + nThisCrossVertex);
                    pStrip[nVertexIndex++] = static_cast<uint16_t>(
                        nStart + nBase + nThisCrossVertex + nLengthSpan * sGuide.m_nVerticesPerGuideVertex
                    );
                }
                if (i < nNumLengthVertices - 2) {
                    pStrip[nVertexIndex] = pStrip[nVertexIndex - 1];
                    ++nVertexIndex;
                }
            }
            else {
                int32_t nBase = nThisLength * sGuide.m_nVerticesPerGuideVertex;
                for (int32_t j = nNumCrossVertices - 1; j >= 0; --j) {
                    uint32_t nThisCrossVertex =
                        static_cast<uint32_t>(static_cast<int64_t>(static_cast<double>(j) * fCrossStep));
                    if (j == nNumCrossVertices - 1) {
                        nThisCrossVertex = sGuide.m_nVerticesPerGuideVertex - 1;
                    }
                    if (nThisCrossVertex > sGuide.m_nVerticesPerGuideVertex - 1) {
                        nThisCrossVertex = sGuide.m_nVerticesPerGuideVertex - 1;
                    }
                    pStrip[nVertexIndex++] = static_cast<uint16_t>(nStart + nBase + nThisCrossVertex);
                    pStrip[nVertexIndex++] = static_cast<uint16_t>(
                        nStart + nBase + nThisCrossVertex + nLengthSpan * sGuide.m_nVerticesPerGuideVertex
                    );
                }
                if (i < nNumLengthVertices - 2) {
                    pStrip[nVertexIndex] = pStrip[nVertexIndex - 1];
                    ++nVertexIndex;
                }
            }
        }
        m_pGeometry->AddStrip(static_cast<uint16_t>(nLod), pStrip, static_cast<uint16_t>(nStripLength));
        ++m_pGeometry->m_nStripCounter;
    }
}

// `new CIdvBezierSpline(ParseString())`: an ourMalloc of the spline, then, when it succeeded, the parsed text as a
// temporary string, the spline constructed from it, and the temporary destroyed. Only the branch where the allocation
// succeeded builds the temporary, so it is freed there.
CIdvBezierSpline* CTreeFileAccess::ParseBranchParameter() {
    void* pMemory = ourMalloc(sizeof(CIdvBezierSpline));
    CIdvBezierSpline* pSpline;
    if (pMemory != nullptr) {
        std::string strText = ParseString();
        pSpline = ::new (pMemory) CIdvBezierSpline(strText);
    }
    else {
        pSpline = nullptr;
    }
    return pSpline;
}

CIdvBezierSpline& CIdvBezierSpline::operator=(const CIdvBezierSpline& arSpline) {
    m_fMin = arSpline.m_fMin;
    m_fMax = arSpline.m_fMax;
    m_fVariance = arSpline.m_fVariance;
    m_vControlPoints = arSpline.m_vControlPoints;
    m_vControlPointTangents = arSpline.m_vControlPointTangents;
    m_vControlPointTangentLengths = arSpline.m_vControlPointTangentLengths;
    m_vEvenlySpacedPoints = arSpline.m_vEvenlySpacedPoints;
    m_vSplinePoints = arSpline.m_vSplinePoints;
    return *this;
}

// Sums the squares in double precision and stores the sum, the root and its inverse as floats; the root is taken as a
// double sqrt.
void CVec3::Normalize() {
    float fLengthSquared = float(
        double(m_afData[0]) * m_afData[0] + double(m_afData[1]) * m_afData[1] + double(m_afData[2]) * m_afData[2]
    );
    float fLength = float(std::sqrt(static_cast<double>(fLengthSquared)));
    float fInvLength = 1.0f / fLength;
    m_afData[0] = m_afData[0] * fInvLength;
    m_afData[1] = m_afData[1] * fInvLength;
    m_afData[2] = m_afData[2] * fInvLength;
}

float CVec3::Magnitude() const {
    int32_t v3;
    *reinterpret_cast<float*>(&v3) =
        m_afData[2] * m_afData[2] + (m_afData[1] * m_afData[1] + m_afData[0] * m_afData[0]);
    return std::bit_cast<float>(((v3 >> 1) + 0x1FC00000));
}
