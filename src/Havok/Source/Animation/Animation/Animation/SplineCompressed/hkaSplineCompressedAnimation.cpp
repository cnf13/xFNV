#include "Havok/Source/Animation/Animation/Animation/SplineCompressed/hkaSplineCompressedAnimation.h"

#include "Havok/Source/Animation/Animation/Animation/hkaAnimation.h"
#include "Havok/Source/Animation/Animation/Animation/hkaAnnotationTrackClass.h"
#include "Havok/Source/Animation/Animation/Animation/Interleaved/hkaInterleavedUncompressedAnimation.h"
#include "Havok/Source/Animation/Animation/Compression/SignedQuaternion/SignedQuaternion.h"
#include "Havok/Source/Animation/Animation/Compression/Spline/hkaNURBS.h"
#include "Havok/Source/Animation/Animation/Playback/Cache/hkaChunkCache.h"
#include "Havok/Source/Animation/Internal/Compression/hkaCompression.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4Util.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"
#include "Havok/Source/Common/Base/Thread/Thread/hkThreadLocalData.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/ConvexHull/hkgpConvexHull.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "SDK/Windows/winbase.h"

#include <bit>
#include <cmath>
#include <x86intrin.h>
#include <xmmintrin.h>

// Extracts one transform track's frames into translation, rotation and scale arrays.
static void extractTrackTransforms(const hkQsTransform* transforms, int32_t numTracks, int32_t numFrames,
	int32_t firstFrame, int32_t track, hkArray<hkVector4, hkContainerHeapAllocator>& translations,
	hkArray<hkVector4, hkContainerHeapAllocator>& rotations, hkArray<hkVector4, hkContainerHeapAllocator>& scales);
// One float track's frames into the x of values.
static void extractTrackFloats(const float* floats, int32_t numTracks, int32_t numFrames, int32_t firstFrame,
	int32_t track, hkArray<hkVector4, hkContainerHeapAllocator>& values);
static void finishLoadedObjecthkaSplineCompressedAnimation(void* p, int32_t finishing);
static void cleanupLoadedObjecthkaSplineCompressedAnimation(void* p);
static int32_t packMaskAndQuantization(uint8_t a1, uint8_t a2);
static int32_t packQuantizationTypes(int32_t a1, int32_t a2, int32_t a3);
static hkVector4* calcMean(hkVector4* apResult, const hkArray<hkVector4>& arPoints);
// The componentwise minimum of the points.
static hkVector4* calcMin(hkVector4* apResult, const hkArray<hkVector4>& arPoints);
// The componentwise maximum of the points.
static hkVector4* calcMax(hkVector4* apResult, const hkArray<hkVector4>& arPoints);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkaSplineCompressedAnimationTrackCompressionParamsTypeInfo = {
	"hkaSplineCompressedAnimationTrackCompressionParams", nullptr,
	cleanupLoadedObjecthkaSplineCompressedAnimationTrackCompressionParams, nullptr,
	&typeid(hkaSplineCompressedAnimation::TrackCompressionParams),
	sizeof(hkaSplineCompressedAnimation::TrackCompressionParams)};
hkTypeInfo hkaSplineCompressedAnimationAnimationCompressionParamsTypeInfo = {
	"hkaSplineCompressedAnimationAnimationCompressionParams", nullptr,
	cleanupLoadedObjecthkaSplineCompressedAnimationAnimationCompressionParams, nullptr,
	&typeid(hkaSplineCompressedAnimation::AnimationCompressionParams),
	sizeof(hkaSplineCompressedAnimation::AnimationCompressionParams)};

// The reflected enums of hkaSplineCompressedAnimation::TrackCompressionParams.
using hkaSplineCompressedAnimationTrackCompressionParamsRotationQuantizationEnumItemsArray = const hkClassEnum::Item[6];
static hkaSplineCompressedAnimationTrackCompressionParamsRotationQuantizationEnumItemsArray
	hkaSplineCompressedAnimationTrackCompressionParamsRotationQuantizationEnumItems{
		{hkaSplineCompressedAnimation::TrackCompressionParams::POLAR32, "POLAR32"},
		{hkaSplineCompressedAnimation::TrackCompressionParams::THREECOMP40, "THREECOMP40"},
		{hkaSplineCompressedAnimation::TrackCompressionParams::THREECOMP48, "THREECOMP48"},
		{hkaSplineCompressedAnimation::TrackCompressionParams::THREECOMP24, "THREECOMP24"},
		{hkaSplineCompressedAnimation::TrackCompressionParams::STRAIGHT16, "STRAIGHT16"},
		{hkaSplineCompressedAnimation::TrackCompressionParams::UNCOMPRESSED, "UNCOMPRESSED"}};
using hkaSplineCompressedAnimationTrackCompressionParamsScalarQuantizationEnumItemsArray = const hkClassEnum::Item[2];
static hkaSplineCompressedAnimationTrackCompressionParamsScalarQuantizationEnumItemsArray
	hkaSplineCompressedAnimationTrackCompressionParamsScalarQuantizationEnumItems{
		{hkaSplineCompressedAnimation::TrackCompressionParams::BITS8, "BITS8"},
		{hkaSplineCompressedAnimation::TrackCompressionParams::BITS16, "BITS16"}};
using hkaSplineCompressedAnimationTrackCompressionParamsEnumsArray = const hkClassEnum[2];
static hkaSplineCompressedAnimationTrackCompressionParamsEnumsArray
	hkaSplineCompressedAnimationTrackCompressionParamsEnums{
		{"RotationQuantization", hkaSplineCompressedAnimationTrackCompressionParamsRotationQuantizationEnumItems, 6,
			nullptr, 0},
		{"ScalarQuantization", hkaSplineCompressedAnimationTrackCompressionParamsScalarQuantizationEnumItems, 2,
			nullptr, 0}};

// The reflected members of hkaSplineCompressedAnimation::AnimationCompressionParams and of
// hkaSplineCompressedAnimation.
using hkaSplineCompressedAnimation_AnimationCompressionParamsClass_MembersArray = const hkClassMember[2];
static hkaSplineCompressedAnimation_AnimationCompressionParamsClass_MembersArray
	hkaSplineCompressedAnimation_AnimationCompressionParamsClass_Members{
		{"maxFramesPerBlock", nullptr, nullptr, hkClassMember::TYPE_UINT16, hkClassMember::TYPE_VOID, 0, 0, 0x0,
			nullptr},
		{"enableSampleSingleTracks", nullptr, nullptr, hkClassMember::TYPE_BOOL, hkClassMember::TYPE_VOID, 0, 0, 0x2,
			nullptr}};
using hkaSplineCompressedAnimationClass_MembersArray = const hkClassMember[13];
static hkaSplineCompressedAnimationClass_MembersArray hkaSplineCompressedAnimationClass_Members{
	{"numFrames", nullptr, nullptr, hkClassMember::TYPE_INT32, hkClassMember::TYPE_VOID, 0, 0, 0x28, nullptr},
	{"numBlocks", nullptr, nullptr, hkClassMember::TYPE_INT32, hkClassMember::TYPE_VOID, 0, 0, 0x2C, nullptr},
	{"maxFramesPerBlock", nullptr, nullptr, hkClassMember::TYPE_INT32, hkClassMember::TYPE_VOID, 0, 0, 0x30, nullptr},
	{"maskAndQuantizationSize", nullptr, nullptr, hkClassMember::TYPE_INT32, hkClassMember::TYPE_VOID, 0, 0, 0x34,
		nullptr},
	{"blockDuration", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x38, nullptr},
	{"blockInverseDuration", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x3C, nullptr},
	{"frameDuration", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x40, nullptr},
	{"blockOffsets", nullptr, nullptr, hkClassMember::TYPE_ARRAY, hkClassMember::TYPE_UINT32, 0, 0, 0x44, nullptr},
	{"floatBlockOffsets", nullptr, nullptr, hkClassMember::TYPE_ARRAY, hkClassMember::TYPE_UINT32, 0, 0, 0x50, nullptr},
	{"transformOffsets", nullptr, nullptr, hkClassMember::TYPE_ARRAY, hkClassMember::TYPE_UINT32, 0, 0, 0x5C, nullptr},
	{"floatOffsets", nullptr, nullptr, hkClassMember::TYPE_ARRAY, hkClassMember::TYPE_UINT32, 0, 0, 0x68, nullptr},
	{"data", nullptr, nullptr, hkClassMember::TYPE_ARRAY, hkClassMember::TYPE_UINT8, 0, 0, 0x74, nullptr},
	{"endian", nullptr, nullptr, hkClassMember::TYPE_INT32, hkClassMember::TYPE_VOID, 0, 0, 0x80, nullptr}};

static const hkClassEnum* hkaSplineCompressedAnimationTrackCompressionParamsRotationQuantizationEnum =
	&hkaSplineCompressedAnimationTrackCompressionParamsEnums[0];
static const hkClassEnum* hkaSplineCompressedAnimationTrackCompressionParamsScalarQuantizationEnum =
	&hkaSplineCompressedAnimationTrackCompressionParamsEnums[1];

// The reflected members of hkaSplineCompressedAnimation::TrackCompressionParams: its four quantization members name
// their enums.
using hkaSplineCompressedAnimation_TrackCompressionParamsClass_MembersArray = hkClassMember[12];
static hkaSplineCompressedAnimation_TrackCompressionParamsClass_MembersArray
	hkaSplineCompressedAnimation_TrackCompressionParamsClass_Members{
		{"rotationTolerance", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x0, nullptr},
		{"translationTolerance", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x4,
			nullptr},
		{"scaleTolerance", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x8, nullptr},
		{"floatingTolerance", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0xC, nullptr},
		{"rotationDegree", nullptr, nullptr, hkClassMember::TYPE_UINT16, hkClassMember::TYPE_VOID, 0, 0, 0x10, nullptr},
		{"translationDegree", nullptr, nullptr, hkClassMember::TYPE_UINT16, hkClassMember::TYPE_VOID, 0, 0, 0x12,
			nullptr},
		{"scaleDegree", nullptr, nullptr, hkClassMember::TYPE_UINT16, hkClassMember::TYPE_VOID, 0, 0, 0x14, nullptr},
		{"floatingDegree", nullptr, nullptr, hkClassMember::TYPE_UINT16, hkClassMember::TYPE_VOID, 0, 0, 0x16, nullptr},
		{"rotationQuantizationType", nullptr, &hkaSplineCompressedAnimationTrackCompressionParamsEnums[0],
			hkClassMember::TYPE_ENUM, hkClassMember::TYPE_UINT8, 0, 0, 0x18, nullptr},
		{"translationQuantizationType", nullptr, &hkaSplineCompressedAnimationTrackCompressionParamsEnums[1],
			hkClassMember::TYPE_ENUM, hkClassMember::TYPE_UINT8, 0, 0, 0x19, nullptr},
		{"scaleQuantizationType", nullptr, &hkaSplineCompressedAnimationTrackCompressionParamsEnums[1],
			hkClassMember::TYPE_ENUM, hkClassMember::TYPE_UINT8, 0, 0, 0x1A, nullptr},
		{"floatQuantizationType", nullptr, &hkaSplineCompressedAnimationTrackCompressionParamsEnums[1],
			hkClassMember::TYPE_ENUM, hkClassMember::TYPE_UINT8, 0, 0, 0x1B, nullptr}};

// The static of hkaSplineCompressedAnimation::unpackQuaternion: the quaternion unpackers, indexed by
// TrackCompressionParams::RotationQuantization.
using unpackfuncArray = void (*[6])(const uint8_t* in, hkQuaternion* out);
static unpackfuncArray unpackfunc{&unpackSignedQuaternion32, &unpackSignedQuaternion40, &unpackSignedQuaternion48,
	&unpackSignedQuaternion24, &unpackSignedQuaternion16, &unpackSignedQuaternion128};
hkClass hkaSplineCompressedAnimationTrackCompressionParamsClass("hkaSplineCompressedAnimationTrackCompressionParams",
	nullptr, 28, nullptr, 0, hkaSplineCompressedAnimationTrackCompressionParamsEnums, 2,
	hkaSplineCompressedAnimation_TrackCompressionParamsClass_Members, 12, nullptr, nullptr, 0, 0);
hkClass hkaSplineCompressedAnimationAnimationCompressionParamsClass("hkaSplineCompressedAnimationAnimationCompressionPa"
																	"rams",
	nullptr, 4, nullptr, 0, nullptr, 0, hkaSplineCompressedAnimation_AnimationCompressionParamsClass_Members, 2,
	nullptr, nullptr, 0, 0);
hkClass hkaSplineCompressedAnimationClass("hkaSplineCompressedAnimation", &hkaAnimationClass, 132, nullptr, 0, nullptr,
	0, hkaSplineCompressedAnimationClass_Members, 13, nullptr, nullptr, 0, 0);
hkTypeInfo hkaSplineCompressedAnimationTypeInfo = {"hkaSplineCompressedAnimation",
	finishLoadedObjecthkaSplineCompressedAnimation, cleanupLoadedObjecthkaSplineCompressedAnimation,
	getVtablehkaSplineCompressedAnimation(), &typeid(hkaSplineCompressedAnimation),
	sizeof(hkaSplineCompressedAnimation)};

// Track `track` of numFrames frames from firstFrame of the interleaved transforms (numTracks per frame) into the
// translation, rotation and scale arrays; the translation's and the scale's w are zero.
static void extractTrackTransforms(const hkQsTransform* transforms, int32_t numTracks, int32_t numFrames,
	int32_t firstFrame, int32_t track, hkArray<hkVector4, hkContainerHeapAllocator>& translations,
	hkArray<hkVector4, hkContainerHeapAllocator>& rotations, hkArray<hkVector4, hkContainerHeapAllocator>& scales) {
	const hkQsTransform* t = transforms + (track + firstFrame * numTracks);
	for (int32_t i = 0; i < numFrames; i++) {
		translations.m_data[i].m_quad =
			_mm_setr_ps(t->m_translation(0), t->m_translation(1), t->m_translation(2), 0.0f);
		rotations.m_data[i].m_quad =
			_mm_setr_ps(t->m_rotation.m_vec(0), t->m_rotation.m_vec(1), t->m_rotation.m_vec(2), t->m_rotation.m_vec(3));
		scales.m_data[i].m_quad = _mm_setr_ps(t->m_scale(0), t->m_scale(1), t->m_scale(2), 0.0f);
		t += numTracks;
	}
}

// Float track `track` of numFrames frames from firstFrame of the interleaved floats (numTracks per frame) into the
// x of values, its y, z and w zero.
static void extractTrackFloats(const float* floats, int32_t numTracks, int32_t numFrames, int32_t firstFrame,
	int32_t track, hkArray<hkVector4, hkContainerHeapAllocator>& values) {
	const float* f = floats + (track + firstFrame * numTracks);
	for (int32_t i = 0; i < numFrames; i++) {
		values.m_data[i].m_quad = _mm_setr_ps(*f, 0.0f, 0.0f, 0.0f);
		f += numTracks;
	}
}

static void cleanupLoadedObjecthkaSplineCompressedAnimation(void* p) {
	static_cast<hkaSplineCompressedAnimation*>(p)->~hkaSplineCompressedAnimation();
}

// The type info's finish-loaded-object callback: the null check, then the animation's finish constructor.
static void finishLoadedObjecthkaSplineCompressedAnimation(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag kFlag;
		kFlag.m_finishing = finishing;
		::new (p) hkaSplineCompressedAnimation(kFlag);
	}
}

static int32_t packMaskAndQuantization(uint8_t a1, uint8_t a2) {
	return a1 | (2 * a2);
}

static int32_t packQuantizationTypes(int32_t a1, int32_t a2, int32_t a3) {
	return a1 | 4 * (a2 | 16 * a3);
}

// The average of the points: their sum times the reciprocal of their count (computed on the x87 stack and rounded to
// a float).
static hkVector4* calcMean(hkVector4* apResult, const hkArray<hkVector4>& arPoints) {
	const int32_t iSize = arPoints.m_size;
	hkQuadReal kSum = _mm_setzero_ps();
	apResult->m_quad = kSum;
	if (iSize > 0) {
		const hkVector4* pPoint = arPoints.m_data;
		for (int32_t i = iSize; i != 0; --i) {
			kSum = _mm_add_ps(pPoint->m_quad, kSum);
			++pPoint;
		}
		apResult->m_quad = kSum;
	}
	const float fInv = 1.0 / static_cast<double>(iSize);
	apResult->m_quad = _mm_mul_ps(_mm_set_ps1(fInv), apResult->m_quad);
	return apResult;
}

// The componentwise minimum of the points; the first point is copied even when the array is empty.
static hkVector4* calcMin(hkVector4* apResult, const hkArray<hkVector4>& arPoints) {
	const hkVector4* pPoint = arPoints.m_data;
	const int32_t iSize = arPoints.m_size;
	*apResult = pPoint[0];
	if (iSize > 1) {
		hkQuadReal kMin = apResult->m_quad;
		for (int32_t i = iSize - 1; i != 0; --i) {
			++pPoint;
			kMin = _mm_min_ps(kMin, pPoint->m_quad);
		}
		apResult->m_quad = kMin;
	}
	return apResult;
}

// The componentwise maximum of the points; the first point is copied even when the array is empty.
static hkVector4* calcMax(hkVector4* apResult, const hkArray<hkVector4>& arPoints) {
	const hkVector4* pPoint = arPoints.m_data;
	const int32_t iSize = arPoints.m_size;
	*apResult = pPoint[0];
	if (iSize > 1) {
		hkQuadReal kMax = apResult->m_quad;
		for (int32_t i = iSize - 1; i != 0; --i) {
			++pPoint;
			kMax = _mm_max_ps(kMax, pPoint->m_quad);
		}
		apResult->m_quad = kMax;
	}
	return apResult;
}

const void* getVtablehkaSplineCompressedAnimation() {
	return hkGetVtableOf<hkaSplineCompressedAnimation>();
}

// The B-spline of degree p at u: the p + 1 basis functions of the span whose knots U[0] .. U[2p - 1] surround u (Piegl
// and Tiller's BasisFuns, the span at index p - 1 of U), each rounded to a float, then the sum of the control points
// P[0] .. P[p] weighted by them.
void evaluateSpline(float u, int32_t p, const float* U, const hkVector4* P, hkVector4& out) {
	float N[4];
	float left[4];
	float right[4];
	N[0] = 1.0f;
	for (int32_t j = 1; j <= p; j++) {
		left[j] = u - U[p - j];
		right[j] = U[p - 1 + j] - u;
		float saved = 0.0f;
		for (int32_t r = 0; r < j; r++) {
			const float temp = N[r] / (right[r + 1] + left[j - r]);
			N[r] = saved + right[r + 1] * temp;
			saved = left[j - r] * temp;
		}
		N[j] = saved;
	}
	__m128 sum = _mm_setzero_ps();
	for (int32_t r = 0; r <= p; r++) {
		const __m128 n = _mm_set_ss(N[r]);
		sum = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(n, n, 0), P[r].m_quad), sum);
	}
	out.m_quad = sum;
}

hkaSplineCompressedAnimation::TrackCompressionParams::TrackCompressionParams()
	: m_rotationTolerance(0.001f), m_translationTolerance(0.001f), m_scaleTolerance(0.001f),
	  m_floatingTolerance(0.001f), m_rotationDegree(3), m_translationDegree(3), m_scaleDegree(3), m_floatingDegree(3) {
	m_rotationQuantizationType = THREECOMP40;
	m_translationQuantizationType = BITS16;
	m_scaleQuantizationType = BITS16;
	m_floatQuantizationType = BITS16;
}

// Every quantization type must be valid and no tolerance may be negative; each accuracy check only reports. The four
// validity calls short-circuit in the order translation, rotation, scale, float, and each writes its own hkBool; the
// accuracy checks run whatever they returned, in the order translation, scale, float, rotation. The tolerance is
// widened to double for the compare and the rotation accuracy is multiplied by the double 0.16260161995887756 first.
// Each report goes through a local hkErrStream (0x200 byte buffer, id 0xABBA5A61).
hkBool* hkaSplineCompressedAnimation::TrackCompressionParams::isOk(hkBool* arResult) const {
	hkBool translationValid;
	hkBool rotationValid;
	hkBool scaleValid;
	hkBool floatValid;
	const bool bQuantizationValid =
		validQuantization(&translationValid, static_cast<ScalarQuantization>(m_translationQuantizationType.m_storage))
			->m_bool &&
		validQuantization(&rotationValid, static_cast<RotationQuantization>(m_rotationQuantizationType.m_storage))
			->m_bool &&
		validQuantization(&scaleValid, static_cast<ScalarQuantization>(m_scaleQuantizationType.m_storage))->m_bool &&
		validQuantization(&floatValid, static_cast<ScalarQuantization>(m_floatQuantizationType.m_storage))->m_bool;
	const bool bTolerancesValid = m_translationTolerance >= 0.0f && m_rotationTolerance >= 0.0f &&
		m_scaleTolerance >= 0.0f && m_floatingTolerance >= 0.0f;

	if (approximateQuantizationAccuracy(static_cast<ScalarQuantization>(m_translationQuantizationType.m_storage)) >
		m_translationTolerance) {
		alignas(4) uint8_t kStreamStorage[sizeof(hkErrStream)];
		char acBuffer[0x200];
		hkErrStream* pStream = ::new (kStreamStorage) hkErrStream(acBuffer, 0x200);
		*pStream << "Quantization may be limiting translation quality.  If artifacts are visible, try increasing "
					"the number of translation quantization bits.";
		hkError::s_instance->message(hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBA5A61u), acBuffer,
			".\\Animation\\SplineCompressed\\hkaSplineCompressedAnimation.cpp", 0x4F);
		FNVD_DESTRUCT(hkOstream)(pStream);
	}

	if (approximateQuantizationAccuracy(static_cast<ScalarQuantization>(m_scaleQuantizationType.m_storage)) >
		m_scaleTolerance) {
		alignas(4) uint8_t kStreamStorage[sizeof(hkErrStream)];
		char acBuffer[0x200];
		hkErrStream* pStream = ::new (kStreamStorage) hkErrStream(acBuffer, 0x200);
		*pStream << "Quantization may be limiting scale quality.  If artifacts are visible, try increasing the "
					"number of scale quantization bits.";
		hkError::s_instance->message(hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBA5A61u), acBuffer,
			".\\Animation\\SplineCompressed\\hkaSplineCompressedAnimation.cpp", 0x54);
		FNVD_DESTRUCT(hkOstream)(pStream);
	}

	if (approximateQuantizationAccuracy(static_cast<ScalarQuantization>(m_floatQuantizationType.m_storage)) >
		m_floatingTolerance) {
		alignas(4) uint8_t kStreamStorage[sizeof(hkErrStream)];
		char acBuffer[0x200];
		hkErrStream* pStream = ::new (kStreamStorage) hkErrStream(acBuffer, 0x200);
		*pStream << "Quantization may be limiting float quality.  If artifacts are visible, try increasing the "
					"number of float quantization bits.";
		hkError::s_instance->message(hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBA5A61u), acBuffer,
			".\\Animation\\SplineCompressed\\hkaSplineCompressedAnimation.cpp", 0x59);
		FNVD_DESTRUCT(hkOstream)(pStream);
	}

	if (approximateQuantizationAccuracy(static_cast<RotationQuantization>(m_rotationQuantizationType.m_storage)) *
			0.16260161995887756 >
		m_rotationTolerance) {
		alignas(4) uint8_t kStreamStorage[sizeof(hkErrStream)];
		char acBuffer[0x200];
		hkErrStream* pStream = ::new (kStreamStorage) hkErrStream(acBuffer, 0x200);
		*pStream << "Quantization may be limiting rotation quality.  If artifacts are visible, try increasing the "
					"number of rotation quantization bits.";
		hkError::s_instance->message(hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBA5A61u), acBuffer,
			".\\Animation\\SplineCompressed\\hkaSplineCompressedAnimation.cpp", 0x64);
		FNVD_DESTRUCT(hkOstream)(pStream);
	}

	arResult->m_bool = (bQuantizationValid && bTolerancesValid) ? 1 : 0;
	return arResult;
}

// Whether the scalar quantization is BITS8 or BITS16.
// KEEP: unregistered. The exe body touches only eax and ecx, and a caller relies on edx surviving the call
// (checkbody: "caller relies on preserved edx"), which clang does not guarantee.
hkBool* hkaSplineCompressedAnimation::TrackCompressionParams::validQuantization(hkBool* arResult,
	hkaSplineCompressedAnimation::TrackCompressionParams::ScalarQuantization aeType) {
	arResult->m_bool = aeType == BITS8 || aeType == BITS16;
	return arResult;
}

// KEEP: unregistered. Whether the rotation quantization is one of the six types (cdecl, 0xE32DF0). The exe body
// touches only eax and ecx, and its caller isOk (0xE330B0) pushes edx at 0xE33154, loaded at 0xE330BA before this
// call, which clang does not guarantee to keep.
hkBool* hkaSplineCompressedAnimation::TrackCompressionParams::validQuantization(hkBool* arResult,
	hkaSplineCompressedAnimation::TrackCompressionParams::RotationQuantization aeType) {
	arResult->m_bool = aeType == POLAR32 || aeType == THREECOMP40 || aeType == THREECOMP48 || aeType == THREECOMP24 ||
		aeType == STRAIGHT16 || aeType == UNCOMPRESSED;
	return arResult;
}

// BITS8 returns 0.0078125 and BITS16 returns 3.0517578125e-05; any other value returns zero.
float hkaSplineCompressedAnimation::TrackCompressionParams::approximateQuantizationAccuracy(ScalarQuantization aeType) {
	if (!aeType) {
		return 0x1p-7f;
	}
	if (((int32_t)aeType) == 1) {
		return 0x1p-15f;
	}
	return 0.0;
}

// Returns one of six constants by aeType, and 0.0f above index 5.
float hkaSplineCompressedAnimation::TrackCompressionParams::
	approximateQuantizationAccuracy(hkaSplineCompressedAnimation::TrackCompressionParams::RotationQuantization aeType) {
	double v1;
	switch (aeType) {
	case hkaSplineCompressedAnimation::TrackCompressionParams::POLAR32: {
		v1 = 0.00390625f;
		break;
	}
	case hkaSplineCompressedAnimation::TrackCompressionParams::THREECOMP40: {
		v1 = 0.00048828125f;
		break;
	}
	case hkaSplineCompressedAnimation::TrackCompressionParams::THREECOMP48: {
		v1 = 6.1035156e-05f;
		break;
	}
	case hkaSplineCompressedAnimation::TrackCompressionParams::THREECOMP24: {
		v1 = 0.015625f;
		break;
	}
	case hkaSplineCompressedAnimation::TrackCompressionParams::STRAIGHT16: {
		v1 = 0.125f;
		break;
	}
	case hkaSplineCompressedAnimation::TrackCompressionParams::UNCOMPRESSED: {
		v1 = 1.1920929e-07f;
		break;
	}
	default: {
		v1 = 0.0;
		break;
	}
	}
	return v1;
}

const hkClass& hkaSplineCompressedAnimation::TrackCompressionParams::staticClass() {
	return hkaSplineCompressedAnimationTrackCompressionParamsClass;
}

hkBool* hkaSplineCompressedAnimation::PerTrackCompressionParams::isOk(hkBool* arResult) const {
	hkBool arResulta;
	int32_t v3 = 0;
	if (m_parameterPalette.m_size <= 0) {
	LABEL_5:;
		arResult->m_bool = 1;
		return arResult;
	} else {
		int32_t v4 = 0;
		while (m_parameterPalette.m_data[v4].isOk(&arResulta)->m_bool) {
			++v3;
			++v4;
			if (v3 >= m_parameterPalette.m_size) {
				goto LABEL_5;
			}
		}
		arResult->m_bool = 0;
		return arResult;
	}
}

// A palette of trackParams alone, every transform and float track indexing it.
hkaSplineCompressedAnimation::hkaSplineCompressedAnimation(const hkaInterleavedUncompressedAnimation& raw,
	const TrackCompressionParams& trackParams, const AnimationCompressionParams& animationParams)
	: hkaAnimation(raw), m_blockOffsets(), m_floatBlockOffsets(), m_transformOffsets(), m_floatOffsets(), m_data() {
	PerTrackCompressionParams perTrackParams;
	perTrackParams.m_parameterPalette.pushBack(trackParams);
	perTrackParams.m_trackIndexToPaletteIndex.setSize(raw.m_numberOfTransformTracks);
	perTrackParams.m_floatTrackIndexToPaletteIndex.setSize(raw.m_numberOfFloatTracks);
	for (int32_t i = 0; i < raw.m_numberOfTransformTracks; ++i) {
		perTrackParams.m_trackIndexToPaletteIndex.m_data[i] = 0;
	}
	for (int32_t i = 0; i < raw.m_numberOfFloatTracks; ++i) {
		perTrackParams.m_floatTrackIndexToPaletteIndex.m_data[i] = 0;
	}
	initialize(raw, perTrackParams, animationParams);
}

hkaSplineCompressedAnimation::hkaSplineCompressedAnimation(const hkaInterleavedUncompressedAnimation& raw,
	const PerTrackCompressionParams& trackParams, const AnimationCompressionParams& animationParams)
	: hkaAnimation(raw), m_blockOffsets(), m_floatBlockOffsets(), m_transformOffsets(), m_floatOffsets(), m_data() {
	initialize(raw, trackParams, animationParams);
}

// The five offset and data arrays go with their members.
hkaSplineCompressedAnimation::~hkaSplineCompressedAnimation() {}

void hkaSplineCompressedAnimation::sampleTracks(float time, hkQsTransform* transformTracksOut, float* floatTracksOut,
	hkaChunkCache* cache) const {
	samplePartialTracks(time, m_numberOfTransformTracks, transformTracksOut, m_numberOfFloatTracks, floatTracksOut,
		cache);
}

// Each transform track has four bytes at the block's start (computePackedNurbsOffsets with offset 0, flag bit 31):
// its quantization types, then the translation, rotation and scale masks; the float tracks' mask bytes follow them.
void hkaSplineCompressedAnimation::samplePartialTracks(float time, uint32_t maxNumTransformTracks,
	hkQsTransform* transformTracksOut, uint32_t maxNumFloatTracks, float* floatTracksOut, hkaChunkCache* cache) const {
	HK_TIMER_BEGIN("SampleSpline", nullptr);

	uint32_t frame;
	float delta;
	getFrameAndDelta(time, frame, delta);
	int32_t block;
	float blockTime;
	uint8_t quantizedTime;
	getBlockAndTime(frame, delta, block, blockTime, quantizedTime);

	const uint8_t* dataPtr = computePackedNurbsOffsets(m_data.m_data, m_blockOffsets.m_data, block,
		static_cast<uint32_t>(m_maskAndQuantizationSize));
	const uint8_t* maskPtr = computePackedNurbsOffsets(m_data.m_data, m_blockOffsets.m_data, block, 0x80000000u);
	if (dataPtr && maskPtr) {
		for (uint32_t i = 0; i < maxNumTransformTracks; i++) {
			const uint32_t quantizationTypes = maskPtr[0];
			hkQsTransform& out = transformTracksOut[i];
			sampleTranslation(blockTime, quantizedTime, maskPtr[1],
				static_cast<TrackCompressionParams::ScalarQuantization>(quantizationTypes & 3), dataPtr,
				out.m_translation);
			sampleRotation(blockTime, quantizedTime, maskPtr[2],
				static_cast<TrackCompressionParams::RotationQuantization>((quantizationTypes >> 2) & 0xF), dataPtr,
				out.m_rotation);
			sampleScale(blockTime, quantizedTime, maskPtr[3],
				static_cast<TrackCompressionParams::ScalarQuantization>(quantizationTypes >> 6), dataPtr, out.m_scale);
			maskPtr += 4;
		}

		const uint8_t* floatDataPtr = m_data.m_data + m_floatBlockOffsets.m_data[block] + m_blockOffsets.m_data[block];
		const uint8_t* floatMaskPtr = m_data.m_data + m_blockOffsets.m_data[block] + 4 * m_numberOfTransformTracks;
		for (uint32_t i = 0; i < maxNumFloatTracks; i++) {
			const uint8_t floatMask = *floatMaskPtr++;
			sampleFloat(blockTime, quantizedTime, static_cast<uint8_t>(floatMask & 0xF9),
				static_cast<TrackCompressionParams::ScalarQuantization>((floatMask >> 1) & 3), floatDataPtr,
				floatTracksOut[i]);
		}
	}

	HK_TIMER_END();
}

// Samples the listed transform tracks at time; a track index out of range gives a zero transform.
void hkaSplineCompressedAnimation::sampleIndividualTransformTracks(float time, const int16_t* tracks,
	uint32_t numTracks, hkQsTransform* transformTracksOut) const {
	uint32_t frame;
	float delta;
	getFrameAndDelta(time, frame, delta);

	int32_t block;
	float blockTime;
	uint8_t quantizedTime;
	getBlockAndTime(frame, delta, block, blockTime, quantizedTime);

	for (uint32_t i = 0; i < numTracks; ++i) {
		const int32_t track = tracks[i];
		if (track < 0 || track >= m_numberOfTransformTracks) {
			transformTracksOut[i].m_translation.setZero4();
			transformTracksOut[i].m_rotation.m_vec.setZero4();
			transformTracksOut[i].m_scale.setZero4();
			continue;
		}
		const uint8_t* dataInOut = computePackedNurbsOffsets(m_data.m_data, m_blockOffsets.m_data, block,
			m_transformOffsets.m_data[block * m_numberOfTransformTracks + track]);
		const uint8_t* mask = computePackedNurbsOffsets(m_data.m_data, m_blockOffsets.m_data, block, 4 * track);
		const uint32_t maskByte = *mask;
		sampleTranslation(blockTime, quantizedTime, mask[1],
			static_cast<TrackCompressionParams::ScalarQuantization>(maskByte & 3), dataInOut,
			transformTracksOut[i].m_translation);
		sampleRotation(blockTime, quantizedTime, mask[2],
			static_cast<TrackCompressionParams::RotationQuantization>((maskByte >> 2) & 0xF), dataInOut,
			transformTracksOut[i].m_rotation);
		sampleScale(blockTime, quantizedTime, mask[3],
			static_cast<TrackCompressionParams::ScalarQuantization>(maskByte >> 6), dataInOut,
			transformTracksOut[i].m_scale);
	}
}

// Samples the listed float tracks at time; a track index out of range gives 0.
void hkaSplineCompressedAnimation::sampleIndividualFloatTracks(float time, const int16_t* tracks, uint32_t numTracks,
	float* floatTracksOut) const {
	uint32_t frame;
	float delta;
	getFrameAndDelta(time, frame, delta);

	int32_t block;
	float blockTime;
	uint8_t quantizedTime;
	getBlockAndTime(frame, delta, block, blockTime, quantizedTime);

	for (uint32_t i = 0; i < numTracks; ++i) {
		const int32_t track = tracks[i];
		if (track < 0 || track >= m_numberOfFloatTracks) {
			floatTracksOut[i] = 0.0f;
			continue;
		}
		const uint8_t* dataInOut = computePackedNurbsOffsets(m_data.m_data, m_blockOffsets.m_data, block,
			m_floatOffsets.m_data[block * m_numberOfFloatTracks + track]);
		const uint8_t* mask = computePackedNurbsOffsets(m_data.m_data, m_blockOffsets.m_data, block,
			4 * m_numberOfTransformTracks + track);
		const uint8_t maskByte = *mask;
		sampleFloat(blockTime, quantizedTime, static_cast<uint8_t>(maskByte & 0xF9),
			static_cast<TrackCompressionParams::ScalarQuantization>((maskByte >> 1) & 3), dataInOut, floatTracksOut[i]);
	}
}

int32_t hkaSplineCompressedAnimation::getNumOriginalFrames() const {
	return (int32_t)((uint32_t)m_numFrames);
}

int32_t hkaSplineCompressedAnimation::getNumDataChunks(uint32_t frame, float delta) const {
	return 3;
}

// The object (its size rounded up to 16 bytes), the block's data (its size rounded up to 16) and the block's float
// block offset.
void hkaSplineCompressedAnimation::getDataChunks(uint32_t frame, float delta, hkaAnimation::DataChunk* dataChunks,
	int32_t numDataChunks) const {
	dataChunks[0].m_data = this;
	dataChunks[0].m_size =
		static_cast<uint32_t>((sizeof(hkaSplineCompressedAnimation) + 15) & ~static_cast<size_t>(15));
	int32_t block;
	float blockTime;
	uint8_t quantizedTime;
	getBlockAndTime(frame, delta, block, blockTime, quantizedTime);
	const uint32_t blockEnd = block == m_numBlocks - 1 ? m_data.m_size : m_blockOffsets.m_data[block + 1];
	const uint32_t blockSize = (blockEnd - m_blockOffsets.m_data[block] + 15) & ~15u;
	dataChunks[1].setDataChunkWith16ByteAlignmentCorrection(m_data.m_data + m_blockOffsets.m_data[block], blockSize);
	dataChunks[2].setDataChunkWith16ByteAlignmentCorrection(&m_floatBlockOffsets.m_data[block], sizeof(uint32_t));
}

int32_t hkaSplineCompressedAnimation::getMaxSizeOfCombinedDataChunks() const {
	int32_t m_size = m_data.m_size;
	int32_t v2 = 0;
	int32_t v3 = m_blockOffsets.m_size - 1;
	if (v3 >= 0) {
		uint32_t* v4 = &m_blockOffsets.m_data[v3];
		do {
			int32_t v5 = *v4;
			int32_t v6 = m_size - *v4;
			if (v2 <= v6) {
				v2 = v6;
			}
			--v3;
			--v4;
			m_size = v5;
		} while (v3 >= 0);
	}
	return (v2 + 15 & 0xFFFFFFF0u) + 160;
}

void hkaSplineCompressedAnimation::sampleRotation(float time, uint8_t quantizedTime, uint8_t mask,
	TrackCompressionParams::RotationQuantization rotationQuantizationType, const uint8_t*& dataInOut,
	hkQuaternion& out) const {
	readNURBSQuaternion(dataInOut, quantizedTime, m_frameDuration, time, mask, rotationQuantizationType, out);
	dataInOut = reinterpret_cast<const uint8_t*>(reinterpret_cast<uintptr_t>(dataInOut + 3) & ~uintptr_t(3));
}

void hkaSplineCompressedAnimation::sampleTranslation(float u, uint8_t quantizedTime, uint8_t mask,
	hkaSplineCompressedAnimation::TrackCompressionParams::ScalarQuantization scalarQuantizationType,
	const uint8_t*& dataInOut, hkVector4& out) const {
	if (mask) {
		hkVector4 I_;
		I_.m_quad = hkQsTransformIdentity.m_translation.m_quad;
		readNURBSCurve(dataInOut, quantizedTime, m_frameDuration, u, mask, scalarQuantizationType, I_, out);
	} else {
		out = hkQsTransformIdentity.m_translation;
	}
	dataInOut = reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>((dataInOut + 3)) & ~uintptr_t(3));
}

void hkaSplineCompressedAnimation::sampleScale(float u, uint8_t quantizedTime, uint8_t mask,
	hkaSplineCompressedAnimation::TrackCompressionParams::ScalarQuantization scalarQuantizationType,
	const uint8_t*& dataInOut, hkVector4& out) const {
	if (mask) {
		hkVector4 I_;
		I_.m_quad = hkQsTransformIdentity.m_scale.m_quad;
		readNURBSCurve(dataInOut, quantizedTime, m_frameDuration, u, mask, scalarQuantizationType, I_, out);
	} else {
		out = hkQsTransformIdentity.m_scale;
	}
	dataInOut = reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>((dataInOut + 3)) & ~uintptr_t(3));
}

void hkaSplineCompressedAnimation::sampleFloat(float u, uint8_t quantizedTime, uint8_t mask,
	hkaSplineCompressedAnimation::TrackCompressionParams::ScalarQuantization scalarQuantizationType,
	const uint8_t*& dataInOut, float& out) const {
	if (mask) {
		hkVector4 I_;
		hkVector4 out_;
		double m_frameDuration = this->m_frameDuration;
		I_.m_quad = _mm_setzero_ps();
		readNURBSCurve(dataInOut, quantizedTime, m_frameDuration, u, mask, scalarQuantizationType, I_, out_);
		out = out_.m_quad[0];
	} else {
		out = 0.0f;
	}
	dataInOut = reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>((dataInOut + 3)) & ~uintptr_t(3));
}

int32_t hkaSplineCompressedAnimation::getSizeInBytes() {
	return m_data.m_size + 4 * (m_blockOffsets.m_size + (m_transformOffsets.m_size + m_floatOffsets.m_size)) + 132;
}

// Builds the animation's block layout from the raw animation's frame count, then per block the static mask and
// quantization bytes (padZeros reserves them), and per transform track (translation, rotation, scale) or float track
// the samples, their mean and range, the static mask (isStatic against the clear value), and for a dynamic one a NURBS
// fit (again at degree 1 when it needs more control points than samples), written out with its knots; then the
// compression report. Each track's sample and fit arrays are hkArray locals. The tolerances are float copies.
void hkaSplineCompressedAnimation::initialize(const hkaInterleavedUncompressedAnimation& raw,
	const hkaSplineCompressedAnimation::PerTrackCompressionParams& trackParams,
	const hkaSplineCompressedAnimation::AnimationCompressionParams& animationParams) {
	// EXE-INLINED none: hkEnum<hkaAnimation::AnimationType, int32_t>::operator=
	m_type.m_storage = HK_SPLINE_COMPRESSED_ANIMATION;
	m_endian = 0;
	m_maxFramesPerBlock = animationParams.m_maxFramesPerBlock;
	const uint8_t enableSampleSingleTracks = animationParams.m_enableSampleSingleTracks.m_bool;
	const int32_t numFrames = static_cast<const hkaAnimation&>(raw).getNumOriginalFrames();
	const int32_t framesPerBlockMinusOne = m_maxFramesPerBlock - 1;
	int32_t numBlocks = (m_maxFramesPerBlock + numFrames - 3) / framesPerBlockMinusOne;
	m_numFrames = numFrames;
	if (numBlocks < 1) {
		numBlocks = 1;
	}
	m_numBlocks = numBlocks;
	m_maskAndQuantizationSize = (m_numberOfFloatTracks + 4 * m_numberOfTransformTracks + 3) & ~3;
	// The frame duration stays on the x87 stack; the block duration is rounded to float before its inverse.
	float blockDuration = static_cast<float>(static_cast<double>(framesPerBlockMinusOne) *
		(m_duration / static_cast<double>(numFrames - 1)));
	m_blockDuration = blockDuration;
	m_blockInverseDuration = 1.0 / blockDuration;
	m_frameDuration = m_duration / static_cast<double>(numFrames - 1);

	// The SDK's clear values per transform component: no translation, the identity rotation, unit scale.
	hkVector4 clearValues[3];
	clearValues[0].m_quad = _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f);
	clearValues[1].m_quad = _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f);
	clearValues[2].m_quad = _mm_set_ps(0.0f, 1.0f, 1.0f, 1.0f);

	using VectorArrays = hkArray<hkVector4>[3];
	using FloatArrays = hkArray<float>[3];
	int32_t firstFrame = 0;
	for (int32_t block = 0; block < m_numBlocks; ++block) {
		{
			// EXE-INLINED none: hkArray<hkUint32>::pushBack
			const uint32_t blockOffset = static_cast<uint32_t>(m_data.m_size);
			hkMemoryRouter& router = hkMemoryRouter::getInstance();
			if (m_blockOffsets.m_size == (m_blockOffsets.m_capacityAndFlags & hkArrayBase<uint32_t>::CAPACITY_MASK)) {
				hkArrayUtil::_reserveMore(*router.m_heap, &m_blockOffsets, static_cast<int32_t>(sizeof(uint32_t)));
			}
			m_blockOffsets.m_data[m_blockOffsets.m_size] = blockOffset;
			++m_blockOffsets.m_size;
		}
		int32_t numBlockFrames = m_maxFramesPerBlock;
		if (numBlockFrames >= m_numFrames - firstFrame) {
			numBlockFrames = m_numFrames - firstFrame;
		}
		const int32_t m = numBlockFrames - 1;
		const float blockTime = m_duration / static_cast<double>(m_numFrames - 1) * static_cast<double>(m);
		int32_t dataOffset = m_data.m_size;
		padZeros(m_maskAndQuantizationSize, &m_data);

		for (int32_t trackType = 0; trackType < 2; ++trackType) {
			const int32_t numTracks = trackType ? m_numberOfFloatTracks : m_numberOfTransformTracks;
			const int32_t numComponents = trackType == 0 ? 3 : 1;
			if (trackType == 1) {
				// EXE-INLINED none: hkArray<hkUint32>::pushBack
				const uint32_t offset = static_cast<uint32_t>(m_data.m_size) - m_blockOffsets.m_data[block];
				hkMemoryRouter& router = hkMemoryRouter::getInstance();
				if (m_floatBlockOffsets.m_size ==
					(m_floatBlockOffsets.m_capacityAndFlags & hkArrayBase<uint32_t>::CAPACITY_MASK)) {
					hkArrayUtil::_reserveMore(*router.m_heap, &m_floatBlockOffsets,
						static_cast<int32_t>(sizeof(uint32_t)));
				}
				m_floatBlockOffsets.m_data[m_floatBlockOffsets.m_size] = offset;
				++m_floatBlockOffsets.m_size;
			}
			for (int32_t track = 0; track < numTracks; ++track) {
				if (enableSampleSingleTracks) {
					// EXE-INLINED none: hkArray<hkUint32>::pushBack
					const uint32_t offset = static_cast<uint32_t>(m_data.m_size) - m_blockOffsets.m_data[block];
					hkArray<uint32_t>& offsets = trackType == 0 ? m_transformOffsets : m_floatOffsets;
					hkMemoryRouter& router = hkMemoryRouter::getInstance();
					if (offsets.m_size == (offsets.m_capacityAndFlags & hkArrayBase<uint32_t>::CAPACITY_MASK)) {
						hkArrayUtil::_reserveMore(*router.m_heap, &offsets, static_cast<int32_t>(sizeof(uint32_t)));
					}
					offsets.m_data[offsets.m_size] = offset;
					++offsets.m_size;
				}
				const int32_t* paletteIndices = trackType ? trackParams.m_floatTrackIndexToPaletteIndex.m_data
														  : trackParams.m_trackIndexToPaletteIndex.m_data;
				const hkaSplineCompressedAnimation::TrackCompressionParams* params =
					&trackParams.m_parameterPalette.m_data[paletteIndices[track]];

				// The samples of each component and their times.
				alignas(16) FNVD_INLINED_LOCAL(VectorArrays, samples, std::nullopt, std::nullopt);
				FNVD_INLINED_LOCAL(FloatArrays, times, std::nullopt, std::nullopt);
				for (int32_t k = 0; k < 3; ++k) {
					// EXE-INLINED none: hkArray<hkVector4>::hkArray
					samples[k].m_data = nullptr;
					samples[k].m_size = 0;
					samples[k].m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
				}
				for (int32_t k = 0; k < 3; ++k) {
					// EXE-INLINED none: hkArray<hkReal>::hkArray
					times[k].m_data = nullptr;
					times[k].m_size = 0;
					times[k].m_capacityAndFlags = hkArrayBase<float>::DONT_DEALLOCATE_FLAG;
				}
				for (int32_t k = 0; k < numComponents; ++k) {
					{
						// EXE-INLINED none: hkArray<hkVector4>::setSize
						hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
						const int32_t capacity = samples[k].m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK;
						if (capacity < numBlockFrames) {
							int32_t request = 2 * capacity;
							if (numBlockFrames >= request) {
								request = numBlockFrames;
							}
							hkArrayUtil::_reserve(*pHeap, &samples[k], request,
								static_cast<int32_t>(sizeof(hkVector4)));
						}
						samples[k].m_size = numBlockFrames;
					}
					{
						// EXE-INLINED none: hkArray<hkReal>::setSize
						hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
						const int32_t capacity = times[k].m_capacityAndFlags & hkArrayBase<float>::CAPACITY_MASK;
						if (capacity < numBlockFrames) {
							int32_t request = 2 * capacity;
							if (numBlockFrames >= request) {
								request = numBlockFrames;
							}
							hkArrayUtil::_reserve(*pHeap, &times[k], request, static_cast<int32_t>(sizeof(float)));
						}
						times[k].m_size = numBlockFrames;
					}
					hkaSplineCompressedAnimation::Linspace(m, times[k], 0.0f, blockTime);
				}

				// Per component: translation, rotation, scale, or the float.
				float tolerances[3];
				int32_t degrees[3];
				if (trackType == 0) {
					tolerances[0] = params->m_translationTolerance;
					tolerances[1] = params->m_rotationTolerance;
					tolerances[2] = params->m_scaleTolerance;
					degrees[0] = params->m_translationDegree;
					degrees[1] = params->m_rotationDegree;
					degrees[2] = params->m_scaleDegree;
					extractTrackTransforms(raw.m_transforms.m_data, m_numberOfTransformTracks, numBlockFrames,
						firstFrame, track, samples[0], samples[1], samples[2]);
				} else {
					tolerances[0] = params->m_floatingTolerance;
					degrees[0] = params->m_floatingDegree;
					extractTrackFloats(raw.m_floats.m_data, m_numberOfFloatTracks, numBlockFrames, firstFrame, track,
						samples[0]);
				}

				for (int32_t k = 0; k < numComponents; ++k) {
					hkVector4 mean;
					hkVector4 minp;
					hkVector4 maxp;
					calcMean(&mean, samples[k]);
					calcMin(&minp, samples[k]);
					calcMax(&maxp, samples[k]);
					if (k == 1) {
						// EXE-INLINED none: hkVector4::normalize4 (the rotation's mean; 0 for a zero sum)
						const __m128 q = mean.m_quad;
						const __m128 sq = _mm_mul_ps(q, q);
						const __m128 sum2 = _mm_add_ps(_mm_shuffle_ps(sq, sq, 0x4E), sq);
						const __m128 len2 = _mm_add_ps(_mm_shuffle_ps(sum2, sum2, 0xB1), sum2);
						const __m128 r = _mm_rsqrt_ss(len2);
						const __m128 isZero = _mm_cmpeq_ss(len2, _mm_setzero_ps());
						const __m128 t = _mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(len2, r), r));
						const __m128 inv = _mm_andnot_ps(isZero, _mm_mul_ss(_mm_mul_ss(_mm_set_ss(0.5f), r), t));
						mean.m_quad = _mm_mul_ps(_mm_shuffle_ps(inv, inv, 0), q);
					}
					uint8_t mask =
						hkaSplineCompressedAnimation::isStatic(mean, minp, maxp, tolerances[k], clearValues[k]);
					if (trackType == 1) {
						// A float track always has its mask bit.
						if (!mask) {
							mask = 1;
						}
						m_data.m_data[dataOffset] = static_cast<uint8_t>(packMaskAndQuantization(mask,
							params->m_floatQuantizationType.m_storage));
						dataOffset += 1;
					} else if (k == 0) {
						m_data.m_data[dataOffset] =
							static_cast<uint8_t>(packQuantizationTypes(params->m_translationQuantizationType.m_storage,
								params->m_rotationQuantizationType.m_storage,
								params->m_scaleQuantizationType.m_storage));
						m_data.m_data[dataOffset + 1] = mask;
						dataOffset += 2;
					} else {
						m_data.m_data[dataOffset] = mask;
						dataOffset += 1;
					}

					// The fitted curve's knots and control points.
					alignas(16) FNVD_INLINED_LOCAL(hkArray<hkVector4>, points, std::nullopt, std::nullopt);
					{
						// EXE-INLINED none: hkArray<hkVector4>::hkArray(int)
						points.m_data = nullptr;
						points.m_size = 0;
						points.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
						hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
						int32_t count = 3 * numBlockFrames;
						hkVector4* pData = count ? pHeap->_bufAlloc<hkVector4>(count) : nullptr;
						points.m_size = count;
						points.m_capacityAndFlags = count ? count : hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
						points.m_data = pData;
					}
					alignas(16) FNVD_INLINED_LOCAL(hkArray<float>, knots, std::nullopt, std::nullopt);
					{
						// EXE-INLINED none: hkArray<hkReal>::hkArray(int)
						knots.m_data = nullptr;
						knots.m_size = 0;
						knots.m_capacityAndFlags = hkArrayBase<float>::DONT_DEALLOCATE_FLAG;
						hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
						int32_t count = 3 * numBlockFrames + 2;
						float* pData = count ? pHeap->_bufAlloc<float>(count) : nullptr;
						knots.m_data = pData;
						knots.m_size = count;
						knots.m_capacityAndFlags = count ? count : hkArrayBase<float>::DONT_DEALLOCATE_FLAG;
					}
					const uint8_t dynamicBits = mask & 0xF0;
					if (dynamicBits) {
						int32_t numKnots;
						int32_t numPoints;
						ApproximateNURBSCurve(numBlockFrames, times[k].m_data, samples[k].m_data, degrees[k],
							tolerances[k], &numKnots, knots.m_data, &numPoints, points.m_data);
						for (int32_t pass = 0; pass < 2; ++pass) {
							{
								// EXE-INLINED none: hkArray<hkReal>::setSize
								hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
								const int32_t capacity = knots.m_capacityAndFlags & hkArrayBase<float>::CAPACITY_MASK;
								if (capacity < numKnots) {
									int32_t request = 2 * capacity;
									if (numKnots >= request) {
										request = numKnots;
									}
									hkArrayUtil::_reserve(*pHeap, &knots, request, static_cast<int32_t>(sizeof(float)));
								}
								knots.m_size = numKnots;
							}
							{
								// EXE-INLINED none: hkArray<hkVector4>::setSize
								hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
								const int32_t capacity =
									points.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK;
								if (capacity < numPoints) {
									int32_t request = 2 * capacity;
									if (numPoints >= request) {
										request = numPoints;
									}
									hkArrayUtil::_reserve(*pHeap, &points, request,
										static_cast<int32_t>(sizeof(hkVector4)));
								}
								points.m_size = numPoints;
							}
							// More control points than samples: the fit again at degree 1.
							if (pass == 1 || numPoints <= numBlockFrames) {
								break;
							}
							degrees[k] = 1;
							ApproximateNURBSCurve(numBlockFrames, times[k].m_data, samples[k].m_data, 1, tolerances[k],
								&numKnots, knots.m_data, &numPoints, points.m_data);
						}
						writeKnots(numPoints - 1, knots.m_size - numPoints - 1, knots);
					}
					if (k == 1) {
						writeRotation(mask,
							static_cast<TrackCompressionParams::RotationQuantization>(params->m_rotationQuantizationType
									.m_storage),
							mean, points.m_size - 1, points);
					} else {
						if (dynamicBits) {
							hkVector4 range;
							minp = *calcMin(&range, points);
							hkVector4 rangeMax;
							maxp = *calcMax(&rangeMax, points);
						}
						uint8_t type;
						if (trackType) {
							type = params->m_floatQuantizationType.m_storage;
						} else if (k) {
							type = params->m_scaleQuantizationType.m_storage;
						} else {
							type = params->m_translationQuantizationType.m_storage;
						}
						writePoints(mask, static_cast<TrackCompressionParams::ScalarQuantization>(type), mean, minp,
							maxp, points.m_size - 1, points);
					}
					writeAlign(4, &m_data);
					{
						// EXE-INLINED none: hkArray<hkReal>::~hkArray
						hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
						const int32_t capacityAndFlags = knots.m_capacityAndFlags;
						knots.m_size = 0;
						if (!(capacityAndFlags & hkArrayBase<float>::DONT_DEALLOCATE_FLAG)) {
							pHeap->bufFree(knots.m_data,
								(capacityAndFlags & hkArrayBase<float>::CAPACITY_MASK) *
									static_cast<int32_t>(sizeof(float)));
						}
						knots.m_data = nullptr;
						knots.m_capacityAndFlags = hkArrayBase<float>::DONT_DEALLOCATE_FLAG;
					}
					{
						// EXE-INLINED none: hkArray<hkVector4>::~hkArray
						hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
						const int32_t capacityAndFlags = points.m_capacityAndFlags;
						points.m_size = 0;
						if (!(capacityAndFlags & hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG)) {
							pHeap->bufFree(points.m_data,
								(capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK) *
									static_cast<int32_t>(sizeof(hkVector4)));
						}
						points.m_data = nullptr;
						points.m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
					}
				}

				for (int32_t k = 2; k >= 0; --k) {
					// EXE-INLINED none: hkArray<hkReal>::~hkArray
					hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
					const int32_t capacityAndFlags = times[k].m_capacityAndFlags;
					times[k].m_size = 0;
					if (capacityAndFlags >= 0) {
						pHeap->bufFree(times[k].m_data,
							(capacityAndFlags & hkArrayBase<float>::CAPACITY_MASK) *
								static_cast<int32_t>(sizeof(float)));
					}
					times[k].m_data = nullptr;
					times[k].m_capacityAndFlags = hkArrayBase<float>::DONT_DEALLOCATE_FLAG;
				}
				for (int32_t k = 2; k >= 0; --k) {
					// EXE-INLINED none: hkArray<hkVector4>::~hkArray
					hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
					const int32_t capacityAndFlags = samples[k].m_capacityAndFlags;
					samples[k].m_size = 0;
					if (capacityAndFlags >= 0) {
						pHeap->bufFree(samples[k].m_data,
							(capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK) *
								static_cast<int32_t>(sizeof(hkVector4)));
					}
					samples[k].m_data = nullptr;
					samples[k].m_capacityAndFlags = hkArrayBase<hkVector4>::DONT_DEALLOCATE_FLAG;
				}
			}
		}
		firstFrame = firstFrame + numBlockFrames - 1;
		writeAlign(16, &m_data);
	}

	// The report, each line through an hkErrStream on buf (the SDK's HK_REPORT).
	FNVD_INLINED_LOCAL(hkErrStream, os, std::nullopt, std::nullopt);
	alignas(16) char buf[512];
	hkError::s_instance->sectionBegin(0x36118E94, "Compression");
	::new (&os) hkErrStream(buf, 512);
	os << "Number of blocks: " << m_numBlocks;
	hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, buf,
		".\\Animation\\SplineCompressed\\hkaSplineCompressedAnimationCtor.cpp", 366);
	FNVD_DESTRUCT(hkOstream)(&os);
	::new (&os) hkErrStream(buf, 512);
	const int32_t maxChunkSize = static_cast<const hkaAnimation*>(this)->getMaxSizeOfCombinedDataChunks();
	os << "Size of largest block (PS3 SPU): " << maxChunkSize << " bytes.";
	hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, buf,
		".\\Animation\\SplineCompressed\\hkaSplineCompressedAnimationCtor.cpp", 367);
	FNVD_DESTRUCT(hkOstream)(&os);
	// The raw size counts 16 floats per transform.
	const uint32_t originalSize = static_cast<uint32_t>((raw.m_transforms.m_size * 16 + raw.m_floats.m_size) *
		static_cast<int32_t>(sizeof(float)));
	::new (&os) hkErrStream(buf, 512);
	const int32_t compressedSize = hkaSplineCompressedAnimation::getSizeInBytes();
	os << "Original Size: " << originalSize << " bytes. Compressed Size: " << compressedSize << " bytes.";
	hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, buf,
		".\\Animation\\SplineCompressed\\hkaSplineCompressedAnimationCtor.cpp", 372);
	FNVD_DESTRUCT(hkOstream)(&os);
	::new (&os) hkErrStream(buf, 512);
	float fOriginalSize = static_cast<float>(originalSize);
	const float ratio = fOriginalSize / static_cast<double>(hkaSplineCompressedAnimation::getSizeInBytes());
	os << "Compression Ratio: " << ratio << ":1.";
	hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, buf,
		".\\Animation\\SplineCompressed\\hkaSplineCompressedAnimationCtor.cpp", 373);
	FNVD_DESTRUCT(hkOstream)(&os);
	hkError::s_instance->sectionEnd();
}

// KEEP: unregistered for uninitialised reads. A mask with bit 3 set copies stat's w (never read from the data) into
// the control points and out; a mask with bit 7 set leaves every control point's w unwritten, and the evaluator reads
// it; and the exe loads the min and max of all three components (0xE33BCA-0xE33BE8) whichever the mask makes dynamic.
// The body follows the exe otherwise. The mask's low nibble marks static components (one float each), the high
// nibble dynamic ones (a min and a max float each, then 8- or 16-bit control points scaled into that range by the
// qword 255.0 or 65535.0); the rest take I's component. The exe aligns the data to 4 for the floats and to 2 for the
// control points, skips to the span's first control point, and finally steps over all n + 1 of them.
void hkaSplineCompressedAnimation::readNURBSCurve(const uint8_t*& dataInOut, uint8_t quantizedTime, float frameDuration,
	float u, uint8_t mask, TrackCompressionParams::ScalarQuantization scalarQuantizationType, hkVector4& I,
	hkVector4& out) {
	const uint8_t uiMask = mask;
	const uint8_t uiDynamicMask = uiMask & 0xF0;
	int32_t p = 0;
	int32_t n = 0;
	float U[6];
	hkVector4 stat;
	hkVector4 minp;
	hkVector4 maxp;
	hkVector4 P[4];
	int32_t span;
	if (uiDynamicMask) {
		span = readKnots(dataInOut, &n, &p, quantizedTime, frameDuration, U);
	} else {
		span = 0;
		p = 0;
		n = 0;
	}
	// EXE-INLINED none: hkaSplineCompressedAnimation::readAlign(4). The data is raw bytes the exe treats as an address.
	dataInOut = reinterpret_cast<const uint8_t*>((reinterpret_cast<uintptr_t>(dataInOut) + 3) & ~uintptr_t(3));
	float* pStat = &reinterpret_cast<float*>(&stat.m_quad)[0];
	float* pMin = &reinterpret_cast<float*>(&minp.m_quad)[0];
	float* pMax = &reinterpret_cast<float*>(&maxp.m_quad)[0];
	for (int32_t c = 0; c < 3; ++c) {
		// EXE-INLINED none: hkaSplineCompressedAnimation::readReal. The data holds raw floats.
		if (uiMask & (1 << c)) {
			pStat[c] = *reinterpret_cast<const float*>(dataInOut);
			dataInOut += sizeof(float);
		} else if (uiMask & (0x10 << c)) {
			pMin[c] = *reinterpret_cast<const float*>(dataInOut);
			dataInOut += sizeof(float);
			pMax[c] = *reinterpret_cast<const float*>(dataInOut);
			dataInOut += sizeof(float);
		}
	}
	if (!uiDynamicMask) {
		recompose(mask, stat, I, out);
		return;
	}

	const int32_t bytesPerComponent = s_bytesPerComponent[scalarQuantizationType];
	// EXE-INLINED none: hkaSplineCompressedAnimation::readAlign(2)
	dataInOut = reinterpret_cast<const uint8_t*>((reinterpret_cast<uintptr_t>(dataInOut) + 1) & ~uintptr_t(1));
	const int32_t numDynamic = s_bitCount[(uiMask >> 4) & 7];
	const uint8_t* pData = dataInOut + (span - p) * bytesPerComponent * numDynamic;
	if (p >= 0) {
		const uint32_t staticMask = uiMask & 0xFu;
		const uint32_t identityMask = ~static_cast<uint32_t>(uiMask) & (~static_cast<uint32_t>(uiMask) >> 4) & 0xFu;
		for (int32_t i = 0; i <= p; ++i) {
			float* pPoint = &reinterpret_cast<float*>(&P[i].m_quad)[0];
			for (int32_t c = 0; c < 3; ++c) {
				if (!(uiMask & (0x10 << c))) {
					continue;
				}
				int32_t value;
				double scaled;
				if (scalarQuantizationType == TrackCompressionParams::BITS8) {
					value = *pData;
					pData += 1;
					scaled = value / 255.0;
				} else {
					// The data holds raw 16-bit values.
					value = *reinterpret_cast<const uint16_t*>(pData);
					pData += 2;
					scaled = value / 65535.0;
				}
				pPoint[c] = static_cast<float>((pMax[c] - pMin[c]) * scaled + pMin[c]);
			}
			for (uint32_t c = 0; c < 4; ++c) {
				if (staticMask & (1u << c)) {
					pPoint[c] = pStat[c];
				} else if (identityMask & (1u << c)) {
					pPoint[c] = (&reinterpret_cast<float*>(&I.m_quad)[0])[c];
				}
			}
		}
	}
	s_evaluateFunctions[p](u, p, U, P, out);
	dataInOut += numDynamic * bytesPerComponent * (n + 1);
}

// The knot count is a 16-bit value read straight from the packed byte stream; the degree and the knots follow as
// bytes.
int32_t hkaSplineCompressedAnimation::readKnots(const uint8_t*& dataInOut, int32_t* _n, int32_t* _p,
	uint8_t quantizedTime, float frameDuration, float* U) {
	const uint16_t nn = *reinterpret_cast<const uint16_t*>(dataInOut);
	dataInOut += sizeof(uint16_t);
	const int32_t pp = *dataInOut;
	dataInOut += sizeof(uint8_t);
	const uint8_t* knots = dataInOut;
	const int32_t span = findSpan(nn, pp, quantizedTime, knots);
	for (int32_t i = 0; i < 2 * pp; i++) {
		U[i] = static_cast<float>(knots[span - pp + 1 + i]) * frameDuration;
	}
	dataInOut = knots + nn + pp + 2;
	*_n = nn;
	*_p = pp;
	return span;
}

// A mask with dynamic bits (high nibble) reads the knots and the span's packed control points and evaluates them at u;
// one with only static bits reads the one packed quaternion; one with neither gives the identity.
void hkaSplineCompressedAnimation::readNURBSQuaternion(const uint8_t*& dataInOut, uint8_t quantizedTime,
	float frameDuration, float u, uint8_t mask, TrackCompressionParams::RotationQuantization type, hkQuaternion& out) {
	if (mask & 0xF0) {
		int32_t n;
		alignas(16) int32_t p;
		float U[6];
		hkVector4 P[4];
		int32_t span = readKnots(dataInOut, &n, &p, quantizedTime, frameDuration, U);
		readPackedQuaternions(dataInOut, type, n, p, span, P);
		// The evaluator writes the quaternion's four components as a vector.
		s_evaluateFunctions[p](u, p, U, P, out.m_vec);
	} else if (mask & 0x0F) {
		// EXE-INLINED none: hkaSplineCompressedAnimation::readAlignQuaternion. The data is raw bytes the exe treats as
		// an address.
		dataInOut = reinterpret_cast<const uint8_t*>(~uintptr_t(rdata_10EB3B8[type] - 1) &
			(reinterpret_cast<uintptr_t>(dataInOut) + rdata_10EB3B8[type] - 1));
		// EXE-INLINED none: hkaSplineCompressedAnimation::unpackQuaternion
		unpackfunc[type](dataInOut, &out);
		dataInOut += rdata_10EB3D0[type];
	} else {
		out.setIdentity();
	}
}

const hkClass& hkaSplineCompressedAnimation::staticClass() {
	return hkaSplineCompressedAnimationClass;
}

uint8_t hkaSplineCompressedAnimation::isStatic(const hkVector4& mean, const hkVector4& minp, const hkVector4& maxp,
	float tol, const hkVector4& a5) {
	uint8_t v6;
	float v10;
	float v11;
	float v16;
	float v17;
	float v22;
	float v23;
	float v28;
	float v29;
	float v8 = mean.m_quad[0] - minp.m_quad[0];
	float v9 = std::fabs((double)v8);
	double v5 = tol;
	if (!(tol >= static_cast<double>(v9)) ||
		(v10 = mean.m_quad[0] - maxp.m_quad[0], (v11 = std::fabs((double)v10), !(v11 <= v5)))) {
		v6 = 16;
	} else {
		float v12 = mean.m_quad[0] - a5.m_quad[0];
		float v13 = std::fabs((double)v12);
		v6 = v13 > v5;
	}
	float v14 = mean.m_quad[1] - minp.m_quad[1];
	float v15 = std::fabs((double)v14);
	if (!(v15 <= v5) || (v16 = mean.m_quad[1] - maxp.m_quad[1], (v17 = std::fabs((double)v16), !(v17 <= v5)))) {
		v6 |= 32u;
	} else {
		float v18 = mean.m_quad[1] - a5.m_quad[1];
		float v19 = std::fabs((double)v18);
		if (!(v19 <= v5)) {
			v6 |= 2u;
		}
	}
	float v20 = mean.m_quad[2] - minp.m_quad[2];
	float v21 = std::fabs((double)v20);
	if (!(v21 <= v5) || (v22 = mean.m_quad[2] - maxp.m_quad[2], (v23 = std::fabs((double)v22), !(v23 <= v5)))) {
		v6 |= 64u;
	} else {
		float v24 = mean.m_quad[2] - a5.m_quad[2];
		float v25 = std::fabs((double)v24);
		if (!(v25 <= v5)) {
			v6 |= 4u;
		}
	}
	float v26 = mean.m_quad[3] - minp.m_quad[3];
	float v27 = std::fabs((double)v26);
	if (!(v27 <= v5) || (v28 = mean.m_quad[3] - maxp.m_quad[3], (v29 = std::fabs((double)v28), !(v29 <= v5)))) {
		v6 |= 128u;
	} else {
		float v30 = mean.m_quad[3] - a5.m_quad[3];
		float v31 = std::fabs((double)v30);
		if (!(v31 <= v5)) {
			return v6 | 8;
		}
	}
	return v6;
}

void hkaSplineCompressedAnimation::reverseEndian(int32_t bytes, uint8_t*& dataInOut) {
	switch (bytes) {
	case 1: {
		++dataInOut;
		break;
	}
	case 2: {
		uint8_t* v2 = dataInOut;
		uint8_t v3 = *dataInOut;
		*v2 = dataInOut[1];
		v2[1] = v3;
		dataInOut += 2;
		break;
	}
	case 4: {
		uint8_t* v4 = dataInOut;
		uint8_t v5 = *dataInOut;
		*v4 = dataInOut[3];
		v4[3] = v5;
		uint8_t* v6 = dataInOut;
		uint8_t v7 = dataInOut[1];
		v6[1] = dataInOut[2];
		v6[2] = v7;
		dataInOut += 4;
		break;
	}
	default: {
		dataInOut += bytes;
		break;
	}
	}
}

// Swaps the knot count, a 16-bit value of the packed byte stream, and steps over the degree byte and the n + p + 2
// knots; returns the knot count, or -1 when the track has no dynamic component.
int32_t hkaSplineCompressedAnimation::handleEndianKnots(int32_t mask, uint8_t*& dataInOut) {
	if ((mask & 0xF0) == 0) {
		return -1;
	}
	reverseEndian(2, dataInOut);
	const int32_t n = reinterpret_cast<const uint16_t*>(dataInOut)[-1];
	const int32_t p = *dataInOut;
	dataInOut += p + n + 3;
	return n;
}

// Swaps a scalar track's control points: aligns to 4, swaps the 4-byte static value of each component with only a
// static bit (mask bits 0-2) and the 4-byte minimum and maximum of each dynamic one (bits 4-6), then the n + 1
// quantized values (1 byte for BITS8, else 2) of each dynamic component.
void hkaSplineCompressedAnimation::handleEndianScalarControlPoints(int32_t mask, int32_t n,
	hkaSplineCompressedAnimation::TrackCompressionParams::ScalarQuantization type, uint8_t*& dataInOut) {
	// Tests the mask's low byte only.
	if (static_cast<uint8_t>(mask)) {
		// The stream is untyped bytes: the pointer itself is rounded up to a multiple of 4.
		dataInOut = reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(dataInOut + 3) & ~uintptr_t(3));
		int32_t numDynamic = 0;
		for (int32_t i = 0; i < 3; i++) {
			if (mask & (0x10 << i)) {
				reverseEndian(4, dataInOut);
				reverseEndian(4, dataInOut);
				++numDynamic;
			} else if (mask & (1 << i)) {
				reverseEndian(4, dataInOut);
			}
		}
		if (numDynamic) {
			const int32_t bytes = (type != TrackCompressionParams::BITS8) + 1;
			for (int32_t i = 0; i <= n; i++) {
				for (int32_t c = 0; c < numDynamic; c++) {
					reverseEndian(bytes, dataInOut);
				}
			}
		}
	}
}

void hkaSplineCompressedAnimation::Linspace(int32_t m, hkArray<float, hkContainerHeapAllocator>& ub, float a, float b) {
	double v4 = a;
	float* m_data = (&ub)->begin();
	*m_data = a;
	double v6 = b;
	m_data[m] = b;
	int32_t v7 = 1;
	int32_t a2 = 1;
	if (m - 1 >= 4) {
		int32_t v8 = 3;
		float a3 = static_cast<float>(m);
		int32_t a4 = 3;
		float* v9 = m_data + 3;
		double v10 = v6 - v4;
		double v11 = a3;
		do {
			double v12 = static_cast<double>(a2);
			int32_t a2a = v8 - 1;
			int32_t v13 = v8 + 1;
			v7 += 4;
			v8 += 4;
			v9 += 4;
			*(v9 - 6) = v12 / v11 * v10 + v4;
			*(v9 - 5) = static_cast<double>(a2a) / v11 * v10 + v4;
			double v14 = static_cast<double>(a4);
			a4 = v8;
			*(v9 - 4) = v14 / v11 * v10 + v4;
			a2 = v7;
			*(v9 - 3) = static_cast<double>(v13) / v11 * v10 + v4;
		} while (v7 < m - 3);
	}
	if (v7 < m) {
		float* v15 = &m_data[v7];
		double v16 = v6 - v4;
		do {
			++v7;
			++v15;
			float a3a = static_cast<float>(m);
			double v17 = static_cast<double>(a2) / a3a;
			a2 = v7;
			*(v15 - 1) = v17 * v16 + v4;
		} while (v7 < m);
	}
}

void hkaSplineCompressedAnimation::handleEndianScalar(int32_t mask,
	hkaSplineCompressedAnimation::TrackCompressionParams::ScalarQuantization type, uint8_t*& dataInOut) {
	// Tests the mask's low byte and passes the whole mask on.
	if (static_cast<uint8_t>(mask)) {
		int32_t v3 = hkaSplineCompressedAnimation::handleEndianKnots(mask, dataInOut);
		hkaSplineCompressedAnimation::handleEndianScalarControlPoints(mask, v3, type, dataInOut);
		dataInOut = reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>((dataInOut + 3)) & ~uintptr_t(3));
	}
}

void hkaSplineCompressedAnimation::handleEndianRotation(int32_t mask,
	hkaSplineCompressedAnimation::TrackCompressionParams::RotationQuantization type, uint8_t*& dataInOut) {
	// Tests the mask's low byte and passes the whole mask on.
	if (static_cast<uint8_t>(mask)) {
		int32_t v3 = hkaSplineCompressedAnimation::handleEndianKnots(mask, dataInOut);
		handleEndianRotationControlPoints(mask, v3, type, dataInOut);
		dataInOut = reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>((dataInOut + 3)) & ~uintptr_t(3));
	}
}

void hkaSplineCompressedAnimation::write16(uint16_t a1, hkArray<uint8_t, hkContainerHeapAllocator>& a2) {
	padZeros(2, &a2);
	// The array is an untyped byte stream: the value is stored into its last two (unaligned) bytes.
	*reinterpret_cast<uint16_t*>(&a2.m_data[a2.m_size - 2]) = a1;
}

void hkaSplineCompressedAnimation::writeReal(float a1, hkArray<uint8_t, hkContainerHeapAllocator>& a2) {
	padZeros(4, &a2);
	// The array is an untyped byte stream: the float is stored into its last four (unaligned) bytes.
	*reinterpret_cast<float*>(&a2.m_data[a2.m_size - 4]) = a1;
}

// Aligns dataInOut to the type's packed quaternion alignment (rdata_10EB3B8) and unpacks the p + 1 control points of
// span from the n + 1 packed ones of rdata_10EB3D0 bytes each, then steps over all of them.
void hkaSplineCompressedAnimation::readPackedQuaternions(const uint8_t*& dataInOut,
	hkaSplineCompressedAnimation::TrackCompressionParams::RotationQuantization type, int32_t n, int32_t p, int32_t span,
	hkVector4* P) {
	const int32_t align = rdata_10EB3B8[type];
	dataInOut = reinterpret_cast<const uint8_t*>((reinterpret_cast<uintptr_t>(dataInOut) + align - 1) &
		~static_cast<uintptr_t>(align - 1));
	const int32_t size = rdata_10EB3D0[type];
	for (int32_t i = 0; i <= p; i++) {
		// EXE-INLINED none: hkaSplineCompressedAnimation::unpackQuaternion
		// The SDK's source unpacks each quaternion into the control point's storage: the unpacker writes the
		// hkVector4 as raw memory.
		unpackfunc[type](dataInOut + (span - p + i) * size, reinterpret_cast<hkQuaternion*>(&P[i]));
	}
	dataInOut += (n + 1) * size;
}

void hkaSplineCompressedAnimation::evaluateSimple2(float time, int32_t p, float* const a3, hkVector4* const a4,
	hkVector4& a5) {
	uint32_t v15;
	uint32_t v17;
	__m128 v5 = _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)));
	*reinterpret_cast<float*>(&v15) = time - a3[1];
	__m128 v6 = _mm_set_ss(std::bit_cast<float>(v15));
	float v16 = a3[2] - time;
	float v8 = 1.0f / static_cast<float>(v16 + v6[0]);
	*reinterpret_cast<float*>(&v17) = time - *a3;
	__m128 v10 = _mm_set_ss(std::bit_cast<float>(v17));
	float v11 = static_cast<float>(v16 * v8) + 0.0f;
	float v12 = v16 + *reinterpret_cast<float*>(&v17);
	float v18 = a3[3] - time;
	float v13 = v11 / v12;
	v5[0] = static_cast<float>(v16 * v13) + 0.0f;
	float v14 = static_cast<float>(v6[0] * v8) / static_cast<float>(v18 + v6[0]);
	v6[0] = v6[0] * v14;
	v10[0] = static_cast<float>(v10[0] * v13) + static_cast<float>(v18 * v14);
	a5.m_quad = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v10, v10, 0), a4[1].m_quad),
							   _mm_mul_ps(_mm_shuffle_ps(v5, v5, 0), a4->m_quad)),
		_mm_mul_ps(_mm_shuffle_ps(v6, v6, 0), a4[2].m_quad));
}

void hkaSplineCompressedAnimation::evaluateSimple3(float time, int32_t p, float* const a3, hkVector4* const a4,
	hkVector4& a5) {
	uint32_t v20;
	uint32_t v22;
	uint32_t v24;
	__m128 v5 = _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)));
	*reinterpret_cast<float*>(&v20) = time - a3[2];
	__m128 v6 = _mm_set_ss(std::bit_cast<float>(v20));
	float v21 = a3[3] - time;
	float v8 = 1.0f / static_cast<float>(v21 + v6[0]);
	float v9 = v21 * v8;
	*reinterpret_cast<float*>(&v22) = time - a3[1];
	__m128 v10 = _mm_set_ss(std::bit_cast<float>(v22));
	float v11 = v9 + 0.0f;
	float v12 = v21 + *reinterpret_cast<float*>(&v22);
	float v23 = a3[4] - time;
	float v26 = v11 / v12;
	float v14 = v23 + v6[0];
	*reinterpret_cast<float*>(&v24) = time - *a3;
	__m128 v27 = _mm_set_ss(std::bit_cast<float>(v24));
	float v15 = static_cast<float>(v6[0] * v8) / v14;
	float v25 = a3[5] - time;
	float v16 = static_cast<float>(static_cast<float>(v21 * v26) + 0.0f) / static_cast<float>(v21 + v27[0]);
	v5[0] = static_cast<float>(v21 * v16) + 0.0f;
	float v17 = static_cast<float>(static_cast<float>(v10[0] * v26) + static_cast<float>(v23 * v15)) /
		static_cast<float>(v23 + v10[0]);
	v27[0] = static_cast<float>(v27[0] * v16) + static_cast<float>(v23 * v17);
	float v19 = static_cast<float>(v6[0] * v15) / static_cast<float>(v25 + v6[0]);
	v6[0] = v6[0] * v19;
	v10[0] = static_cast<float>(v10[0] * v17) + static_cast<float>(v25 * v19);
	a5.m_quad = _mm_add_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(v27, v27, 0), a4[1].m_quad),
										  _mm_mul_ps(_mm_shuffle_ps(v5, v5, 0), a4->m_quad)),
							   _mm_mul_ps(_mm_shuffle_ps(v10, v10, 0), a4[2].m_quad)),
		_mm_mul_ps(_mm_shuffle_ps(v6, v6, 0), a4[3].m_quad));
}

void hkaSplineCompressedAnimation::evaluateSimple1(float u, int32_t p, float* U, hkVector4* P, hkVector4& out) {
	uint32_t v8;
	*reinterpret_cast<float*>(&v8) = u - *U;
	__m128 v5 = _mm_set_ss(std::bit_cast<float>(v8));
	float v9 = U[1] - u;
	v5[0] = v5[0] / static_cast<float>(v9 + v5[0]);
	__m128 v6 = _mm_shuffle_ps(v5, v5, 0);
	out.m_quad = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, v6), P->m_quad), _mm_mul_ps(P[1].m_quad, v6));
}

int32_t hkaSplineCompressedAnimation::findSpan(int32_t a1, int32_t a2, uint8_t a3, const uint8_t* a4) {
	int32_t result = a1;
	if (a3 < a4[a1 + 1]) {
		if (a3 > *a4) {
			int32_t v5 = a1 + 1;
			int32_t v6 = a2;
			int32_t v7 = a1 + 1 + a2;
			while (1) {
				result = v7 / 2;
				uint8_t v8 = a4[result];
				if (a3 < v8) {
					goto LABEL_8;
				}
				if (a3 < a4[result + 1]) {
					return result;
				}
				if (a3 >= v8) {
					v6 = result;
					v7 = v5 + result;
				} else {
				LABEL_8:;
					v5 = result;
					v7 = result + v6;
				}
			}
		}
		return a2;
	}
	return result;
}

// The SDK's samplePartialWithDataChunks: chunk 0 is the animation, chunk 1 its data from the block's start (the
// per-track masks first, then the transform values after m_maskAndQuantizationSize bytes, the float masks at four
// bytes per transform track), chunk 2 the block's float offset. Each transform track's mask byte packs the
// translation (bits 0-1), rotation (bits 2-5) and scale (bits 6-7) quantizations; each float track's mask byte its
// quantization in bits 1-2.
void hkaSplineCompressedAnimation::samplePartialWithDataChunks(uint32_t frameIndex, float frameDelta,
	uint32_t maxNumTransformTracks, hkQsTransform* transformTracksOut, uint32_t maxNumFloatTracks,
	float* floatTracksOut, const hkaAnimation::DataChunk* dataChunks, int32_t numDataChunks) {
	const hkaSplineCompressedAnimation* anim = static_cast<const hkaSplineCompressedAnimation*>(dataChunks[0].m_data);
	int32_t block;
	float blockTime;
	uint8_t quantizedTime;
	anim->getBlockAndTime(frameIndex, frameDelta, block, blockTime, quantizedTime);
	const uint8_t* mask = static_cast<const uint8_t*>(dataChunks[1].m_data) + dataChunks[1].m_offset;
	const uint8_t* dataInOut = mask + anim->m_maskAndQuantizationSize;
	for (uint32_t i = 0; i < maxNumTransformTracks; ++i) {
		const uint8_t quantizationTypes = mask[0];
		hkQsTransform& out = transformTracksOut[i];
		anim->sampleTranslation(blockTime, quantizedTime, mask[1],
			static_cast<TrackCompressionParams::ScalarQuantization>(quantizationTypes & 3), dataInOut,
			out.m_translation);
		anim->sampleRotation(blockTime, quantizedTime, mask[2],
			static_cast<TrackCompressionParams::RotationQuantization>((quantizationTypes >> 2) & 0xF), dataInOut,
			out.m_rotation);
		anim->sampleScale(blockTime, quantizedTime, mask[3],
			static_cast<TrackCompressionParams::ScalarQuantization>(quantizationTypes >> 6), dataInOut, out.m_scale);
		mask += 4;
	}
	const uint8_t* blockData = static_cast<const uint8_t*>(dataChunks[1].m_data) + dataChunks[1].m_offset;
	// Chunk 2 is the block's float offset, a 32-bit value read from the chunk's bytes.
	const uint32_t floatOffset =
		*reinterpret_cast<const uint32_t*>(static_cast<const uint8_t*>(dataChunks[2].m_data) + dataChunks[2].m_offset);
	const uint8_t* floatDataInOut = blockData + floatOffset;
	const uint8_t* floatMask = blockData + anim->m_numberOfTransformTracks * 4;
	for (uint32_t i = 0; i < maxNumFloatTracks; ++i) {
		const uint8_t floatMaskByte = *floatMask++;
		anim->sampleFloat(blockTime, quantizedTime, floatMaskByte & 0xF9,
			static_cast<TrackCompressionParams::ScalarQuantization>((floatMaskByte >> 1) & 3), floatDataInOut,
			floatTracksOut[i]);
	}
}
