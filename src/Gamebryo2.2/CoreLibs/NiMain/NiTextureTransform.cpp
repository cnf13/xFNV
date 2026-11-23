#include "Gamebryo2.2/CoreLibs/NiMain/NiTextureTransform.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"

const NiTextureTransform NiTextureTransform::IDENTITY(
    NiPoint2(0.0f, 0.0f),
    0.0f,
    NiPoint2(1.0f, 1.0f),
    NiPoint2(0.5f, 0.5f),
    NiTextureTransform::MAYA_TRANSFORM
);

// The matrix is left unset and marked for rebuilding.
NiTextureTransform::NiTextureTransform()
  : m_kTranslate(NiPoint2::ZERO), m_fRotate(0.0f), m_kScale(1.0f, 1.0f), m_kCenter(0.5f, 0.5f), m_bMatrixDirty(true),
    m_eMethod(MAYA_TRANSFORM) {}

NiTextureTransform::NiTextureTransform(
    const NiPoint2& kTranslate,
    float fRotate,
    const NiPoint2& kScale,
    const NiPoint2& kCenter,
    TransformMethod eMethod
)
  : m_kTranslate(kTranslate), m_fRotate(fRotate), m_kScale(kScale), m_kCenter(kCenter), m_eMethod(eMethod) {
    UpdateMatrix();
}

// Read each serialized part in stream order and leave the matrix marked for rebuilding.
// KEEP: the method is read into the stack slot the prologue filled with `this`, so a short read stores the object's
// address into m_eMethod; src cannot reproduce that slot without seeding the local from the pointer.
void NiTextureTransform::LoadBinary(NiStream& kStream) {
    m_kTranslate.LoadBinary(kStream);
    m_kScale.LoadBinary(kStream);
    uint32_t uiComponentSize = 4;
    kStream.m_pkIstr->BinaryRead(&m_fRotate, 4, &uiComponentSize, 1);
    TransformMethod eMethod;
    uiComponentSize = 4;
    kStream.m_pkIstr->BinaryRead(&eMethod, 4, &uiComponentSize, 1);
    m_eMethod = eMethod;
    m_kCenter.LoadBinary(kStream);
    m_bMatrixDirty = true;
}

void NiTextureTransform::SaveBinary(NiStream& kStream) {
    uint32_t v8;
    NiStream* v2 = &kStream;
    m_kTranslate.SaveBinary(kStream);
    m_kScale.SaveBinary(*v2);
    NiBinaryStream* m_pkOstr = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN m_pfnWrite = m_pkOstr->m_pfnWrite;
    v8 = 4;
    m_pkOstr->m_uiAbsoluteCurrentPos += m_pfnWrite(m_pkOstr, &m_fRotate, 4, &v8, 1);
    NiBinaryStream* v6 = v2->Ostr();
    NiBinaryStream::NIBINARYSTREAMWRITEFN v7 = v6->m_pfnWrite;
    v8 = 4;
    v6->m_uiAbsoluteCurrentPos += v7(v6, &m_eMethod, 4, &v8, 1);
    m_kCenter.SaveBinary(*v2);
}

bool NiTextureTransform::operator!=(const NiTextureTransform& kXform) const {
    return kXform.m_fRotate != m_fRotate ||
        kXform.m_kTranslate.x != m_kTranslate.x ||
        kXform.m_kTranslate.y != m_kTranslate.y ||
        m_kScale != kXform.m_kScale ||
        m_eMethod != kXform.m_eMethod ||
        m_kCenter != kXform.m_kCenter;
}

void NiTextureTransform::SetTranslate(const NiPoint2& kTranslate) {
    if ((!m_bMatrixDirty) && ((kTranslate.x == m_kTranslate.x) && (kTranslate.y == m_kTranslate.y))) {
        m_bMatrixDirty = false;
        m_kTranslate = kTranslate;
    }
    else {
        m_bMatrixDirty = true;
        m_kTranslate = kTranslate;
    }
}

void NiTextureTransform::SetRotate(float fRotate) {
    if (m_bMatrixDirty || (fRotate != m_fRotate)) {
        m_fRotate = fRotate;
        m_bMatrixDirty = true;
    }
    else {
        m_fRotate = fRotate;
        m_bMatrixDirty = false;
    }
}

void NiTextureTransform::SetScale(const NiPoint2& kScale) {
    if ((!m_bMatrixDirty) && ((kScale.x == m_kScale.x) && (kScale.y == m_kScale.y))) {
        m_bMatrixDirty = false;
        m_kScale = kScale;
    }
    else {
        m_bMatrixDirty = true;
        m_kScale = kScale;
    }
}

void NiTextureTransform::UpdateMatrix() const {
    int32_t v1 = static_cast<int32_t>(512.0 / NI_TWO_PI * m_fRotate) & 511;
    float v21 = s_fSinTableA[v1];
    double v2 = s_fCosTableA[v1];
    float v27 = v2;
    if (m_eMethod == NiTextureTransform::MAYA_DEPRECATED_TRANSFORM) {
        double v17 = v21;
        double v18 = v27;
        float v26 = -v17;
        double v19 = m_kTranslate.y - m_kCenter.y;
        m_kMatrix.SetCol(0, m_kScale.x * v27, m_kScale.x * v21, 0.0f);
        m_kMatrix.SetCol(1, v26 * m_kScale.y, m_kScale.y * v27, 0.0f);
        m_kMatrix.SetCol(
            2,
            (m_kTranslate.x - m_kCenter.x) * v18 + v26 * v19 + m_kCenter.x,
            v18 * v19 + v17 * (m_kTranslate.x - m_kCenter.x) + m_kCenter.y,
            1.0f
        );
    }
    else if (m_eMethod == NiTextureTransform::MAX_TRANSFORM) {
        double v12 = v21;
        double v13 = v27;
        double v14 = m_kTranslate.y - m_kCenter.y;
        double v15 = -m_kCenter.x - m_kTranslate.x;
        m_kMatrix.SetCol(0, m_kScale.x * v27, -v21 * m_kScale.y, 0.0f);
        m_kMatrix.SetCol(1, m_kScale.x * v12, m_kScale.y * v27, 0.0f);
        m_kMatrix.SetCol(
            2,
            (v12 * v14 + v13 * v15) * m_kScale.x + m_kCenter.x,
            (v13 * v14 - v12 * v15) * m_kScale.y + m_kCenter.y,
            1.0f
        );
    }
    else if (m_eMethod == NiTextureTransform::MAYA_TRANSFORM) {
        double v5 = v21;
        float v22 = -v21;
        double v6 = v22;
        double v7 = v27;
        double v9 = 1.0 - (m_kTranslate.y + m_kCenter.y);
        m_kMatrix.SetCol(0, m_kScale.x * v27, v22 * m_kScale.x, 0.0f);
        m_kMatrix.SetCol(1, v6 * m_kScale.y, -v27 * m_kScale.y, 0.0f);
        m_kMatrix.SetCol(
            2,
            v5 * v9 + v7 * (m_kTranslate.x - m_kCenter.x) + m_kCenter.x,
            (m_kCenter.x - m_kTranslate.x) * v5 + v7 * v9 + m_kCenter.y,
            1.0f
        );
    }
    m_bMatrixDirty = false;
}
