#pragma once

#include "Reconstruction.hpp"

class CVec3;

class CRotTransform {
public:
	CRotTransform();

	// The product with cTrans, through a constructed temporary (thiscall, ret 8, the result through a hidden
	// pointer, 0xB12940). Defined in Branch.cpp.
	CRotTransform operator*(const CRotTransform& cTrans) const;
	// Multiplies the matrix by a rotation of fAngle degrees about y, from the left.
	void RotateY(float fAngle);
	// Multiplies the matrix by a rotation of fAngle degrees about x, from the left
	// (thiscall, ret 4, 0xB1AE00; FrondEngine.cpp's instance of the inline member).
	void RotateX(float fAngle);
	// Multiplies the matrix by a rotation of fAngle degrees about cAxis, from the left (thiscall, ret 0x10: cAxis
	// by value, 0xB12D50). Defined in Branch.cpp.
	void RotateAxis(float fAngle, CVec3 cAxis);
	// Multiplies the matrix by rotations of fYangle degrees about y and fZangle about z, from the left.
	// Defined in Branch.cpp.
	void RotateYZ(float fYangle, float fZangle);
	// Sets the matrix to a rotation of fAngle degrees about cAxis (thiscall, ret 0x10: cAxis by value, 0xB12EC0).
	// Defined in Branch.cpp.
	void RotateAxisFromIdentity(float fAngle, CVec3 cAxis);
	// cVec3 rotated by the matrix (thiscall, ret 8, the result through a hidden pointer, 0xB12B50). Defined in
	// Branch.cpp.
	CVec3 operator*(const CVec3& cVec3) const;
	// Multiplies the matrix by a rotation of fAngle degrees about z, from the left
	// (thiscall, ret 4, 0xB273D0; SimpleBillboard.cpp's instance of the inline member).
	void RotateZ(float fAngle);

	float m_afData[3][3];
};
