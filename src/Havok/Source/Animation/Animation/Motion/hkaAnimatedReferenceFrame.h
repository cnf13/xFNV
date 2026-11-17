#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"

class hkQsTransform;

// The motion an animation extracts from its root: the reference frame over time.
class hkaAnimatedReferenceFrame : public hkReferencedObject {
public:
	hkaAnimatedReferenceFrame() = default;
	// The SDK's finish constructor: a loaded frame keeps the fields the load wrote.
	hkaAnimatedReferenceFrame(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag) {}

	// The reference frame at time.
	virtual void getReferenceFrame(float time, hkQsTransform& motionOut) const = 0;
	// The change of the reference frame from time to nextTime, wrapping loops times.
	virtual void getDeltaReferenceFrame(float time, float nextTime, int32_t loops,
		hkQsTransform& deltaMotionOut) const = 0;
	// The length of the extracted motion in seconds.
	virtual float getDuration() const = 0;
};

// The class's reflection record.
extern hkClass hkaAnimatedReferenceFrameClass;
