#include "Havok/Source/Animation/Animation/Animation/WaveletCompressed/hkaWaveletCompressedAnimation.h"

#include "Havok/Source/Animation/Animation/Animation/DeltaCompressed/hkaDeltaCompressedAnimation.h"
#include "Havok/Source/Animation/Animation/Animation/Util/hkaTrackAnalysis.h"
#include "Havok/Source/Animation/Animation/Animation/hkaAnimation.h"
#include "Havok/Source/Animation/Animation/Playback/Cache/hkaChunkCache.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

#include <bit>
#include <xmmintrin.h>

static void cleanupLoadedObjecthkaWaveletCompressedAnimationQuantizationFormat(void* p);
static void cleanupLoadedObjecthkaWaveletCompressedAnimationCompressionParams(void* p);
static void finishLoadedObjecthkaWaveletCompressedAnimationQuantizationFormat(void* p, int32_t finishing);
static void finishLoadedObjecthkaWaveletCompressedAnimation(void* p, int32_t finishing);
// Havok SDK signature (hkaAnimation.lib, tied by reloc,target).
static const void* getVtablehkaWaveletCompressedAnimation();
static void cleanupLoadedObjecthkaWaveletCompressedAnimation(void* p);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkaWaveletCompressedAnimationCompressionParamsTypeInfo = {
    "hkaWaveletCompressedAnimationCompressionParams",
    nullptr,
    cleanupLoadedObjecthkaWaveletCompressedAnimationCompressionParams,
    nullptr,
    &typeid(hkaWaveletCompressedAnimation::CompressionParams),
    sizeof(hkaWaveletCompressedAnimation::CompressionParams)
};
hkTypeInfo hkaWaveletCompressedAnimationQuantizationFormatTypeInfo = {
    "hkaWaveletCompressedAnimationQuantizationFormat",
    finishLoadedObjecthkaWaveletCompressedAnimationQuantizationFormat,
    cleanupLoadedObjecthkaWaveletCompressedAnimationQuantizationFormat,
    nullptr,
    &typeid(hkaWaveletCompressedAnimation::QuantizationFormat),
    sizeof(hkaWaveletCompressedAnimation::QuantizationFormat)
};

// The reflected members of hkaWaveletCompressedAnimation::CompressionParams, ::QuantizationFormat and of
// hkaWaveletCompressedAnimation.
using hkaWaveletCompressedAnimation_CompressionParamsClass_MembersArray = const hkClassMember[10];
static hkaWaveletCompressedAnimation_CompressionParamsClass_MembersArray
    hkaWaveletCompressedAnimation_CompressionParamsClass_Members{
        {"quantizationBits", nullptr, nullptr, hkClassMember::TYPE_UINT16, hkClassMember::TYPE_VOID, 0, 0, 0x0,
         nullptr},
        {"blockSize", nullptr, nullptr, hkClassMember::TYPE_UINT16, hkClassMember::TYPE_VOID, 0, 0, 0x2, nullptr},
        {"preserve", nullptr, nullptr, hkClassMember::TYPE_UINT16, hkClassMember::TYPE_VOID, 0, 0, 0x4, nullptr},
        {"truncProp", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x8, nullptr},
        {"useOldStyleTruncation", nullptr, nullptr, hkClassMember::TYPE_BOOL, hkClassMember::TYPE_VOID, 0, 0, 0xC,
         nullptr},
        {"absolutePositionTolerance", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x10,
         nullptr},
        {"relativePositionTolerance", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x14,
         nullptr},
        {"rotationTolerance", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x18,
         nullptr},
        {"scaleTolerance", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x1C, nullptr},
        {"absoluteFloatTolerance", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x20,
         nullptr}
};
using hkaWaveletCompressedAnimation_QuantizationFormatClass_MembersArray = const hkClassMember[6];
static hkaWaveletCompressedAnimation_QuantizationFormatClass_MembersArray
    hkaWaveletCompressedAnimation_QuantizationFormatClass_Members{
        {"maxBitWidth", nullptr, nullptr, hkClassMember::TYPE_UINT8, hkClassMember::TYPE_VOID, 0, 0, 0x0, nullptr},
        {"preserved", nullptr, nullptr, hkClassMember::TYPE_UINT8, hkClassMember::TYPE_VOID, 0, 0, 0x1, nullptr},
        {"numD", nullptr, nullptr, hkClassMember::TYPE_UINT32, hkClassMember::TYPE_VOID, 0, 0, 0x4, nullptr},
        {"offsetIdx", nullptr, nullptr, hkClassMember::TYPE_UINT32, hkClassMember::TYPE_VOID, 0, 0, 0x8, nullptr},
        {"scaleIdx", nullptr, nullptr, hkClassMember::TYPE_UINT32, hkClassMember::TYPE_VOID, 0, 0, 0xC, nullptr},
        {"bitWidthIdx", nullptr, nullptr, hkClassMember::TYPE_UINT32, hkClassMember::TYPE_VOID, 0, 0, 0x10, nullptr}
};
using hkaWaveletCompressedAnimationClass_MembersArray = const hkClassMember[12];
static hkaWaveletCompressedAnimationClass_MembersArray hkaWaveletCompressedAnimationClass_Members{
    {"numberOfPoses", nullptr, nullptr, hkClassMember::TYPE_INT32, hkClassMember::TYPE_VOID, 0, 0, 0x28, nullptr},
    {"blockSize", nullptr, nullptr, hkClassMember::TYPE_INT32, hkClassMember::TYPE_VOID, 0, 0, 0x2C, nullptr},
    {"qFormat", &hkaWaveletCompressedAnimationQuantizationFormatClass, nullptr, hkClassMember::TYPE_STRUCT,
     hkClassMember::TYPE_VOID, 0, 0, 0x30, nullptr},
    {"staticMaskIdx", nullptr, nullptr, hkClassMember::TYPE_UINT32, hkClassMember::TYPE_VOID, 0, 0, 0x44, nullptr},
    {"staticDOFsIdx", nullptr, nullptr, hkClassMember::TYPE_UINT32, hkClassMember::TYPE_VOID, 0, 0, 0x48, nullptr},
    {"numStaticTransformDOFs", nullptr, nullptr, hkClassMember::TYPE_UINT32, hkClassMember::TYPE_VOID, 0, 0, 0x4C,
     nullptr},
    {"numDynamicTransformDOFs", nullptr, nullptr, hkClassMember::TYPE_UINT32, hkClassMember::TYPE_VOID, 0, 0, 0x50,
     nullptr},
    {"blockIndexIdx", nullptr, nullptr, hkClassMember::TYPE_UINT32, hkClassMember::TYPE_VOID, 0, 0, 0x54, nullptr},
    {"blockIndexSize", nullptr, nullptr, hkClassMember::TYPE_UINT32, hkClassMember::TYPE_VOID, 0, 0, 0x58, nullptr},
    {"quantizedDataIdx", nullptr, nullptr, hkClassMember::TYPE_UINT32, hkClassMember::TYPE_VOID, 0, 0, 0x5C, nullptr},
    {"quantizedDataSize", nullptr, nullptr, hkClassMember::TYPE_UINT32, hkClassMember::TYPE_VOID, 0, 0, 0x60, nullptr},
    {"dataBuffer", nullptr, nullptr, hkClassMember::TYPE_ARRAY, hkClassMember::TYPE_UINT8, 0, 0, 0x64, nullptr}
};
hkClass hkaWaveletCompressedAnimationCompressionParamsClass(
    "hkaWaveletCompressedAnimationCompressionParams",
    nullptr,
    36,
    nullptr,
    0,
    nullptr,
    0,
    hkaWaveletCompressedAnimation_CompressionParamsClass_Members,
    10,
    nullptr,
    nullptr,
    0,
    0
);
hkClass hkaWaveletCompressedAnimationQuantizationFormatClass(
    "hkaWaveletCompressedAnimationQuantizationFormat",
    nullptr,
    20,
    nullptr,
    0,
    nullptr,
    0,
    hkaWaveletCompressedAnimation_QuantizationFormatClass_Members,
    6,
    nullptr,
    nullptr,
    0,
    0
);
hkClass hkaWaveletCompressedAnimationClass(
    "hkaWaveletCompressedAnimation",
    &hkaAnimationClass,
    112,
    nullptr,
    0,
    nullptr,
    0,
    hkaWaveletCompressedAnimationClass_Members,
    12,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkaWaveletCompressedAnimationTypeInfo = {
    "hkaWaveletCompressedAnimation",
    finishLoadedObjecthkaWaveletCompressedAnimation,
    cleanupLoadedObjecthkaWaveletCompressedAnimation,
    getVtablehkaWaveletCompressedAnimation(),
    &typeid(hkaWaveletCompressedAnimation),
    sizeof(hkaWaveletCompressedAnimation)
};

static void cleanupLoadedObjecthkaWaveletCompressedAnimationQuantizationFormat(void* p) {
    ;
}

static void cleanupLoadedObjecthkaWaveletCompressedAnimationCompressionParams(void* p) {
    ;
}

static void finishLoadedObjecthkaWaveletCompressedAnimationQuantizationFormat(void* p, int32_t finishing) {
    ;
}

static const void* getVtablehkaWaveletCompressedAnimation() {
    return hkGetVtableOf<hkaWaveletCompressedAnimation>();
}

static void cleanupLoadedObjecthkaWaveletCompressedAnimation(void* p) {
    static_cast<hkaWaveletCompressedAnimation*>(p)->~hkaWaveletCompressedAnimation();
}

// The type info's finish-loaded-object callback: the null check, then the animation's finish constructor.
static void finishLoadedObjecthkaWaveletCompressedAnimation(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag kFlag;
        kFlag.m_finishing = finishing;
        ::new (p) hkaWaveletCompressedAnimation(kFlag);
    }
}

hkaWaveletCompressedAnimation::CompressionParams::CompressionParams()
  : m_quantizationBits(8), m_blockSize(0xFFFF), m_preserve(0), m_truncProp(0.1f), m_useOldStyleTruncation(false),
    m_absolutePositionTolerance(0.0f), m_relativePositionTolerance(0.01f), m_rotationTolerance(0.001f),
    m_scaleTolerance(0.01f), m_absoluteFloatTolerance(0.001f) {}

const hkClass& hkaWaveletCompressedAnimation::CompressionParams::staticClass() {
    return hkaWaveletCompressedAnimationCompressionParamsClass;
}

const hkClass& hkaWaveletCompressedAnimation::QuantizationFormat::staticClass() {
    return hkaWaveletCompressedAnimationQuantizationFormatClass;
}

// The data buffer goes with its member.
hkaWaveletCompressedAnimation::~hkaWaveletCompressedAnimation() {}

void hkaWaveletCompressedAnimation::sampleTracks(
    float time,
    hkQsTransform* transformTracksOut,
    float* floatTracksOut,
    hkaChunkCache* cache
) const {
    samplePartialTracks(
        time, m_numberOfTransformTracks, transformTracksOut, m_numberOfFloatTracks, floatTracksOut, cache
    );
}

// The tracks at a time between two frames: the dynamic degrees of freedom of both frames' blocks (from the cache or
// decompressed into a stack buffer) are blended into a second stack buffer, which recompose merges with the static
// ones. Asking for fewer tracks than the animation has decompresses only the dynamic degrees of freedom they need (all
// transform tracks' when float tracks are asked for). The cache's chunks hold m_qFormat.m_numD channels each, so the
// cache helpers take that count, not the reduced one.
void hkaWaveletCompressedAnimation::samplePartialTracks(
    float time,
    uint32_t maxNumTransformTracks,
    hkQsTransform* transformTracksOut,
    uint32_t maxNumFloatTracks,
    float* floatTracksOut,
    hkaChunkCache* cache
) const {
    HK_TIMER_BEGIN_LIST("SampleWave", "StaticW");

    // The static mask is untyped bytes of m_dataBuffer, read through a cast as in the SDK.
    const uint16_t* staticMask = reinterpret_cast<const uint16_t*>(m_dataBuffer.m_data + m_staticMaskIdx);
    const uint32_t cacheNumD = m_qFormat.m_numD;
    uint32_t numD = cacheNumD;
    if (maxNumTransformTracks != static_cast<uint32_t>(m_numberOfTransformTracks) ||
        maxNumFloatTracks != static_cast<uint32_t>(m_numberOfFloatTracks)) {
        // EXE-INLINED none: hkaTrackAnalysis::AnalysisStats::AnalysisStats
        hkaTrackAnalysis::AnalysisStats stats = {};
        if (maxNumFloatTracks) {
            hkaTrackAnalysis::calcDynamicStats(
                staticMask, staticMask + m_numberOfTransformTracks, m_numberOfTransformTracks, maxNumFloatTracks, stats
            );
            numD = stats.m_dynamicPosDOF + stats.m_dynamicRotDOF + stats.m_dynamicSclDOF + stats.m_dynamicFloatDOF;
        }
        else {
            hkaTrackAnalysis::calcDynamicStats(
                staticMask, staticMask + m_numberOfTransformTracks, maxNumTransformTracks, 0, stats
            );
            numD = stats.m_dynamicPosDOF + stats.m_dynamicRotDOF + stats.m_dynamicSclDOF;
        }
    }

    HK_TIMER_SPLIT_LIST("DecompressW");
    uint32_t frame;
    float delta;
    getFrameAndDelta(time, frame, delta);
    const float oneMinusDelta = 1.0f - delta;

    // The stack allocator hands out raw memory.
    // EXE-INLINED none: hkAllocateStack<float>
    hkMemoryRouter* router = &hkMemoryRouter::getInstance();
    float* decompressed = reinterpret_cast<float*>(router->m_stack.m_next.m_storage);
    router->m_stack.m_next.m_storage += (numD * static_cast<uint32_t>(m_blockSize) * 4 + 127) & ~127u;
    // EXE-INLINED none: hkAllocateStack<float>
    router = &hkMemoryRouter::getInstance();
    float* interpolated = reinterpret_cast<float*>(router->m_stack.m_next.m_storage);
    router->m_stack.m_next.m_storage += (numD * 4 + 127) & ~127u;

    // The cache's chunks are untyped bytes.
    const float* data = nullptr;
    if (cache) {
        data = reinterpret_cast<const float*>(retrieveFromCache(this, frame, static_cast<int32_t>(cacheNumD), &cache));
    }
    if (!data) {
        decompressBlockToCache(frame, decompressed, numD, this);
        data = decompressed;
    }

    HK_TIMER_SPLIT_LIST("InterpW");
    uint32_t idx = frame % static_cast<uint32_t>(m_blockSize);
    if (idx + 1 != static_cast<uint32_t>(m_blockSize)) {
        // Both frames are in this block.
        for (uint32_t i = 0; i < numD; i++) {
            interpolated[i] = data[idx + 1] * delta + data[idx] * oneMinusDelta;
            idx += m_blockSize;
        }
        if (data != decompressed) {
            releaseFromCache(static_cast<int32_t>(cacheNumD), this, frame, &cache);
        }
    }
    else {
        // The next frame is the first of the next block.
        for (uint32_t i = 0; i < numD; i++) {
            interpolated[i] = data[idx] * oneMinusDelta;
            idx += m_blockSize;
        }
        if (data != decompressed) {
            releaseFromCache(static_cast<int32_t>(cacheNumD), this, frame, &cache);
        }

        HK_TIMER_SPLIT_LIST("DecompressW");
        frame++;
        const float* nextData = nullptr;
        if (cache) {
            nextData =
                reinterpret_cast<const float*>(retrieveFromCache(this, frame, static_cast<int32_t>(cacheNumD), &cache));
        }
        if (!nextData) {
            decompressBlockToCache(frame, decompressed, numD, this);
            nextData = decompressed;
        }

        HK_TIMER_SPLIT_LIST("InterpW");
        uint32_t nextIdx = 0;
        for (uint32_t i = 0; i < numD; i++) {
            interpolated[i] += nextData[nextIdx] * delta;
            nextIdx += m_blockSize;
        }
        if (nextData != decompressed) {
            releaseFromCache(static_cast<int32_t>(cacheNumD), this, frame, &cache);
        }
    }

    HK_TIMER_SPLIT_LIST("RecomposeW");
    // The static degrees of freedom are untyped bytes of m_dataBuffer, read through a cast as in the SDK.
    const float* staticDOFs = reinterpret_cast<const float*>(m_dataBuffer.m_data + m_staticDOFsIdx);
    hkaTrackAnalysis::AnalysisCompose compose;
    compose.m_numberOfTransformTracks = maxNumTransformTracks;
    compose.m_numberOfFloatTracks = maxNumFloatTracks;
    compose.m_staticMaskTransform = staticMask;
    compose.m_staticMaskFloat = staticMask + m_numberOfTransformTracks;
    compose.m_staticDOFsTransform = staticDOFs;
    compose.m_staticDOFsFloat = staticDOFs + m_numStaticTransformDOFs;
    compose.m_dynamicDOFsTransform = interpolated;
    compose.m_dynamicDOFsFloat = interpolated + m_numDynamicTransformDOFs;
    hkaTrackAnalysis::recompose(compose, transformTracksOut, floatTracksOut);
    HK_TIMER_END_LIST();

    // The stack allocator's next pointer is raw memory.
    // EXE-INLINED none: hkDeallocateStack<float>
    hkMemoryRouter::getInstance().m_stack.m_next.m_storage = reinterpret_cast<char*>(interpolated);
    // EXE-INLINED none: hkDeallocateStack<float>
    hkMemoryRouter::getInstance().m_stack.m_next.m_storage = reinterpret_cast<char*>(decompressed);
}

// Flushes every block's key from the cache. A key is getFullCacheKey's: the data's address as a 32-bit number plus
// the block number.
void hkaWaveletCompressedAnimation::clearAllCacheKeys(hkaChunkCache* cache) {
    const int32_t blockSize = m_blockSize;
    const int32_t numBlocks = (getNumOriginalFrames() - 1) / blockSize + 1;
    const uint32_t chunkSize = static_cast<uint32_t>(blockSize * m_qFormat.m_numD * sizeof(float));
    for (int32_t i = 0; i < numBlocks; ++i) {
        cache->flushKey(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(m_dataBuffer.m_data)) + i, chunkSize);
    }
}

void hkaWaveletCompressedAnimation::sampleIndividualTransformTracks(
    float time,
    const int16_t* tracks,
    uint32_t numTracks,
    hkQsTransform* transformTracksOut
) const {
    uint32_t v5 = numTracks;
    if (numTracks) {
        hkVector4 v6;
        v6.m_quad = _mm_shuffle_ps(
            _mm_set_ss(std::bit_cast<float>(0x3F800000u)), _mm_set_ss(std::bit_cast<float>(0x3F800000u)), 0
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

// Fills the float tracks with zeroes, and only when the count is nonzero; the time and the track list are not read.
void hkaWaveletCompressedAnimation::sampleIndividualFloatTracks(
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

int32_t hkaWaveletCompressedAnimation::getNumOriginalFrames() const {
    return (int32_t)((uint32_t)m_numberOfPoses);
}

int32_t hkaWaveletCompressedAnimation::getNumDataChunks(uint32_t frame, float delta) const {
    return 4 - ((uint8_t)(((uint32_t)m_blockSize) != ((frame % ((uint32_t)m_blockSize)) + 1)));
}

// The chunks a frame's sampling reads: the animation itself and its static data up to the quantized data (both
// starts kept as they are, with their low four bits as the offset), the frame's block and, when four are asked for,
// the next block (wrapping to the first).
void hkaWaveletCompressedAnimation::getDataChunks(
    uint32_t frame,
    float delta,
    hkaAnimation::DataChunk* dataChunks,
    int32_t numDataChunks
) const {
    dataChunks[0].m_offset = static_cast<uint8_t>(reinterpret_cast<uintptr_t>(this) & 0xF);
    dataChunks[0].m_data = this;
    dataChunks[0].m_size = sizeof(hkaWaveletCompressedAnimation);
    const uint8_t* data = m_dataBuffer.m_data;
    dataChunks[1].m_data = data;
    dataChunks[1].m_size = (m_quantizedDataIdx + 15) & ~15u;
    dataChunks[1].m_offset = static_cast<uint8_t>(reinterpret_cast<uintptr_t>(data) & 0xF);
    // div: frame and the block size are divided unsigned.
    const uint32_t blockNum = frame / m_blockSize;
    getBlockDataAndSize(static_cast<int32_t>(blockNum), dataChunks[2]);
    if (numDataChunks == 4) {
        getBlockDataAndSize(static_cast<int32_t>((blockNum + 1) % m_blockIndexSize), dataChunks[3]);
    }
}

// The largest span two consecutive blocks take with the animation and its static data: the animation, the static data
// up to the quantized data rounded up to 16 bytes, the block and the next one (the last block alone). The chunk is
// zeroed before each block, as the game does.
int32_t hkaWaveletCompressedAnimation::getMaxSizeOfCombinedDataChunks() const {
    int32_t maxSize = 0;
    const uint32_t staticSize = (m_quantizedDataIdx + 15) & ~15u;
    const uint32_t lastBlock = m_blockIndexSize - 1;
    for (uint32_t blockNum = 0; blockNum < m_blockIndexSize; ++blockNum) {
        hkaAnimation::DataChunk chunk;
        chunk.init();
        getBlockDataAndSize(static_cast<int32_t>(blockNum), chunk);
        int32_t size = static_cast<int32_t>(staticSize + chunk.m_size + sizeof(hkaWaveletCompressedAnimation));
        if (blockNum < lastBlock) {
            getBlockDataAndSize(static_cast<int32_t>(blockNum + 1), chunk);
            size += chunk.m_size;
        }
        if (size > maxSize) {
            maxSize = size;
        }
    }
    return maxSize;
}

// The cache key is the address of the animation's data, used as a number, plus the block number, truncated to hkUint32.
uint32_t hkaWaveletCompressedAnimation::getFullCacheKey(uint32_t poseIdx) const {
    return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(m_dataBuffer.m_data)) + poseIdx / m_blockSize;
}

// The block index (one 32-bit value per block: its start in the quantized data) and the quantized data are untyped
// bytes of m_dataBuffer, read through casts. The last block runs to the end of the quantized data.
void hkaWaveletCompressedAnimation::getBlockDataAndSize(int32_t blockNum, hkaAnimation::DataChunk& dataChunkOut) const {
    const uint32_t* blockIndex = reinterpret_cast<const uint32_t*>(m_dataBuffer.m_data + m_blockIndexIdx);
    const uint8_t* blockData = m_dataBuffer.m_data + m_quantizedDataIdx + blockIndex[blockNum];
    int32_t numBytes;
    if (static_cast<uint32_t>(blockNum) == m_blockIndexSize - 1) {
        numBytes = static_cast<int32_t>(m_quantizedDataSize - blockIndex[blockNum]);
    }
    else {
        numBytes = static_cast<int32_t>(blockIndex[blockNum + 1] - blockIndex[blockNum]);
    }
    dataChunkOut.setDataChunkWith16ByteAlignmentCorrection(blockData, numBytes);
}

// Swaps the byte order of a big-endian animation's data in place: each word of the static mask, each static degree of
// freedom, each dynamic degree of freedom's offset and scale (in turn) and each 32-bit entry of the block index. The
// data need it when their first byte is nonzero, in the layout whose static mask starts at 4. The arrays are untyped
// bytes of m_dataBuffer, read through casts.
void hkaWaveletCompressedAnimation::handleEndian() {
    if (m_staticMaskIdx != 4) {
        return;
    }
    uint8_t* data = m_dataBuffer.m_data;
    if (data[0] == 0) {
        return;
    }
    uint16_t* staticMask = reinterpret_cast<uint16_t*>(data + 4);
    for (int32_t i = 0; i < m_numberOfTransformTracks + m_numberOfFloatTracks; ++i) {
        staticMask[i] = __builtin_bswap16(staticMask[i]);
    }

    // The SDK's AnalysisStats constructor zeroes every count and the ratio.
    hkaTrackAnalysis::AnalysisStats stats = {};
    hkaTrackAnalysis::calcStats(
        staticMask, staticMask + m_numberOfTransformTracks, static_cast<uint32_t>(m_numberOfTransformTracks),
        static_cast<uint32_t>(m_numberOfFloatTracks), stats
    );

    float* staticDOFs = reinterpret_cast<float*>(m_dataBuffer.m_data + m_staticDOFsIdx);
    const int32_t numStatic = static_cast<int32_t>(
        stats.m_staticPosDOF + stats.m_staticRotDOF + stats.m_staticSclDOF + stats.m_staticFloatDOF
    );
    for (int32_t i = 0; i < numStatic; ++i) {
        union {
            float r;
            uint8_t c[4];
        } in, out;
        in.r = staticDOFs[i];
        out.c[0] = in.c[3];
        out.c[1] = in.c[2];
        out.c[2] = in.c[1];
        out.c[3] = in.c[0];
        staticDOFs[i] = out.r;
    }

    uint8_t* dynamicData = m_dataBuffer.m_data;
    const int32_t numDynamic = static_cast<int32_t>(
        stats.m_dynamicPosDOF + stats.m_dynamicRotDOF + stats.m_dynamicSclDOF + stats.m_dynamicFloatDOF
    );
    float* offsets = reinterpret_cast<float*>(dynamicData + m_qFormat.m_offsetIdx);
    float* scales = reinterpret_cast<float*>(dynamicData + m_qFormat.m_scaleIdx);
    // The count is compared unsigned.
    for (uint32_t i = 0; i < static_cast<uint32_t>(numDynamic); ++i) {
        union {
            float r;
            uint8_t c[4];
        } in, out;
        in.r = offsets[i];
        out.c[0] = in.c[3];
        out.c[1] = in.c[2];
        out.c[2] = in.c[1];
        out.c[3] = in.c[0];
        offsets[i] = out.r;
        in.r = scales[i];
        out.c[0] = in.c[3];
        out.c[1] = in.c[2];
        out.c[2] = in.c[1];
        out.c[3] = in.c[0];
        scales[i] = out.r;
    }

    uint32_t* blockIndex = reinterpret_cast<uint32_t*>(m_dataBuffer.m_data + m_blockIndexIdx);
    for (uint32_t i = 0; i < m_blockIndexSize; ++i) {
        blockIndex[i] = __builtin_bswap32(blockIndex[i]);
    }
}

const hkClass& hkaWaveletCompressedAnimation::staticClass() {
    return hkaWaveletCompressedAnimationClass;
}

// Samples the dynamic degrees of freedom of the block chunk 2 holds at frameIndex + delta: the block is decompressed
// into a Havok-stack buffer (hkLocalBuffer), the frame and the next one blended into a second buffer (with 4 chunks the
// next frame is the first of the next block, chunk 3, decompressed into the same buffer), then recomposed with the
// static ones. With fewer tracks than the animation has, only the requested tracks' dynamic DOFs are decompressed.
// KEEP: the exe copies the whole 0x1C-byte params local onto the stack for _decompress (rep movsd, 7 dwords, 0xE303E6
// and 0xE30627) and never writes its bytes 0xA-0xB (padding after m_maxBitWidth; only +8 and +9 are stored at
// 0xE303A0 and 0xE303C1): bytes nothing wrote, pushed to a callee.
void hkaWaveletCompressedAnimation::samplePartialWithDataChunks(
    uint32_t frameIndex,
    float delta,
    uint32_t maxNumTransformTracks,
    hkQsTransform* transformTracksOut,
    uint32_t maxNumFloatTracks,
    float* floatTracksOut,
    const hkaAnimation::DataChunk* dataChunks,
    int32_t numDataChunks
) {
    // The chunks are raw memory read untyped (chunk 0: the animation object's bytes, chunk 1: its data buffer, read at
    // the animation's byte indices, 2 and 3: blocks).
    const hkaWaveletCompressedAnimation* anim = reinterpret_cast<const hkaWaveletCompressedAnimation*>(
        static_cast<const uint8_t*>(dataChunks[0].m_data) + dataChunks[0].m_offset
    );
    HK_TIMER_BEGIN_LIST("SampleWaveChunk", "StaticW");

    const uint8_t* data = static_cast<const uint8_t*>(dataChunks[1].m_data) + dataChunks[1].m_offset;
    const float* staticDOFs = reinterpret_cast<const float*>(data + anim->m_staticDOFsIdx);
    const uint16_t* staticMask = reinterpret_cast<const uint16_t*>(data + anim->m_staticMaskIdx);
    uint32_t numD = anim->m_qFormat.m_numD;
    if (maxNumTransformTracks != static_cast<uint32_t>(anim->m_numberOfTransformTracks) ||
        maxNumFloatTracks != static_cast<uint32_t>(anim->m_numberOfFloatTracks)) {
        hkaTrackAnalysis::AnalysisStats stats;
        stats.m_ratio = 0.0f;
        stats.m_staticPosDOF = 0;
        stats.m_staticRotDOF = 0;
        stats.m_staticSclDOF = 0;
        stats.m_staticFloatDOF = 0;
        stats.m_clearPosDOF = 0;
        stats.m_clearRotDOF = 0;
        stats.m_clearSclDOF = 0;
        stats.m_dynamicPosDOF = 0;
        stats.m_dynamicRotDOF = 0;
        stats.m_dynamicSclDOF = 0;
        stats.m_dynamicFloatDOF = 0;
        stats.m_numTransformDOFS = 0;
        stats.m_numFloatDOFS = 0;
        const uint16_t* staticMaskF = staticMask + anim->m_numberOfTransformTracks;
        if (maxNumFloatTracks == 0) {
            hkaTrackAnalysis::calcDynamicStats(staticMask, staticMaskF, maxNumTransformTracks, 0, stats);
            numD = stats.m_dynamicRotDOF + stats.m_dynamicSclDOF;
        }
        else {
            hkaTrackAnalysis::calcDynamicStats(
                staticMask, staticMaskF, anim->m_numberOfTransformTracks, maxNumFloatTracks, stats
            );
            numD = stats.m_dynamicSclDOF + stats.m_dynamicFloatDOF + stats.m_dynamicRotDOF;
        }
        numD += stats.m_dynamicPosDOF;
    }

    HK_TIMER_SPLIT_LIST("DecompressWChunk");
    // 1 - delta is rounded to a float.
    float oneMinusDelta = 1.0f - delta;
    const int32_t numDecompressed = anim->m_blockSize * static_cast<int32_t>(numD);
    // EXE-INLINED none: hkLocalBuffer's stack allocation
    char* decompressedTop = hkMemoryRouter::getInstance().m_stack.m_next.m_storage;
    hkMemoryRouter::getInstance().m_stack.m_next.m_storage =
        decompressedTop + ((numDecompressed * static_cast<int32_t>(sizeof(float)) + 127) & ~127);
    // The Havok stack is raw, untyped memory.
    float* decompressed = reinterpret_cast<float*>(decompressedTop);
    // EXE-INLINED none: hkLocalBuffer's stack allocation
    char* resultTop = hkMemoryRouter::getInstance().m_stack.m_next.m_storage;
    hkMemoryRouter::getInstance().m_stack.m_next.m_storage =
        resultTop + ((static_cast<int32_t>(numD) * static_cast<int32_t>(sizeof(float)) + 127) & ~127);
    float* result = reinterpret_cast<float*>(resultTop);

    hkWaveletDecompressionParameters params;
    params.m_blockSize = anim->m_blockSize;
    params.m_preserved = anim->m_qFormat.m_preserved;
    params.m_offset = reinterpret_cast<const float*>(data + anim->m_qFormat.m_offsetIdx);
    params.m_scale = reinterpret_cast<const float*>(data + anim->m_qFormat.m_scaleIdx);
    params.m_encodedData = static_cast<const uint8_t*>(dataChunks[2].m_data) + dataChunks[2].m_offset;
    params.m_maxBitWidth = anim->m_qFormat.m_maxBitWidth;
    params.m_bitWidth = data + anim->m_qFormat.m_bitWidthIdx;
    params.m_numD = numD;
    _decompress(params, decompressed);

    // div: the frame and the block size are divided unsigned.
    uint32_t index = frameIndex % static_cast<uint32_t>(anim->m_blockSize);
    if (numDataChunks == 3) {
        const uint32_t blockSize = static_cast<uint32_t>(anim->m_blockSize);
        for (uint32_t i = 0; i < numD; ++i) {
            result[i] = static_cast<double>(decompressed[index + 1]) * delta +
                static_cast<double>(decompressed[index]) * oneMinusDelta;
            index += blockSize;
        }
    }
    else {
        const uint32_t blockSize = static_cast<uint32_t>(anim->m_blockSize);
        for (uint32_t i = 0; i < numD; ++i) {
            result[i] = static_cast<double>(decompressed[index]) * oneMinusDelta;
            index += blockSize;
        }
        params.m_encodedData = static_cast<const uint8_t*>(dataChunks[3].m_data) + dataChunks[3].m_offset;
        _decompress(params, decompressed);
        uint32_t next = 0;
        for (uint32_t i = 0; i < numD; ++i) {
            result[i] = static_cast<double>(decompressed[next]) * delta + result[i];
            next += static_cast<uint32_t>(anim->m_blockSize);
        }
    }

    HK_TIMER_SPLIT_LIST("RecomposeWChunk");
    hkaTrackAnalysis::AnalysisCompose input;
    input.m_numberOfTransformTracks = maxNumTransformTracks;
    input.m_numberOfFloatTracks = maxNumFloatTracks;
    input.m_staticMaskTransform = staticMask;
    input.m_staticDOFsTransform = staticDOFs;
    input.m_dynamicDOFsTransform = result;
    input.m_staticDOFsFloat = staticDOFs + anim->m_numStaticTransformDOFs;
    input.m_dynamicDOFsFloat = result + anim->m_numDynamicTransformDOFs;
    input.m_staticMaskFloat = staticMask + anim->m_numberOfTransformTracks;
    hkaTrackAnalysis::recompose(input, transformTracksOut, floatTracksOut);

    // EXE-INLINED none: hkLocalBuffer's stack release, the result buffer first
    hkMemoryRouter::getInstance().m_stack.m_next.m_storage = resultTop;
    hkMemoryRouter::getInstance().m_stack.m_next.m_storage = decompressedTop;
    HK_TIMER_END_LIST();
}

// The cache's chunks are untyped bytes: an allocated one is filled with the block's floats, hence the cast for
// decompressBlockToCache.
const uint8_t* retrieveFromCache(
    const hkaWaveletCompressedAnimation* apAnimation,
    uint32_t auiChunkId,
    int32_t aiSize,
    hkaChunkCache** appCache
) {
    HK_TIMER_BEGIN("cacheW", nullptr);

    const uint32_t key = apAnimation->getFullCacheKey(auiChunkId);
    const uint32_t chunkSize = static_cast<uint32_t>(4 * (aiSize * apAnimation->m_blockSize));
    (*appCache)->enterCriticalSection();
    if ((*appCache)->isKeyLockedForWrite(key, chunkSize)) {
        (*appCache)->leaveCriticalSection();
        HK_TIMER_END();
        return nullptr;
    }

    const uint8_t* chunk = (*appCache)->retrieveChunk(key, chunkSize);
    if (chunk) {
        (*appCache)->lockKeyForRead(key, chunkSize);
        (*appCache)->leaveCriticalSection();
    }
    else {
        uint8_t* newChunk = (*appCache)->allocateChunk(key, chunkSize);
        chunk = newChunk;
        if (!newChunk) {
            (*appCache)->leaveCriticalSection();
            HK_TIMER_END();
            return nullptr;
        }
        (*appCache)->lockKeyForRead(key, chunkSize);
        (*appCache)->lockKeyForWrite(key, chunkSize);
        (*appCache)->leaveCriticalSection();
        decompressBlockToCache(
            auiChunkId, reinterpret_cast<float*>(newChunk), static_cast<uint32_t>(aiSize), apAnimation
        );
        (*appCache)->enterCriticalSection();
        (*appCache)->unlockKeyForWrite(key, chunkSize);
        (*appCache)->leaveCriticalSection();
    }

    HK_TIMER_END();
    return chunk;
}

void releaseFromCache(
    int32_t aiSize,
    const hkaWaveletCompressedAnimation* apAnimation,
    uint32_t auiChunkId,
    hkaChunkCache** appCache
) {
    const uint32_t key = apAnimation->getFullCacheKey(auiChunkId);
    const uint32_t chunkSize = static_cast<uint32_t>(4 * (aiSize * apAnimation->m_blockSize));
    (*appCache)->enterCriticalSection();
    (*appCache)->unlockKeyForRead(key, chunkSize);
    (*appCache)->leaveCriticalSection();
}

double byteSwapFloat(float a1) {
    float a1a;
    a1a = std::bit_cast<float>(
        (std::bit_cast<uint32_t>(a1a) & 0xFFFFFF00u) |
        (static_cast<uint32_t>(static_cast<uint8_t>(std::bit_cast<uint32_t>(a1) >> 24)))
    );
    a1a = std::bit_cast<float>(
        (std::bit_cast<uint32_t>(a1a) & 0xFFFF00FFu) |
        (static_cast<uint32_t>(static_cast<uint8_t>(std::bit_cast<uint32_t>(a1) >> 16)) << 8)
    );
    a1a = std::bit_cast<float>(
        (std::bit_cast<uint32_t>(a1a) & 0xFF00FFFFu) |
        (static_cast<uint32_t>(static_cast<uint8_t>(std::bit_cast<uint32_t>(a1) >> 8)) << 16)
    );
    a1a = std::bit_cast<float>(
        (std::bit_cast<uint32_t>(a1a) & 0xFFFFFFu) |
        (static_cast<uint32_t>(static_cast<uint8_t>(std::bit_cast<uint32_t>(a1))) << 24)
    );
    return a1a;
}

int32_t byteSwap32(int32_t r_eax) {
    int32_t v2;
    v2 = static_cast<int32_t>(
        (static_cast<uint32_t>(v2) & 0xFFFFFF00u) | (static_cast<uint32_t>(static_cast<uint8_t>(r_eax >> 24)))
    );
    v2 = static_cast<int32_t>(
        (static_cast<uint32_t>(v2) & 0xFFFF00FFu) | (static_cast<uint32_t>(static_cast<uint8_t>(r_eax >> 16)) << 8)
    );
    v2 = static_cast<int32_t>(
        (static_cast<uint32_t>(v2) & 0xFF00FFFFu) | (static_cast<uint32_t>(static_cast<uint8_t>(r_eax >> 8)) << 16)
    );
    v2 = static_cast<int32_t>(
        (static_cast<uint32_t>(v2) & 0xFFFFFFu) | (static_cast<uint32_t>(static_cast<uint8_t>(r_eax)) << 24)
    );
    return v2;
}

int16_t byteSwap16(int32_t r_eax) {
    int16_t v2;
    v2 = static_cast<int16_t>(
        (static_cast<uint32_t>(v2) & 0xFFFFFF00u) | (static_cast<uint32_t>(static_cast<uint8_t>(r_eax >> 8)))
    );
    v2 = static_cast<int16_t>(
        (static_cast<uint32_t>(v2) & 0xFFFF00FFu) | (static_cast<uint32_t>(static_cast<uint8_t>(r_eax)) << 8)
    );
    return v2;
}
