#pragma once

#include "Havok/Source/Animation/Animation/Animation/hkaAnimation.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

#include <stdint.h>

class hkaInterleavedUncompressedAnimation;
class hkQsTransform;
class hkQuaternion;

// Havok SDK signature (hkaAnimation.lib, tied by reloc).
const void* getVtablehkaSplineCompressedAnimation();
void padZeros(int32_t a1, hkArrayBase<uint8_t>* apArray);
void writeAlign(int32_t a1, hkArrayBase<uint8_t>* apArray);
int32_t writeAlignQuaternion(int32_t a1, hkArrayBase<uint8_t>* apArray);
// Evaluates the B-spline of degree p at u from the knots U and the control points P into out (the general case of the
// evaluateSimple functions).
void evaluateSpline(float u, int32_t p, const float* U, const hkVector4* P, hkVector4& out);

// An animation stored as spline-compressed blocks. Only the virtuals this class overrides are declared;
// clearAllCacheKeys stays hkaAnimation's.
class hkaSplineCompressedAnimation : public hkaAnimation {
public:
    // How each kind of track was compressed. Layout from the Havok SDK.
    struct TrackCompressionParams {
        enum RotationQuantization : int32_t {
            POLAR32 = 0,
            THREECOMP40 = 1,
            THREECOMP48 = 2,
            THREECOMP24 = 3,
            STRAIGHT16 = 4,
            UNCOMPRESSED = 5,
        };

        enum ScalarQuantization : int32_t {
            BITS8 = 0,
            BITS16 = 1,
        };

        TrackCompressionParams();

        // Whether every quantization type is valid and no tolerance is negative, reporting each quantization that
        // is coarser than its tolerance.
        hkBool isOk() const;

        // Whether the scalar quantization is one of BITS8 and BITS16.
        static hkBool validQuantization(ScalarQuantization aeType);
        // Whether the rotation quantization is one of the six values; the hkBool result is written through arResult.
        static hkBool validQuantization(RotationQuantization aeType);
        // The accuracy BITS8 or BITS16 loses: 0.0078125f, 3.0517578125e-05f, or 0.0f for anything else.
        static float approximateQuantizationAccuracy(ScalarQuantization aeType);
        // The accuracy the rotation quantization loses.
        static float approximateQuantizationAccuracy(RotationQuantization aeType);
        // Havok SDK signature (hkaAnimation.lib, tied by target).
        static const hkClass& staticClass();

        float m_rotationTolerance;
        float m_translationTolerance;
        float m_scaleTolerance;
        float m_floatingTolerance;
        uint16_t m_rotationDegree;
        uint16_t m_translationDegree;
        uint16_t m_scaleDegree;
        uint16_t m_floatingDegree;
        hkEnum<RotationQuantization, uint8_t> m_rotationQuantizationType;
        hkEnum<ScalarQuantization, uint8_t> m_translationQuantizationType;
        hkEnum<ScalarQuantization, uint8_t> m_scaleQuantizationType;
        hkEnum<ScalarQuantization, uint8_t> m_floatQuantizationType;
    };

    // How the animation as a whole is split into blocks.
    struct AnimationCompressionParams {
        AnimationCompressionParams();

        // Havok SDK signature (hkaAnimation.lib, tied by target).
        static const hkClass& staticClass();

        uint16_t m_maxFramesPerBlock;
        hkBool m_enableSampleSingleTracks;
        uint8_t padding03[1];
    };

    // Per-track compression: a palette of parameters and each transform and float track's index into it.
    struct PerTrackCompressionParams {
        ~PerTrackCompressionParams();

        // Whether every palette entry isOk, with the result written through arResult.
        hkBool isOk() const;

        hkArray<TrackCompressionParams> m_parameterPalette;
        hkArray<int32_t> m_trackIndexToPaletteIndex;
        hkArray<int32_t> m_floatTrackIndexToPaletteIndex;
    };

    hkaSplineCompressedAnimation(
        const hkaInterleavedUncompressedAnimation& raw,
        const TrackCompressionParams& trackParams,
        const AnimationCompressionParams& animationParams
    );

    hkaSplineCompressedAnimation(hkFinishLoadedObjectFlag aFlag)
      : hkaAnimation(aFlag), m_blockOffsets(aFlag), m_floatBlockOffsets(aFlag), m_transformOffsets(aFlag),
        m_floatOffsets(aFlag), m_data(aFlag) {
        if (aFlag.m_finishing) {
            handleEndian();
        }
    }

    hkaSplineCompressedAnimation(
        const hkaInterleavedUncompressedAnimation& raw,
        const PerTrackCompressionParams& trackParams,
        const AnimationCompressionParams& animationParams
    );
    ~hkaSplineCompressedAnimation() override;

    void sampleTracks(
        float time,
        hkQsTransform* transformTracksOut,
        float* floatTracksOut,
        hkaChunkCache* cache
    ) const override;
    void samplePartialTracks(
        float time,
        uint32_t maxNumTransformTracks,
        hkQsTransform* transformTracksOut,
        uint32_t maxNumFloatTracks,
        float* floatTracksOut,
        hkaChunkCache* cache
    ) const override;
    void sampleIndividualTransformTracks(
        float time,
        const int16_t* tracks,
        uint32_t numTracks,
        hkQsTransform* transformTracksOut
    ) const override;
    void sampleIndividualFloatTracks(
        float time,
        const int16_t* tracks,
        uint32_t numTracks,
        float* floatTracksOut
    ) const override;
    int32_t getNumOriginalFrames() const override;
    int32_t getNumDataChunks(uint32_t frame, float delta) const override;
    void getDataChunks(
        uint32_t frame,
        float delta,
        hkaAnimation::DataChunk* dataChunks,
        int32_t numDataChunks
    ) const override;
    int32_t getMaxSizeOfCombinedDataChunks() const override;

    // Swaps the byte order of the block data in m_data when m_endian (+0x80) says the data is foreign.
    void handleEndian();
    // Appends the rotation curve's control points, normalized and packed by type, to m_data
    // (+0x74); only the mean when the mask has no dynamic bit.
    void writeRotation(
        uint8_t mask,
        TrackCompressionParams::RotationQuantization type,
        const hkVector4& mean,
        int32_t n,
        const hkArray<hkVector4>& P
    );
    // Appends the static values or ranges of the masked components, then the n + 1 quantized
    // control points, to m_data (+0x74).
    void writePoints(
        uint8_t mask,
        TrackCompressionParams::ScalarQuantization type,
        const hkVector4& mean,
        const hkVector4& minp,
        const hkVector4& maxp,
        int32_t n,
        hkArray<hkVector4>& P
    );
    // Appends n and p, then the n + p + 2 knots quantized against m_duration and
    // m_numFrames, to m_data (+0x74).
    void writeKnots(int32_t n, int32_t p, const hkArray<float>& U);
    void sampleRotation(
        float time,
        uint8_t quantizedTime,
        uint8_t mask,
        TrackCompressionParams::RotationQuantization rotationQuantizationType,
        const uint8_t*& dataInOut,
        hkQuaternion& out
    ) const;
    void sampleTranslation(
        float u,
        uint8_t quantizedTime,
        uint8_t mask,
        TrackCompressionParams::ScalarQuantization scalarQuantizationType,
        const uint8_t*& dataInOut,
        hkVector4& out
    ) const;
    // As sampleTranslation, with the identity's scale.
    void sampleScale(
        float u,
        uint8_t quantizedTime,
        uint8_t mask,
        TrackCompressionParams::ScalarQuantization scalarQuantizationType,
        const uint8_t*& dataInOut,
        hkVector4& out
    ) const;
    // As sampleTranslation, for one float track.
    void sampleFloat(
        float u,
        uint8_t quantizedTime,
        uint8_t mask,
        TrackCompressionParams::ScalarQuantization scalarQuantizationType,
        const uint8_t*& dataInOut,
        float& out
    ) const;

    // The block holding frame plus delta, the time into it and that time quantized to the block's 8-bit range.
    void getBlockAndTime(
        uint32_t frame,
        float delta,
        int32_t& blockOut,
        float& blockTimeOut,
        uint8_t& quantizedTimeOut
    ) const {
        blockOut = static_cast<int32_t>(frame / static_cast<uint32_t>(m_maxFramesPerBlock - 1));
        // EXE-INLINED none: hkMath::max2<int>
        blockOut = blockOut > 0 ? blockOut : 0;
        // EXE-INLINED none: hkMath::min2<int>
        blockOut = blockOut < m_numBlocks - 1 ? blockOut : m_numBlocks - 1;
        const uint32_t uiFirstFrameOfBlock = static_cast<uint32_t>(blockOut * (m_maxFramesPerBlock - 1));
        const float fRealFrame = static_cast<float>(frame - uiFirstFrameOfBlock) + delta;
        blockTimeOut = fRealFrame * m_frameDuration;
        // Truncates to an int and keeps its low byte.
        quantizedTimeOut = static_cast<uint8_t>(
            static_cast<int32_t>((blockTimeOut * m_blockInverseDuration) * static_cast<float>(m_maxFramesPerBlock - 1))
        );
    }

    int32_t getSizeInBytes();
    // Havok SDK signature (hkaAnimation.lib, tied by reloc,string).
    void initialize(
        const hkaInterleavedUncompressedAnimation& raw,
        const hkaSplineCompressedAnimation::PerTrackCompressionParams& trackParams,
        const hkaSplineCompressedAnimation::AnimationCompressionParams& animationParams
    );

    // Reads one vector curve (knots, then the per-component static values or ranges, then the control points of the
    // span) and evaluates it at u into out; components neither static nor dynamic come from I.
    static void readNURBSCurve(
        const uint8_t*& dataInOut,
        uint8_t quantizedTime,
        float frameDuration,
        float u,
        uint8_t mask,
        TrackCompressionParams::ScalarQuantization scalarQuantizationType,
        hkVector4& I,
        hkVector4& out
    );
    // Reads the knot vector around quantizedTime into U, its count into n and the degree into p; returns the span.
    static int32_t readKnots(
        const uint8_t*& dataInOut,
        int32_t* _n,
        int32_t* _p,
        uint8_t quantizedTime,
        float frameDuration,
        float* U
    );
    // Reads one rotation curve (knots and packed quaternion control points) and evaluates it
    // at u into out; a mask without dynamic bits reads the static quaternion, or the identity.
    static void readNURBSQuaternion(
        const uint8_t*& dataInOut,
        uint8_t quantizedTime,
        float frameDuration,
        float u,
        uint8_t mask,
        TrackCompressionParams::RotationQuantization type,
        hkQuaternion& out
    );

    // Writes the static components of stat and the identity components of I into out: bits 0 to 3 of mask select the
    // static ones, the components neither static (low bits) nor dynamic (high bits) are the identity's.
    static void recompose(uint8_t mask, const hkVector4& S, const hkVector4& I, hkVector4& inOut) {
        int32_t iStat = mask & 0x0F;
        int32_t iIden = ~mask & (~mask >> 4) & 0x0F;
        int32_t iShift = 0x01;
        for (int32_t i = 0; i < 4; i++) {
            if (iStat & iShift) {
                inOut(i) = S(i);
            }
            else if (iIden & iShift) {
                inOut(i) = I(i);
            }
            iShift <<= 1;
        }
    }

    // Havok SDK signature (hkaAnimation.lib, tied by target).
    static const hkClass& staticClass();
    // Havok SDK signature (hkaAnimation.lib, tied by bytes).
    static uint8_t
    isStatic(const hkVector4& mean, const hkVector4& minp, const hkVector4& maxp, float tol, const hkVector4& a5);
    // Havok SDK signature (hkaAnimation.lib, tied by bytes,reloc).
    static void reverseEndian(int32_t bytes, uint8_t*& dataInOut);
    // mask is used only as a byte, but it is declared int32_t because callers pass their whole mask value.
    static int32_t handleEndianKnots(int32_t mask, uint8_t*& dataInOut);
    // mask is used as a byte, but the caller passes its whole mask value; a uint8_t parameter would be zero-extended.
    static void handleEndianScalarControlPoints(
        int32_t mask,
        int32_t n,
        hkaSplineCompressedAnimation::TrackCompressionParams::ScalarQuantization type,
        uint8_t*& dataInOut
    );
    // Havok SDK signature (hkaAnimation.lib, tied by bytes).
    static void Linspace(int32_t m, hkArray<float, hkContainerHeapAllocator>& ub, float a, float b);
    // mask is int32_t rather than an unsigned char because the caller's whole value is passed on; uint8_t would be
    // zero-extended.
    static void handleEndianScalar(
        int32_t mask,
        hkaSplineCompressedAnimation::TrackCompressionParams::ScalarQuantization type,
        uint8_t*& dataInOut
    );
    // mask is int32_t rather than uint8_t because the caller's whole mask is passed on to handleEndianKnots and
    // handleEndianRotationControlPoints; a uint8_t would be zero-extended.
    static void handleEndianRotation(
        int32_t mask,
        hkaSplineCompressedAnimation::TrackCompressionParams::RotationQuantization type,
        uint8_t*& dataInOut
    );
    // Havok SDK signature (hkaAnimation.lib, tied by bytes,reloc).
    static void write16(uint16_t a1, hkArray<uint8_t, hkContainerHeapAllocator>& a2);
    // Packs the quaternion in into out as type says, through the static table packfunc.
    static void packQuaternion(TrackCompressionParams::RotationQuantization type, const hkQuaternion* in, uint8_t* out);
    // Havok SDK signature (hkaAnimation.lib, tied by bytes,reloc).
    static void writeReal(float a1, hkArray<uint8_t, hkContainerHeapAllocator>& a2);
    // Havok SDK signature (hkaAnimation.lib, tied by bytes).
    static void readPackedQuaternions(
        const uint8_t*& dataInOut,
        hkaSplineCompressedAnimation::TrackCompressionParams::RotationQuantization type,
        int32_t n,
        int32_t p,
        int32_t span,
        hkVector4* P
    );
    // Havok SDK signature (hkaAnimation.lib, tied by data).
    static void evaluateSimple2(float u, int32_t p, float* U, hkVector4* P, hkVector4& out);
    // Havok SDK signature (hkaAnimation.lib, tied by data).
    static void evaluateSimple3(float u, int32_t p, float* U, hkVector4* P, hkVector4& out);
    // The SDK's degree-1 evaluator.
    static void evaluateSimple1(float u, int32_t p, float* U, hkVector4* P, hkVector4& out);
    // The knot span of the quantized time u in the n + p + 2 knots U.
    static int32_t findSpan(int32_t n, int32_t p, uint8_t u, const uint8_t* U);
    // Swaps the byte order of one packed quaternion of the given type.
    static void handleEndianQuaternion(TrackCompressionParams::RotationQuantization type, uint8_t* dataInOut);
    // Aligns dataInOut to the type's alignment and swaps the byte order of the n + 1 control points (only the static
    // one when mask has no dynamic bit), advancing dataInOut past them. Only the low byte of mask is used, but the
    // caller passes its whole mask, so mask is declared int32_t rather than uint8_t.
    static void handleEndianRotationControlPoints(
        int32_t mask,
        int32_t n,
        TrackCompressionParams::RotationQuantization type,
        uint8_t*& dataInOut
    );
    // Havok SDK signature (hkaAnimation.lib, tied by bytes).
    static void samplePartialWithDataChunks(
        uint32_t frameIndex,
        float frameDelta,
        uint32_t maxNumTransformTracks,
        hkQsTransform* transformTracksOut,
        uint32_t maxNumFloatTracks,
        float* floatTracksOut,
        const hkaAnimation::DataChunk* dataChunks,
        int32_t numDataChunks
    );

    int32_t m_numFrames;
    int32_t m_numBlocks;
    int32_t m_maxFramesPerBlock;
    int32_t m_maskAndQuantizationSize;
    float m_blockDuration;
    float m_blockInverseDuration;
    float m_frameDuration;
    hkArray<uint32_t> m_blockOffsets;
    hkArray<uint32_t> m_floatBlockOffsets;
    hkArray<uint32_t> m_transformOffsets;
    hkArray<uint32_t> m_floatOffsets;
    hkArray<uint8_t> m_data;
    int32_t m_endian;
};

// The curve evaluators by degree: none for 0, then evaluateSimple1 to evaluateSimple3.
using hkaSplineEvaluateFunction = void (*)(float time, int32_t p, float* U, hkVector4* P, hkVector4& out);

// The bytes per quantized component, by TrackCompressionParams::ScalarQuantization.
using s_bytesPerComponent_t = int32_t[2];
inline s_bytesPerComponent_t s_bytesPerComponent{1, 2};

// The number of bits set in a three-bit mask (the dynamic components of a vector curve).
using s_bitCount_t = int32_t[8];
inline s_bitCount_t s_bitCount{0, 1, 1, 2, 1, 2, 2, 3};
using s_evaluateFunctions_t = hkaSplineEvaluateFunction[4];
inline s_evaluateFunctions_t s_evaluateFunctions{
    nullptr, &hkaSplineCompressedAnimation::evaluateSimple1, &hkaSplineCompressedAnimation::evaluateSimple2,
    &hkaSplineCompressedAnimation::evaluateSimple3
};

// The static `align` table of hkaSplineCompressedAnimation::readAlignQuaternion: each
// TrackCompressionParams::RotationQuantization's packed alignment, shared by all of this class's files.
using rdata_10EB3B8_t = const int32_t[6];
// The statics `align` of readAlignQuaternion and `size` of bytesPerQuaternion.
inline rdata_10EB3B8_t rdata_10EB3B8{4, 1, 2, 1, 2, 4};

// The static of hkaSplineCompressedAnimation::bytesPerQuaternion: each RotationQuantization's packed size in bytes.
using rdata_10EB3D0_t = const int32_t[6];
inline rdata_10EB3D0_t rdata_10EB3D0{4, 5, 6, 3, 2, 16};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkaSplineCompressedAnimationTrackCompressionParamsTypeInfo;
extern hkTypeInfo hkaSplineCompressedAnimationAnimationCompressionParamsTypeInfo;
extern hkClass hkaSplineCompressedAnimationAnimationCompressionParamsClass;
extern hkClass hkaSplineCompressedAnimationClass;
extern hkTypeInfo hkaSplineCompressedAnimationTypeInfo;
