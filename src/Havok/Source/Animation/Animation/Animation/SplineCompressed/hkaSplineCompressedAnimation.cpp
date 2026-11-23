#include "Havok/Source/Animation/Animation/Animation/SplineCompressed/hkaSplineCompressedAnimation.h"

#include "Havok/Source/Animation/Animation/Animation/Interleaved/hkaInterleavedUncompressedAnimation.h"
#include "Havok/Source/Animation/Animation/Animation/hkaAnimation.h"
#include "Havok/Source/Animation/Animation/Animation/hkaAnnotationTrackClass.h"
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
#include "xfnv/Compiler.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

// Extracts one transform track's frames into translation, rotation and scale arrays.
static void extractTrackTransforms(
    const hkQsTransform* transforms,
    int32_t numTracks,
    int32_t numFrames,
    int32_t firstFrame,
    int32_t track,
    hkArray<hkVector4, hkContainerHeapAllocator>& translations,
    hkArray<hkVector4, hkContainerHeapAllocator>& rotations,
    hkArray<hkVector4, hkContainerHeapAllocator>& scales
);
// One float track's frames into the x of values.
static void extractTrackFloats(
    const float* floats,
    int32_t numTracks,
    int32_t numFrames,
    int32_t firstFrame,
    int32_t track,
    hkArray<hkVector4, hkContainerHeapAllocator>& values
);
static void finishLoadedObjecthkaSplineCompressedAnimation(void* p, int32_t finishing);
static void cleanupLoadedObjecthkaSplineCompressedAnimation(void* p);
static int32_t packMaskAndQuantization(uint8_t mask, uint8_t floatQuantization);
static int32_t packQuantizationTypes(int32_t position, int32_t rotation, int32_t scale);
static hkVector4 calcMean(const hkArray<hkVector4>& arPoints);
// The componentwise minimum of the points.
static hkVector4 calcMin(const hkArray<hkVector4>& arPoints);
// The componentwise maximum of the points.
static hkVector4 calcMax(const hkArray<hkVector4>& arPoints);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkaSplineCompressedAnimationTrackCompressionParamsTypeInfo = {
    "hkaSplineCompressedAnimationTrackCompressionParams",
    nullptr,
    cleanupLoadedObjecthkaSplineCompressedAnimationTrackCompressionParams,
    nullptr,
    &typeid(hkaSplineCompressedAnimation::TrackCompressionParams),
    sizeof(hkaSplineCompressedAnimation::TrackCompressionParams)
};
hkTypeInfo hkaSplineCompressedAnimationAnimationCompressionParamsTypeInfo = {
    "hkaSplineCompressedAnimationAnimationCompressionParams",
    nullptr,
    cleanupLoadedObjecthkaSplineCompressedAnimationAnimationCompressionParams,
    nullptr,
    &typeid(hkaSplineCompressedAnimation::AnimationCompressionParams),
    sizeof(hkaSplineCompressedAnimation::AnimationCompressionParams)
};

// The reflected enums of hkaSplineCompressedAnimation::TrackCompressionParams.
using hkaSplineCompressedAnimationTrackCompressionParamsRotationQuantizationEnumItemsArray = const hkClassEnum::Item[6];
static hkaSplineCompressedAnimationTrackCompressionParamsRotationQuantizationEnumItemsArray
    hkaSplineCompressedAnimationTrackCompressionParamsRotationQuantizationEnumItems{
        {hkaSplineCompressedAnimation::TrackCompressionParams::POLAR32, "POLAR32"},
        {hkaSplineCompressedAnimation::TrackCompressionParams::THREECOMP40, "THREECOMP40"},
        {hkaSplineCompressedAnimation::TrackCompressionParams::THREECOMP48, "THREECOMP48"},
        {hkaSplineCompressedAnimation::TrackCompressionParams::THREECOMP24, "THREECOMP24"},
        {hkaSplineCompressedAnimation::TrackCompressionParams::STRAIGHT16, "STRAIGHT16"},
        {hkaSplineCompressedAnimation::TrackCompressionParams::UNCOMPRESSED, "UNCOMPRESSED"}
};
using hkaSplineCompressedAnimationTrackCompressionParamsScalarQuantizationEnumItemsArray = const hkClassEnum::Item[2];
static hkaSplineCompressedAnimationTrackCompressionParamsScalarQuantizationEnumItemsArray
    hkaSplineCompressedAnimationTrackCompressionParamsScalarQuantizationEnumItems{
        {hkaSplineCompressedAnimation::TrackCompressionParams::BITS8, "BITS8"},
        {hkaSplineCompressedAnimation::TrackCompressionParams::BITS16, "BITS16"}
};
using hkaSplineCompressedAnimationTrackCompressionParamsEnumsArray = const hkClassEnum[2];
static hkaSplineCompressedAnimationTrackCompressionParamsEnumsArray
    hkaSplineCompressedAnimationTrackCompressionParamsEnums{
        {"RotationQuantization",
         hkaSplineCompressedAnimationTrackCompressionParamsRotationQuantizationEnumItems,
         6,
         nullptr,
         0},
        {"ScalarQuantization",
         hkaSplineCompressedAnimationTrackCompressionParamsScalarQuantizationEnumItems,
         2,
         nullptr,
         0}
};

// The reflected members of hkaSplineCompressedAnimation::AnimationCompressionParams and of
// hkaSplineCompressedAnimation.
using hkaSplineCompressedAnimation_AnimationCompressionParamsClass_MembersArray = const hkClassMember[2];
static hkaSplineCompressedAnimation_AnimationCompressionParamsClass_MembersArray
    hkaSplineCompressedAnimation_AnimationCompressionParamsClass_Members{
        {"maxFramesPerBlock",
         nullptr,
         nullptr,
         hkClassMember::TYPE_UINT16,
         hkClassMember::TYPE_VOID,
         0,
         0,
         0x0,
         nullptr},
        {"enableSampleSingleTracks",
         nullptr,
         nullptr,
         hkClassMember::TYPE_BOOL,
         hkClassMember::TYPE_VOID,
         0,
         0,
         0x2,
         nullptr}
};
using hkaSplineCompressedAnimationClass_MembersArray = const hkClassMember[13];
static hkaSplineCompressedAnimationClass_MembersArray hkaSplineCompressedAnimationClass_Members{
    {"numFrames", nullptr, nullptr, hkClassMember::TYPE_INT32, hkClassMember::TYPE_VOID, 0, 0, 0x28, nullptr},
    {"numBlocks", nullptr, nullptr, hkClassMember::TYPE_INT32, hkClassMember::TYPE_VOID, 0, 0, 0x2C, nullptr},
    {"maxFramesPerBlock", nullptr, nullptr, hkClassMember::TYPE_INT32, hkClassMember::TYPE_VOID, 0, 0, 0x30, nullptr},
    {"maskAndQuantizationSize",
     nullptr,
     nullptr,
     hkClassMember::TYPE_INT32,
     hkClassMember::TYPE_VOID,
     0,
     0,
     0x34,
     nullptr},
    {"blockDuration", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x38, nullptr},
    {"blockInverseDuration", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x3C, nullptr},
    {"frameDuration", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x40, nullptr},
    {"blockOffsets", nullptr, nullptr, hkClassMember::TYPE_ARRAY, hkClassMember::TYPE_UINT32, 0, 0, 0x44, nullptr},
    {"floatBlockOffsets", nullptr, nullptr, hkClassMember::TYPE_ARRAY, hkClassMember::TYPE_UINT32, 0, 0, 0x50, nullptr},
    {"transformOffsets", nullptr, nullptr, hkClassMember::TYPE_ARRAY, hkClassMember::TYPE_UINT32, 0, 0, 0x5C, nullptr},
    {"floatOffsets", nullptr, nullptr, hkClassMember::TYPE_ARRAY, hkClassMember::TYPE_UINT32, 0, 0, 0x68, nullptr},
    {"data", nullptr, nullptr, hkClassMember::TYPE_ARRAY, hkClassMember::TYPE_UINT8, 0, 0, 0x74, nullptr},
    {"endian", nullptr, nullptr, hkClassMember::TYPE_INT32, hkClassMember::TYPE_VOID, 0, 0, 0x80, nullptr}
};

// The reflected members of hkaSplineCompressedAnimation::TrackCompressionParams: its four quantization members name
// their enums.
using hkaSplineCompressedAnimation_TrackCompressionParamsClass_MembersArray = hkClassMember[12];
static hkaSplineCompressedAnimation_TrackCompressionParamsClass_MembersArray
    hkaSplineCompressedAnimation_TrackCompressionParamsClass_Members{
        {"rotationTolerance", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x0, nullptr},
        {"translationTolerance",
         nullptr,
         nullptr,
         hkClassMember::TYPE_REAL,
         hkClassMember::TYPE_VOID,
         0,
         0,
         0x4,
         nullptr},
        {"scaleTolerance", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x8, nullptr},
        {"floatingTolerance", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0xC, nullptr},
        {"rotationDegree", nullptr, nullptr, hkClassMember::TYPE_UINT16, hkClassMember::TYPE_VOID, 0, 0, 0x10, nullptr},
        {"translationDegree",
         nullptr,
         nullptr,
         hkClassMember::TYPE_UINT16,
         hkClassMember::TYPE_VOID,
         0,
         0,
         0x12,
         nullptr},
        {"scaleDegree", nullptr, nullptr, hkClassMember::TYPE_UINT16, hkClassMember::TYPE_VOID, 0, 0, 0x14, nullptr},
        {"floatingDegree", nullptr, nullptr, hkClassMember::TYPE_UINT16, hkClassMember::TYPE_VOID, 0, 0, 0x16, nullptr},
        {"rotationQuantizationType",
         nullptr,
         &hkaSplineCompressedAnimationTrackCompressionParamsEnums[0],
         hkClassMember::TYPE_ENUM,
         hkClassMember::TYPE_UINT8,
         0,
         0,
         0x18,
         nullptr},
        {"translationQuantizationType",
         nullptr,
         &hkaSplineCompressedAnimationTrackCompressionParamsEnums[1],
         hkClassMember::TYPE_ENUM,
         hkClassMember::TYPE_UINT8,
         0,
         0,
         0x19,
         nullptr},
        {"scaleQuantizationType",
         nullptr,
         &hkaSplineCompressedAnimationTrackCompressionParamsEnums[1],
         hkClassMember::TYPE_ENUM,
         hkClassMember::TYPE_UINT8,
         0,
         0,
         0x1A,
         nullptr},
        {"floatQuantizationType",
         nullptr,
         &hkaSplineCompressedAnimationTrackCompressionParamsEnums[1],
         hkClassMember::TYPE_ENUM,
         hkClassMember::TYPE_UINT8,
         0,
         0,
         0x1B,
         nullptr}
};

hkClass hkaSplineCompressedAnimationTrackCompressionParamsClass(
    "hkaSplineCompressedAnimationTrackCompressionParams",
    nullptr,
    28,
    nullptr,
    0,
    hkaSplineCompressedAnimationTrackCompressionParamsEnums,
    2,
    hkaSplineCompressedAnimation_TrackCompressionParamsClass_Members,
    12,
    nullptr,
    nullptr,
    0,
    0
);
hkClass hkaSplineCompressedAnimationAnimationCompressionParamsClass(
    "hkaSplineCompressedAnimationAnimationCompressionPa"
    "rams",
    nullptr,
    4,
    nullptr,
    0,
    nullptr,
    0,
    hkaSplineCompressedAnimation_AnimationCompressionParamsClass_Members,
    2,
    nullptr,
    nullptr,
    0,
    0
);
hkClass hkaSplineCompressedAnimationClass(
    "hkaSplineCompressedAnimation",
    &hkaAnimationClass,
    132,
    nullptr,
    0,
    nullptr,
    0,
    hkaSplineCompressedAnimationClass_Members,
    13,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkaSplineCompressedAnimationTypeInfo = {
    "hkaSplineCompressedAnimation",
    finishLoadedObjecthkaSplineCompressedAnimation,
    cleanupLoadedObjecthkaSplineCompressedAnimation,
    getVtablehkaSplineCompressedAnimation(),
    &typeid(hkaSplineCompressedAnimation),
    sizeof(hkaSplineCompressedAnimation)
};

// Track `track` of numFrames frames from firstFrame of the interleaved transforms (numTracks per frame) into the
// translation, rotation and scale arrays; the translation's and the scale's w are zero.
static void extractTrackTransforms(
    const hkQsTransform* transforms,
    int32_t numTracks,
    int32_t numFrames,
    int32_t firstFrame,
    int32_t track,
    hkArray<hkVector4, hkContainerHeapAllocator>& translations,
    hkArray<hkVector4, hkContainerHeapAllocator>& rotations,
    hkArray<hkVector4, hkContainerHeapAllocator>& scales
) {
    const hkQsTransform* t = transforms + (track + firstFrame * numTracks);
    for (int32_t i = 0; i < numFrames; i++) {
        translations.m_data[i]
            .m_quad = _mm_setr_ps(t->m_translation(0), t->m_translation(1), t->m_translation(2), 0.0f);
        rotations.m_data[i].m_quad = _mm_setr_ps(
            t->m_rotation.m_vec(0),
            t->m_rotation.m_vec(1),
            t->m_rotation.m_vec(2),
            t->m_rotation.m_vec(3)
        );
        scales.m_data[i].m_quad = _mm_setr_ps(t->m_scale(0), t->m_scale(1), t->m_scale(2), 0.0f);
        t += numTracks;
    }
}

// Float track `track` of numFrames frames from firstFrame of the interleaved floats (numTracks per frame) into the
// x of values, its y, z and w zero.
static void extractTrackFloats(
    const float* floats,
    int32_t numTracks,
    int32_t numFrames,
    int32_t firstFrame,
    int32_t track,
    hkArray<hkVector4, hkContainerHeapAllocator>& values
) {
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

static int32_t packMaskAndQuantization(uint8_t mask, uint8_t floatQuantization) {
    return mask | (2 * floatQuantization);
}

static int32_t packQuantizationTypes(int32_t position, int32_t rotation, int32_t scale) {
    return position | 4 * (rotation | 16 * scale);
}

// The average of the points: their sum times the reciprocal of their count (computed on the x87 stack and rounded to
// a float).
static hkVector4 calcMean(const hkArray<hkVector4>& arPoints) {
    hkVector4 result;
    const int32_t iSize = arPoints.m_size;
    hkQuadReal kSum = _mm_setzero_ps();
    result.m_quad = kSum;
    if (iSize > 0) {
        const hkVector4* pPoint = arPoints.m_data;
        for (int32_t i = iSize; i != 0; --i) {
            kSum = _mm_add_ps(pPoint->m_quad, kSum);
            ++pPoint;
        }
        result.m_quad = kSum;
    }
    const float fInv = 1.0 / static_cast<double>(iSize);
    result.m_quad = _mm_mul_ps(_mm_set_ps1(fInv), result.m_quad);
    return result;
}

// The componentwise minimum of the points; the first point is copied even when the array is empty.
static hkVector4 calcMin(const hkArray<hkVector4>& arPoints) {
    hkVector4 result;
    const hkVector4* pPoint = arPoints.m_data;
    const int32_t iSize = arPoints.m_size;
    result = pPoint[0];
    if (iSize > 1) {
        hkQuadReal kMin = result.m_quad;
        for (int32_t i = iSize - 1; i != 0; --i) {
            ++pPoint;
            kMin = _mm_min_ps(kMin, pPoint->m_quad);
        }
        result.m_quad = kMin;
    }
    return result;
}

// The componentwise maximum of the points; the first point is copied even when the array is empty.
static hkVector4 calcMax(const hkArray<hkVector4>& arPoints) {
    hkVector4 result;
    const hkVector4* pPoint = arPoints.m_data;
    const int32_t iSize = arPoints.m_size;
    result = pPoint[0];
    if (iSize > 1) {
        hkQuadReal kMax = result.m_quad;
        for (int32_t i = iSize - 1; i != 0; --i) {
            ++pPoint;
            kMax = _mm_max_ps(kMax, pPoint->m_quad);
        }
        result.m_quad = kMax;
    }
    return result;
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
  : m_rotationTolerance(0.001f), m_translationTolerance(0.001f), m_scaleTolerance(0.001f), m_floatingTolerance(0.001f),
    m_rotationDegree(3), m_translationDegree(3), m_scaleDegree(3), m_floatingDegree(3) {
    m_rotationQuantizationType = THREECOMP40;
    m_translationQuantizationType = BITS16;
    m_scaleQuantizationType = BITS16;
    m_floatQuantizationType = BITS16;
}

// Every quantization type must be valid and no tolerance may be negative; each accuracy check only reports. The four
// validity calls short-circuit in the order translation, rotation, scale, float; the
// accuracy checks run whatever they returned, in the order translation, scale, float, rotation. The tolerance is
// widened to double for the compare and the rotation accuracy is multiplied by the double 0.16260161995887756 first.
// Each report goes through a local hkErrStream (0x200 byte buffer, id 0xABBA5A61).
hkBool hkaSplineCompressedAnimation::TrackCompressionParams::isOk() const {
    const bool bQuantizationValid = validQuantization(
                                        static_cast<ScalarQuantization>(m_translationQuantizationType.m_storage)
                                    )
                                        .m_bool &&
        validQuantization(static_cast<RotationQuantization>(m_rotationQuantizationType.m_storage)).m_bool &&
        validQuantization(static_cast<ScalarQuantization>(m_scaleQuantizationType.m_storage)).m_bool &&
        validQuantization(static_cast<ScalarQuantization>(m_floatQuantizationType.m_storage)).m_bool;
    const bool bTolerancesValid = m_translationTolerance >= 0.0f &&
        m_rotationTolerance >= 0.0f &&
        m_scaleTolerance >= 0.0f &&
        m_floatingTolerance >= 0.0f;

    if (approximateQuantizationAccuracy(static_cast<ScalarQuantization>(m_translationQuantizationType.m_storage)) >
        m_translationTolerance) {
        alignas(4) uint8_t kStreamStorage[sizeof(hkErrStream)];
        char acBuffer[0x200];
        hkErrStream* pStream = ::new (kStreamStorage) hkErrStream(acBuffer, 0x200);
        *pStream << "Quantization may be limiting translation quality.  If artifacts are visible, try increasing "
                    "the number of translation quantization bits.";
        hkError::s_instance->message(
            hkError::MESSAGE_WARNING,
            static_cast<int32_t>(0xABBA5A61u),
            acBuffer,
            ".\\Animation\\SplineCompressed\\hkaSplineCompressedAnimation.cpp",
            0x4F
        );
        pStream->~hkErrStream();
    }

    if (approximateQuantizationAccuracy(static_cast<ScalarQuantization>(m_scaleQuantizationType.m_storage)) >
        m_scaleTolerance) {
        alignas(4) uint8_t kStreamStorage[sizeof(hkErrStream)];
        char acBuffer[0x200];
        hkErrStream* pStream = ::new (kStreamStorage) hkErrStream(acBuffer, 0x200);
        *pStream << "Quantization may be limiting scale quality.  If artifacts are visible, try increasing the "
                    "number of scale quantization bits.";
        hkError::s_instance->message(
            hkError::MESSAGE_WARNING,
            static_cast<int32_t>(0xABBA5A61u),
            acBuffer,
            ".\\Animation\\SplineCompressed\\hkaSplineCompressedAnimation.cpp",
            0x54
        );
        pStream->~hkErrStream();
    }

    if (approximateQuantizationAccuracy(static_cast<ScalarQuantization>(m_floatQuantizationType.m_storage)) >
        m_floatingTolerance) {
        alignas(4) uint8_t kStreamStorage[sizeof(hkErrStream)];
        char acBuffer[0x200];
        hkErrStream* pStream = ::new (kStreamStorage) hkErrStream(acBuffer, 0x200);
        *pStream << "Quantization may be limiting float quality.  If artifacts are visible, try increasing the "
                    "number of float quantization bits.";
        hkError::s_instance->message(
            hkError::MESSAGE_WARNING,
            static_cast<int32_t>(0xABBA5A61u),
            acBuffer,
            ".\\Animation\\SplineCompressed\\hkaSplineCompressedAnimation.cpp",
            0x59
        );
        pStream->~hkErrStream();
    }

    if (approximateQuantizationAccuracy(static_cast<RotationQuantization>(m_rotationQuantizationType.m_storage)) *
            0.16260161995887756 >
        m_rotationTolerance) {
        alignas(4) uint8_t kStreamStorage[sizeof(hkErrStream)];
        char acBuffer[0x200];
        hkErrStream* pStream = ::new (kStreamStorage) hkErrStream(acBuffer, 0x200);
        *pStream << "Quantization may be limiting rotation quality.  If artifacts are visible, try increasing the "
                    "number of rotation quantization bits.";
        hkError::s_instance->message(
            hkError::MESSAGE_WARNING,
            static_cast<int32_t>(0xABBA5A61u),
            acBuffer,
            ".\\Animation\\SplineCompressed\\hkaSplineCompressedAnimation.cpp",
            0x64
        );
        pStream->~hkErrStream();
    }

    return (bQuantizationValid && bTolerancesValid) ? 1 : 0;
}

// Whether the scalar quantization is BITS8 or BITS16.
// KEEP: unregistered. The body touches only two scratch registers, and a caller relies on a third surviving the call,
// which clang does not guarantee.
hkBool hkaSplineCompressedAnimation::TrackCompressionParams::validQuantization(
    hkaSplineCompressedAnimation::TrackCompressionParams::ScalarQuantization aeType
) {
    return aeType == BITS8 || aeType == BITS16;
}

// KEEP: unregistered. Whether the rotation quantization is one of the six types. The body touches only two scratch
// registers, and its caller isOk keeps a value loaded before this call in a third one across it, which clang does not
// guarantee to keep.
hkBool hkaSplineCompressedAnimation::TrackCompressionParams::validQuantization(
    hkaSplineCompressedAnimation::TrackCompressionParams::RotationQuantization aeType
) {
    return aeType == POLAR32 ||
        aeType == THREECOMP40 ||
        aeType == THREECOMP48 ||
        aeType == THREECOMP24 ||
        aeType == STRAIGHT16 ||
        aeType == UNCOMPRESSED;
}

// BITS8 returns 0.0078125 and BITS16 returns 3.0517578125e-05; any other value returns zero.
float hkaSplineCompressedAnimation::TrackCompressionParams::approximateQuantizationAccuracy(ScalarQuantization aeType) {
    if (!aeType) {
        return 0x1p-7f;
    }
    if (aeType == BITS16) {
        return 0x1p-15f;
    }
    return 0.0;
}

// Returns one of six constants by aeType, and 0.0f above index 5.
float hkaSplineCompressedAnimation::TrackCompressionParams::approximateQuantizationAccuracy(
    hkaSplineCompressedAnimation::TrackCompressionParams::RotationQuantization aeType
) {
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
            v1 = HK_REAL_EPSILON;
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

hkBool hkaSplineCompressedAnimation::PerTrackCompressionParams::isOk() const {
    int32_t v3 = 0;
    if (m_parameterPalette.m_size <= 0) {
        return 1;
    }

    int32_t v4 = 0;
    while (m_parameterPalette.m_data[v4].isOk().m_bool) {
        ++v3;
        ++v4;
        if (v3 >= m_parameterPalette.m_size) {
            return 1;
        }
    }
    return 0;
}

// A palette of trackParams alone, every transform and float track indexing it.
hkaSplineCompressedAnimation::hkaSplineCompressedAnimation(
    const hkaInterleavedUncompressedAnimation& raw,
    const TrackCompressionParams& trackParams,
    const AnimationCompressionParams& animationParams
)
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

hkaSplineCompressedAnimation::hkaSplineCompressedAnimation(
    const hkaInterleavedUncompressedAnimation& raw,
    const PerTrackCompressionParams& trackParams,
    const AnimationCompressionParams& animationParams
)
  : hkaAnimation(raw), m_blockOffsets(), m_floatBlockOffsets(), m_transformOffsets(), m_floatOffsets(), m_data() {
    initialize(raw, trackParams, animationParams);
}

// The five offset and data arrays go with their members.
hkaSplineCompressedAnimation::~hkaSplineCompressedAnimation() {}

void hkaSplineCompressedAnimation::sampleTracks(
    float time,
    hkQsTransform* transformTracksOut,
    float* floatTracksOut,
    hkaChunkCache* cache
) const {
    samplePartialTracks(
        time,
        m_numberOfTransformTracks,
        transformTracksOut,
        m_numberOfFloatTracks,
        floatTracksOut,
        cache
    );
}

// Each transform track has four bytes at the block's start (computePackedNurbsOffsets with offset 0, flag bit 31):
// its quantization types, then the translation, rotation and scale masks; the float tracks' mask bytes follow them.
void hkaSplineCompressedAnimation::samplePartialTracks(
    float time,
    uint32_t maxNumTransformTracks,
    hkQsTransform* transformTracksOut,
    uint32_t maxNumFloatTracks,
    float* floatTracksOut,
    hkaChunkCache* cache
) const {
    HK_TIMER_BEGIN("SampleSpline", nullptr);

    uint32_t frame;
    float delta;
    getFrameAndDelta(time, frame, delta);
    int32_t block;
    float blockTime;
    uint8_t quantizedTime;
    getBlockAndTime(frame, delta, block, blockTime, quantizedTime);

    const uint8_t* dataPtr = computePackedNurbsOffsets(
        m_data.m_data,
        m_blockOffsets.m_data,
        block,
        static_cast<uint32_t>(m_maskAndQuantizationSize)
    );
    const uint8_t* maskPtr = computePackedNurbsOffsets(m_data.m_data, m_blockOffsets.m_data, block, 0x80000000u);
    if (dataPtr && maskPtr) {
        for (uint32_t i = 0; i < maxNumTransformTracks; i++) {
            const uint32_t quantizationTypes = maskPtr[0];
            hkQsTransform& out = transformTracksOut[i];
            sampleTranslation(
                blockTime,
                quantizedTime,
                maskPtr[1],
                static_cast<TrackCompressionParams::ScalarQuantization>(quantizationTypes & 3),
                dataPtr,
                out.m_translation
            );
            sampleRotation(
                blockTime,
                quantizedTime,
                maskPtr[2],
                static_cast<TrackCompressionParams::RotationQuantization>((quantizationTypes >> 2) & 0xF),
                dataPtr,
                out.m_rotation
            );
            sampleScale(
                blockTime,
                quantizedTime,
                maskPtr[3],
                static_cast<TrackCompressionParams::ScalarQuantization>(quantizationTypes >> 6),
                dataPtr,
                out.m_scale
            );
            maskPtr += 4;
        }

        const uint8_t* floatDataPtr = m_data.m_data + m_floatBlockOffsets.m_data[block] + m_blockOffsets.m_data[block];
        const uint8_t* floatMaskPtr = m_data.m_data + m_blockOffsets.m_data[block] + 4 * m_numberOfTransformTracks;
        for (uint32_t i = 0; i < maxNumFloatTracks; i++) {
            const uint8_t floatMask = *floatMaskPtr++;
            sampleFloat(
                blockTime,
                quantizedTime,
                floatMask & 0xF9,
                static_cast<TrackCompressionParams::ScalarQuantization>((floatMask >> 1) & 3),
                floatDataPtr,
                floatTracksOut[i]
            );
        }
    }

    HK_TIMER_END();
}

// Samples the listed transform tracks at time; a track index out of range gives a zero transform.
void hkaSplineCompressedAnimation::sampleIndividualTransformTracks(
    float time,
    const int16_t* tracks,
    uint32_t numTracks,
    hkQsTransform* transformTracksOut
) const {
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
        const uint8_t* dataInOut = computePackedNurbsOffsets(
            m_data.m_data,
            m_blockOffsets.m_data,
            block,
            m_transformOffsets.m_data[block * m_numberOfTransformTracks + track]
        );
        const uint8_t* mask = computePackedNurbsOffsets(m_data.m_data, m_blockOffsets.m_data, block, 4 * track);
        const uint32_t maskByte = *mask;
        sampleTranslation(
            blockTime,
            quantizedTime,
            mask[1],
            static_cast<TrackCompressionParams::ScalarQuantization>(maskByte & 3),
            dataInOut,
            transformTracksOut[i].m_translation
        );
        sampleRotation(
            blockTime,
            quantizedTime,
            mask[2],
            static_cast<TrackCompressionParams::RotationQuantization>((maskByte >> 2) & 0xF),
            dataInOut,
            transformTracksOut[i].m_rotation
        );
        sampleScale(
            blockTime,
            quantizedTime,
            mask[3],
            static_cast<TrackCompressionParams::ScalarQuantization>(maskByte >> 6),
            dataInOut,
            transformTracksOut[i].m_scale
        );
    }
}

// Samples the listed float tracks at time; a track index out of range gives 0.
void hkaSplineCompressedAnimation::sampleIndividualFloatTracks(
    float time,
    const int16_t* tracks,
    uint32_t numTracks,
    float* floatTracksOut
) const {
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
        const uint8_t* dataInOut = computePackedNurbsOffsets(
            m_data.m_data,
            m_blockOffsets.m_data,
            block,
            m_floatOffsets.m_data[block * m_numberOfFloatTracks + track]
        );
        const uint8_t* mask = computePackedNurbsOffsets(
            m_data.m_data,
            m_blockOffsets.m_data,
            block,
            4 * m_numberOfTransformTracks + track
        );
        const uint8_t maskByte = *mask;
        sampleFloat(
            blockTime,
            quantizedTime,
            maskByte & 0xF9,
            static_cast<TrackCompressionParams::ScalarQuantization>((maskByte >> 1) & 3),
            dataInOut,
            floatTracksOut[i]
        );
    }
}

int32_t hkaSplineCompressedAnimation::getNumOriginalFrames() const {
    return m_numFrames;
}

int32_t hkaSplineCompressedAnimation::getNumDataChunks(uint32_t frame, float delta) const {
    return 3;
}

// The object (its size rounded up to 16 bytes), the block's data (its size rounded up to 16) and the block's float
// block offset.
void hkaSplineCompressedAnimation::getDataChunks(
    uint32_t frame,
    float delta,
    hkaAnimation::DataChunk* dataChunks,
    int32_t numDataChunks
) const {
    dataChunks[0].m_data = this;
    dataChunks[0].m_size = static_cast<uint32_t>(
        (sizeof(hkaSplineCompressedAnimation) + 15) & ~static_cast<size_t>(15)
    );
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
        }
        while (v3 >= 0);
    }
    return (v2 + 15 & 0xFFFFFFF0u) + 160;
}

void hkaSplineCompressedAnimation::sampleRotation(
    float time,
    uint8_t quantizedTime,
    uint8_t mask,
    TrackCompressionParams::RotationQuantization rotationQuantizationType,
    const uint8_t*& dataInOut,
    hkQuaternion& out
) const {
    readNURBSQuaternion(dataInOut, quantizedTime, m_frameDuration, time, mask, rotationQuantizationType, out);
    dataInOut = reinterpret_cast<const uint8_t*>(
        reinterpret_cast<uintptr_t>(dataInOut + 3) & ~static_cast<uintptr_t>(3)
    );
}

void hkaSplineCompressedAnimation::sampleTranslation(
    float u,
    uint8_t quantizedTime,
    uint8_t mask,
    hkaSplineCompressedAnimation::TrackCompressionParams::ScalarQuantization scalarQuantizationType,
    const uint8_t*& dataInOut,
    hkVector4& out
) const {
    if (mask) {
        hkVector4 I_;
        I_.m_quad = hkQsTransformIdentity.m_translation.m_quad;
        readNURBSCurve(dataInOut, quantizedTime, m_frameDuration, u, mask, scalarQuantizationType, I_, out);
    }
    else {
        out = hkQsTransformIdentity.m_translation;
    }
    dataInOut = reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>((dataInOut + 3)) & ~static_cast<uintptr_t>(3));
}

void hkaSplineCompressedAnimation::sampleScale(
    float u,
    uint8_t quantizedTime,
    uint8_t mask,
    hkaSplineCompressedAnimation::TrackCompressionParams::ScalarQuantization scalarQuantizationType,
    const uint8_t*& dataInOut,
    hkVector4& out
) const {
    if (mask) {
        hkVector4 I_;
        I_.m_quad = hkQsTransformIdentity.m_scale.m_quad;
        readNURBSCurve(dataInOut, quantizedTime, m_frameDuration, u, mask, scalarQuantizationType, I_, out);
    }
    else {
        out = hkQsTransformIdentity.m_scale;
    }
    dataInOut = reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>((dataInOut + 3)) & ~static_cast<uintptr_t>(3));
}

void hkaSplineCompressedAnimation::sampleFloat(
    float u,
    uint8_t quantizedTime,
    uint8_t mask,
    hkaSplineCompressedAnimation::TrackCompressionParams::ScalarQuantization scalarQuantizationType,
    const uint8_t*& dataInOut,
    float& out
) const {
    if (mask) {
        hkVector4 I_;
        hkVector4 out_;
        double m_frameDuration = this->m_frameDuration;
        I_.m_quad = _mm_setzero_ps();
        readNURBSCurve(dataInOut, quantizedTime, m_frameDuration, u, mask, scalarQuantizationType, I_, out_);
        out = XFNV_M128_F32(out_.m_quad, 0);
    }
    else {
        out = 0.0f;
    }
    dataInOut = reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>((dataInOut + 3)) & ~static_cast<uintptr_t>(3));
}

int32_t hkaSplineCompressedAnimation::getSizeInBytes() {
    return m_data.m_size + 4 * (m_blockOffsets.m_size + (m_transformOffsets.m_size + m_floatOffsets.m_size)) + 132;
}

// Builds the animation's block layout from the raw animation's frame count, then per block the static mask and
// quantization bytes (padZeros reserves them), and per transform track (translation, rotation, scale) or float track
// the samples, their mean and range, the static mask (isStatic against the clear value), and for a dynamic one a NURBS
// fit (again at degree 1 when it needs more control points than samples), written out with its knots; then the
// compression report. Each track's sample and fit arrays are hkArray locals. The tolerances are float copies.
void hkaSplineCompressedAnimation::initialize(
    const hkaInterleavedUncompressedAnimation& raw,
    const hkaSplineCompressedAnimation::PerTrackCompressionParams& trackParams,
    const hkaSplineCompressedAnimation::AnimationCompressionParams& animationParams
) {
    m_type = HK_SPLINE_COMPRESSED_ANIMATION;
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
    float blockDuration = static_cast<float>(
        static_cast<double>(framesPerBlockMinusOne) * (m_duration / static_cast<double>(numFrames - 1))
    );
    m_blockDuration = blockDuration;
    m_blockInverseDuration = 1.0 / blockDuration;
    m_frameDuration = m_duration / static_cast<double>(numFrames - 1);

    // The SDK's clear values per transform component: no translation, the identity rotation, unit scale.
    hkVector4 clearValues[3];
    clearValues[0].setZero4();
    clearValues[1].set(0.0f, 0.0f, 0.0f, 1.0f);
    clearValues[2].set(1.0f, 1.0f, 1.0f, 0.0f);

    using VectorArrays = hkArray<hkVector4>[3];
    using FloatArrays = hkArray<float>[3];
    int32_t firstFrame = 0;
    for (int32_t block = 0; block < m_numBlocks; ++block) {
        m_blockOffsets.pushBack(static_cast<uint32_t>(m_data.m_size));
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
                const uint32_t offset = static_cast<uint32_t>(m_data.m_size) - m_blockOffsets.m_data[block];
                m_floatBlockOffsets.pushBack(offset);
            }
            for (int32_t track = 0; track < numTracks; ++track) {
                if (enableSampleSingleTracks) {
                    const uint32_t offset = static_cast<uint32_t>(m_data.m_size) - m_blockOffsets.m_data[block];
                    hkArray<uint32_t>& offsets = trackType == 0 ? m_transformOffsets : m_floatOffsets;
                    offsets.pushBack(offset);
                }
                const int32_t* paletteIndices = trackType ? trackParams.m_floatTrackIndexToPaletteIndex.m_data
                                                          : trackParams.m_trackIndexToPaletteIndex.m_data;
                const hkaSplineCompressedAnimation::TrackCompressionParams*
                    params = &trackParams.m_parameterPalette.m_data[paletteIndices[track]];

                // The samples of each component and their times.
                alignas(16) VectorArrays samples;
                FloatArrays times;
                for (int32_t k = 0; k < numComponents; ++k) {
                    samples[k].setSize(numBlockFrames);
                    times[k].setSize(numBlockFrames);
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
                    extractTrackTransforms(
                        raw.m_transforms.m_data,
                        m_numberOfTransformTracks,
                        numBlockFrames,
                        firstFrame,
                        track,
                        samples[0],
                        samples[1],
                        samples[2]
                    );
                }
                else {
                    tolerances[0] = params->m_floatingTolerance;
                    degrees[0] = params->m_floatingDegree;
                    extractTrackFloats(
                        raw.m_floats.m_data,
                        m_numberOfFloatTracks,
                        numBlockFrames,
                        firstFrame,
                        track,
                        samples[0]
                    );
                }

                for (int32_t k = 0; k < numComponents; ++k) {
                    hkVector4 mean = calcMean(samples[k]);
                    hkVector4 minp = calcMin(samples[k]);
                    hkVector4 maxp = calcMax(samples[k]);
                    if (k == 1) {
                        // The rotation's mean is normalized (0 for a zero sum).
                        mean.normalize4();
                    }
                    uint8_t
                        mask = hkaSplineCompressedAnimation::isStatic(mean, minp, maxp, tolerances[k], clearValues[k]);
                    if (trackType == 1) {
                        // A float track always has its mask bit.
                        if (!mask) {
                            mask = 1;
                        }
                        m_data.m_data[dataOffset] = static_cast<uint8_t>(
                            packMaskAndQuantization(mask, params->m_floatQuantizationType.m_storage)
                        );
                        dataOffset += 1;
                    }
                    else if (k == 0) {
                        m_data.m_data[dataOffset] = static_cast<uint8_t>(packQuantizationTypes(
                            params->m_translationQuantizationType.m_storage,
                            params->m_rotationQuantizationType.m_storage,
                            params->m_scaleQuantizationType.m_storage
                        ));
                        m_data.m_data[dataOffset + 1] = mask;
                        dataOffset += 2;
                    }
                    else {
                        m_data.m_data[dataOffset] = mask;
                        dataOffset += 1;
                    }

                    // The fitted curve's knots and control points.
                    alignas(16) hkArray<hkVector4> points(3 * numBlockFrames);
                    alignas(16) hkArray<float> knots(3 * numBlockFrames + 2);
                    const uint8_t dynamicBits = mask & 0xF0;
                    if (dynamicBits) {
                        int32_t numKnots;
                        int32_t numPoints;
                        ApproximateNURBSCurve(
                            numBlockFrames,
                            times[k].m_data,
                            samples[k].m_data,
                            degrees[k],
                            tolerances[k],
                            &numKnots,
                            knots.m_data,
                            &numPoints,
                            points.m_data
                        );
                        for (int32_t pass = 0; pass < 2; ++pass) {
                            knots.setSize(numKnots);
                            points.setSize(numPoints);
                            // More control points than samples: the fit again at degree 1.
                            if (pass == 1 || numPoints <= numBlockFrames) {
                                break;
                            }
                            degrees[k] = 1;
                            ApproximateNURBSCurve(
                                numBlockFrames,
                                times[k].m_data,
                                samples[k].m_data,
                                1,
                                tolerances[k],
                                &numKnots,
                                knots.m_data,
                                &numPoints,
                                points.m_data
                            );
                        }
                        writeKnots(numPoints - 1, knots.m_size - numPoints - 1, knots);
                    }
                    if (k == 1) {
                        writeRotation(
                            mask,
                            static_cast<TrackCompressionParams::RotationQuantization>(
                                params->m_rotationQuantizationType.m_storage
                            ),
                            mean,
                            points.m_size - 1,
                            points
                        );
                    }
                    else {
                        if (dynamicBits) {
                            minp = calcMin(points);
                            maxp = calcMax(points);
                        }
                        uint8_t type;
                        if (trackType) {
                            type = params->m_floatQuantizationType.m_storage;
                        }
                        else if (k) {
                            type = params->m_scaleQuantizationType.m_storage;
                        }
                        else {
                            type = params->m_translationQuantizationType.m_storage;
                        }
                        writePoints(
                            mask,
                            static_cast<TrackCompressionParams::ScalarQuantization>(type),
                            mean,
                            minp,
                            maxp,
                            points.m_size - 1,
                            points
                        );
                    }
                    writeAlign(4, &m_data);
                }
            }
        }
        firstFrame = firstFrame + numBlockFrames - 1;
        writeAlign(16, &m_data);
    }

    // The report, each line through an hkErrStream on buf (the SDK's HK_REPORT).
    alignas(16) char buf[512];
    hkError::s_instance->sectionBegin(0x36118E94, "Compression");
    {
        hkErrStream os(buf, 512);
        os << "Number of blocks: " << m_numBlocks;
        hkError::s_instance->message(
            hkError::MESSAGE_REPORT,
            -1,
            buf,
            ".\\Animation\\SplineCompressed\\hkaSplineCompressedAnimationCtor.cpp",
            366
        );
    }
    {
        hkErrStream os(buf, 512);
        const int32_t maxChunkSize = static_cast<const hkaAnimation*>(this)->getMaxSizeOfCombinedDataChunks();
        os << "Size of largest block (PS3 SPU): " << maxChunkSize << " bytes.";
        hkError::s_instance->message(
            hkError::MESSAGE_REPORT,
            -1,
            buf,
            ".\\Animation\\SplineCompressed\\hkaSplineCompressedAnimationCtor.cpp",
            367
        );
    }
    // The raw size counts 16 floats per transform.
    const uint32_t originalSize = static_cast<uint32_t>(
        (raw.m_transforms.m_size * 16 + raw.m_floats.m_size) * static_cast<int32_t>(sizeof(float))
    );
    {
        hkErrStream os(buf, 512);
        const int32_t compressedSize = hkaSplineCompressedAnimation::getSizeInBytes();
        os << "Original Size: " << originalSize << " bytes. Compressed Size: " << compressedSize << " bytes.";
        hkError::s_instance->message(
            hkError::MESSAGE_REPORT,
            -1,
            buf,
            ".\\Animation\\SplineCompressed\\hkaSplineCompressedAnimationCtor.cpp",
            372
        );
    }
    {
        hkErrStream os(buf, 512);
        float fOriginalSize = static_cast<float>(originalSize);
        const float ratio = fOriginalSize / static_cast<double>(hkaSplineCompressedAnimation::getSizeInBytes());
        os << "Compression Ratio: " << ratio << ":1.";
        hkError::s_instance->message(
            hkError::MESSAGE_REPORT,
            -1,
            buf,
            ".\\Animation\\SplineCompressed\\hkaSplineCompressedAnimationCtor.cpp",
            373
        );
    }
    hkError::s_instance->sectionEnd();
}

// KEEP: unregistered for uninitialised reads. A mask with bit 3 set copies stat's w (never read from the data) into
// the control points and out; a mask with bit 7 set leaves every control point's w unwritten, and the evaluator reads
// it; and the min and max of all three components are loaded whichever the mask makes dynamic. The mask's low nibble
// marks static components (one float each), the high nibble dynamic ones (a min and a max float each, then 8- or
// 16-bit control points scaled into that range by the double 255.0 or 65535.0); the rest take I's component. The data
// is aligned to 4 for the floats and to 2 for the control points, skipped to the span's first control point, and
// finally stepped over all n + 1 of them.
void hkaSplineCompressedAnimation::readNURBSCurve(
    const uint8_t*& dataInOut,
    uint8_t quantizedTime,
    float frameDuration,
    float u,
    uint8_t mask,
    TrackCompressionParams::ScalarQuantization scalarQuantizationType,
    hkVector4& I,
    hkVector4& out
) {
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
    }
    else {
        span = 0;
        p = 0;
        n = 0;
    }
    readAlign(4, dataInOut);
    float* pStat = &reinterpret_cast<float*>(&stat.m_quad)[0];
    float* pMin = &reinterpret_cast<float*>(&minp.m_quad)[0];
    float* pMax = &reinterpret_cast<float*>(&maxp.m_quad)[0];
    for (int32_t c = 0; c < 3; ++c) {
        if (uiMask & (1 << c)) {
            pStat[c] = readReal(dataInOut);
        }
        else if (uiMask & (0x10 << c)) {
            pMin[c] = readReal(dataInOut);
            pMax[c] = readReal(dataInOut);
        }
    }
    if (!uiDynamicMask) {
        recompose(mask, stat, I, out);
        return;
    }

    const int32_t bytesPerComponent = s_bytesPerComponent[scalarQuantizationType];
    readAlign(2, dataInOut);
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
                }
                else {
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
                }
                else if (identityMask & (1u << c)) {
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
int32_t hkaSplineCompressedAnimation::readKnots(
    const uint8_t*& dataInOut,
    int32_t* _n,
    int32_t* _p,
    uint8_t quantizedTime,
    float frameDuration,
    float* U
) {
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
void hkaSplineCompressedAnimation::readNURBSQuaternion(
    const uint8_t*& dataInOut,
    uint8_t quantizedTime,
    float frameDuration,
    float u,
    uint8_t mask,
    TrackCompressionParams::RotationQuantization type,
    hkQuaternion& out
) {
    if (mask & 0xF0) {
        int32_t n;
        alignas(16) int32_t p;
        float U[6];
        hkVector4 P[4];
        int32_t span = readKnots(dataInOut, &n, &p, quantizedTime, frameDuration, U);
        readPackedQuaternions(dataInOut, type, n, p, span, P);
        // The evaluator writes the quaternion's four components as a vector.
        s_evaluateFunctions[p](u, p, U, P, out.m_vec);
    }
    else if (mask & 0x0F) {
        readAlignQuaternion(type, dataInOut);
        unpackQuaternion(type, dataInOut, &out);
        dataInOut += quaternionSize[type];
    }
    else {
        out.setIdentity();
    }
}

const hkClass& hkaSplineCompressedAnimation::staticClass() {
    return hkaSplineCompressedAnimationClass;
}

namespace {
    // Whether component i of mean is within tol of both ends of the range.
    bool InRange(const hkVector4& mean, const hkVector4& minp, const hkVector4& maxp, int32_t i, double tol) {
        float fToMin = std::fabs(static_cast<double>(XFNV_M128_F32(mean.m_quad, i) - XFNV_M128_F32(minp.m_quad, i)));
        if (!(fToMin <= tol)) {
            return false;
        }
        float fToMax = std::fabs(static_cast<double>(XFNV_M128_F32(mean.m_quad, i) - XFNV_M128_F32(maxp.m_quad, i)));
        return fToMax <= tol;
    }

    // The distance of component i of mean from the clear value.
    float FromClear(const hkVector4& mean, const hkVector4& I, int32_t i) {
        return std::fabs(static_cast<double>(XFNV_M128_F32(mean.m_quad, i) - XFNV_M128_F32(I.m_quad, i)));
    }
} // namespace

uint8_t hkaSplineCompressedAnimation::isStatic(
    const hkVector4& mean,
    const hkVector4& minp,
    const hkVector4& maxp,
    float tol,
    const hkVector4& I
) {
    double fTol = tol;
    // Bits 4-7: the component varies; bits 0-3: it is constant but not at the clear value. The first component's
    // clear test is false for a NaN distance, the others' true.
    uint8_t mask;
    if (!InRange(mean, minp, maxp, 0, fTol)) {
        mask = 16;
    }
    else {
        mask = FromClear(mean, I, 0) > fTol;
    }
    for (int32_t i = 1; i < 4; ++i) {
        if (!InRange(mean, minp, maxp, i, fTol)) {
            mask |= 16 << i;
        }
        else if (!(FromClear(mean, I, i) <= fTol)) {
            mask |= 1 << i;
        }
    }
    return mask;
}

void hkaSplineCompressedAnimation::reverseEndian(int32_t bytes, uint8_t*& dataInOut) {
    switch (bytes) {
        case 1: {
            ++dataInOut;
            break;
        }
        case 2: {
            uint8_t* v2 = dataInOut;
            uint8_t v3 = v2[0];
            v2[0] = v2[1];
            v2[1] = v3;
            dataInOut += 2;
            break;
        }
        case 4: {
            uint8_t* v4 = dataInOut;
            uint8_t v5 = v4[0];
            v4[0] = v4[3];
            v4[3] = v5;
            uint8_t* v6 = dataInOut;
            uint8_t v7 = v6[1];
            v6[1] = v6[2];
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
void hkaSplineCompressedAnimation::handleEndianScalarControlPoints(
    int32_t mask,
    int32_t n,
    hkaSplineCompressedAnimation::TrackCompressionParams::ScalarQuantization type,
    uint8_t*& dataInOut
) {
    // Tests the mask's low byte only.
    if (static_cast<uint8_t>(mask)) {
        // The stream is untyped bytes: the pointer itself is rounded up to a multiple of 4.
        dataInOut = reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(dataInOut + 3) & ~static_cast<uintptr_t>(3));
        int32_t numDynamic = 0;
        for (int32_t i = 0; i < 3; i++) {
            if (mask & (0x10 << i)) {
                reverseEndian(4, dataInOut);
                reverseEndian(4, dataInOut);
                ++numDynamic;
            }
            else if (mask & (1 << i)) {
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
    double v6 = b;
    float* m_data = ub.begin();
    m_data[0] = a;
    m_data[m] = b;
    int32_t v7 = 1;
    int32_t index = 1;
    if (m - 1 >= 4) {
        int32_t v8 = 3;
        float realM = static_cast<float>(m);
        int32_t index2 = 3;
        float* v9 = m_data + 3;
        double v10 = v6 - v4;
        double v11 = realM;
        do {
            double v12 = static_cast<double>(index);
            int32_t a2a = v8 - 1;
            int32_t v13 = v8 + 1;
            v7 += 4;
            v8 += 4;
            v9 += 4;
            *(v9 - 6) = v12 / v11 * v10 + v4;
            *(v9 - 5) = static_cast<double>(a2a) / v11 * v10 + v4;
            double v14 = static_cast<double>(index2);
            index2 = v8;
            *(v9 - 4) = v14 / v11 * v10 + v4;
            index = v7;
            *(v9 - 3) = static_cast<double>(v13) / v11 * v10 + v4;
        }
        while (v7 < m - 3);
    }
    if (v7 < m) {
        float* v15 = &m_data[v7];
        double v16 = v6 - v4;
        do {
            ++v7;
            ++v15;
            float a3a = static_cast<float>(m);
            double v17 = static_cast<double>(index) / a3a;
            index = v7;
            *(v15 - 1) = v17 * v16 + v4;
        }
        while (v7 < m);
    }
}

void hkaSplineCompressedAnimation::handleEndianScalar(
    int32_t mask,
    hkaSplineCompressedAnimation::TrackCompressionParams::ScalarQuantization type,
    uint8_t*& dataInOut
) {
    // Tests the mask's low byte and passes the whole mask on.
    if (static_cast<uint8_t>(mask)) {
        int32_t v3 = hkaSplineCompressedAnimation::handleEndianKnots(mask, dataInOut);
        hkaSplineCompressedAnimation::handleEndianScalarControlPoints(mask, v3, type, dataInOut);
        dataInOut = reinterpret_cast<uint8_t*>(
            reinterpret_cast<uintptr_t>((dataInOut + 3)) & ~static_cast<uintptr_t>(3)
        );
    }
}

void hkaSplineCompressedAnimation::handleEndianRotation(
    int32_t mask,
    hkaSplineCompressedAnimation::TrackCompressionParams::RotationQuantization type,
    uint8_t*& dataInOut
) {
    // Tests the mask's low byte and passes the whole mask on.
    if (static_cast<uint8_t>(mask)) {
        int32_t v3 = hkaSplineCompressedAnimation::handleEndianKnots(mask, dataInOut);
        handleEndianRotationControlPoints(mask, v3, type, dataInOut);
        dataInOut = reinterpret_cast<uint8_t*>(
            reinterpret_cast<uintptr_t>((dataInOut + 3)) & ~static_cast<uintptr_t>(3)
        );
    }
}

void hkaSplineCompressedAnimation::write16(uint16_t value, hkArray<uint8_t, hkContainerHeapAllocator>& data) {
    padZeros(2, &data);
    // The array is an untyped byte stream: the value is stored into its last two (unaligned) bytes.
    *reinterpret_cast<uint16_t*>(&data.m_data[data.m_size - 2]) = value;
}

void hkaSplineCompressedAnimation::writeReal(float value, hkArray<uint8_t, hkContainerHeapAllocator>& data) {
    padZeros(4, &data);
    // The array is an untyped byte stream: the float is stored into its last four (unaligned) bytes.
    *reinterpret_cast<float*>(&data.m_data[data.m_size - 4]) = value;
}

// Aligns dataInOut to the type's packed quaternion alignment (quaternionAlign) and unpacks the p + 1 control points
// of span from the n + 1 packed ones of quaternionSize bytes each, then steps over all of them.
void hkaSplineCompressedAnimation::readPackedQuaternions(
    const uint8_t*& dataInOut,
    hkaSplineCompressedAnimation::TrackCompressionParams::RotationQuantization type,
    int32_t n,
    int32_t p,
    int32_t span,
    hkVector4* P
) {
    readAlignQuaternion(type, dataInOut);
    const int32_t size = quaternionSize[type];
    for (int32_t i = 0; i <= p; i++) {
        // The SDK's source unpacks each quaternion into the control point's storage: the unpacker writes the
        // hkVector4 as raw memory.
        unpackQuaternion(type, dataInOut + (span - p + i) * size, reinterpret_cast<hkQuaternion*>(&P[i]));
    }
    dataInOut += (n + 1) * size;
}

void hkaSplineCompressedAnimation::evaluateSimple2(
    float time,
    int32_t p,
    float* const U,
    hkVector4* const P,
    hkVector4& out
) {
    uint32_t v15;
    uint32_t v17;
    __m128 v5 = _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)));
    *reinterpret_cast<float*>(&v15) = time - U[1];
    __m128 v6 = _mm_set_ss(std::bit_cast<float>(v15));
    float v16 = U[2] - time;
    float v8 = 1.0f / (v16 + XFNV_M128_F32(v6, 0));
    *reinterpret_cast<float*>(&v17) = time - U[0];
    __m128 v10 = _mm_set_ss(std::bit_cast<float>(v17));
    float v11 = (v16 * v8) + 0.0f;
    float v12 = v16 + *reinterpret_cast<float*>(&v17);
    float v18 = U[3] - time;
    float v13 = v11 / v12;
    XFNV_M128_F32(v5, 0) = (v16 * v13) + 0.0f;
    float v14 = (XFNV_M128_F32(v6, 0) * v8) / (v18 + XFNV_M128_F32(v6, 0));
    XFNV_M128_F32(v6, 0) = XFNV_M128_F32(v6, 0) * v14;
    XFNV_M128_F32(v10, 0) = (XFNV_M128_F32(v10, 0) * v13) + (v18 * v14);
    out.m_quad = _mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(v10, v10, 0), P[1].m_quad),
            _mm_mul_ps(_mm_shuffle_ps(v5, v5, 0), P[0].m_quad)
        ),
        _mm_mul_ps(_mm_shuffle_ps(v6, v6, 0), P[2].m_quad)
    );
}

void hkaSplineCompressedAnimation::evaluateSimple3(
    float time,
    int32_t p,
    float* const U,
    hkVector4* const P,
    hkVector4& out
) {
    uint32_t v20;
    uint32_t v22;
    uint32_t v24;
    __m128 v5 = _mm_set_ss(std::bit_cast<float>(std::bit_cast<uint32_t>(zero)));
    *reinterpret_cast<float*>(&v20) = time - U[2];
    __m128 v6 = _mm_set_ss(std::bit_cast<float>(v20));
    float v21 = U[3] - time;
    float v8 = 1.0f / (v21 + XFNV_M128_F32(v6, 0));
    float v9 = v21 * v8;
    *reinterpret_cast<float*>(&v22) = time - U[1];
    __m128 v10 = _mm_set_ss(std::bit_cast<float>(v22));
    float v11 = v9 + 0.0f;
    float v12 = v21 + *reinterpret_cast<float*>(&v22);
    float v23 = U[4] - time;
    float v26 = v11 / v12;
    float v14 = v23 + XFNV_M128_F32(v6, 0);
    *reinterpret_cast<float*>(&v24) = time - U[0];
    __m128 v27 = _mm_set_ss(std::bit_cast<float>(v24));
    float v15 = (XFNV_M128_F32(v6, 0) * v8) / v14;
    float v25 = U[5] - time;
    float v16 = ((v21 * v26) + 0.0f) / (v21 + XFNV_M128_F32(v27, 0));
    XFNV_M128_F32(v5, 0) = (v21 * v16) + 0.0f;
    float v17 = ((XFNV_M128_F32(v10, 0) * v26) + (v23 * v15)) / (v23 + XFNV_M128_F32(v10, 0));
    XFNV_M128_F32(v27, 0) = (XFNV_M128_F32(v27, 0) * v16) + (v23 * v17);
    float v19 = (XFNV_M128_F32(v6, 0) * v15) / (v25 + XFNV_M128_F32(v6, 0));
    XFNV_M128_F32(v6, 0) = XFNV_M128_F32(v6, 0) * v19;
    XFNV_M128_F32(v10, 0) = (XFNV_M128_F32(v10, 0) * v17) + (v25 * v19);
    out.m_quad = _mm_add_ps(
        _mm_add_ps(
            _mm_add_ps(
                _mm_mul_ps(_mm_shuffle_ps(v27, v27, 0), P[1].m_quad),
                _mm_mul_ps(_mm_shuffle_ps(v5, v5, 0), P[0].m_quad)
            ),
            _mm_mul_ps(_mm_shuffle_ps(v10, v10, 0), P[2].m_quad)
        ),
        _mm_mul_ps(_mm_shuffle_ps(v6, v6, 0), P[3].m_quad)
    );
}

void hkaSplineCompressedAnimation::evaluateSimple1(float u, int32_t p, float* U, hkVector4* P, hkVector4& out) {
    uint32_t v8;
    *reinterpret_cast<float*>(&v8) = u - U[0];
    __m128 v5 = _mm_set_ss(std::bit_cast<float>(v8));
    float v9 = U[1] - u;
    XFNV_M128_F32(v5, 0) = XFNV_M128_F32(v5, 0) / (v9 + XFNV_M128_F32(v5, 0));
    __m128 v6 = _mm_shuffle_ps(v5, v5, 0);
    out.m_quad = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, v6), P[0].m_quad), _mm_mul_ps(P[1].m_quad, v6));
}

int32_t hkaSplineCompressedAnimation::findSpan(int32_t n, int32_t p, uint8_t u, const uint8_t* U) {
    int32_t result = n;
    if (u < U[n + 1]) {
        if (u > *U) {
            int32_t v5 = n + 1;
            int32_t v6 = p;
            int32_t v7 = n + 1 + p;
            while (true) {
                result = v7 / 2;
                uint8_t v8 = U[result];
                if (u < v8) {
                    v5 = result;
                    v7 = result + v6;
                }
                else {
                    if (u < U[result + 1]) {
                        return result;
                    }
                    v6 = result;
                    v7 = v5 + result;
                }
            }
        }
        return p;
    }
    return result;
}

// The SDK's samplePartialWithDataChunks: chunk 0 is the animation, chunk 1 its data from the block's start (the
// per-track masks first, then the transform values after m_maskAndQuantizationSize bytes, the float masks at four
// bytes per transform track), chunk 2 the block's float offset. Each transform track's mask byte packs the
// translation (bits 0-1), rotation (bits 2-5) and scale (bits 6-7) quantizations; each float track's mask byte its
// quantization in bits 1-2.
void hkaSplineCompressedAnimation::samplePartialWithDataChunks(
    uint32_t frameIndex,
    float frameDelta,
    uint32_t maxNumTransformTracks,
    hkQsTransform* transformTracksOut,
    uint32_t maxNumFloatTracks,
    float* floatTracksOut,
    const hkaAnimation::DataChunk* dataChunks,
    int32_t numDataChunks
) {
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
        anim->sampleTranslation(
            blockTime,
            quantizedTime,
            mask[1],
            static_cast<TrackCompressionParams::ScalarQuantization>(quantizationTypes & 3),
            dataInOut,
            out.m_translation
        );
        anim->sampleRotation(
            blockTime,
            quantizedTime,
            mask[2],
            static_cast<TrackCompressionParams::RotationQuantization>((quantizationTypes >> 2) & 0xF),
            dataInOut,
            out.m_rotation
        );
        anim->sampleScale(
            blockTime,
            quantizedTime,
            mask[3],
            static_cast<TrackCompressionParams::ScalarQuantization>(quantizationTypes >> 6),
            dataInOut,
            out.m_scale
        );
        mask += 4;
    }
    const uint8_t* blockData = static_cast<const uint8_t*>(dataChunks[1].m_data) + dataChunks[1].m_offset;
    // Chunk 2 is the block's float offset, a 32-bit value read from the chunk's bytes.
    const uint32_t floatOffset = *reinterpret_cast<const uint32_t*>(
        static_cast<const uint8_t*>(dataChunks[2].m_data) + dataChunks[2].m_offset
    );
    const uint8_t* floatDataInOut = blockData + floatOffset;
    const uint8_t* floatMask = blockData + anim->m_numberOfTransformTracks * 4;
    for (uint32_t i = 0; i < maxNumFloatTracks; ++i) {
        const uint8_t floatMaskByte = *floatMask++;
        anim->sampleFloat(
            blockTime,
            quantizedTime,
            floatMaskByte & 0xF9,
            static_cast<TrackCompressionParams::ScalarQuantization>((floatMaskByte >> 1) & 3),
            floatDataInOut,
            floatTracksOut[i]
        );
    }
}
