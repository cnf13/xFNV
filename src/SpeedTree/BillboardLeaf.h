#pragma once

#include "SpeedTree/Camera.h"

class CTreeFileAccess;

// A speed tree leaf billboard: the angle, colour, lighting vectors, texture and wind data of one leaf, derived from the
// camera so that the leaf keeps its position.
class CBillboardLeaf : public CIdvCamera {
public:
    CBillboardLeaf();
    CBillboardLeaf(const CVec3& cPos, uint16_t nColorScale, int32_t nAngleIndex, float fWindWeight, int32_t nWindGroup);
    ~CBillboardLeaf() override;

    // User-declared copy assignment: unless cRight is this leaf, copies the camera part through CIdvCamera::operator=,
    // then the leaf's fields; returns this. It takes a CBillboardLeaf, so it does not override the camera's virtual
    // operator=.
    const CBillboardLeaf& operator=(const CBillboardLeaf& cRight);
    // The leaf's colour as red, green and blue in [0, 1], from the low three bytes of m_lColor.
    CVec3 GetColor() const;
    // Dims the colour by the angle between the leaf and the light, seen from the tree centre.
    void AdjustStaticLighting(const CVec3& cTreeCenter, const CVec3& cLightPos, float fLightScalar);
    // Stores cColor in m_lColor, dimmed when bApplyDimming.
    void SetColor(const CVec3& cColor, bool bApplyDimming);
    // A new leaf with this leaf's data.
    CBillboardLeaf* Clone() const;

    // A new leaf read from cFile token by token up to File_EndBillboardLeaf; throws IdvFileError on an unknown token.
    static CBillboardLeaf* Parse(CTreeFileAccess& cFile);

    uint8_t m_ucAngleIndex;
    uint8_t padding11[3];
    uint32_t m_lColor;
    uint8_t m_ucColorScale;
    uint8_t padding19[3];
    CVec3 m_cNormal;
    CVec3 m_cTangent;
    CVec3 m_cBinormal;
    uint8_t m_ucTextureIndex;
    uint8_t padding41[3];
    float m_fWindWeight;
    int32_t m_nWindGroup;
};
