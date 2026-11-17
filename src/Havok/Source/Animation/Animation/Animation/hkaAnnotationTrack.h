#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Reconstruction.hpp"

// The named annotations of one animation track: text attached to points in time. Not polymorphic.
class hkaAnnotationTrack {
public:
	// One annotation: its time and text.
	struct Annotation {
		static const hkClass& staticClass();

		float m_time;
		hkStringPtr m_text;
	};

	hkaAnnotationTrack();
	hkaAnnotationTrack(const hkaAnnotationTrack& other);
	~hkaAnnotationTrack();

	// m_trackName's assignment, then m_annotations' with each annotation's text assigned.
	hkaAnnotationTrack& operator=(const hkaAnnotationTrack& other);

	static const hkClass& staticClass();

	hkStringPtr m_trackName;
	hkArray<Annotation> m_annotations;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkaAnnotationTrackAnnotationTypeInfo;
extern hkTypeInfo hkaAnnotationTrackTypeInfo;
