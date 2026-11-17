#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix4.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

// Havok's matrix helpers.
namespace hkMathUtil {

    // A matrix split into its translation, rotation, scale and skew.
    struct Decomposition {
        hkVector4 m_translation;
        hkQuaternion m_rotation;
        hkVector4 m_scale;
        hkBool m_hasScale;
        hkBool m_hasSkew;
        hkBool m_flips;
        hkMatrix4 m_scaleAndSkew;
        hkMatrix4 m_skew;
        hkRotation m_basis;
    };

    // Splits the 4x4 column-major matrix at matrixIn into decomposition. matrixIn is read only through the float-array
    // setters of hkTransform and hkMatrix4::set4x4ColumnMajor, and callers pass an hkTransform's floats or a matrix's.
    void decompose4x4ColTransform(const float* matrixIn, Decomposition& decompOut);
    // Splits the matrix into decomposition. The hkMatrix4 overload passes the matrix's floats to
    // decompose4x4ColTransform; the hkMatrix3 overload calls it with a matrix built from its input.
    void decomposeMatrix(const hkMatrix4& matrixIn, Decomposition& decomposition);
    // Splits the rotation and scale matrixIn into decomposition, with no translation.
    void decomposeMatrix(const hkMatrix3& matrixIn, Decomposition& decomposition);

} // namespace hkMathUtil
