#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/Util/hkMathUtil.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "SDK/MSVC/string.h"

#include <bit>
#include <xmmintrin.h>

hkQsTransform hkQsTransformIdentity = {hkVector4(0.0f, 0.0f, 0.0f, 0.0f),
	hkQuaternion(hkVector4(0.0f, 0.0f, 0.0f, 1.0f)), hkVector4(1.0f, 1.0f, 1.0f, 0.0f)};

// The exe's out-of-line copy assignment: the translation, rotation and scale, three 16-byte copies, and this in eax.
// The same address serves hkRotation::operator= (hkRotation is 0x30 bytes with the identical implicit body, and
// hkDataObjectUtil's TYPE_ROTATION and TYPE_QSTRANSFORM branches both call 0xC6CD30): the linker folded the two.
hkQsTransform& hkQsTransform::operator=(const hkQsTransform& other) {
	m_translation = other.m_translation;
	m_rotation = other.m_rotation;
	m_scale = other.m_scale;
	return *this;
}

void hkQsTransform::setFromTransformNoScale(const hkTransform& transform) {
	m_rotation.set(transform.m_rotation);
	m_translation = transform.m_translation;
	m_scale.m_quad = hkQuadReal1111;
}

void hkQsTransform::setFromTransform(const hkTransform& transform) {
	hkBool v2 = setFromTransformChecked(transform);
}

// The exe returns the hkBool through the caller's slot; the rest of the decomposition (skew and basis) is not read.
hkBool hkQsTransform::setFromTransformChecked(const hkTransform& arTransform) {
	hkMathUtil::Decomposition kDecomposition;
	hkMathUtil::decompose4x4ColTransform(reinterpret_cast<const float*>(&arTransform), kDecomposition);
	m_translation = kDecomposition.m_translation;
	m_rotation = kDecomposition.m_rotation;
	m_scale = kDecomposition.m_scale;
	hkBool kResult;
	kResult.m_bool = kDecomposition.m_hasSkew.m_bool == 0;
	return kResult;
}

// The exe's body is a jump to the out-of-line copy.
void hkQsTransform::copyToTransform(hkTransform& transformOut) const {
	get4x4ColumnMajor(reinterpret_cast<float*>(&transformOut));
}

// The SDK's copyToTransformNoScale.
void hkQsTransform::copyToTransformNoScale(hkTransform& transformOut) const {
	transformOut.m_rotation.set(m_rotation);
	transformOut.m_translation.m_quad = m_translation.m_quad;
}

void hkQsTransform::get4x4ColumnMajor(float* d) const {
	hkMatrix3 arB;
	hkRotation v5;
	hkTransform v6;
	hkRotation arA;
	arA.set(m_rotation);
	double v3 = m_scale.m_quad[0u];
	std::memset(&arB, 0, sizeof(arB));
	arB.m_col0.m_quad[0] = v3;
	arB.m_col1.m_quad[1] = m_scale.m_quad[1];
	arB.m_col2.m_quad[2] = m_scale.m_quad[2];
	v5.setMul(arA, arB);
	v6.m_rotation = v5;
	v6.m_translation = m_translation;
	v6.get4x4ColumnMajorQuads(reinterpret_cast<__m128*>(d));
}

// The SDK's: the twelve floats from the translation on, all but the translation's and the scale's w, each tested
// for a finite exponent. The exe copies each float through the x87 stack before testing its bits, which keeps the
// exponent.
hkBool* hkQsTransform::isOk(hkBool* apResult) const {
	const float* pValues = &m_translation(0);
	for (int32_t i = 0; i < 12; ++i) {
		// EXE-INLINED none: hkMath::isFinite
		if (i != 3 && i != 11 && (std::bit_cast<uint32_t>(pValues[i]) & 0x7F800000u) == 0x7F800000u) {
			apResult->m_bool = 0;
			return apResult;
		}
	}
	apResult->m_bool = 1;
	return apResult;
}

bool* hkQsTransform::isApproximatelyEqual(bool* a1, __m128* a2, uint32_t a3) {
	__m128 v4 = _mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(a3)), _mm_set_ss(std::bit_cast<float>(a3)), 0);
	if ((_mm_movemask_ps(_mm_cmple_ps(_mm_and_ps(_mm_sub_ps(m_translation.m_quad, *a2), hkVector4_setAbs4_absMask),
			 v4)) &
			7) == 7 &&
		(_mm_movemask_ps(_mm_cmple_ps(_mm_and_ps(_mm_sub_ps(m_scale.m_quad, a2[2]), hkVector4_setAbs4_absMask), v4)) &
			7) == 7) {
		hkQuaternion v6;
		v6.m_vec.m_quad = m_rotation.m_vec.m_quad;
		__m128 v7 = a2[1];
		__m128 v8 = _mm_mul_ps(v6.m_vec.m_quad, v7);
		__m128 v9 = _mm_add_ps(_mm_shuffle_ps(v8, v8, 78), v8);
		if (static_cast<float>(_mm_shuffle_ps(v9, v9, 177)[0] + v9[0]) < 0.0f) {
			v6.m_vec.m_quad = _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(std::bit_cast<float>(0xBF800000u)),
											 _mm_set_ss(std::bit_cast<float>(0xBF800000u)), 0),
				v6.m_vec.m_quad);
		}
		*a1 =
			(_mm_movemask_ps(_mm_cmple_ps(_mm_and_ps(_mm_sub_ps(v6.m_vec.m_quad, v7), hkVector4_setAbs4_absMask), v4)) &
				15) == 15;
		return a1;
	} else {
		*a1 = 0;
		return a1;
	}
}

// Copies the scale of each of the aiCount transforms at apSrc into the transforms at apDst. The exe copies one
// 16-byte vector per 0x30-byte element and writes nothing when aiCount is zero or negative (cdecl, 0xCE9840).
void hkQsTransformCopyScales(hkQsTransform* apDst, const hkQsTransform* apSrc, int32_t aiCount) {
	if (aiCount > 0) {
		for (int32_t i = 0; i < aiCount; ++i) {
			apDst[i].m_scale = apSrc[i].m_scale;
		}
	}
}

// Normalizes the rotations of the transforms: four at a time with one rsqrtps refined by a Newton step,
// 0.5 * e * (3 - len2 * e * e), then the rest one by one (the SDK's inline hkQuaternion::normalize).
void normalizeRotations(hkQsTransform* apTransforms, int32_t aiCount) {
	hkQsTransform* pTransform = apTransforms;
	for (uint32_t uiBlocks = aiCount / 4; uiBlocks != 0; --uiBlocks) {
		alignas(16) float afLengthsSquared[4];
		for (int32_t i = 0; i < 4; ++i) {
			const hkVector4& rRotation = pTransform[i].m_rotation.m_vec;
			afLengthsSquared[i] = static_cast<float>(rRotation.dot4(rRotation));
		}
		const hkQuadReal len2 = _mm_load_ps(afLengthsSquared);
		const hkQuadReal approx = _mm_rsqrt_ps(len2);
		const hkQuadReal inverse = _mm_mul_ps(_mm_sub_ps(hkQuadReal3333, _mm_mul_ps(_mm_mul_ps(approx, len2), approx)),
			_mm_mul_ps(hkQuadRealHalf, approx));
		pTransform[0].m_rotation.m_vec.m_quad = _mm_mul_ps(_mm_shuffle_ps(inverse, inverse, _MM_SHUFFLE(0, 0, 0, 0)),
			pTransform[0].m_rotation.m_vec.m_quad);
		pTransform[1].m_rotation.m_vec.m_quad = _mm_mul_ps(_mm_shuffle_ps(inverse, inverse, _MM_SHUFFLE(1, 1, 1, 1)),
			pTransform[1].m_rotation.m_vec.m_quad);
		pTransform[2].m_rotation.m_vec.m_quad = _mm_mul_ps(_mm_shuffle_ps(inverse, inverse, _MM_SHUFFLE(2, 2, 2, 2)),
			pTransform[2].m_rotation.m_vec.m_quad);
		pTransform[3].m_rotation.m_vec.m_quad = _mm_mul_ps(_mm_shuffle_ps(inverse, inverse, _MM_SHUFFLE(3, 3, 3, 3)),
			pTransform[3].m_rotation.m_vec.m_quad);
		pTransform += 4;
	}
	for (uint32_t uiLeft = aiCount % 4; uiLeft != 0; --uiLeft) {
		pTransform->m_rotation.normalize();
		++pTransform;
	}
}

// Divides the translation and scale of each of the auiCount transforms by its weight, then normalizes their
// rotations: four at a time with one rsqrtps refined by a Newton step, 0.5 * e * (3 - len2 * e * e), then the rest
// one by one (the SDK's inline hkQuaternion::normalize).
void blendNormalizeBatch(hkQsTransform* apTransforms, const float* apWeights, uint32_t auiCount) {
	for (uint32_t i = 0; i < auiCount; ++i) {
		const hkQuadReal weight = _mm_load_ss(&apWeights[i]);
		const hkQuadReal inverse = _mm_div_ss(_mm_move_ss(weight, _mm_set_ss(1.0f)), weight);
		const hkQuadReal factor = _mm_shuffle_ps(inverse, inverse, _MM_SHUFFLE(0, 0, 0, 0));
		apTransforms[i].m_translation.m_quad = _mm_mul_ps(factor, apTransforms[i].m_translation.m_quad);
		apTransforms[i].m_scale.m_quad = _mm_mul_ps(factor, apTransforms[i].m_scale.m_quad);
	}
	hkQsTransform* pTransform = apTransforms;
	for (uint32_t uiBlocks = auiCount >> 2; uiBlocks != 0; --uiBlocks) {
		alignas(16) float afLengthsSquared[4];
		for (int32_t i = 0; i < 4; ++i) {
			const hkVector4& rRotation = pTransform[i].m_rotation.m_vec;
			afLengthsSquared[i] = static_cast<float>(rRotation.dot4(rRotation));
		}
		const hkQuadReal len2 = _mm_load_ps(afLengthsSquared);
		const hkQuadReal approx = _mm_rsqrt_ps(len2);
		const hkQuadReal inverse = _mm_mul_ps(_mm_sub_ps(hkQuadReal3333, _mm_mul_ps(_mm_mul_ps(approx, len2), approx)),
			_mm_mul_ps(approx, hkQuadRealHalf));
		pTransform[0].m_rotation.m_vec.m_quad = _mm_mul_ps(_mm_shuffle_ps(inverse, inverse, _MM_SHUFFLE(0, 0, 0, 0)),
			pTransform[0].m_rotation.m_vec.m_quad);
		pTransform[1].m_rotation.m_vec.m_quad = _mm_mul_ps(_mm_shuffle_ps(inverse, inverse, _MM_SHUFFLE(1, 1, 1, 1)),
			pTransform[1].m_rotation.m_vec.m_quad);
		pTransform[2].m_rotation.m_vec.m_quad = _mm_mul_ps(_mm_shuffle_ps(inverse, inverse, _MM_SHUFFLE(2, 2, 2, 2)),
			pTransform[2].m_rotation.m_vec.m_quad);
		pTransform[3].m_rotation.m_vec.m_quad = _mm_mul_ps(_mm_shuffle_ps(inverse, inverse, _MM_SHUFFLE(3, 3, 3, 3)),
			pTransform[3].m_rotation.m_vec.m_quad);
		pTransform += 4;
	}
	for (uint32_t uiLeft = auiCount & 3; uiLeft != 0; --uiLeft) {
		pTransform->m_rotation.normalize();
		++pTransform;
	}
}
