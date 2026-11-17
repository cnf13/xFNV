#pragma once

#include "Havok/Source/Animation/Animation/Animation/hkaAnnotationTrack.h"
#include "Havok/Source/Animation/Animation/Motion/hkaAnimatedReferenceFrame.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

class hkaChunkCache;
class hkQsTransform;

// The base of Havok's animation formats: the track counts and the sampling interface every format implements.
class hkaAnimation : public hkReferencedObject {
public:
	enum AnimationType : int32_t {
		HK_UNKNOWN_ANIMATION = 0,
		HK_INTERLEAVED_ANIMATION = 1,
		HK_DELTA_COMPRESSED_ANIMATION = 2,
		HK_WAVELET_COMPRESSED_ANIMATION = 3,
		HK_MIRRORED_ANIMATION = 4,
		HK_SPLINE_COMPRESSED_ANIMATION = 5,
	};

	// A block of animation data a format hands out for sampling on another core: the block's start rounded down to 16
	// bytes, the bytes it spans from there (its offset included, rounded up to 16) and the low four bits of the
	// original start (setDataChunkWith16ByteAlignmentCorrection). getDataChunks fills an array of these: the wavelet's
	// three and a fourth when numDataChunks is 4, the spline's three, and samplePartialWithDataChunks reads m_data +
	// m_offset of the first two chunks and m_data of the third and fourth.
	struct DataChunk {
		// Points the chunk at dataPtrIn rounded down to 16 bytes and spans numBytes from dataPtrIn. The data are
		// untyped bytes whose address is rounded as an integer. Only the low four bits are masked, because a 0xFFFFFFF0
		// mask would clear the upper half of the address on x64.
		void setDataChunkWith16ByteAlignmentCorrection(const void* dataPtrIn, int32_t numBytes) {
			uintptr_t actualStart = reinterpret_cast<uintptr_t>(dataPtrIn);
			m_offset = static_cast<uint8_t>(actualStart & 0xF);
			actualStart &= ~static_cast<uintptr_t>(0xF);
			m_data = reinterpret_cast<const void*>(actualStart);
			m_size = numBytes;
			m_size += m_offset;
			m_size = (m_size + 15) & ~15u;
		}

		// Does the zeroing a constructor would do; DataChunk deliberately has no constructor so that arrays of it stay
		// uninitialised, as the game's are.
		void init() {
			m_data = nullptr;
			m_size = 0;
			m_offset = 0;
		}

		const void* m_data;
		uint32_t m_size;
		uint8_t m_offset;
	};

	// One annotation getAnnotations hands out: the track it belongs to and the annotation itself. getAnnotations
	// stores the track index as a word and steps the array 12 bytes at a time.
	struct TrackAnnotation {
		uint16_t m_trackID;
		hkaAnnotationTrack::Annotation m_annotation;
	};

	// The reflection's member table (typed as hkInternalClassMember, which has hkClassMember's layout). The type
	// member's enum is stored at run time.
	using MembersArray = const hkClassMember[6];

	hkaAnimation(const hkaAnimation& other);
	// The SDK's finish constructor of a loaded object (the formats' finish constructors call it): it keeps the
	// fields the load wrote; the extracted motion is a plain pointer here, which needs no fix-up.
	hkaAnimation(hkFinishLoadedObjectFlag flag) : hkReferencedObject(flag), m_annotationTracks(flag) {}
	~hkaAnimation() override;

	virtual void sampleTracks(float time, hkQsTransform* transformTracksOut, float* floatTracksOut,
		hkaChunkCache* cache) const = 0;

	// The inline body raises HK_ERROR, kept in the release build.
	virtual void samplePartialTracks(float time, uint32_t maxNumTransformTracks, hkQsTransform* transformTracksOut,
		uint32_t maxNumFloatTracks, float* floatTracksOut, hkaChunkCache* cache) const {
		char buf[512];
		hkErrStream kStream(buf, 512);
		kStream.operator<<("samplePartialPose not implemented for this type of animation");
		if (hkError::s_instance->message(hkError::MESSAGE_ERROR, 0x54E32123, buf,
				"d:\\Projects\\fallout\\code\\Havok\\Source\\Animation/Animation/Animation/hkaAnimation.inl", 25)) {
			__builtin_debugtrap();
		}
	}

	virtual void clearAllCacheKeys(hkaChunkCache* cache);
	virtual void sampleIndividualTransformTracks(float time, const int16_t* tracks, uint32_t numTracks,
		hkQsTransform* transformOut) const = 0;
	virtual void sampleIndividualFloatTracks(float time, const int16_t* tracks, uint32_t numTracks,
		float* out) const = 0;
	virtual int32_t getNumOriginalFrames() const = 0;
	// The SDK's inline body.
	virtual int32_t getNumDataChunks(uint32_t frame, float delta) const { return 0; }

	// The body raises HK_ERROR, which is kept in the release build.
	virtual void getDataChunks(uint32_t frame, float delta, hkaAnimation::DataChunk* dataChunks,
		int32_t numDataChunks) const {
		char buf[512];
		hkErrStream kStream(buf, 512);
		kStream.operator<<("getDataChunks not implemented for this type of animation");
		if (hkError::s_instance->message(hkError::MESSAGE_ERROR, 0x54E32124, buf,
				"d:\\Projects\\fallout\\code\\Havok\\Source\\Animation/Animation/Animation/hkaAnimation.inl", 37)) {
			__builtin_debugtrap();
		}
	}

	// The SDK's inline body.
	virtual int32_t getMaxSizeOfCombinedDataChunks() const { return 0; }

	virtual void getExtractedMotionReferenceFrame(float time, hkQsTransform& motionOut) const {
		m_extractedMotion->getReferenceFrame(time, motionOut);
	}

	virtual void getExtractedMotionDeltaReferenceFrame(float time, float nextTime, int32_t loops,
		hkQsTransform& deltaMotionOut) const {
		m_extractedMotion->getDeltaReferenceFrame(time, nextTime, loops, deltaMotionOut);
	}

	virtual uint32_t getNumAnnotations(float startTime, float deltaTime) const;
	virtual uint32_t getAnnotations(float startTime, float deltaTime, hkaAnimation::TrackAnnotation* annotations,
		uint32_t maxNumAnnotations) const;

	void getFrameAndDelta(float time, uint32_t& frameOut, float& deltaOut) const;

	static void operator delete(void* apMemory) { hkReferencedObject::deleteObject(apMemory); }

	hkEnum<AnimationType, int32_t> m_type;
	float m_duration;
	int32_t m_numberOfTransformTracks;
	int32_t m_numberOfFloatTracks;
	hkaAnimatedReferenceFrame* m_extractedMotion;
	hkArray<hkaAnnotationTrack> m_annotationTracks;

	static MembersArray Members;
};

// The class's reflection record.
extern hkClass hkaAnimationClass;
