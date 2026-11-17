#include "Havok/Source/Animation/Animation/Animation/hkaAnimation.h"

#include "Havok/Source/Animation/Animation/Animation/hkaAnnotationTrack.h"
#include "Havok/Source/Animation/Animation/Animation/hkaAnnotationTrackClass.h"
#include "Havok/Source/Animation/Animation/Motion/hkaAnimatedReferenceFrame.h"
#include "Havok/Source/Animation/Animation/Playback/Cache/hkaChunkCache.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"

// The reflection's enum records: the names of AnimationType's values, and the enum.
using hkaAnimationAnimationTypeEnumItemsArray = const hkClassEnum::Item[6];
static hkaAnimationAnimationTypeEnumItemsArray hkaAnimationAnimationTypeEnumItems{
    {0, "HK_UNKNOWN_ANIMATION"},          {1, "HK_INTERLEAVED_ANIMATION"},
    {2, "HK_DELTA_COMPRESSED_ANIMATION"}, {3, "HK_WAVELET_COMPRESSED_ANIMATION"},
    {4, "HK_MIRRORED_ANIMATION"},         {5, "HK_SPLINE_COMPRESSED_ANIMATION"}
};
using hkaAnimationEnumsArray = const hkClassEnum[1];
static hkaAnimationEnumsArray hkaAnimationEnums{{"AnimationType", hkaAnimationAnimationTypeEnumItems, 6, nullptr, {0}}};
static const hkClassEnum* hkaAnimationAnimationTypeEnum = &hkaAnimationEnums[0];
// The reflection's member table.
hkaAnimation::MembersArray hkaAnimation::Members{
    {"type",
     nullptr,
     &hkaAnimationEnums[0],
     {hkClassMember::TYPE_ENUM},
     {hkClassMember::TYPE_INT32},
     0,
     {0},
     0x8,
     nullptr},
    {"duration", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0xC, nullptr},
    {"numberOfTransformTracks",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_INT32},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x10,
     nullptr},
    {"numberOfFloatTracks",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_INT32},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x14,
     nullptr},
    {"extractedMotion",
     &hkaAnimatedReferenceFrameClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x18,
     nullptr},
    {"annotationTracks",
     &hkaAnnotationTrackClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     0x1C,
     nullptr}
};

hkClass hkaAnimationClass(
    "hkaAnimation",
    &hkReferencedObjectClass,
    0x28,
    nullptr,
    0,
    hkaAnimationEnums,
    1,
    hkaAnimation::Members,
    6,
    nullptr,
    nullptr,
    0,
    1
);

// Natively: hkaAnimation(const hkaAnimation&) below.
hkaAnimation::hkaAnimation(const hkaAnimation& other)
  : m_type(other.m_type), m_duration(other.m_duration), m_numberOfTransformTracks(other.m_numberOfTransformTracks),
    m_numberOfFloatTracks(other.m_numberOfFloatTracks), m_extractedMotion(other.m_extractedMotion) {
    if (m_extractedMotion) {
        m_extractedMotion->addReference();
    }
    m_annotationTracks = other.m_annotationTracks;
}

hkaAnimation::~hkaAnimation() {
    if (m_extractedMotion) {
        m_extractedMotion->removeReference();
    }
    m_extractedMotion = nullptr;
}

void hkaAnimation::clearAllCacheKeys(hkaChunkCache* cache) {
    ;
}

// Counts the annotations of every track whose time lies in [startTime, startTime + deltaTime), or, when that range runs
// past the end of the animation, before the wrapped-around end. The end and wrap times are kept as floats.
uint32_t hkaAnimation::getNumAnnotations(float startTime, float deltaTime) const {
    float endTime = startTime + deltaTime;
    float wrapTime = m_duration < endTime ? endTime - m_duration : -3.402820018375656e+38;
    uint32_t numAnnotations = 0;
    for (int32_t i = 0; i < m_annotationTracks.m_size; ++i) {
        const hkaAnnotationTrack& track = m_annotationTracks.m_data[i];
        for (int32_t j = 0; j < track.m_annotations.m_size; ++j) {
            float time = track.m_annotations.m_data[j].m_time;
            if ((time >= startTime && time < endTime) || time < wrapTime) {
                ++numAnnotations;
            }
        }
    }
    return numAnnotations;
}

// Copies up to maxNumAnnotations of the annotations getNumAnnotations counts into annotations, each with its track's
// index, and returns how many it copied.
uint32_t hkaAnimation::getAnnotations(
    float startTime,
    float deltaTime,
    hkaAnimation::TrackAnnotation* annotations,
    uint32_t maxNumAnnotations
) const {
    if (!maxNumAnnotations) {
        return 0;
    }
    float endTime = startTime + deltaTime;
    float wrapTime = m_duration < endTime ? endTime - m_duration : -3.402820018375656e+38;
    uint32_t numAnnotations = 0;
    int32_t numTracks = m_annotationTracks.m_size;
    for (uint16_t i = 0; i < numTracks; ++i) {
        const hkaAnnotationTrack& track = m_annotationTracks.m_data[i];
        int32_t numTrackAnnotations = track.m_annotations.m_size;
        for (int32_t j = 0; j < numTrackAnnotations; ++j) {
            float time = track.m_annotations.m_data[j].m_time;
            if ((startTime <= time && time < endTime) || wrapTime > time) {
                hkaAnimation::TrackAnnotation& out = annotations[numAnnotations];
                out.m_trackID = i;
                out.m_annotation.m_time = track.m_annotations.m_data[j].m_time;
                out.m_annotation.m_text = track.m_annotations.m_data[j].m_text;
                ++numAnnotations;
                if (numAnnotations == maxNumAnnotations) {
                    return numAnnotations;
                }
            }
        }
    }
    return numAnnotations;
}

void hkaAnimation::getFrameAndDelta(float time, uint32_t& frameOut, float& deltaOut) const {
    int32_t v5 = getNumOriginalFrames();
    uint32_t v6 = v5 - 2;
    float a1a = time / m_duration * static_cast<double>(static_cast<uint32_t>(v5 - 1));
    uint32_t result = static_cast<int64_t>(a1a);
    frameOut = result;
    if (result <= v6) {
        deltaOut = a1a - static_cast<double>(result);
    }
    else {
        frameOut = v6;
        deltaOut = 1.0;
    }
}

// Natively: hkaAnnotationTrack(const hkaAnnotationTrack&) below.
hkaAnnotationTrack::hkaAnnotationTrack(const hkaAnnotationTrack& other)
  : m_trackName(other.m_trackName), m_annotations(other.m_annotations) {}

// Assigns m_trackName, then assigns the hkArray<Annotation>: the elements both arrays hold are assigned, the surplus
// texts destroyed, and the rest copy-constructed.
hkaAnnotationTrack& hkaAnnotationTrack::operator=(const hkaAnnotationTrack& other) {
    m_trackName = other.m_trackName;

    // EXE-INLINED none: hkArray<hkaAnnotationTrack::Annotation, hkContainerHeapAllocator>::operator=
    hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
    const int32_t iOldSize = m_annotations.m_size;
    const int32_t iNewSize = other.m_annotations.m_size;
    int32_t iCopied = iOldSize;
    if (iNewSize <= iOldSize) {
        iCopied = iNewSize;
    }
    const int32_t iCapacity = m_annotations.m_capacityAndFlags & hkArrayBase<Annotation>::CAPACITY_MASK;
    if (iCapacity < iNewSize) {
        int32_t iReserve = 2 * iCapacity;
        if (iNewSize >= iReserve) {
            iReserve = iNewSize;
        }
        hkArrayUtil::_reserve(*pHeap, &m_annotations, iReserve, static_cast<int32_t>(sizeof(Annotation)));
    }
    Annotation* pDst = m_annotations.m_data + iNewSize;
    for (int32_t i = iOldSize - iNewSize; i > 0; --i, ++pDst) {
        pDst->m_text.~hkStringPtr();
    }
    const Annotation* pSrc = other.m_annotations.m_data;
    pDst = m_annotations.m_data;
    for (int32_t i = 0; i < iCopied; ++i) {
        float fTime = pSrc[i].m_time;
        pDst[i].m_time = fTime;
        pDst[i].m_text = pSrc[i].m_text;
    }
    pSrc = other.m_annotations.m_data + iCopied;
    pDst = m_annotations.m_data + iCopied;
    for (int32_t i = iNewSize - iCopied; i > 0; --i, ++pSrc, ++pDst) {
        if (pDst) {
            float fTime = pSrc->m_time;
            pDst->m_time = fTime;
            ::new (&pDst->m_text) hkStringPtr(pSrc->m_text);
        }
    }
    m_annotations.m_size = iNewSize;
    return *this;
}
