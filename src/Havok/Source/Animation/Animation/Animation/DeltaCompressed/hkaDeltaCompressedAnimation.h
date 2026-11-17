#pragma once

#include "Havok/Source/Animation/Animation/Animation/hkaAnimation.h"

#include <stdint.h>

class hkaWaveletCompressedAnimation;
struct hkWaveletDecompressionParameters;

// An animation stored as quantized deltas in blocks of m_blockSize poses. Only the virtuals and the fields this code
// reads are declared.
class hkaDeltaCompressedAnimation : public hkaAnimation {
public:
	// The compression settings of the whole animation: the quantization bits, the poses per block and the tolerances.
	struct CompressionParams {
		CompressionParams();

		uint16_t m_quantizationBits;
		uint16_t m_blockSize;
		float m_absolutePositionTolerance;
		float m_relativePositionTolerance;
		float m_rotationTolerance;
		float m_scaleTolerance;
		float m_absoluteFloatTolerance;
	};

	// How the animation's values are quantized: the bit widths and the indices of the offsets, scales and bit widths in
	// the data.
	struct QuantizationFormat {
		// The reflection class hkaDeltaCompressedAnimationQuantizationFormatClass.
		static const hkClass& staticClass();

		uint8_t m_maxBitWidth;
		uint8_t m_preserved;
		uint32_t m_numD;
		uint32_t m_offsetIdx;
		uint32_t m_scaleIdx;
		uint32_t m_bitWidthIdx;
	};

	// Per-track compression: a palette of parameters and each transform and float track's index into it.
	struct PerTrackCompressionParams {
		hkArray<CompressionParams> m_parameterPalette;
		hkArray<int32_t> m_trackIndexToPaletteIndex;
		hkArray<int32_t> m_floatTrackIndexToPaletteIndex;
	};

	// The SDK's finish constructor: while finishing, swaps the loaded data's byte order where needed.
	hkaDeltaCompressedAnimation(hkFinishLoadedObjectFlag aFlag) : hkaAnimation(aFlag), m_dataBuffer(aFlag) {
		if (aFlag.m_finishing) {
			handleEndian();
		}
	}

	~hkaDeltaCompressedAnimation() override;

	void sampleTracks(float time, hkQsTransform* transformTracksOut, float* floatTracksOut,
		hkaChunkCache* cache) const override;
	void samplePartialTracks(float time, uint32_t maxNumTransformTracks, hkQsTransform* transformTracksOut,
		uint32_t maxNumFloatTracks, float* floatTracksOut, hkaChunkCache* cache) const override;
	void clearAllCacheKeys(hkaChunkCache* cache) override;
	void sampleIndividualTransformTracks(float time, const int16_t* tracks, uint32_t numTracks,
		hkQsTransform* transformTracksOut) const override;
	void sampleIndividualFloatTracks(float time, const int16_t* tracks, uint32_t numTracks,
		float* floatTracksOut) const override;
	int32_t getNumOriginalFrames() const override;
	int32_t getNumDataChunks(uint32_t frame, float delta) const override;
	void getDataChunks(uint32_t frame, float delta, hkaAnimation::DataChunk* dataChunks,
		int32_t numDataChunks) const override;
	int32_t getMaxSizeOfCombinedDataChunks() const override;
	virtual uint32_t getFullCacheKey(uint32_t poseIdx) const;

	// Swaps the byte order of the animation's data; runs only when m_staticMaskIdx holds 4 and the data start with a
	// nonzero byte. Called by the class's finish function.
	void handleEndian();
	// Havok SDK signature (hkaAnimation.lib, tied by bytes,reloc).
	void getBlockDataAndSize(int32_t blockNum, int32_t numBlocks, hkaAnimation::DataChunk& dataChunkOut) const;

	// Samples maxNumTransformTracks transform tracks and maxNumFloatTracks float tracks out of
	// the data chunks.
	static void samplePartialWithDataChunks(uint32_t frameIndex, float delta, uint32_t maxNumTransformTracks,
		hkQsTransform* transformTracksOut, uint32_t maxNumFloatTracks, float* floatTracksOut,
		const hkaAnimation::DataChunk* dataChunks, int32_t numDataChunks);
	// Havok SDK signature (hkaAnimation.lib, tied by target).
	static const hkClass& staticClass();

	int32_t m_numberOfPoses;
	int32_t m_blockSize;
	hkaDeltaCompressedAnimation::QuantizationFormat m_qFormat;
	uint32_t m_quantizedDataIdx;
	uint32_t m_quantizedDataSize;
	uint32_t m_staticMaskIdx;
	uint32_t m_staticMaskSize;
	uint32_t m_staticDOFsIdx;
	uint32_t m_staticDOFsSize;
	uint32_t m_numStaticTransformDOFs;
	uint32_t m_numDynamicTransformDOFs;
	uint32_t m_totalBlockSize;
	uint32_t m_lastBlockSize;
	hkArray<uint8_t> m_dataBuffer;
};

// What one block's decompression reads: the quantization of the block and pointers into the animation's data. It is
// passed by value to decompressBlock.
struct hkDeltaDecompressionParameters {
	uint32_t m_numD;
	int32_t m_blockSize;
	int32_t m_numValues;
	uint8_t m_preserved;
	uint8_t m_maxBitWidth;
	const float* m_offset;
	const float* m_scale;
	const uint8_t* m_bitWidth;
	const uint8_t* m_quantizedData;
};

// Decompresses one block into apOut; called by decompressBlockToCache and samplePartialWithDataChunks.
void decompressBlock(hkDeltaDecompressionParameters aParams, float* apOut);
// The cache's chunk for apAnimation's chunk auiChunkId, aiSize * m_blockSize floats, read-locked: retrieved, or
// allocated, write-locked and decompressed by decompressBlockToCache; null when it is write-locked elsewhere or
// allocation fails. Its own "TtcacheD" monitor timer. appCache is the caller's address of its cache argument.
const uint8_t* retrieveFromCache(const hkaDeltaCompressedAnimation* apAnimation, uint32_t auiChunkId, int32_t aiSize,
	hkaChunkCache** appCache);
// Releases the read lock retrieveFromCache took: enterCriticalSection, unlockKeyForRead(getFullCacheKey(auiChunkId),
// aiSize * m_blockSize * 4), then leaveCriticalSection.
void unlockCachedChunk(int32_t aiSize, const hkaDeltaCompressedAnimation* apAnimation, uint32_t auiChunkId,
	hkaChunkCache** appCache);
float* hkInverseDeltaTransform(float* a1, uint32_t a2);
// Decompresses one block, params.m_numD degrees of freedom of params.m_blockSize floats each, into out.
void _decompress(hkWaveletDecompressionParameters params, float* out);
// Decompresses into out the block of apAnimation that holds frame auiChunkId: numD degrees of freedom of m_blockSize
// floats each (_decompress), under its "decompressBlockCacheW" monitor timer. numD is stored into
// hkWaveletDecompressionParameters::m_numD.
void decompressBlockToCache(uint32_t auiChunkId, float* out, uint32_t numD,
	const hkaWaveletCompressedAnimation* apAnimation);

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkaDeltaCompressedAnimationQuantizationFormatTypeInfo;
