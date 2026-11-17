#pragma once

class CVec3;

class CRotTransform {
public:
	CRotTransform();

	// Defined in Branch.cpp.
	CRotTransform operator*(const CRotTransform& cTrans) const;
	// Multiplies the matrix by a rotation of fAngle degrees about y, from the left.
	void RotateY(float fAngle);
	// Multiplies the matrix by a rotation of fAngle degrees about x, from the left.
	void RotateX(float fAngle);
	// Multiplies the matrix by a rotation of fAngle degrees about cAxis, from the left. Defined in Branch.cpp.
	void RotateAxis(float fAngle, CVec3 cAxis);
	// Multiplies the matrix by rotations of fYangle degrees about y and fZangle about z, from the left.
	// Defined in Branch.cpp.
	void RotateYZ(float fYangle, float fZangle);
	// Sets the matrix to a rotation of fAngle degrees about cAxis. Defined in Branch.cpp.
	void RotateAxisFromIdentity(float fAngle, CVec3 cAxis);
	// cVec3 rotated by the matrix. Defined in Branch.cpp.
	CVec3 operator*(const CVec3& cVec3) const;
	// Multiplies the matrix by a rotation of fAngle degrees about z, from the left.
	void RotateZ(float fAngle);

	float m_afData[3][3];
};
