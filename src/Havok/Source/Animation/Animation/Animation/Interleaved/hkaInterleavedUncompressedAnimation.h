#pragma once

#include "Havok/Source/Animation/Animation/Animation/hkaAnimation.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"

// An uncompressed animation: per frame, every transform track's transform, then every float track's value.
class hkaInterleavedUncompressedAnimation : public hkaAnimation {
public:
    hkaInterleavedUncompressedAnimation(hkFinishLoadedObjectFlag aFlag)
      : hkaAnimation(aFlag), m_transforms(aFlag), m_floats(aFlag) {}

    hkaInterleavedUncompressedAnimation(const hkaInterleavedUncompressedAnimation& other);
    ~hkaInterleavedUncompressedAnimation() override;

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
    // The frame count: the transforms per transform track, else the floats per float track.
    int32_t getNumOriginalFrames() const override;
    // One chunk for the header, one more for the transforms and one more for the floats when there are any.
    int32_t getNumDataChunks(uint32_t frame, float delta) const override;
    void getDataChunks(
        uint32_t frame,
        float delta,
        hkaAnimation::DataChunk* dataChunks,
        int32_t numDataChunks
    ) const override;
    // The header's 0x40 bytes, two frames of transforms (0x30 bytes each) and two frames of floats rounded up to 16.
    int32_t getMaxSizeOfCombinedDataChunks() const override;

    void transformTrack(int32_t track, const hkQsTransform& transform);

    // Samples transforms and floats at frameIndex + delta from the chunks getDataChunks fills.
    static void samplePartialWithDataChunks(
        uint32_t frameIndex,
        float delta,
        uint32_t maxNumTransformTracks,
        hkQsTransform* transformTracksOut,
        uint32_t maxNumFloatTracks,
        float* floatTracksOut,
        const hkaAnimation::DataChunk* dataChunks,
        int32_t numDataChunks
    );
    // Havok SDK signature (hkaAnimation.lib, tied by target).
    static const hkClass& staticClass();

    hkArray<hkQsTransform> m_transforms;
    hkArray<float> m_floats;
};

extern hkClass hkaInterleavedUncompressedAnimationClass;

extern hkTypeInfo hkaInterleavedUncompressedAnimationTypeInfo;
