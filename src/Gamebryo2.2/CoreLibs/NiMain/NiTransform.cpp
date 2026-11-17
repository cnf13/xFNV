#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"

// The fields are left unset.
NiTransform::NiTransform() {}

void NiTransform::MakeIdentity() {
    m_Rotate.m_pEntry[0][0] = 1.0f;
    m_Rotate.m_pEntry[0][1] = 0.0f;
    m_Rotate.m_pEntry[0][2] = 0.0f;
    m_Rotate.m_pEntry[1][0] = 0.0f;
    m_Rotate.m_pEntry[1][1] = 1.0f;
    m_Rotate.m_pEntry[1][2] = 0.0f;
    m_Rotate.m_pEntry[2][0] = 0.0f;
    m_Rotate.m_pEntry[2][1] = 0.0f;
    m_Rotate.m_pEntry[2][2] = 1.0f;
    m_Translate = NiPoint3_ZERO;
    m_fScale = 1.0f;
}

bool NiTransform::operator!=(const NiTransform& xform) const {
    return !(m_Rotate == xform.m_Rotate) || m_Translate != xform.m_Translate || m_fScale != xform.m_fScale;
}

NiPoint3 NiTransform::operator*(const NiPoint3& arPoint) const {
    NiPoint3 result;
    const NiPoint3* p_m_Translate = &m_Translate;
    float fScale = m_fScale;
    result = m_Rotate * arPoint * fScale + *p_m_Translate;
    return result;
}

NiTransform NiTransform::operator*(const NiTransform& arXform) const {
    NiTransform result;
    NiPoint3 v7;
    NiPoint3 dst;
    NiPoint3 v9;
    NiMatrix3 resulta;
    NiTransform res;
    res.~NiTransform();
    ::new (&res) NiTransform;
    res.m_fScale = m_fScale * arXform.m_fScale;
    res.m_Rotate = (resulta = m_Rotate * arXform.m_Rotate);
    NiPoint3* v5 = &(v9 = m_Rotate * arXform.m_Translate);
    NiPoint3* v3 = &(dst = m_fScale * *v5);
    res.m_Translate = (v7 = m_Translate + *v3);
    result = res;
    return result;
}

// NiTransform::Invert is in Fallout Shared/GlobalFunc.cpp.
void NiTransform::InvertNonUniform(NiTransform& kDest) const {
    NiPoint3 dst;
    NiPoint3 v6;
    NiPoint3 result;
    m_Rotate.Inverse(kDest.m_Rotate);
    kDest.m_fScale = 1.0 / m_fScale;
    NiPoint3* v2 = &(result = -m_Translate);
    NiPoint3* v3 = &(v6 = kDest.m_Rotate * *v2);
    kDest.m_Translate = (dst = kDest.m_fScale * *v3);
}

void NiTransform::LoadBinary(NiStream& stream) {
    m_Rotate.LoadBinary(stream);
    m_Translate.LoadBinary(stream);
    NiStreamLoadBinary(stream, m_fScale);
}

void NiTransform::SaveBinary(NiStream& stream) {
    m_Rotate.SaveBinary(stream);
    m_Translate.SaveBinary(stream);
    NiStreamSaveBinary(stream, m_fScale);
}
