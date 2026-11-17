#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Reconstruction.hpp"

#include <xmmintrin.h>

class hkRotation;

// The Jacobi rotation diagonalizeSymmetric applies: its cosine and sine and the element (p, q) it zeroes. The PDB's
// file-local struct of hkMatrix3.cpp; the exe's _constructSchurMatrix (0xCD5270) writes c, s at 0, 4 and p, q at 8,
// 0xC, which setJacobiRotation (0xCD54D0) reads.
struct SchurMatrix {
	float c;
	float s;
	int32_t p;
	int32_t q;
};

// A three by three matrix stored as three sixteen byte columns.
class hkMatrix3 {
public:
	// The SDK's: its arrays treat it as plain data (hkTypeTraits.h).
	HK_DECLARE_POD_TYPE();

	// The SDK's inline copy assignment (hkMatrix3.inl:70): copies the three columns. The exe inlines it at some
	// sites and calls its out-of-line copy (thiscall, ret 4, 0xC6CD00) at others.
	void operator=(const hkMatrix3& a) {
		m_col0 = a.getColumn(0);
		m_col1 = a.getColumn(1);
		m_col2 = a.getColumn(2);
	}

	// Sets this to the product of arA and arB.
	void setMul(const hkMatrix3& arA, const hkMatrix3& arB);
	// this = this * temp, through setMul into a local.
	void mul(const hkMatrix3& temp);
	// Scales every element by s's first lane. The SDK's name.
	void mul(const hkSimdReal& s);
	// IDA name; not in the PDB. The product helper changeBasis (0xCD5880) calls: writes this
	// from arA's columns and the transpose of arB (setTranspose 0xCD4BA0 into a local); reads arA and arB only
	// (thiscall, ret 8, 0xCD5610).
	// this = a times the inverse (transpose) of the rotation b.
	void setMulInverse(const hkMatrix3& aTb, const hkMatrix3& cTb);
	// The Havok SDK's validity check of the matrix (thiscall, ret 4, hidden result, 0xCD4AF0).
	hkBool isOk() const;
	// Whether every element is within epsilon of m's; the hkBool comes back through a hidden
	// pointer.
	hkBool isApproximatelyEqual(const hkMatrix3& m, float zero) const;
	// Whether the matrix equals its transpose within epsilon.
	bool isSymmetric(float epsilon) const;
	// IDA name _sub_CD51C0; the PDB's hkMatrix3::getDeterminant, matched by what the body
	// computes: col0 . (col1 x col2), in lane 0 of the hkSimdReal that comes back through a hidden pointer
	// (thiscall, ret 4, 0xCD51C0).
	hkSimdReal getDeterminant() const;
	// this += arA, column by column.
	void add(const hkMatrix3& a);
	// this = arR * this * arR^T: a local from 0xCD5610 (this, arR), then setMul(arR, local)
	// (thiscall, ret 4, 0xCD5880).
	void changeBasis(const hkRotation& r);
	// Transposes the matrix in place, swapping the three pairs of off-diagonal elements.
	void transpose();
	// this = the transpose of s (thiscall, ret 4, 0xCD4BA0; matched to the PDB's
	// setTranspose by its body).
	void setTranspose(const hkMatrix3& s);
	// The matrix of the cross product with r: this * v == r x v.
	void setCrossSkewSymmetric(const hkVector4& r);
	// this -= arA, column by column.
	void sub(const hkMatrix3& a);
	// Inverts the matrix in place; hkResult 0, or 1 when the determinant is within epsilon
	// of zero.
	hkResult invert(float epsilon);
	// Havok SDK: the eigenvectors (columns of eigenVec) and eigenvalues (x, y, z of eigenVal, w zero) of the
	// symmetric matrix by Jacobi rotations, at most maxIter of them; hkResult 0, or 1 when the matrix is not
	// symmetric within epsilon or the off-diagonal part did not fall below epsilon times the matrix's norm.
	hkResult diagonalizeSymmetric(hkMatrix3& eigenVec, hkVector4& eigenVal, int32_t maxIter, float epsilon) const;
	// The norm of the off-diagonal part; the result in ST(0) (thiscall, ret 0, 0xCD53A0).
	double _offDiagNorm() const;
	// The SDK's inline column accessor: the three columns are contiguous (pinned at 0x0, 0x10 and 0x20).
	const hkVector4& getColumn(int32_t i) const { return (&m_col0)[i]; }
	// The SDK's writable column accessor; the exe's out-of-line copy is 0x4B4CF0 (folded with
	// NiFrustumPlanes::GetPlane).
	hkVector4& getColumn(int32_t i) { return (&m_col0)[i]; }
	// The SDK's inline element access: component r of column c (getColumn(c)(r)). The exe's out-of-line copy
	// (thiscall, ret 8, 0x5770E0; frame-pointer code, the PDB's copy is in TESObjectREFR.cpp) calls 0x4B4CF0 (the
	// column, folded with NiFrustumPlanes::GetPlane) and hkVector4::operator() (0x560D30).
	float& operator()(int32_t r, int32_t c) { return (&m_col0)[c](r); }
	// Havok SDK signature (hkBase.lib, tied by reloc). this = the transpose of a times b.
	void setMulInverseMul(const hkRotation& bTa, const hkMatrix3& bTc);
	__m128* setMulScalar(__m128* a1, __m128* a2);
	__m128* addMul(__m128* a1, __m128* a2);
	void invertSymmetric();

	// The SDK's generic inline body: every column zero. The exe's out-of-line copy (thiscall, 0x577110) zeroes a
	// vector with setZero4 and copies it into each column through the column accessor 0x4B4CF0 and
	// hkVector4::operator= (0x4A3F10).
	void setZero() {
		hkVector4 zero;
		zero.setZero4();
		(&m_col0)[0] = zero;
		(&m_col0)[1] = zero;
		(&m_col0)[2] = zero;
	}

	// The SDK's inline body: zero, then ones on the diagonal.
	void setIdentity() {
		setZero();
		float one = 1.0f;
		(*this)(0, 0) = one;
		(*this)(1, 1) = one;
		(*this)(2, 2) = one;
	}

	// The transform of an unrotated, unmoved object.
	static const hkMatrix3& getIdentity();
	// The SDK's file-local Jacobi helpers diagonalizeSymmetric calls, under the PDB's names; each takes its arguments
	// in registers, so they go through usercall. PDB: file-static functions of hkMatrix3.cpp.
	// The norm of the whole matrix, apM in eax; the result in ST(0) (0xCD5340).
	static double _frobeniusNorm(const hkMatrix3* apM);
	// The row and column of the largest off-diagonal element of apM (eax) into *apP (esi) and *apQ (edx)
	// (0xCD5400).
	static void _findLargestOffDiagEntry(const hkMatrix3* apM, int32_t* apP, int32_t* apQ);
	// The Jacobi rotation zeroing element (aiP, aiQ) of apM (edi; aiQ in ecx, aiP in edx), into apRotOut (the one
	// stack argument, caller-popped) (0xCD5270).
	static void _constructSchurMatrix(const hkMatrix3* apM, int32_t aiP, int32_t aiQ, SchurMatrix* apRotOut);
	// Not named in the PDB or IDA; name chosen here. The rotation matrix apOut (ecx) of the
	// Jacobi rotation at apRot (eax) (0xCD54D0).
	static void setJacobiRotation(const SchurMatrix* apRot, hkMatrix3* apOut);

	hkVector4 m_col0;
	hkVector4 m_col1;
	hkVector4 m_col2;
};

// The PDB's hkMatrix3_setMulMat3Mat3 (hkMatrix3.cpp): the product apA * apB into apResult (cdecl, 0xCD5530).
void hkMatrix3_setMulMat3Mat3(hkMatrix3* apResult, const hkMatrix3* apA, const hkMatrix3* apB);
// Not named in the PDB or IDA; name chosen here. A cdecl wrapper: loads apM into ecx and jumps to
// hkMatrix3::invertSymmetric (0xCD55A0).
void hkMatrix3_invertSymmetric(hkMatrix3* apM);
