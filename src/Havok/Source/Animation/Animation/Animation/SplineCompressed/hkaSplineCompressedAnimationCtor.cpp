#include "Havok/Source/Animation/Animation/Animation/SplineCompressed/hkaSplineCompressedAnimation.h"
#include "Havok/Source/Animation/Animation/Compression/SignedQuaternion/SignedQuaternion.h"
#include "Havok/Source/Animation/Animation/Compression/Spline/hkaNURBS.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

// Callers pass the animation's byte stream m_data (hkArray<hkUint8>).
static uint8_t* write8(char val, hkArrayBase<uint8_t>* apArray);
static char pack8(float minp, float maxp, float val);
static int32_t pack16(float minp, float maxp, float val);

// The static `mask` of hkaSplineCompressedAnimation::writeAlignQuaternion: each
// TrackCompressionParams::RotationQuantization's alignment less one.
using maskArray = const int32_t[6];
static maskArray mask{3, 0, 1, 0, 1, 3};

// The statics `packfunc` of hkaSplineCompressedAnimation::packQuaternion and `endianfunc` of handleEndianQuaternion,
// each indexed by TrackCompressionParams::RotationQuantization (POLAR32 .. UNCOMPRESSED).
using packfuncArray = void (*[6])(const hkQuaternion* in, uint8_t* out);
static packfuncArray packfunc{
    &packSignedQuaternion32,
    &packSignedQuaternion40,
    &packSignedQuaternion48,
    &packSignedQuaternion24,
    &packSignedQuaternion16,
    &packSignedQuaternion128
};
using endianfuncArray = void (*[6])(uint8_t* dataInOut);
static endianfuncArray endianfunc{
    &handleEndianSignedQuaternion32,
    &handleEndianSignedQuaternion40,
    &handleEndianSignedQuaternion48,
    &handleEndianSignedQuaternion24,
    &handleEndianSignedQuaternion16,
    &handleEndianSignedQuaternion128
};

static uint8_t* write8(char val, hkArrayBase<uint8_t>* apArray) {
    uint8_t* result = &apArray->_expandOne(hkMemoryRouter::getInstance().heap());
    *result = static_cast<uint8_t>(val);
    return result;
}

static char pack8(float minp, float maxp, float val) {
    return static_cast<int8_t>(static_cast<int32_t>(((val - minp) / (maxp - minp)) * 255.0));
}

static int32_t pack16(float minp, float maxp, float val) {
    return static_cast<uint16_t>(static_cast<int32_t>((val - minp) / (maxp - minp) * 65535.0));
}

void padZeros(int32_t numBytes, hkArrayBase<uint8_t>* apArray) {
    int32_t v2 = numBytes;
    if (numBytes > 0) {
        do {
            apArray->_pushBack(hkMemoryRouter::getInstance().heap(), static_cast<uint8_t>(0));
            --v2;
        }
        while (v2);
    }
}

void writeAlign(int32_t align, hkArrayBase<uint8_t>* apArray) {
    while ((align - 1 & apArray->m_size) != 0) {
        apArray->_pushBack(hkMemoryRouter::getInstance().heap(), static_cast<uint8_t>(0));
    }
}

int32_t writeAlignQuaternion(int32_t type, hkArrayBase<uint8_t>* apArray) {
    int32_t result;
    for (result = mask[type]; (result & apArray->m_size) != 0; result = mask[type]) {
        apArray->_pushBack(hkMemoryRouter::getInstance().heap(), static_cast<uint8_t>(0));
    }
    return result;
}

hkaSplineCompressedAnimation::AnimationCompressionParams::AnimationCompressionParams()
  : m_maxFramesPerBlock(256), m_enableSampleSingleTracks(false) {}

const hkClass& hkaSplineCompressedAnimation::AnimationCompressionParams::staticClass() {
    return hkaSplineCompressedAnimationAnimationCompressionParamsClass;
}

// The three arrays go with their members.
hkaSplineCompressedAnimation::PerTrackCompressionParams::~PerTrackCompressionParams() {}

// Each block holds 4 bytes per transform track (quantization types, then the translation, rotation and scale masks)
// and 1 per float track (mask with the quantization in bits 1-2), followed at m_maskAndQuantizationSize by the
// tracks' data.
void hkaSplineCompressedAnimation::handleEndian() {
    if (m_endian) {
        for (int32_t block = 0; block < m_numBlocks; block++) {
            uint8_t* maskPtr = m_data.m_data + m_blockOffsets.m_data[block];
            uint8_t* dataPtr = maskPtr + m_maskAndQuantizationSize;
            for (int32_t i = 0; i < m_numberOfTransformTracks; i++) {
                const uint8_t quantizationTypes = maskPtr[0];
                handleEndianScalar(
                    maskPtr[1],
                    static_cast<TrackCompressionParams::ScalarQuantization>(quantizationTypes & 3),
                    dataPtr
                );
                handleEndianRotation(
                    maskPtr[2],
                    static_cast<TrackCompressionParams::RotationQuantization>((quantizationTypes >> 2) & 0xF),
                    dataPtr
                );
                handleEndianScalar(
                    maskPtr[3],
                    static_cast<TrackCompressionParams::ScalarQuantization>(quantizationTypes >> 6),
                    dataPtr
                );
                maskPtr += 4;
            }
            for (int32_t i = 0; i < m_numberOfFloatTracks; i++) {
                const uint8_t floatMask = *maskPtr++;
                handleEndianScalar(
                    floatMask & 0xF9,
                    static_cast<TrackCompressionParams::ScalarQuantization>((floatMask >> 1) & 3),
                    dataPtr
                );
            }
        }
    }
}

// With any of the mask's dynamic bits (0xF0) set, the n + 1 control points P, each normalized; else, with a static
// bit (0x0F), the mean; each packed as type says, after padding the stream to the type's alignment.
void hkaSplineCompressedAnimation::writeRotation(
    uint8_t mask,
    hkaSplineCompressedAnimation::TrackCompressionParams::RotationQuantization type,
    const hkVector4& mean,
    int32_t n,
    const hkArray<hkVector4, hkContainerHeapAllocator>& P
) {
    if (mask & 0xF0) {
        writeAlignQuaternion(type, &m_data);
        const int32_t size = quaternionSize[type];
        for (int32_t i = 0; i <= n; i++) {
            alignas(16) hkQuaternion q;
            q.m_vec.m_quad = P.m_data[i].m_quad;
            q.normalize();
            padZeros(size, &m_data);
            packQuaternion(type, &q, m_data.m_data + (m_data.m_size - size));
        }
    }
    else if (mask & 0x0F) {
        writeAlignQuaternion(type, &m_data);
        const int32_t size = quaternionSize[type];
        alignas(16) hkQuaternion q;
        q.m_vec.m_quad = mean.m_quad;
        q.normalize();
        padZeros(size, &m_data);
        packQuaternion(type, &q, m_data.m_data + (m_data.m_size - size));
    }
}

// Per component x, y, z: a dynamic one (mask bit 4 + i) writes its minimum and maximum, a static one (bit i) the mean.
// Then, with any dynamic component, each of the n + 1 control points' dynamic components quantized between them (8 or
// 16 bits as type says).
void hkaSplineCompressedAnimation::writePoints(
    uint8_t mask,
    hkaSplineCompressedAnimation::TrackCompressionParams::ScalarQuantization type,
    const hkVector4& mean,
    const hkVector4& minp,
    const hkVector4& maxp,
    int32_t n,
    hkArray<hkVector4, hkContainerHeapAllocator>& P
) {
    writeAlign(4, &m_data);
    for (int32_t k = 0; k < 3; k++) {
        if (mask & (0x10 << k)) {
            float fMin = minp(k);
            writeReal(fMin, m_data);
            float fMax = maxp(k);
            writeReal(fMax, m_data);
        }
        else if (mask & (1 << k)) {
            float fMean = mean(k);
            writeReal(fMean, m_data);
        }
    }
    if (mask & 0xF0) {
        for (int32_t i = 0; i <= n; i++) {
            for (int32_t k = 0; k < 3; k++) {
                if (mask & (0x10 << k)) {
                    float fValue = P.m_data[i](k);
                    float fMax = maxp(k);
                    float fMin = minp(k);
                    if (type == TrackCompressionParams::BITS8) {
                        write8(pack8(fMin, fMax, fValue), &m_data);
                    }
                    else {
                        write16(static_cast<uint16_t>(pack16(fMin, fMax, fValue)), m_data);
                    }
                }
            }
        }
    }
}

// The knot count n (16 bits) and the degree p (8 bits), then the n + p + 2 knots, each the nearest frame index of
// its time (truncated after adding 0.5).
void hkaSplineCompressedAnimation::writeKnots(int32_t n, int32_t p, const hkArray<float, hkContainerHeapAllocator>& U) {
    const int32_t last = n + p + 1;
    write16(static_cast<uint16_t>(n), m_data);
    write8(static_cast<char>(p), &m_data);
    for (int32_t i = 0; i <= last; i++) {
        const uint8_t knot = static_cast<uint8_t>(
            static_cast<int32_t>(U.m_data[i] / m_duration * (m_numFrames - 1) + 0.5)
        );
        write8(static_cast<char>(knot), &m_data);
    }
}

void hkaSplineCompressedAnimation::packQuaternion(
    TrackCompressionParams::RotationQuantization type,
    const hkQuaternion* in,
    uint8_t* out
) {
    packfunc[type](in, out);
}

void hkaSplineCompressedAnimation::handleEndianQuaternion(
    TrackCompressionParams::RotationQuantization type,
    uint8_t* dataInOut
) {
    endianfunc[type](dataInOut);
}

void hkaSplineCompressedAnimation::handleEndianRotationControlPoints(
    int32_t mask,
    int32_t n,
    TrackCompressionParams::RotationQuantization type,
    uint8_t*& dataInOut
) {
    // Tests the mask's low byte only.
    if (static_cast<uint8_t>(mask)) {
        uint8_t* v4 = reinterpret_cast<uint8_t*>(
            ~static_cast<uintptr_t>(quaternionAlign[type] - 1) &
            (reinterpret_cast<uintptr_t>(dataInOut) + quaternionAlign[type] - 1)
        );
        dataInOut = v4;
        if ((mask & 0xF0) != 0) {
            if (n >= 0) {
                int32_t v5 = n + 1;
                do {
                    handleEndianQuaternion(type, dataInOut);
                    dataInOut += quaternionSize[type];
                    --v5;
                }
                while (v5);
            }
        }
        else {
            handleEndianQuaternion(type, v4);
            dataInOut += quaternionSize[type];
        }
    }
}
