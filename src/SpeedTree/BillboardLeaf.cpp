#include "SpeedTree/BillboardLeaf.h"

#include "BSCore/MemoryManager.h"
#include "Reconstruction.hpp"
#include "SpeedTree/Camera.h"
#include "SpeedTree/EFileToken.h"
#include "SpeedTree/FileAccess.h"
#include "SpeedTree/LibGlobals_Source/IdvGlobals.h"
#include "SpeedTree/SpeedTreeRT.h"

#include <cmath>
#include <string>

CBillboardLeaf::CBillboardLeaf()
  : m_ucAngleIndex(0), m_lColor(0xFFFFFFFFu), m_ucColorScale(0), m_cNormal(0.0f, 0.0f, 0.0f),
    m_cTangent(0.0f, 0.0f, 0.0f), m_cBinormal(0.0f, 0.0f, 0.0f), m_ucTextureIndex(0), m_fWindWeight(0.0f),
    m_nWindGroup(0) {}

CBillboardLeaf::CBillboardLeaf(
    const CVec3& cPos,
    uint16_t nColorScale,
    int32_t nAngleIndex,
    float fWindWeight,
    int32_t nWindGroup
)
  : m_ucAngleIndex(static_cast<uint8_t>(nAngleIndex)), m_lColor(0xFFFFFFFFu),
    m_ucColorScale(static_cast<uint8_t>(nColorScale)), m_cNormal(0.0f, 0.0f, 0.0f), m_cTangent(0.0f, 0.0f, 0.0f),
    m_cBinormal(0.0f, 0.0f, 0.0f), m_ucTextureIndex(0), m_fWindWeight(fWindWeight), m_nWindGroup(nWindGroup) {
    m_cPosition = cPos;
}

// The leaf has nothing of its own to destroy.
CBillboardLeaf::~CBillboardLeaf() {}

const CBillboardLeaf& CBillboardLeaf::operator=(const CBillboardLeaf& cRight) {
    if (&cRight != this) {
        CIdvCamera::operator=(cRight);
        m_ucAngleIndex = cRight.m_ucAngleIndex;
        m_lColor = cRight.m_lColor;
        m_ucColorScale = cRight.m_ucColorScale;
        m_cNormal = cRight.m_cNormal;
        m_ucTextureIndex = cRight.m_ucTextureIndex;
        m_fWindWeight = cRight.m_fWindWeight;
        m_nWindGroup = cRight.m_nWindGroup;
        m_cTangent = cRight.m_cTangent;
        m_cBinormal = cRight.m_cBinormal;
    }
    return *this;
}

// Each channel is divided by the double 255.0 on the x87 stack and stored as a float local before the copy out.
CVec3 CBillboardLeaf::GetColor() const {
    float fRed = (float)((m_lColor & 0xFF) / 255.0);
    float fGreen = (float)(((m_lColor & 0xFF00) >> 8) / 255.0);
    float fBlue = (float)(((m_lColor & 0xFF0000) >> 16) / 255.0);
    CVec3 cColor;
    cColor.m_afData[0] = fRed;
    cColor.m_afData[1] = fGreen;
    cColor.m_afData[2] = fBlue;
    return cColor;
}

// The angle and its cosine are stored as floats; (cos + 1.0) * 0.5 uses double constants and is stored as a float, then
// blended with fLightScalar and stored as a float again. The colour is GetColor's temporary scaled through CVec3::Scale
// and set with dimming.
void CBillboardLeaf::AdjustStaticLighting(const CVec3& cTreeCenter, const CVec3& cLightPos, float fLightScalar) {
    CVec3 cLeafAngle;
    m_cPosition.Diff(&cLeafAngle, &cTreeCenter);
    cLeafAngle.Normalize();
    CVec3 cLightAngle;
    cLightPos.Diff(&cLightAngle, &cTreeCenter);
    cLightAngle.Normalize();
    float fAngle = cLeafAngle.AngleBetween(cLightAngle);
    float fCosine = (float)std::cos(static_cast<double>(fAngle));
    float fScale = (float)((fCosine + 1.0) * 0.5);
    float fLight = (float)(fScale * (1.0 - (double)fLightScalar) + fLightScalar);
    CVec3 cScaled;
    SetColor(*GetColor().Scale(&cScaled, fLight), true);
}

void CBillboardLeaf::SetColor(const CVec3& cColor, bool bApplyDimming) {
    CVec3 cScaledColor_2 = cColor;
    if (bApplyDimming) {
        float cScaledColor = static_cast<double>(m_ucColorScale) / 255.0;
        cScaledColor_2.operator*=(cScaledColor);
    }
    for (int32_t i = 0; i < 3; ++i) {
        if (cScaledColor_2.m_afData[i] > 1.0) {
            cScaledColor_2.m_afData[i] = 1.0f;
        }
        if (cScaledColor_2.m_afData[i] < 0.0) {
            cScaledColor_2.m_afData[i] = 0.0f;
        }
    }
    m_lColor = (static_cast<int32_t>(cScaledColor_2.m_afData[1] * 255.0) << 8) +
        static_cast<int32_t>(cScaledColor_2.m_afData[0] * 255.0) +
        (static_cast<int32_t>(cScaledColor_2.m_afData[2] * 255.0) << 16) - 0x1000000;
}

CBillboardLeaf* CBillboardLeaf::Clone() const {
    CBillboardLeaf* pClone;
    CBillboardLeaf* apMemory = static_cast<CBillboardLeaf*>(ourMalloc(sizeof(CBillboardLeaf)));
    if (apMemory) {
        pClone = ::new (apMemory) CBillboardLeaf;
    }
    else {
        pClone = nullptr;
    }
    pClone->operator=(*this);
    return pClone;
}

// A new leaf, its fields read token by token up to File_EndBillboardLeaf. It throws IdvFileError on an unknown token,
// as SIdvBranchInfo::Parse (BranchInfo.cpp) does. The time offset is read and dropped.
CBillboardLeaf* CBillboardLeaf::Parse(CTreeFileAccess& cFile) {
    void* pMemory = ourMalloc(sizeof(CBillboardLeaf));
    CBillboardLeaf* pLeaf;
    if (pMemory != nullptr) {
        pLeaf = ::new (pMemory) CBillboardLeaf;
    }
    else {
        pLeaf = nullptr;
    }
    int32_t eToken = cFile.ParseToken();
    do {
        switch (eToken) {
            case File_BillboardLeaf_Color: pLeaf->m_lColor = cFile.ParseLongSkipPadding(); break;
            case File_BillboardLeaf_AngleIndex: pLeaf->m_ucAngleIndex = cFile.ParseByte(); break;
            case File_BillboardLeaf_ColorScale: pLeaf->m_ucColorScale = cFile.ParseByte(); break;
            case File_BillboardLeaf_Normal: {
                float afNormal[3];
                float* pNormal = cFile.ParseVector3(afNormal);
                pLeaf->m_cNormal.m_afData[0] = pNormal[0];
                pLeaf->m_cNormal.m_afData[1] = pNormal[1];
                pLeaf->m_cNormal.m_afData[2] = pNormal[2];
                break;
            }
            case File_BillboardLeaf_TextureIndex: pLeaf->m_ucTextureIndex = cFile.ParseByte(); break;
            case File_BillboardLeaf_WindWeight: {
                // The byte is zero-extended and divided by the double 255.0.
                int32_t iWeight = cFile.ParseByte();
                pLeaf->m_fWindWeight = static_cast<double>(iWeight) / 255.0;
                break;
            }
            case File_BillboardLeaf_WindGroup: pLeaf->m_nWindGroup = cFile.ParseInt(); break;
            case File_BillboardLeaf_Pos: {
                float afPosition[3];
                float* pPosition = cFile.ParseVector3(afPosition);
                pLeaf->m_cPosition.m_afData[0] = pPosition[0];
                pLeaf->m_cPosition.m_afData[1] = pPosition[1];
                pLeaf->m_cPosition.m_afData[2] = pPosition[2];
                break;
            }
            case File_BillboardLeaf_TimeOffset: cFile.ParseFloat(); break;
            default: {
                std::string strDetails;
                XFNV_RECONSTRUCT(std::string)(&strDetails, "malformed billboard leaf");
                throw IdvFileError(strDetails, false);
            }
        }
        eToken = cFile.ParseToken();
    }
    while (eToken != File_EndBillboardLeaf);
    return pLeaf;
}
