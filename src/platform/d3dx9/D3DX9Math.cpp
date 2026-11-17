// d3dx9's math functions the game calls, in plain float C++ (row vectors times row-major matrices, as d3dx9). Each
// output may alias an input: results are computed in locals first.

#include "D3DX9Internal.h"

#include <cmath>
#include <cstring>

namespace {

struct M4 {
	float m[4][4];
};

M4 Load(const D3DXMATRIX* apMatrix)
{
	M4 kMatrix;
	std::memcpy(&kMatrix, apMatrix, sizeof(kMatrix));
	return kMatrix;
}

void Store(D3DXMATRIX* apMatrix, const M4& arMatrix)
{
	std::memcpy(apMatrix, &arMatrix, sizeof(arMatrix));
}

// d3dx9_38 sums the four products of rows 0 and 3 pairwise, (p0 + p1) + (p2 + p3), and those of rows 1 and 2 in
// order, ((p0 + p1) + p2) + p3 (measured against the DLL: bit-equal on every element).
M4 Multiply(const M4& arA, const M4& arB)
{
	M4 kOut;
	for (int32_t i = 0; i < 4; ++i) {
		for (int32_t j = 0; j < 4; ++j) {
			const float p0 = arA.m[i][0] * arB.m[0][j];
			const float p1 = arA.m[i][1] * arB.m[1][j];
			const float p2 = arA.m[i][2] * arB.m[2][j];
			const float p3 = arA.m[i][3] * arB.m[3][j];
			kOut.m[i][j] = (i == 0 || i == 3) ? (p0 + p1) + (p2 + p3) : ((p0 + p1) + p2) + p3;
		}
	}
	return kOut;
}

M4 Transpose(const M4& arA)
{
	M4 kOut;
	for (int32_t i = 0; i < 4; ++i) {
		for (int32_t j = 0; j < 4; ++j) {
			kOut.m[i][j] = arA.m[j][i];
		}
	}
	return kOut;
}

// The row vector (x, y, z, w) times the matrix, the products summed pairwise as d3dx9 does.
void Transform(const float* apIn, const M4& arM, float* apOut)
{
	for (int32_t j = 0; j < 4; ++j) {
		apOut[j] = (apIn[0] * arM.m[0][j] + apIn[1] * arM.m[1][j]) + (apIn[2] * arM.m[2][j] + apIn[3] * arM.m[3][j]);
	}
}

// 1 / sqrt(afSquare) correctly rounded to float: d3dx9 scales by a refined reciprocal square root, which this
// matches more often than a float division by the root does.
float InverseLength(float afSquare)
{
	return static_cast<float>(1.0 / std::sqrt(static_cast<double>(afSquare)));
}

// Four floats, as the SSE registers of d3dx9_38's inverse.
struct L4 {
	float v[4];
};

L4 operator*(const L4& a, const L4& b)
{
	return {{a.v[0] * b.v[0], a.v[1] * b.v[1], a.v[2] * b.v[2], a.v[3] * b.v[3]}};
}

L4 operator+(const L4& a, const L4& b)
{
	return {{a.v[0] + b.v[0], a.v[1] + b.v[1], a.v[2] + b.v[2], a.v[3] + b.v[3]}};
}

L4 operator-(const L4& a, const L4& b)
{
	return {{a.v[0] - b.v[0], a.v[1] - b.v[1], a.v[2] - b.v[2], a.v[3] - b.v[3]}};
}

// shufps: two lanes of a, then two of b.
L4 Shuffle(const L4& a, const L4& b, int32_t i0, int32_t i1, int32_t i2, int32_t i3)
{
	return {{a.v[i0], a.v[i1], b.v[i2], b.v[i3]}};
}

// The lane swaps the inverse uses: (1 0 3 2) and (2 3 0 1).
L4 SwapPairs(const L4& a)
{
	return Shuffle(a, a, 1, 0, 3, 2);
}

L4 SwapHalves(const L4& a)
{
	return Shuffle(a, a, 2, 3, 0, 1);
}

} // namespace

extern "C" D3DXMATRIX* WINAPI D3DXMatrixMultiply(D3DXMATRIX* pOut, const D3DXMATRIX* pM1,
	const D3DXMATRIX* pM2)
{
	Store(pOut, Multiply(Load(pM1), Load(pM2)));
	return pOut;
}

extern "C" D3DXMATRIX* WINAPI D3DXMatrixMultiplyTranspose(D3DXMATRIX* pOut, const D3DXMATRIX* pM1,
	const D3DXMATRIX* pM2)
{
	Store(pOut, Transpose(Multiply(Load(pM1), Load(pM2))));
	return pOut;
}

extern "C" D3DXMATRIX* WINAPI D3DXMatrixTranspose(D3DXMATRIX* pOut, const D3DXMATRIX* pM)
{
	Store(pOut, Transpose(Load(pM)));
	return pOut;
}

// d3dx9_38's SSE inverse (Intel's "Streaming SIMD Extensions - Inverse of 4x4 Matrix", the path its CPU dispatch takes
// on SSE machines): the cofactors of the transpose from pair products, in the DLL's order of operations, so each
// element rounds as the DLL's does. The DLL scales by a refined rcpss estimate of 1/det (CPU-dependent bits); this
// divides exactly, the one difference left.
extern "C" D3DXMATRIX* WINAPI D3DXMatrixInverse(D3DXMATRIX* pOut, float* pDeterminant, const D3DXMATRIX* pM)
{
	const M4 a = Load(pM);
	const L4 kLow01 = {{a.m[0][0], a.m[0][1], a.m[1][0], a.m[1][1]}};
	const L4 kLow23 = {{a.m[2][0], a.m[2][1], a.m[3][0], a.m[3][1]}};
	const L4 kHigh01 = {{a.m[0][2], a.m[0][3], a.m[1][2], a.m[1][3]}};
	const L4 kHigh23 = {{a.m[2][2], a.m[2][3], a.m[3][2], a.m[3][3]}};
	// The columns of the matrix, columns 1 and 3 with their halves swapped.
	const L4 row0 = Shuffle(kLow01, kLow23, 0, 2, 0, 2);
	const L4 row1 = Shuffle(kLow23, kLow01, 1, 3, 1, 3);
	L4 row2 = Shuffle(kHigh01, kHigh23, 0, 2, 0, 2);
	const L4 row3 = Shuffle(kHigh23, kHigh01, 1, 3, 1, 3);

	L4 tmp = SwapPairs(row2 * row3);
	L4 minor0 = row1 * tmp;
	L4 minor1 = row0 * tmp;
	tmp = SwapHalves(tmp);
	minor0 = row1 * tmp - minor0;
	minor1 = SwapHalves(row0 * tmp - minor1);

	tmp = SwapPairs(row1 * row2);
	minor0 = row3 * tmp + minor0;
	L4 minor3 = row0 * tmp;
	tmp = SwapHalves(tmp);
	minor0 = minor0 - row3 * tmp;
	minor3 = SwapHalves(row0 * tmp - minor3);

	tmp = SwapPairs(SwapHalves(row1) * row3);
	row2 = SwapHalves(row2);
	minor0 = row2 * tmp + minor0;
	L4 minor2 = row0 * tmp;
	tmp = SwapHalves(tmp);
	minor0 = minor0 - row2 * tmp;
	minor2 = SwapHalves(row0 * tmp - minor2);

	tmp = SwapPairs(row0 * row1);
	minor2 = row3 * tmp + minor2;
	minor3 = row2 * tmp - minor3;
	tmp = SwapHalves(tmp);
	minor2 = row3 * tmp - minor2;
	minor3 = minor3 - row2 * tmp;

	tmp = SwapPairs(row0 * row3);
	minor1 = minor1 - row2 * tmp;
	minor2 = row1 * tmp + minor2;
	tmp = SwapHalves(tmp);
	minor1 = row2 * tmp + minor1;
	minor2 = minor2 - row1 * tmp;

	tmp = SwapPairs(row0 * row2);
	minor1 = row3 * tmp + minor1;
	minor3 = minor3 - row1 * tmp;
	tmp = SwapHalves(tmp);
	minor1 = minor1 - row3 * tmp;
	minor3 = row1 * tmp + minor3;

	const L4 kProducts = row0 * minor0;
	const float fDet = (kProducts.v[1] + kProducts.v[3]) + (kProducts.v[0] + kProducts.v[2]);
	// The DLL's comiss test: a zero or NaN determinant returns nullptr before the determinant is stored.
	if (fDet == 0.0f || std::isnan(fDet)) {
		return nullptr;
	}
	if (pDeterminant) {
		*pDeterminant = fDet;
	}
	const float fInv = 1.0f / fDet;
	const L4 kInv = {{fInv, fInv, fInv, fInv}};
	const L4 akRows[4] = {minor0 * kInv, minor1 * kInv, minor2 * kInv, minor3 * kInv};
	std::memcpy(pOut, akRows, sizeof(akRows));
	return pOut;
}

// Roll about z, then pitch about x, then yaw about y.
extern "C" D3DXMATRIX* WINAPI D3DXMatrixRotationYawPitchRoll(D3DXMATRIX* pOut, float Yaw, float Pitch,
	float Roll)
{
	const float fSinRoll = std::sin(Roll);
	const float fCosRoll = std::cos(Roll);
	const float fSinPitch = std::sin(Pitch);
	const float fCosPitch = std::cos(Pitch);
	const float fSinYaw = std::sin(Yaw);
	const float fCosYaw = std::cos(Yaw);
	M4 b;
	b.m[0][0] = fSinRoll * fSinPitch * fSinYaw + fCosRoll * fCosYaw;
	b.m[0][1] = fSinRoll * fCosPitch;
	b.m[0][2] = fSinRoll * fSinPitch * fCosYaw - fCosRoll * fSinYaw;
	b.m[0][3] = 0.0f;
	b.m[1][0] = fCosRoll * fSinPitch * fSinYaw - fSinRoll * fCosYaw;
	b.m[1][1] = fCosRoll * fCosPitch;
	b.m[1][2] = fCosRoll * fSinPitch * fCosYaw + fSinRoll * fSinYaw;
	b.m[1][3] = 0.0f;
	b.m[2][0] = fCosPitch * fSinYaw;
	b.m[2][1] = -fSinPitch;
	b.m[2][2] = fCosPitch * fCosYaw;
	b.m[2][3] = 0.0f;
	b.m[3][0] = 0.0f;
	b.m[3][1] = 0.0f;
	b.m[3][2] = 0.0f;
	b.m[3][3] = 1.0f;
	Store(pOut, b);
	return pOut;
}

// The plane over the length of its normal; a zero normal gives the zero plane.
extern "C" D3DXPLANE* WINAPI D3DXPlaneNormalize(D3DXPLANE* pOut, const D3DXPLANE* pP)
{
	const float a = pP->a, b = pP->b, c = pP->c, d = pP->d;
	const float fSquare = a * a + b * b + c * c;
	if (fSquare == 0.0f) {
		pOut->a = pOut->b = pOut->c = pOut->d = 0.0f;
		return pOut;
	}
	const float fScale = InverseLength(fSquare);
	pOut->a = a * fScale;
	pOut->b = b * fScale;
	pOut->c = c * fScale;
	pOut->d = d * fScale;
	return pOut;
}

// The plane (a, b, c, d) as a row vector times the matrix (the inverse transpose of the points' matrix).
extern "C" D3DXPLANE* WINAPI D3DXPlaneTransform(D3DXPLANE* pOut, const D3DXPLANE* pP, const D3DXMATRIX* pM)
{
	const float afIn[4] = {pP->a, pP->b, pP->c, pP->d};
	float afOut[4];
	Transform(afIn, Load(pM), afOut);
	pOut->a = afOut[0];
	pOut->b = afOut[1];
	pOut->c = afOut[2];
	pOut->d = afOut[3];
	return pOut;
}

extern "C" D3DXVECTOR3* WINAPI D3DXVec3Normalize(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV)
{
	const float x = pV->x, y = pV->y, z = pV->z;
	const float fSquare = x * x + y * y + z * z;
	if (fSquare == 0.0f) {
		pOut->x = pOut->y = pOut->z = 0.0f;
		return pOut;
	}
	const float fScale = InverseLength(fSquare);
	pOut->x = x * fScale;
	pOut->y = y * fScale;
	pOut->z = z * fScale;
	return pOut;
}

// The point (x, y, z, 1) times the matrix, projected back by its w.
extern "C" D3DXVECTOR3* WINAPI D3DXVec3TransformCoord(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV,
	const D3DXMATRIX* pM)
{
	const float afIn[4] = {pV->x, pV->y, pV->z, 1.0f};
	float afOut[4];
	Transform(afIn, Load(pM), afOut);
	pOut->x = afOut[0] / afOut[3];
	pOut->y = afOut[1] / afOut[3];
	pOut->z = afOut[2] / afOut[3];
	return pOut;
}

// The direction (x, y, z) times the matrix's upper 3x3, ((p0 + p1) + p2) as d3dx9 sums it.
extern "C" D3DXVECTOR3* WINAPI D3DXVec3TransformNormal(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV,
	const D3DXMATRIX* pM)
{
	const M4 m = Load(pM);
	const float x = pV->x, y = pV->y, z = pV->z;
	pOut->x = (x * m.m[0][0] + y * m.m[1][0]) + z * m.m[2][0];
	pOut->y = (x * m.m[0][1] + y * m.m[1][1]) + z * m.m[2][1];
	pOut->z = (x * m.m[0][2] + y * m.m[1][2]) + z * m.m[2][2];
	return pOut;
}

extern "C" D3DXVECTOR4* WINAPI D3DXVec4Transform(D3DXVECTOR4* pOut, const D3DXVECTOR4* pV,
	const D3DXMATRIX* pM)
{
	const float afIn[4] = {pV->x, pV->y, pV->z, pV->w};
	float afOut[4];
	Transform(afIn, Load(pM), afOut);
	pOut->x = afOut[0];
	pOut->y = afOut[1];
	pOut->z = afOut[2];
	pOut->w = afOut[3];
	return pOut;
}
