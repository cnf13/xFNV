#pragma once

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix4.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

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

// Splits the 4x4 column-major matrix at matrixIn into decomposition (cdecl, 0xD72E90). The PDB's
// decompose4x4ColTransform: the exe's body reads matrixIn only through the float-array setters hkTransform's 0xCB20B0
// and hkMatrix4::set4x4ColumnMajor, and its callers pass an hkTransform's floats (0xCE5290) or, through the jump
// 0xD73380, a matrix's.
void decompose4x4ColTransform(const float* matrixIn, Decomposition& decompOut);
// Splits the matrix into decomposition (cdecl, 0xD73380). The PDB's hkMatrix4 overload: the exe's whole body is a
// jump to decompose4x4ColTransform (0xD72E90) with the matrix's floats; the hkMatrix3 overload (0xD73390) calls it
// with a matrix built on its stack.
void decomposeMatrix(const hkMatrix4& matrixIn, Decomposition& decomposition);
// Splits the rotation and scale matrixIn into decomposition, with no translation (cdecl, 0xD73390).
void decomposeMatrix(const hkMatrix3& matrixIn, Decomposition& decomposition);

} // namespace hkMathUtil
