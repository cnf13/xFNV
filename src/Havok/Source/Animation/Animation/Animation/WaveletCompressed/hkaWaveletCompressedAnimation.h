#pragma once

#include "Havok/Source/Animation/Animation/Animation/hkaAnimation.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"

#include <stdint.h>

class hkQsTransform;

// An animation stored as wavelet-compressed blocks of m_blockSize poses. Only the virtuals and the fields this code
// reads are declared.
class hkaWaveletCompressedAnimation : public hkaAnimation {
public:
	struct CompressionParams {
		CompressionParams();

		static const hkClass& staticClass();

		uint16_t m_quantizationBits;
		uint16_t m_blockSize;
		uint16_t m_preserve;
		uint8_t padding06[2];
		float m_truncProp;
		hkBool m_useOldStyleTruncation;
		uint8_t padding0D[3];
		float m_absolutePositionTolerance;
		float m_relativePositionTolerance;
		float m_rotationTolerance;
		float m_scaleTolerance;
		float m_absoluteFloatTolerance;
	};

	// How a block's coefficients are quantized: the widest bit width, the preserved count and the indices of the
	// offsets, scales and bit widths in the static data.
	struct QuantizationFormat {
		static const hkClass& staticClass();

		uint8_t m_maxBitWidth;
		uint8_t m_preserved;
		uint8_t padding02[2];
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

	hkaWaveletCompressedAnimation(hkFinishLoadedObjectFlag aFlag) : hkaAnimation(aFlag), m_dataBuffer(aFlag) {
		if (aFlag.m_finishing) {
			handleEndian();
		}
	}

	~hkaWaveletCompressedAnimation() override;

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

	// Havok SDK signature (hkaAnimation.lib, tied by bytes,reloc).
	void getBlockDataAndSize(int32_t blockNum, hkaAnimation::DataChunk& dataChunkOut) const;
	// Swaps the byte order of the animation's data; runs only when m_staticMaskIdx holds 4 and the data start with a
	// nonzero byte. Called by the class's finish function.
	void handleEndian();

	static const hkClass& staticClass();
	static void samplePartialWithDataChunks(uint32_t frameIndex, float delta, uint32_t maxNumTransformTracks,
		hkQsTransform* transformTracksOut, uint32_t maxNumFloatTracks, float* floatTracksOut,
		const hkaAnimation::DataChunk* dataChunks, int32_t numDataChunks);

	int32_t m_numberOfPoses;
	int32_t m_blockSize;
	QuantizationFormat m_qFormat;
	uint32_t m_staticMaskIdx;
	uint32_t m_staticDOFsIdx;
	uint32_t m_numStaticTransformDOFs;
	uint32_t m_numDynamicTransformDOFs;
	uint32_t m_blockIndexIdx;
	uint32_t m_blockIndexSize;
	uint32_t m_quantizedDataIdx;
	uint32_t m_quantizedDataSize;
	hkArray<uint8_t> m_dataBuffer;
};

// The cache's chunk for apAnimation's chunk auiChunkId, aiSize * m_blockSize floats, read-locked: retrieved, or
// allocated, write-locked and decompressed by decompressBlockToCache; null when it is write-locked elsewhere or
// allocation fails. Its own "TtcacheW" monitor timer. appCache is the caller's address of its cache argument.
const uint8_t* retrieveFromCache(const hkaWaveletCompressedAnimation* apAnimation, uint32_t auiChunkId, int32_t aiSize,
	hkaChunkCache** appCache);
// Releases the read lock retrieveFromCache took: enterCriticalSection, unlockKeyForRead(getFullCacheKey(auiChunkId),
// aiSize * m_blockSize * 4), then leaveCriticalSection.
void releaseFromCache(int32_t aiSize, const hkaWaveletCompressedAnimation* apAnimation, uint32_t auiChunkId,
	hkaChunkCache** appCache);
double byteSwapFloat(float a1);
int32_t byteSwap32(int32_t r_eax);
int16_t byteSwap16(int32_t r_eax);

// What one block's decompression reads: the quantization of the block and pointers into the animation's data. It is
// passed by value to _decompress.
struct hkWaveletDecompressionParameters {
	uint32_t m_numD;
	int32_t m_blockSize;
	uint8_t m_preserved;
	uint8_t m_maxBitWidth;
	const float* m_offset;
	const float* m_scale;
	const uint8_t* m_bitWidth;
	const uint8_t* m_encodedData;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkaWaveletCompressedAnimationCompressionParamsTypeInfo;
extern hkTypeInfo hkaWaveletCompressedAnimationQuantizationFormatTypeInfo;
extern hkClass hkaWaveletCompressedAnimationCompressionParamsClass;
extern hkClass hkaWaveletCompressedAnimationQuantizationFormatClass;
extern hkClass hkaWaveletCompressedAnimationClass;
extern hkTypeInfo hkaWaveletCompressedAnimationTypeInfo;
