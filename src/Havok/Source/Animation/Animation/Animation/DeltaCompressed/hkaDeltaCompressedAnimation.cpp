#include "Havok/Source/Animation/Animation/Animation/DeltaCompressed/hkaDeltaCompressedAnimation.h"

#include "BSCore/MemoryManager.h"
#include "Havok/Source/Animation/Animation/Animation/Util/hkaTrackAnalysis.h"
#include "Havok/Source/Animation/Animation/Animation/WaveletCompressed/hkaWaveletCompressedAnimation.h"
#include "Havok/Source/Animation/Animation/Animation/hkaAnimation.h"
#include "Havok/Source/Animation/Animation/Animation/hkaAnnotationTrackClass.h"
#include "Havok/Source/Animation/Animation/Compression/Encoding/hkaBlockDecode.h"
#include "Havok/Source/Animation/Animation/Compression/Quantization/hkaQuantizeReal.h"
#include "Havok/Source/Animation/Animation/Compression/Spline/hkaNURBS.h"
#include "Havok/Source/Animation/Animation/Compression/Wavelet/hkaWavelet.h"
#include "Havok/Source/Animation/Animation/Playback/Cache/hkaChunkCache.h"
#include "Havok/Source/Animation/Internal/Compression/hkaCompression.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalBuffer.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Thread/Thread/hkThreadLocalData.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "SDK/Windows/winbase.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

// Decompresses the block holding frame auiFrame of apAnimation's aiNumD dimensions into apOut through decompressBlock,
// under the "decompressBlockCache" monitor timer.
static void
decompressBlockToCache(uint32_t auiFrame, float* apOut, int32_t aiNumD, const hkaDeltaCompressedAnimation* apAnimation);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkaDeltaCompressedAnimationQuantizationFormatTypeInfo = {
    "hkaDeltaCompressedAnimationQuantizationFormat",
    finishLoadedObjecthkaDeltaCompressedAnimationQuantizationFormat,
    cleanupLoadedObjecthkaDeltaCompressedAnimationQuantizationFormat,
    nullptr,
    &typeid(hkaDeltaCompressedAnimation::QuantizationFormat),
    sizeof(hkaDeltaCompressedAnimation::QuantizationFormat)
};

// The reflected members of hkaDeltaCompressedAnimation::QuantizationFormat and of hkaDeltaCompressedAnimation.
using hkaDeltaCompressedAnimation_QuantizationFormatClass_MembersArray = const hkClassMember[6];
static hkaDeltaCompressedAnimation_QuantizationFormatClass_MembersArray
    hkaDeltaCompressedAnimation_QuantizationFormatClass_Members{
        {"maxBitWidth", nullptr, nullptr, hkClassMember::TYPE_UINT8, hkClassMember::TYPE_VOID, 0, 0, 0x0, nullptr},
        {"preserved", nullptr, nullptr, hkClassMember::TYPE_UINT8, hkClassMember::TYPE_VOID, 0, 0, 0x1, nullptr},
        {"numD", nullptr, nullptr, hkClassMember::TYPE_UINT32, hkClassMember::TYPE_VOID, 0, 0, 0x4, nullptr},
        {"offsetIdx", nullptr, nullptr, hkClassMember::TYPE_UINT32, hkClassMember::TYPE_VOID, 0, 0, 0x8, nullptr},
        {"scaleIdx", nullptr, nullptr, hkClassMember::TYPE_UINT32, hkClassMember::TYPE_VOID, 0, 0, 0xC, nullptr},
        {"bitWidthIdx", nullptr, nullptr, hkClassMember::TYPE_UINT32, hkClassMember::TYPE_VOID, 0, 0, 0x10, nullptr}
};
using hkaDeltaCompressedAnimationClass_MembersArray = const hkClassMember[14];
static hkaDeltaCompressedAnimationClass_MembersArray hkaDeltaCompressedAnimationClass_Members{
    {"numberOfPoses", nullptr, nullptr, hkClassMember::TYPE_INT32, hkClassMember::TYPE_VOID, 0, 0, 0x28, nullptr},
    {"blockSize", nullptr, nullptr, hkClassMember::TYPE_INT32, hkClassMember::TYPE_VOID, 0, 0, 0x2C, nullptr},
    {"qFormat",
     &hkaDeltaCompressedAnimationQuantizationFormatClass,
     nullptr,
     hkClassMember::TYPE_STRUCT,
     hkClassMember::TYPE_VOID,
     0,
     0,
     0x30,
     nullptr},
    {"quantizedDataIdx", nullptr, nullptr, hkClassMember::TYPE_UINT32, hkClassMember::TYPE_VOID, 0, 0, 0x44, nullptr},
    {"quantizedDataSize", nullptr, nullptr, hkClassMember::TYPE_UINT32, hkClassMember::TYPE_VOID, 0, 0, 0x48, nullptr},
    {"staticMaskIdx", nullptr, nullptr, hkClassMember::TYPE_UINT32, hkClassMember::TYPE_VOID, 0, 0, 0x4C, nullptr},
    {"staticMaskSize", nullptr, nullptr, hkClassMember::TYPE_UINT32, hkClassMember::TYPE_VOID, 0, 0, 0x50, nullptr},
    {"staticDOFsIdx", nullptr, nullptr, hkClassMember::TYPE_UINT32, hkClassMember::TYPE_VOID, 0, 0, 0x54, nullptr},
    {"staticDOFsSize", nullptr, nullptr, hkClassMember::TYPE_UINT32, hkClassMember::TYPE_VOID, 0, 0, 0x58, nullptr},
    {"numStaticTransformDOFs",
     nullptr,
     nullptr,
     hkClassMember::TYPE_UINT32,
     hkClassMember::TYPE_VOID,
     0,
     0,
     0x5C,
     nullptr},
    {"numDynamicTransformDOFs",
     nullptr,
     nullptr,
     hkClassMember::TYPE_UINT32,
     hkClassMember::TYPE_VOID,
     0,
     0,
     0x60,
     nullptr},
    {"totalBlockSize", nullptr, nullptr, hkClassMember::TYPE_UINT32, hkClassMember::TYPE_VOID, 0, 0, 0x64, nullptr},
    {"lastBlockSize", nullptr, nullptr, hkClassMember::TYPE_UINT32, hkClassMember::TYPE_VOID, 0, 0, 0x68, nullptr},
    {"dataBuffer", nullptr, nullptr, hkClassMember::TYPE_ARRAY, hkClassMember::TYPE_UINT8, 0, 0, 0x6C, nullptr}
};
hkClass hkaDeltaCompressedAnimationQuantizationFormatClass(
    "hkaDeltaCompressedAnimationQuantizationFormat",
    nullptr,
    20,
    nullptr,
    0,
    nullptr,
    0,
    hkaDeltaCompressedAnimation_QuantizationFormatClass_Members,
    6,
    nullptr,
    nullptr,
    0,
    0
);
hkClass hkaDeltaCompressedAnimationClass(
    "hkaDeltaCompressedAnimation",
    &hkaAnimationClass,
    120,
    nullptr,
    0,
    nullptr,
    0,
    hkaDeltaCompressedAnimationClass_Members,
    14,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkaDeltaCompressedAnimationTypeInfo = {
    "hkaDeltaCompressedAnimation",
    finishLoadedObjecthkaDeltaCompressedAnimation,
    cleanupLoadedObjecthkaDeltaCompressedAnimation,
    getVtablehkaDeltaCompressedAnimation(),
    &typeid(hkaDeltaCompressedAnimation),
    sizeof(hkaDeltaCompressedAnimation)
};

// The block's frame count is m_blockSize, clamped to the animation's frames; the frame-to-block division is unsigned,
// the clamps signed. The offsets and scales are floats inside the animation's byte buffer, treated as untyped memory,
// hence the casts.
// KEEP: the whole params local is passed by value to decompressBlock, and the padding after m_maxBitWidth is never
// written (only m_preserved and m_maxBitWidth are stored): bytes nothing wrote, passed to a callee.
static void decompressBlockToCache(
    uint32_t auiFrame,
    float* apOut,
    int32_t aiNumD,
    const hkaDeltaCompressedAnimation* apAnimation
) {
    HK_TIMER_BEGIN("decompressBlockCacheD", nullptr);
    const int32_t blockSize = apAnimation->m_blockSize;
    const int32_t numberOfPoses = apAnimation->m_numberOfPoses;
    uint32_t blockIdx = auiFrame / blockSize;
    int32_t numValues = blockSize;
    if (blockSize > numberOfPoses) {
        numValues = numberOfPoses;
        blockIdx = 0;
    }
    if (static_cast<int32_t>(blockIdx * blockSize + numValues) > numberOfPoses) {
        numValues = numberOfPoses - blockIdx * blockSize;
    }
    const uint8_t* pData = apAnimation->m_dataBuffer.m_data;
    hkDeltaDecompressionParameters params;
    params.m_numD = aiNumD;
    params.m_blockSize = blockSize;
    params.m_numValues = numValues;
    params.m_preserved = apAnimation->m_qFormat.m_preserved;
    params.m_maxBitWidth = apAnimation->m_qFormat.m_maxBitWidth;
    params.m_offset = reinterpret_cast<const float*>(&pData[apAnimation->m_qFormat.m_offsetIdx]);
    params.m_scale = reinterpret_cast<const float*>(&pData[apAnimation->m_qFormat.m_scaleIdx]);
    params.m_bitWidth = &pData[apAnimation->m_qFormat.m_bitWidthIdx];
    params.m_quantizedData = &pData[apAnimation->m_quantizedDataIdx + blockIdx * apAnimation->m_totalBlockSize];
    decompressBlock(params, apOut);
    HK_TIMER_END();
}

hkaDeltaCompressedAnimation::CompressionParams::CompressionParams()
  : m_quantizationBits(8), m_blockSize(0xFFFF), m_absolutePositionTolerance(0.0f), m_relativePositionTolerance(0.01f),
    m_rotationTolerance(0.001f), m_scaleTolerance(0.01f), m_absoluteFloatTolerance(0.001f) {}

// The data buffer goes with its member.
hkaDeltaCompressedAnimation::~hkaDeltaCompressedAnimation() {}

void hkaDeltaCompressedAnimation::sampleTracks(
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

// The dynamic degrees of freedom of the frame's block are taken from the chunk cache or decompressed into a stack
// array, interpolated into another stack array (at a block's last frame the next block's first values are fetched the
// same way and added in), then recomposed with the static ones. A partial sample handles only the dynamic degrees of
// freedom of the tracks asked for.
void hkaDeltaCompressedAnimation::samplePartialTracks(
    float time,
    uint32_t maxNumTransformTracks,
    hkQsTransform* transformTracksOut,
    uint32_t maxNumFloatTracks,
    float* floatTracksOut,
    hkaChunkCache* cache
) const {
    HK_TIMER_BEGIN_LIST("SampleDelta", "StaticD");
    // The data buffer is untyped bytes; the static masks are uint16_t values at m_staticMaskIdx.
    const uint16_t* staticMaskT = reinterpret_cast<const uint16_t*>(&m_dataBuffer.m_data[m_staticMaskIdx]);
    const int32_t numD = static_cast<int32_t>(m_qFormat.m_numD);
    uint32_t numDToSample = static_cast<uint32_t>(numD);
    if (maxNumTransformTracks != static_cast<uint32_t>(m_numberOfTransformTracks) ||
        maxNumFloatTracks != static_cast<uint32_t>(m_numberOfFloatTracks)) {
        hkaTrackAnalysis::AnalysisStats stats;
        // With float tracks asked for, every transform track's degrees of freedom come first in the data.
        if (maxNumFloatTracks == 0) {
            hkaTrackAnalysis::calcDynamicStats(
                staticMaskT,
                staticMaskT + m_numberOfTransformTracks,
                maxNumTransformTracks,
                0,
                stats
            );
            numDToSample = stats.m_dynamicPosDOF + stats.m_dynamicRotDOF + stats.m_dynamicSclDOF;
        }
        else {
            hkaTrackAnalysis::calcDynamicStats(
                staticMaskT,
                staticMaskT + m_numberOfTransformTracks,
                static_cast<uint32_t>(m_numberOfTransformTracks),
                maxNumFloatTracks,
                stats
            );
            numDToSample = stats.m_dynamicPosDOF + stats.m_dynamicRotDOF + stats.m_dynamicSclDOF +
                stats.m_dynamicFloatDOF;
        }
    }

    HK_TIMER_SPLIT_LIST("DecompressD");
    uint32_t frame;
    float delta;
    getFrameAndDelta(time, frame, delta);
    const float oneMinusDelta = 1.0f - delta;
    const int32_t numSampled = static_cast<int32_t>(numDToSample);
    hkLocalArray<float> sampled(numSampled);
    sampled.setSize(numSampled);
    hkLocalArray<float> decompressed(numSampled * m_blockSize);
    decompressed.setSize(numSampled * m_blockSize);

    // The chunk cache hands out untyped bytes; the decompressed chunk is floats.
    const float* data = nullptr;
    if (cache) {
        data = reinterpret_cast<const float*>(retrieveFromCache(this, frame, numD, &cache));
    }
    if (!data) {
        decompressBlockToCache(frame, decompressed.m_data, numSampled, this);
        data = decompressed.m_data;
    }
    HK_TIMER_SPLIT_LIST("InterpD");
    uint32_t index = frame % static_cast<uint32_t>(m_blockSize);
    if (index + 1 != static_cast<uint32_t>(m_blockSize)) {
        // Both frames are in this block.
        for (uint32_t i = 0; i < numDToSample; ++i) {
            sampled.m_data[i] = data[index + 1] * delta + data[index] * oneMinusDelta;
            index += m_blockSize;
        }
        if (data != decompressed.m_data) {
            unlockCachedChunk(numD, this, frame, &cache);
        }
    }
    else {
        // The next frame is the next block's first.
        for (uint32_t i = 0; i < numDToSample; ++i) {
            sampled.m_data[i] = data[index] * oneMinusDelta;
            index += m_blockSize;
        }
        if (data != decompressed.m_data) {
            unlockCachedChunk(numD, this, frame, &cache);
        }
        HK_TIMER_SPLIT_LIST("DecompressD");
        ++frame;
        const float* nextData = nullptr;
        if (cache) {
            nextData = reinterpret_cast<const float*>(retrieveFromCache(this, frame, numD, &cache));
        }
        if (!nextData) {
            decompressBlockToCache(frame, decompressed.m_data, numSampled, this);
            nextData = decompressed.m_data;
        }
        HK_TIMER_SPLIT_LIST("InterpD");
        uint32_t nextIndex = 0;
        for (uint32_t i = 0; i < numDToSample; ++i) {
            sampled.m_data[i] = nextData[nextIndex] * delta + sampled.m_data[i];
            nextIndex += m_blockSize;
        }
        if (nextData != decompressed.m_data) {
            unlockCachedChunk(numD, this, frame, &cache);
        }
    }

    HK_TIMER_SPLIT_LIST("RecomposeD");
    // The static degrees of freedom are floats at m_staticDOFsIdx in the untyped data buffer.
    const float* staticDOFs = reinterpret_cast<const float*>(&m_dataBuffer.m_data[m_staticDOFsIdx]);
    hkaTrackAnalysis::AnalysisCompose input;
    input.m_numberOfTransformTracks = maxNumTransformTracks;
    input.m_numberOfFloatTracks = maxNumFloatTracks;
    input.m_staticMaskTransform = staticMaskT;
    input.m_staticMaskFloat = staticMaskT + m_numberOfTransformTracks;
    input.m_staticDOFsTransform = staticDOFs;
    input.m_staticDOFsFloat = staticDOFs + m_numStaticTransformDOFs;
    input.m_dynamicDOFsTransform = sampled.m_data;
    input.m_dynamicDOFsFloat = sampled.m_data + m_numDynamicTransformDOFs;
    hkaTrackAnalysis::recompose(input, transformTracksOut, floatTracksOut);
    HK_TIMER_END_LIST();
}

void hkaDeltaCompressedAnimation::clearAllCacheKeys(hkaChunkCache* cache) {
    int32_t m_blockSize = this->m_blockSize;
    int32_t v4 = (getNumOriginalFrames() - 1) / m_blockSize;
    uint32_t v5 = 4 * (m_blockSize * m_qFormat.m_numD);
    int32_t v6 = 0;
    for (int32_t i = v4 + 1; v6 < i; ++v6) {
        // Havok's cache key is the data buffer's address truncated to hkUint32 plus the block index, as
        // getFullCacheKey forms it.
        cache->flushKey(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(m_dataBuffer.begin())) + v6, v5);
    }
}

void hkaDeltaCompressedAnimation::sampleIndividualTransformTracks(
    float time,
    const int16_t* tracks,
    uint32_t numTracks,
    hkQsTransform* transformTracksOut
) const {
    uint32_t v5 = numTracks;
    if (numTracks) {
        hkVector4 v6;
        v6.m_quad = _mm_shuffle_ps(
            _mm_set_ss(std::bit_cast<float>(0x3F800000u)),
            _mm_set_ss(std::bit_cast<float>(0x3F800000u)),
            0
        );
        hkVector4* p_m_scale = &transformTracksOut->m_scale;
        do {
            p_m_scale[-2].m_quad = _mm_setzero_ps();
            p_m_scale[-1].m_quad = hkQuadReal0001;
            (*p_m_scale).m_quad = v6.m_quad;
            p_m_scale += 3;
            --v5;
        }
        while (v5);
    }
}

// Fills the float tracks with zeroes, only when the count is nonzero; the time and the track list are not read.
void hkaDeltaCompressedAnimation::sampleIndividualFloatTracks(
    float time,
    const int16_t* tracks,
    uint32_t numTracks,
    float* floatTracksOut
) const {
    if (numTracks) {
        for (uint32_t i = 0; i < numTracks; ++i) {
            floatTracksOut[i] = 0.0f;
        }
    }
}

int32_t hkaDeltaCompressedAnimation::getNumOriginalFrames() const {
    return m_numberOfPoses;
}

int32_t hkaDeltaCompressedAnimation::getNumDataChunks(uint32_t frame, float delta) const {
    return 4 - (static_cast<uint32_t>(m_blockSize) != ((frame % static_cast<uint32_t>(m_blockSize)) + 1));
}

// Chunk 0 is the animation object itself, chunk 1 the buffer's header up to the quantized data rounded up to 16, chunk
// 2 the frame's block and, when four chunks are asked for, chunk 3 the next block (wrapping round). The block index is
// divided unsigned, the block count signed.
void hkaDeltaCompressedAnimation::getDataChunks(
    uint32_t frame,
    float delta,
    hkaAnimation::DataChunk* dataChunks,
    int32_t numDataChunks
) const {
    dataChunks[0].m_offset = static_cast<uint8_t>(reinterpret_cast<uintptr_t>(this) & 15);
    dataChunks[0].m_data = this;
    dataChunks[0].m_size = 128;
    const uint8_t* pData = m_dataBuffer.m_data;
    dataChunks[1].m_data = pData;
    dataChunks[1].m_offset = static_cast<uint8_t>(reinterpret_cast<uintptr_t>(pData) & 15);
    dataChunks[1].m_size = (m_quantizedDataIdx + 15) & ~15u;
    const uint32_t blockNum = frame / m_blockSize;
    const int32_t numBlocks = (m_numberOfPoses + m_blockSize - 1) / m_blockSize;
    getBlockDataAndSize(blockNum, numBlocks, dataChunks[2]);
    if (numDataChunks == 4) {
        getBlockDataAndSize((blockNum + 1) % numBlocks, numBlocks, dataChunks[3]);
    }
}

// The largest size of the chunks getDataChunks hands out for one frame: the object (128 bytes), the buffer's header up
// to the quantized data rounded up to 16, a block and, but for the last block, the next one. The block count and the
// running maximum are signed, the block index unsigned.
int32_t hkaDeltaCompressedAnimation::getMaxSizeOfCombinedDataChunks() const {
    const uint32_t numBlocks = (m_numberOfPoses + m_blockSize - 1) / m_blockSize;
    int32_t maxSize = 0;
    if (!numBlocks) {
        return 0;
    }
    const uint32_t uiHeaderSize = (m_quantizedDataIdx + 15) & ~15u;
    for (uint32_t blockNum = 0; blockNum < numBlocks; ++blockNum) {
        // The chunk is zeroed (DataChunk's constructor) each time round, before getBlockDataAndSize overwrites every
        // member.
        hkaAnimation::DataChunk chunk;
        chunk.m_data = nullptr;
        chunk.m_size = 0;
        chunk.m_offset = 0;
        getBlockDataAndSize(blockNum, numBlocks, chunk);
        int32_t size = static_cast<int32_t>(chunk.m_size + uiHeaderSize + 128);
        if (blockNum < numBlocks - 1) {
            getBlockDataAndSize(blockNum + 1, numBlocks, chunk);
            size += chunk.m_size;
        }
        if (size > maxSize) {
            maxSize = size;
        }
    }
    return maxSize;
}

// The SDK keys the cache by the data buffer's address plus the block index; the key is 32 bits wide.
uint32_t hkaDeltaCompressedAnimation::getFullCacheKey(uint32_t poseIdx) const {
    return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(m_dataBuffer.begin())) + poseIdx / m_blockSize;
}

// The data's first byte is nonzero for data saved with the other byte order. Each value is swapped through a union, as
// the SDK's swaps are; the data buffer is untyped bytes the values are read from in place.
void hkaDeltaCompressedAnimation::handleEndian() {
    if (m_staticMaskIdx == 4) {
        uint8_t* pData = m_dataBuffer.begin();
        if (*pData) {
            // The static masks, one uint16_t per transform track and then per float track, at m_staticMaskIdx (4).
            uint16_t* pStaticMaskT = reinterpret_cast<uint16_t*>(pData + 4);
            uint16_t* pMask = pStaticMaskT;
            for (int32_t i = 0; i < m_numberOfTransformTracks + m_numberOfFloatTracks; ++i, ++pMask) {
                union {
                    uint16_t u;
                    uint8_t c[2];
                } in, out;
                in.u = *pMask;
                out.c[0] = in.c[1];
                out.c[1] = in.c[0];
                *pMask = out.u;
            }

            hkaTrackAnalysis::AnalysisStats stats;
            hkaTrackAnalysis::calcStats(
                pStaticMaskT,
                pStaticMaskT + m_numberOfTransformTracks,
                m_numberOfTransformTracks,
                m_numberOfFloatTracks,
                stats
            );

            // The static values: one float per static degree of freedom.
            float* pStaticDOFs = reinterpret_cast<float*>(m_dataBuffer.begin() + m_staticDOFsIdx);
            const int32_t numStaticDOFs = stats.m_staticPosDOF + stats.m_staticRotDOF + stats.m_staticSclDOF +
                stats.m_staticFloatDOF;
            for (int32_t i = 0; i < numStaticDOFs; ++i) {
                union {
                    float r;
                    uint8_t c[4];
                } in, out;
                in.r = pStaticDOFs[i];
                out.c[0] = in.c[3];
                out.c[1] = in.c[2];
                out.c[2] = in.c[1];
                out.c[3] = in.c[0];
                pStaticDOFs[i] = out.r;
            }

            // The quantization's offset and scale of each dynamic degree of freedom, swapped in pairs.
            uint8_t* pQuantData = m_dataBuffer.begin();
            const uint32_t numDynamicDOFs = stats.m_dynamicPosDOF + stats.m_dynamicRotDOF + stats.m_dynamicSclDOF +
                stats.m_dynamicFloatDOF;
            float* pOffsets = reinterpret_cast<float*>(pQuantData + m_qFormat.m_offsetIdx);
            float* pScales = reinterpret_cast<float*>(pQuantData + m_qFormat.m_scaleIdx);
            for (uint32_t i = 0; i < numDynamicDOFs; ++i) {
                union {
                    float r;
                    uint8_t c[4];
                } in, out;
                in.r = pOffsets[i];
                out.c[0] = in.c[3];
                out.c[1] = in.c[2];
                out.c[2] = in.c[1];
                out.c[3] = in.c[0];
                pOffsets[i] = out.r;
                in.r = pScales[i];
                out.c[0] = in.c[3];
                out.c[1] = in.c[2];
                out.c[2] = in.c[1];
                out.c[3] = in.c[0];
                pScales[i] = out.r;
            }
        }
    }
}

// The block's data start at m_quantizedDataIdx in the buffer, m_totalBlockSize bytes per block. The chunk is that
// address rounded down to 16 bytes with the remainder as its offset; its size covers the offset and the block (the
// last one is m_lastBlockSize long), rounded up to 16.
void hkaDeltaCompressedAnimation::getBlockDataAndSize(
    int32_t blockNum,
    int32_t numBlocks,
    hkaAnimation::DataChunk& dataChunkOut
) const {
    const uint8_t* pBlock = &m_dataBuffer.m_data[blockNum * m_totalBlockSize + m_quantizedDataIdx];
    const uint8_t ucOffset = static_cast<uint8_t>(reinterpret_cast<uintptr_t>(pBlock) & 15);
    dataChunkOut.m_data = reinterpret_cast<const void*>(
        reinterpret_cast<uintptr_t>(pBlock) & ~static_cast<uintptr_t>(15)
    );
    dataChunkOut.m_offset = ucOffset;
    const uint32_t uiBlockSize = (blockNum == numBlocks - 1) ? m_lastBlockSize : m_totalBlockSize;
    dataChunkOut.m_size = (ucOffset + uiBlockSize + 15) & ~15u;
}

// samplePartialTracks working from the data chunks getDataChunks filled (chunk 0 the animation object, 1 the data
// buffer's header, 2 the frame's block and, for four chunks, 3 the next block) instead of the animation and its cache.
// The dynamic degrees of freedom of the frame's block are decompressed into stack memory and interpolated into another
// stack buffer (at a block's last frame the next block is decompressed and added in), then recomposed with the static
// ones. The stack memory is taken and given back inline (hkAllocateStack, hkDeallocateStack). The decompression
// parameters are passed by value with their two padding bytes unwritten; decompressBlock reads neither.
void hkaDeltaCompressedAnimation::samplePartialWithDataChunks(
    uint32_t frameIndex,
    float delta,
    uint32_t maxNumTransformTracks,
    hkQsTransform* transformTracksOut,
    uint32_t maxNumFloatTracks,
    float* floatTracksOut,
    const hkaAnimation::DataChunk* dataChunks,
    int32_t numDataChunks
) {
    // The chunks are untyped bytes: chunk 0 holds the animation object, chunk 1 the data buffer.
    const hkaDeltaCompressedAnimation* anim = static_cast<const hkaDeltaCompressedAnimation*>(
        static_cast<const void*>(static_cast<const uint8_t*>(dataChunks[0].m_data) + dataChunks[0].m_offset)
    );
    HK_TIMER_BEGIN_LIST("SampleDeltaChunk", "StaticD");
    const uint8_t* dataBuffer = static_cast<const uint8_t*>(dataChunks[1].m_data) + dataChunks[1].m_offset;
    // The static masks are uint16_t values and the static degrees of freedom floats in the untyped buffer.
    const uint16_t* staticMaskT = static_cast<const uint16_t*>(
        static_cast<const void*>(dataBuffer + anim->m_staticMaskIdx)
    );
    const float* staticDOFs = static_cast<const float*>(static_cast<const void*>(dataBuffer + anim->m_staticDOFsIdx));
    uint32_t numD = anim->m_qFormat.m_numD;
    if (maxNumTransformTracks != static_cast<uint32_t>(anim->m_numberOfTransformTracks) ||
        maxNumFloatTracks != static_cast<uint32_t>(anim->m_numberOfFloatTracks)) {
        hkaTrackAnalysis::AnalysisStats stats;
        // With float tracks asked for, every transform track's degrees of freedom come first in the data.
        if (maxNumFloatTracks == 0) {
            hkaTrackAnalysis::calcDynamicStats(
                staticMaskT,
                staticMaskT + anim->m_numberOfTransformTracks,
                maxNumTransformTracks,
                0,
                stats
            );
            numD = stats.m_dynamicSclDOF + stats.m_dynamicRotDOF + stats.m_dynamicPosDOF;
        }
        else {
            hkaTrackAnalysis::calcDynamicStats(
                staticMaskT,
                staticMaskT + anim->m_numberOfTransformTracks,
                static_cast<uint32_t>(anim->m_numberOfTransformTracks),
                maxNumFloatTracks,
                stats
            );
            numD = stats.m_dynamicFloatDOF + stats.m_dynamicSclDOF + stats.m_dynamicRotDOF + stats.m_dynamicPosDOF;
        }
    }

    HK_TIMER_SPLIT_LIST("DecompressDChunk");
    const float oneMinusDelta = 1.0f - delta;
    float* decompressed = hkAllocateStack<float>(anim->m_blockSize * static_cast<int32_t>(numD), nullptr);
    float* sampled = hkAllocateStack<float>(static_cast<int32_t>(numD), nullptr);

    // The frame's block holds m_blockSize poses, the last block what is left of m_numberOfPoses.
    hkDeltaDecompressionParameters params;
    {
        int32_t numValues = anim->m_blockSize;
        const int32_t numPoses = anim->m_numberOfPoses;
        if (numValues > numPoses) {
            numValues = numPoses;
        }
        else {
            const uint32_t blockNum = frameIndex / static_cast<uint32_t>(numValues);
            if (static_cast<int32_t>((blockNum + 1) * numValues) > numPoses) {
                numValues = numPoses - static_cast<int32_t>(blockNum * numValues);
            }
        }
        params.m_numValues = numValues;
    }
    params.m_blockSize = anim->m_blockSize;
    params.m_preserved = anim->m_qFormat.m_preserved;
    params.m_maxBitWidth = anim->m_qFormat.m_maxBitWidth;
    // The offsets and scales are floats in the untyped buffer, the bit widths bytes.
    params.m_offset = static_cast<const float*>(static_cast<const void*>(dataBuffer + anim->m_qFormat.m_offsetIdx));
    params.m_scale = static_cast<const float*>(static_cast<const void*>(dataBuffer + anim->m_qFormat.m_scaleIdx));
    params.m_bitWidth = dataBuffer + anim->m_qFormat.m_bitWidthIdx;
    params.m_numD = numD;
    params.m_quantizedData = static_cast<const uint8_t*>(dataChunks[2].m_data) + dataChunks[2].m_offset;
    decompressBlock(params, decompressed);
    const uint32_t blockSize = static_cast<uint32_t>(anim->m_blockSize);
    uint32_t index = frameIndex % blockSize;
    if (numDataChunks == 3) {
        // Both frames are in this block.
        for (uint32_t i = 0; i < numD; ++i) {
            sampled[i] = decompressed[index + 1] * delta + decompressed[index] * oneMinusDelta;
            index += blockSize;
        }
    }
    else {
        // The next frame is the next block's first.
        for (uint32_t i = 0; i < numD; ++i) {
            sampled[i] = decompressed[index] * oneMinusDelta;
            index += blockSize;
        }
        params.m_quantizedData = static_cast<const uint8_t*>(dataChunks[3].m_data) + dataChunks[3].m_offset;
        const uint32_t nextBlockNum = (frameIndex + 1) / blockSize;
        if (static_cast<int32_t>((nextBlockNum + 1) * anim->m_blockSize) > anim->m_numberOfPoses) {
            params.m_numValues = anim->m_numberOfPoses - static_cast<int32_t>(nextBlockNum * blockSize);
        }
        decompressBlock(params, decompressed);
        uint32_t nextIndex = 0;
        for (uint32_t i = 0; i < numD; ++i) {
            sampled[i] = decompressed[nextIndex] * delta + sampled[i];
            nextIndex += anim->m_blockSize;
        }
    }

    HK_TIMER_SPLIT_LIST("RecomposeDChunk");
    hkaTrackAnalysis::AnalysisCompose input;
    input.m_numberOfTransformTracks = maxNumTransformTracks;
    input.m_numberOfFloatTracks = maxNumFloatTracks;
    input.m_staticMaskTransform = staticMaskT;
    input.m_staticDOFsTransform = staticDOFs;
    input.m_staticDOFsFloat = staticDOFs + anim->m_numStaticTransformDOFs;
    input.m_dynamicDOFsTransform = sampled;
    input.m_dynamicDOFsFloat = sampled + anim->m_numDynamicTransformDOFs;
    input.m_staticMaskFloat = staticMaskT + anim->m_numberOfTransformTracks;
    hkaTrackAnalysis::recompose(input, transformTracksOut, floatTracksOut);
    hkDeallocateStack(sampled);
    hkDeallocateStack(decompressed);
    HK_TIMER_END_LIST();
}

const hkClass& hkaDeltaCompressedAnimation::staticClass() {
    return hkaDeltaCompressedAnimationClass;
}

// Decompresses one block: for each of the m_numD dimensions, expands its quantized values into apOut, undoes the
// delta encoding, then steps apOut by the block size and the quantized data by that dimension's quantized size. The
// bit width is the dimension's own when the block has a table of them, else the format's maximum.
void decompressBlock(hkDeltaDecompressionParameters aParams, float* apOut) {
    hkaQuantizeDesc desc;
    desc.m_preserved = aParams.m_preserved;
    int32_t iDataOffset = 0;
    for (uint32_t i = 0; i < aParams.m_numD; ++i) {
        desc.m_offset = aParams.m_offset[i];
        desc.m_scale = aParams.m_scale[i];
        desc.m_bitWidth = aParams.m_bitWidth ? aParams.m_bitWidth[i] : aParams.m_maxBitWidth;
        hkExpandReal(aParams.m_quantizedData + iDataOffset, desc, apOut, aParams.m_numValues);
        hkInverseDeltaTransform(apOut, aParams.m_numValues);
        apOut += aParams.m_blockSize;
        iDataOffset += hkCalcQuantizedSize(aParams.m_numValues, desc);
    }
}

// The chunk cache hands out untyped bytes; the decompressed chunk is floats, so decompressBlockToCache gets it as
// float*. *appCache is reloaded before every call of the cache.
const uint8_t* retrieveFromCache(
    const hkaDeltaCompressedAnimation* apAnimation,
    uint32_t auiChunkId,
    int32_t aiSize,
    hkaChunkCache** appCache
) {
    HK_TIMER_BEGIN("cacheD", nullptr);
    const uint32_t key = apAnimation->getFullCacheKey(auiChunkId);
    const uint32_t chunkSize = static_cast<uint32_t>(aiSize * apAnimation->m_blockSize * sizeof(float));
    (*appCache)->enterCriticalSection();
    if ((*appCache)->isKeyLockedForWrite(key, chunkSize)) {
        (*appCache)->leaveCriticalSection();
        HK_TIMER_END();
        return nullptr;
    }
    const uint8_t* pChunk = (*appCache)->retrieveChunk(key, chunkSize);
    if (pChunk) {
        (*appCache)->lockKeyForRead(key, chunkSize);
        (*appCache)->leaveCriticalSection();
    }
    else {
        uint8_t* pNewChunk = (*appCache)->allocateChunk(key, chunkSize);
        if (!pNewChunk) {
            (*appCache)->leaveCriticalSection();
            HK_TIMER_END();
            return nullptr;
        }
        (*appCache)->lockKeyForRead(key, chunkSize);
        (*appCache)->lockKeyForWrite(key, chunkSize);
        (*appCache)->leaveCriticalSection();
        decompressBlockToCache(auiChunkId, reinterpret_cast<float*>(pNewChunk), aiSize, apAnimation);
        (*appCache)->enterCriticalSection();
        (*appCache)->unlockKeyForWrite(key, chunkSize);
        (*appCache)->leaveCriticalSection();
        pChunk = pNewChunk;
    }
    HK_TIMER_END();
    return pChunk;
}

void unlockCachedChunk(
    int32_t aiSize,
    const hkaDeltaCompressedAnimation* apAnimation,
    uint32_t auiChunkId,
    hkaChunkCache** appCache
) {
    const uint32_t uiKey = apAnimation->getFullCacheKey(auiChunkId);
    const uint32_t uiChunkSize = 4 * (aiSize * apAnimation->m_blockSize);
    (*appCache)->enterCriticalSection();
    (*appCache)->unlockKeyForRead(uiKey, uiChunkSize);
    (*appCache)->leaveCriticalSection();
}

float* hkInverseDeltaTransform(float* data, uint32_t nValues) {
    uint32_t v5;
    double v6;
    int32_t v7;
    double v8;
    float v10;
    float v11;
    float v12;
    float v9 = *data;
    float* result = data + 1;
    uint32_t v4 = 1;
    if (static_cast<int32_t>(nValues - 1) >= 4) {
        v5 = ((nValues - 5) >> 2) + 1;
        v4 = (4 * v5) + 1;
        do {
            v6 = *result;
            result += 4;
            --v5;
            v10 = static_cast<float>(v6 + v9);
            *(result - 4) = v10;
            v11 = v10 + (*(result - 3));
            *(result - 3) = v11;
            v12 = v11 + (*(result - 2));
            *(result - 2) = v12;
            v9 = v12 + (*(result - 1));
            *(result - 1) = v9;
        }
        while (v5);
    }
    if (v4 < nValues) {
        v7 = static_cast<int32_t>(nValues - v4);
        do {
            v8 = *(result++);
            --v7;
            v9 = static_cast<float>(v8 + v9);
            *(result - 1) = v9;
        }
        while (v7);
    }
    return result;
}

// The block's rawData buffer is sized for maxBitWidth (hkCalcQuantizedSize reads only m_bitWidth and m_preserved of the
// desc).
void _decompress(hkWaveletDecompressionParameters params, float* out) {
    hkaQuantizeDesc quantDesc;
    hkaBlockDesc blockDesc;
    quantDesc.m_bitWidth = params.m_maxBitWidth;
    quantDesc.m_preserved = params.m_preserved;
    blockDesc.m_preserved = params.m_preserved;
    const int32_t maxSize = hkCalcQuantizedSize(params.m_blockSize, quantDesc);
    hkLocalBuffer<uint8_t> rawDataBuffer(maxSize, nullptr);
    uint8_t* rawData = rawDataBuffer.begin();

    const uint8_t* encodedData = params.m_encodedData;
    for (uint32_t i = 0; i < params.m_numD; ++i) {
        quantDesc.m_offset = params.m_offset[i];
        quantDesc.m_scale = params.m_scale[i];
        quantDesc.m_bitWidth = params.m_bitWidth ? params.m_bitWidth[i] : params.m_maxBitWidth;
        const int32_t rawDataSizeBytes = hkCalcQuantizedSize(params.m_blockSize, quantDesc);
        blockDesc.m_bitWidth = quantDesc.m_bitWidth;
        // The quantized value of 0, the run symbol; it is truncated to a 64-bit integer and the low 32 bits are kept.
        const int32_t numSymbols = 1 << quantDesc.m_bitWidth;
        blockDesc.m_runSymbol = static_cast<uint32_t>(
            static_cast<int64_t>(-quantDesc.m_offset * numSymbols / quantDesc.m_scale)
        );
        if (blockDesc.m_runSymbol == static_cast<uint32_t>(numSymbols)) {
            blockDesc.m_runSymbol = blockDesc.m_runSymbol - 1;
        }
        encodedData += hkBlockDecode(encodedData, blockDesc, rawData, rawDataSizeBytes);
        hkExpandReal(rawData, quantDesc, out, params.m_blockSize);
        hkInverseWaveletTransform(out, params.m_blockSize);
        out += params.m_blockSize;
    }
}

// The block's offsets, scales and bit widths are arrays in the animation's data buffer, raw bytes read untyped at the
// QuantizationFormat's indices (hence the casts); m_blockIndexIdx holds each block's offset into the quantized data.
// KEEP: the whole params local is passed by value to _decompress, and the padding after m_maxBitWidth is never
// written (only m_preserved and m_maxBitWidth are stored): bytes nothing wrote, passed to a callee.
void decompressBlockToCache(
    uint32_t auiChunkId,
    float* out,
    uint32_t numD,
    const hkaWaveletCompressedAnimation* apAnimation
) {
    HK_TIMER_BEGIN("decompressBlockCacheW", nullptr);

    const uint8_t* data = apAnimation->m_dataBuffer.m_data;
    const hkaWaveletCompressedAnimation::QuantizationFormat& qFormat = apAnimation->m_qFormat;
    const uint32_t* blockIndex = reinterpret_cast<const uint32_t*>(data + apAnimation->m_blockIndexIdx);
    hkWaveletDecompressionParameters params;
    params.m_numD = numD;
    params.m_blockSize = apAnimation->m_blockSize;
    params.m_preserved = qFormat.m_preserved;
    params.m_maxBitWidth = qFormat.m_maxBitWidth;
    params.m_offset = reinterpret_cast<const float*>(data + qFormat.m_offsetIdx);
    params.m_scale = reinterpret_cast<const float*>(data + qFormat.m_scaleIdx);
    params.m_bitWidth = data + qFormat.m_bitWidthIdx;
    params.m_encodedData = data + apAnimation->m_quantizedDataIdx +
        blockIndex[auiChunkId / static_cast<uint32_t>(params.m_blockSize)];
    _decompress(params, out);

    HK_TIMER_END();
}
