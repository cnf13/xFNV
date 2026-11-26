#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"

class NiStream;

// A 2D texture-coordinate transform; m_kMatrix is rebuilt from the parts when m_bMatrixDirty is set.
class NiTextureTransform : public NiMemObject {
public:
    enum TransformMethod {
        MAYA_DEPRECATED_TRANSFORM = 0,
        MAX_TRANSFORM = 1,
        MAYA_TRANSFORM = 2,
    };

    // Runs the default constructor on apMemory and returns it.
    NiTextureTransform();
    NiTextureTransform(
        const NiPoint2& kTranslate,
        float fRotate,
        const NiPoint2& kScale,
        const NiPoint2& kCenter,
        TransformMethod eMethod
    );

    void LoadBinary(NiStream& kStream);
    void SaveBinary(NiStream& kStream);
    bool operator!=(const NiTextureTransform& kXform) const;
    // The matrix stays clean only when the compare is ordered and equal; a NaN marks it dirty.
    void SetTranslate(const NiPoint2& kTranslate);
    void SetRotate(float fRotate);
    void SetScale(const NiPoint2& kScale);
    // The matrix, rebuilt first when dirty.
    const NiMatrix3* GetMatrix() const;

    void UpdateMatrix() const;

    NiPoint2 m_kTranslate;
    float m_fRotate;
    NiPoint2 m_kScale;
    NiPoint2 m_kCenter;
    // mutable as in Gamebryo: the const GetMatrix and UpdateMatrix rebuild the cached matrix.
    mutable bool m_bMatrixDirty;
    mutable NiMatrix3 m_kMatrix;
    TransformMethod m_eMethod;

    // The identity transform: no translation or rotation, unit scale, centred at (0.5, 0.5), MAYA_TRANSFORM.
    static const NiTextureTransform IDENTITY;
};

inline const NiMatrix3* NiTextureTransform::GetMatrix() const {
    if (m_bMatrixDirty) {
        UpdateMatrix();
    }
    return &m_kMatrix;
}
