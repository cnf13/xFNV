#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Animation/Animation/Animation/hkaAnnotationTrack.h"
#include "Havok/Source/Animation/Animation/Motion/hkaAnimatedReferenceFrame.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

class hkQsTransform;
class hkaChunkCache;

// The base of Havok's animation formats: the track counts and the sampling interface every format implements. The
// virtuals follow the exe's vtable (slots 3 to 15 after hkReferencedObject's three); the parameters are the Havok
// SDK's, their stack sizes checked against the exe's ret N.
class hkaAnimation : public hkReferencedObject {
public:
	// The animation formats (the PDB's values).
	enum AnimationType : int32_t {
		HK_UNKNOWN_ANIMATION = 0,
		HK_INTERLEAVED_ANIMATION = 1,
		HK_DELTA_COMPRESSED_ANIMATION = 2,
		HK_WAVELET_COMPRESSED_ANIMATION = 3,
		HK_MIRRORED_ANIMATION = 4,
		HK_SPLINE_COMPRESSED_ANIMATION = 5,
	};

	// A block of animation data a format hands out for sampling on another core: the block's start rounded down to
	// 16 bytes, the bytes it spans from there (its offset included, rounded up to 16) and the low four bits of the
	// original start (setDataChunkWith16ByteAlignmentCorrection). getDataChunks fills an array of these at a
	// 12-byte stride: the wavelet's three and a fourth when numDataChunks is 4, the spline's
	// three, and samplePartialWithDataChunks reads m_data + m_offset of the first two chunks and m_data
	// of the third and fourth.
	struct DataChunk {
		// The SDK's inline: points the chunk at dataPtrIn rounded down to 16 bytes and spans numBytes from
		// dataPtrIn. The data are untyped bytes whose address is rounded as an integer. The SDK masks with
		// 0xFFFFFFF0, which on x64 would clear the address's upper half: src masks the low four bits only, the same
		// bits as the exe's 32-bit mask.
		void setDataChunkWith16ByteAlignmentCorrection(const void* dataPtrIn, int32_t numBytes) {
			uintptr_t actualStart = reinterpret_cast<uintptr_t>(dataPtrIn);
			m_offset = static_cast<uint8_t>(actualStart & 0xF);
			actualStart &= ~static_cast<uintptr_t>(0xF);
			m_data = reinterpret_cast<const void*>(actualStart);
			m_size = numBytes;
			m_size += m_offset;
			m_size = (m_size + 15) & ~15u;
		}

		// The SDK's inline (its constructor's zeroing; src's DataChunk keeps no constructor so that arrays of it
		// stay uninitialised as the exe's are).
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

	// The reflection's member table (the PDB's hkaAnimation::Members; the SDK types it hkInternalClassMember, of
	// hkClassMember's layout). The exe's initialiser 0xFC0B20 stores the type member's enum at run time.
	using MembersArray = const hkClassMember[6];

	hkaAnimation(const hkaAnimation& other);
	// The SDK's finish constructor of a loaded object (the formats' finish constructors call it): it keeps the
	// fields the load wrote; the extracted motion is a plain pointer here, which needs no fix-up.
	hkaAnimation(hkFinishLoadedObjectFlag flag) : hkReferencedObject(flag), m_annotationTracks(flag) {}
	~hkaAnimation() override;

	virtual void sampleTracks(float time, hkQsTransform* transformTracksOut, float* floatTracksOut,
		hkaChunkCache* cache) const = 0;

	// The SDK's inline body (HK_ERROR, kept in the release build); the exe's out-of-line copy (0xDFB610) is
	// registered in hkaAnimation.cpp.
	virtual void samplePartialTracks(float time, uint32_t maxNumTransformTracks, hkQsTransform* transformTracksOut,
		uint32_t maxNumFloatTracks, float* floatTracksOut, hkaChunkCache* cache) const {
		FNVD_INLINED_LOCAL(hkErrStream, kStream, std::nullopt, std::nullopt);
		char buf[512];
		::new (&kStream) hkErrStream(buf, 512);
		kStream.operator<<("samplePartialPose not implemented for this type of animation");
		if (hkError::s_instance->message(hkError::MESSAGE_ERROR, 0x54E32123, buf,
				"d:\\Projects\\fallout\\code\\Havok\\Source\\Animation/Animation/Animation/hkaAnimation.inl", 25)) {
			__builtin_debugtrap();
		}
		FNVD_DESTRUCT(hkOstream)(&kStream);
	}

	virtual void clearAllCacheKeys(hkaChunkCache* cache);
	virtual void sampleIndividualTransformTracks(float time, const int16_t* tracks, uint32_t numTracks,
		hkQsTransform* transformOut) const = 0;
	virtual void sampleIndividualFloatTracks(float time, const int16_t* tracks, uint32_t numTracks,
		float* out) const = 0;
	virtual int32_t getNumOriginalFrames() const = 0;
	// The SDK's inline body.
	virtual int32_t getNumDataChunks(uint32_t frame, float delta) const { return 0; }

	// The SDK's inline body (HK_ERROR, kept in the release build); the exe's out-of-line copy (0xDFB670) is
	// registered in hkaAnimation.cpp.
	virtual void getDataChunks(uint32_t frame, float delta, hkaAnimation::DataChunk* dataChunks,
		int32_t numDataChunks) const {
		FNVD_INLINED_LOCAL(hkErrStream, kStream, std::nullopt, std::nullopt);
		char buf[512];
		::new (&kStream) hkErrStream(buf, 512);
		kStream.operator<<("getDataChunks not implemented for this type of animation");
		if (hkError::s_instance->message(hkError::MESSAGE_ERROR, 0x54E32124, buf,
				"d:\\Projects\\fallout\\code\\Havok\\Source\\Animation/Animation/Animation/hkaAnimation.inl", 37)) {
			__builtin_debugtrap();
		}
		FNVD_DESTRUCT(hkOstream)(&kStream);
	}

	// The SDK's inline body.
	virtual int32_t getMaxSizeOfCombinedDataChunks() const { return 0; }

	// The SDK's inline bodies; the exe's out-of-line copies (0xDFB6D0, 0xDFB6F0) are registered in hkaAnimation.cpp.
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

	// The SDK's inline getFrameAndDelta; the exe's out-of-line copy (thiscall, ret 0xC) leaves the frame in eax,
	// which none of its eight callers reads.
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
inline constexpr fnvd::Vtable hkaAnimation_vftable{};

// The class's reflection record.
extern hkClass hkaAnimationClass;
