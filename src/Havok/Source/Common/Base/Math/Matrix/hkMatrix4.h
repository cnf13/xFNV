#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Reconstruction.hpp"

class hkTransform;

class hkMatrix4 {
public:
	// The SDK's: its arrays treat it as plain data (hkTypeTraits.h).
	HK_DECLARE_POD_TYPE();
	// Inverts the matrix in place; HK_FAILURE when the determinant is within epsilon of zero.
	hkResult invert(float epsilon);
	// Sets this to the inverse of m; HK_FAILURE when the determinant is within epsilon of zero.
	hkResult setInverse(const hkMatrix4& m, float epsilon);
	// Transposes the matrix in place.
	void transpose();
	// Sets this to a * b.
	void setMul(const hkMatrix4& a, const hkMatrix4& b);
	// Sets this to a * inverse(b).
	void setMulInverse(const hkMatrix4& a, const hkMatrix4& b);
	// Sets each column of this to the matching column of a times scale (the SDK's
	// setMul(hkSimdRealParameter, const hkMatrix4&); thiscall, ret 8, 0xDE88A0: lane 0 of the first argument is
	// broadcast and multiplied into the second's four quads).
	void setMul(const hkSimdReal& scale, const hkMatrix4& a);
	// Multiplies each column by scale (the SDK's mul(hkSimdRealParameter); thiscall, ret 4,
	// 0xDE8D30).
	void mul(const hkSimdReal& scale);

	// Copies the four columns. The exe's out-of-line copy (thiscall, ret 4, 0xDC3710) is registered in hkMatrix4.cpp.
	void operator=(const hkMatrix4& a) {
		m_col0 = a.m_col0;
		m_col1 = a.m_col1;
		m_col2 = a.m_col2;
		m_col3 = a.m_col3;
	}

	hkBool* isOk(hkBool* apResult);
	void setTranspose(const hkMatrix4& s);
	void add(const hkMatrix4& a);
	void sub(const hkMatrix4& a);
	void get4x4RowMajor(float* d);
	void set4x4RowMajor(const float* p);
	// Not reconstructed yet. Copies the 16 floats as they are (thiscall, ret 4, 0xDE9010; Havok SDK symbol,
	// hkBase.lib, tied by bytes).
	void get4x4ColumnMajor(float* d) const;
	// Copies the 16 column-major floats at p into the columns.
	void set4x4ColumnMajor(const float* p);
	// Sets the fourth row to 0, 0, 0, 1.
	void resetFourthRow();
	void set(const hkTransform& t);
	void get(hkTransform& t);
	void setMulInverseAffine(const hkMatrix4& a, const hkMatrix4& b);
	void mul(hkMatrix4* b);

	static int32_t isAffineTransformation(uint32_t* a1);

	hkVector4 m_col0;
	hkVector4 m_col1;
	hkVector4 m_col2;
	hkVector4 m_col3;
};

// Not named in the PDB or IDA; name chosen here. The identity matrix hkMatrix4::getIdentity returns; the exe inlines
// getIdentity as a copy of these four quads.
extern hkMatrix4 hkMatrix4Identity;
// The PDB's global: the identity matrix's sixteen floats (.data), which hkMatrix4Identity's initialiser copies.
using hkMatrix4Identity_storage_t = float[16];
inline hkMatrix4Identity_storage_t hkMatrix4Identity_storage{1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
