#include "Havok/Source/Animation/Animation/Animation/Interleaved/hkaInterleavedUncompressedAnimation.h"

#include "Havok/Source/Animation/Animation/Animation/hkaAnimation.h"
#include "Havok/Source/Animation/Animation/Playback/Cache/hkaChunkCache.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "xfnv/Compiler.h"

#include <xmmintrin.h>

static void finishLoadedObjecthkaInterleavedUncompressedAnimation(void* p, int32_t finishing);
// Havok SDK signature (hkaAnimation.lib, tied by reloc,target).
static const void* getVtablehkaInterleavedUncompressedAnimation();
static void cleanupLoadedObjecthkaInterleavedUncompressedAnimation(void* p);

using hkaInterleavedUncompressedAnimationClass_MembersArray = const hkClassMember[2];
static hkaInterleavedUncompressedAnimationClass_MembersArray hkaInterleavedUncompressedAnimationClass_Members{
    {"transforms", nullptr, nullptr, hkClassMember::TYPE_ARRAY, hkClassMember::TYPE_QSTRANSFORM, 0, 0, 0x28, nullptr},
    {"floats", nullptr, nullptr, hkClassMember::TYPE_ARRAY, hkClassMember::TYPE_REAL, 0, 0, 0x34, nullptr}
};
hkClass hkaInterleavedUncompressedAnimationClass(
    "hkaInterleavedUncompressedAnimation",
    &hkaAnimationClass,
    64,
    nullptr,
    0,
    nullptr,
    0,
    hkaInterleavedUncompressedAnimationClass_Members,
    2,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkaInterleavedUncompressedAnimationTypeInfo = {
    "hkaInterleavedUncompressedAnimation",
    finishLoadedObjecthkaInterleavedUncompressedAnimation,
    cleanupLoadedObjecthkaInterleavedUncompressedAnimation,
    getVtablehkaInterleavedUncompressedAnimation(),
    &typeid(hkaInterleavedUncompressedAnimation),
    sizeof(hkaInterleavedUncompressedAnimation)
};

// The type info's finish-loaded-object callback for hkaInterleavedUncompressedAnimation: the null check, then its
// finish constructor.
static void finishLoadedObjecthkaInterleavedUncompressedAnimation(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag kFlag;
        kFlag.m_finishing = finishing;
        ::new (p) hkaInterleavedUncompressedAnimation(kFlag);
    }
}

static const void* getVtablehkaInterleavedUncompressedAnimation() {
    return hkGetVtableOf<hkaInterleavedUncompressedAnimation>();
}

static void cleanupLoadedObjecthkaInterleavedUncompressedAnimation(void* p) {
    static_cast<hkaInterleavedUncompressedAnimation*>(p)->~hkaInterleavedUncompressedAnimation();
}

// Natively: hkaInterleavedUncompressedAnimation(const hkaInterleavedUncompressedAnimation&) below.
hkaInterleavedUncompressedAnimation::hkaInterleavedUncompressedAnimation(
    const hkaInterleavedUncompressedAnimation& other
)
  : hkaAnimation(other) {
    m_transforms.copyFromArray(hkContainerHeapAllocator().get(&m_transforms), other.m_transforms, hkTypeIsPod());
    m_floats.copyFromArray(hkContainerHeapAllocator().get(&m_floats), other.m_floats, hkTypeIsPod());
}

// The float and transform arrays go with their members.
hkaInterleavedUncompressedAnimation::~hkaInterleavedUncompressedAnimation() {}

void hkaInterleavedUncompressedAnimation::sampleTracks(
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

// Past the end of the animation, the last frame's transforms and floats; otherwise the frame's chunks (getDataChunks)
// sampled at its delta. HK_TIMER_BEGIN and HK_TIMER_END enclose both, with the end written once in each branch.
void hkaInterleavedUncompressedAnimation::samplePartialTracks(
    float time,
    uint32_t maxNumTransformTracks,
    hkQsTransform* transformTracksOut,
    uint32_t maxNumFloatTracks,
    float* floatTracksOut,
    hkaChunkCache* cache
) const {
    HK_TIMER_BEGIN("samplePose", nullptr);

    // A NaN time samples.
    if (m_duration <= time) {
        for (uint32_t i = 0; i < maxNumTransformTracks; ++i) {
            const hkQsTransform& rLast = m_transforms.m_data[m_transforms.m_size - m_numberOfTransformTracks + i];
            // Copies the three vectors individually, not through operator=.
            transformTracksOut[i].m_translation = rLast.m_translation;
            transformTracksOut[i].m_rotation.m_vec = rLast.m_rotation.m_vec;
            transformTracksOut[i].m_scale = rLast.m_scale;
        }
        for (uint32_t i = 0; i < maxNumFloatTracks; ++i) {
            floatTracksOut[i] = m_floats.m_data[m_floats.m_size - m_numberOfFloatTracks + i];
        }
    }
    else {
        uint32_t frame;
        float delta;
        getFrameAndDelta(time, frame, delta);
        // The SDK's DataChunk constructor clears each chunk's data, size and offset.
        hkaAnimation::DataChunk dataChunks[3] = {};
        getDataChunks(frame, delta, dataChunks, 3);
        samplePartialWithDataChunks(
            frame,
            delta,
            maxNumTransformTracks,
            transformTracksOut,
            maxNumFloatTracks,
            floatTracksOut,
            dataChunks,
            3
        );
    }

    HK_TIMER_END();
}

// Each listed track's transform between this frame's and the next one's.
void hkaInterleavedUncompressedAnimation::sampleIndividualTransformTracks(
    float time,
    const int16_t* tracks,
    uint32_t numTracks,
    hkQsTransform* transformTracksOut
) const {
    uint32_t frame;
    float delta;
    getFrameAndDelta(time, frame, delta);
    for (uint32_t i = 0; i < numTracks; ++i) {
        const int16_t track = tracks[i];
        if (track >= 0 && track < m_numberOfTransformTracks) {
            const hkQsTransform* frameTransforms = &m_transforms.m_data[frame * m_numberOfTransformTracks];
            // Builds hkSimdReal(delta): delta in the first lane, the others cleared.
            hkSimdReal t;
            t.m_real = _mm_set_ss(delta);
            transformTracksOut[i]
                .setInterpolate4(frameTransforms[track], frameTransforms[track + m_numberOfTransformTracks], t);
        }
    }
}

// Each listed track's float between this frame's value and the next one's.
void hkaInterleavedUncompressedAnimation::sampleIndividualFloatTracks(
    float time,
    const int16_t* tracks,
    uint32_t numTracks,
    float* floatTracksOut
) const {
    uint32_t frame;
    float delta;
    getFrameAndDelta(time, frame, delta);
    const float oneMinusT = 1.0f - delta;
    for (uint32_t i = 0; i < numTracks; ++i) {
        const int16_t track = tracks[i];
        if (track >= 0 && track < m_numberOfFloatTracks) {
            const float* frameFloats = &m_floats.m_data[frame * m_numberOfFloatTracks];
            floatTracksOut[i] = frameFloats[track + m_numberOfFloatTracks] * delta + frameFloats[track] * oneMinusT;
        }
    }
}

// The division is signed.
int32_t hkaInterleavedUncompressedAnimation::getNumOriginalFrames() const {
    if (m_numberOfTransformTracks) {
        return m_transforms.m_size / m_numberOfTransformTracks;
    }
    return m_floats.m_size / m_numberOfFloatTracks;
}

int32_t hkaInterleavedUncompressedAnimation::getNumDataChunks(uint32_t frame, float delta) const {
    int32_t numChunks = 1;
    if (m_numberOfTransformTracks > 0) {
        numChunks = 2;
    }
    if (m_numberOfFloatTracks > 0) {
        ++numChunks;
    }
    return numChunks;
}

// The chunks the frame needs: the animation itself; this frame's and the next one's transforms when there are
// transform tracks; this frame's and the next one's floats, from their start rounded down to 16 bytes, when there
// are float tracks.
void hkaInterleavedUncompressedAnimation::getDataChunks(
    uint32_t frame,
    float delta,
    hkaAnimation::DataChunk* dataChunks,
    int32_t numDataChunks
) const {
    dataChunks[0].m_data = this;
    dataChunks[0].m_size = sizeof(hkaInterleavedUncompressedAnimation);
    int32_t chunk = 1;
    if (m_numberOfTransformTracks > 0) {
        dataChunks[1].m_data = &m_transforms.m_data[frame * m_numberOfTransformTracks];
        dataChunks[1].m_size = static_cast<uint32_t>(2 * m_numberOfTransformTracks * sizeof(hkQsTransform));
        chunk = 2;
    }
    if (m_numberOfFloatTracks > 0) {
        dataChunks[chunk].setDataChunkWith16ByteAlignmentCorrection(
            &m_floats.m_data[frame * m_numberOfFloatTracks],
            static_cast<int32_t>(2 * m_numberOfFloatTracks * sizeof(float))
        );
    }
}

int32_t hkaInterleavedUncompressedAnimation::getMaxSizeOfCombinedDataChunks() const {
    return ((m_numberOfFloatTracks * 8 + 0x1F) & ~0xF) + m_numberOfTransformTracks * 0x60 + 0x40;
}

void hkaInterleavedUncompressedAnimation::transformTrack(int32_t track, const hkQsTransform& transform) {
    int32_t v4 = 0;
    int32_t v15 = m_transforms.m_size / m_numberOfTransformTracks;
    if (v15 > 0) {
        for (const hkQsTransform* i = &transform;; i = &transform) {
            hkQuaternion oldRotation;
            hkQsTransform* m_data = m_transforms.begin();
            int32_t v7 = track + v4 * m_numberOfTransformTracks;
            __m128 m_quad = m_data[v7].m_translation.m_quad;
            __m128* p_m_quad = &m_data[v7].m_translation.m_quad;
            oldRotation.m_vec.m_quad = m_data[v7].m_rotation.m_vec.m_quad;
            __m128 v17 = m_data[v7].m_scale.m_quad;
            __m128 v10 = i->m_rotation.m_vec.m_quad;
            __m128 v11 = _mm_mul_ps(v10, m_quad);
            __m128 v12 = _mm_shuffle_ps(v11, v11, 170);
            XFNV_M128_F32(v12, 0) = XFNV_M128_F32(v12, 0) +
                (XFNV_M128_F32(_mm_shuffle_ps(v11, v11, 85), 0) + XFNV_M128_F32(v11, 0));
            __m128 v13 = _mm_shuffle_ps(v10, v10, 255);
            __m128 v14 = _mm_add_ps(
                _mm_mul_ps(
                    _mm_sub_ps(
                        _mm_mul_ps(_mm_shuffle_ps(v10, v10, 201), _mm_shuffle_ps(m_quad, m_quad, 210)),
                        _mm_mul_ps(_mm_shuffle_ps(v10, v10, 210), _mm_shuffle_ps(m_quad, m_quad, 201))
                    ),
                    v13
                ),
                _mm_add_ps(
                    _mm_mul_ps(_mm_add_ps(_mm_mul_ps(v13, v13), hkQuadRealMinusHalf), m_quad),
                    _mm_mul_ps(_mm_shuffle_ps(v12, v12, 0), v10)
                )
            );
            m_data[v7].m_translation.m_quad = _mm_add_ps(_mm_add_ps(v14, v14), i->m_translation.m_quad);
            m_data[v7].m_rotation.setMul(i->m_rotation, oldRotation);
            ++v4;
            p_m_quad[2] = _mm_mul_ps(transform.m_scale.m_quad, v17);
            if (v4 >= v15) {
                break;
            }
        }
    }
}

// The transforms and floats between frameIndex and the next frame at delta, from the chunks getDataChunks fills: the
// animation, then the two frames of transforms when it has transform tracks, then the two frames of floats (at m_data +
// m_offset) when it has float tracks.
void hkaInterleavedUncompressedAnimation::samplePartialWithDataChunks(
    uint32_t frameIndex,
    float delta,
    uint32_t maxNumTransformTracks,
    hkQsTransform* transformTracksOut,
    uint32_t maxNumFloatTracks,
    float* floatTracksOut,
    const hkaAnimation::DataChunk* dataChunks,
    int32_t numDataChunks
) {
    const hkaInterleavedUncompressedAnimation* anim = static_cast<const hkaInterleavedUncompressedAnimation*>(
        dataChunks[0].m_data
    );
    int32_t chunk = 1;
    if (anim->m_numberOfTransformTracks > 0) {
        const hkQsTransform* transforms0 = static_cast<const hkQsTransform*>(dataChunks[1].m_data);
        const hkQsTransform* transforms1 = transforms0 + anim->m_numberOfTransformTracks;
        for (uint32_t i = 0; i < maxNumTransformTracks; ++i) {
            // Builds hkSimdReal(delta): delta in the first lane, the others cleared.
            hkSimdReal t;
            t.m_real = _mm_set_ss(delta);
            transformTracksOut[i].setInterpolate4(transforms0[i], transforms1[i], t);
        }
        chunk = 2;
    }
    if (anim->m_numberOfFloatTracks > 0) {
        // The chunk is raw bytes: its floats start m_offset bytes past its 16-aligned m_data.
        const float* floats0 = reinterpret_cast<const float*>(
            static_cast<const uint8_t*>(dataChunks[chunk].m_data) + dataChunks[chunk].m_offset
        );
        const float* floats1 = floats0 + anim->m_numberOfFloatTracks;
        const float oneMinusT = 1.0f - delta;
        for (uint32_t i = 0; i < maxNumFloatTracks; ++i) {
            floatTracksOut[i] = floats0[i] * oneMinusT + delta * floats1[i];
        }
    }
}

const hkClass& hkaInterleavedUncompressedAnimation::staticClass() {
    return hkaInterleavedUncompressedAnimationClass;
}
